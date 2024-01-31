#include "AdjointNLProblem.hpp"

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
#include <polyfem/solver/NLProblem.hpp>
#include <polyfem/utils/Overlapping.hpp>

namespace polyfem::solver
{
	AdjointNLProblem::AdjointNLProblem(std::shared_ptr<CompositeForm> composite_form, const std::vector<std::shared_ptr<VariableToSimulation>> &variables_to_simulation, const std::vector<std::shared_ptr<State>> &all_states, const json &args)
		: FullNLProblem({composite_form}),
		  composite_form_(composite_form),
		  variables_to_simulation_(variables_to_simulation),
		  all_states_(all_states),
		  solve_log_level(args["output"]["solve_log_level"]),
		  save_freq(args["output"]["save_frequency"]),
		  solve_in_parallel(args["solver"]["advanced"]["solve_in_parallel"]),
		  better_initial_guess(args["solver"]["advanced"]["better_initial_guess"]),
                  max_step_size_(args["solver"]["nonlinear"]["max_step_size"]),
                  solution_is(args["output"]["solution_is"]),
                  variance_(args["grad_filter"]["variance"]),
                  abs_max_(args["grad_filter"]["abs_max"])
	{
		cur_grad.setZero(0);

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
	}

	AdjointNLProblem::AdjointNLProblem(std::shared_ptr<CompositeForm> composite_form, const std::vector<std::shared_ptr<AdjointForm>> stopping_conditions, const std::vector<std::shared_ptr<VariableToSimulation>> &variables_to_simulation, const std::vector<std::shared_ptr<State>> &all_states, const json &args) : AdjointNLProblem(composite_form, variables_to_simulation, all_states, args)
	{
		stopping_conditions_ = stopping_conditions;
	}

	void AdjointNLProblem::hessian(const Eigen::VectorXd &x, StiffnessMatrix &hessian)
	{
		log_and_throw_error("Hessian not supported!");
	}

