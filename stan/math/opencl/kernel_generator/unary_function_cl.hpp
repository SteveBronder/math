#ifndef STAN_MATH_OPENCL_KERNEL_GENERATOR_UNARY_FUNCTION_CL_HPP
#define STAN_MATH_OPENCL_KERNEL_GENERATOR_UNARY_FUNCTION_CL_HPP
#ifdef STAN_OPENCL

#include <stan/math/prim/meta.hpp>
#include <stan/math/opencl/err.hpp>
#include <stan/math/opencl/kernels/device_functions/digamma.hpp>
#include <stan/math/opencl/kernels/device_functions/log1m_exp.hpp>
#include <stan/math/opencl/kernels/device_functions/log1p_exp.hpp>
#include <stan/math/opencl/matrix_cl_view.hpp>
#include <stan/math/opencl/kernel_generator/type_str.hpp>
#include <stan/math/opencl/kernel_generator/name_generator.hpp>
#include <stan/math/opencl/kernel_generator/operation_cl.hpp>
#include <stan/math/opencl/kernel_generator/as_operation_cl.hpp>
#include <stan/math/opencl/kernel_generator/is_valid_expression.hpp>
#include <string>
#include <type_traits>
#include <set>
#include <utility>

namespace stan {
namespace math {

/** \addtogroup opencl_kernel_generator
 *  @{
 */

/**
 * Represents a unary function in kernel generator expressions.
 * @tparam Derived derived type
 * @tparam T type of argument
 */
template <typename Derived, typename T>
class unary_function_cl
    : public operation_cl<Derived, typename std::remove_reference_t<T>::Scalar,
                          T> {
 public:
  using Scalar = typename std::remove_reference_t<T>::Scalar;
  static_assert(std::is_floating_point<Scalar>::value,
                "unary_function_cl: argument must be expression with floating "
                "point return type!");
  using base = operation_cl<Derived, Scalar, T>;
  using base::var_name;

  /**
   * Constructor
   * @param a argument expression
   * @param fun function
   */
  unary_function_cl(T&& a, const std::string& fun)
      : base(std::forward<T>(a)), fun_(fun) {}

  /**
   * generates kernel code for this expression.
   * @param i row index variable name
   * @param j column index variable name
   * @param view_handled whether whether caller already handled matrix view
   * @param var_name_arg variable name of the nested expression
   * @return part of kernel with code for this expression
   */
  inline kernel_parts generate(const std::string& i, const std::string& j,
                               const bool view_handled,
                               const std::string& var_name_arg) const {
    kernel_parts res{};
    res.includes = base::derived().include;
    res.body = type_str<Scalar>() + " " + var_name + " = " + fun_ + "("
               + var_name_arg + ");\n";
    return res;
  }

 protected:
  std::string fun_;
};

/**
 * generates a class and function for a general unary function that is defined
 * by OpenCL.
 * @param fun function
 * @param incl function source to include into kernel
 */
#define ADD_UNARY_FUNCTION_WITH_INCLUDE(fun, incl)                            \
  template <typename T>                                                       \
  class fun##_ : public unary_function_cl<fun##_<T>, T> {                     \
    using base = unary_function_cl<fun##_<T>, T>;                             \
    using base::arguments_;                                                   \
                                                                              \
   public:                                                                    \
    using base::rows;                                                         \
    using base::cols;                                                         \
    static const char* include;                                               \
    explicit fun##_(T&& a) : base(std::forward<T>(a), #fun) {}                \
    inline auto deep_copy() const {                                           \
      auto&& arg_copy = this->template get_arg<0>().deep_copy();              \
      return fun##_<std::remove_reference_t<decltype(arg_copy)>>{             \
          std::move(arg_copy)};                                               \
    }                                                                         \
    inline std::pair<int, int> extreme_diagonals() const {                    \
      return {-rows() + 1, cols() - 1};                                       \
    }                                                                         \
  };                                                                          \
                                                                              \
  template <typename T, typename Cond                                         \
                        = require_all_valid_expressions_and_none_scalar_t<T>> \
  inline fun##_<as_operation_cl_t<T>> fun(T&& a) {                            \
    return fun##_<as_operation_cl_t<T>>(as_operation_cl(std::forward<T>(a))); \
  }                                                                           \
  template <typename T>                                                       \
  const char* fun##_<T>::include(incl);

