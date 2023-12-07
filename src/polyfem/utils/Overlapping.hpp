#include <ipc/ipc.hpp>
#include <polyfem/State.hpp>

namespace polyfem {
    void relax_overlapping(const State& state, Eigen::MatrixXd& sol, int max_iter);
}
