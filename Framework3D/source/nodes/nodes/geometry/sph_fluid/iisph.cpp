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
    omp_set_num_threads(32);
}

void IISPH::step()
{
    frame++;
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
    unsigned iter;
    for (iter = 0; iter < max_iter_; iter++) {
        double avg_density_error = pressure_solve_iteration();
		// std::cout << "#" << frame << ": " << iter + 1 << " iters | err = " << avg_density_error << std::endl;
        if (avg_density_error < threshold)
            break;
    }
    std::cout << "#" << frame << ": " << iter + 1 << " iters\n";

#pragma omp parallel for
    for (int i = 0; i < ps().particles().size(); i++) {
		auto& p = ps().particles()[i];
        last_pressure_(i) = p->pressure();
    }
}

void IISPH::predict_advection()
{
    compute_density();    // rho_0
    double sum = 0;
    for (int i = 0; i < ps().particles().size(); i++) {
        sum += ps().particles()[i]->density();
    }
    //std::cout << "avg: " << sum / ps().particles().size() << std::endl;

    compute_non_pressure_acceleration();  // a_adv

#pragma omp parallel for
    for (int i = 0; i < ps().particles().size(); i++) {
		auto& p = ps().particles()[i];
        p->vel() += dt() * p->acceleration(); // v_adv
        p->pressure() = last_pressure_(i) * 0.5;
    }
#pragma omp parallel for
    for (int i = 0; i < ps().particles().size(); i++) {
        auto& p = ps().particles()[i];
        Vector3d dii = Vector3d::Zero();
        predict_density_[i] = p->density();  // rho_*
        for (auto& q : p->neighbors()) {
            predict_density_[i] +=
                dt() * ps().mass() * (p->vel() - q->vel()).dot(grad_W(p->x() - q->x(), ps().h()));
            dii += ps().mass() / p->density() / p->density() * grad_W(p->x() - q->x(), ps().h());
        }
		aii_[i] = 0;
        for (auto& q : p->neighbors()) {
            Vector3d dji = ps().mass() / p->density() / p->density() * grad_W(q->x() - p->x(), ps().h());
            aii_[i] -= ps().mass() * (dii - dji).dot(grad_W(p->x() - q->x(), ps().h()));
        }
        b_[i] = -(predict_density_[i] - p->density()) / dt() / dt();
    }
    compute_pressure_gradient_acceleration();
}

void IISPH::compute_predict_density()
{
#pragma omp parallel for
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
			// if (std::isnan(predict_density_[i]))
			// {
            //     if (std::isnan(q->vel()[0]) || std::isnan(q->vel()[1]) || std::isnan(q->vel()[2]))
			// 		std::cout << "q->vel nan\n";
            //     //q->acc nan
            //     if (std::isnan(q->acceleration()[0]) || std::isnan(q->acceleration()[1]) ||
            //         std::isnan(q->acceleration()[2]))
            //         std::cout << "q->acc nan\n";
            //     // q->x nan
            //     if (std::isnan(q->x()[0]) || std::isnan(q->x()[1]) || std::isnan(q->x()[2]))
			// 		std::cout << "q->x nan\n";


			// 	std::cout << "density = " << p->density() << std::endl;
			// 	std::cout << "neigh = " << p->neighbors().size() << std::endl;
			// 				int a;
			// 				std::cin >> a;
			// }
        }
	}
}

double IISPH::pressure_solve_iteration()
{
    // (HW Optional)   
    // One step iteration to solve the pressure poisson equation of IISPH
    double err = 0;
#pragma omp parallel for
    for (int i = 0; i < ps().particles().size(); i++) {
		auto& p = ps().particles()[i];
        if (aii_[i] == 0) {
            p->pressure() = 0;
            continue;
        }
        Api_[i] = 0;
        for (auto& q : p->neighbors()) {
            Api_[i] += ps().mass() * (p->acceleration() - q->acceleration()).dot(grad_W(p->x() - q->x(), ps().h()));
        }
        // if(i == 15457 && frame >= 123)std::cout << p->pressure() << " + " << omega() / aii_[i] << " * (" << b_[i] << " - " << Api_[i] << ") =";
        p->pressure() = p->pressure() + omega() / aii_[i] * (b_[i] - Api_[i]);
        // if(i==15457 && frame >= 123)std::cout << p->pressure() << std::endl;
        p->pressure() = std::clamp(p->pressure(), 0.0, 1e5);
        // if (i == 15457 && frame >= 123) {
        //     std::cout << "acc = " << p->acceleration().norm() << std::endl;
        //     std::cout << "density = " << p->density() << std::endl;
        //     std::cout << "density pred = " << predict_density_[i] << std::endl;
        // }
	}
    compute_pressure_gradient_acceleration();
	compute_predict_density();
#pragma omp parallel for
	for (int i = 0; i < ps().particles().size(); i++) {
        auto& p = ps().particles()[i];
        err += abs(predict_density_[i] - p->density());
        // if (std::isnan(p->density()))
        //     std::cout << "density nan\n";
        // if (std::isnan(predict_density_[i]))
        //     std::cout << "predict nan\n";
	}
    err /= ps().particles().size();
       //std::cout << "err = " << err << std::endl;
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