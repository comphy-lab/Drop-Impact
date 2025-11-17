/**
 * @file geometry.h
 * @brief Geometry and initialization functions for drop impact simulations
 * @author Vatsal Sanjay (vatsal.sanjay@comphy-lab.org)
 * CoMPhy Lab, Durham University
 *
 * This header provides modular functions for:
 * - Drop geometry calculations
 * - Initial grid refinement
 * - Initial condition setup
 * - Extensible for complex geometries
 */

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "params.h"

/**
 * @brief Calculate squared distance from drop center
 * @param x Radial coordinate
 * @param y Axial coordinate
 * @param p Parameter structure containing drop position
 * @return Squared distance from drop center
 *
 * For axisymmetric coordinates, distance from drop center at (drop_x, drop_y)
 *
 * Implemented as macro for compatibility with Basilisk's qcc preprocessor.
 */
#define drop_distance_squared(x, y, p) \
    ((x - (p)->drop_x) * (x - (p)->drop_x) + (y - (p)->drop_y) * (y - (p)->drop_y))

/**
 * @brief Check if point is inside drop
 * @param x Radial coordinate
 * @param y Axial coordinate
 * @param p Parameter structure
 * @return 1 if inside drop, 0 otherwise
 */
static inline int is_inside_drop(double x, double y, const struct SimulationParams *p) {
    double r_sq = drop_distance_squared(x, y, p);
    double radius_sq = p->drop_radius * p->drop_radius;
    return (r_sq < radius_sq) ? 1 : 0;
}

/**
 * Macros for initialization (must be macros to work with Basilisk grid context)
 *
 * These are defined as macros rather than functions because they use Basilisk's
 * grid traversal macros (refine, fraction, foreach) which must be expanded
 * in the correct context.
 */

/**
 * @brief Refine initial grid around drop
 *
 * Creates adaptive initial grid with fine resolution near drop interface.
 * Refinement region extends slightly beyond drop radius for smoother interface.
 *
 * Usage: REFINE_INITIAL_GRID(¶ms);
 */
#define REFINE_INITIAL_GRID(p) do { \
    const double _margin = 1.05; \
    const double _refine_r_sq = _margin * _margin * (p)->drop_radius * (p)->drop_radius; \
    refine(drop_distance_squared(x, y, (p)) < _refine_r_sq && level < (p)->MAXlevel); \
    fprintf(stderr, "Initial grid refinement complete (MAXlevel = %d)\n", (p)->MAXlevel); \
} while(0)

/**
 * @brief Setup initial drop shape and velocity field
 *
 * Initializes:
 * - VOF field f (1 inside drop, 0 outside)
 * - Velocity field (impact velocity inside drop, zero outside)
 *
 * Extensible for:
 * - Different drop shapes (ellipsoids, etc.)
 * - Variable velocity profiles
 * - Multiple drops
 *
 * Usage: SETUP_INITIAL_DROP(¶ms);
 */
#define SETUP_INITIAL_DROP(p) do { \
    fraction(f, (p)->drop_radius * (p)->drop_radius - drop_distance_squared(x, y, (p))); \
    foreach() { \
        u.x[] = (p)->impact_velocity * f[]; \
        u.y[] = 0.0; \
    } \
    fprintf(stderr, "Initial drop setup complete:\n"); \
    fprintf(stderr, "  Drop center: (%g, %g)\n", (p)->drop_x, (p)->drop_y); \
    fprintf(stderr, "  Drop radius: %g\n", (p)->drop_radius); \
    fprintf(stderr, "  Impact velocity: %g\n", (p)->impact_velocity); \
} while(0)

/**
 * @brief Alternative: Setup drop with custom shape function
 * @param shape_func Function pointer defining drop shape
 * @param p Parameter structure
 *
 * Example for future extensibility:
 * - Ellipsoidal drops
 * - Deformed drops
 * - Multiple drops
 */
#ifdef ENABLE_CUSTOM_SHAPES
typedef double (*ShapeFunction)(double x, double y, const struct SimulationParams *p);

static inline void setup_custom_drop(ShapeFunction shape_func, const struct SimulationParams *p) {
    fraction(f, shape_func(x, y, p));

    foreach() {
        u.x[] = p->impact_velocity * f[];
        u.y[] = 0.0;
    }
}
#endif

#endif // GEOMETRY_H
