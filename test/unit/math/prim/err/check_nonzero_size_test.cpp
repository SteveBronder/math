
#include <stan/math/prim.hpp>
#include <gtest/gtest.h>
#include <test/unit/util.hpp>
#include <limits>
#include <vector>

TEST(ErrorHandlingMatrix, checkNonzeroSizeMatrix) {
  using stan::math::check_nonzero_size;

  std::vector<double> a;
  a.push_back(3.0);
  a.push_back(3.0);
  a.push_back(3.0);
  a.push_back(3.0);

  EXPECT_NO_THROW(stan::math::check_nonzero_size("checkNonzeroSize", "a", a));

  a.resize(2);
  EXPECT_NO_THROW(stan::math::check_nonzero_size("checkNonzeroSize", "a", a));

  a.resize(0);
  EXPECT_THROW_MSG(stan::math::check_nonzero_size("checkNonzeroSize", "a", a),
                   std::invalid_argument, "has size 0");
}

TEST(ErrorHandlingMatrix, checkNonzeroSizeMatrix_nan) {
  double nan = std::numeric_limits<double>::quiet_NaN();

  std::vector<double> a;
  a.push_back(nan);
  a.push_back(nan);
  a.push_back(nan);
  a.push_back(nan);

  EXPECT_NO_THROW(stan::math::check_nonzero_size("checkNonzeroSize", "a", a));

  a.resize(2);
  EXPECT_NO_THROW(stan::math::check_nonzero_size("checkNonzeroSize", "a", a));

  a.resize(0);
  EXPECT_THROW_MSG(stan::math::check_nonzero_size("checkNonzeroSize", "a", a),
                   std::invalid_argument, "has size 0");
}

TEST(ErrorHandlingMatrix_mat, checkNonzeroSizeMatrix) {
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> y;
  using stan::math::check_nonzero_size;

  y.resize(3, 3);
  EXPECT_NO_THROW(check_nonzero_size("checkNonzeroSize", "y", y));
  y.resize(2, 3);
  EXPECT_NO_THROW(check_nonzero_size("checkNonzeroSize", "y", y));

  y.resize(0, 0);
  EXPECT_THROW_MSG(check_nonzero_size("checkNonzeroSize", "y", y),
                   std::invalid_argument, "has size 0");
}

TEST(ErrorHandlingMatrix_mat, checkNonzeroSizeMatrix_nan) {
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> y;
  double nan = std::numeric_limits<double>::quiet_NaN();

  y.resize(3, 3);
  y << nan, nan, nan, nan, nan, nan, nan, nan, nan;
  EXPECT_NO_THROW(stan::math::check_nonzero_size("checkNonzeroSize", "y", y));
  y.resize(2, 3);
  y << nan, nan, nan, nan, nan, nan;
  EXPECT_NO_THROW(stan::math::check_nonzero_size("checkNonzeroSize", "y", y));

  y.resize(0, 0);
  EXPECT_THROW_MSG(stan::math::check_nonzero_size("checkNonzeroSize", "y", y),
                   std::invalid_argument, "has size 0");
}
