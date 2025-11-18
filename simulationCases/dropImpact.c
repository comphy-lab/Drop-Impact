/**
# Drop Impact on Solid Surface

Axisymmetric simulation of a liquid drop impacting a solid surface using
the Volume-of-Fluid (VOF) method with adaptive mesh refinement.

## Physical Problem

A spherical drop of radius $R$ impacts a solid surface with velocity $U$.
The simulation captures:

- Two-phase flow (liquid drop in surrounding gas)
- Surface tension effects at the liquid-gas interface
- Viscous dissipation in both phases
- Dynamic contact line motion
- Drop spreading and potential splashing

## Coordinate System

Axisymmetric cylindrical coordinates:
- $x$: radial coordinate (distance from axis of symmetry)
- $y$: axial coordinate (height above surface)
- Azimuthal symmetry assumed (no $\theta$ dependence)

## Governing Equations

Incompressible Navier-Stokes equations for two-phase flow:

$$\nabla \cdot \mathbf{u} = 0$$

$$\rho \left(\frac{\partial \mathbf{u}}{\partial t} + \mathbf{u} \cdot \nabla \mathbf{u}\right) =
-\nabla p + \nabla \cdot (2\mu \mathbf{D}) + \sigma \kappa \mathbf{n} \delta_s$$

where:
- $\mathbf{u}$: velocity field
- $p$: pressure
- $\rho$, $\mu$: density and viscosity (phase-dependent)
- $\mathbf{D}$: deformation tensor
- $\sigma$: surface tension coefficient
- $\kappa$: interface curvature
- $\mathbf{n}$: interface normal
- $\delta_s$: interface delta function

## Dimensionless Numbers

Variables are normalized by drop radius $R$, impact velocity $U$,
liquid density $\rho_\ell$, and surface tension $\sigma$:

- **Weber number**: $We = \frac{\rho_\ell U^2 R}{\sigma}$ (inertia vs surface tension)
- **Ohnesorge number**: $Oh = \frac{\mu}{\sqrt{\rho_\ell \sigma R}}$ (viscous vs surface tension)
- **Reynolds number**: $Re = \frac{\rho_\ell U R}{\mu} = \frac{\sqrt{We}}{Oh}$ (inertia vs viscous)
- **Bond number**: $Bo = \frac{\rho_\ell g R^2}{\sigma}$ (gravity vs surface tension, often negligible)

## Numerical Method

- **Spatial discretization**: Adaptive quadtree grid (Basilisk framework)
- **VOF method**: Interface tracking using volume fraction field
- **Projection method**: Incompressibility constraint enforcement
- **Adaptive refinement**: Error-based mesh adaptation for interface and flow features
- **Time integration**: CFL-limited explicit time stepping

## Usage

### From parameter file:
```bash
qcc -I../src-local -O2 -Wall -disable-dimensions dropImpact.c -o dropImpact -lm
./dropImpact examples/default.params
```

### From command line (legacy mode):
```bash
./dropImpact <MAXlevel> <tmax> <We> <Ohd> <Ohs> <Ldomain> [drop_x] [drop_y] [impact_vel]
```

## References

- Vatsal Sanjay (vatsal.sanjay@comphy-lab.org)
- CoMPhy Lab, Durham University
- Date: 2022-02-08 (original), 2025 (refactored)

## See Also

- Basilisk: https://basilisk.fr
- [Sanjay & Lohse (2025)](https://doi.org/10.1103/PhysRevLett.134.104003) - Unifying theory of scaling in drop impact
- [Josserand & Thoroddsen (2016)](http://dx.doi.org/10.1146/annurev-fluid-122414-034401) - Drop impact review
- [Yarin (2006)](http://dx.doi.org/10.1146/annurev.fluid.38.050304.092144) - Drop impact dynamics

*/

// Basilisk includes
#include "axi.h"                           // Axisymmetric coordinates
#include "navier-stokes/centered.h"        // Centered NS solver
#define FILTERED 1                          // Filtered interface for stability
#include "two-phase.h"                     // Two-phase flow (VOF method)
#include "navier-stokes/conserving.h"     // Momentum conservation
#include "tension.h"                       // Surface tension forces

