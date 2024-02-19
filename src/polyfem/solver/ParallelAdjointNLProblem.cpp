#include "ParallelAdjointNLProblem.hpp"
#include "polyfem/io/OutData.hpp"
#include "polyfem/solver/forms/adjoint_forms/AdjointForm.hpp"
#include "polyfem/solver/forms/adjoint_forms/VariableToSimulation.hpp"
#include "polyfem/utils/MatrixUtils.hpp"
#include "polyfem/utils/StringUtils.hpp"

#include <polyfem/solver/forms/adjoint_forms/ParallelForm.hpp>
#include <polyfem/solver/forms/adjoint_forms/CompositeForm.hpp>
#include <polyfem/solver/forms/BodyForm.hpp>
#include <polyfem/solver/forms/ElasticForm.hpp>
#include <polyfem/solver/forms/ContactForm.hpp>
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
#include <polyfem/utils/Overlapping.hpp>

#include <polyfem/solver/NLProblem.hpp>
#include <units/unit_definitions.hpp>
#include <unordered_map>

#include <polyfem/assembler/Mass.hpp>
namespace polyfem::solver
{
    ParallelAdjointNLProblem::ParallelAdjointNLProblem(
            const std::vector<std::shared_ptr<CompositeForm>> &forms,
            const int global,
            const std::vector<std::shared_ptr<VariableToSimulation>> &variables_to_simulation,
            const std::vector<std::shared_ptr<State>> &all_states,
            const Eigen::VectorXd &x,
            const int iter,
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
          solution_is(args["output"]["solution_is"]),                   // true or false
          display_form_index_(args["output"]["display_form_index"]),     // 0 or 1, if both, -1
          variance_(args["grad_filter"]["variance"]),                   // double
          abs_max_(args["grad_filter"]["abs_max"])                      // double
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
        set_iter(iter);
        if (args["output"].contains("save_csv_path")) {
            auto state = all_states_[0];
            auto name = args["output"]["save_csv_path"].get<std::string>();
            energy_writer = 
              std::make_unique<io::POptCSVWriter>(
                state->resolve_output_path(name),
                state->solve_data,
                forms_,
                x,
                iter == 0);
            const std::vector<std::string> names = {"angle", "norm", "term"};
            general_writer = 
              std::make_unique<io::ScatterCSVWriter>(
                state->resolve_output_path("gdn_" + name),
                names,
                iter == 0);
        }
    }

    ParallelAdjointNLProblem::ParallelAdjointNLProblem(
                    const std::vector<std::shared_ptr<CompositeForm>> &forms,
                    const int global,
                    const std::vector<std::shared_ptr<AdjointForm>> stopping_conditions,
                    const std::vector<std::shared_ptr<VariableToSimulation>> &variables_to_simulation,
                    const std::vector<std::shared_ptr<State>> &all_states,
                    const Eigen::VectorXd &x,
                    const int iter,
                    const json &args)
          : ParallelAdjointNLProblem(forms, global, variables_to_simulation, all_states, x, iter, args)
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
        atns.clear();
        const auto cur_log_level = logger().level();
        Eigen::VectorXd rhs = Eigen::VectorXd::Zero(get_state(0)->ndof());
        for (int i = 0; i < grads.cols(); ++i)
        {
            all_states_[0]->set_log_level(static_cast<spdlog::level::level_enum>(solve_log_level));
            Eigen::VectorXd d,g;
            g = grads.col(i);
            Eigen::VectorXd tmp = directors[i]->compute_update_direction(x, g, d);
            if (tmp.size() == rhs.size()) {
                rhs += tmp;
            }
            all_states_[0]->set_log_level(cur_log_level);
            //p_project_to_normal(d, directors[i]->get_v2s());
            direction += d;

            if (i != global) {
        std::cout<< "reached here: l." << __LINE__ << "." << __FILE__ << std::endl;
                log_angle_to_normal(x, d, i);
            }
        }
        //direction *= 100;
        /*
        if (solution_is == "rhs")
        {
            display_.setZero(rhs.size());
            display_ = rhs;
        }
        */
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

