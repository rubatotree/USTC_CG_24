#include "MassSpring.h"
#include <iostream>
#include <chrono>

namespace USTC_CG::node_mass_spring {
MassSpring::MassSpring(const Eigen::MatrixXd& X, const EdgeSet& E)
{
    this->X = this->init_X = X;
    this->vel = Eigen::MatrixXd::Zero(X.rows(), X.cols());
    this->E = E;

    std::cout << "number of edges: " << E.size() << std::endl;
    std::cout << "init mass spring" << std::endl;

    // Compute the rest pose edge length
    for (const auto& e : E) {
        Eigen::Vector3d x0 = X.row(e.first);
        Eigen::Vector3d x1 = X.row(e.second);
        this->E_rest_length.push_back((x0 - x1).norm());
    }

    // Initialize the mask for Dirichlet boundary condition
    dirichlet_bc_mask.resize(X.rows(), false);

    // (HW_TODO) Fix two vertices, feel free to modify this 

    unsigned n_fix = sqrt(X.rows());  // Here we assume the cloth is square
    dirichlet_bc_mask[0] = true;
    dirichlet_bc_mask[n_fix - 1] = true;

    int n_vertices = X.rows();
    n_active = n_vertices - 2;

	// Select Matrix (K & Kt) 
	std::vector<Triplet<double>> tripletList;
	tripletList.reserve(n_active);

	K = SparseMatrix<double>(3 * n_active, 3 * n_vertices);
	Kt = SparseMatrix<double>(3 * n_vertices, 3 * n_active);
	int active_i = 0;
	for (int i = 0; i < n_vertices; i++)
	{
        if (!dirichlet_bc_mask[i])
        {
            tripletList.push_back({ active_i * 3 + 0, i * 3 + 0, 1 });
			tripletList.push_back({ active_i * 3 + 1, i * 3 + 1, 1 });
			tripletList.push_back({ active_i * 3 + 2, i * 3 + 2, 1 });
			active_i++;
        }
	}
	K.setFromTriplets(tripletList.begin(), tripletList.end());
	Kt = K.transpose();

	// Mass Matrix (M & Minv)
	M = SparseMatrix<double>(n_vertices * 3, n_vertices * 3);
	Minv = SparseMatrix<double>(n_vertices * 3, n_vertices * 3);
	M.setIdentity();
	Minv.setIdentity();
	M *= mass / n_vertices;
	Minv *= n_vertices / mass;
}

void MassSpring::step()
{
	TIC(step)
    static int step_n = 0;
    static double sum_step_time = 0;
    static int sum_itr = 0;
    step_n++;
    Eigen::Vector3d acceleration_ext = gravity + wind_ext_acc;

    unsigned n_vertices = X.rows();

    // The reason to not use 1.0 as mass per vertex: the cloth gets heavier as we increase the resolution
    double mass_per_vertex =
        mass / n_vertices; 

    //----------------------------------------------------
    // (HW Optional) Bonus part: Sphere collision
    Eigen::MatrixXd acceleration_collision =
        getSphereCollisionForce(sphere_center.cast<double>(), sphere_radius);
    //----------------------------------------------------

    if (time_integrator == IMPLICIT_EULER) {
        // Implicit Euler

        // compute Y 
		VectorXd x = VectorXd::Zero(n_vertices * 3);
		VectorXd y = VectorXd::Zero(n_vertices * 3);
		VectorXd b = VectorXd::Zero(n_vertices * 3);
		VectorXd xa = VectorXd::Zero(n_active * 3);
		VectorXd xa_prev = VectorXd::Zero(n_active * 3);
		VectorXd f_int = VectorXd::Zero(n_vertices * 3);
		MatrixXd X_prev = X;

        for (int i = 0; i < n_vertices; i++)
        {
			Vector3d acc_ext = acceleration_ext;
			if (enable_sphere_collision) {
				acc_ext += acceleration_collision.row(i);
			}
            for (int c = 0; c < 3; c++)
            {
				x(i * 3 + c) = X(i, c);
				// a_ext = m^-1 * f_ext, so there's no Minv.
                y(i * 3 + c) = X(i, c) + h * vel(i, c) + h * h * acc_ext(c);
			}
        }
        b = x - Kt * K * x;
        x = y;// initial guess
        xa = K * x; 

        const int itr_max = 20;
        const double itr_epsilon = 1e-3;

        int itr;
        for (itr = 0; itr < itr_max; itr++)
        {
			auto H_elastic = computeHessianSparse(stiffness);  // size = [nx3, nx3]
			auto egrad = computeGrad(stiffness);
			for (int i = 0; i < n_vertices; i++)
				for (int c = 0; c < 3; c++)
                    f_int(i * 3 + c) = -egrad(i, c);
			VectorXd g = K * (M * (x - y) - h * h * f_int);
			auto g_diff_ = K * (M - h * h * H_elastic) * Kt;

			// Solve Newton's search direction with linear solver 
            SimplicialCholesky<SparseMatrix<double>> split_(g_diff_);
			xa = split_.solve(g_diff_ * xa - g);
            x = Kt * xa + b;

            // update X
            for (int i = 0; i < n_vertices; i++)
				for (int c = 0; c < 3; c++)
                    X(i, c) = x(i * 3 + c);

            if (itr > 0 && (xa_prev - xa).norm() < itr_epsilon) break;
            xa_prev = xa;
        }
        vel = (X - X_prev) / h;
        if (enable_damping) vel *= pow(damping, h);

        sum_itr += itr + 1;
        // std::cout << "Average Iteration Number: " << (double)sum_itr / step_n << std::endl;
    }
    else if (time_integrator == SEMI_IMPLICIT_EULER) {

        // Semi-implicit Euler
        Eigen::MatrixXd acceleration = -computeGrad(stiffness) / mass_per_vertex;
        acceleration.rowwise() += acceleration_ext.transpose();

        // -----------------------------------------------
        // (HW Optional)
        if (enable_sphere_collision) {
            acceleration += acceleration_collision;
        }
        // -----------------------------------------------

        // (HW TODO): Implement semi-implicit Euler time integration
        vel += h * acceleration;
        for (int i = 0; i < n_vertices; i++) 
        {
            if (dirichlet_bc_mask[i]) 
            {
				vel.row(i).setZero();
			}
		}
        if (enable_damping) vel *= pow(damping, h);
        X += vel * h;
        // Update X and vel 

        const bool calculate_energy = false; 
        if (calculate_energy)
		{
			double energy1 = 0, energy2 = 0, energy3 = 0;
			for (int i = 0; i < n_vertices; i++) 
			{
				energy1 += 0.5 * mass_per_vertex * vel.row(i).squaredNorm();
				energy2 += -mass_per_vertex * gravity.dot(X.row(i));
			}
			int i = 0;
			for (const auto& e : E) 
			{
				energy3 += 0.5 * stiffness * pow((X.row(e.first) - X.row(e.second)).norm() - E_rest_length[i], 2);
				i++;
			}
			std::cout << step_n << ": " << energy1 + energy2 + energy3 << " = \t" << energy1 << " + \t"
					  << energy2 << " + \t" << energy3
					  << std::endl;
        }
    }
    else {
        std::cerr << "Unknown time integrator!" << std::endl;
        return;
    }

    TOC(step)
    double steptime = std::chrono::duration_cast<std::chrono::microseconds>(end_step - start_step).count();
    sum_step_time += steptime;
    // std::cout << "Average Step Time: " << sum_step_time / step_n << " microseconds.\n";
}

// There are different types of mass spring energy:
// For this homework we will adopt Prof. Huamin Wang's energy definition introduced in GAMES103
// course Lecture 2 E = 0.5 * stiffness * sum_{i=1}^{n} (||x_i - x_j|| - l)^2 There exist other
// types of energy definition, e.g., Prof. Minchen Li's energy definition
// https://www.cs.cmu.edu/~15769-f23/lec/3_Mass_Spring_Systems.pdf
double MassSpring::computeEnergy(double stiffness)
{
    double sum = 0.;
    unsigned i = 0;
    for (const auto& e : E) {
        auto diff = X.row(e.first) - X.row(e.second);
        auto l = E_rest_length[i];
        sum += 0.5 * stiffness * std::pow((diff.norm() - l), 2);
        i++;
    }
    return sum;
}

Eigen::MatrixXd MassSpring::computeGrad(double stiffness)
{
    Eigen::MatrixXd g = Eigen::MatrixXd::Zero(X.rows(), X.cols());
    unsigned i = 0;
    const double epsilon = 0.0001;
    for (const auto& e : E)
    {
        // --------------------------------------------------
        // (HW TODO): Implement the gradient computation
        double cur_len = (X.row(e.first) - X.row(e.second)).norm();
        double cof = stiffness * (cur_len - E_rest_length[i]) / cur_len;
		g.row(e.first) += cof * (X.row(e.first) - X.row(e.second));
		g.row(e.second) += cof * (X.row(e.second) - X.row(e.first));
        // --------------------------------------------------
        i++;
    }
    return g;
}

Eigen::SparseMatrix<double> MassSpring::computeHessianSparse(double stiffness)
{
    unsigned n_vertices = X.rows();
    Eigen::SparseMatrix<double> H(n_vertices * 3, n_vertices * 3);
	std::vector<Triplet<double>> tripletList;

    unsigned i = 0;
    auto k = stiffness;
    const auto I = Eigen::MatrixXd::Identity(3, 3);
    for (const auto& e : E) {
        // --------------------------------------------------
        // (HW TODO): Implement the sparse version Hessian computation
        // Remember to consider fixed points 
        // You can also consider positive definiteness here
       
        // --------------------------------------------------
        Vector3d r = X.row(e.first).transpose() - X.row(e.second).transpose();
		double l = E_rest_length[i];
        double rl = r.norm();
	    Matrix3d he = k * (l / rl - 1) * Matrix3d::Identity() - k * l / rl / rl / rl * r * r.transpose();

        // Method 1
        if (l > rl)
            he = -k * l / rl / rl / rl * r * r.transpose();

        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 3; c++)
            {
				tripletList.push_back({3 * e.first  + r, 3 * e.first  + c, he(r, c)});
				tripletList.push_back({3 * e.second + r, 3 * e.second + c, he(r, c)});
				tripletList.push_back({3 * e.first  + r, 3 * e.second + c, -he(r, c)});
				tripletList.push_back({3 * e.second + r, 3 * e.first  + c, -he(r, c)});
			}
        }
        i++;
    }
    H.setFromTriplets(tripletList.begin(), tripletList.end());
    H.makeCompressed();
    return H;
}


bool MassSpring::checkSPD(const Eigen::SparseMatrix<double>& A)
{
    // Eigen::SimplicialLDLT<SparseMatrix_d> ldlt(A);
    // return ldlt.info() == Eigen::Success;
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(A);
    auto eigen_values = es.eigenvalues();
    return eigen_values.minCoeff() >= 1e-10;
}

void MassSpring::reset()
{
    std::cout << "reset" << std::endl;
    this->X = this->init_X;
    this->vel.setZero();
}

// ----------------------------------------------------------------------------------
// (HW Optional) Bonus part
Eigen::MatrixXd MassSpring::getSphereCollisionForce(Eigen::Vector3d center, double radius)
{
    Eigen::MatrixXd force = Eigen::MatrixXd::Zero(X.rows(), X.cols());
    for (int i = 0; i < X.rows(); i++) {
       // (HW Optional) Implement penalty-based force here 
        Vector3d vec = X.row(i).transpose() - center;
        double dist = vec.norm();
        if (dist < collision_scale_factor * radius)
			force.row(i) = collision_penalty_k * (collision_scale_factor * radius - dist) * vec / dist;
    }
    return force;
}
// ----------------------------------------------------------------------------------


}  // namespace USTC_CG::node_mass_spring

