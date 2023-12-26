#include <polyfem/utils/Filter.hpp>
namespace polyfem
{
    double std_deviation(Eigen::ArrayXd a) {
        return std::sqrt((a-a.mean()).square().sum()/(a.size()-1));
    }
    void filter_outlier(Eigen::VectorXd &v) {
        bool ok = false;
        const double variance = 10;
        const double filter_rate = 0.9;
        while (!ok) {
            double std_dev = std_deviation(v.array());
            int r_max, r_min, c;
            double max = v.maxCoeff(&r_max, &c);
            double min = v.minCoeff(&r_min, &c);
            ok = std_dev * variance >= max - min;
            //std::cout << "std_dev=" << std_dev << ", max-min=" << max - min << std::endl;
            if (!ok) {
                v(r_max) *= filter_rate;
                v(r_min) *= filter_rate;
            }
        }
    }
}