/**
 * generates a class and function for a general unary function that is defined
 * by OpenCL.
 * @param fun function
 */
#define ADD_UNARY_FUNCTION(fun) ADD_UNARY_FUNCTION_WITH_INCLUDE(fun, "")

/**
 * generates a class and function for an unary function, defined by OpenCL with
 * special property that it passes trough zero. That is \f$ f(0)=0 \f$. Such a
 * function can have triangular view equal to its argument's.
 * @param fun function name
 */
#define ADD_UNARY_FUNCTION_PASS_ZERO(fun)                                     \
  template <typename T>                                                       \
  class fun##_ : public unary_function_cl<fun##_<T>, T> {                     \
    using base = unary_function_cl<fun##_<T>, T>;                             \
    using base::arguments_;                                                   \
                                                                              \
   public:                                                                    \
    using base::rows;                                                         \
    using base::cols;                                                         \
    static constexpr auto view_transitivness = std::make_tuple(true);         \
    static const char* include;                                               \
    explicit fun##_(T&& a) : base(std::forward<T>(a), #fun) {}                \
    inline auto deep_copy() const {                                           \
      auto&& arg_copy = this->template get_arg<0>().deep_copy();              \
      return fun##_<std::remove_reference_t<decltype(arg_copy)>>{             \
          std::move(arg_copy)};                                               \
    }                                                                         \
  };                                                                          \
                                                                              \
  template <typename T, typename Cond                                         \
                        = require_all_valid_expressions_and_none_scalar_t<T>> \
  inline fun##_<as_operation_cl_t<T>> fun(T&& a) {                            \
    return fun##_<as_operation_cl_t<T>>(as_operation_cl(std::forward<T>(a))); \
  }                                                                           \
  template <typename T>                                                       \
  const char* fun##_<T>::include = "";

ADD_UNARY_FUNCTION(rsqrt)
ADD_UNARY_FUNCTION_PASS_ZERO(sqrt)
ADD_UNARY_FUNCTION_PASS_ZERO(cbrt)

ADD_UNARY_FUNCTION(exp)
ADD_UNARY_FUNCTION(exp2)
ADD_UNARY_FUNCTION_PASS_ZERO(expm1)

ADD_UNARY_FUNCTION(log)
ADD_UNARY_FUNCTION(log2)
ADD_UNARY_FUNCTION(log10)
ADD_UNARY_FUNCTION_PASS_ZERO(log1p)

ADD_UNARY_FUNCTION_PASS_ZERO(sin)
ADD_UNARY_FUNCTION_PASS_ZERO(sinh)
ADD_UNARY_FUNCTION(cos)
ADD_UNARY_FUNCTION(cosh)
ADD_UNARY_FUNCTION_PASS_ZERO(tan)
ADD_UNARY_FUNCTION_PASS_ZERO(tanh)
ADD_UNARY_FUNCTION_PASS_ZERO(asin)
ADD_UNARY_FUNCTION_PASS_ZERO(asinh)
ADD_UNARY_FUNCTION(acos)
ADD_UNARY_FUNCTION(acosh)
ADD_UNARY_FUNCTION_PASS_ZERO(atan)
ADD_UNARY_FUNCTION_PASS_ZERO(atanh)

ADD_UNARY_FUNCTION(tgamma)
ADD_UNARY_FUNCTION(lgamma)
ADD_UNARY_FUNCTION_PASS_ZERO(erf)
ADD_UNARY_FUNCTION(erfc)

ADD_UNARY_FUNCTION_PASS_ZERO(floor)
ADD_UNARY_FUNCTION_PASS_ZERO(round)
ADD_UNARY_FUNCTION_PASS_ZERO(ceil)

ADD_UNARY_FUNCTION_WITH_INCLUDE(digamma,
                                opencl_kernels::digamma_device_function)
ADD_UNARY_FUNCTION_WITH_INCLUDE(log1m_exp,
                                opencl_kernels::log1m_exp_device_function)
ADD_UNARY_FUNCTION_WITH_INCLUDE(log1p_exp,
                                opencl_kernels::log1p_exp_device_function)

#undef ADD_UNARY_FUNCTION_WITH_INCLUDE
#undef ADD_UNARY_FUNCTION
#undef ADD_UNARY_FUNCTION_PASS_ZERO
/** @}*/
}  // namespace math
}  // namespace stan
#endif
#endif