                //p_project_to_normal(tmp_grad, variables_to_simulation_); 
            }
            utils::filter_outlier(tmp_grad, variance_, abs_max_);
            grads.col(i) = tmp_grad;
        }
        log_gradient_terms(x);
    }

    void ParallelAdjointNLProblem::log_angle_to_normal(const Eigen::VectorXd& x, const Eigen::VectorXd &target, const int idx) 
    { // to get additional info about the angle between the shape grads and the surface normals
        const double PI=3.14159;
        auto state = all_states_[0];
        Eigen::VectorXd term, partial_grad, contact_force;
        term.setZero(target.size());
        {   // compute contact force
            auto v2s = variables_to_simulation_[idx];
            auto shape_v2s = std::dynamic_pointer_cast<ShapeVariableToSimulation>(v2s);
            state->solve_data.contact_form->first_derivative(state->diff_cached.u(0), contact_force);
            contact_force = v2s->simulation_param_to_optimization_param(contact_force);
            //contact_term = state.gbasis_nodes_to_basis_nodes * contact_term;
            term = contact_force;
        }
        std::cout << target.size() << " vs " << term.size() << std::endl;
        assert(target.size() == term.size());
        Eigen::VectorXd dots, angles;
        Eigen::VectorXi indexing;
        std::vector<double> filtered_dots;
        std::vector<int> filter_indices;
        dots = p_abs_dot_to_normal(target, variables_to_simulation_);
        for (int i = 0; i < dots.size(); ++i) {
            if (dots(i) >= 0) {
                filtered_dots.push_back(dots(i));
                filter_indices.push_back(i);
            }
        }
        angles.resize(filtered_dots.size());
        indexing.resize(filter_indices.size());
        for (int i = 0; i < filtered_dots.size(); ++i) {
            angles(i) = std::acos(filtered_dots[i]) * 180 / PI;
            indexing(i) = filter_indices[i];
        }
        Eigen::MatrixXd target_mat = 
          utils::unflatten(target, all_states_[0]->mesh->dimension());
        Eigen::MatrixXd term_mat = 
          utils::unflatten(term, all_states_[0]->mesh->dimension());
        Eigen::VectorXd norms = target_mat.rowwise().norm()(indexing);
        Eigen::VectorXd term_norms = term_mat.rowwise().norm()(indexing);
        /*
        for (int i = 0; i < norms.size(); ++i) {
          if (abs(norms(i)) > 0)
            term_norms(i) /= norms(i);
        }
        */
        Eigen::MatrixXd atn;
        assert(angles.rows() == norms.rows());
        assert(angles.rows() == term_norms.rows());
        atn.resize(angles.rows(), angles.cols() + norms.cols() + term_norms.cols());
        atn << angles, norms, term_norms;
        atns.push_back(atn);
    }

    void ParallelAdjointNLProblem::log_gradient_terms(const Eigen::VectorXd &x) 
    { // debugging ==================================================================
        out_params.clear();
        //display_v2s_index_ = 1; // 0 or 1 for now
        //auto v2s = variables_to_simulation_[display_v2s_index_];
        auto state = all_states_[0];

        //if (solution_is == "grad") {
        Eigen::VectorXd grad_display, partial_grad_display, adjoint_term_display, elasticity_term_display, body_term_display, contact_term_display;
        { // initialize
            auto sol_size = state->ndof();
            grad_display.setZero(sol_size);
            partial_grad_display.setZero(sol_size);
            adjoint_term_display.setZero(sol_size);
            elasticity_term_display.setZero(sol_size);
            body_term_display.setZero(sol_size);
            contact_term_display.setZero(sol_size);
        }
        int form_index = 0;
        for (const auto&form: forms_) {
            const int idx = form_index;
            form_index++;
            if (idx == global)
                continue;
            if (display_form_index_ >= 0 && display_form_index_ != idx)
                continue;

            auto v2s = variables_to_simulation_[idx]; // assuming form and v2s has the same index
            { // grad
                Eigen::VectorXd grad_;
                form->first_derivative(x, grad_);
                //p_project_to_normal(grad_, variables_to_simulation_); 
                utils::filter_outlier(grad_, variance_, abs_max_);
                grad_display += v2s->optimization_param_to_simulation_param(-grad_);         // ! gradient display is -1 * grad !
            } // partial_grad
            {
                Eigen::VectorXd grad_;
                form->compute_partial_gradient_unweighted(x, grad_);
                //filter_outlier(grad_, variance_, abs_max_);
                partial_grad_display += v2s->optimization_param_to_simulation_param(-grad_);         // ! gradient display is -1 * grad !
            }

            state->solve_adjoint_cached(form->compute_adjoint_rhs(x, *state));
            const auto adjoint_term_ = v2s->compute_adjoint_term(x);
            { // adjoint_term
                //filter_outlier(adjoint_term, variance_, abs_max_);
                adjoint_term_display += v2s->optimization_param_to_simulation_param(-adjoint_term_);         // ! gradient display is -1 * grad !
            }
            if (v2s->get_parameter_type() == ParameterType::Shape) {
                auto shape_v2s = std::dynamic_pointer_cast<ShapeVariableToSimulation>(v2s);
                Eigen::VectorXd elasticity_term, body_term, contact_term;
                Eigen::VectorXd display;
                std::tie(elasticity_term, body_term, contact_term) = shape_v2s->compute_adjoint_terms(x);
                { // test
                    Eigen::VectorXd one_form = -1 * (elasticity_term+body_term+contact_term);
                    one_form = utils::flatten(utils::unflatten(one_form, state->mesh->dimension())(state->primitive_to_node(), Eigen::all));
                    one_form = v2s->apply_parametrization_jacobian(one_form, x);
                    //filter_outlier(one_form, variance_, abs_max_);
                    auto error = adjoint_term_.norm() - one_form.norm();
                    //assert(abs(error) < 1e-10);
                }
                { // elasticity_term
                    elasticity_term *= -1;
                    elasticity_term = utils::flatten(utils::unflatten(elasticity_term, state->mesh->dimension())(state->primitive_to_node(), Eigen::all));
                    elasticity_term = v2s->apply_parametrization_jacobian(elasticity_term, x);
                    display = elasticity_term;
                    //filter_outlier(display, variance_, abs_max_);
                    elasticity_term_display += v2s->optimization_param_to_simulation_param(-display);         // ! gradient display is -1 * grad !
                }
                { // body_term, or rhs_term
                    body_term *= -1;
                    body_term = utils::flatten(utils::unflatten(body_term, state->mesh->dimension())(state->primitive_to_node(), Eigen::all));
                    body_term = v2s->apply_parametrization_jacobian(body_term, x);
                    display = body_term;
                    //filter_outlier(display, variance_, abs_max_);
                    body_term_display += v2s->optimization_param_to_simulation_param(-display);         // ! gradient display is -1 * grad !
                }
                { // contact_term
                    contact_term *= -1;
                    contact_term = utils::flatten(utils::unflatten(contact_term, state->mesh->dimension())(state->primitive_to_node(), Eigen::all));
                    contact_term = v2s->apply_parametrization_jacobian(contact_term, x);
                    display = contact_term;
                    //filter_outlier(display, variance_, abs_max_);
                    contact_term_display += v2s->optimization_param_to_simulation_param(-display);         // ! gradient display is -1 * grad !
                }
            }
        } // ========================================================================
        /*
        Eigen::MatrixXd rhs;
        {       // rhs
            state->solve_data.rhs_assembler->assemble(state->mass_matrix_assembler->density(), rhs);
            state->solve_data.rhs_assembler->set_bc(
                  state->local_boundary, state->boundary_nodes, state->n_boundary_samples(),
                  state->local_neumann_boundary , rhs);
            assert(grad_display.size() == rhs.size());
        }
        out_params.emplace("rhs", rhs);
        */
        out_params.emplace("grad", grad_display);
        out_params.emplace("partial_grad", partial_grad_display);
        out_params.emplace("adjoint_term", adjoint_term_display);
        out_params.emplace("elasticity_term", elasticity_term_display);
        out_params.emplace("body_term", body_term_display);
        out_params.emplace("contact_term", contact_term_display);
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
        iter_++;
        for (const auto& form: forms_) {
          form->post_step(iter_num, x);
        }
    }
    void ParallelAdjointNLProblem::save_to_file(const Eigen::VectorXd &x0) {
        save_to_file(x0, Eigen::VectorXd::Zero(x0.size()), 0);
    }
    void ParallelAdjointNLProblem::save_to_file(const Eigen::VectorXd &x, const Eigen::VectorXd &delta_x, const double step_rate)
    {
        logger().info("Saving iter {}", iter_);
        int id = 0;
        if (iter_ % save_freq != 0)
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

            std::string vis_mesh_path = state->resolve_output_path(fmt::format("parallel_iter_{:d}.vtu", iter_));
            //std::string rest_mesh_path = state->resolve_output_path(fmt::format("parallel_iter_{:d}.obj", iter_));
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

            {
                // add delta_x to paraview output
                Eigen::MatrixXd delta_x_ext;
                delta_x_ext.setZero(sol.rows(), sol.cols());
                for (auto &v2s : variables_to_simulation_)
                {
                    if (v2s->get_parameter_type() != ParameterType::Shape)
                        continue;
                    delta_x_ext += v2s->optimization_param_to_simulation_param(delta_x);
                }
                out_params.emplace("delta_x", delta_x_ext);
            }
            
            state->out_geom.save_vtu(
                vis_mesh_path,
                *state,
                sol,
                Eigen::MatrixXd::Zero(state->n_pressure_bases, 1),
                tend, dt,
                io::OutGeometryData::ExportOptions(state->args, state->mesh->is_linear(), state->problem->is_scalar(), state->solve_export_to_file),
                state->is_contact_enabled(),
                state->solution_frames,
                out_params);

            /*
            if (!save_rest_mesh)
                continue;
            logger().debug("Save rest mesh to file {} ...", rest_mesh_path);
            */

            if (iter_ == 0 || iter_ % 10 != 0) {
                // write to csv
                if (energy_writer) {
                    // write stats to csv file
                    energy_writer->write(iter_, x, delta_x, step_rate);
                }
                if (general_writer) {
                    for (const auto& data: atns) {
                        general_writer->write(data);
                    }
                }
            }
            if (iter_ > 0 && iter_ % 10 == 0) { // TODO modify "iter_ > 0"
                // export mesh
                std::set<int> body_ids;
                std::unordered_map<int, Eigen::MatrixXd> Vs;
                std::unordered_map<int, Eigen::MatrixXi> Es, VMs, EMs;
                std::tie(body_ids, Vs, Es, VMs, EMs) = state->export_vertices_and_edges();
                for (auto &b : body_ids) {
                    std::string mesh_path = state->resolve_output_path(fmt::format("body_{:d}_iter_{:d}.obj", b, iter_));
                    if (b == 1) {
                        mesh_path = state->resolve_output_path(fmt::format("concave_iter_{:d}.obj", iter_));
                    } else if (b == 2){
                        mesh_path = state->resolve_output_path(fmt::format("convex_iter_{:d}.obj", iter_));
                    }
                    Eigen::MatrixXd V, V2, H;
                    Eigen::MatrixXi E, VM, EM, F2, VM2, E2, EM2;
                    V = Vs[b];
                    E = Es[b];
                    VM = VMs[b];
                    EM = EMs[b];
                    std::vector<int> body_ids;
                    std::vector<int> boundary_ids;
                    const std::string flags = "pq25a0.003";
                    igl::triangle::triangulate(V, E, H, VM, EM, flags, V2, F2, VM2, E2, EM2);
                    io::OBJWriter::write(mesh_path, V2, F2);
                }
            }
        }
    }

    /*
    void ParallelAdjointNLProblem::log_contact_force(const Eigen::VectorXd &x) {
        auto state = all_states_[0];
        auto sol = state->diff_cached.u(0);
        state->solve_data.contact_form->value_per_element(sol);
    }
    */
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

    // if vector's norm is equal to zero, corresponding dot value is equal to -1.
    Eigen::VectorXd ParallelAdjointNLProblem::p_abs_dot_to_normal(const Eigen::VectorXd &x, std::vector<std::shared_ptr<polyfem::solver::VariableToSimulation>> var2sims) {
        auto state = all_states_[0];
        const int dim = state->mesh->dimension();
        Eigen::VectorXd out, out_s;
        out_s.setZero(x.size());
        out.resize(x.size()/dim);
        for (const auto& v2s : var2sims) {
            auto x_ = v2s->optimization_param_to_simulation_param(x);
            Eigen::MatrixXd mat = polyfem::utils::unflatten(x_, dim);
            Eigen::MatrixXd out_mat = Eigen::MatrixXd::Constant(mat.rows(), mat.cols(), -1);
            for (auto &lb: state->total_local_boundary) {
                assert(lb.type() == polyfem::mesh::BoundaryType::TRI_LINE);
                const int e = lb.element_id();

                Eigen::VectorXd normal;
                for (int i = 0; i < lb.size(); ++i) {
                    int gid = lb.global_primitive_id(i);
                    polyfem::get_normal_outward(*state, e, gid, normal);
                    for (int lv_id = 0; lv_id < dim; ++lv_id) {
                        auto v_id = state->mesh->boundary_element_vertex(gid, lv_id);
                        if (v_id < 0) {
                            continue;
                        }
                        assert(out_mat.cols() == normal.rows());
                        auto n_id = state->mesh_nodes->primitive_to_node()[v_id];
                        auto v = mat.row(n_id);
                        v.normalize();
                        if (v.norm() > 0) {
                            out_mat(n_id, 0) = abs(v.dot(normal));
                        }
                    }
                }
            }
            out_s += v2s->simulation_param_to_optimization_param(polyfem::utils::flatten(out_mat));
        }
        for (int i = 0; i < out.size(); ++i) {
              out(i) = out_s(i * dim);
        }
        return out;
    }
    Eigen::VectorXd ParallelAdjointNLProblem::p_project_to_normal(Eigen::VectorXd &x, std::vector<std::shared_ptr<polyfem::solver::VariableToSimulation>> var2sims) {
        auto state = all_states_[0];
        const int dim = state->mesh->dimension();
        Eigen::VectorXd out;
        out.setZero(x.size());
        for (const auto& v2s : var2sims) {
            auto x_ = v2s->optimization_param_to_simulation_param(x);
            Eigen::MatrixXd mat = polyfem::utils::unflatten(x_, dim);
            Eigen::MatrixXd out_mat = Eigen::MatrixXd::Zero(mat.rows(), mat.cols());
            for (auto &lb: state->total_local_boundary) {
                assert(lb.type() == polyfem::mesh::BoundaryType::TRI_LINE);
                const int e = lb.element_id();

                Eigen::VectorXd normal;
                for (int i = 0; i < lb.size(); ++i) {
                    int gid = lb.global_primitive_id(i);
                    polyfem::get_normal_outward(*state, e, gid, normal);
                    for (int lv_id = 0; lv_id < dim; ++lv_id) {
                        auto v_id = state->mesh->boundary_element_vertex(gid, lv_id);
                        if (v_id < 0) {
                            continue;
                        }
                        assert(out_mat.cols() == normal.rows());
                        auto n_id = state->mesh_nodes->primitive_to_node()[v_id];
                        out_mat.row(n_id) += mat.row(n_id).dot(normal) * normal.transpose();
                    }
                }
            }
            out += v2s->simulation_param_to_optimization_param(polyfem::utils::flatten(out_mat));
        }
        return out;
    }
} // namespace polyfem::solver