// Local modular headers
#include "params.h"                        // Parameter management
#include "geometry.h"                      // Geometry and initialization
#include "diagnostics.h"                   // Statistics and output

/**
## Compile-Time Constants

Basilisk's event system requires compile-time constants for time intervals.
These cannot be runtime variables from parameter files.
*/
#define TSNAP (0.01)                       // Snapshot interval (fixed at compile time)

/**
## Geometry Helper Macros

These must be defined here for Basilisk's qcc preprocessor to see them.
*/
#ifndef drop_distance_squared
#define drop_distance_squared(x, y, p) \
    ((x - (p)->drop_x) * (x - (p)->drop_x) + (y - (p)->drop_y) * (y - (p)->drop_y))
#endif

/**
## Global Configuration

All simulation parameters are stored in a single structure for:
- Easy parameter passing
- Clear organization
- Extensibility
*/
struct SimulationParams params;

/**
## Boundary Conditions

Axisymmetric flow requires special treatment at axis and appropriate
outflow conditions:
*/

// Left boundary (axis of symmetry, x=0)
u.t[left] = dirichlet(0.0);   // No tangential velocity (azimuthal component)
f[left] = dirichlet(0.0);     // No fluid crosses axis (ensures symmetry)

// Right boundary (outflow, x=Ldomain)
u.n[right] = neumann(0.);     // Stress-free outflow (du/dn = 0)
p[right] = dirichlet(0.0);    // Zero reference pressure

// Top boundary (outflow, y=Ldomain)
u.n[top] = neumann(0.);       // Stress-free outflow
p[top] = dirichlet(0.0);      // Zero reference pressure

// Bottom boundary (y=0, solid surface)
// Uses default no-slip condition from centered solver

/**
## Main Function

Handles parameter parsing, validation, domain setup, and simulation launch.
*/
int main(int argc, char const *argv[]) {
    /**
    ### Parameter Parsing

    Supports two modes:
    1. Parameter file: `./dropImpact config.params`
    2. Command line: `./dropImpact <MAXlevel> <tmax> <We> <Ohd> <Ohs> <Ldomain> ...`
    */

    if (argc == 2 && access(argv[1], F_OK) == 0) {
        // Parameter file mode
        if (parse_params_from_file(argv[1], &params) != 0) {
            fprintf(stderr, "ERROR: Failed to parse parameter file\n");
            return 1;
        }
    } else if (argc >= 7) {
        // Command line mode (legacy compatibility)
        if (parse_params_from_cli(argc, (char**)argv, &params) != 0) {
            return 1;
        }
    } else {
        // Print usage and exit
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s <params_file>\n", argv[0]);
        fprintf(stderr, "  %s <MAXlevel> <tmax> <We> <Ohd> <Ohs> <Ldomain> [drop_x] [drop_y] [impact_vel]\n", argv[0]);
        fprintf(stderr, "\nParameter file format: key=value (one per line)\n");
        fprintf(stderr, "See examples/default.params for template\n");
        return 1;
    }

    /**
    ### Parameter Validation

    Check physical constraints and consistency before starting simulation.
    */
    if (!validate_params(&params)) {
        fprintf(stderr, "ERROR: Parameter validation failed. Exiting.\n");
        return 1;
    }

    /**
    ### Domain Setup

    Initialize computational domain with specified size and initial grid.
    */
    L0 = params.Ldomain;          // Domain size
    X0 = 0.;                       // Origin at axis
    Y0 = 0.;                       // Origin at surface
    init_grid(1 << params.init_grid_level);  // Initial grid: 2^init_grid_level

    /**
    ### Material Properties

    Set phase properties from dimensionless numbers:
    - Phase 1 (f=1): Drop (liquid, typically water)
    - Phase 2 (f=0): Surrounding (gas, typically air)
    */
    rho1 = 1.0;                            // Drop density (normalized)
    mu1 = params.Ohd / sqrt(params.We);    // Drop viscosity from Oh and We
    rho2 = params.rho_ratio;               // Gas density
    mu2 = params.Ohs / sqrt(params.We);    // Gas viscosity
    f.sigma = 1.0 / params.We;             // Surface tension from We

    /**
    ### Output Setup

    Create output directory and open log files.
    File handles remain open for performance (avoids repeated open/close).
    */
    create_output_directory(params.output_dir);
    open_log_files(&params);

    /**
    ### Print Configuration

    Write complete parameter summary to stderr for verification.
    */
    print_params(&params, stderr);

    /**
    ### Launch Simulation

    Start time-stepping loop (controlled by Basilisk event system).
    */
    run();

    return 0;
}

