#include "ParallelAdjointNLProblem.hpp"
#include "polyfem/solver/forms/adjoint_forms/AdjointForm.hpp"

#include <memory>
#include <polyfem/solver/forms/adjoint_forms/ParallelForm.hpp>
#include <polyfem/solver/forms/adjoint_forms/CompositeForm.hpp>
#include <polyfem/utils/Filter.hpp>
#include <polyfem/utils/Logger.hpp>
#include <polyfem/utils/MaybeParallelFor.hpp>
#include <polyfem/utils/Timer.hpp>
#include <polyfem/io/OBJWriter.hpp>
#include <polyfem/State.hpp>
#include <igl/boundary_facets.h>
#include <igl/writeOBJ.h>
#include <igl/triangle/triangulate.h>
#include <polyfem/solver/Optimizations.hpp>

#include <polyfem/solver/NLProblem.hpp>
#include <unordered_map>

namespace polyfem::solver
{
    ParallelAdjointNLProblem::ParallelAdjointNLProblem(
            const std::vector<std::shared_ptr<CompositeForm>> &forms,
            const int global,
            const std::vector<std::shared_ptr<VariableToSimulation>> &variables_to_simulation,
            const std::vector<std::shared_ptr<State>> &all_states,
            const json &args)
        : FullNLProblem({}),
          forms_(forms),
          global(global),
          variables_to_simulation_(variables_to_simulation),
          all_states_(all_states),
          solve_log_level(args["output"]["solve_log_level"]),
          save_freq(args["output"]["save_frequency"]),
          solve_in_parallel(args["solver"]["advanced"]["solve_in_parallel"]),
          better_initial_guess(args["solver"]["advanced"]["better_initial_guess"]),
          max_step_size_(args["solver"]["nonlinear"]["max_step_size"]),
          solution_is(args["output"]["solution_is"])
    {
        assert(variables_to_simulation_.size() > 0);
        //cur_grad.setZero(0);

        solve_in_order.clear();
        if (args["solver"]["advanced"]["solve_in_order"].size() > 0)
        {
            for (int i : args["solver"]["advanced"]["solve_in_order"])
                solve_in_order.push_back(i);

            if (solve_in_parallel)
                logger().error("Cannot solve both in order and in parallel, ignoring the order!");

            assert(solve_in_order.size() == all_states.size());
        }
        else
        {
            for (int i = 0; i < all_states_.size(); i++)
                solve_in_order.push_back(i);
        }

        active_state_mask.assign(all_states_.size(), false);
        for (int i = 0; i < all_states_.size(); i++)
        {
            for (const auto &v2sim : variables_to_simulation_)
            {
                for (const auto &state : v2sim->get_states())
                {
                    if (all_states_[i].get() == state.get())
                    {
                        active_state_mask[i] = true;
                        break;
                    }
                }
            }
        }
        set_gradient_methods(args["functional_compositions"]);
    }

    ParallelAdjointNLProblem::ParallelAdjointNLProblem(
                    const std::vector<std::shared_ptr<CompositeForm>> &forms,
                    const int global,
                    const std::vector<std::shared_ptr<AdjointForm>> stopping_conditions,
                    const std::vector<std::shared_ptr<VariableToSimulation>> &variables_to_simulation,
                    const std::vector<std::shared_ptr<State>> &all_states,
                    const json &args)
          : ParallelAdjointNLProblem(forms, global, variables_to_simulation, all_states, args)
    {
        stopping_conditions_ = stopping_conditions;
    }

    void ParallelAdjointNLProblem::set_gradient_methods(const polyfem::json &compositions)
    {
        assert(compositions.is_array());
        for (const auto& param: compositions) {
            if (!param.value("is_global", false)) {
                // local
                int parameter_index = param["parameter_index"];
                assert(parameter_index < variables_to_simulation_.size());
                directors.push_back(AdjointOptUtils::make_gradient_method({variables_to_simulation_[parameter_index]}, param["gradient_method"]));
            } else {
                // global
                directors.push_back(AdjointOptUtils::make_gradient_method(variables_to_simulation_, param["gradient_method"]));
            }
        }
    }

