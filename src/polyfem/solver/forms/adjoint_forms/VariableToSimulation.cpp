#include "VariableToSimulation.hpp"
#include "polyfem/utils/Types.hpp"
#include <Eigen/src/Core/ArithmeticSequence.h>
#include <polyfem/State.hpp>
#include <polyfem/solver/forms/parametrization/SDFParametrizations.hpp>
#include <polyfem/assembler/ViscousDamping.hpp>

#include <polyfem/mesh/mesh2D/Mesh2D.hpp>
#include <polyfem/mesh/mesh3D/Mesh3D.hpp>
#include "polyfem/utils/Filter.hpp"

namespace polyfem::solver
{
	Eigen::VectorXi VariableToSimulation::get_output_indexing(const Eigen::VectorXd &x) const
	{
		const int out_size = parametrization_.size(x.size());
		if (output_indexing_.size() == out_size)
			return output_indexing_;
		else if (output_indexing_.size() == 0)
		{
			Eigen::VectorXi ind;
			ind.setLinSpaced(out_size, 0, out_size - 1);
			return ind;
		}
		else
			log_and_throw_error(fmt::format("Indexing size and output size of the Parametrization do not match! {} vs {}", output_indexing_.size(), out_size));
		return Eigen::VectorXi();
	}

	Eigen::VectorXd VariableToSimulation::apply_parametrization_jacobian(const Eigen::VectorXd &term, const Eigen::VectorXd &x) const
	{
		return parametrization_.apply_jacobian(term(get_output_indexing(x)), x);
	}

	Eigen::VectorXd VariableToSimulation::inverse_eval()
	{
		log_and_throw_error("Not implemented!");
		return Eigen::VectorXd();
	}

        Eigen::VectorXd VariableToSimulation::optimization_param_to_simulation_param(const Eigen::VectorXd &opt_param) const
        {
		assert(get_parameter_type() == ParameterType::Shape);
                auto state = get_state();
                const int dim = state->mesh->dimension();
                Eigen::VectorXd sim_param;
                sim_param.resize(state->ndof(), 1);
                sim_param.setZero();
		auto reduced_opt_param = parametrization_.eval(opt_param);
		auto output_indexing = get_output_indexing(opt_param);
                assert(reduced_opt_param.size()==output_indexing.size());
                for (int i = 0; i < output_indexing.size(); i+=dim){
                    int v_id = output_indexing(i) / dim;
                    int n_id = state->mesh_nodes->primitive_to_node()[v_id];
                    sim_param.block(n_id * dim, 0, dim, 1) = reduced_opt_param(Eigen::seqN(i, dim));
                }
                return sim_param;
        }

        Eigen::VectorXd VariableToSimulation::simulation_param_to_optimization_param(const Eigen::VectorXd &sim_param)
        {
                assert(get_parameter_type() == ParameterType::Shape);
                auto state = get_state();
                const int dim = state->mesh->dimension();
                const int npts = state->mesh->n_vertices();
                assert(npts * dim == sim_param.size());
                Eigen::VectorXd reduced_opt_param, reordered_sim_param;

                // If parametarization is slicemap, this proplerly returns output_indexing_.
                Eigen::VectorXi indices_map = get_output_indexing(reduced_opt_param);
                if (indices_map.size() == 0) {
                    // If opt and sim params are bijective. In my research definitely this won't be the case. so please ignore this.
                    indices_map.setLinSpaced(npts * dim, 0, npts * dim - 1); 
                }

                reordered_sim_param.setZero(sim_param.size());
                auto n2p = state->node_to_primitive();
                for (int i = 0; i < sim_param.size() / dim; ++i) {
                    int vid = n2p[i];
                    //assert(state->mesh->is_boundary_vertex(vid));
                    // reordered_sim_param is ordered with vertex indices (and dimension), while sim_param with node indices (and dimension).
                    for (int d = 0; d < dim; ++d) {
                        reordered_sim_param(vid * dim + d) = sim_param(i * dim + d);
                    }
                }

                reduced_opt_param.setZero(indices_map.size());
                for (int i = 0; i < indices_map.size(); i++) {
                    // opt_param's order is the same as indices_map's one, and along the order indices_map contains corresponding vertex indices (and dimension).
                    reduced_opt_param(i) = reordered_sim_param(indices_map(i));
                }
                auto opt_param =  parametrization_.inverse_eval(reduced_opt_param);
                return opt_param;
        }

	void VariableToSimulation::update_state(const Eigen::VectorXd &state_variable, const Eigen::VectorXi &indices)
	{
		log_and_throw_error("Not implemented!");
	}

