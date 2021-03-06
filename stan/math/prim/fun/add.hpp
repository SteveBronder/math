#ifndef STAN_MATH_PRIM_FUN_ADD_HPP
#define STAN_MATH_PRIM_FUN_ADD_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/err.hpp>
#include <stan/math/prim/fun/Eigen.hpp>

namespace stan {
namespace math {

/**
 * Return the sum of the specified matrices.  The two matrices
 * must have the same dimensions.
 *
 * @tparam Mat1 type of the first matrix or expression
 * @tparam Mat2 type of the second matrix or expression
 *
 * @param m1 First matrix or expression.
 * @param m2 Second matrix or expression.
 * @return Sum of the matrices.
 * @throw std::invalid_argument if m1 and m2 do not have the same
 * dimensions.
 */
template <typename Mat1, typename Mat2,
          typename = require_all_eigen_t<Mat1, Mat2>>
inline auto add(const Mat1& m1, const Mat2& m2) {
  check_matching_dims("add", "m1", m1, "m2", m2);
  return (m1 + m2).eval();
}

/**
 * Return the sum of the specified matrix and specified scalar.
 *
 * @tparam Mat type of the matrix or expression
 * @tparam Scal type of the scalar
 * @param m Matrix or expression.
 * @param c Scalar.
 * @return The matrix plus the scalar.
 */
template <typename Mat, typename Scal, typename = require_eigen_t<Mat>,
          typename = require_stan_scalar_t<Scal>>
inline auto add(const Mat& m, const Scal c) {
  return (m.array() + c).matrix().eval();
}

/**
 * Return the sum of the specified scalar and specified matrix.
 *
 * @tparam Scal type of the scalar
 * @tparam Mat type of the matrix or expression
 * @param c Scalar.
 * @param m Matrix.
 * @return The scalar plus the matrix.
 */
template <typename Scal, typename Mat, typename = require_stan_scalar_t<Scal>,
          typename = require_eigen_t<Mat>>
inline auto add(const Scal c, const Mat& m) {
  return (c + m.array()).matrix().eval();
}

}  // namespace math
}  // namespace stan

#endif
