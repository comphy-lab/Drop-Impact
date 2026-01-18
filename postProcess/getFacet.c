/**
# getFacet

Extract interface facets from Basilisk VOF snapshots using PLIC/MYC
reconstruction and stream line segments to stderr.

## Output
Each facet is written as gnuplot-compatible line segments:
```
x1 y1
x2 y2

```

## Usage
```
./getFacet <snapshot-file>
```

## Author
Vatsal Sanjay (vatsal.sanjay@comphy-lab.org)
CoMPhy Lab, Durham University
Last updated: Nov 17, 2025
*/

#include "utils.h"
#include "output.h"
#include "fractions.h"

scalar f[];
char filename[4096];

int main(int a, char const *arguments[])
{
  if (a != 2) {
    fprintf(stderr, "Error: Expected 1 argument\n");
    fprintf(stderr, "Usage: %s <snapshot-file>\n", arguments[0]);
    return 1;
  }

  snprintf(filename, sizeof(filename), "%s", arguments[1]);
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
