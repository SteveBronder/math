#ifndef STAN_MATH_LAPLACE_LGP_DENSE_SYSTEM_HPP
#define STAN_MATH_LAPLACE_LGP_DENSE_SYSTEM_HPP

#include <stan/math/prim/mat/fun/Eigen.hpp>
#include <stan/math/prim/mat/fun/inverse_spd.hpp>
#include <stan/math/prim/mat.hpp>
#include <stan/math/fwd/mat/functor/jacobian.hpp>
#include <stan/math/fwd/mat.hpp>
#include <stan/math/rev/mat/fun/mdivide_left.hpp>
#include <stan/math/rev/mat.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
// TO DO - include specific files instead of rev/mat.hpp to
// improve compilation speed of unit tests.

namespace stan {
namespace math {

// TO DO - find ways to generalize / allow the user to specify
// the input covariance matrix, and the parameters.

/**
* A function to constructs the covariance matrix,
* based on the global paramter, phi, which includes variance
* and correlation of the local parameters.
* The covariance matrix can have two structures:
*   (i) homogeneous structure (same variance / covariance for each pair)
*   (ii) spatial structure (distant points have a lower covariance)
*/
template <typename T>
Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>
lgp_covariance (Eigen::Matrix<T, Eigen::Dynamic, 1> phi, int M,
                bool space_matters = false) {
  using std::pow;
  T sigma = phi[0];
  T rho = phi[1];
  double exponent;

  Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> Sigma(M, M);

  for (int i = 0; i < M; i++) {
    for (int j = 0; j < i; j++) {
      if (space_matters) {exponent = i - j;} else {exponent = 1;}
      Sigma(i, j) = pow(rho, exponent) * sigma;
      Sigma(j, i) = Sigma(i, j);
    }
    Sigma(i, i) = sigma;
  }

  return Sigma;
}

/**
* A structure for the parameters and data in a latent
* Gaussian Poisson (lgp) model. Returns the gradient of 
* the log density of the local parameter (theta) conditioned
* on the observed data (y) and the global parameter (phi),
* and also returns the conditional hessian.
*
* Note theta is not a member of the structure. This is because
* in one HMC iteration, theta is computed using a Newton 
* solver (i.e. unlike the other parameters, it is not fixed
* during that iteration).
*/
template<typename T0>
struct lgp_dense_system {
  Eigen::Matrix<T0, Eigen::Dynamic, 1> phi_;
  int M_;  // size of the system.
  bool space_matters_;
  std::vector<double> dat;
  Eigen::VectorXd n_samples_;  // number of samples for local parameter
  Eigen::VectorXd sums_;  // sums of observation for local parameter
  Eigen::Matrix<T0, Eigen::Dynamic, Eigen::Dynamic> Q_;  // precision matrix

  lgp_dense_system() {}

  lgp_dense_system(const Eigen::Matrix<T0, Eigen::Dynamic, 1>& phi,
                   const Eigen::VectorXd n_samples,
                   const Eigen::VectorXd& sums,
                   bool space_matters = false)
    : phi_(phi), M_(n_samples.size()), n_samples_(n_samples), sums_(sums),
      space_matters_(space_matters) {
    Q_ = inverse_spd(lgp_covariance(phi, M_, true));
  }

  /**
   * Function to generate dat for kinsol solver.
   * FIX ME -- use pointer approach, that avoids duplicating data.
   */
  std::vector<double> get_dat() const {
    std::vector<double> dat(M_ * (2 + M_));
    std::vector<double> Q_array = to_array_1d(Q_);
    for (int i = 0; i < M_; i++) dat[i] = n_samples_(i);
    for (int i = 0; i < M_; i++) dat[M_ + i] = sums_(i);
    for (int i = 0; i < M_ * M_; i++) dat[2 * M_ + i] = Q_array[i];
    return dat;
  }

  /**
  * Functions which retun class members
  * NOTE: can also access elements directly, since they're public.
  * Likely more efficient to use ->
  */
  Eigen::Matrix<T0, Eigen::Dynamic, 1> get_phi() const { return phi_; }
  Eigen::VectorXd get_n_samples() const { return n_samples_; }
  Eigen::VectorXd get_sums() const { return sums_; }
  Eigen::Matrix<T0, Eigen::Dynamic, 1> get_Q() const {return Q_; }
  bool get_space_matters() const { return space_matters_; }

  /**
  * An operator that returns the log conditional density, up to a
  * constant term. The returned object works as an
  * objective function we can optimize to find the mode. 
  */
  template <typename T1>
  typename stan::return_type<T0, T1>::type
    log_density (const Eigen::Matrix<T1, Eigen::Dynamic, 1> theta) const {
      Eigen::Matrix<T1, Eigen::Dynamic, 1> 
        poisson_term = elt_multiply(sums_, theta) - exp(theta);
      return sum(poisson_term) -
        0.5 * theta.transpose() * multiply(Q_, theta);
    }

  /**
  * An operator that returns the gradient of the log conditional density.
  */
  template <typename T1>
  Eigen::Matrix<typename stan::return_type<T0, T1>::type, 
                Eigen::Dynamic, Eigen::Dynamic>
  cond_gradient(const Eigen::Matrix<T1, Eigen::Dynamic, 1>& theta) const {
    return sums_ - elt_multiply(n_samples_, exp(theta)) -
      multiply(Q_, theta);
  }

  /**
  * An operator that returns the hessian of the log conditional density.
  * Required for Newton solver.
  */
  template <typename T1>
  Eigen::Matrix<typename stan::return_type<T0, T1>::type, 
                Eigen::Dynamic, Eigen::Dynamic>
  cond_hessian(const Eigen::Matrix<T1, Eigen::Dynamic, 1>& theta) const {
    Eigen::Matrix<T1, Eigen::Dynamic, Eigen::Dynamic> first_term = 
      elt_multiply(n_samples_, exp(theta)).asDiagonal();

    return - (first_term + Q_);
  }

  /**
  * A functor on which the Jacobian function can be called,
  * to compute the derivative of the objective function with
  * respect to phi.
  * NOTE: because we only want the derivatives with respect to
  * phi, we only compute the (second) term, which depends on phi.
  */
  struct deriv_objective {
    Eigen::VectorXd theta_;
    int M_;
    bool space_;

    deriv_objective (const Eigen::VectorXd theta, int M, bool space) : 
      theta_(theta), M_(M), space_(space) { }

    template <typename T>
    inline Eigen::Matrix<T, Eigen::Dynamic, 1>
    operator ()(const Eigen::Matrix<T, Eigen::Dynamic, 1>& phi) const {
      Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>
      Sigma = lgp_covariance(phi, M_, space_);

      return - mdivide_left(Sigma, theta_);
      // CHECK - can we precompute the covariance or precision matrix?
    }
  };

  /**
   * An operator that returns the Jacobian of theta with respect
   * to phi. This is done using the implicit function theorem.
   */
  template<typename T1>
  Eigen::Matrix<typename stan::return_type<T0, T1>::type,
                Eigen::Dynamic, Eigen::Dynamic>
  solver_gradient(const Eigen::Matrix<T1, Eigen::Dynamic, 1>& theta) const {
    Eigen::VectorXd dummy;
    Eigen::MatrixXd phi_sensitivities;
    deriv_objective f(value_of(theta), theta.size(), space_matters_);

    jacobian_fwd(f, phi_, dummy, phi_sensitivities);

    return - mdivide_left(cond_hessian(theta), phi_sensitivities);
  }
};

}  // namespace math
}  // namespace stan

#endif
