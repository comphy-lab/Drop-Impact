# Drop Impact Simulations

Computational fluid dynamics simulations for drop impact studies using the Basilisk C framework.

## Overview

This repository contains axisymmetric two-phase flow simulations with adaptive mesh refinement for studying drop impact phenomena. The simulations use the Volume-of-Fluid (VOF) method to track the interface between the drop and surrounding fluid, with automatic mesh refinement focused on regions of interest.

## Key Features

- **Adaptive Mesh Refinement**: Quad/octree-based grids with automatic refinement at interfaces and high-velocity regions
- **Two-Phase Flow**: Volume-of-Fluid (VOF) method with surface tension modeling
- **Axisymmetric Formulation**: Efficient 2D simulations with cylindrical symmetry
- **Modular Architecture**: Separated parameter management, geometry initialization, and diagnostics
- **HPC Ready**: MPI parallel execution support for large-scale simulations
- **Case-Based Organization**: Automatic folder management with unique case numbers

## Quick Start

### Single Simulation

```bash
# Edit parameters
vim default.params      # Set CaseNo, We, Oh, etc.

# Run simulation (serial)
./runSimulation.sh

# Run with MPI (4 cores)
./runSimulation.sh --mpi
```

### Parameter Sweep

```bash
# Configure sweep
vim sweep.params        # Set CASE_START, CASE_END, sweep variables

# Run sweep (serial)
./runParameterSweep.sh

# Run sweep with MPI (4 cores per case)
./runParameterSweep.sh --mpi
```

## Repository Structure

```
├── src-local/              Modular header files
│   ├── params.h           Parameter structures and parsing
│   ├── geometry.h         Drop geometry and initialization
│   └── diagnostics.h      Statistics and output handling
├── postProcess/           Post-processing tools and visualization
│   ├── getData-generic.c  Field extraction on structured grids
│   ├── getFacet.c        Interface geometry extraction
│   ├── getFootPrint.c    Footprint height analysis
│   ├── getFootPrint.py   Multi-cutoff footprint time-series
│   ├── plotFootPrint.py  Publication-quality footprint plots
│   └── Video-generic.py  Frame-by-frame visualization pipeline
├── simulationCases/       Case-based simulation outputs
│   └── dropImpact.c      Main simulation case
├── runSimulation.sh       Single case runner
├── runParameterSweep.sh   Parameter sweep runner
├── default.params         Single-case configuration
└── sweep.params           Sweep configuration
```

## Key Parameters

- **Weber Number (We)**: Ratio of inertial to surface tension forces
- **Ohnesorge Numbers (Ohd, Ohs)**: Viscous/surface tension ratios for drop and surrounding phases
- **Reynolds Number**: Re = √We/Oh
- **Maximum Refinement Level**: Controls mesh resolution (e.g., level 10 = 1024 cells)
- **Domain Size**: Computational domain size in drop radii

## Requirements

- **Basilisk Framework**: Open-source CFD solver ([basilisk.fr](https://basilisk.fr))
- **MPI** (optional): For parallel execution
  - macOS: `brew install open-mpi`
  - Linux: `sudo apt-get install libopenmpi-dev`

## References

### Key Publications

- **Sanjay, V. & Lohse, D.** (2025). *Unifying theory of scaling in drop impact: forces and maximum spreading diameter*. Physical Review Letters, 134(10), 104003. [DOI](https://doi.org/10.1103/PhysRevLett.134.104003)

- **Josserand, C. & Thoroddsen, S. T.** (2016). *Drop impact on a solid surface*. Annual Review of Fluid Mechanics, 48, 365-391. [DOI](https://doi.org/10.1146/annurev-fluid-122414-034401)

- **Yarin, A. L.** (2006). *Drop impact dynamics: Splashing, spreading, receding, bouncing...*. Annual Review of Fluid Mechanics, 38, 159-192. [DOI](https://doi.org/10.1146/annurev.fluid.38.050304.092144)

## Documentation

Comprehensive documentation is available in [CLAUDE.md](CLAUDE.md), including:
- Coding standards and best practices
- Build and compilation instructions
- Simulation physics and numerical methods
- Parameter descriptions and typical values
- Output file formats and visualization

## License

See [LICENSE](LICENSE) file for details.

## Contributing

This repository follows the CoMPhy Lab coding standards. See [CLAUDE.md](CLAUDE.md) for detailed guidelines on:
- Code style (2-space indentation, 80-character lines)
- Naming conventions (snake_case for variables, camelCase for functions)
- Documentation requirements
- Testing procedures

## Contact

For questions or collaboration inquiries, please contact the [CoMPhy Lab](https://comphy-lab.org).
