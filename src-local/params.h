/**
 * @file params.h
 * @brief Parameter management for drop impact simulations
 * @author Vatsal Sanjay (vatsal.sanjay@comphy-lab.org)
 * CoMPhy Lab, Durham University
 *
 * This header defines configuration structures and parameter handling functions
 * for modular, maintainable simulations.
 */

#ifndef PARAMS_H
#define PARAMS_H

// Minimal includes needed for this header
// Most standard library functions are available through Basilisk includes
#include <ctype.h>      // For isspace()
#include <sys/stat.h>   // For mkdir()
#include <errno.h>      // For errno

/**
 * @struct SimulationParams
 * @brief Complete simulation configuration
 *
 * Consolidates all simulation parameters in one structure for:
 * - Easy parameter passing
 * - Configuration file I/O
 * - Parameter validation
 * - Self-documenting code
 */
struct SimulationParams {
    // Case identification
    int CaseNo;        /**< Case number for folder naming (4-digit: 1000-9999) */

    // Physical parameters (dimensionless numbers)
    double We;          /**< Weber number: ρU²R/σ (inertia vs surface tension) */
    double Ohd;         /**< Ohnesorge number (drop): μ/√(ρσR) */
    double Ohs;         /**< Ohnesorge number (surrounding fluid) */
    double rho_ratio;   /**< Density ratio: ρ_surrounding/ρ_drop */

    // Geometry parameters
    double Ldomain;         /**< Domain size in drop radii */
    double drop_x;          /**< Initial drop center x-position (radii) */
    double drop_y;          /**< Initial drop center y-position (radii) */
    double drop_radius;     /**< Drop radius (normalized, typically 1.0) */
    double impact_velocity; /**< Initial impact velocity (negative = downward) */

    // Numerical parameters (mesh adaptation)
    int MAXlevel;      /**< Maximum refinement level (2^MAXlevel cells) */
    int MINlevel;      /**< Minimum refinement level */
    int init_grid_level; /**< Initial grid level: 2^init_grid_level */
    double fErr;       /**< VOF error tolerance for adaptation */
    double KErr;       /**< Curvature error tolerance */
    double VelErr;     /**< Velocity error tolerance */

    // Time control
    double tmax;       /**< Maximum simulation time */
    double tsnap;      /**< Snapshot interval for dump files */

    // Output configuration
    char output_dir[256];  /**< Output directory path */
    int log_interval;      /**< Write statistics every N iterations */

    // Outflow boundary control
    double outflow_x_frac; /**< Unrefine if x > this fraction of Ldomain */
    double outflow_y_max;  /**< Unrefine if y > this value */
};

/**
 * @brief Default parameter values
 *
 * Provides sensible defaults for typical drop impact simulations
 * (water drop impacting solid surface in air)
 */
static inline void set_default_params(struct SimulationParams *p) {
    // Case identification
    p->CaseNo = 1000;  // Default case number

    // Physical parameters (water-air system)
    p->We = 10.0;
    p->Ohd = 5.0e-3;
    p->Ohs = 1.0e-5;
    p->rho_ratio = 1.0e-3;  // Air/water density ratio

    // Geometry
    p->Ldomain = 8.0;
    p->drop_x = 1.5;
    p->drop_y = 1.0;
    p->drop_radius = 1.0;
    p->impact_velocity = -1.0;  // Unit velocity downward

    // Numerical parameters
    p->MAXlevel = 10;
    p->MINlevel = 4;
    p->init_grid_level = 6;
    p->fErr = 1.0e-3;
    p->KErr = 1.0e-6;
    p->VelErr = 1.0e-2;

    // Time control
    p->tmax = 4.0;
    p->tsnap = 0.01;

    // Output
    strcpy(p->output_dir, "results");
    p->log_interval = 1;

    // Outflow boundaries
    p->outflow_x_frac = 0.95;
    p->outflow_y_max = 4.0;
}

/**
 * @brief Parse parameters from configuration file
 * @param filename Path to parameter file (key=value format)
 * @param p Pointer to parameter structure to populate
 * @return 0 on success, -1 on error
 *
 * File format: key=value (one per line, # for comments)
 */
