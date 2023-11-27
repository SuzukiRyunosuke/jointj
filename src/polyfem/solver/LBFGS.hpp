// L-BFGS solver (Using the LBFGSpp under MIT License).

#pragma once

#include <polyfem/Common.hpp>
#include "NonlinearSolver.hpp"
#include <polysolve/LinearSolver.hpp>
#include <polyfem/utils/MatrixUtils.hpp>

#include <polyfem/utils/Logger.hpp>

#include <igl/Timer.h>

#include <LBFGSpp/BFGSMat.h>

namespace cppoptlib
{
	class LBFGS
	{
                typedef double Scalar;
                using TVector   = Eigen::VectorXd;
	public:
		LBFGS(const json &solver_params, const double dt, const double characteristic_length);

		std::string name() const { return "L-BFGS"; }

		bool compute_update_direction(
			const TVector &x,
			const TVector &grad,
			TVector &direction);

		int default_descent_strategy() { return 1; }

		std::string descent_strategy_name() const { return descent_strategy_name(descent_strategy); };
		std::string descent_strategy_name(int descent_strategy) const;

		void increase_descent_strategy();

		void reset(const int ndof);
		int descent_strategy; // 0, newton, 1 spd, 2 gradiant

		LBFGSpp::BFGSMat<Scalar> m_bfgs; // Approximation to the Hessian matrix

		/// The number of corrections to approximate the inverse Hessian matrix.
		/// The L-BFGS routine stores the computation results of previous \ref m
		/// iterations to approximate the inverse Hessian matrix of the current
		/// iteration. This parameter controls the size of the limited memories
		/// (corrections). The default value is \c 6. Values less than \c 3 are
		/// not recommended. Large values will result in excessive computing time.
		int m_history_size = 6;

		TVector m_prev_x;    // Previous x
		TVector m_prev_grad; // Previous gradient
	};
} // namespace cppoptlib

//#include "LBFGS.cpp"