	double AdjointNLProblem::value(const Eigen::VectorXd &x)
	{
		return composite_form_->value(x);
	}
        void AdjointNLProblem::project_to_normal(Eigen::VectorXd &x) {
                Eigen::VectorXd out;
                out.setZero(x.size());
                for (const auto& v2s : variables_to_simulation_) {
                    auto x_ = v2s->optimization_param_to_simulation_param(x);
                    auto state = all_states_[0];
                    const int dim = state->mesh->dimension();
                    Eigen::MatrixXd mat = polyfem::utils::unflatten(x_, dim);
                    Eigen::MatrixXd out_mat = Eigen::MatrixXd::Zero(mat.rows(), mat.cols());
                    for (auto &lb: state->total_local_boundary) {
                        assert(lb.type() == polyfem::mesh::BoundaryType::TRI_LINE);
                        const int e = lb.element_id();

                        Eigen::MatrixXd uv, samples, gtmp, rhs_fun, deform_mat, trafo;
                        Eigen::MatrixXd points, normals;
                        Eigen::VectorXd weights, normal;

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
                x = out;
        }
        void AdjointNLProblem::set_csv_writer(const json &args, const Eigen::VectorXd &x) {
                if (args["output"].contains("save_csv_path")) {
                    auto state = all_states_[0];
                    csv_writer = 
                      std::make_unique<io::OptCSVWriter>(
                        state->resolve_output_path(args["output"]["save_csv_path"]),
                        state->solve_data,
                        composite_form_,
                        x,
                        iter_ == 0);
                }
        }
	void AdjointNLProblem::gradient(const Eigen::VectorXd &x, Eigen::VectorXd &gradv)
	{
		if (cur_grad.size() == x.size())
			gradv = cur_grad;
		else
		{
			gradv.setZero(x.size());

			{
				POLYFEM_SCOPED_TIMER("adjoint solve");
				for (int i = 0; i < all_states_.size(); i++)
					all_states_[i]->solve_adjoint_cached(composite_form_->compute_adjoint_rhs(x, *all_states_[i])); // caches inside state
			}

			{
				POLYFEM_SCOPED_TIMER("gradient assembly");
				composite_form_->first_derivative(x, gradv);
			}

                        //project_to_normal(gradv); 
                        filter_outlier(gradv, variance_, abs_max_);
			cur_grad = gradv;
		}

                { // debugging
                    out_params.clear();
                    auto form = composite_form_;
                    auto state = all_states_[0];
                    //if (solution_is == "grad") {
                    {
                        Eigen::VectorXd out;
                        Eigen::VectorXd grad_;
                        form->first_derivative(x, grad_);
                        project_to_normal(grad_); 
                        filter_outlier(grad_, variance_, abs_max_);
                        for (auto& v2s: variables_to_simulation_) {
                            if (!out.size())
                              out = v2s->optimization_param_to_simulation_param(-grad_);         // ! gradient display is -1 * grad !
                            else
                              out += v2s->optimization_param_to_simulation_param(-grad_);         // ! gradient display is -1 * grad !
                        }
                        assert(out.norm() > 0 && abs(out.norm() - grad_.norm()) < 1e-10);
                        out_params.emplace("grad", out);
                    } //else if (solution_is == "partial_grad")
                    {
                        Eigen::VectorXd out;
                        Eigen::VectorXd grad_;
                        form->compute_partial_gradient_unweighted(x, grad_);
                        filter_outlier(grad_, variance_, abs_max_);
                        for (auto& v2s: variables_to_simulation_) {
                            if (!out.size())
                              out = v2s->optimization_param_to_simulation_param(-grad_);         // ! gradient display is -1 * grad !
                            else
                              out += v2s->optimization_param_to_simulation_param(-grad_);         // ! gradient display is -1 * grad !
                        }
                        assert(abs(out.norm() - grad_.norm()) < 1e-10);
                        out_params.emplace("partial_grad", out);
                    }
                    //if (solution_is == "adjoint_term")
                    state->solve_adjoint_cached(form->compute_adjoint_rhs(x, *state));
                    Eigen::VectorXd adjoint_term;
                    for (auto& v2s: variables_to_simulation_) {
                        if (!adjoint_term.size())
                          adjoint_term = v2s->compute_adjoint_term(x);
                        else
                          adjoint_term += v2s->compute_adjoint_term(x);
                    }
                    {
                        Eigen::VectorXd out;
                        filter_outlier(adjoint_term, variance_, abs_max_);
                        for (auto& v2s: variables_to_simulation_) {
                            if (!out.size())
                              out = v2s->optimization_param_to_simulation_param(-adjoint_term);         // ! gradient display is -1 * grad !
                            else
                              out += v2s->optimization_param_to_simulation_param(-adjoint_term);         // ! gradient display is -1 * grad !
                        }
                        out_params.emplace("adjoint_term", out);
                    }
                    for (auto& v2s: variables_to_simulation_) {
                        auto shape_v2s = std::dynamic_pointer_cast<ShapeVariableToSimulation>(v2s);
                        Eigen::VectorXd elasticity_term, body_term, contact_term;
                        Eigen::VectorXd display, out;
                        std::tie(elasticity_term, body_term, contact_term) = shape_v2s->compute_adjoint_terms(x);
                        {
                            Eigen::VectorXd one_form = -1 * (elasticity_term+body_term+contact_term);
                            one_form = utils::flatten(utils::unflatten(one_form, state->mesh->dimension())(state->primitive_to_node(), Eigen::all));
                            one_form = v2s->apply_parametrization_jacobian(one_form, x);
                            if (!out.size())
                              out = v2s->optimization_param_to_simulation_param(-adjoint_term);         // ! gradient display is -1 * grad !
                            else
                              out += v2s->optimization_param_to_simulation_param(-adjoint_term);         // ! gradient display is -1 * grad !
                            filter_outlier(one_form, variance_, abs_max_);
                            auto error = adjoint_term.norm() - one_form.norm();
                            //assert(abs(error) < 1e-10);
                        }
                        {
                            elasticity_term *= -1;
                            elasticity_term = utils::flatten(utils::unflatten(elasticity_term, state->mesh->dimension())(state->primitive_to_node(), Eigen::all));
                            elasticity_term = v2s->apply_parametrization_jacobian(elasticity_term, x);
                            display = elasticity_term;
                            filter_outlier(display, variance_, abs_max_);
                            out = v2s->optimization_param_to_simulation_param(-display);         // ! gradient display is -1 * grad !
                            out_params.emplace("elasticity_term", out);
                        } //else if (solution_is == "body_term") // rhs_term
                        {
                            body_term *= -1;
                            body_term = utils::flatten(utils::unflatten(body_term, state->mesh->dimension())(state->primitive_to_node(), Eigen::all));
                            body_term = v2s->apply_parametrization_jacobian(body_term, x);
                            display = body_term;
                            filter_outlier(display, variance_, abs_max_);
                            out = v2s->optimization_param_to_simulation_param(-display);         // ! gradient display is -1 * grad !
                            out_params.emplace("body_term", out);
                        } //else if (solution_is == "contact_term")
                        {
                            contact_term *= -1;
                            contact_term = utils::flatten(utils::unflatten(contact_term, state->mesh->dimension())(state->primitive_to_node(), Eigen::all));
                            contact_term = v2s->apply_parametrization_jacobian(contact_term, x);
                            display = contact_term;
                            filter_outlier(display, variance_, abs_max_);
                            out = v2s->optimization_param_to_simulation_param(-display);         // ! gradient display is -1 * grad !
                            out_params.emplace("contact_term", out);
                        }
                    }
                }
	}

	bool AdjointNLProblem::is_step_valid(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) const
	{
		return composite_form_->is_step_valid(x0, x1);
	}

	bool AdjointNLProblem::is_step_collision_free(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) const
	{
		return composite_form_->is_step_collision_free(x0, x1);
	}

	double AdjointNLProblem::max_step_size(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1) const
	{
		return std::min(max_step_size_,composite_form_->max_step_size(x0, x1));
	}

	void AdjointNLProblem::line_search_begin(const Eigen::VectorXd &x0, const Eigen::VectorXd &x1)
	{
		composite_form_->line_search_begin(x0, x1);
	}

	void AdjointNLProblem::line_search_end()
	{
		composite_form_->line_search_end();
	}

	void AdjointNLProblem::post_step(const int iter_num, const Eigen::VectorXd &x)
	{
		iter_++;
		composite_form_->post_step(iter_num, x);
	}
        void AdjointNLProblem::save_to_file(const Eigen::VectorXd &x0)
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
			// 	if (p->contains_state(*state))
			// 	{
			// 		save_vtu = true;
			// 		if (p->name() == "shape")
			// 			save_rest_mesh = true;
			// 	}

			std::string vis_mesh_path = state->resolve_output_path(fmt::format("opt_iter_{:d}.vtu", iter_));
			std::string rest_mesh_path = state->resolve_output_path(fmt::format("opt_iter_{:d}.obj", iter_));
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

                        /*
                        {
                            Eigen::VectorXd delta_x_ext;
                            delta_x_ext.setZero(sol.size());
                            auto delta_x = x0;
                            for (auto &v2s : variables_to_simulation_)
                            {
                                if (v2s->get_parameter_type() != ParameterType::Shape)
                                    continue;
                                delta_x_ext += v2s->optimization_param_to_simulation_param(delta_x);
                            }
                            out_params.emplace("delta_x", delta_x_ext);
                        }
                        */
                        if (csv_writer) {
                            // write stats to csv file
                            csv_writer->write(iter_, x0);
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

			// If shape opt, save rest meshes as well
                        if (iter_ > 0 && iter_ % 10 == 0) {
                                // export mesh
                                std::set<int> body_ids;
                                std::unordered_map<int, Eigen::MatrixXd> Vs;
                                std::unordered_map<int, Eigen::MatrixXi> Es, VMs, EMs;
                                std::tie(body_ids, Vs, Es, VMs, EMs) = state->export_vertices_and_edges();
                                for (auto &b : body_ids) {
                                    std::string mesh_path = state->resolve_output_path(fmt::format("body_{:d}_iter_{:d}.obj", b, iter_));
                                    if (b == 1) {
                                        mesh_path = state->resolve_output_path(fmt::format("run_concave_iter_{:d}.obj", iter_));
                                    } else if (b == 2){
                                        mesh_path = state->resolve_output_path(fmt::format("run_convex_iter_{:d}.obj", iter_));
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
			Eigen::MatrixXd V;
			Eigen::MatrixXi F;
			state->get_vertices(V);
			state->get_elements(F);
			if (state->mesh->dimension() == 3)
				F = igl::boundary_facets<Eigen::MatrixXi, Eigen::MatrixXi>(F);

			io::OBJWriter::write(rest_mesh_path, V, F);
		}
	}
	void AdjointNLProblem::solution_changed(const Eigen::VectorXd &newX)
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

		composite_form_->solution_changed(newX);
	}

	void AdjointNLProblem::solve_pde()
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

		cur_grad.resize(0);
	}

	bool AdjointNLProblem::stop(const TVector &x)
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

        bool AdjointNLProblem::remesh() {
                for (auto &state: all_states_) {
                    if (state->mesh->is_volume() || !state->remesh_2d_with_triangle()) {
                          return false;
                    }
                }
                reinit_forms();
                return true;
        }

        void AdjointNLProblem::reinit_forms() { 
            composite_form_->init_form();
        }
} // namespace polyfem::solver