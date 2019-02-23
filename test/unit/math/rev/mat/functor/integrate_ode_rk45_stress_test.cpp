#include <stan/math/rev/mat.hpp>
#include <boost/random.hpp>
#include <stan/math/prim/scal/prob/lognormal_rng.hpp>
#include <gtest/gtest.h>
// very small michaelis menten example
#include <test/unit/math/rev/arr/functor/coupled_mm.hpp>
#include <test/unit/util.hpp>
#include <vector>

// test which triggers the too much work exception from odeint
TEST(StanOde_tooMuchWork_test, rk45_coupled_mm) {
  coupled_mm_ode_fun f_;

  boost::ecuyer1988 rng;

  // initial value and parameters from model definition

  double t0 = 0;

  std::vector<double> ts_long;
  ts_long.push_back(1E3);

  std::vector<double> ts_short;
  ts_short.push_back(1);

  std::vector<double> data;

  std::vector<int> data_int;

  typedef stan::math::var scalar_type;

  for (std::size_t i = 0; i < 10; i++) {
    stan::math::start_nested();

    std::vector<scalar_type> theta_v(4);

    theta_v[0] = stan::math::lognormal_rng(1, 2, rng);
    theta_v[1] = stan::math::lognormal_rng(-1, 2, rng);
    theta_v[2] = stan::math::lognormal_rng(-1, 2, rng);
    theta_v[3] = stan::math::lognormal_rng(-2, 2, rng);

    std::vector<scalar_type> y0_v(2);
    y0_v[0] = stan::math::lognormal_rng(5, 2, rng);
    y0_v[1] = stan::math::lognormal_rng(-1, 2, rng);

    std::vector<std::vector<scalar_type> > res
        = stan::math::integrate_ode_rk45(f_, y0_v, t0, ts_long, theta_v, data,
                                         data_int, 0, 1E-6, 1E-6, 1000000000);

    stan::math::grad(res[0][0].vi_);

    stan::math::recover_memory_nested();
  }

  stan::math::print_stack(std::cout);
}