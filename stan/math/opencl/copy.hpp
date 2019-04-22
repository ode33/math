#ifndef STAN_MATH_PRIM_MAT_FUN_OPENCL_COPY_HPP
#define STAN_MATH_PRIM_MAT_FUN_OPENCL_COPY_HPP
#ifdef STAN_OPENCL

#include <stan/math/opencl/opencl_context.hpp>
#include <stan/math/opencl/kernel_cl.hpp>
#include <stan/math/opencl/matrix_cl.hpp>
#include <stan/math/opencl/kernels/copy.hpp>
#include <stan/math/opencl/kernels/pack.hpp>
#include <stan/math/opencl/kernels/unpack.hpp>
#include <stan/math/opencl/buffer_types.hpp>
#include <stan/math/opencl/err/check_opencl.hpp>
#include <stan/math/prim/mat/fun/Eigen.hpp>
#include <stan/math/prim/scal/err/check_size_match.hpp>
#include <stan/math/prim/arr/fun/vec_concat.hpp>

#include <CL/cl.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <type_traits>

namespace stan {
namespace math {

/**
 * Copies the source Eigen matrix to
 * the destination matrix that is stored
 * on the OpenCL device.
 *
 * @tparam T type of data in the Eigen matrix
 * @param dst destination matrix on the OpenCL device
 * @param src source Eigen matrix
 *
 * @throw <code>std::invalid_argument</code> if the
 * matrices do not have matching dimensions
 */
template <int R, int C>
void copy(matrix_cl& dst, const Eigen::Matrix<double, R, C>& src) {
  check_size_match("copy (Eigen -> (OpenCL))", "src.rows()", src.rows(),
                   "dst.rows()", dst.rows());
  check_size_match("copy (Eigen -> (OpenCL))", "src.cols()", src.cols(),
                   "dst.cols()", dst.cols());
  if (src.size() == 0) {
    return;
  }
  try {
    /**
     * Writes the contents of src to the OpenCL buffer
     * starting at the offset 0
     * CL_FALSE denotes that the call is non-blocking
     * This means that future kernels need to know about the copy_event
     * So that they do not execute until this transfer finishes.
     */
    dst.wait_for_read_events();
    dst.clear_read_events();
    dst.wait_for_write_events();
    dst.clear_write_events();
    cl::Event copy_event;
    cl::CommandQueue& queue = opencl_context.queue();
    queue.enqueueWriteBuffer(dst.buffer(), CL_FALSE, 0,
                             sizeof(double) * dst.size(), src.data(), NULL,
                             &copy_event);
    dst.add_write_event(copy_event);
  } catch (const cl::Error& e) {
    check_opencl_error("copy Eigen->(OpenCL)", e);
  }
}

/**
 * Copies the source matrix that is stored
 * on the OpenCL device to the destination Eigen
 * matrix.
 *
 * @tparam T type of data in the Eigen matrix
 * @param dst destination Eigen matrix
 * @param src source matrix on the OpenCL device
 *
 * @throw <code>std::invalid_argument</code> if the
 * matrices do not have matching dimensions
 */
template <int R, int C>
void copy(Eigen::Matrix<double, R, C>& dst, const matrix_cl& src) {
  check_size_match("copy ((OpenCL) -> Eigen)", "src.rows()", src.rows(),
                   "dst.rows()", dst.rows());
  check_size_match("copy ((OpenCL) -> Eigen)", "src.cols()", src.cols(),
                   "dst.cols()", dst.cols());
  if (src.size() == 0) {
    return;
  }
  try {
    /**
     * Reads the contents of the OpenCL buffer
     * starting at the offset 0 to the Eigen
     * matrix
     * CL_TRUE denotes that the call is blocking
     * We do not want to pass data back to the CPU until all of the jobs
     * called on the source matrix are finished.
     */
    cl::CommandQueue queue = opencl_context.queue();
    cl::Event copy_event;
    queue.enqueueReadBuffer(src.buffer(), CL_FALSE, 0,
                            sizeof(double) * dst.size(), dst.data(),
                            &src.write_events(), &copy_event);
    copy_event.wait();
    src.clear_write_events();
  } catch (const cl::Error& e) {
    check_opencl_error("copy (OpenCL)->Eigen", e);
  }
}

/**
 * Packs the flat triagnular matrix on the OpenCL device and
 * copies it to the std::vector.
 *
 * @tparam triangular_view the triangularity of the source matrix
 * @param src the flat triangular source matrix on the OpenCL device
 * @return the packed std::vector
 */
template <TriangularViewCL triangular_view>
inline std::vector<double> packed_copy(const matrix_cl& src) {
  const int packed_size = src.rows() * (src.rows() + 1) / 2;
  std::vector<double> dst(packed_size);
  if (dst.size() == 0) {
    return dst;
  }
  try {
    cl::CommandQueue queue = opencl_context.queue();
    matrix_cl packed(packed_size, 1);
    stan::math::opencl_kernels::pack(cl::NDRange(src.rows(), src.rows()),
                                     packed, src, src.rows(), src.rows(),
                                     triangular_view);
    auto mat_events = vec_concat(packed.read_events(), src.write_events());
    cl::Event copy_event;
    queue.enqueueReadBuffer(packed.buffer(), CL_FALSE, 0,
                            sizeof(double) * packed_size, dst.data(),
                            &mat_events, &copy_event);
    copy_event.wait();
    src.clear_write_events();
    src.clear_read_events();
  } catch (const cl::Error& e) {
    check_opencl_error("packed_copy (OpenCL->std::vector)", e);
  }
  return dst;
}

/**
 * Copies the packed triangular matrix from
 * the source std::vector to an OpenCL buffer and
 * unpacks it to a flat matrix on the OpenCL device.
 *
 * @tparam triangular_view the triangularity of the source matrix
 * @param src the packed source std::vector
 * @param rows the number of rows in the flat matrix
 * @return the destination flat matrix on the OpenCL device
 * @throw <code>std::invalid_argument</code> if the
 * size of the vector does not match the expected size
 * for the packed triangular matrix
 */
template <TriangularViewCL triangular_view>
inline matrix_cl packed_copy(const std::vector<double>& src, int rows) {
  const int packed_size = rows * (rows + 1) / 2;
  check_size_match("copy (packed std::vector -> OpenCL)", "src.size()",
                   src.size(), "rows * (rows + 1) / 2", packed_size);
  matrix_cl dst(rows, rows);
  if (dst.size() == 0) {
    return dst;
  }
  try {
    cl::CommandQueue queue = opencl_context.queue();
    matrix_cl packed(packed_size, 1);
    cl::Event packed_event;
    queue.enqueueWriteBuffer(packed.buffer(), CL_FALSE, 0,
                             sizeof(double) * packed_size, src.data(), NULL,
                             &packed_event);
    packed.add_write_event(packed_event);
    stan::math::opencl_kernels::unpack(cl::NDRange(dst.rows(), dst.rows()), dst,
                                       packed, dst.rows(), dst.rows(),
                                       triangular_view);
  } catch (const cl::Error& e) {
    check_opencl_error("packed_copy (std::vector->OpenCL)", e);
  }
  return dst;
}

/**
 * Copies the source matrix to the
 * destination matrix. Both matrices
 * are stored on the OpenCL device.
 *
 * @param dst destination matrix
 * @param src source matrix
 *
 * @throw <code>std::invalid_argument</code> if the
 * matrices do not have matching dimensions
 */
inline void copy(matrix_cl& dst, const matrix_cl& src) {
  check_size_match("copy ((OpenCL) -> (OpenCL))", "src.rows()", src.rows(),
                   "dst.rows()", dst.rows());
  check_size_match("copy ((OpenCL) -> (OpenCL))", "src.cols()", src.cols(),
                   "dst.cols()", dst.cols());
  if (src.size() == 0) {
    return;
  }
  try {
    /**
     * Copies the contents of the src buffer to the dst buffer
     * see the matrix_cl(matrix_cl&) constructor
     *  for explanation
     */
    cl::CommandQueue queue = opencl_context.queue();
    auto mat_events = vec_concat(dst.read_events(), src.write_events());
    cl::Event copy_event;
    queue.enqueueCopyBuffer(src.buffer(), dst.buffer(), 0, 0,
                            sizeof(double) * src.size(), &mat_events,
                            &copy_event);
    dst.add_write_event(copy_event);
    src.add_read_event(copy_event);
  } catch (const cl::Error& e) {
    check_opencl_error("copy (OpenCL)->(OpenCL)", e);
  }
}

/**
 * Copy A 1 by 1 source matrix from the Device to  the host.
 * @tparam An arithmetic type to pass the value from the OpenCL matrix to.
 * @param dst Arithmetic to receive the matrix_cl value.
 * @param src A 1x1 matrix on the device.
 */
template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
inline void copy(T& dst, const matrix_cl& src) {
  check_size_match("copy ((OpenCL) -> (OpenCL))", "src.rows()", src.rows(),
                   "dst.rows()", 1);
  check_size_match("copy ((OpenCL) -> (OpenCL))", "src.cols()", src.cols(),
                   "dst.cols()", 1);
  try {
    cl::CommandQueue queue = opencl_context.queue();
    cl::Event copy_event;
    queue.enqueueReadBuffer(src.buffer(), CL_FALSE, 0, sizeof(int), &dst,
                            &src.write_events(), &copy_event);
    copy_event.wait();
    src.clear_write_events();
  } catch (const cl::Error& e) {
    check_opencl_error("copy (OpenCL)->(OpenCL)", e);
  }
}

/**
 * Copy an arithmetic type to the device.
 * @tparam An arithmetic type to pass the value from the OpenCL matrix to.
 * @param src Arithmetic to receive the matrix_cl value.
 * @param dst A 1x1 matrix on the device.
 */
template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
inline void copy(matrix_cl& dst, const T& src) {
  check_size_match("copy ((OpenCL) -> (OpenCL))", "src.rows()", dst.rows(),
                   "dst.rows()", 1);
  check_size_match("copy ((OpenCL) -> (OpenCL))", "src.cols()", dst.cols(),
                   "dst.cols()", 1);
  try {
    dst.wait_for_read_events();
    dst.clear_read_events();
    cl::CommandQueue queue = opencl_context.queue();
    cl::Event copy_event;
    queue.enqueueWriteBuffer(dst.buffer(), CL_FALSE, 0, sizeof(T), &src, NULL,
                             &copy_event);
    dst.add_write_event(copy_event);
  } catch (const cl::Error& e) {
    check_opencl_error("copy (OpenCL)->(OpenCL)", e);
  }
}

}  // namespace math
}  // namespace stan
#endif
#endif
