#ifndef STAN_MATH_PRIM_FUN_LGAMMA_STIRLING_DIFF_HPP
#define STAN_MATH_PRIM_FUN_LGAMMA_STIRLING_DIFF_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/err.hpp>
#include <stan/math/prim/fun/constants.hpp>
#include <stan/math/prim/fun/inv.hpp>
#include <stan/math/prim/fun/lgamma.hpp>
#include <stan/math/prim/fun/lgamma_stirling.hpp>
#include <stan/math/prim/fun/square.hpp>
#include <stan/math/prim/fun/value_of.hpp>
#include <cmath>

namespace stan {
namespace math {

constexpr double lgamma_stirling_diff_useful = 10;

/**
 * Return the difference between log of the gamma function and its Stirling
 * approximation.
 * This is useful to stably compute log of ratios of gamma functions with large
 * arguments where the Stirling approximation allows for analytic solution
 * and the (small) differences can be added afterwards.
 * This is for example used in the implementation of lbeta.
 *
 * The function will return correct value for all arguments, but using it can 
 * lead to a loss of precision when x < lgamma_stirling_diff_useful.
 *
   \f[
   \mbox{lgamma_stirling_diff}(x) =
    \log(\Gamma(x)) - \frac{1}{2} \log(2\pi) +
    (x-\frac{1}{2})*\log(x) - x
   \f]

 *
 * @tparam T Type of value.
 * @param x value
 * @return Difference between lgamma(x) and its Stirling approximation.
 */

template <typename T>
return_type_t<T> lgamma_stirling_diff(const T x) {
  if (is_nan(value_of_rec(x))) {
    return NOT_A_NUMBER;
  }
  typedef return_type_t<T> T_Ret;

  static const char* function = "lgamma_stirling_diff";
  check_nonnegative(function, "argument", x);

  if (x == 0) {
    return INFTY;
  } 
  if (value_of(x) < lgamma_stirling_diff_useful) {
    return lgamma(x) - lgamma_stirling(x);
  } 

  // Using the Stirling series as expressed in formula 5.11.1. at
  // https://dlmf.nist.gov/5.11
  constexpr double stirling_series[] {
      0.0833333333333333333333333,
      -0.00277777777777777777777778,
      0.000793650793650793650793651,
      -0.000595238095238095238095238, //currently not used
  };
  constexpr int n_stirling_terms = 3;
  T_Ret inv_x = inv(x);
  T_Ret inv_x_squared = square(inv_x);
  T_Ret inv_x_cubed = inv_x * inv_x_squared;
  T_Ret inv_x_fifth = inv_x_cubed * inv_x_squared;
  return stirling_series[0] * inv_x + stirling_series[1] * inv_x_cubed
          + stirling_series[2] * inv_x_fifth;
}

}  // namespace math
}  // namespace stan

#endif
