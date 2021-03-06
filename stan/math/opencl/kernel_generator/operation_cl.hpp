#ifndef STAN_MATH_OPENCL_KERNEL_GENERATOR_OPERATION_CL_HPP
#define STAN_MATH_OPENCL_KERNEL_GENERATOR_OPERATION_CL_HPP
#ifdef STAN_OPENCL

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/err.hpp>
#include <stan/math/opencl/kernel_generator/wrapper.hpp>
#include <stan/math/opencl/kernel_generator/type_str.hpp>
#include <stan/math/opencl/kernel_generator/name_generator.hpp>
#include <stan/math/opencl/kernel_generator/is_valid_expression.hpp>
#include <stan/math/opencl/matrix_cl_view.hpp>
#include <stan/math/opencl/matrix_cl.hpp>
#include <stan/math/opencl/kernel_cl.hpp>
#include <CL/cl2.hpp>
#include <algorithm>
#include <string>
#include <utility>
#include <tuple>
#include <set>
#include <array>
#include <numeric>
#include <vector>

namespace stan {
namespace math {

/** \addtogroup opencl_kernel_generator
 *  @{
 */
/**
 * Parts of an OpenCL kernel, generated by an expression
 */
struct kernel_parts {
  std::string includes;  // any function definitions - as if they were includet
                         // at the start of kernel source
  std::string initialization;  // the code for initializations done by all
                               // threads, even if they have no work
  std::string body_prefix;     // the code that should be placed at the start of
                               // the kernel body
  std::string body;       // the body of the kernel - code executing operations
  std::string reduction;  // the code for reductions within work group by all
                          // threads, even if they have no work
  std::string args;       // kernel arguments

  kernel_parts operator+(const kernel_parts& other) {
    return {
        includes + other.includes,       initialization + other.initialization,
        body_prefix + other.body_prefix, body + other.body,
        reduction + other.reduction,     args + other.args};
  }

  kernel_parts operator+=(const kernel_parts& other) {
    includes += other.includes;
    initialization += other.initialization;
    body_prefix += other.body_prefix;
    body += other.body;
    reduction += other.reduction;
    args += other.args;
    return *this;
  }
};

/**
 * Base for all kernel generator operations.
 * @tparam Derived derived type
 * @tparam Scalar scalar type of the result
 * @tparam Args types of arguments to this operation
 */
template <typename Derived, typename Scalar, typename... Args>
class operation_cl : public operation_cl_base {
  static_assert(
      conjunction<std::is_base_of<operation_cl_base,
                                  std::remove_reference_t<Args>>...>::value,
      "operation_cl: all arguments to operation must be operations!");

 protected:
  std::tuple<internal::wrapper<Args>...> arguments_;
  mutable std::string var_name;  // name of the variable that holds result of
                                 // this operation in the kernel

  /**
   * Casts the instance into its derived type.
   * @return \c this cast into derived type
   */
  inline Derived& derived() { return *static_cast<Derived*>(this); }

  /**
   * Casts the instance into its derived type.
   * @return \c this cast into derived type
   */
  inline const Derived& derived() const {
    return *static_cast<const Derived*>(this);
  }

 public:
  using Deriv = Derived;
  using ArgsTuple = std::tuple<Args...>;
  static const bool require_specific_local_size;
  // number of arguments this operation has
  static constexpr int N = sizeof...(Args);
  using view_transitivity = std::tuple<std::is_same<Args, void>...>;
  // value representing a not yet determined size
  static const int dynamic = -1;

  /**
    Returns an argument to this operation
    @tparam N index of the argument
    */
  template <size_t N>
  const auto& get_arg() const {
    return std::get<N>(arguments_).x;
  }

  /**
   * Constructor
   * @param arguments Arguments of this expression that are also valid
   * expressions
   */
  explicit operation_cl(Args&&... arguments)
      : arguments_(internal::wrapper<Args>(std::forward<Args>(arguments))...) {}

  /**
   * Evaluates the expression.
   * @return Result of the expression.
   */
  matrix_cl<Scalar> eval() const {
    int rows = derived().rows();
    int cols = derived().cols();
    check_nonnegative("operation_cl.eval", "this->rows()", rows);
    check_nonnegative("operation_cl.eval", "this->cols()", cols);
    matrix_cl<Scalar> res(rows, cols, derived().view());
    if (res.size() > 0) {
      this->evaluate_into(res);
    }
    return res;
  }

