
#ifdef STAN_OPENCL
#include <stan/math/prim.hpp>
#include <stan/math/opencl/copy.hpp>
#include <stan/math/opencl/matrix_cl.hpp>
#include <test/unit/util.hpp>
#include <gtest/gtest.h>
#include <algorithm>

TEST(MathMatrixCL, rep_v_exception_pass) {
  stan::math::matrix_cl<double> a(2, 2);
  EXPECT_THROW(stan::math::rep_vector(a, 5), std::invalid_argument);
  EXPECT_THROW(stan::math::rep_vector(a, -1), std::domain_error);
  stan::math::matrix_cl<double> b(1, 1);
  EXPECT_THROW(stan::math::rep_vector(b, -1), std::domain_error);
  stan::math::matrix_cl<double> c(1, 0);
  EXPECT_THROW(stan::math::rep_vector(c, 5), std::invalid_argument);

  EXPECT_NO_THROW(stan::math::rep_vector(b, 0));
  EXPECT_THROW(stan::math::rep_vector(a, 0), std::invalid_argument);
  EXPECT_THROW(stan::math::rep_vector(c, 0), std::invalid_argument);
  EXPECT_NO_THROW(stan::math::rep_vector(b, 1));
}

TEST(MathMatrixCL, rep_v_value_check) {
  double val = -5.0;
  stan::math::matrix_cl<double> m0_cl(val);

  stan::math::matrix_d m1 = stan::math::rep_vector(val, 1);
  stan::math::matrix_cl<double> m1_cl = stan::math::rep_vector(m0_cl, 1);
  stan::math::matrix_d m1_cl_res = stan::math::from_matrix_cl(m1_cl);
  EXPECT_MATRIX_FLOAT_EQ(m1, m1_cl_res);

  double val00 = -7.0;
  stan::math::matrix_cl<double> m00_cl(val00);

  stan::math::matrix_d m2 = stan::math::rep_vector(val00, 7);
  stan::math::matrix_cl<double> m2_cl = stan::math::rep_vector(m00_cl, 7);
  stan::math::matrix_d m2_cl_res = stan::math::from_matrix_cl(m2_cl);
  EXPECT_MATRIX_FLOAT_EQ(m2, m2_cl_res);
}
#endif
