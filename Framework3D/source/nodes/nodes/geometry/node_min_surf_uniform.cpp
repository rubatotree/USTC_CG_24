#include "GCore/Components/MeshOperand.h"
#include "Nodes/node.hpp"
#include "Nodes/node_declare.hpp"
#include "Nodes/node_register.h"
#include "geom_node_base.h"
#include "utils/util_openmesh_bind.h"

#include <iostream>
#include <vector>
#include <tuple>
#include <Eigen/Sparse>
/*
** @brief HW4_TutteParameterization
**
** This file presents the basic framework of a "node", which processes inputs
** received from the left and outputs specific variables for downstream nodes to
** use.
** - In the first function, node_declare, you can set up the node's input and
** output variables.
** - The second function, node_exec is the execution part of the node, where we
** need to implement the node's functionality.
** - The third function generates the node's registration information, which
** eventually allows placing this node in the GUI interface.
**
** Your task is to fill in the required logic at the specified locations
** within this template, espacially in node_exec.
*/

namespace USTC_CG::node_min_surf_uniform {
static void node_min_surf_uniform_declare(NodeDeclarationBuilder& b)
{
    // Input-1: Original 3D mesh with boundary
    b.add_input<decl::Geometry>("Input");

    /*
    ** NOTE: You can add more inputs or outputs if necessary. For example, in some cases,
    ** additional information (e.g. other mesh geometry, other parameters) is required to perform
    ** the computation.
    **
    ** Be sure that the input/outputs do not share the same name. You can add one geometry as
    **
    **                b.add_input<decl::Geometry>("Input");
    **
    ** Or maybe you need a value buffer like:
    **
    **                b.add_input<decl::Float1Buffer>("Weights");
    */

    // Output-1: Minimal surface with fixed boundary
    b.add_output<decl::Geometry>("Output");
}

static void node_min_surf_uniform_exec(ExeParams params)
{
    // Get the input from params
    auto input = params.get_input<GOperandBase>("Input");

    // (TO BE UPDATED) Avoid processing the node when there is no input
    // if (!input.get_component<MeshComponent>()) {
    //     throw std::runtime_error("Minimal Surface: Need Geometry Input.");
    // }
    // throw std::runtime_error("Not implemented");

    /* ----------------------------- Preprocess -------------------------------
    ** Create a halfedge structure (using OpenMesh) for the input mesh. The
    ** half-edge data structure is a widely used data structure in geometric
    ** processing, offering convenient operations for traversing and modifying
    ** mesh elements.
    */
    auto halfedge_mesh = operand_to_openmesh(&input);
   
    if (halfedge_mesh->n_vertices() > 0) {
        size_t size = halfedge_mesh->n_vertices();
        std::vector<Eigen::Triplet<float>> triplet_list;
        Eigen::VectorXf F[3] = { Eigen::VectorXf(size),
                                 Eigen::VectorXf(size),
                                 Eigen::VectorXf(size) };
        Eigen::VectorXf G[3] = { Eigen::VectorXf(size),
                                 Eigen::VectorXf(size),
                                 Eigen::VectorXf(size) };
        /* ---------------- [HW4] TASK 1: Minimal Surface --------------------
        ** In this task, you are required to generate a 'minimal surface' mesh with
        ** the boundary of the input mesh as its boundary.
        **
        ** Specifically, the positions of the boundary vertices of the input mesh
        ** should be fixed. By solving a global Laplace equation on the mesh,
        ** recalculate the coordinates of the vertices inside the mesh to achieve
        ** the minimal surface configuration.
        **
        ** (Recall the Poisson equation with Dirichlet Boundary Condition in HW3)
        */

        /*
        ** Algorithm Pseudocode for Minimal Surface Calculation
        ** ------------------------------------------------------------------------
        ** 1. Initialize mesh with input boundary conditions.
        **    - For each boundary vertex, fix its position.
        **    - For internal vertices, initialize with initial guess if necessary.
        */
        /* 2. Construct Laplacian matrix for the mesh.
        **    - Compute weights for each edge based on the chosen weighting scheme
        **      (e.g., uniform weights for simplicity).
        **    - Assemble the global Laplacian matrix.
        */
        for (auto vertex : halfedge_mesh->vertices()) {
            if (vertex.is_boundary()) {
                triplet_list.push_back({ vertex.idx(), vertex.idx(), 1 });
                for (int c = 0; c < 3; c++)
                    G[c](vertex.idx()) = halfedge_mesh->point(vertex)[c];
            }
            else {
                float weight_sum = 0;
                for (auto outedge : vertex.outgoing_halfedges()) {
                    auto neighbor = outedge.to();
                    float weight = 1;
                    triplet_list.push_back({ vertex.idx(), neighbor.idx(), weight });
                    weight_sum += weight;
                }
                triplet_list.push_back({ vertex.idx(), vertex.idx(), -weight_sum });
                for (int c = 0; c < 3; c++)
                    G[c](vertex.idx()) = 0;
            }
        }
        /* 3. Solve the Laplace equation for interior vertices.
        **    - Apply Dirichlet boundary conditions for boundary vertices.
        **    - Solve the linear system (Laplacian * X = 0) to find new positions
        **      for internal vertices.
        */

        Eigen::SparseMatrix<float> mat(size, size);
        mat.setFromTriplets(triplet_list.begin(), triplet_list.end());
        Eigen::SparseLU<Eigen::SparseMatrix<float>> solver;
        solver.compute(mat);
        for (int c = 0; c < 3; c++)
            F[c] = solver.solve(G[c]);

        /* 4. Update mesh geometry with new vertex positions.
        **    - Ensure the mesh respects the minimal surface configuration.
        */

        for (auto vertex : halfedge_mesh->vertices()) {
            int idx = vertex.idx();
            halfedge_mesh->set_point(vertex, { F[0](idx), F[1](idx), F[2](idx) });
        }

        /* Note: This pseudocode outlines the general steps for calculating a
        ** minimal surface mesh given fixed boundary conditions using the Laplace
        ** equation. The specific implementation details may vary based on the mesh
        ** representation and numerical methods used.
        **
        */
    }

    /* ----------------------------- Postprocess ------------------------------
    ** Convert the minimal surface mesh from the halfedge structure back to
    ** GOperandBase format as the node's output.
    */
    
    auto operand_base = openmesh_to_operand(halfedge_mesh.get());
    
    // Set the output of the nodes
    params.set_output("Output", std::move(*operand_base));
}

static void node_register()
{
    static NodeTypeInfo ntype;

    strcpy(ntype.ui_name, "Minimal Surface (Uniform Weight)");
    strcpy_s(ntype.id_name, "geom_min_surf_uniform");

    geo_node_type_base(&ntype);
    ntype.node_execute = node_min_surf_uniform_exec;
    ntype.declare = node_min_surf_uniform_declare;
    nodeRegisterType(&ntype);
}

NOD_REGISTER_NODE(node_register)
}  // namespace USTC_CG::node_min_surf
