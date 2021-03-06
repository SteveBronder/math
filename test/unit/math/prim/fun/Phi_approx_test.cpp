#include <stan/math/prim.hpp>
#include <gtest/gtest.h>
#include <cmath>
#include <limits>

TEST(MathFunctions, Phi_approx) {
  EXPECT_EQ(0.5, stan::math::Phi_approx(0.0));
  EXPECT_NEAR(stan::math::Phi(0.9), stan::math::Phi_approx(0.9), 0.00014);
  EXPECT_NEAR(stan::math::Phi(-5.0), stan::math::Phi_approx(-5.0), 0.00014);
}

TEST(MathFunctions, Phi_approx_nan) {
  double nan = std::numeric_limits<double>::quiet_NaN();

  EXPECT_TRUE(std::isnan(stan::math::Phi_approx(nan)));
}

TEST(MathFunctions, Phi_approx__works_with_other_functions) {
  Eigen::VectorXd a(5);
  a << 1.1, 1.2, 1.3, 1.4, 1.5;
  Eigen::RowVectorXd b(5);
  b << 1.1, 1.2, 1.3, 1.4, 1.5;
  stan::math::multiply(a, stan::math::Phi_approx(b));
}
