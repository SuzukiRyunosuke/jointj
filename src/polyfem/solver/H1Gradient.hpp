#pragma once

#include <polyfem/Common.hpp>
#include <polyfem/State.hpp>
#include <polyfem/solver/SolveData.hpp>
#include <polyfem/solver/Director.hpp>
#include <polyfem/solver/forms/adjoint_forms/VariableToSimulation.hpp>

namespace cppoptlib {
    class H1Gradient : public Director
    {
    public:
        H1Gradient(
            std::vector<std::shared_ptr<polyfem::solver::VariableToSimulation>> var2sims,
            const json &params);
        ~H1Gradient() {}

        virtual Eigen::VectorXd compute_update_direction (
            const Eigen::VectorXd &x,
            const Eigen::VectorXd &grad,
            Eigen::VectorXd &direction) final override;

        virtual std::string name() const final override { return "H1 Gradient"; }

        std::string descent_strategy_name(int descent_strategy) const override { return "H1 gradient"; }
    private:
        json grad_to_rhs(const Eigen::VectorXd &grad) const;

        void overwrite_boundary_conditions();
        void setup_bc();
        std::shared_ptr<polyfem::assembler::RhsAssembler> build_rhs_assembler() const;

        void init_solve(
                Eigen::MatrixXd &sol,
                polyfem::solver::SolveData &solve_data,
                const Eigen::MatrixXd &rhs);

        void solve(Eigen::MatrixXd &sol, polyfem::solver::SolveData &solve_data, const Eigen::MatrixXd &rhs);
        Eigen::VectorXd grad_to_normal_load(const Eigen::VectorXd &grad) const;
    private:

        std::shared_ptr<polyfem::State> state;
        polyfem::assembler::GenericTensorProblem problem;

        std::vector<int> boundary_nodes;
        std::vector<int> pressure_boundary_nodes;
        std::vector<polyfem::mesh::LocalBoundary> total_local_boundary;
        std::vector<polyfem::mesh::LocalBoundary> local_boundary;
        std::vector<polyfem::mesh::LocalBoundary> local_neumann_boundary;
        //std::map<int, polyfem::basis::InterfaceData> poly_edge_to_data;
        std::vector<int> dirichlet_nodes;
        std::vector<polyfem::RowVectorNd> dirichlet_nodes_position;
        /// per node neumann
        std::vector<int> neumann_nodes;
        std::vector<polyfem::RowVectorNd> neumann_nodes_position;

        // rhs weight
        const double weight = 1;

        // solve parameters
        const bool contact_enabled;
        const double friction_coefficient;
        const double dhat;
        const double barrier_stiffness;

        std::string instance_name;
        int subsolve_count = 0;
        /// per node dirichlet. (might be extended i.e: neumann b.c is forcibly treated also as dirichlet one)
        //std::vector<int> dirichlet_nodes;
        //std::vector<polyfem::RowVectorNd> dirichlet_nodes_position;
    };
}
