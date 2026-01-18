/**
# getData-generic

Sample Basilisk snapshot fields onto a uniform grid and stream columns to
stderr for post-processing (gnuplot, pandas, etc.).

## Geometry
Set `AXI=1` for axisymmetric (default) or `AXI=0` for 2D Cartesian:
```
qcc -DAXI=0 ...
```

## Usage
```
./getData-generic <filename> <xmin> <ymin> <xmax> <ymax> <ny>
```

## Output
Each row is: `x y <field_0> <field_1> ...` written to stderr.

## Author
Vatsal Sanjay (vatsal.sanjay@comphy-lab.org)
CoMPhy Lab, Durham University
Last updated: Nov 17, 2025
*/

#include "utils.h"
#include "output.h"

/**
Geometry configuration: Set `AXI=1` for axisymmetric, `AXI=0` for 2D Cartesian.
- Axisymmetric: x=radial, y=axial (includes azimuthal D22 term)
- 2D Cartesian: x=x-coordinate, y=y-coordinate (no D22 term)

To change geometry:
1. Edit the definition below and set `AXI` to 0, or
2. Compile with `qcc -DAXI=0 ...` to build a 2D executable.
*/
#ifndef AXI
#define AXI 1  // Default to axisymmetric
#endif

scalar f[];
vector u[];

/**
Lightweight utility for extracting Basilisk snapshot data on a structured
Cartesian sampling grid. The workflow is intentionally linear:
1. Parse CLI bounds/grid spacing into `extraction_config`.
2. Restore the snapshot (`restore(file=...)`).
3. Register each derived scalar in `field_list`.
4. Compute fields and interpolate them onto a regular grid.
5. Stream `x y <fields...>` rows to stderr.

To add a new derived quantity (e.g., `Aij`):
1. Declare scalar: `scalar Aij[];`
2. Register in `register_fields()`: `field_list = list_add(field_list, Aij);`
3. Compute in `compute_fields()`: `compute_Aij_field(Aij);`
4. Implement: `static void compute_Aij_field(scalar target) { ... }`
*/
typedef struct {
  char filename[4096];
  double xmin, ymin, xmax, ymax;
  double Deltax, Deltay;
  int nx, ny;
} extraction_config;

scalar D2c[], vel[];
scalar * field_list = NULL;

static int parse_arguments(int argc, char const *argv[],
                           extraction_config *cfg);
static int configure_grid(extraction_config *cfg);
static void register_fields(void);
static void compute_fields(void);
static double ** allocate_field_buffer(const extraction_config *cfg,
                                       int field_count);
static void sample_fields(const extraction_config *cfg, double **field_buffer,
                          int field_count);
static void write_fields(const extraction_config *cfg, double **field_buffer,
                         int field_count, FILE *fp);
static void cleanup_output(FILE *fp, double **field_buffer);
static void compute_D2c_field(scalar target);
static void compute_velocity_field(scalar target);

/**
### main()

Validate CLI arguments, configure the sampling grid, restore the snapshot,
compute requested fields, and emit the sampled data to stderr.

#### Arguments
- `arguments[1]`: snapshot filename
- `arguments[2..5]`: `xmin`, `ymin`, `xmax`, `ymax`
- `arguments[6]`: `ny` (number of cells in y)

#### Returns
- `0` on success
- `1` on invalid arguments or grid configuration
*/
int main(int a, char const *arguments[])
{
  extraction_config cfg;
  if (!parse_arguments(a, arguments, &cfg))
    return 1;

  if (!configure_grid(&cfg))
    return 1;

  register_fields();
  restore (file = cfg.filename);
  compute_fields();

  int registered_fields = list_len(field_list);
  double ** field =
    allocate_field_buffer(&cfg, registered_fields);
  sample_fields(&cfg, field, registered_fields);

  FILE * fp = ferr;
  write_fields(&cfg, field, registered_fields, fp);
  cleanup_output(fp, field);
}

static int parse_arguments(int argc, char const *argv[],
                           extraction_config *cfg)
{
  /** Read CLI arguments and guard against invalid bounds/grid sizes. */
  if (argc != 7) {
    fprintf(stderr, "Error: Expected 6 arguments\n");
    fprintf(stderr,
            "Usage: %s <filename> <xmin> <ymin> "
            "<xmax> <ymax> <ny>\n", argv[0]);
    return 0;
  }

  snprintf(cfg->filename, sizeof(cfg->filename), "%s", argv[1]);
  cfg->xmin = atof(argv[2]);
  cfg->ymin = atof(argv[3]);
  cfg->xmax = atof(argv[4]);
  cfg->ymax = atof(argv[5]);
  cfg->ny = atoi(argv[6]);

  if (cfg->ny <= 0) {
    fprintf(stderr, "Error: ny must be positive.\n");
    return 0;
  }

  if (cfg->xmax <= cfg->xmin || cfg->ymax <= cfg->ymin) {
    fprintf(stderr, "Error: Bounds must satisfy xmax>xmin "
                    "and ymax>ymin.\n");
    return 0;
  }

  return 1;
}

