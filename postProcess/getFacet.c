/* Title: Getting Facets
# Author: Vatsal Sanjay
# vatsal.sanjay@comphy-lab.org
# CoMPhy Lab
# Durham University
# Last updated: Nov 17, 2025
*/

#include "utils.h"
#include "output.h"
#include "fractions.h"

/**
 * Utility for extracting interface facets from Basilisk VOF snapshots using
 * piecewise linear interface reconstruction (PLIC/MYC approximation).
 *
 * Output format (gnuplot-compatible line segments to stderr):
 *   x1 y1
 *   x2 y2
 *   [blank line]
 *   ...
 *
 * Usage: ./getFacet <snapshot-file>
 */

scalar f[];
char filename[80];

int main(int a, char const *arguments[])
{
  if (a != 2) {
    fprintf(stderr, "Error: Expected 1 argument\n");
    fprintf(stderr, "Usage: %s <snapshot-file>\n", arguments[0]);
    return 1;
  }

  sprintf (filename, "%s", arguments[1]);
  restore (file = filename);

  // Boundary: no fluid at left (axis), with proper VOF refinement
  f[left] = dirichlet(0.);
  f.prolongation = fraction_refine;
  f.dirty = true;

  // Output facets (interface segments where 0 < f < 1)
  FILE * fp = ferr;
  output_facets(f, fp);
  fflush (fp);
  fclose (fp);

  return 0;
}
