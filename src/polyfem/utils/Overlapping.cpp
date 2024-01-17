#include <ipc/ipc.hpp>
#include <ipc/utils/world_bbox_diagonal_length.hpp>
#include <igl/predicates/segment_segment_intersect.h>
#include <polyfem/State.hpp>
#include <polyfem/utils/BoundarySampler.hpp>
#include <polyfem/utils/Overlapping.hpp>

namespace polyfem {
    using namespace mesh;

    bool is_edge_intersecting_triangle(
        const Eigen::Vector3d& e0,
        const Eigen::Vector3d& e1,
        const Eigen::Vector3d& t0,
        const Eigen::Vector3d& t1,
        const Eigen::Vector3d& t2)
    {
        igl::predicates::exactinit();
        const auto ori1 = igl::predicates::orient3d(t0, t1, t2, e0);
        const auto ori2 = igl::predicates::orient3d(t0, t1, t2, e1);

        if (ori1 != igl::predicates::Orientation::COPLANAR
            && ori2 != igl::predicates::Orientation::COPLANAR && ori1 == ori2) {
            // edge is completly on one side of the plane that triangle is in
            return false;
        }

        Eigen::Matrix3d M;
        M.col(0) = t1 - t0;
        M.col(1) = t2 - t0;
        M.col(2) = e0 - e1;
        Eigen::Vector3d uvt = M.fullPivLu().solve(e0 - t0);
        return uvt[0] >= 0.0 && uvt[1] >= 0.0 && uvt[0] + uvt[1] <= 1.0
            && uvt[2] >= 0.0 && uvt[2] <= 1.0;
    }

    std::set<int> overlapping_nodes(
        const ipc::CollisionMesh& mesh,
        const Eigen::MatrixXd& vertices,
        const ipc::BroadPhaseMethod broad_phase_method = ipc::DEFAULT_BROAD_PHASE_METHOD)
    {
        assert(vertices.rows() == mesh.num_vertices());
        std::set<int> overlapping_node_ids;

        const double conservative_inflation_radius =
            1e-6 * ipc::world_bbox_diagonal_length(vertices);

        std::shared_ptr<ipc::BroadPhase> broad_phase =
            ipc::BroadPhase::make_broad_phase(broad_phase_method);
        broad_phase->can_vertices_collide = mesh.can_collide;

        broad_phase->build(
            vertices, mesh.edges(), mesh.faces(), conservative_inflation_radius);

        if (vertices.cols() == 2) {
            // Need to check segment-segment intersections in 2D
            std::vector<ipc::EdgeEdgeCandidate> ee_candidates;

            broad_phase->detect_edge_edge_candidates(ee_candidates);
            broad_phase->clear();

            // narrow-phase using igl
            igl::predicates::exactinit();
            for (const auto& [ea_id, eb_id] : ee_candidates) {
                if (igl::predicates::segment_segment_intersect(
                        vertices.row(mesh.edges()(ea_id, 0)).head<2>(),
                        vertices.row(mesh.edges()(ea_id, 1)).head<2>(),
                        vertices.row(mesh.edges()(eb_id, 0)).head<2>(),
                        vertices.row(mesh.edges()(eb_id, 1)).head<2>())) {
                    overlapping_node_ids.insert(mesh.to_full_vertex_id(mesh.edges()(ea_id, 0)));
                    overlapping_node_ids.insert(mesh.to_full_vertex_id(mesh.edges()(ea_id, 1)));
                    overlapping_node_ids.insert(mesh.to_full_vertex_id(mesh.edges()(eb_id, 0)));
                    overlapping_node_ids.insert(mesh.to_full_vertex_id(mesh.edges()(eb_id, 1)));
                }
            }
        } else {
            // Need to check segment-triangle intersections in 3D
            assert(vertices.cols() == 3);

            std::vector<ipc::EdgeFaceCandidate> ef_candidates;
            broad_phase->detect_edge_face_candidates(ef_candidates);
            broad_phase->clear();

            for (const auto& [e_id, f_id] : ef_candidates) {
                if (is_edge_intersecting_triangle(
                        vertices.row(mesh.edges()(e_id, 0)),
                        vertices.row(mesh.edges()(e_id, 1)),
                        vertices.row(mesh.faces()(f_id, 0)),
                        vertices.row(mesh.faces()(f_id, 1)),
                        vertices.row(mesh.faces()(f_id, 2)))) {
                    overlapping_node_ids.insert(mesh.to_full_vertex_id(mesh.edges()(e_id, 0)));
                    overlapping_node_ids.insert(mesh.to_full_vertex_id(mesh.edges()(e_id, 1)));
                    overlapping_node_ids.insert(mesh.to_full_vertex_id(mesh.faces()(f_id, 0)));
                    overlapping_node_ids.insert(mesh.to_full_vertex_id(mesh.faces()(f_id, 1)));
                    overlapping_node_ids.insert(mesh.to_full_vertex_id(mesh.faces()(f_id, 2)));
                }
            }
        }

        return overlapping_node_ids;
    }

