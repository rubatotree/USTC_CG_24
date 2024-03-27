
#include <iostream>
#include <vector>
#include <tuple>
#include <Eigen/Sparse>
#include "GCore/Components/MeshOperand.h"
#include "Nodes/node.hpp"
#include "Nodes/node_declare.hpp"
#include "Nodes/node_register.h"
#include "geom_node_base.h"
#include "utils/util_openmesh_bind.h"


namespace USTC_CG::node_min_surf_floater {
static void node_min_surf_floater_declare(NodeDeclarationBuilder& b)
{
    b.add_input<decl::Geometry>("Mesh");
    b.add_input<decl::Geometry>("Mesh with Boundary Fixed");

    b.add_output<decl::Geometry>("Output");
}

static float angle(OpenMesh::DefaultTraits::Point vert, OpenMesh::DefaultTraits::Point p1, OpenMesh::DefaultTraits::Point p2)
{
    auto v1 = (p1 - vert).normalized(), v2 = (p2 - vert).normalized();
    double cosine = v1.dot(v2);
    return acos(cosine);
}
static float cross2d(OpenMesh::Vec2f a, OpenMesh::Vec2f b)
{
    return a[0] * b[1] - a[1] * b[0];
}
static void node_min_surf_floater_exec(ExeParams params)
    {
    auto input = params.get_input<GOperandBase>("Mesh");
    auto input_boundary = params.get_input<GOperandBase>("Mesh with Boundary Fixed");
    auto halfedge_mesh = operand_to_openmesh(&input);
    auto halfedge_mesh_fixed = operand_to_openmesh(&input_boundary);
    if (halfedge_mesh_fixed->n_vertices() == 0)
        halfedge_mesh_fixed = operand_to_openmesh(&input);
    
   
    if (halfedge_mesh->n_vertices() > 0) {
        size_t size = halfedge_mesh->n_vertices();
        std::vector<Eigen::Triplet<float>> triplet_list;
        Eigen::VectorXf F[3] = { Eigen::VectorXf(size),
                                 Eigen::VectorXf(size),
                                 Eigen::VectorXf(size) };
        Eigen::VectorXf G[3] = { Eigen::VectorXf(size),
                                 Eigen::VectorXf(size),
                                 Eigen::VectorXf(size) };

        for (auto vertex : halfedge_mesh->vertices()) {
            if (halfedge_mesh_fixed->point(vertex)[2] < 0.1) {
                triplet_list.push_back({ vertex.idx(), vertex.idx(), 1 });
                for (int c = 0; c < 3; c++)
                    G[c](vertex.idx()) = halfedge_mesh_fixed->point(vertex)[c];
            }
            else {
                float weight_sum = 0;
                auto pfrom = halfedge_mesh->point(vertex);
                std::vector<OpenMesh::SmartVertexHandle> neighbor_list;
                int n_neighbor = 0, cur_neighbor_index = 0, prev_idx = -1;
                OpenMesh::SmartVertexHandle cur_neighbor;
                for (auto outedge : vertex.outgoing_halfedges_cw())
                {
                    n_neighbor++;
                    cur_neighbor = outedge.to();
					neighbor_list.push_back(cur_neighbor);
                }
                std::vector<float> weight;
                std::vector<OpenMesh::Vec2f> coord2d;
                weight.resize(n_neighbor, 0);
                coord2d.resize(n_neighbor, { 0, 0 });
                float angle_sum = 0, angle_cur = 0; 
                for (int i = 0; i < n_neighbor; i++)
                {
                    auto neighbor = halfedge_mesh->point(neighbor_list[i]);
                    auto neighbor_nxt = halfedge_mesh->point(neighbor_list[(i + 1) % n_neighbor]);
                    float ang = angle(pfrom, neighbor, neighbor_nxt);
                    if (n_neighbor == 2)
                        ang = M_PI * 2 - ang;
                    angle_sum += ang;
                }
                for (int i = 0; i < n_neighbor; i++)
                {
                    auto neighbor = halfedge_mesh->point(neighbor_list[i]);
                    auto neighbor_nxt = halfedge_mesh->point(neighbor_list[(i + 1) % n_neighbor]);
                    float ang = angle(pfrom, neighbor, neighbor_nxt);
                    if (n_neighbor == 2)
                        ang = M_PI * 2 - ang;
                    float len = (neighbor - pfrom).norm();
                    coord2d[i] = { cos(angle_cur) * len, sin(angle_cur) * len };
                    angle_cur += ang * M_PI * 2 / angle_sum;
                }
                int back_ind = 0;
                for (int i = 0; i < n_neighbor; i++)
                {
                    while (back_ind % n_neighbor == i ||
                        cross2d(coord2d[i], coord2d[i] - coord2d[back_ind % n_neighbor]) *
                           cross2d(coord2d[i], coord2d[i] - coord2d[(back_ind + 1) % n_neighbor]) > 0)
                        back_ind++;
                    auto p1 = coord2d[i];
                    auto p2 = coord2d[(back_ind) % n_neighbor];
                    auto p3 = coord2d[(back_ind + 1) % n_neighbor]; 
                    float s1 = abs(cross2d(p2, p3));
                    float s2 = abs(cross2d(p3, p1));
                    float s3 = abs(cross2d(p1, p2));
                    float s = s1 + s2 + s3;
                    weight[i] += s1 / s;                    
                    weight[(back_ind) % n_neighbor] += s2 / s;
                    weight[(back_ind + 1) % n_neighbor] += s3 / s;
                    weight_sum += 1;
                }
                for (int i = 0; i < n_neighbor; i++)
                {
                    auto neighbor = neighbor_list[i];
                    triplet_list.push_back({ vertex.idx(), neighbor.idx(), weight[i] });
                }
                triplet_list.push_back({ vertex.idx(), vertex.idx(), -weight_sum });
                for (int c = 0; c < 3; c++)
                    G[c](vertex.idx()) = 0;
            }
        }

        Eigen::SparseMatrix<float> mat(size, size);
        mat.setFromTriplets(triplet_list.begin(), triplet_list.end());
        Eigen::SparseLU<Eigen::SparseMatrix<float>> solver;
        solver.compute(mat);
        for (int c = 0; c < 3; c++)
            F[c] = solver.solve(G[c]);

        for (auto vertex : halfedge_mesh->vertices()) {
            int idx = vertex.idx();
            halfedge_mesh->set_point(vertex, { F[0](idx), F[1](idx), F[2](idx) });
        }
    }

    auto operand_base = openmesh_to_operand(halfedge_mesh.get());

    params.set_output("Output", std::move(*operand_base));
}

static void node_register()
{
    static NodeTypeInfo ntype;

    strcpy(ntype.ui_name, "Minimal Surface (Floater Weight)");
    strcpy_s(ntype.id_name, "geom_min_surf_floater");

    geo_node_type_base(&ntype);
    ntype.node_execute = node_min_surf_floater_exec;
    ntype.declare = node_min_surf_floater_declare;
    nodeRegisterType(&ntype);
}

NOD_REGISTER_NODE(node_register)
}  
