// L-BFGS solver (Using the LBFGSpp under MIT License).

#include "LBFGS.hpp"

namespace cppoptlib
{
    std::string LBFGS::descent_strategy_name(int descent_strategy) const
    {
        switch (descent_strategy)
        {
        case 1:
            return "L-BFGS";
        case 2:
            return "gradient descent";
        default:
            throw std::invalid_argument("invalid descent strategy");
        }
    }

    void LBFGS::increase_descent_strategy()
    {
        if (descent_strategy == 1)
            descent_strategy++;

        m_bfgs.reset(m_prev_x.size(), m_history_size);

        assert(descent_strategy <= 2);
    }

    Eigen::VectorXd LBFGS::compute_update_direction(
        const TVector &x,
        const TVector &grad,
        TVector &direction)
    {
        for (auto &v2s: var2sims) {
            TVector x_s, grad_s, direction_s, tmp;
            x_s = v2s->get_parametrization().eval(x);
            grad_s = v2s->get_parametrization().eval(grad);
            if (this->descent_strategy == 2)
            {
                // Use gradient descent in the first iteration or if the previous iteration failed
                direction_s = -grad_s;
            }
            else
            {
                // Update s and y
                // s_{i+1} = x_{i+1} - x_i
                // y_{i+1} = g_{i+1} - g_i
                assert(m_prev_x.size() == x_s.size());
                assert(m_prev_grad.size() == grad_s.size());
                m_bfgs.add_correction(x_s - m_prev_x, grad_s - m_prev_grad);

                // Recursive formula to compute d = -H * g
                m_bfgs.apply_Hv(grad_s, -Scalar(1), direction_s);
            }

            m_prev_x = x_s;
            m_prev_grad = grad_s;

            tmp = v2s->get_parametrization().inverse_eval(direction_s);
            if (direction.size() == 0) {
                direction.setZero(tmp.size());
            }
            assert(direction.size() == tmp.size());
            direction += tmp;
        }
        return Eigen::VectorXd();
    }
} // namespace cppoptlib
