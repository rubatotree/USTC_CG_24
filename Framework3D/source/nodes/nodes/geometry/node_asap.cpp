#include "GCore/Components/MeshOperand.h"
#include "Nodes/node.hpp"
#include "Nodes/node_declare.hpp"
#include "Nodes/node_register.h"
#include "geom_node_base.h"
#include "utils/util_openmesh_bind.h"
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <iostream>
#include <set>
#include <Eigen/IterativeLinearSolvers>

namespace USTC_CG::node_asap {

struct TriangleASAP
{
	int idx_mesh[3] = { -1, -1, -1 };
	Eigen::Vector2f coord[3] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
	TriangleASAP()
	{
	}
	TriangleASAP(
		OpenMesh::SmartFaceHandle& face,
		const std::shared_ptr<USTC_CG::PolyMesh>& mesh)
	{
		double len[3];
		int i = 0;
		for (auto vh : mesh->fv_cw_range(face)) {
			idx_mesh[i] = vh.idx();
			i++;
		}
		for (int i = 0; i < 3; i++) {
			int j = (i + 1) % 3;
			int k = (i + 2) % 3;
			auto p1 = mesh->point(mesh->vertex_handle(idx_mesh[j]));
			auto p2 = mesh->point(mesh->vertex_handle(idx_mesh[k]));
			len[i] = (p1 - p2).length();
		}
		double cos0 = (len[1] * len[1] + len[2] * len[2] - len[0] * len[0]) / (2 * len[1] * len[2]);
		coord[0] = Eigen::Vector2f(0, 0);
		coord[1] = Eigen::Vector2f(len[2], 0);
		coord[2] = Eigen::Vector2f(len[1] * cos0, len[1] * sqrt(1 - cos0 * cos0));
	}
	double cot(int local_idx)
	{
		Eigen::Vector2f p1 = coord[(local_idx + 1) % 3] - coord[local_idx];
		Eigen::Vector2f p2 = coord[(local_idx + 2) % 3] - coord[local_idx];
		return p1.dot(p2) / abs(p1[0] * p2[1] - p1[1] * p2[0]);
	}
	int index(int mesh_idx)
	{
		for (int i = 0; i < 3; i++) {
			if (idx_mesh[i] == mesh_idx)
				return i;
		}
		return -1;
	}
	int oppose(int local_idx_i, int local_idx_j)
	{
		return 0 + 1 + 2 - local_idx_i - local_idx_j;
	}
};

class ParameterizeASAP
{
   private:
    const std::shared_ptr<USTC_CG::PolyMesh>& mesh;
    int fixed_point1, fixed_point2;
    bool compress = true;
    Eigen::SparseLU<Eigen::SparseMatrix<float>> solver_;
    Eigen::SparseMatrix<float> matrix_;
    std::vector<TriangleASAP> triangle_maps;
   
   public:
    ParameterizeASAP(
        std::shared_ptr<USTC_CG::PolyMesh>& mesh,
        bool compress = true)
        : mesh(mesh),
          compress(compress)
    {
        triangle_maps.resize(mesh->n_faces());
        for (auto face : mesh->all_faces()) {
            triangle_maps[face.idx()] = TriangleASAP(face, mesh);
        }

        fixed_point1 = 0;
        fixed_point2 = 0;
        double xmin = FLT_MAX, xmax = FLT_MIN;
        for (auto vertex : mesh->vertices()) {
            int idx = vertex.idx();
            float x = mesh->point(vertex)[0];
            if (x < xmin)
            {
                xmin = x;
                fixed_point1 = idx;
            }
            if (x > xmax)
            {
                xmax = x;
				fixed_point2 = idx;
            }
        }
        if (fixed_point1 == fixed_point2)
        {
            fixed_point2 = fixed_point1 + 1;
            if (fixed_point2 >= mesh->n_vertices())
				fixed_point2 = fixed_point1 - 1;
        }
	}