    void get_normal_outward(const State& state, const int element_id, const int boundary_primitive_id, Eigen::VectorXd& normal) {
        assert(state.mesh->dimension() == 2);
        assert(state.mesh->is_simplex(element_id));
        int e = element_id;
        int b = boundary_primitive_id;
        int dim = state.mesh->dimension();
        Eigen::RowVectorXd element_barycenter, boundary_barycenter, inward, boundary_vector;
        normal.setZero(dim);
        element_barycenter.setZero(dim);
        boundary_barycenter.setZero(dim);
        inward.setZero(dim);
        boundary_vector.setZero(dim);


        int v0 = state.mesh->boundary_element_vertex(b, 0);
        int v1 = state.mesh->boundary_element_vertex(b, 1);
        boundary_vector = state.mesh->point(v0) - state.mesh->point(v1);
        normal(0) = boundary_vector(1);
        normal(1) = -boundary_vector(0);

        for (int i = 0; i < dim+1; ++i) {
            int vertex_id = state.mesh->element_vertex(e, i);
            element_barycenter += state.mesh->point(vertex_id);
        }
        for (int i = 0; i < dim; ++i) {
            int vertex_id = state.mesh->boundary_element_vertex(b, i);
            boundary_barycenter += state.mesh->point(vertex_id);
        }
        element_barycenter /= dim + 1;
        boundary_barycenter /= dim;

        inward = element_barycenter - boundary_barycenter;
        if (inward * normal > 0) {
            // normal is inward
            normal *= -1;
        }
        normal.normalize();
    }
    void relax_overlapping(const State& state, Eigen::MatrixXd& sol, int max_iter) {
        double eps = 0.0001;
        for (int iter = 0; iter < max_iter; ++iter) {
            auto solution = utils::unflatten(sol, state.mesh->dimension());
            Eigen::MatrixXd displaced = state.collision_mesh.displace_vertices(solution);
            auto node_ids = overlapping_nodes(state.collision_mesh, displaced);
            std::set<int> ovl_v_ids;     // global vertex ids of overlapped nodes
            for (auto &n_id: node_ids) {
                assert(state.mesh_nodes->is_vertex_node(n_id));
                assert(state.mesh_nodes->is_boundary(n_id));
                ovl_v_ids.insert(state.mesh_nodes->node_to_primitive()[n_id]);
            }
            for (auto &lb: state.total_local_boundary) {
                assert(lb.type() == BoundaryType::TRI_LINE);
                const int e = lb.element_id();

                std::vector<bool> contains_ovl_v;
                for (int i = 0; i < lb.size(); ++i) {
                      int gid = lb.global_primitive_id(i);
                      contains_ovl_v.push_back(false);
                      // 2D boundary primitive is edge (with 2 vertices),
                      // 3d boundary primitive is face (with 3 vertices)
                      for (int lv_id = 0; lv_id < state.mesh->dimension(); ++lv_id) {
                          int v_id = state.mesh->boundary_element_vertex(gid, lv_id);
                          if (ovl_v_ids.find(v_id) != ovl_v_ids.end()) {
                            contains_ovl_v[i] = true;
                          }
                      }
                }
                if (std::all_of(contains_ovl_v.begin(), contains_ovl_v.end(), [](bool b){ return !b; })) {
                    // if all false
                    continue;
                }

                Eigen::MatrixXd uv, samples, gtmp, rhs_fun, deform_mat, trafo;
                Eigen::MatrixXd points, normals;
                Eigen::VectorXd weights, normal;

                for (int i = 0; i < lb.size(); ++i) {
                    if (!contains_ovl_v[i]) {
                        continue;
                    }
                    int gid = lb.global_primitive_id(i);
                    /*
                    bool normal_flipped = false;
                    // only use normals
                    utils::BoundarySampler::boundary_quadrature(lb,
                        state.n_boundary_samples(), *state.mesh, i, false, uv, points, normals, weights);
                    assembler::ElementAssemblyValues vals;
                    vals.compute(e, state.mesh->is_volume(), points, state.bases[e], state.geom_bases()[e]);
                    normal = normals.row(1);
                    normal = vals.jac_it[0] * normal; // assuming linear geometry
                    normal.normalize();
                    */
                    polyfem::get_normal_outward(state, e, gid, normal);
                    for (int lv_id = 0; lv_id < state.mesh->dimension(); ++lv_id) {
                        auto v_id = state.mesh->boundary_element_vertex(gid, lv_id);
                        auto v = state.mesh->boundary_element_vertex(lb.global_primitive_id(i), lv_id);
                        auto x = state.mesh->point(v);
                        if (v_id < 0) {
                            continue;
                        }
                        assert(solution.cols() == normal.rows());
                        auto n_id = state.mesh_nodes->primitive_to_node()[v_id];
                        solution.row(n_id) += -1 * eps * normal.transpose();
                    //std::cout << "x: (" << x[0] << ", "<< x[1] << ") normal : (" << normal[0] << ", " << normal[1] << ")";
                    }
                    //std::cout << std::endl;
                }
            }
            sol = utils::flatten(solution);
            const Eigen::MatrixXd relaxed = state.collision_mesh.displace_vertices(
            utils::unflatten(sol, state.mesh->dimension()));
            if (!ipc::has_intersections(state.collision_mesh, relaxed)) {
              std::cout << "relaxing overlaps done: eps=" << eps << ", |u|=" << sol.norm() << std::endl;
                  break;
            } else {
                  eps *= 1.2;
            }
        }
    }
}