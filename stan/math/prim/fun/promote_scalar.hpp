#ifndef STAN_MATH_PRIM_FUN_PROMOTE_SCALAR_HPP
#define STAN_MATH_PRIM_FUN_PROMOTE_SCALAR_HPP

#include <stan/math/prim/fun/promote_scalar_type.hpp>
#include <stan/math/prim/meta/index_type.hpp>
#include <stan/math/prim/fun/promote_scalar.hpp>
#include <vector>
#include <stan/math/prim/fun/Eigen.hpp>

namespace stan {
namespace math {

/**
 * General struct to hold static function for promoting underlying
 * scalar types.
 *
 * @tparam T return type of nested static function.
 * @tparam S input type for nested static function, whose underlying
 * scalar type must be assignable to T.
 */
template <typename T, typename S>
struct promote_scalar_struct {
  /**
   * Return the value of the input argument promoted to the type
   * specified by the template parameter.
   *
   * This is the base case for mismatching template parameter
   * types in which the underlying scalar type of template
   * parameter <code>S</code> is assignable to type <code>T</code>.
   *
   * @param x input of type S.
   * @return input promoted to have scalars of type T.
   */
  static T apply(S x) { return T(x); }
};

/**
 * Struct to hold static function for promoting underlying scalar
 * types.  This specialization is for equal input and output types
 * of function types.
 *
 * @tparam T input and return type of nested static function.
 */
template <typename T>
struct promote_scalar_struct<T, T> {
  /**
   * Return the unmodified input.
   *
   * @param x input of type T.
   * @return input unmodified.
   */
  static T apply(const T& x) { return x; }
};

/**
 * This is the top-level function to call to promote the scalar
 * types of an input of type S to type T.
 *
 * @tparam T scalar type of output.
 * @tparam S input type.
 * @param x input vector.
 * @return input vector with scalars promoted to type T.
 */
template <typename T, typename S>
typename promote_scalar_type<T, S>::type promote_scalar(const S& x) {
  return promote_scalar_struct<T, S>::apply(x);
}

}  // namespace math
}  // namespace stan

namespace stan {
namespace math {

/**
 * Struct to hold static function for promoting underlying scalar
 * types.  This specialization is for standard vector inputs.
 *
 * @tparam T return scalar type
 * @tparam S input type for standard vector elements in static
 * nested function, which must have an underlying scalar type
 * assignable to T.
 */
template <typename T, typename S>
struct promote_scalar_struct<T, std::vector<S> > {
  /**
   * Return the standard vector consisting of the recursive
   * promotion of the elements of the input standard vector to the
   * scalar type specified by the return template parameter.
   *
   * @param x input standard vector.
   * @return standard vector with values promoted from input vector.
   */
  static std::vector<typename promote_scalar_type<T, S>::type> apply(
      const std::vector<S>& x) {
    typedef std::vector<typename promote_scalar_type<T, S>::type> return_t;
    typedef typename index_type<return_t>::type idx_t;
    return_t y(x.size());
    for (idx_t i = 0; i < x.size(); ++i)
      y[i] = promote_scalar_struct<T, S>::apply(x[i]);
    return y;
  }
};

}  // namespace math
}  // namespace stan

namespace stan {
namespace math {

/**
 * Struct to hold static function for promoting underlying scalar
 * types.  This specialization is for Eigen matrix inputs.
 *
 * @tparam T return scalar type
 * @tparam S input matrix scalar type for static nested function, which must
 * have a scalar type assignable to T
 */
template <typename T, typename S>
struct promote_scalar_struct<T, Eigen::Matrix<S, -1, -1> > {
  /**
   * Return the matrix consisting of the recursive promotion of
   * the elements of the input matrix to the scalar type specified
   * by the return template parameter.
   *
   * @param x input matrix.
   * @return matrix with values promoted from input vector.
   */
  static Eigen::Matrix<typename promote_scalar_type<T, S>::type, -1, -1> apply(
      const Eigen::Matrix<S, -1, -1>& x) {
    Eigen::Matrix<typename promote_scalar_type<T, S>::type, -1, -1> y(x.rows(),
                                                                      x.cols());
    for (int i = 0; i < x.size(); ++i)
      y(i) = promote_scalar_struct<T, S>::apply(x(i));
    return y;
  }
};

/**
 * Struct to hold static function for promoting underlying scalar
 * types.  This specialization is for Eigen column vector inputs.
 *
 * @tparam T return scalar type
 * @tparam S input matrix scalar type for static nested function, which must
 * have a scalar type assignable to T
 */
template <typename T, typename S>
struct promote_scalar_struct<T, Eigen::Matrix<S, 1, -1> > {
  /**
   * Return the column vector consisting of the recursive promotion of
   * the elements of the input column vector to the scalar type specified
   * by the return template parameter.
   *
   * @param x input column vector.
   * @return column vector with values promoted from input vector.
   */
  static Eigen::Matrix<typename promote_scalar_type<T, S>::type, 1, -1> apply(
      const Eigen::Matrix<S, 1, -1>& x) {
    Eigen::Matrix<typename promote_scalar_type<T, S>::type, 1, -1> y(x.rows(),
                                                                     x.cols());
    for (int i = 0; i < x.size(); ++i)
      y(i) = promote_scalar_struct<T, S>::apply(x(i));
    return y;
  }
};

/**
 * Struct to hold static function for promoting underlying scalar
 * types.  This specialization is for Eigen row vector inputs.
 *
 * @tparam T return scalar type
 * @tparam S input matrix scalar type for static nested function, which must
 * have a scalar type assignable to T
 */
template <typename T, typename S>
struct promote_scalar_struct<T, Eigen::Matrix<S, -1, 1> > {
  /**
   * Return the row vector consisting of the recursive promotion of
   * the elements of the input row vector to the scalar type specified
   * by the return template parameter.
   *
   * @param x input row vector.
   * @return row vector with values promoted from input vector.
   */
  static Eigen::Matrix<typename promote_scalar_type<T, S>::type, -1, 1> apply(
      const Eigen::Matrix<S, -1, 1>& x) {
    Eigen::Matrix<typename promote_scalar_type<T, S>::type, -1, 1> y(x.rows(),
                                                                     x.cols());
    for (int i = 0; i < x.size(); ++i)
      y(i) = promote_scalar_struct<T, S>::apply(x(i));
    return y;
  }
};

}  // namespace math
}  // namespace stan

#endif
