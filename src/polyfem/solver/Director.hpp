#pragma once

#include <polyfem/Common.hpp>
namespace polyfem {
        class State;
}
namespace polyfem::solver {
        class VariableToSimulation;
}
namespace cppoptlib {
    class Director {
    public:
        Director(
          std::vector<std::shared_ptr<polyfem::solver::VariableToSimulation>> var2sims)
          : var2sims(var2sims) {};

        virtual ~Director() {}

        virtual Eigen::VectorXd compute_update_direction (
            const Eigen::VectorXd &x,
            const Eigen::VectorXd &grad,
            Eigen::VectorXd &direction) = 0;

        virtual std::string name() const = 0;

        virtual int default_descent_strategy() { return 1; };
        virtual void increase_descent_strategy() { descent_strategy++; };
        virtual std::string descent_strategy_name(int descent_strategy) const = 0;
        std::string descent_strategy_name() const { return descent_strategy_name(descent_strategy); };

        std::vector<std::shared_ptr<polyfem::solver::VariableToSimulation>> get_v2s()
        { 
            assert(var2sims.size() > 0);
            return var2sims;
        };

    protected:
        std::vector<std::shared_ptr<polyfem::solver::VariableToSimulation>> var2sims;
        int descent_strategy;
    };
}