    bool ParallelAdjointNLProblem::compute_update_direction(const Eigen::VectorXd &x, const Eigen::MatrixXd &grads, Eigen::VectorXd &direction) {
        assert(grads.rows() == x.size());
        assert(grads.cols() == directors.size());
        direction.setZero(x.size());
        const auto cur_log_level = logger().level();
        Eigen::VectorXd tmp = Eigen::VectorXd::Zero(get_state(0)->ndof());
        for (int i = 0; i < grads.cols(); ++i)
        {
            all_states_[0]->set_log_level(static_cast<spdlog::level::level_enum>(solve_log_level));
            Eigen::VectorXd d;
            tmp += directors[i]->compute_update_direction(x, grads.col(i), d);
            all_states_[0]->set_log_level(cur_log_level);
            direction += d;
        }
        //direction *= 100;
        rhs_.setZero(tmp.size());
        rhs_ = tmp;
        logger().info("ParallelAdjointNLProblem::compute_update_direction() done. |direction|={}",direction.norm());
        return true;
    }

    void ParallelAdjointNLProblem::hessian(const Eigen::VectorXd &x, StiffnessMatrix &hessian)
    {
        log_and_throw_error("Hessian not supported!");
    }

    double ParallelAdjointNLProblem::value(const Eigen::VectorXd &x)
    {
        double sum = 0;
        for (const auto &form: forms_) {
            sum += form->value(x);
        }
        return sum;
    }

    Eigen::VectorXd ParallelAdjointNLProblem::values(const Eigen::VectorXd &x)
    {
        Eigen::VectorXd values;
        values.resize(forms_.size());
        int i = 0;
        for (const auto &form: forms_) {
            values(i) = form->value(x);
            ++i;
        }
        return values;
    }

    void ParallelAdjointNLProblem::gradient(const Eigen::VectorXd &x, Eigen::VectorXd &grad)
    {
        Eigen::MatrixXd grads = Eigen::MatrixXd::Zero(x.size(), n_objectives());
        gradients(x, grads);
        grad = grads.rowwise().sum();
    }

    void ParallelAdjointNLProblem::gradients(const Eigen::VectorXd &x, Eigen::MatrixXd &grads)
    {
        assert(grads.rows() == x.size());
        assert(grads.cols() == forms_.size());
        for (int i = 0; i < grads.cols(); ++i)
        {
            Eigen::VectorXd tmp_grad;
            tmp_grad.setZero(x.size());
            {
                POLYFEM_SCOPED_TIMER("gradient assembly");
                for (int i = 0; i < all_states_.size(); i++) {
                    // cache overwritted but ok b.c. used right after (first_derivative)
                    all_states_[i]->solve_adjoint_cached(forms_[i]->compute_adjoint_rhs(x, *all_states_[i])); 
                }
                forms_[i]->first_derivative(x, tmp_grad);
            }

            logger().debug("b/|grad|={}", tmp_grad.norm());
            filter_outlier(tmp_grad);
            logger().debug("a/|grad|={}", tmp_grad.norm());
            grads.col(i) = tmp_grad;
        }
    }

    bool ParallelAdjointNLProblem::is_step_valid(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) const
    {
          for (const auto& form: forms_) {
                if (!form->is_step_valid(x0, x1))
                    //log_and_throw_error("flipped");
                    return false;
                }
          return true;
    }

    bool ParallelAdjointNLProblem::is_step_collision_free(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) const
    {
                for (const auto& form: forms_) {
          if (!form->is_step_collision_free(x0, x1))
                    return false;
                }
                return true;
    }

