
#include <stan/math/rev.hpp>
#include <gtest/gtest.h>

#include <stan/math/prim/meta/is_constant.hpp>
#include <vector>








TEST(MetaTraits, containsNonconstantStruct) {
  using stan::contains_nonconstant_struct;

  EXPECT_TRUE(contains_nonconstant_struct<stan::math::var>::value);
}




TEST(MetaTraits_arr, containsNonconstantStruct) {
  using stan::contains_nonconstant_struct;
  using std::vector;

  EXPECT_TRUE(contains_nonconstant_struct<stan::math::var>::value);
  EXPECT_TRUE(contains_nonconstant_struct<vector<stan::math::var> >::value);
  EXPECT_TRUE(
      contains_nonconstant_struct<vector<vector<stan::math::var> > >::value);
  EXPECT_TRUE(contains_nonconstant_struct<
              vector<vector<vector<stan::math::var> > > >::value);
}





using stan::length;

typedef Eigen::Matrix<stan::math::var, Eigen::Dynamic, Eigen::Dynamic> var_t1;
typedef std::vector<var_t1> var_t2;
typedef std::vector<var_t2> var_t3;

typedef Eigen::Matrix<stan::math::var, Eigen::Dynamic, 1> var_u1;
typedef std::vector<var_u1> var_u2;
typedef std::vector<var_u2> var_u3;

typedef Eigen::Matrix<stan::math::var, 1, Eigen::Dynamic> var_v1;
typedef std::vector<var_v1> var_v2;
typedef std::vector<var_v2> var_v3;

TEST(MetaTraits_mat, containsNonconstantStruct) {
  using Eigen::Dynamic;
  using Eigen::Matrix;
  using stan::contains_nonconstant_struct;
  using std::vector;
  std::cout << stan::is_nonconstant_struct<double>::value << std::endl;
  EXPECT_TRUE(contains_nonconstant_struct<var_t1>::value);
  EXPECT_TRUE(contains_nonconstant_struct<var_t2>::value);
  EXPECT_TRUE(contains_nonconstant_struct<var_t3>::value);
  EXPECT_TRUE(contains_nonconstant_struct<var_u1>::value);
  EXPECT_TRUE(contains_nonconstant_struct<var_u2>::value);
  EXPECT_TRUE(contains_nonconstant_struct<var_u3>::value);
  EXPECT_TRUE(contains_nonconstant_struct<var_v1>::value);
  EXPECT_TRUE(contains_nonconstant_struct<var_v2>::value);
  EXPECT_TRUE(contains_nonconstant_struct<var_v3>::value);

  bool temp
      = contains_nonconstant_struct<var_v3, var_v2, var_v1, double, int>::value;
  EXPECT_TRUE(temp);
}