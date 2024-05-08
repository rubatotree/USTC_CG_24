#include "wcsph.h"
#include <iostream>
using namespace Eigen;

namespace USTC_CG::node_sph_fluid {

WCSPH::WCSPH(const MatrixXd& X, const Vector3d& box_min, const Vector3d& box_max)
    : SPHBase(X, box_min, box_max)
{
}

void WCSPH::compute_density()
{
	// -------------------------------------------------------------
	// (HW TODO) Implement the density computation
    // You can also compute pressure in this function 
	// -------------------------------------------------------------
    for (auto& p : ps_.particles()) {
        double rho = ps_.mass() * W_zero(ps_.h());
        for (auto& q : p->neighbors()) {
            rho += ps_.mass() * W(p->x() - q->x(), ps_.h());
        }
        p->density() = rho;
        p->pressure() = stiffness_ * (pow(p->density() / ps_.density0(), exponent_) - 1);
        p->pressure() = std::max(0.0, p->pressure());
    }
	
}

void WCSPH::step()
{
    TIC(step)
    // -------------------------------------------------------------
    // (HW TODO) Follow the instruction in documents and PPT,
    // implement the pipeline of fluid simulation
    // -------------------------------------------------------------

	// Search neighbors, compute density, advect, solve pressure acceleration, etc.
	ps_.assign_particles_to_cells();
	ps_.search_neighbors();

	compute_density();
	compute_non_pressure_acceleration();
	for (auto& p : ps_.particles()) {
		p->vel() = p->vel() + dt() * p->acceleration();
	}
	compute_pressure_gradient_acceleration();
	advect();
    TOC(step)
}
}  // namespace USTC_CG::node_sph_fluid