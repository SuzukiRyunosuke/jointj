#pragma once

#include <polyfem/Common.hpp>

// Line search methods
#include "line_search/LineSearch.hpp"
#include "polyfem/solver/ParallelAdjointNLProblem.hpp"
#include "polyfem/solver/line_search/ParallelBacktrackingLineSearch.hpp"
#include "polyfem/solver/LBFGS.hpp"

#include <polyfem/utils/Logger.hpp>
#include <polyfem/utils/Timer.hpp>

#include <cppoptlib/solver/isolver.h>

// clang-format off
//template <> struct fmt::formatter<cppoptlib::Status> : ostream_formatter {};
// clang-format on

namespace cppoptlib
{

        /*
        enum class ErrorCode
	{
		NAN_ENCOUNTERED = -10,
		STEP_TOO_SMALL = -1,
		SUCCESS = 0,
	};
        */


        typedef polyfem::solver::ParallelAdjointNLProblem ProblemType;
	class ParallelNonlinearSolver : public ISolver<ProblemType, /*Ord=*/-1>
	{
	public:
		using Superclass = ISolver<ProblemType, /*Ord=*/-1>;
		using typename Superclass::Scalar;
		using typename Superclass::TCriteria;
		using typename Superclass::TVector;

		/// @brief Construct a new Nonlinear Solver object
		/// @param solver_params JSON of solver parameters (see input spec.)
		/// @param dt time step size (use 1 if not time-dependent)
		ParallelNonlinearSolver(const polyfem::json &solver_params, const double dt, const double characteristic_length, const int n_domains);

		virtual double compute_grad_norm(const Eigen::VectorXd &x, const Eigen::VectorXd &grad) const;

		std::string name() const { return "Parallel NonLinear"; };

		void set_line_search(const polyfem::json &line_search_params);

	        void set_directors(int n_domains, const polyfem::json &solver_params);

		void /* parallel */ minimize(ProblemType &objFunc, TVector &x) override;

		double /* parallel */ line_search(const TVector &x, const TVector &delta_x, ProblemType &objFunc);

		const polyfem::json &get_info() const { return solver_info; }

		ErrorCode error_code() const { return m_error_code; }

		const typename Superclass::TCriteria &getStopCriteria() { return this->m_stop; }
		// setStopCriteria already in ISolver

		bool converged() const
		{
			return this->m_status == Status::XDeltaTolerance
				   || this->m_status == Status::FDeltaTolerance
				   || this->m_status == Status::GradNormTolerance;
		}

		size_t max_iterations() const { return this->m_stop.iterations; }
		size_t &max_iterations() { return this->m_stop.iterations; }
		bool allow_out_of_iterations = false;

	protected:
		// ====================================================================
		//                        Solver parameters
		// ====================================================================

		bool normalize_gradient;
		double use_grad_norm_tol;
		double first_grad_norm_tol;
		double dt;

		// ====================================================================
		//                           Solver state
		// ====================================================================

		// Reset the solver at the start of a minimization
		virtual void reset(const int ndof);

		// Compute the search/update direction
                std::vector<std::shared_ptr<LBFGS>> directors;
                std::shared_ptr<LBFGS> g_director;
		bool compute_update_direction(ProblemType &objFunc, const TVector &x, const TVector &grad, TVector &direction);

		virtual int default_descent_strategy() { return directors[0]->default_descent_strategy(); };
		virtual void increase_descent_strategy() { for (auto d: directors) d->increase_descent_strategy(); };

		virtual std::string descent_strategy_name(int descent_strategy) { return directors[0]->descent_strategy_name(descent_strategy); };
		virtual std::string descent_strategy_name() const { return directors[0]->descent_strategy_name(descent_strategy); };

		std::shared_ptr<polyfem::solver::line_search::ParallelBacktrackingLineSearch<ProblemType>> m_line_search;

		int descent_strategy; // 0, newton, 1 spd, 2 gradiant

		// ====================================================================
		//                            Solver info
		// ====================================================================

		virtual void update_solver_info(const double energy);
		void reset_times();
		void log_times();

		polyfem::json solver_info;

		double total_time;
		double grad_time;
		double assembly_time;
		double inverting_time;
		double line_search_time;
		double constraint_set_update_time;
		double obj_fun_time;

		ErrorCode m_error_code;

		// ====================================================================
		//                                 END
		// ====================================================================
	};
} // namespace cppoptlib

//#include "ParallelNonlinearSolver.cpp"
