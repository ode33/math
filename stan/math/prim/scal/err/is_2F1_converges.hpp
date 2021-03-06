#ifndef STAN_MATH_PRIM_SCAL_ERR_IS_2F1_CONVERGES_HPP
#define STAN_MATH_PRIM_SCAL_ERR_IS_2F1_CONVERGES_HPP

#include <stan/math/prim/scal/err/is_not_nan.hpp>
#include <stan/math/prim/scal/fun/is_nonpositive_integer.hpp>
#include <stan/math/prim/scal/fun/value_of_rec.hpp>
#include <cmath>
#include <limits>

namespace stan {
namespace math {

/**
 * Check if the hypergeometric function (2F1) called with
 * supplied arguments will converge, assuming arguments are
 * finite values.
 * @tparam T_a1 Type of <code>a1</code>
 * @tparam T_a2 Type of <code>a2</code>
 * @tparam T_b1 Type of <code>b1</code>
 * @tparam T_z Type of <code>z</code>
 * @param a1 Variable to check
 * @param a2 Variable to check
 * @param b1 Variable to check
 * @param z Variable to check
 * @return <code>true</code> if <code>2F1(a1, a2, b1, z)</code>
 *   does meets convergence conditions, and if no coefficient is NaN
 */
template <typename T_a1, typename T_a2, typename T_b1, typename T_z>
inline bool is_2F1_converges(const T_a1& a1, const T_a2& a2, const T_b1& b1,
                             const T_z& z) {
  using std::fabs;
  using std::floor;

  if (!(is_not_nan(a1) || is_not_nan(a2) || is_not_nan(b1) || is_not_nan(z)))
    return false;

  int num_terms = 0;
  bool is_polynomial = false;

  if (is_nonpositive_integer(a1) && fabs(a1) >= num_terms) {
    is_polynomial = true;
    num_terms = floor(fabs(value_of_rec(a1)));
  }
  if (is_nonpositive_integer(a2) && fabs(a2) >= num_terms) {
    is_polynomial = true;
    num_terms = floor(fabs(value_of_rec(a2)));
  }

  bool is_undefined = is_nonpositive_integer(b1) && fabs(b1) <= num_terms;

  if (is_undefined
      || !(is_polynomial || fabs(z) < 1 || (fabs(z) == 1 && b1 > a1 + a2)))
    return false;

  return true;
}

}  // namespace math
}  // namespace stan
#endif
