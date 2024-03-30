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

namespace USTC_CG::node_asap_itr {

struct TriangleASAP_Itr
{
	Eigen::Matrix2f Lt = Eigen::Matrix2f::Identity();
	int idx_mesh[3] = { -1, -1, -1 };
	Eigen::Vector2f coord[3] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
	Eigen::Vector2f texcoord[3] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
	TriangleASAP_Itr()
	{
	}
	TriangleASAP_Itr(
		OpenMesh::SmartFaceHandle& face,
		const std::shared_ptr<USTC_CG::PolyMesh>& mesh,
		const pxr::VtArray<pxr::GfVec2f>& texcoords)
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
		update_texcoords(texcoords);
	}
	int index(int mesh_idx)
	{
		for (int i = 0; i < 3; i++) {
			if (idx_mesh[i] == mesh_idx)
				return i;
		}
		return -1;
	}
	double cot(int local_idx)
	{
		Eigen::Vector2f p1 = coord[(local_idx + 1) % 3] - coord[local_idx];
		Eigen::Vector2f p2 = coord[(local_idx + 2) % 3] - coord[local_idx];
		return p1.dot(p2) / abs(p1[0] * p2[1] - p1[1] * p2[0]);
	}
	void update_texcoords(const pxr::VtArray<pxr::GfVec2f>& texcoords)
	{
		for (int i = 0; i < 3; i++)
			texcoord[i] = { texcoords[idx_mesh[i]][0], texcoords[idx_mesh[i]][1] };
	}
	void update_Lt()
	{
		Eigen::Matrix2f St = Eigen::Matrix2f::Zero();
		for (int i = 0; i < 3; i++) {
			int j = (i + 1) % 3;
			int k = (i + 2) % 3;
			St += cot(k) * (texcoord[i] - texcoord[j]) * (coord[i] - coord[j]).transpose();
		}
		Eigen::Matrix2f matX = Eigen::Matrix2f(),
						matU = Eigen::Matrix2f();
		matX << coord[1], coord[2];
		matU << texcoord[1] - texcoord[0], texcoord[2] - texcoord[0];
		St = matU * matX.inverse();
		Eigen::JacobiSVD<Eigen::Matrix2f> svd(St, Eigen::ComputeFullU | Eigen::ComputeFullV);
		float singular_avg = (svd.singularValues()[0] + svd.singularValues()[1]) / 2;
		Lt = svd.matrixU() * (Eigen::Matrix2f::Identity() * singular_avg) * svd.matrixV().transpose();
	}
	int oppose(int local_idx_i, int local_idx_j)
	{
		return 0 + 1 + 2 - local_idx_i - local_idx_j;
	}
	double energy()
	{
		double result = 0;
		for (int i = 0; i < 3; i++) {
			int j = (i + 1) % 3;
			auto uij_om = texcoord[i] - texcoord[j];
			auto xij_om = coord[i] - coord[j];
			auto uij_eigen = Eigen::Vector2f(uij_om[0], uij_om[1]);
			auto xij_eigen = Eigen::Vector2f(xij_om[0], xij_om[1]);
			result += (uij_eigen - Lt * xij_eigen).squaredNorm();
		}
		return result / 2;
	}
};