static inline int parse_params_from_file(const char *filename, struct SimulationParams *p) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot open parameter file: %s\n", filename);
        return -1;
    }

    // Start with defaults
    set_default_params(p);

    char line[512];
    int line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;

        // Skip comments and empty lines
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';

        // Trim whitespace
        char *start = line;
        while (*start && isspace(*start)) start++;
        if (*start == '\0') continue;

        // Parse key=value
        char *eq = strchr(start, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = start;
        char *value = eq + 1;

        // Trim trailing whitespace from key
        char *end = key + strlen(key) - 1;
        while (end > key && isspace(*end)) *end-- = '\0';

        // Trim whitespace from value
        while (*value && isspace(*value)) value++;
        end = value + strlen(value) - 1;
        while (end > value && isspace(*end)) *end-- = '\0';

        // Parse based on key
        if (strcmp(key, "CaseNo") == 0) p->CaseNo = atoi(value);
        else if (strcmp(key, "We") == 0) p->We = atof(value);
        else if (strcmp(key, "Ohd") == 0) p->Ohd = atof(value);
        else if (strcmp(key, "Ohs") == 0) p->Ohs = atof(value);
        else if (strcmp(key, "rho_ratio") == 0) p->rho_ratio = atof(value);
        else if (strcmp(key, "Ldomain") == 0) p->Ldomain = atof(value);
        else if (strcmp(key, "drop_x") == 0) p->drop_x = atof(value);
        else if (strcmp(key, "drop_y") == 0) p->drop_y = atof(value);
        else if (strcmp(key, "drop_radius") == 0) p->drop_radius = atof(value);
        else if (strcmp(key, "impact_velocity") == 0) p->impact_velocity = atof(value);
        else if (strcmp(key, "MAXlevel") == 0) p->MAXlevel = atoi(value);
        else if (strcmp(key, "MINlevel") == 0) p->MINlevel = atoi(value);
        else if (strcmp(key, "init_grid_level") == 0) p->init_grid_level = atoi(value);
        else if (strcmp(key, "fErr") == 0) p->fErr = atof(value);
        else if (strcmp(key, "KErr") == 0) p->KErr = atof(value);
        else if (strcmp(key, "VelErr") == 0) p->VelErr = atof(value);
        else if (strcmp(key, "tmax") == 0) p->tmax = atof(value);
        else if (strcmp(key, "tsnap") == 0) p->tsnap = atof(value);
        else if (strcmp(key, "output_dir") == 0) strncpy(p->output_dir, value, sizeof(p->output_dir)-1);
        else if (strcmp(key, "log_interval") == 0) p->log_interval = atoi(value);
        else if (strcmp(key, "outflow_x_frac") == 0) p->outflow_x_frac = atof(value);
        else if (strcmp(key, "outflow_y_max") == 0) p->outflow_y_max = atof(value);
        else {
            fprintf(stderr, "WARNING: Unknown parameter '%s' at line %d\n", key, line_num);
        }
    }

    fclose(fp);
    return 0;
}

/**
 * @brief Parse parameters from command line arguments (legacy mode)
 * @param argc Argument count
 * @param argv Argument values
 * @param p Pointer to parameter structure to populate
 * @return 0 on success, -1 on error
 *
 * Legacy format: MAXlevel tmax We Ohd Ohs Ldomain [drop_x] [drop_y] [impact_vel]
 */
static inline int parse_params_from_cli(int argc, char **argv, struct SimulationParams *p) {
    // Start with defaults
    set_default_params(p);

    if (argc < 7) {
        fprintf(stderr, "ERROR: Insufficient command line arguments\n");
        fprintf(stderr, "Required: <MAXlevel> <tmax> <We> <Ohd> <Ohs> <Ldomain>\n");
        fprintf(stderr, "Optional: [drop_x] [drop_y] [impact_velocity]\n");
        return -1;
    }

    // Required parameters
    p->MAXlevel = atoi(argv[1]);
    p->tmax = atof(argv[2]);
    p->We = atof(argv[3]);
    p->Ohd = atof(argv[4]);
    p->Ohs = atof(argv[5]);
    p->Ldomain = atof(argv[6]);

    // Optional parameters
    if (argc > 7) p->drop_x = atof(argv[7]);
    if (argc > 8) p->drop_y = atof(argv[8]);
    if (argc > 9) p->impact_velocity = atof(argv[9]);

    return 0;
}

/**
 * @brief Validate parameter values
 * @param p Pointer to parameter structure
 * @return 1 if valid, 0 if invalid
 *
 * Checks physical constraints and consistency
 */
