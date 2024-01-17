#include "polyfem/assembler/GenericProblem.hpp"
#include "polyfem/problem/ProblemFactory.hpp"
#include "polyfem/utils/MatrixUtils.hpp"
#include <polyfem/State.hpp>
#include <polyfem/Common.hpp>
#include <polyfem/utils/JSONUtils.hpp>
#include <polyfem/utils/BoundarySampler.hpp>
#include <polyfem/assembler/Mass.hpp>
#include <polyfem/assembler/ViscousDamping.hpp>
#include <polyfem/assembler/RhsAssembler.hpp>

#include <polyfem/solver/forms/adjoint_forms/VariableToSimulation.hpp>
#include <polyfem/solver/forms/Form.hpp>
#include <polyfem/solver/forms/BodyForm.hpp>
#include <polyfem/solver/forms/ContactForm.hpp>
#include <polyfem/solver/forms/ElasticForm.hpp>
#include <polyfem/solver/forms/FrictionForm.hpp>
#include <polyfem/solver/forms/InertiaForm.hpp>
#include <polyfem/solver/forms/LaggedRegForm.hpp>
#include <polyfem/solver/forms/RayleighDampingForm.hpp>

#include <polyfem/solver/NLProblem.hpp>
#include <polyfem/solver/ALSolver.hpp>
#include <polyfem/utils/Overlapping.hpp>

#include <polyfem/solver/H1Gradient.hpp>
#include <polyfem/utils/Logger.hpp>

#include <ipc/ipc.hpp>

namespace cppoptlib
{
        /*
        void neumann_to_dirichlet(const &json bc) {
            assert(is_param_valid(bc, "dirichlet_boundary"))
        }
        */
    // compute update direction by traction method (or, 'Chikara Hou')
    H1Gradient::H1Gradient(
            std::vector<std::shared_ptr<polyfem::solver::VariableToSimulation>> var2sims,
            const json &params)
    : Director(var2sims),
      state(var2sims[0]->get_state()),
      problem(*std::dynamic_pointer_cast<polyfem::assembler::GenericTensorProblem>(state->problem)),
      weight(params["weight"]),
      contact_enabled(params["solver"].value("contact_enabled", var2sims[0]->get_state()->args["contact"]["enabled"].get<bool>())),
      friction_coefficient(params["solver"].value("friction_coefficient", var2sims[0]->get_state()->args["contact"]["friction_coefficient"].get<double>())),
      dhat(params["solver"].value("dhat", var2sims[0]->get_state()->args["contact"]["dhat"].get<double>())),
      barrier_stiffness(params["solver"].value("barrier_stiffness", var2sims[0]->get_state()->args["solver"]["contact"]["barrier_stiffness"].get<double>())),
      instance_name(params.value("name",""))
    {
        assert(problem.name() == "GenericTensor");
        overwrite_boundary_conditions();
    }

    void H1Gradient::overwrite_boundary_conditions()
    {
        //problem.add_dirichlet_boundary();
        auto bc = state->args["boundary_conditions"];
        std::vector<int> ids;
        for (auto& nb: bc["neumann_boundary"]) {
            ids.push_back(nb["id"].get<int>());
        }
        for (const auto id: ids) {
            problem.update_neumann_boundary(id, Eigen::RowVector3d({0,0,0}));
            problem.add_dirichlet_boundary(id, Eigen::RowVector3d({0,0,0}), false, true, true);
        }
    }