  /**
   * Evaluates \c this expression into given left-hand-side expression.
   * If the kernel for this expression is not cached it is generated and then
   * executed.
   * @tparam T_lhs type of the left-hand-side expression
   * @param lhs Left-hand-side expression
   */
  template <typename T_lhs>
  inline void evaluate_into(T_lhs& lhs) const;

  /**
   * Generates kernel source for evaluating \c this expression into given
   * left-hand-side expression.
   * @tparam T_lhs type of the left-hand-side expression
   * @param lhs Left-hand-side expression
   * @return kernel source
   */
  template <typename T_lhs>
  inline std::string get_kernel_source_for_evaluating_into(
      const T_lhs& lhs) const;

  template <typename T_lhs>
  struct cache {
    static std::string source;  // kernel source - not used anywhere. Only
                                // intended for debugging.
    static cl::Kernel kernel;   // cached kernel - different for every
                                // combination of template instantiation of \c
                                // operation and every \c T_lhs
  };

  /**
   * Generates kernel code for assigning this expression into result expression.
   * @param[in,out] generated set of (pointer to) already generated operations
   * @param ng name generator for this kernel
   * @param i row index variable name
   * @param j column index variable name
   * @param result expression into which result is to be assigned
   * @return part of kernel with code for this and nested expressions
   */
  template <typename T_result>
  kernel_parts get_whole_kernel_parts(
      std::set<const operation_cl_base*>& generated, name_generator& ng,
      const std::string& i, const std::string& j,
      const T_result& result) const {
    kernel_parts parts = derived().get_kernel_parts(generated, ng, i, j, false);
    kernel_parts out_parts = result.get_kernel_parts_lhs(generated, ng, i, j);
    out_parts.body += " = " + derived().var_name + ";\n";
    parts += out_parts;
    return parts;
  }

  /**
   * generates kernel code for this and nested expressions.
   * @param[in,out] generated set of (pointer to) already generated operations
   * @param name_gen name generator for this kernel
   * @param i row index variable name
   * @param j column index variable name
   * @param view_handled whether caller already handled matrix view
   * @return part of kernel with code for this and nested expressions
   */
  inline kernel_parts get_kernel_parts(
      std::set<const operation_cl_base*>& generated, name_generator& name_gen,
      const std::string& i, const std::string& j, bool view_handled) const {
    kernel_parts res{};
    if (generated.count(this) == 0) {
      this->var_name = name_gen.generate();
      generated.insert(this);
      std::string i_arg = i;
      std::string j_arg = j;
      derived().modify_argument_indices(i_arg, j_arg);
      std::array<kernel_parts, N> args_parts = index_apply<N>([&](auto... Is) {
        return std::array<kernel_parts, N>{this->get_arg<Is>().get_kernel_parts(
            generated, name_gen, i_arg, j_arg,
            view_handled
                && std::tuple_element_t<
                       Is, typename Deriv::view_transitivity>::value)...};
      });
      res = std::accumulate(args_parts.begin(), args_parts.end(),
                            kernel_parts{});
      kernel_parts my_part = index_apply<N>([&](auto... Is) {
        return this->derived().generate(i, j, view_handled,
                                        this->get_arg<Is>().var_name...);
      });
      res += my_part;
      res.body = res.body_prefix + res.body;
      res.body_prefix = "";
    }
    return res;
  }

  /**
   * Does nothing. Derived classes can override this to modify how indices are
   * passed to its argument expressions. On input arguments \c i and \c j are
   * expressions for indices of this operation. On output they are expressions
   * for indices of argument operations.
   * @param[in, out] i row index
   * @param[in, out] j column index
   */
  inline void modify_argument_indices(std::string& i, std::string& j) const {}