static inline int validate_params(const struct SimulationParams *p) {
    int valid = 1;

    // Case number must be in valid range
    if (p->CaseNo < 1000 || p->CaseNo > 9999) {
        fprintf(stderr, "ERROR: CaseNo must be 4-digit (1000-9999), got %d\n", p->CaseNo);
        valid = 0;
    }

    // Physical parameters must be positive
    if (p->We <= 0) {
        fprintf(stderr, "ERROR: Weber number must be positive (We = %g)\n", p->We);
        valid = 0;
    }
    if (p->Ohd <= 0) {
        fprintf(stderr, "ERROR: Ohnesorge (drop) must be positive (Ohd = %g)\n", p->Ohd);
        valid = 0;
    }
    if (p->Ohs <= 0) {
        fprintf(stderr, "ERROR: Ohnesorge (surrounding) must be positive (Ohs = %g)\n", p->Ohs);
        valid = 0;
    }
    if (p->rho_ratio <= 0) {
        fprintf(stderr, "ERROR: Density ratio must be positive (rho_ratio = %g)\n", p->rho_ratio);
        valid = 0;
    }

    // Geometry constraints
    if (p->Ldomain <= 2.0 * p->drop_radius) {
        fprintf(stderr, "ERROR: Domain too small (Ldomain = %g, need > 2*drop_radius)\n", p->Ldomain);
        valid = 0;
    }
    if (p->drop_radius <= 0) {
        fprintf(stderr, "ERROR: Drop radius must be positive (drop_radius = %g)\n", p->drop_radius);
        valid = 0;
    }

    // Numerical parameters
    if (p->MAXlevel < p->MINlevel) {
        fprintf(stderr, "ERROR: MAXlevel (%d) must be >= MINlevel (%d)\n", p->MAXlevel, p->MINlevel);
        valid = 0;
    }
    if (p->MAXlevel > 15) {
        fprintf(stderr, "WARNING: Very high MAXlevel (%d) may cause memory issues\n", p->MAXlevel);
    }
    if (p->MINlevel < 2) {
        fprintf(stderr, "ERROR: MINlevel (%d) must be >= 2\n", p->MINlevel);
        valid = 0;
    }

    // Error tolerances
    if (p->fErr <= 0 || p->KErr <= 0 || p->VelErr <= 0) {
        fprintf(stderr, "ERROR: Error tolerances must be positive\n");
        valid = 0;
    }

    // Time parameters
    if (p->tmax <= 0) {
        fprintf(stderr, "ERROR: tmax must be positive (tmax = %g)\n", p->tmax);
        valid = 0;
    }
    if (p->tsnap <= 0 || p->tsnap > p->tmax) {
        fprintf(stderr, "ERROR: Invalid tsnap (tsnap = %g, tmax = %g)\n", p->tsnap, p->tmax);
        valid = 0;
    }

    return valid;
}

/**
 * @brief Print parameter summary
 * @param p Pointer to parameter structure
 * @param fp File pointer for output (e.g., stderr or log file)
 *
 * Prints formatted parameter summary for logging and verification
 */
static inline void print_params(const struct SimulationParams *p, FILE *fp) {
    fprintf(fp, "\n");
    fprintf(fp, "========================================\n");
    fprintf(fp, "Drop Impact Simulation Configuration\n");
    fprintf(fp, "========================================\n");
    fprintf(fp, "\n");
    fprintf(fp, "Case Identification:\n");
    fprintf(fp, "  Case Number:              %04d\n", p->CaseNo);
    fprintf(fp, "\n");
    fprintf(fp, "Physical Parameters:\n");
    fprintf(fp, "  Weber number (We):        %g\n", p->We);
    fprintf(fp, "  Ohnesorge (drop):         %g\n", p->Ohd);
    fprintf(fp, "  Ohnesorge (surround):     %g\n", p->Ohs);
    fprintf(fp, "  Density ratio:            %g\n", p->rho_ratio);
    fprintf(fp, "  Reynolds (drop):          %g\n", sqrt(p->We) / p->Ohd);
    fprintf(fp, "\n");
    fprintf(fp, "Geometry:\n");
    fprintf(fp, "  Domain size (Ldomain):    %g\n", p->Ldomain);
    fprintf(fp, "  Drop position (x, y):     (%g, %g)\n", p->drop_x, p->drop_y);
    fprintf(fp, "  Drop radius:              %g\n", p->drop_radius);
    fprintf(fp, "  Impact velocity:          %g\n", p->impact_velocity);
    fprintf(fp, "\n");
    fprintf(fp, "Numerical Settings:\n");
    fprintf(fp, "  Grid levels (min/max):    %d / %d\n", p->MINlevel, p->MAXlevel);
    fprintf(fp, "  Initial grid level:       %d (2^%d = %d cells)\n",
            p->init_grid_level, p->init_grid_level, 1 << p->init_grid_level);
    fprintf(fp, "  Error tolerances:\n");
    fprintf(fp, "    VOF (fErr):             %g\n", p->fErr);
    fprintf(fp, "    Curvature (KErr):       %g\n", p->KErr);
    fprintf(fp, "    Velocity (VelErr):      %g\n", p->VelErr);
    fprintf(fp, "\n");
    fprintf(fp, "Time Control:\n");
    fprintf(fp, "  Maximum time (tmax):      %g\n", p->tmax);
    fprintf(fp, "  Snapshot interval:        %g\n", p->tsnap);
    fprintf(fp, "\n");
    fprintf(fp, "Output:\n");
    fprintf(fp, "  Output directory:         %s\n", p->output_dir);
    fprintf(fp, "  Log interval:             %d iterations\n", p->log_interval);
    fprintf(fp, "\n");
    fprintf(fp, "========================================\n");
    fprintf(fp, "\n");
    fflush(fp);
}

/**
 * @brief Create output directory if it doesn't exist
 * @param dirname Directory path
 * @return 0 on success, -1 on error
 */
static inline int create_output_directory(const char *dirname) {
    struct stat st = {0};

    if (stat(dirname, &st) == -1) {
        if (mkdir(dirname, 0755) == -1) {
            fprintf(stderr, "WARNING: Could not create directory '%s': %s\n",
                    dirname, strerror(errno));
            return -1;
        }
        fprintf(stderr, "Created output directory: %s\n", dirname);
    }
    return 0;
}

#endif // PARAMS_H