/**
## Event: Initialization

Sets up initial conditions: drop shape, velocity field, and refined mesh.

Attempts to restore from restart file if available; otherwise initializes
from scratch.
*/
event init(t = 0) {
    char restart_path[512];
    snprintf(restart_path, sizeof(restart_path), "%s/restart", params.output_dir);

    if (!restore(file = restart_path)) {
        // No restart file found - initialize from scratch
        fprintf(stderr, "\nInitializing simulation from initial conditions...\n");

        /**
        ### Grid Refinement

        Refine grid around drop for accurate interface resolution.
        Uses 5% margin beyond drop radius.
        */
        const double refine_margin = 1.05;
        const double refine_r_sq = refine_margin * refine_margin *
                                   params.drop_radius * params.drop_radius;
        refine(drop_distance_squared(x, y, &params) < refine_r_sq &&
               level < params.MAXlevel);
        fprintf(stderr, "Initial grid refinement complete (MAXlevel = %d)\n", params.MAXlevel);

        /**
        ### Drop Initialization

        Initialize VOF field and velocity field.
        */
        fraction(f, params.drop_radius * params.drop_radius -
                    drop_distance_squared(x, y, &params));

        foreach() {
            u.x[] = params.impact_velocity * f[];
            u.y[] = 0.0;
        }

        fprintf(stderr, "Initial drop setup complete:\n");
        fprintf(stderr, "  Drop center: (%g, %g)\n", params.drop_x, params.drop_y);
        fprintf(stderr, "  Drop radius: %g\n", params.drop_radius);
        fprintf(stderr, "  Impact velocity: %g\n", params.impact_velocity);

        fprintf(stderr, "Initialization complete.\n\n");
    } else {
        fprintf(stderr, "\nSimulation restored from restart file.\n\n");
    }
}

/**
## Event: Adaptive Mesh Refinement

Dynamically adapts the mesh based on interface position and flow features.

Refinement criteria:
- VOF field (interface tracking)
- Curvature (surface tension resolution)
- Velocity gradients (flow features)

Un-refinement near outflow boundaries prevents unnecessary resolution in
far-field regions.
*/
event adapt(i++) {
    /**
    ### Curvature Calculation

    Interface curvature needed for surface tension and adaptation criterion.
    */
    scalar KAPPA[];
    curvature(f, KAPPA);

    /**
    ### Wavelet-based Adaptation

    Adapt mesh based on multi-scale wavelet decomposition of fields.
    */
    adapt_wavelet((scalar *){f, KAPPA, u.x, u.y},
                  (double[]){params.fErr, params.KErr, params.VelErr, params.VelErr},
                  params.MAXlevel,
                  params.MINlevel);

    /**
    ### Outflow Region Un-refinement

    Prevent spurious refinement near outflow boundaries where solution
    should be smooth (no drop, low velocities).
    */
    unrefine(x > params.outflow_x_frac * params.Ldomain ||
             y > params.outflow_y_max);
}

/**
## Event: Statistics Output

Calculates and logs flow statistics at regular intervals.

Avoids file I/O performance penalty by:
- Keeping log file open throughout simulation
- Writing buffered output
- Flushing periodically
*/
event statistics(i++) {
    // Write statistics at specified interval
    if (i % params.log_interval == 0) {
        write_statistics(i, t, dt, &params);
    }
}

/**
## Event: Snapshot Output

Saves complete simulation state for:
- Restart capability
- Post-processing and visualization
- Long-term archival

Note: The time interval (TSNAP) must be a compile-time constant for Basilisk.
The end time (params.tmax) can be a runtime variable.
*/
event snapshots(t = 0; t += TSNAP; t <= params.tmax) {
    save_snapshot(t, &params);
}

/**
## Event: Cleanup

Properly close all files at simulation end.
*/
event cleanup(t = end) {
    close_log_files();
    fprintf(stderr, "\nSimulation completed successfully.\n");
}
