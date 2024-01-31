#pragma once

#include <polyfem/Common.hpp>
#include "NonlinearSolver.hpp"
#include "polyfem/solver/forms/adjoint_forms/VariableToSimulation.hpp"
#include <polysolve/LinearSolver.hpp>
#include <polyfem/utils/MatrixUtils.hpp>

#include <polyfem/utils/Logger.hpp>

#include <polyfem/solver/H1Gradient.hpp>
#include <vector>

namespace cppoptlib
{
	template <typename ProblemType>
	class H1GradientSolver : public NonlinearSolver<ProblemType>
	{
	public:
		using Superclass = NonlinearSolver<ProblemType>;
		using typename Superclass::Scalar;
		using typename Superclass::TVector;

		H1GradientSolver(std::vector<std::shared_ptr<polyfem::solver::VariableToSimulation>> var2sims,
                    const json &solver_params, const double dt, const double characteristic_length) 
                  : Superclass(solver_params, dt, characteristic_length), h1(H1Gradient(var2sims, solver_params["h1"])) {};

		std::string name() const override { return h1.name(); }

	protected:
		virtual int default_descent_strategy() override { return h1.default_descent_strategy(); }

		using Superclass::descent_strategy_name;
		std::string descent_strategy_name(int descent_strategy) const override { return h1.descent_strategy_name(descent_strategy); };

		void increase_descent_strategy() override { h1.increase_descent_strategy(); };

		void reset(const int ndof) override {};

		bool compute_update_direction(
			ProblemType &objFunc,
			const TVector &x,
			const TVector &grad,
			TVector &direction) override 
                { 
                  const auto cur_log_level = polyfem::logger().level();
                  objFunc.get_state(0)->set_log_level(static_cast<spdlog::level::level_enum>(3));
                  h1.compute_update_direction(x, grad, direction);
                  objFunc.get_state(0)->set_log_level(cur_log_level);
                  return true;
                }

                H1Gradient h1;
	};
} // namespace cppoptlib

#include "LBFGSSolver.tpp"
