#include <polyfem/Common.hpp>

namespace polyfem
{
    double std_deviation(Eigen::ArrayXd a);
    void filter_outlier(Eigen::VectorXd &v);
}