  /**
   * Sets kernel arguments for nested expressions.
   * @param[in,out] generated set of expressions that already set their kernel
   * arguments
   * @param kernel kernel to set arguments on
   * @param[in,out] arg_num consecutive number of the first argument to set.
   * This is incremented for each argument set by this function.
   */
  inline void set_args(std::set<const operation_cl_base*>& generated,
                       cl::Kernel& kernel, int& arg_num) const {
    if (generated.count(this) == 0) {
      generated.insert(this);
      // parameter pack expansion returns a comma-separated list of values,
      // which can not be used as an expression. We work around that by using
      // comma operator to get a list of ints, which we use to construct an
      // initializer_list from. Cast to voids avoids warnings about unused
      // expression.
      index_apply<N>([&](auto... Is) {
        static_cast<void>(std::initializer_list<int>{
            (this->get_arg<Is>().set_args(generated, kernel, arg_num), 0)...});
      });
    }
  }

  /**
   * Adds read event to any matrices used by nested expressions.
   * @param e the event to add
   */
  inline void add_read_event(cl::Event& e) const {
    index_apply<N>([&](auto... Is) {
      static_cast<void>(std::initializer_list<int>{
          (this->get_arg<Is>().add_read_event(e), 0)...});
    });
  }

  /**
   * Adds all write events on any matrices used by nested expressions to a list
   * and clears them from those matrices.
   * @param[out] events List of all events.
   */
  inline void get_clear_write_events(std::vector<cl::Event>& events) const {
    index_apply<N>([&](auto... Is) {
      static_cast<void>(std::initializer_list<int>{
          (this->template get_arg<Is>().get_clear_write_events(events), 0)...});
    });
  }

  /**
   * Number of rows of a matrix that would be the result of evaluating this
   * expression. Some subclasses may need to override this.
   * @return number of rows
   */
  inline int rows() const {
    return index_apply<N>([&](auto... Is) {
      // assuming all non-dynamic sizes match
      return std::max({this->get_arg<Is>().rows()...});
    });
  }

  /**
   * Number of columns of a matrix that would be the result of evaluating this
   * expression. Some subclasses may need to override this.
   * @return number of columns
   */
  inline int cols() const {
    return index_apply<N>([&](auto... Is) {
      // assuming all non-dynamic sizes match
      return std::max({this->get_arg<Is>().cols()...});
    });
  }

  /**
   * Number of rows threads need to be launched for. For most expressions this
   * equals number of rows of the result.
   * @return number of rows
   */
  inline int thread_rows() const { return derived().rows(); }

  /**
   * Number of columns threads need to be launched for. For most expressions
   * this equals number of cols of the result.
   * @return number of columns
   */
  inline int thread_cols() const { return derived().cols(); }

  /**
   * Determine indices of extreme sub- and superdiagonals written. Some
   * subclasses may need to override this.
   * @return pair of indices - bottom and top diagonal
   */
  inline std::pair<int, int> extreme_diagonals() const {
    return index_apply<N>([&](auto... Is) {
      auto arg_diags
          = std::make_tuple(this->get_arg<Is>().extreme_diagonals()...);
      int bottom = std::min(
          std::initializer_list<int>({std::get<Is>(arg_diags).first...}));
      int top = std::max(
          std::initializer_list<int>({std::get<Is>(arg_diags).second...}));
      return std::make_pair(bottom, top);
    });
  }

  /**
   * View of a matrix that would be the result of evaluating this expression.
   * @return view
   */
  inline matrix_cl_view view() const {
    std::pair<int, int> diagonals = derived().extreme_diagonals();
    matrix_cl_view view;
    if (diagonals.first < 0) {
      view = matrix_cl_view::Lower;
    } else {
      view = matrix_cl_view::Diagonal;
    }
    if (diagonals.second > 0) {
      view = either(view, matrix_cl_view::Upper);
    }
    return view;
  }
};

template <typename Derived, typename Scalar, typename... Args>
template <typename T_lhs>
cl::Kernel operation_cl<Derived, Scalar, Args...>::cache<T_lhs>::kernel;

template <typename Derived, typename Scalar, typename... Args>
template <typename T_lhs>
std::string operation_cl<Derived, Scalar, Args...>::cache<T_lhs>::source;

template <typename Derived, typename Scalar, typename... Args>
const bool operation_cl<Derived, Scalar, Args...>::require_specific_local_size
    = std::max({false,
                std::decay_t<Args>::Deriv::require_specific_local_size...});
/** @}*/
}  // namespace math
}  // namespace stan

#endif
#endif
