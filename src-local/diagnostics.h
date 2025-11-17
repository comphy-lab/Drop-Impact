/**
 * @file diagnostics.h
 * @brief Statistics and output functions for drop impact simulations
 * @author Vatsal Sanjay (vatsal.sanjay@comphy-lab.org)
 * CoMPhy Lab, Durham University
 *
 * This header provides modular output handling:
 * - File handle management (no repeated open/close)
 * - Statistics calculations (kinetic energy, etc.)
 * - Snapshot management
 * - Extensible for additional diagnostics
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "params.h"

// Global file handles (kept open for performance)
static FILE *log_fp = NULL;

/**
 * @brief Open log files and write headers
 * @param p Parameter structure
 * @return 0 on success, -1 on error
 *
 * Opens log file once and keeps handle open for entire simulation.
 * This fixes the performance bug in original code.
 */
static inline int open_log_files(const struct SimulationParams *p) {
    // Create intermediate directory for snapshots
    char intermediate_dir[512];
    snprintf(intermediate_dir, sizeof(intermediate_dir), "%s/intermediate", p->output_dir);
    create_output_directory(intermediate_dir);

    // Open main log file
    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/log", p->output_dir);

    log_fp = fopen(log_path, "w");
    if (!log_fp) {
        fprintf(stderr, "ERROR: Cannot open log file: %s\n", log_path);
        return -1;
    }

    // Write header with full parameter information
    fprintf(log_fp, "# Drop Impact Simulation Log\n");
    fprintf(log_fp, "# Parameters:\n");
    fprintf(log_fp, "#   We = %g, Ohd = %g, Ohs = %g\n", p->We, p->Ohd, p->Ohs);
    fprintf(log_fp, "#   rho_ratio = %g, Re = %g\n", p->rho_ratio, sqrt(p->We) / p->Ohd);
    fprintf(log_fp, "#   Ldomain = %g, MAXlevel = %d, MINlevel = %d\n",
            p->Ldomain, p->MAXlevel, p->MINlevel);
    fprintf(log_fp, "#   drop_position = (%g, %g), radius = %g\n",
            p->drop_x, p->drop_y, p->drop_radius);
    fprintf(log_fp, "#   impact_velocity = %g, tmax = %g\n", p->impact_velocity, p->tmax);
    fprintf(log_fp, "# Columns: iteration  dt  time  kinetic_energy\n");
    fflush(log_fp);

    fprintf(stderr, "Log file opened: %s\n", log_path);
    return 0;
}

/**
 * @brief Calculate total kinetic energy in the system
 * @return Total kinetic energy (integrated over domain)
 *
 * For axisymmetric flow: KE = ∫ 2πy * 0.5*ρ*(u_x² + u_y²) dV
 */
static inline double calculate_kinetic_energy(void) {
    double ke = 0.0;

    foreach(reduction(+:ke)) {
        // Axisymmetric volume element: 2πy * Δx * Δy
        // Density from VOF field f
        double rho_local = rho(f[]);
        double u_mag_sq = sq(u.x[]) + sq(u.y[]);
        ke += 2.0 * M_PI * y * (0.5 * rho_local * u_mag_sq) * sq(Delta);
    }

    return ke;
}

/**
 * @brief Write statistics to log file
 * @param iter Current iteration number
 * @param time Current simulation time
 * @param timestep Current timestep size
 * @param p Parameter structure
 *
 * Writes: iteration, dt, time, kinetic_energy
 * File handle stays open - no performance penalty
 */
static inline void write_statistics(int iter, double time, double timestep,
                                     const struct SimulationParams *p) {
    if (!log_fp) {
        fprintf(stderr, "ERROR: Log file not open\n");
        return;
    }

    // Calculate statistics
    double ke = calculate_kinetic_energy();

    // Write to log file
    fprintf(log_fp, "%d %g %g %g\n", iter, timestep, time, ke);
    fflush(log_fp);  // Ensure data is written

    // Also write to stderr for monitoring
    if (iter == 0 || iter % 100 == 0) {
        fprintf(stderr, "i=%d  t=%g  dt=%g  KE=%g\n", iter, time, timestep, ke);
    }
}

/**
 * @brief Save simulation snapshot (dump file)
 * @param time Current simulation time
 * @param p Parameter structure
 *
 * Creates dump files for restart and post-processing
 */
static inline void save_snapshot(double time, const struct SimulationParams *p) {
    char filename[512];

    // Save restart file in output directory
    snprintf(filename, sizeof(filename), "%s/restart", p->output_dir);
    dump(file = filename);

    // Save numbered snapshot in intermediate directory
    snprintf(filename, sizeof(filename), "%s/intermediate/snapshot-%5.4f", p->output_dir, time);
    dump(file = filename);

    fprintf(stderr, "Snapshot saved at t = %g\n", time);
}

/**
 * @brief Close all log files
 *
 * Called at end of simulation to properly close files
 */
static inline void close_log_files(void) {
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
        fprintf(stderr, "Log file closed\n");
    }
}

/**
 * @brief Additional diagnostics for future extensibility
 *
 * Placeholder functions for future diagnostic capabilities:
 * - Drop spreading radius
 * - Contact line position
 * - Interface area
 * - Pressure forces
 * - Energy dissipation
 */
#ifdef ENABLE_ADVANCED_DIAGNOSTICS

/**
 * @brief Calculate drop spreading radius
 * @return Maximum radial extent of drop
 */
static inline double calculate_spreading_radius(void) {
    double r_max = 0.0;

    foreach(reduction(max:r_max)) {
        if (f[] > 0.5) {  // Inside drop
            if (x > r_max) r_max = x;
        }
    }

    return r_max;
}

/**
 * @brief Calculate contact line position (y-coordinate where drop meets surface)
 * @return Contact line y-position
 */
static inline double calculate_contact_line_y(void) {
    double y_contact = 0.0;
    // Implementation depends on surface definition
    // Placeholder for future development
    return y_contact;
}

/**
 * @brief Calculate total interface area
 * @return Interface area (2D: length, 3D: area)
 */
static inline double calculate_interface_area(void) {
    double area = 0.0;

    foreach(reduction(+:area)) {
        // Interface area proportional to |∇f|
        double grad_f_mag = sqrt(sq((f[1,0] - f[-1,0])/(2.0*Delta)) +
                                 sq((f[0,1] - f[0,-1])/(2.0*Delta)));
        if (grad_f_mag > 0.1) {  // Near interface
            area += 2.0 * M_PI * y * grad_f_mag * sq(Delta);
        }
    }

    return area;
}

#endif // ENABLE_ADVANCED_DIAGNOSTICS

#endif // DIAGNOSTICS_H
