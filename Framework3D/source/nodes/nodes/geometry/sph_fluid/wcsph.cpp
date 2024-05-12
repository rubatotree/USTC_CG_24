#include "wcsph.h"
#include <iostream>
using namespace Eigen;

namespace USTC_CG::node_sph_fluid {

WCSPH::WCSPH(const MatrixXd& X, const Vector3d& box_min, const Vector3d& box_max)
    : SPHBase(X, box_min, box_max)
{
    omp_set_num_threads(32);
}

void WCSPH::compute_density()
{
	// -------------------------------------------------------------
	// (HW TODO) Implement the density computation
    // You can also compute pressure in this function 
	// -------------------------------------------------------------
    int sz = ps().particles().size();
#pragma omp parallel for
    for (int i = 0; i < sz; i++) {
        auto& p = ps().particles()[i];
		double rho = ps().mass() * W_zero(ps().h());
        for (auto& q : p->neighbors()) {
            rho += ps().mass() * W(p->x() - q->x(), ps().h());
        }
        p->density() = rho;
        p->pressure() = stiffness() * (pow(p->density() / ps().density0(), exponent()) - 1);
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
    TIC(cell)
	ps().assign_particles_to_cells();
    TOC(cell)
	TIC(neighbor)
	ps().search_neighbors();
	TOC(neighbor)
	TIC(density)
	compute_density();
	TOC(density)
	TIC(non_pressure)
	compute_non_pressure_acceleration();
	TOC(non_pressure)
	TIC(updv)
    int sz = ps().particles().size();
#pragma omp parallel for
    for (int i = 0; i < sz; i++) {
        auto& p = ps().particles()[i];
		p->vel() = p->vel() + dt() * p->acceleration();
	}
    TOC(updv)
	TIC(pga)
	compute_pressure_gradient_acceleration();
	TOC(pga)
	TIC(advect)
	advect();
	TOC(advect)
    TOC(step)
}
}  // namespace USTC_CG::node_sph_fluid