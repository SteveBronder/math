#ifndef STAN_MATH_OPENCL_GP_EXP_QUAD_COV_HPP
#define STAN_MATH_OPENCL_GP_EXP_QUAD_COV_HPP
#ifdef STAN_OPENCL

#include <stan/math/opencl/matrix_cl.hpp>
#include <stan/math/opencl/kernels/gp_exp_quad_cov.hpp>
#include <stan/math/opencl/err/check_matching_dims.hpp>
#include <stan/math/prim/meta.hpp>
#include <CL/cl.hpp>

namespace stan {
namespace math {
/**
 * Squared exponential kernel on the GPU.
 *
 * @param x input vector or matrix
 * @param sigma standard deviation
 * @param length_scale length scale
 *
 * @return Squared distance between elements of x.
 */
template <typename T1, typename T2, typename T3,
          typename = enable_if_all_arithmetic<T1, T2, T3>>
inline matrix_cl<return_type_t<T1, T2, T3>> gp_exp_quad_cov(
    const matrix_cl<T1>& x, const T2 sigma, const T3 length_scale) try {
  matrix_cl<return_type_t<T1, T2, T3>> res(x.cols(), x.cols());
  opencl_kernels::gp_exp_quad_cov(cl::NDRange(x.cols(), x.cols()), x, res,
                                  sigma * sigma, -0.5 / square(length_scale),
                                  x.cols(), x.rows());
  return res;
} catch (const cl::Error& e) {
  check_opencl_error("gp_exp_quad_cov", e);
  // check above causes termination so below will not happen
  matrix_cl<return_type_t<T1, T2, T3>> res(x.cols(), x.cols());
  return res;
}

/**
 * Squared exponential kernel on the GPU.
 *
 * This function is for the cross covariance
 * matrix needed to compute the posterior predictive density.
 *
 * @param x first input vector or matrix
 * @param y second input vector or matrix
 * @param sigma standard deviation
 * @param length_scale length scale
 *
 * @return Squared distance between elements of x and y.
 */
template <typename T1, typename T2, typename T3, typename T4,
          typename = enable_if_all_arithmetic<T1, T2, T3, T4>>
inline matrix_cl<return_type_t<T1, T2, T3, T4>> gp_exp_quad_cov(
    const matrix_cl<T1>& x, const matrix_cl<T2>& y, const T3 sigma,
    const T4 length_scale) try {
  check_size_match("gp_exp_quad_cov_cross", "x", x.rows(), "y", y.rows());
  matrix_cl<return_type_t<T1, T2, T3, T4>> res(x.cols(), y.cols());
  opencl_kernels::gp_exp_quad_cov_cross(
      cl::NDRange(x.cols(), y.cols()), x, y, res, sigma * sigma,
      -0.5 / square(length_scale), x.cols(), y.cols(), x.rows());
  return res;
} catch (const cl::Error& e) {
  check_opencl_error("gp_exp_quad_cov_cross", e);
  // check above causes termination so below will not happen
  matrix_cl<return_type_t<T1, T2, T3, T4>> res(x.cols(), x.cols());
  return res;
}

}  // namespace math
}  // namespace stan

#endif
#endif