static int configure_grid(extraction_config *cfg)
{
  /** Translate bounds and ny into nx, Δx, Δy for regular sampling. */
  cfg->Deltay = (cfg->ymax - cfg->ymin)/((double) cfg->ny);
  cfg->nx = (int) ((cfg->xmax - cfg->xmin)/cfg->Deltay);

  if (cfg->nx <= 0) {
    fprintf(stderr, "Error: Computed nx <= 0. "
                    "Check the provided bounds.\n");
    return 0;
  }

  cfg->Deltax = (cfg->xmax - cfg->xmin)/((double) cfg->nx);
  return 1;
}

static void register_fields(void)
{
  /**
  Populate Basilisk list with each scalar field.
  To add a new field, declare the scalar at the top and add it here.
  */
  field_list = list_add(field_list, D2c);
  field_list = list_add(field_list, vel);
}

static void compute_fields(void)
{
  /**
  Dispatch compute callbacks for each field.
  To add a new field, add a compute call here.
  */
  compute_D2c_field(D2c);
  compute_velocity_field(vel);
}

static double ** allocate_field_buffer(const extraction_config *cfg,
                                       int registered_fields)
{
  return (double **) matrix_new (cfg->nx, cfg->ny + 1,
                                 registered_fields*sizeof(double));
}

static void sample_fields(const extraction_config *cfg, double **field_buffer,
                          int registered_fields)
{
  /**
  Interpolate every registered scalar on the regular grid that we later dump.
  The matrix layout follows Basilisk's `matrix_new`: row-major on i (x),
  with contiguous blocks of `registered_fields` entries per (i, j).
  */
  for (int i = 0; i < cfg->nx; i++) {
    double x = cfg->Deltax*(i + 1./2) + cfg->xmin;
    for (int j = 0; j < cfg->ny; j++) {
      double y = cfg->Deltay*(j + 1./2) + cfg->ymin;
      int k = 0;
      for (scalar s in field_list)
        field_buffer[i][registered_fields*j + k++] =
          interpolate (s, x, y);
    }
  }
}

static void write_fields(const extraction_config *cfg, double **field_buffer,
                         int registered_fields, FILE *fp)
{
  /** Stream rows in the format: x y field0 field1 ... */
  for (int i = 0; i < cfg->nx; i++) {
    double x = cfg->Deltax*(i + 1./2) + cfg->xmin;
    for (int j = 0; j < cfg->ny; j++) {
      double y = cfg->Deltay*(j + 1./2) + cfg->ymin;
      fprintf (fp, "%g %g", x, y);
      int k = 0;
      for (scalar s in field_list)
        fprintf (fp, " %g",
                 field_buffer[i][registered_fields*j + k++]);
      fputc ('\n', fp);
    }
  }
}

static void cleanup_output(FILE *fp, double **field_buffer)
{
  fflush (fp);
  fclose (fp);
  matrix_free (field_buffer);
}

/**
### compute_D2c_field()

Compute `log10(f * D^2)` where `D^2` is the second invariant of the strain
rate tensor.

Geometry-dependent formulation:

Axisymmetric (AXI=1, x=radial, y=axial):
- `D11 = ∂u_y/∂y` (axial velocity gradient)
- `D22 = u_y/y` (azimuthal component)
- `D33 = ∂u_x/∂x` (radial velocity gradient)
- `D13 = (∂u_y/∂x + ∂u_x/∂y)/2` (shear)
- `D^2 = D11^2 + D22^2 + D33^2 + 2 D13^2`

2D Cartesian (AXI=0):
- `D11 = ∂u_y/∂y`
- `D33 = ∂u_x/∂x`
- `D13 = (∂u_y/∂x + ∂u_x/∂y)/2`
- `D^2 = D11^2 + D33^2 + 2 D13^2`

Multiplying by `f[]` restricts the calculation to the drop phase.
Returns `log10(f * D^2)` for positive values, and `-10` otherwise (floor for
visualization).
*/
static void compute_D2c_field(scalar target)
{
  foreach() {
    double D11 = (u.y[0,1] - u.y[0,-1])/(2*Delta);
#if AXI
    double D22 = (y > 1e-10) ? u.y[]/y : 0.0;  // Epsilon guard for axis
#endif
    double D33 = (u.x[1,0] - u.x[-1,0])/(2*Delta);
    double D13 =
      0.5*((u.y[1,0] - u.y[-1,0] + u.x[0,1] - u.x[0,-1])/(2*Delta));
#if AXI
    double D2 = sq(D11) + sq(D22) + sq(D33) + 2.*sq(D13);
#else
    double D2 = sq(D11) + sq(D33) + 2.*sq(D13);
#endif
    target[] = f[]*D2;
    if (target[] > 0.)
      target[] = log(target[])/log(10);
    else
      target[] = -10;
  }
}

/**
### compute_velocity_field()

Compute velocity magnitude `|u| = sqrt(u_x^2 + u_y^2)`.

Geometry-independent interpretation:
- Axisymmetric (AXI=1): `u.x` radial, `u.y` axial
- 2D Cartesian (AXI=0): `u.x` x-component, `u.y` y-component
*/
static void compute_velocity_field(scalar target)
{
  foreach()
    target[] = sqrt(sq(u.x[]) + sq(u.y[]));
}
