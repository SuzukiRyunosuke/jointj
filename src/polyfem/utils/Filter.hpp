#include <polyfem/Common.hpp>

namespace polyfem::utils
{
    double std_deviation(Eigen::ArrayXd a);
    void filter_outlier(Eigen::VectorXd &v, const double variance=10, const double abs_max = 1e-4, const int dim = 2);
}
