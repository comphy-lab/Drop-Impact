/**
# geometry.h

Geometry helpers and initialization macros for drop impact simulations.

## Provides
- Drop geometry calculations
- Initial grid refinement
- Initial condition setup
- Hooks for custom shapes

## Author
Vatsal Sanjay (vatsal.sanjay@comphy-lab.org)
CoMPhy Lab, Durham University
*/

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "params.h"

/**
### drop_distance_squared()

Squared distance from the drop center in axisymmetric coordinates.

#### Parameters
- `x`: radial coordinate
- `y`: axial coordinate
- `p`: parameter structure containing `drop_x`, `drop_y`

#### Returns
Squared distance from the drop center.

Implemented as a macro for compatibility with Basilisk's `qcc` preprocessor.
*/
#define drop_distance_squared(x, y, p) \
    ((x - (p)->drop_x) * (x - (p)->drop_x) + (y - (p)->drop_y) * (y - (p)->drop_y))

/**
### is_inside_drop()

Return 1 if a point is inside the drop, 0 otherwise.

#### Parameters
- `x`: radial coordinate
- `y`: axial coordinate
- `p`: parameter structure
*/
static inline int is_inside_drop(double x, double y, const struct SimulationParams *p) {
    double r_sq = drop_distance_squared(x, y, p);
    double radius_sq = p->drop_radius * p->drop_radius;
    return (r_sq < radius_sq) ? 1 : 0;
}

/**
## Initialization Macros

These are macros rather than functions because they use Basilisk grid
traversal constructs (`refine`, `fraction`, `foreach`) that must be expanded
in the calling context.
*/

/**
### REFINE_INITIAL_GRID()

Refine the initial grid around the drop interface. The refinement region
extends slightly beyond the drop radius for a smoother interface.

#### Usage
```
REFINE_INITIAL_GRID(params);
```
*/
#define REFINE_INITIAL_GRID(p) do { \
    const double _margin = 1.05; \
    const double _refine_r_sq = _margin * _margin * (p)->drop_radius * (p)->drop_radius; \
    refine(drop_distance_squared(x, y, (p)) < _refine_r_sq && level < (p)->MAXlevel); \
    fprintf(stderr, "Initial grid refinement complete (MAXlevel = %d)\n", (p)->MAXlevel); \
} while(0)

/**
### SETUP_INITIAL_DROP()

Initialize the VOF field and velocity field for the drop:
- `f = 1` inside the drop, `0` outside
- `u.x` set to the impact velocity inside the drop
- `u.y` set to zero everywhere

#### Usage
```
SETUP_INITIAL_DROP(params);
```
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
### Custom Shapes (optional)

Enable `ENABLE_CUSTOM_SHAPES` to supply a custom shape function for the drop.
Potential extensions include ellipsoids, deformed drops, or multiple drops.
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
