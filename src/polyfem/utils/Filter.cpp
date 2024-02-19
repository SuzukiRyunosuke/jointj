#include <polyfem/utils/Filter.hpp>
#include <polyfem/utils/MatrixUtils.hpp>
#include <iostream>
namespace polyfem::utils
{
    double std_deviation(Eigen::ArrayXd a) {
        return std::sqrt((a-a.mean()).square().sum()/(a.size()-1));
    }
    void filter_outlier(Eigen::VectorXd &v, const double variance, const double abs_max, const int dim) {
        Eigen::MatrixXd m = unflatten(v, dim);
        bool ok = false;
        const double filter_rate = 0.9;
        int count = 0;
        Eigen::VectorXd norms = m.rowwise().norm();
        Eigen::VectorXd filtered = norms;
        for (int i = 0; i < filtered.size(); ++i) {
          filtered(i) = std::min(filtered(i), abs_max);
        }
        while (!ok) {
            count++;
            double std_dev = std_deviation(filtered.array());//v.array());
            int i_max, c;
            double max = filtered.maxCoeff(&i_max, &c);
            ok = std_dev == 0 || std_dev * variance >= max;
            if (!ok) {
                filtered(i_max) *= filter_rate;
            }
            if (count > 1e8) {
              std::cout << "Filter.cpp says 'something seems gone wrong' like ... std_dev = " << std_dev << ", max = " << max << std::endl;
            }
        }
        for (int i = 0; i < filtered.size(); ++i) {
          if (norms(i) > 0) {
            filtered(i) /= norms(i);
          }
        }
        v = flatten(m.array().colwise() * filtered.array());
    }
}