    pxr::VtArray<pxr::GfVec2f> compute(bool output = true)
    {
        int vsize = mesh->n_vertices();
        int fsize = mesh->n_faces();
        pxr::VtArray<pxr::GfVec2f> texcoords = pxr::VtArray<pxr::GfVec2f>(vsize);
        
        // x0, x1, ..., xn, y0, y1, ..., yn, a1, a2, ..., at, b1, b2, ..., bt
        matrix_ = Eigen::SparseMatrix<float>(vsize * 2 + fsize * 2, vsize * 2 + fsize * 2);

        std::vector<Eigen::Triplet<float>> tripletList;

        Eigen::VectorXf B = Eigen::VectorXf::Zero(vsize * 2 + fsize * 2);

        tripletList.push_back({ fixed_point1, fixed_point1, 1 });
        tripletList.push_back({ fixed_point2, fixed_point2, 1 });
        tripletList.push_back({ fixed_point1 + vsize, fixed_point1 + vsize, 1 });
        tripletList.push_back({ fixed_point2 + vsize, fixed_point2 + vsize, 1 });
        B(fixed_point1) = 0;
        B(fixed_point1 + vsize) = 0;
        B(fixed_point2) = 1;
        B(fixed_point2 + vsize) = 0;

        for (auto vi : mesh->vertices()) {
            int idx_i = vi.idx();

            if (idx_i == fixed_point1 || idx_i == fixed_point2)
                continue; 

            float weight_sum = 0;

            for (auto outedge : vi.outgoing_halfedges_cw()) {
                auto vj = outedge.to();
                int idx_j = vj.idx();
                float cot_weight = 0;
                int face = outedge.face().idx(), face_opp = outedge.opp().face().idx();
                if (face != -1)
                {
                    auto& tri = triangle_maps[face];
                    int loc_i = tri.index(idx_i), loc_j = tri.index(idx_j);
				    auto cot = tri.cot(tri.oppose(loc_i, loc_j));
                    cot_weight += cot;
                    auto v = -cot * (tri.coord[loc_i] - tri.coord[loc_j]);
                    tripletList.push_back({ idx_i, 2 * vsize + face, v.x() });
                    tripletList.push_back({ idx_i, 2 * vsize + face + fsize, v.y() });
                    tripletList.push_back({ idx_i + vsize, 2 * vsize + face, v.y() });
                    tripletList.push_back({ idx_i + vsize, 2 * vsize + face + fsize, -v.x() });
                }
                if (face_opp != -1)
                {
                    auto& tri = triangle_maps[face_opp];
                    int loc_i = tri.index(idx_i), loc_j = tri.index(idx_j);
                    auto cot = tri.cot(tri.oppose(loc_i, loc_j));
                    cot_weight += cot;
                    auto v = -cot * (tri.coord[loc_i] - tri.coord[loc_j]);
                    tripletList.push_back({ idx_i, 2 * vsize + face_opp, v.x() });
                    tripletList.push_back({ idx_i, 2 * vsize + face_opp + fsize, v.y() });
                    tripletList.push_back({ idx_i + vsize, 2 * vsize + face_opp, v.y() });
                    tripletList.push_back({ idx_i + vsize, 2 * vsize + face_opp + fsize, -v.x() });
                }
                weight_sum += cot_weight; 
                tripletList.push_back({ idx_i, idx_j, -cot_weight });
                tripletList.push_back({ idx_i + vsize, idx_j + vsize, -cot_weight });
            }
            tripletList.push_back({ idx_i, idx_i, weight_sum });
            tripletList.push_back({ idx_i + vsize, idx_i + vsize, weight_sum });
        }

        for (auto face : mesh->all_faces()) {
            int idx = face.idx();
            int a = 2 * vsize + idx, b = 2 * vsize + idx + fsize;
            auto& tri = triangle_maps[idx];
            float c1 = 0; 
            for (int i = 0; i < 3; i++) {
                int j = (i + 1) % 3;
                int k = (i + 2) % 3;
                auto dv = tri.coord[i] - tri.coord[j];
                float cot = tri.cot(k);
                int idx_u = tri.idx_mesh[i], idx_ud = tri.idx_mesh[j];
                
                c1 += cot * (dv.x() * dv.x() + dv.y() * dv.y()); 
                tripletList.push_back({a, idx_u,            -cot * dv.x()});
                tripletList.push_back({a, idx_u + vsize,    -cot * dv.y()});
                tripletList.push_back({a, idx_ud,           +cot * dv.x()});
                tripletList.push_back({a, idx_ud + vsize,   +cot * dv.y()});
                tripletList.push_back({b, idx_u,            -cot * dv.y()});
                tripletList.push_back({b, idx_u + vsize,    +cot * dv.x()});
                tripletList.push_back({b, idx_ud,           +cot * dv.y()});
                tripletList.push_back({b, idx_ud + vsize,   -cot * dv.x()});
            }
            tripletList.push_back({ a, a, c1 });
            tripletList.push_back({ b, b, c1 });
		}

        matrix_.setFromTriplets(tripletList.begin(), tripletList.end());
        matrix_.makeCompressed();

        solver_.compute(matrix_);

        Eigen::VectorXf X = solver_.solve(B);
        for (int i = 0; i < vsize; i++)
        {
			texcoords[i][0] = X(i);
			texcoords[i][1] = X(i + vsize);
		}
        if (compress) {
            float xmin = FLT_MAX, xmax = FLT_MIN, ymin = FLT_MAX, ymax = FLT_MIN;
            for (int i = 0; i < mesh->n_vertices(); i++) {
                xmin = std::min(xmin, texcoords[i][0]);
                xmax = std::max(xmax, texcoords[i][0]);
                ymin = std::min(ymin, texcoords[i][1]);
                ymax = std::max(ymax, texcoords[i][1]);
            }
            for (int i = 0; i < mesh->n_vertices(); i++) {
                texcoords[i][0] -= xmin;
                texcoords[i][1] -= ymin;
                texcoords[i] /= std::max(xmax - xmin, ymax - ymin);
            }
        }
        return texcoords;
    }
};

static void node_asap_declare(NodeDeclarationBuilder& b)
{
    b.add_input<decl::Geometry>("Input");
    b.add_output<decl::Float2Buffer>("OutputUV");
}
static void node_asap_exec(ExeParams params)
{
    auto input = params.get_input<GOperandBase>("Input");

    auto mesh = operand_to_openmesh(&input);

    auto parameterizer = new ParameterizeASAP(mesh);
    pxr::VtArray<pxr::GfVec2f> uv_result = parameterizer->compute();
    delete parameterizer; 

    params.set_output("OutputUV", uv_result);
}

static void node_register()
{
    static NodeTypeInfo ntype;

    strcpy(ntype.ui_name, "ASAP Parameterization");
    strcpy_s(ntype.id_name, "geom_asap");

    geo_node_type_base(&ntype);
    ntype.node_execute = node_asap_exec;
    ntype.declare = node_asap_declare;
    nodeRegisterType(&ntype);
}

NOD_REGISTER_NODE(node_register)
}  // namespace USTC_CG::node_asap