    double ParallelAdjointNLProblem::max_step_size(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) const
    {
                double max_step_size = max_step_size_;
                for (const auto& form: forms_) {
                  double tmp = form->max_step_size(x0, x1);
          if (max_step_size > tmp)
                    max_step_size = tmp;
                }
                return max_step_size;
    }

    void ParallelAdjointNLProblem::line_search_begin(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1)
    {
                for (const auto& form: forms_) {
          form->line_search_begin(x0, x1);
                }
    }

    void ParallelAdjointNLProblem::line_search_end()
    {
                for (const auto& form: forms_) {
          form->line_search_end();
                }
    }

    void ParallelAdjointNLProblem::post_step(const int iter_num, const Eigen::VectorXd &x)
    {
        iter++;
        for (const auto& form: forms_) {
          form->post_step(iter_num, x);
        }
    }
    void ParallelAdjointNLProblem::save_to_file(const Eigen::VectorXd &x0) {
        std::unordered_map<std::string, Eigen::VectorXd> out_params;
        out_params.emplace("x0", x0);
        save_to_file(out_params);
    }
    void ParallelAdjointNLProblem::save_to_file(const std::unordered_map<std::string, Eigen::VectorXd> &out_params)
    {
        logger().info("Saving iter {}", iter);
        int id = 0;
        if (iter % save_freq != 0)
            return;
        for (const auto &state : all_states_)
        {
            bool save_vtu = true;
            bool save_rest_mesh = true;
            // for (const auto &p : parameters_)
            //     if (p->contains_state(*state))
            //     {
            //         save_vtu = true;
            //         if (p->name() == "shape")
            //             save_rest_mesh = true;
            //     }

            std::string vis_mesh_path = state->resolve_output_path(fmt::format("opt_state_{:d}_iter_{:d}.vtu", id, iter));
            std::string rest_mesh_path = state->resolve_output_path(fmt::format("opt_state_{:d}_iter_{:d}.obj", id, iter));
            id++;

            if (!save_vtu)
                continue;
            logger().debug("Save final vtu to file {} ...", vis_mesh_path);

            double tend = state->args.value("tend", 1.0);
            double dt = 1;
            if (!state->args["time"].is_null())
                dt = state->args["time"]["dt"];

            Eigen::MatrixXd sol;
            if (state->args["time"].is_null())
                sol = state->diff_cached.u(0);
            else
                sol = state->diff_cached.u(state->diff_cached.size() - 1);

            if (out_params.count(solution_is)) {
                if (solution_is == "delta_x") {
                    auto delta_x = out_params.at("delta_x");
                    sol.setZero();
                    for (auto &v2s : variables_to_simulation_)
                    {
                        if (v2s->get_parameter_type() != ParameterType::Shape)
                            continue;
                        sol += v2s->optimization_param_to_simulation_param(delta_x);
                    }
                } else if (solution_is == "rhs") {
                    auto rhs = out_params.at("rhs");
                    assert(rhs.size()==sol.size());
                    sol.setZero();
                    sol = rhs;
                } else if (solution_is == "grad") {
                    auto grad = out_params.at("grad");
                    sol.setZero();
                    for (auto &v2s : variables_to_simulation_)
                    {
                        if (v2s->get_parameter_type() != ParameterType::Shape)
                            continue;
                        sol += v2s->optimization_param_to_simulation_param(grad);
                    }
                }
            }

            state->out_geom.save_vtu(
                vis_mesh_path,
                *state,
                sol,
                Eigen::MatrixXd::Zero(state->n_pressure_bases, 1),
                tend, dt,
                io::OutGeometryData::ExportOptions(state->args, state->mesh->is_linear(), state->problem->is_scalar(), state->solve_export_to_file),
                state->is_contact_enabled(),
                state->solution_frames);

            if (!save_rest_mesh)
                continue;
            logger().debug("Save rest mesh to file {} ...", rest_mesh_path);

            // If shape opt, save rest meshes as well
            if (iter % 10 == 0) {
                std::set<int> body_ids;
                std::unordered_map<int, Eigen::MatrixXd> Vs;
                std::unordered_map<int, Eigen::MatrixXi> Es, VMs, EMs;
                std::tie(body_ids, Vs, Es, VMs, EMs) = state->export_vertices_and_edges();
                for (auto &b : body_ids) {
                    std::string mesh_path = state->resolve_output_path(fmt::format("body_{:d}_iter_{:d}.obj", b, iter));
                    Eigen::MatrixXd V, V2, H;
                    Eigen::MatrixXi E, VM, EM, F2, VM2, E2, EM2;
                    V = Vs[b];
                    E = Es[b];
                    VM = VMs[b];
                    EM = EMs[b];
                    std::vector<int> body_ids;
                    std::vector<int> boundary_ids;
                    const std::string flags = "pqa0.005Y";
                    igl::triangle::triangulate(V, E, H, VM, EM, flags, V2, F2, VM2, E2, EM2);
                    io::OBJWriter::write(mesh_path, V2, F2);
                }
            }
        }
    }

