#ifndef STAN_MATH_PRIM_ERR_CHECK_FINITE_HPP
#define STAN_MATH_PRIM_ERR_CHECK_FINITE_HPP

#include <stan/math/prim/err/domain_error.hpp>
#include <stan/math/prim/err/domain_error_vec.hpp>
#include <stan/math/prim/meta/length.hpp>
#include <stan/math/prim/meta/is_vector_like.hpp>
#include <stan/math/prim/fun/value_of_rec.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <stan/math/prim/err/check_finite.hpp>
#include <stan/math/prim/fun/value_of.hpp>
#include <Eigen/Dense>













namespace stan {
namespace math {

namespace {
template <typename T_y, bool is_vec>
struct finite {
  static void check(const char* function, const char* name, const T_y& y) {
    if (!(boost::math::isfinite)(value_of_rec(y)))
      domain_error(function, name, y, "is ", ", but must be finite!");
  }
};

template <typename T_y>
struct finite<T_y, true> {
  static void check(const char* function, const char* name, const T_y& y) {
    using stan::length;
    for (size_t n = 0; n < length(y); n++) {
      if (!(boost::math::isfinite)(value_of_rec(stan::get(y, n))))
        domain_error_vec(function, name, y, n, "is ", ", but must be finite!");
    }
  }
};
}  // namespace

/**
 * Check if <code>y</code> is finite.
 *
 * This function is vectorized and will check each element of
 * <code>y</code>.
 *
 * @tparam T_y Type of y
 *
 * @param function Function name (for error messages)
 * @param name Variable name (for error messages)
 * @param y Variable to check
 *
 * @throw <code>domain_error</code> if y is infinity, -infinity, or
 *   NaN.
 */
template <typename T_y>
inline void check_finite(const char* function, const char* name, const T_y& y) {
  finite<T_y, is_vector_like<T_y>::value>::check(function, name, y);
}
}  // namespace math
}  // namespace stan










namespace stan {
namespace math {
namespace {
template <typename T, int R, int C>
struct finite<Eigen::Matrix<T, R, C>, true> {
  static void check(const char* function, const char* name,
                    const Eigen::Matrix<T, R, C>& y) {
    if (!value_of(y).allFinite()) {
      for (int n = 0; n < y.size(); ++n) {
        if (!(boost::math::isfinite)(y(n)))
          domain_error_vec(function, name, y, n, "is ",
                           ", but must be finite!");
      }
    }
  }
};
}  // namespace
}  // namespace math
}  // namespace stan

#endif