    void H1Gradient::setup_bc() {
        problem.set_units(*state->assembler, state->units);
        for (auto&lb: state->total_local_boundary) {
          local_boundary.push_back(lb);
        }
        problem.setup_bc(*state->mesh, state->n_bases - state->obstacle.n_vertices(),
                          state->bases, state->geom_bases(), state->pressure_bases,
                          local_boundary, boundary_nodes, local_neumann_boundary, pressure_boundary_nodes,
                          dirichlet_nodes, neumann_nodes);
        // setup nodal values
        {
            dirichlet_nodes_position.resize(dirichlet_nodes.size());
            for (int n = 0; n < dirichlet_nodes.size(); ++n)
            {
                const int n_id = dirichlet_nodes[n];
                bool found = false;
                for (const auto &bs : state->bases)
                {
                    for (const auto &b : bs.bases)
                    {
                        for (const auto &lg : b.global())
                        {
                            if (lg.index == n_id)
                            {
                                dirichlet_nodes_position[n] = lg.node;
                                found = true;
                                break;
                            }
                        }

                        if (found)
                            break;
                    }

                    if (found)
                        break;
                }

                assert(found);
            }

            neumann_nodes_position.resize(neumann_nodes.size());
            for (int n = 0; n < neumann_nodes.size(); ++n)
            {
                const int n_id = neumann_nodes[n];
                bool found = false;
                for (const auto &bs : state->bases)
                {
                    for (const auto &b : bs.bases)
                    {
                        for (const auto &lg : b.global())
                        {
                            if (lg.index == n_id)
                            {
                                neumann_nodes_position[n] = lg.node;
                                found = true;
                                break;
                            }
                        }

                        if (found)
                            break;
                    }

                    if (found)
                        break;
                }

                assert(found);
            }
        }
    }
    Eigen::VectorXd H1Gradient::compute_update_direction (
        const Eigen::VectorXd &x,
        const Eigen::VectorXd &grad,
        Eigen::VectorXd &direction)
    {
        Eigen::MatrixXd rhs, sol; // typed as matrix, but are actually flattened vectors
        polyfem::solver::SolveData solve_data;
        rhs.setZero(state->ndof(), 1);
        direction.setZero(x.size());
        solve_data.rhs_assembler = build_rhs_assembler();
        for (auto &v2s: var2sims) {
            Eigen::VectorXd weighted_grad;
            weighted_grad = weight * v2s->optimization_param_to_simulation_param(grad);
            assert(weighted_grad.size() == state->ndof());
            //rhs += grad_to_normal_load(weighted_grad);
            rhs += weighted_grad;
        }
        rhs *= -1;
        assert(rhs.size() == state->ndof());
        problem.set_nodal_neumann_mat(polyfem::utils::unflatten(rhs, state->mesh->dimension()));
        setup_bc();

        init_solve(sol, solve_data, rhs);
        solve(sol, solve_data, rhs);

        {
          /*
          // to test v2s conversion functions uncomment below
          Eigen::VectorXd target;
          target.setZero(grad.size());
          target = grad;
          auto tmp = var2sims[0]->optimization_param_to_simulation_param(target);
          auto opt_after_sim = var2sims[0]->simulation_param_to_optimization_param(tmp);
          auto p = var2sims[0]->get_parametrization();
          auto extended_after_reduced = p.inverse_eval(p.eval(target));
          bool success = true;
          assert(opt_after_sim.size() == extended_after_reduced.size());
          for (int i = 0; i < opt_after_sim.size(); ++i) {
            const double oas = opt_after_sim(i);
            const double ear = extended_after_reduced(i);
            const double error = std::abs(oas - ear);
            if (error < 1e-8) {
              std::cout << "test v2s ... i: "<<i<<", ok ..."  << oas <<std::endl;
            } else {
              success = false;
              std::cout << "test v2s ... i: "<<i<<", failed ... " << "oas=" << oas << ", ear=" << ear << ", |error|="<< error << std::endl;
            }
          }
          if (!success) {
            assert(false);
          }
          */
        }
        for (auto &v2s: var2sims) {
            Eigen::VectorXd tmp;
            tmp = v2s->simulation_param_to_optimization_param(sol);
            assert(direction.size() == tmp.size());
            direction += tmp;
        }
        std::cout << "dhat: " << dhat << ", barrier_stiffness: " << barrier_stiffness << std::endl;
        std::cout << instance_name << ": |grad| = " << grad.norm() << ", weight = " << weight << ", |rhs| = "<< rhs.norm() << ", |direction| = " << direction.norm() << std::endl;
        assert(sol.size() == state->ndof());
        return sol;
    }

    std::shared_ptr<polyfem::assembler::RhsAssembler> H1Gradient::build_rhs_assembler() const
    {
        json rhs_solver_params = state->args["solver"]["linear"];
        if (!rhs_solver_params.contains("Pardiso"))
            rhs_solver_params["Pardiso"] = {};
        rhs_solver_params["Pardiso"]["mtype"] = -2; // matrix type for Pardiso (2 = SPD)

        const int size = state->problem->is_scalar() ? 1 : state->mesh->dimension();

        return std::make_shared<polyfem::assembler::RhsAssembler>(
            *state->assembler, *state->mesh, state->obstacle,
            dirichlet_nodes, neumann_nodes,
            dirichlet_nodes_position, neumann_nodes_position,
            state->n_bases, size, state->bases, state->geom_bases(), state->mass_ass_vals_cache, problem,
            state->args["space"]["advanced"]["bc_method"],
            state->args["solver"]["linear"]["solver"],
            state->args["solver"]["linear"]["precond"],
            rhs_solver_params);
    }