    void ParallelAdjointNLProblem::solution_changed(const Eigen::VectorXd &newX)
    {
        bool need_rebuild_basis = false;

        // update to new parameter and check if the new parameter is valid to solve
        for (const auto &v : variables_to_simulation_)
        {
            v->update(newX);
            if (v->get_parameter_type() == ParameterType::Shape)
                need_rebuild_basis = true;
        }

        if (need_rebuild_basis)
        {
            const auto cur_log_level = logger().level();
            all_states_[0]->set_log_level(static_cast<spdlog::level::level_enum>(solve_log_level)); // log level is global, only need to change in one state
            for (const auto &state : all_states_)
                state->build_basis();
            all_states_[0]->set_log_level(cur_log_level);
        }

        // solve PDE
        solve_pde();

                for (const auto& form: forms_) {
          form->solution_changed(newX);
                }
    }

    void ParallelAdjointNLProblem::solve_pde()
    {
        const auto cur_log_level = logger().level();
        all_states_[0]->set_log_level(static_cast<spdlog::level::level_enum>(solve_log_level)); // log level is global, only need to change in one state
        
        if (solve_in_parallel)
        {
            logger().info("Run simulations in parallel...");

            utils::maybe_parallel_for(all_states_.size(), [&](int start, int end, int thread_id) {
                for (int i = start; i < end; i++)
                {
                    auto state = all_states_[i];
                    if (active_state_mask[i] || state->diff_cached.size() == 0)
                    {
                        state->assemble_rhs();
                        state->assemble_mass_mat();
                        Eigen::MatrixXd sol, pressure; // solution is also cached in state
                        state->solve_problem(sol, pressure);
                    }
                }
            });
        }
        else
        {
            Eigen::MatrixXd sol, pressure; // solution is also cached in state
            for (int i : solve_in_order)
            {
                auto state = all_states_[i];
                if (active_state_mask[i] || state->diff_cached.size() == 0)
                {
                    state->assemble_rhs();
                    state->assemble_mass_mat();

                    state->solve_problem(sol, pressure);
                }
            }
        }

        all_states_[0]->set_log_level(cur_log_level);

        //cur_grad.resize(0);
    }

    bool ParallelAdjointNLProblem::stop(const TVector &x)
    {
        if (stopping_conditions_.size() == 0)
            return false;

        for (auto &obj : stopping_conditions_)
        {
            obj->solution_changed(x);
            if (obj->value(x) > 0)
                return false;
        }
        return true;
    }

        bool ParallelAdjointNLProblem::remesh() {
                for (auto &state: all_states_) {
                    if (!state->remesh_2d_with_triangle()) {
                          return false;
                    }
                }
                reinit_forms();
                return true;
        }

        void ParallelAdjointNLProblem::reinit_forms() { 
            for (auto &form: forms_) {
                form->init_form();
            }
        }
} // namespace polyfem::solver
