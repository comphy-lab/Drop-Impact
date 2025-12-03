/* Title: Footprint height extraction
# Author: Vatsal Sanjay
# vatsal.sanjay@comphy-lab.org
# CoMPhy Lab
# Durham University
# Last updated: Nov 17, 2025
*/

#include "utils.h"
#include "output.h"
#include "fractions.h"
#include <stdbool.h>

/**
 * Probe interface heights along the substrate (axis) to report the maximum
 * elevation of the footprint/contact point. The VOF `f` is interpreted through
 * MYC/PLIC reconstruction via `facets`, so the geometry is completely driven by
 * Basilisk's native routines.
 *
 * Output format (CSV piped to `stderr` for gnuplot/pandas):
 *   t,y_max
 *
 * Usage: ./getFootPrint <snapshot-file> <xCutoff>
 *   snapshot-file  Basilisk dump restored via `restore(file=...)`
 *   xCutoff        Upper bound in x for the search window (axisymmetric radius)
 *
 * The code is intentionally single-file but decomposed into helpers so that
 * extending it (different statistics, filtering, etc.) only requires editing
 * small, well-documented functions.
 */

scalar f[];

typedef struct {
  char snapshot[256];
  double x_cutoff;
} footprint_config;

static int parse_arguments(int argc, char const *argv[],
                           footprint_config *cfg);
static void restore_snapshot(const footprint_config *cfg);
static void configure_vof_boundary(void);
static double compute_maximum_interface_height(double x_cutoff);
static inline bool interface_cell(double vof_value);
static inline double segment_midpoint(double cell_center, double delta,
                                      double end0, double end1);
static void emit_footprint(double timestamp, double y_max);

int main(int argc, char const *argv[])
{
  footprint_config cfg;
  if (!parse_arguments(argc, argv, &cfg))
    return 1;

  restore_snapshot(&cfg);
  configure_vof_boundary();

  double y_max = compute_maximum_interface_height(cfg.x_cutoff);
  emit_footprint(t, y_max);
  return 0;
}

static int parse_arguments(int argc, char const *argv[],
                           footprint_config *cfg)
{
  /** Validate CLI input and capture the snapshot filename and search window. */
  if (argc != 3) {
    fprintf(stderr, "Error: Expected 2 arguments\n");
    fprintf(stderr,
            "Usage: %s <snapshot-file> <xCutoff>\n",
            argv[0]);
    return 0;
  }

  snprintf(cfg->snapshot, sizeof(cfg->snapshot), "%s", argv[1]);
  cfg->x_cutoff = atof(argv[2]);

  if (cfg->x_cutoff <= 0.) {
    fprintf(stderr, "Error: xCutoff must be positive.\n");
    return 0;
  }

  return 1;
}

static void restore_snapshot(const footprint_config *cfg)
{
  /** Each invocation handles a single snapshot; restoring is a one-liner. */
  restore (file = cfg->snapshot);
}

static void configure_vof_boundary(void)
{
  /**
   * Boundary: no fluid at the axis (left) with proper VOF prolongation.
   * Keeping it in one routine avoids repeated `f[...]` ceremony.
   */
  f[left] = dirichlet(0.);
  f.prolongation = fraction_refine;
  f.dirty = true;
}

static inline bool interface_cell(double vof_value)
{
  const double eps = 1e-6;
  return (vof_value > eps) && (vof_value < 1. - eps);
}

static inline double segment_midpoint(double cell_center, double delta,
                                      double end0, double end1)
{
  return cell_center + 0.5*delta*(end0 + end1);
}

static double compute_maximum_interface_height(double x_cutoff)
{
  /**
   * Search x<x_cutoff for the highest facet midpoint. `facets` returns up to
   * two points per cell, which we immediately collapse to a midpoint.
   */
  double y_max = 0.;
  face vector s = {{-1}};

  foreach (reduction(max:y_max)) {
    if (x >= x_cutoff || !interface_cell(f[]))
      continue;

    coord n = facet_normal(point, f, s);
    double alpha = plane_alpha(f[], n);
    coord segment[2];

    if (facets(n, alpha, segment) != 2)
      continue;

    double y_mid = segment_midpoint(y, Delta,
                                    segment[0].y, segment[1].y);
    if (y_mid > y_max)
      y_max = y_mid;
  }

  return y_max;
}

static void emit_footprint(double timestamp, double y_max)
{
  /** Stream CSV to stderr; consumer scripts read via pipes. */
  FILE * fp = ferr;
  fprintf(fp, "%g,%g\n", timestamp, y_max);
  fflush (fp);
  fclose (fp);
}
