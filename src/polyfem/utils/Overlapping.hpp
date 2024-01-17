#include <ipc/ipc.hpp>
#include <polyfem/State.hpp>

namespace polyfem {
    void get_normal_outward(const State& state, const int element_id, const int boundary_primitive_id, Eigen::VectorXd& normal);
    void relax_overlapping(const State& state, Eigen::MatrixXd& sol, int max_iter=30);
}