class ParameterASAP_Iteration
{
   private:
    const std::shared_ptr<USTC_CG::PolyMesh>& mesh;
    pxr::VtArray<pxr::GfVec2f> texcoords;
    int iterate_number;
    int fixed_point1, fixed_point2;
    bool compress = true;
    Eigen::SimplicialCholesky<Eigen::SparseMatrix<float>> solver_;
    Eigen::SparseMatrix<float> matrix_;
    std::vector<TriangleASAP_Itr> triangle_maps;
    void local_phase()
    {
        for (auto face : mesh->all_faces()) 
        {
            triangle_maps[face.idx()].update_texcoords(texcoords);
            triangle_maps[face.idx()].update_Lt();
        }
    }
    void global_phase()
    {
        int size = mesh->n_vertices();

        Eigen::VectorXf U[2] = { Eigen::VectorXf(size), Eigen::VectorXf(size) },
                 B[2] = { Eigen::VectorXf(size), Eigen::VectorXf(size) };
		
		B[0](fixed_point1) = texcoords[fixed_point1][0];
		B[1](fixed_point1) = texcoords[fixed_point1][1];

        auto fixed_point1_coord = Eigen::Vector2f(texcoords[fixed_point1][0], texcoords[fixed_point1][1]);
        auto fixed_point2_coord = Eigen::Vector2f(texcoords[fixed_point2][0], texcoords[fixed_point2][1]);
        for (auto vi : mesh->vertices()) {
            int idx_i = vi.idx();

            if (idx_i == fixed_point1)
                continue; 

            float weight_sum = 0;

            Eigen::Vector2f Bi = Eigen::Vector2f::Zero();
            for (auto outedge : vi.outgoing_halfedges_ccw()) {
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
                    Bi += cot * tri.Lt * (tri.coord[loc_i] - tri.coord[loc_j]); 
                }
                if (face_opp != -1)
                {
                    auto& tri = triangle_maps[face_opp];
                    int loc_i = tri.index(idx_i), loc_j = tri.index(idx_j);
                    auto cot = tri.cot(tri.oppose(loc_i, loc_j));
                    cot_weight += cot;
                    Bi += cot * tri.Lt * (tri.coord[loc_i] - tri.coord[loc_j]);
                }
                if (idx_j == fixed_point1)
                {
                    Bi[0] += cot_weight * fixed_point1_coord[0];
                    Bi[1] += cot_weight * fixed_point1_coord[1];
                }
                weight_sum += cot_weight; 
            }
            B[0](idx_i) = Bi.x();
            B[1](idx_i) = Bi.y();
        }

        for (int c = 0; c < 2; c++)
            U[c] = solver_.solve(B[c]);

        auto fixed_point2_coord_upd = Eigen::Vector2f(U[0](fixed_point2), U[1](fixed_point2));
        auto vsrc = (fixed_point2_coord_upd - fixed_point1_coord).normalized();
        auto vdest = (fixed_point2_coord - fixed_point1_coord).normalized();
        auto zrotate = Eigen::Vector2f(
            vdest[0] * vsrc[0] + vdest[1] * vsrc[1], -vdest[0] * vsrc[1] + vdest[1] * vsrc[0]);

