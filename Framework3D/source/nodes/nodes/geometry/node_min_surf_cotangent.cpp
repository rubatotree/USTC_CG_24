#include <Eigen/Sparse>
#include <iostream>
#include <tuple>
#include <vector>

#include "GCore/Components/MeshOperand.h"
#include "Nodes/node.hpp"
#include "Nodes/node_declare.hpp"
#include "Nodes/node_register.h"
#include "geom_node_base.h"
#include "utils/util_openmesh_bind.h"

namespace USTC_CG::node_min_surf_cotangent {
static void node_min_surf_cotangent_declare(NodeDeclarationBuilder& b)
{
    b.add_input<decl::Geometry>("Mesh");
    b.add_input<decl::Geometry>("Mesh with Boundary Fixed");

    b.add_output<decl::Geometry>("Output");
}

static float cotangent(OpenMesh::DefaultTraits::Point vert, OpenMesh::DefaultTraits::Point p1, OpenMesh::DefaultTraits::Point p2)
{
    auto v1 = (p1 - vert).normalized(), v2 = (p2 - vert).normalized();
    return v1.dot(v2) / v1.cross(v2).norm();
}

static void node_min_surf_cotangent_exec(ExeParams params)
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
            if (vertex.is_boundary()) {
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
                for (int i = 0; i < n_neighbor; i++)
                {
                    auto neighbor = neighbor_list[i];
                    auto pto = halfedge_mesh->point(neighbor);
                    auto pprev = halfedge_mesh->point(neighbor_list[(i - 1 + n_neighbor) % n_neighbor]),
                         pnext = halfedge_mesh->point(neighbor_list[(i + 1) % n_neighbor]);
                    float weight = cotangent(pprev, pfrom, pto) + cotangent(pnext, pfrom, pto);
                    triplet_list.push_back({ vertex.idx(), neighbor.idx(), weight });
                    weight_sum += weight;
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

    strcpy(ntype.ui_name, "Minimal Surface (Cotangent Weight)");
    strcpy_s(ntype.id_name, "geom_min_surf_cotangent");

    geo_node_type_base(&ntype);
    ntype.node_execute = node_min_surf_cotangent_exec;
    ntype.declare = node_min_surf_cotangent_declare;
    nodeRegisterType(&ntype);
}

NOD_REGISTER_NODE(node_register)
} 
