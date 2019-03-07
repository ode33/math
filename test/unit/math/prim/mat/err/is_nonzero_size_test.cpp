#include <stan/math/prim/mat.hpp>
#include <gtest/gtest.h>
#include <test/unit/util.hpp>
#include <limits>

TEST(ErrorHandlingMatrix, isNonzeroSizeMatrix) {
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> y;
  using stan::math::is_nonzero_size;

  y.resize(3, 3);
  EXPECT_TRUE(is_nonzero_size(y));
  
  y.resize(2, 3);
  EXPECT_TRUE(is_nonzero_size(y));

  y.resize(0, 0);
  EXPECT_FALSE_MSG(is_nonzero_size(y));
}

TEST(ErrorHandlingMatrix, isNonzeroSizeMatrix_nan) {
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> y;
  double nan = std::numeric_limits<double>::quiet_NaN();

  y.resize(3, 3);
  y << nan, nan, nan, nan, nan, nan, nan, nan, nan;
  EXPECT_TRUE(stan::math::is_nonzero_size(y));
  
  y.resize(2, 3);
  y << nan, nan, nan, nan, nan, nan;
  EXPECT_TRUE(stan::math::is_nonzero_size(y));

  y.resize(0, 0);
  EXPECT_FALSE_MSG(stan::math::is_nonzero_size(y));
}