        for (int i = 0; i < size; i++)
        {
            auto relate_coord = Eigen::Vector2f(U[0](i), U[1](i)) - fixed_point1_coord;
            auto relate_coord_rotated = Eigen::Vector2f(
                relate_coord[0] * zrotate[0] - relate_coord[1] * zrotate[1],
                relate_coord[0] * zrotate[1] + relate_coord[1] * zrotate[0]);
            auto texcoord_eigen = fixed_point1_coord + relate_coord_rotated;
            texcoords[i] = { texcoord_eigen[0], texcoord_eigen[1] };
        }
    }
   
   public:
    ParameterASAP_Iteration(
        std::shared_ptr<USTC_CG::PolyMesh>& mesh,
        pxr::VtArray<pxr::GfVec2f> texcoords,
        int iterate_number,
        bool compress = true)
        : mesh(mesh),
          texcoords(texcoords),
          iterate_number(iterate_number),
          compress(compress)
    {
        triangle_maps.resize(mesh->n_faces());
        for (auto face : mesh->all_faces()) {
            triangle_maps[face.idx()] = TriangleASAP_Itr(face, mesh, texcoords);
        }

        fixed_point1 = 0;
        fixed_point2 = 0;
        double min1 = FLT_MAX, min2 = FLT_MAX;
        for (auto vertex : mesh->vertices()) {
            int idx = vertex.idx();
            auto texcoord = texcoords[idx];
            double len1 = texcoord[0] * texcoord[0] + texcoord[1] * texcoord[1];
            double len2 =
                (1 - texcoord[0]) * (1 - texcoord[0]) + (1 - texcoord[1]) * (1 - texcoord[1]);
            if (len1 < min1 && len1 > 0.1) {
                min1 = len1;
                fixed_point1 = idx;
            }
            if (len2 < min2 && len2 > 0.1) {
                min2 = len2;
                fixed_point2 = idx;
            }
        }
        
        // Precalculate Matrix
        matrix_ = Eigen::SparseMatrix<float>(mesh->n_vertices(), mesh->n_vertices());

        std::vector<Eigen::Triplet<float>> tripletList;

        int size = mesh->n_vertices();
        tripletList.reserve(size);
        tripletList.push_back({ fixed_point1, fixed_point1, 1 });

        for (auto vi : mesh->vertices()) {
            int idx_i = vi.idx();

            if (idx_i == fixed_point1)
                continue;

            float weight_sum = 0;

            for (auto outedge : vi.outgoing_halfedges_ccw()) {
                auto vj = outedge.to();
                int idx_j = vj.idx();

                float cot_weight = 0;
                if (!outedge.is_boundary()) {
                    auto& tri = triangle_maps[outedge.face().idx()];
                    int loc_i = tri.index(idx_i), loc_j = tri.index(idx_j);
                    auto cot = tri.cot(tri.oppose(loc_i, loc_j));
                    cot_weight += cot;
                }
                if (!outedge.opp().is_boundary()) {
                    auto& tri = triangle_maps[outedge.opp().face().idx()];
                    int loc_i = tri.index(idx_i), loc_j = tri.index(idx_j);
                    auto cot = tri.cot(tri.oppose(loc_i, loc_j));
                    cot_weight += cot;
                }
                weight_sum += cot_weight;
                if (idx_j != fixed_point1)
                    tripletList.push_back({ idx_i, idx_j, -cot_weight });
            }
            tripletList.push_back({ idx_i, idx_i, weight_sum });
        }

        matrix_.setFromTriplets(tripletList.begin(), tripletList.end());
        matrix_.makeCompressed();

        solver_.compute(matrix_);
	}
    double total_energy()
    {
        double result = 0.0;
        for (auto face : mesh->all_faces()) 
        {
            result += triangle_maps[face.idx()].energy();
        }
        return result;
    }
    decltype(texcoords) compute(bool output = true)
    {
        if (iterate_number > 0) {
            if(output) std::cout << "ASAP Start with Energy: " << total_energy() << std::endl;

            for (int iter = 0; iter < iterate_number; iter++) {
                local_phase();
                if(output) std::cout << "Local #" << iter + 1 << " Energy: " << total_energy() << std::endl;
                global_phase();
                if(output) std::cout << "Global #" << iter + 1 << " Energy: " << total_energy() << std::endl;
            }
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
    b.add_input<decl::Float2Buffer>("Initial Guess");
    b.add_input<decl::Int>("Iterate Number").min(0).max(30).default_val(10);
    b.add_output<decl::Float2Buffer>("OutputUV");
}
static void node_asap_exec(ExeParams params)
{
    auto input = params.get_input<GOperandBase>("Input");
    auto texcoords = params.get_input<pxr::VtArray<pxr::GfVec2f>>("Initial Guess");
    int iterate_number = params.get_input<int>("Iterate Number");

    auto mesh = operand_to_openmesh(&input);

    auto parameterizer = new ParameterASAP_Iteration(mesh, texcoords, iterate_number);

    pxr::VtArray<pxr::GfVec2f> uv_result = parameterizer->compute();

    delete parameterizer; 

    params.set_output("OutputUV", uv_result);
}

static void node_register()
{
    static NodeTypeInfo ntype;

    strcpy(ntype.ui_name, "ASAP Parameterization (Iteration)");
    strcpy_s(ntype.id_name, "geom_asap_itr");

    geo_node_type_base(&ntype);
    ntype.node_execute = node_asap_exec;
    ntype.declare = node_asap_declare;
    nodeRegisterType(&ntype);
}

NOD_REGISTER_NODE(node_register)
}  // namespace USTC_CG::node_asap_itr