        void ShapeVariableToSimulation::update_direct(const Eigen::VectorXd &shape_diff) {
                for (auto state: states_)
                {
                        assert(shape_diff.size() == state->ndof());
                        const int dim = state->mesh->dimension();
                        for (int i = 0; i < shape_diff.size() / dim; i += dim) {
                                int vid = i;
                                RowVectorNd v = shape_diff(Eigen::seqN(i, dim));
                                state->set_mesh_vertex(vid, v);
                                assert(state->mesh->point(vid) == v);
                        }
                }
        }
	void ShapeVariableToSimulation::update_state(const Eigen::VectorXd &state_variable, const Eigen::VectorXi &indices)
	{
		for (auto state : states_)
		{
			const int dim = state->mesh->dimension();

			// If indices include one vertex entry, we assume it include all entries of this vertex.
			for (int i = 0; i < indices.size(); i += dim)
				for (int j = 0; j < dim; j++)
					assert(indices(i + j) == indices(i) + j);

			for (int i = 0; i < indices.size(); i += dim){
                                int vid = indices(i) / dim;
                                RowVectorNd v = state_variable(Eigen::seqN(i, dim));
				state->set_mesh_vertex(indices(i) / dim, state_variable(Eigen::seqN(i, dim)));
                                assert(state->mesh->point(vid) == v);
                        }
		}
	}

        // just for debugging
        std::tuple<Eigen::VectorXd, Eigen::VectorXd, Eigen::VectorXd> ShapeVariableToSimulation::compute_adjoint_terms(const Eigen::VectorXd &x, const bool filtering) const {
                Eigen::VectorXd e, r, c;
                auto state = states_[0];
		AdjointTools::dJ_shape_static_adjoint_terms(*state, state->diff_cached.u(0), state->get_adjoint_mat(0), e, r, c, variance_, abs_max_);
                return std::tuple(e, r, c);
	}

	Eigen::VectorXd ShapeVariableToSimulation::compute_adjoint_term(const Eigen::VectorXd &x) const
	{
		Eigen::VectorXd term, cur_term;
		for (auto state : states_)
		{
			if (state->problem->is_time_dependent())
			{
				Eigen::MatrixXd adjoint_nu, adjoint_p;
				adjoint_nu = state->get_adjoint_mat(1);
				adjoint_p = state->get_adjoint_mat(0);
				AdjointTools::dJ_shape_transient_adjoint_term(*state, adjoint_nu, adjoint_p, cur_term);
			} else {
				AdjointTools::dJ_shape_static_adjoint_term(*state, state->diff_cached.u(0), state->get_adjoint_mat(0), cur_term, variance_, abs_max_);
                        }
			if (term.size() != cur_term.size()) {
				term = cur_term;
                        } else {
				term += cur_term;
                        }
		}
                //utils::filter_outlier(term, variance_, abs_max_);

		return apply_parametrization_jacobian(term, x);
	}
	Eigen::VectorXd ShapeVariableToSimulation::inverse_eval()
	{
		const int dim = states_[0]->mesh->dimension();
		const int npts = states_[0]->mesh->n_vertices();

		Eigen::VectorXd x;
		Eigen::VectorXi indices = get_output_indexing(x);

		if (indices.size() == 0)
			indices.setLinSpaced(npts * dim, 0, npts * dim - 1);

		Eigen::MatrixXd V, V_flat;
		states_[0]->get_vertices(V);
		V_flat = utils::flatten(V);

		x.setZero(indices.size());
		for (int i = 0; i < indices.size(); i++)
			x(i) = V_flat(indices(i));

		return parametrization_.inverse_eval(x);
	}

