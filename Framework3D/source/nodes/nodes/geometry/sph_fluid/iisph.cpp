#include "iisph.h"
#include <iostream>

namespace USTC_CG::node_sph_fluid {

using namespace Eigen;

IISPH::IISPH(const MatrixXd& X, const Vector3d& box_min, const Vector3d& box_max)
    : SPHBase(X, box_min, box_max)
{
    // (HW TODO) Feel free to modify this part to remove or add necessary member variables
    predict_density_ = VectorXd::Zero(ps_.particles().size());
    origin_density_ = VectorXd::Zero(ps_.particles().size());
    aii_ = VectorXd::Zero(ps_.particles().size());
    Api_ = VectorXd::Zero(ps_.particles().size());
    b_ = VectorXd::Zero(ps_.particles().size());
    last_pressure_ = VectorXd::Zero(ps_.particles().size());
}

void IISPH::step()
{
    // (HW Optional)
	ps().assign_particles_to_cells();
	ps().search_neighbors();
    predict_advection();
    compute_pressure();
    compute_pressure_gradient_acceleration();
    advect();
}

void IISPH::compute_pressure()
{
    // (HW Optional) solve pressure using relaxed Jacobi iteration 
    // Something like this: 
    double threshold = 0.001;
    for (unsigned iter = 0; iter < max_iter_; iter++) {
        double avg_density_error = pressure_solve_iteration();
        std::cout << "iter #" << iter + 1 << " err = " << avg_density_error << std::endl;
        if (avg_density_error < threshold)
            break;
    }
    for (int i = 0; i < ps().particles().size(); i++) {
		auto& p = ps().particles()[i];
        last_pressure_(i) = p->pressure();
    }
}

void IISPH::predict_advection()
{
    compute_density();    // rho_0
    compute_non_pressure_acceleration();  // a_adv

// #pragma omp parallel for
    for (int i = 0; i < ps().particles().size(); i++) {
		auto& p = ps().particles()[i];
        p->vel() += dt() * p->acceleration(); // v_adv
        p->pressure() = last_pressure_(i) * 0.5;
    }
//#pragma omp parallel for
    for (int i = 0; i < ps().particles().size(); i++) {
        auto& p = ps().particles()[i];
        Vector3d dii = Vector3d::Zero();
        predict_density_[i] = p->density();  // rho_*
        for (auto& q : p->neighbors()) {
            predict_density_[i] +=
                dt() * ps().mass() * (p->vel() - q->vel()).dot(grad_W(p->x() - q->x(), ps().h()));
            dii += ps().mass() / p->density() / p->density() * grad_W(p->x() - q->x(), ps().h());
        }
        if(p->density() != predict_density_[i] && i == 10000)std::cout << "#" << i << " " << p->density() << "->" << predict_density_[i] << std::endl;
		aii_[i] = 0;
        for (auto& q : p->neighbors()) {
            Vector3d dji = ps().mass() / p->density() / p->density() * grad_W(q->x() - p->x(), ps().h());
            aii_[i] -= ps().mass() * (dii - dji).dot(grad_W(p->x() - q->x(), ps().h()));
        }
        b_[i] = (predict_density_[i] - p->density()) / dt() / dt();
    }
    compute_pressure_gradient_acceleration();
}

void IISPH::compute_predict_density()
{
	for (int i = 0; i < ps().particles().size(); i++) {
        auto& p = ps().particles()[i];
		// p->density() = origin_density_[i];
        predict_density_[i] = p->density();
        Vector3d px, pv, qx, qv;
		pv = p->vel() + dt() * p->acceleration();
        px = p->x() + dt() * pv;
        for (auto& q : p->neighbors()) {
            qv = q->vel() + dt() * q->acceleration();
            qx = q->x() + dt() * qv;
            predict_density_[i] += dt() * ps().mass() * (pv - qv).dot(grad_W(px - qx, ps().h()));
        }
	}
}

double IISPH::pressure_solve_iteration()
{
    // (HW Optional)   
    // One step iteration to solve the pressure poisson equation of IISPH
    double err = 0;
    //#pragma omp parallel for
    for (int i = 0; i < ps().particles().size(); i++) {
		auto& p = ps().particles()[i];
        Api_[i] = 0;
        for (auto& q : p->neighbors()) {
            Api_[i] += ps().mass() * (p->acceleration() - q->acceleration()).dot(grad_W(p->x() - q->x(), ps().h()));
        }
        if (i == 10000)
            std::cout << p->pressure() << " + " << omega() << " / " << aii_[i] << " * (" << b_[i]
                      << " - " << Api_[i] << ") = ";
        p->pressure() = p->pressure() + omega() / aii_[i] * (b_[i] - Api_[i]);
        if (i == 10000)
            std::cout << p->pressure() << std::endl;
	}
    compute_pressure_gradient_acceleration();
	compute_predict_density();
	for (int i = 0; i < ps().particles().size(); i++) {
        auto& p = ps().particles()[i];
        err += abs(predict_density_[i] - p->density());
        if (i == 10000)
            std::cout << "acc = \n" << p->acceleration() << std::endl;
        if(p->density() != predict_density_[i] && i == 10000)std::cout << "#" << i << " " << p->density() << "->" << predict_density_[i] << std::endl;
	}
    return err; 
}

// ------------------ helper function, no need to modify ---------------------
void IISPH::reset()
{
    SPHBase::reset();

    predict_density_ = VectorXd::Zero(ps_.particles().size());
    aii_ = VectorXd::Zero(ps_.particles().size());
    Api_ = VectorXd::Zero(ps_.particles().size());
    b_ = VectorXd::Zero(ps_.particles().size());
    origin_density_ = VectorXd::Zero(ps_.particles().size());
    last_pressure_ = VectorXd::Zero(ps_.particles().size());
}
}  // namespace USTC_CG::node_sph_fluid