#include "FastMassSpring.h"
#include <iostream>


namespace USTC_CG::node_mass_spring {
FastMassSpring::FastMassSpring(const Eigen::MatrixXd& X, const EdgeSet& E) : MassSpring(X, E)
{
    // construct L and J at initialization
    std::cout << "init fast mass spring" << std::endl;

    unsigned n_vertices = X.rows();

    unsigned n_fix = sqrt(X.rows());  // Here we assume the cloth is square
    dirichlet_bc_mask[0] = true;
    dirichlet_bc_mask[n_fix - 1] = true;

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

void FastMassSpring::step()
{
	TIC(step)
    static int step_n = 0;
    static int sum_itr = 0;
    static double sum_step_time = 0;
    step_n++;

    unsigned n_vertices = X.rows();
	int n_spring = E.size();

	if (!initialized)
	{
		// Fill l
		L = SparseMatrix<double>(n_vertices * 3, n_vertices * 3);
		J = SparseMatrix<double>(n_vertices * 3, n_spring * 3);

		std::vector<Triplet<double>> tripletList_L, tripletList_J;
		tripletList_L.reserve(n_spring * 4);
		tripletList_J.reserve(n_spring * 2);
		int i = 0;
		for (const auto& e : E) 
		{
			int i1 = e.first, i2 = e.second;
			for (int c = 0; c < 3; c++)
			{
				tripletList_L.push_back({i1 * 3 + c, i1 * 3 + c, stiffness});
				tripletList_L.push_back({i2 * 3 + c, i2 * 3 + c, stiffness});
				tripletList_L.push_back({i1 * 3 + c, i2 * 3 + c, -stiffness});
				tripletList_L.push_back({i2 * 3 + c, i1 * 3 + c, -stiffness});
				tripletList_J.push_back({i1 * 3 + c, i * 3 + c, stiffness});
				tripletList_J.push_back({i2 * 3 + c, i * 3 + c, -stiffness});
			}
			i++;
		}
		L.setFromTriplets(tripletList_L.begin(), tripletList_L.end());
		J.setFromTriplets(tripletList_J.begin(), tripletList_J.end());
		solver.compute(K * (M + h * h * L) * Kt);
		initialized = true;

		std::cout << "A precalculated" << std::endl;
	}

    // (HW Optional) Necessary preparation
    // ...
    Eigen::Vector3d acceleration_ext = gravity + wind_ext_acc;
	Eigen::MatrixXd acceleration_collision =
		getSphereCollisionForce(sphere_center.cast<double>(), sphere_radius);

	VectorXd x = VectorXd::Zero(n_vertices * 3);
	VectorXd y = VectorXd::Zero(n_vertices * 3);
	VectorXd b = VectorXd::Zero(n_vertices * 3);
	VectorXd xa = VectorXd::Zero(n_active * 3);
	VectorXd xa_prev = VectorXd::Zero(n_active * 3);
	VectorXd d = VectorXd::Zero(n_spring * 3);
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
			y(i * 3 + c) = X(i, c) + h * vel(i, c) + h * h * acc_ext(c);
		}
	}
	b = x - Kt * K * x;
	x = y;// initial guess
	xa = K * x; 

	const double itr_epsilon = 1e-4;
	unsigned iter;
    for (iter = 0; iter < max_iter; iter++) 
	{
		// Local Phase
		int i = 0;
		for (const auto& e : E)
		{
			Vector3d vec;
			for (int c = 0; c < 3; c++)
				vec(c) = x(e.first * 3 + c) - x(e.second * 3 + c);
			Vector3d di = E_rest_length[i] / vec.norm() * vec;
			for (int c = 0; c < 3; c++) d(i * 3 + c) = di(c);	
			i++;
		}

		// Global Phase
		xa = solver.solve(K * (h * h * J * d + M * y - (M + h * h * L) * b));
		x = Kt * xa + b;

		if (iter > 0 && (xa_prev - xa).norm() < itr_epsilon) break;
		xa_prev = xa;
    }

	// update X
	for (int i = 0; i < n_vertices; i++)
		for (int c = 0; c < 3; c++)
			X(i, c) = x(i * 3 + c);
	
	vel = (X - X_prev) / h;
	if (enable_damping) vel *= pow(damping, h);

    TOC(step)
    double steptime = std::chrono::duration_cast<std::chrono::microseconds>(end_step - start_step).count();
    sum_step_time += steptime;
	sum_itr += iter + 1;
	// std::cout << "Average Iteration Number: " << (double)sum_itr / step_n << std::endl;
    // std::cout << "Average Step Time: " << sum_step_time / step_n << " microseconds.\n";
}

}  // namespace USTC_CG::node_mass_spring