	void ElasticVariableToSimulation::update_state(const Eigen::VectorXd &state_variable, const Eigen::VectorXi &indices)
	{
		for (auto state : states_)
		{
			const int n_elem = state->bases.size();
			assert(n_elem * 2 == state_variable.size());
			state->assembler->update_lame_params(state_variable.segment(0, n_elem), state_variable.segment(n_elem, n_elem));
		}
	}
	Eigen::VectorXd ElasticVariableToSimulation::compute_adjoint_term(const Eigen::VectorXd &x) const
	{
		Eigen::VectorXd term, cur_term;
		for (auto state : states_)
		{
			if (state->problem->is_time_dependent())
			{
				Eigen::MatrixXd adjoint_nu, adjoint_p;
				adjoint_nu = state->get_adjoint_mat(1);
				adjoint_p = state->get_adjoint_mat(0);
				AdjointTools::dJ_material_transient_adjoint_term(*state, adjoint_nu, adjoint_p, cur_term);
			}
			else
			{
				AdjointTools::dJ_material_static_adjoint_term(*state, state->diff_cached.u(0), state->get_adjoint_mat(0), cur_term);
			}
			if (term.size() != cur_term.size())
				term = cur_term;
			else
				term += cur_term;
		}
		return apply_parametrization_jacobian(term, x);
	}
	Eigen::VectorXd ElasticVariableToSimulation::inverse_eval()
	{
		auto &state = *(states_[0]);
		auto params_map = state.assembler->parameters();

		auto search_lambda = params_map.find("lambda");
		auto search_mu = params_map.find("mu");
		if (search_lambda == params_map.end() || search_mu == params_map.end())
		{
			log_and_throw_error("Failed to find Lame parameters!");
			return Eigen::VectorXd();
		}

		Eigen::VectorXd lambdas(state.mesh->n_elements());
		Eigen::VectorXd mus(state.mesh->n_elements());
		for (int e = 0; e < state.mesh->n_elements(); e++)
		{
			RowVectorNd barycenter;
			if (!state.mesh->is_volume())
			{
				const auto &mesh2d = *dynamic_cast<mesh::Mesh2D *>(state.mesh.get());
				barycenter = mesh2d.face_barycenter(e);
			}
			else
			{
				const auto &mesh3d = *dynamic_cast<mesh::Mesh3D *>(state.mesh.get());
				barycenter = mesh3d.cell_barycenter(e);
			}
			lambdas(e) = search_lambda->second(RowVectorNd::Zero(state.mesh->dimension()), barycenter, 0., e);
			mus(e) = search_mu->second(RowVectorNd::Zero(state.mesh->dimension()), barycenter, 0., e);
		}
		state.assembler->update_lame_params(lambdas, mus);

		Eigen::VectorXd params(lambdas.size() + mus.size());
		params << lambdas, mus;

		return parametrization_.inverse_eval(params);
	}

	void FrictionCoeffientVariableToSimulation::update_state(const Eigen::VectorXd &state_variable, const Eigen::VectorXi &indices)
	{
		assert(state_variable.size() == 1);
		assert(state_variable(0) >= 0);
		for (auto state : states_)
			state->args["contact"]["friction_coefficient"] = state_variable(0);
	}
	Eigen::VectorXd FrictionCoeffientVariableToSimulation::compute_adjoint_term(const Eigen::VectorXd &x) const
	{
		Eigen::VectorXd term, cur_term;
		for (auto state : states_)
		{
			if (state->problem->is_time_dependent())
			{
				Eigen::MatrixXd adjoint_nu, adjoint_p;
				adjoint_nu = state->get_adjoint_mat(1);
				adjoint_p = state->get_adjoint_mat(0);
				AdjointTools::dJ_friction_transient_adjoint_term(*state, adjoint_nu, adjoint_p, cur_term);
			}
			else
			{
				log_and_throw_error("Friction coefficient grad in static simulations not implemented!");
			}
			if (term.size() != cur_term.size())
				term = cur_term;
			else
				term += cur_term;
		}
		return apply_parametrization_jacobian(term, x);
	}
	Eigen::VectorXd FrictionCoeffientVariableToSimulation::inverse_eval()
	{
		log_and_throw_error("Not implemented!");
		return Eigen::VectorXd();
	}

	void DampingCoeffientVariableToSimulation::update_state(const Eigen::VectorXd &state_variable, const Eigen::VectorXi &indices)
	{
		assert(state_variable.size() == 2);
		json damping_param = {
			{"psi", state_variable(0)},
			{"phi", state_variable(1)},
		};
		for (auto state : states_)
		{
			if (!state->args["materials"].is_array())
			{
				state->args["materials"]["psi"] = damping_param["psi"];
				state->args["materials"]["phi"] = damping_param["phi"];
			}
			else
			{
				for (auto &arg : state->args["materials"])
				{
					arg["psi"] = damping_param["psi"];
					arg["phi"] = damping_param["phi"];
				}
			}

			if (state->damping_assembler)
				state->damping_assembler->add_multimaterial(0, damping_param, state->units);
		}
		logger().info("Current damping params: {}, {}", state_variable(0), state_variable(1));
	}
	Eigen::VectorXd DampingCoeffientVariableToSimulation::compute_adjoint_term(const Eigen::VectorXd &x) const
	{
		Eigen::VectorXd term, cur_term;
		for (auto state : states_)
		{
			if (state->problem->is_time_dependent())
			{
				Eigen::MatrixXd adjoint_nu, adjoint_p;
				adjoint_nu = state->get_adjoint_mat(1);
				adjoint_p = state->get_adjoint_mat(0);
				AdjointTools::dJ_damping_transient_adjoint_term(*state, adjoint_nu, adjoint_p, cur_term);
			}
			else
			{
				log_and_throw_error("Static damping not supported!");
			}
			if (term.size() != cur_term.size())
				term = cur_term;
			else
				term += cur_term;
		}
		return apply_parametrization_jacobian(term, x);
	}
	Eigen::VectorXd DampingCoeffientVariableToSimulation::inverse_eval()
	{
		log_and_throw_error("Not implemented!");
		return Eigen::VectorXd();
	}

