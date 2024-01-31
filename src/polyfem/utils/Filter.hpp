#include <polyfem/Common.hpp>

namespace polyfem
{
    double std_deviation(Eigen::ArrayXd a);
    void filter_outlier(Eigen::VectorXd &v, const double variance=10, const double v_max = 1e-4);
}
