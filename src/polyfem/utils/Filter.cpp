#include <polyfem/utils/Filter.hpp>
#include <iostream>
namespace polyfem
{
    double std_deviation(Eigen::ArrayXd a) {
        return std::sqrt((a-a.mean()).square().sum()/(a.size()-1));
    }
    void filter_outlier(Eigen::VectorXd &v, const double variance, const double v_max) {
        bool ok = false;
        const double filter_rate = 0.9;
        int count = 0;
        while (!ok) {
          count++;
            double std_dev = std_deviation(v.array());
            int i_max, i_min, c;
            double max = v.maxCoeff(&i_max, &c);
            double min = v.minCoeff(&i_min, &c);
            ok = (std_dev == 0 || std_dev * variance >= abs(max - min)) && max < v_max && min > -1 * v_max;
            if (!ok) {
                v(i_max) = std::min(v_max, v(i_max)*filter_rate);
                v(i_min) = std::max(-1 * v_max, v(i_min)*filter_rate);
                v(i_min) *= filter_rate;
            }
            if (count > 1e5) {
              std::cout << "Filter.cpp says 'something seems gone wrong' like ... std_dev = " << std_dev << ", abs(max-min) = " << abs(max-min) << ", max = " << max << ", min = " << min << std::endl;
            }
        }
    }
}