	void InitialConditionVariableToSimulation::update_state(const Eigen::VectorXd &state_variable, const Eigen::VectorXi &indices)
	{
		for (auto state : states_)
		{
			if (state_variable.size() != state->ndof() * 2)
			{
				log_and_throw_error("Inconsistent number of parameters {} and number of dofs in forward {}!", state_variable.size(), state->ndof() * 2);
			}
			state->initial_sol_update = state_variable.head(state->ndof());
			state->initial_vel_update = state_variable.tail(state->ndof());
		}
	}
	Eigen::VectorXd InitialConditionVariableToSimulation::compute_adjoint_term(const Eigen::VectorXd &x) const
	{
		Eigen::VectorXd term, cur_term;
		for (auto state : states_)
		{
			if (state->problem->is_time_dependent())
			{
				Eigen::MatrixXd adjoint_nu, adjoint_p;
				adjoint_nu = state->get_adjoint_mat(1);
				adjoint_p = state->get_adjoint_mat(0);
				AdjointTools::dJ_initial_condition_adjoint_term(*state, adjoint_nu, adjoint_p, cur_term);
			}
			else
			{
				log_and_throw_error("Static initial condition not supported!");
			}
			if (term.size() != cur_term.size())
				term = cur_term;
			else
				term += cur_term;
		}
		return apply_parametrization_jacobian(term, x);
	}
	Eigen::VectorXd InitialConditionVariableToSimulation::inverse_eval()
	{
		log_and_throw_error("Not implemented!");
		return Eigen::VectorXd();
	}

	void DirichletVariableToSimulation::update_state(const Eigen::VectorXd &state_variable, const Eigen::VectorXi &indices)
	{
		log_and_throw_error("Dirichlet variable to simulation not implemented!");
		// auto &problem = *dynamic_cast<assembler::GenericTensorProblem *>(state_ptr_->problem.get());
		// // This should eventually update dirichlet boundaries per boundary element, using the shape constraint.
		// auto constraint_string = control_constraints_->constraint_to_string(state_variable);
		// for (const auto &kv : boundary_id_to_reduced_param)
		// {
		// 	json dirichlet_bc = constraint_string[kv.first];
		// 	// Need time_steps + 1 entry, though unused.
		// 	for (int k = 0; k < states_ptr_[0]->mesh->dimension(); ++k)
		// 		dirichlet_bc[k].push_back(dirichlet_bc[k][time_steps - 1]);
		// 	logger().trace("Updating boundary id {} to dirichlet bc {}", kv.first, dirichlet_bc);
		// 	problem.update_dirichlet_boundary(kv.first, dirichlet_bc, true, true, true, "");
		// }
	}
	Eigen::VectorXd DirichletVariableToSimulation::compute_adjoint_term(const Eigen::VectorXd &x) const
	{
		Eigen::VectorXd term, cur_term;
		for (auto state : states_)
		{
			if (state->problem->is_time_dependent())
			{
				Eigen::MatrixXd adjoint_nu, adjoint_p;
				adjoint_nu = state->get_adjoint_mat(1);
				adjoint_p = state->get_adjoint_mat(0);
				AdjointTools::dJ_dirichlet_transient_adjoint_term(*state, adjoint_nu, adjoint_p, cur_term);
			}
			else
			{
				log_and_throw_error("Static dirichlet boundary optimization not supported!");
			}
			if (term.size() != cur_term.size())
				term = cur_term;
			else
				term += cur_term;
		}
		return apply_parametrization_jacobian(term, x);
	}
	std::string DirichletVariableToSimulation::variable_to_string(const Eigen::VectorXd &variable)
	{
		return "";
	}
	Eigen::VectorXd DirichletVariableToSimulation::inverse_eval()
	{
		log_and_throw_error("Not implemented!");
		return Eigen::VectorXd();
	}
} // namespace polyfem::solver