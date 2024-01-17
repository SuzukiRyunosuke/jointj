#pragma once

#include <polyfem/Common.hpp>
#include "FullNLProblem.hpp"
#include <polyfem/solver/Director.hpp>

namespace polyfem
{
    class State;
}

namespace polyfem::solver
{
    class AdjointForm;
    class CompositeForm;
    class ParallelForm;
    class VariableToSimulation;

    class ParallelAdjointNLProblem : public FullNLProblem
    {
    public:
        ParallelAdjointNLProblem(
                    const std::vector<std::shared_ptr<CompositeForm>> &forms,
                    const int global,
                    const std::vector<std::shared_ptr<VariableToSimulation>> &variables_to_simulation,
                    const std::vector<std::shared_ptr<State>> &all_states,
                    const json &args);
        ParallelAdjointNLProblem(
                    const std::vector<std::shared_ptr<CompositeForm>> &forms,
                    const int global,
                    const std::vector<std::shared_ptr<AdjointForm>> stopping_conditions,
                    const std::vector<std::shared_ptr<VariableToSimulation>> &variables_to_simulation,
                    const std::vector<std::shared_ptr<State>> &all_states,
                    const json &args);

        void set_gradient_methods(const polyfem::json &compositions);

        double value(const Eigen::VectorXd &x) override;
                Eigen::VectorXd values(const Eigen::VectorXd &x);

        void gradient(const Eigen::VectorXd &x, Eigen::VectorXd &grad) override;
        void gradients(const Eigen::VectorXd &x, Eigen::MatrixXd &grads);
        void hessian(const Eigen::VectorXd &x, StiffnessMatrix &hessian) override;
        void save_to_file(const Eigen::VectorXd &x0) override;
        void save_to_file(const std::unordered_map<std::string, Eigen::VectorXd> &out_params);
        bool is_step_valid(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) const override;
        bool is_step_collision_free(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) const override;
        double max_step_size(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) const override;

        void line_search_begin(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) override;
        void line_search_end() override;
        void post_step(const int iter_num, const Eigen::VectorXd &x) override;
        bool stop(const TVector &x) override;

        void solution_changed(const Eigen::VectorXd &new_x) override;

        void solve_pde();

        int n_states() const { return all_states_.size(); }
                int n_var2sim() const { return variables_to_simulation_.size(); }
                int n_forms() const { return forms_.size(); }

        std::shared_ptr<State> get_state(int state_id) { return all_states_[state_id]; }

        virtual bool is_optimization() override { return true; }

        bool remesh();

        void reinit_forms();

        int n_objectives() { assert(forms_.size() == directors.size()); return forms_.size(); }
        void set_iter(int i) { iter = i; }
        int get_iter() override { return iter; }
        bool compute_update_direction(const Eigen::VectorXd &x, const Eigen::MatrixXd &grads, Eigen::VectorXd &direction);

        int default_descent_strategy() { return directors[global]->default_descent_strategy(); };
        void increase_descent_strategy() { for (auto d: directors) d->increase_descent_strategy(); };

        std::string descent_strategy_name(int descent_strategy) { return directors[global]->descent_strategy_name(descent_strategy); };
        std::string descent_strategy_name() const { return directors[global]->descent_strategy_name(descent_strategy); };
        int descent_strategy; // 0, newton, 1 spd, 2 gradiant
        Eigen::VectorXd shape_diff_;
        Eigen::VectorXd rhs_; // for debugging
    private:

        // Compute the search/update direction
        std::vector<std::shared_ptr<cppoptlib::Director>> directors;

        //std::vector<AdjointForm> forms_;
        std::vector<std::shared_ptr<CompositeForm>> forms_;
        const int global;
        std::vector<std::shared_ptr<VariableToSimulation>> variables_to_simulation_;
        std::vector<std::shared_ptr<State>> all_states_;
        std::vector<bool> active_state_mask;
        //Eigen::VectorXd cur_grad;
        int iter = 0;

        const int solve_log_level;
        const int save_freq;

        const bool solve_in_parallel;
        std::vector<int> solve_in_order;
        const bool better_initial_guess;
        const double max_step_size_;
        const std::string solution_is;

        std::vector<std::shared_ptr<AdjointForm>> stopping_conditions_; // if all the stopping conditions are non-positive, stop the optimization
    };
} // namespace polyfem::solver