    void H1Gradient::init_solve(
            Eigen::MatrixXd &sol,
            polyfem::solver::SolveData &solve_data,
            const Eigen::MatrixXd &rhs)
    {
        sol.setZero(state->ndof(), 1);
        //assert(!state->assembler->is_linear() || state->is_contact_enabled()); // non-linear
        //assert(!state->problem->is_scalar());                           // tensor
        assert(state->mixed_assembler == nullptr);

        const int t = 0;        // static
        // --------------------------------------------------------------------
        // Check for initial intersections
        if (state->is_contact_enabled())
        {
            POLYFEM_SCOPED_TIMER("Check for initial intersections");

            const Eigen::MatrixXd displaced = state->collision_mesh.displace_vertices(
                polyfem::utils::unflatten(sol, state->mesh->dimension()));

            if (ipc::has_intersections(state->collision_mesh, displaced))
            {
                polyfem::relax_overlapping(*state, sol);
                const Eigen::MatrixXd relaxed = state->collision_mesh.displace_vertices(
                polyfem::utils::unflatten(sol, state->mesh->dimension()));
                if (ipc::has_intersections(state->collision_mesh, relaxed)) {
                    polyfem::log_and_throw_error("Unable to solve, initial solution has intersections!");
                }
            }
        }
        // --------------------------------------------------------------------
        // Initialize forms

        auto damping_assembler = std::make_shared<polyfem::assembler::ViscousDamping>();
        state->set_materials(*damping_assembler);

        // for backward solve
        auto damping_prev_assembler = std::make_shared<polyfem::assembler::ViscousDampingPrev>();
        state->set_materials(*damping_prev_assembler);

        const std::vector<std::shared_ptr<polyfem::solver::Form>> forms = solve_data.init_forms(
            // General
            state->units,
            state->mesh->dimension(), t,
            // Elastic form
            state->n_bases, state->bases, state->geom_bases(), *state->assembler, state->ass_vals_cache, state->mass_ass_vals_cache,
            // Body form
            state->n_pressure_bases, boundary_nodes, local_boundary, local_neumann_boundary,
            state->n_boundary_samples(), rhs, sol, state->mass_matrix_assembler->density(),
            // Inertia form
            state->args.value("/time/quasistatic"_json_pointer, true), state->mass,
            damping_assembler->is_valid() ? damping_assembler : nullptr,
            // Lagged regularization form
            state->args["solver"]["advanced"]["lagged_regularization_weight"],
            state->args["solver"]["advanced"]["lagged_regularization_iterations"],
            // Augmented lagrangian form
            state->obstacle.ndof(),
            // Contact form
            contact_enabled, state->collision_mesh, 
            dhat,
            state->avg_mass, state->args["contact"]["use_convergent_formulation"],
            barrier_stiffness,
            state->args["solver"]["contact"]["CCD"]["broad_phase"],
            state->args["solver"]["contact"]["CCD"]["tolerance"],
            state->args["solver"]["contact"]["CCD"]["max_iterations"],
            state->optimization_enabled,
            // Friction form
            friction_coefficient,
            state->args["contact"]["epsv"],
            state->args["solver"]["contact"]["friction_iterations"],
            // Rayleigh damping form
            state->args["solver"]["rayleigh_damping"]);

        if (solve_data.contact_form != nullptr)
            solve_data.contact_form->save_ccd_debug_meshes = state->args["output"]["advanced"]["save_ccd_debug_meshes"];

        // --------------------------------------------------------------------
        // Initialize nonlinear problems

        const int ndof = state->n_bases * state->mesh->dimension();
        solve_data.nl_problem = std::make_shared<polyfem::solver::NLProblem>(
            ndof, boundary_nodes, local_boundary, state->n_boundary_samples(),
            *solve_data.rhs_assembler, t, forms);
    }

