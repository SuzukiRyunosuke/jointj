// L-BFGS solver (Using the LBFGSpp under MIT License).

#pragma once

#include <polyfem/Common.hpp>
#include <polyfem/solver/NonlinearSolver.hpp>
#include <polysolve/LinearSolver.hpp>
#include <polyfem/utils/MatrixUtils.hpp>
#include <polyfem/solver/Director.hpp>
#include <polyfem/solver/forms/adjoint_forms/VariableToSimulation.hpp>

#include <polyfem/utils/Logger.hpp>

#include <igl/Timer.h>

#include <LBFGSpp/BFGSMat.h>

namespace cppoptlib
{
    class LBFGS: public Director
    {
                typedef double Scalar;
                using TVector   = Eigen::VectorXd;
    public:
        LBFGS(std::vector<std::shared_ptr<polyfem::solver::VariableToSimulation>> var2sims, const json &params)
          : Director(var2sims, params.value("weight", 1)), m_history_size(params.value("history_size", 6))
        {
            int size = 0;
            for (auto &v2s: var2sims) {
                size += v2s->get_parametrization().size(0); // assuming parametarization is slice map
            }
            m_bfgs.reset(size, m_history_size);
            // Use gradient descent for first iteration
            descent_strategy = 2;
        }

        virtual std::string name() const final override { return "L-BFGS"; }

        virtual Eigen::VectorXd compute_update_direction(
            const TVector &x,
            const TVector &grad,
            TVector &direction) override;

        virtual std::string descent_strategy_name(int descent_strategy) const override;

        virtual void increase_descent_strategy() override;
    private:
        LBFGSpp::BFGSMat<Scalar> m_bfgs; // Approximation to the Hessian matrix

        /// The number of corrections to approximate the inverse Hessian matrix.
        /// The L-BFGS routine stores the computation results of previous \ref m
        /// iterations to approximate the inverse Hessian matrix of the current
        /// iteration. This parameter controls the size of the limited memories
        /// (corrections). The default value is \c 6. Values less than \c 3 are
        /// not recommended. Large values will result in excessive computing time.
        const int m_history_size = 6;

        TVector m_prev_x;    // Previous x
        TVector m_prev_grad; // Previous gradient
                             //
        int descent_strategy; // 0, newton, 1 spd, 2 gradiant
    };
} // namespace cppoptlib

//#include "LBFGS.cpp"