    void H1Gradient::solve(Eigen::MatrixXd &sol, polyfem::solver::SolveData &solve_data, const Eigen::MatrixXd &rhs)
    {
        assert(solve_data.nl_problem != nullptr);
                polyfem::solver::NLProblem &nl_problem = *(solve_data.nl_problem);

        assert(sol.size() == rhs.size());

        const int t = 0;

        std::shared_ptr<cppoptlib::NonlinearSolver<polyfem::solver::NLProblem>> nl_solver = state->make_nl_solver<polyfem::solver::NLProblem>();

        polyfem::solver::ALSolver al_solver(
            nl_solver, solve_data.al_lagr_form, solve_data.al_pen_form,
            state->args["solver"]["augmented_lagrangian"]["initial_weight"],
            state->args["solver"]["augmented_lagrangian"]["scaling"],
            state->args["solver"]["augmented_lagrangian"]["max_weight"],
            state->args["solver"]["augmented_lagrangian"]["eta"],
            state->args["solver"]["augmented_lagrangian"]["max_solver_iters"],
            [&](const Eigen::VectorXd &x) {
                solve_data.update_barrier_stiffness(sol);
            });

        al_solver.post_subsolve = [&](const double al_weight) {
                  /*
            state->stats.solver_info.push_back(
                {{"type", al_weight > 0 ? "al" : "rc"},
                 {"t", t}, // TODO: null if static?
                 {"info", nl_solver->get_info()}});
            if (al_weight > 0)
                state->stats.solver_info.back()["weight"] = al_weight;
            state->save_subsolve(++subsolve_count, t, sol, Eigen::MatrixXd()); // no pressure
                  */ 
        };

        al_solver.solve(nl_problem, sol, state->args["solver"]["augmented_lagrangian"]["force"]);

        state->out_geom.save_vtu(
            state->resolve_output_path(fmt::format("solve_h1_{}_{:d}.vtu", instance_name, subsolve_count++)),
            //*state, sol, Eigen::MatrixXd(), t, 1,
            *state, rhs, Eigen::MatrixXd(), t, 1,
            polyfem::io::OutGeometryData::ExportOptions(state->args, state->mesh->is_linear(), problem.is_scalar(), true),
            state->is_contact_enabled(), state->solution_frames);
    }

    Eigen::VectorXd H1Gradient::grad_to_normal_load(const Eigen::VectorXd &grad) const {
        const int dim = state->mesh->dimension();
        Eigen::MatrixXd grad_mat = polyfem::utils::unflatten(grad, dim);
        Eigen::MatrixXd load_mat = Eigen::MatrixXd::Zero(grad_mat.rows(), grad_mat.cols());
        for (auto &lb: state->total_local_boundary) {
            assert(lb.type() == polyfem::mesh::BoundaryType::TRI_LINE);
            const int e = lb.element_id();

            Eigen::MatrixXd uv, samples, gtmp, rhs_fun, deform_mat, trafo;
            Eigen::MatrixXd points, normals;
            Eigen::VectorXd weights, normal;

            for (int i = 0; i < lb.size(); ++i) {
                int gid = lb.global_primitive_id(i);
                /*
                bool normal_flipped = false;
                // only use normals
                polyfem::utils::BoundarySampler::boundary_quadrature(lb,
                    state->n_boundary_samples(), *state->mesh, i, false, uv, points, normals, weights);
                polyfem::assembler::ElementAssemblyValues vals;
                vals.compute(e, state->mesh->is_volume(), points, state->bases[e], state->geom_bases()[e]);
                normal = normals.row(1);
                normal = vals.jac_it[0] * normal; // assuming linear geometry
                normal.normalize();
                */
                polyfem::get_normal_outward(*state, e, gid, normal);
                for (int lv_id = 0; lv_id < dim; ++lv_id) {
                    auto v_id = state->mesh->boundary_element_vertex(gid, lv_id);
                    if (v_id < 0) {
                        continue;
                    }
                    assert(load_mat.cols() == normal.rows());
                    auto n_id = state->mesh_nodes->primitive_to_node()[v_id];
                    load_mat.row(n_id) += grad_mat.row(n_id).dot(normal) * normal.transpose();
                }
            }
        }
        Eigen::VectorXd load = polyfem::utils::flatten(load_mat);
        return load;
    }
    ////////////////////////////////////////////////////////////////////////
    // Template instantiations
    //template std::shared_ptr<cppoptlib::NonlinearSolver<NLProblem>> State::make_nl_solver(const std::string &) const;
}
