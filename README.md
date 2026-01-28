# Drop Impact Simulations

Computational fluid dynamics simulations for drop impact studies using the Basilisk C framework.

## Basilisk (Required)

First-time install (or reinstall):
```bash
curl -sL https://raw.githubusercontent.com/comphy-lab/basilisk-C/main/reset_install_basilisk-ref-locked.sh | bash -s -- --ref=v2026-01-13 --hard
```

Subsequent runs (reuses existing `basilisk/` if same ref):
```bash
curl -sL https://raw.githubusercontent.com/comphy-lab/basilisk-C/main/reset_install_basilisk-ref-locked.sh | bash -s -- --ref=v2026-01-13
```

> **Note**: Replace `v2026-01-13` with the [latest release tag](https://github.com/comphy-lab/basilisk-C/releases).

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
```

```bash
# Run simulation (serial)
./runSimulation.sh
```

```bash
# Run with MPI (4 cores)
./runSimulation.sh --mpi
```

### Parameter Sweep

```bash
# Configure sweep
vim sweep.params        # Set CASE_START, CASE_END, sweep variables
```

```bash
# Run sweep (serial)
./runParameterSweep.sh
```

```bash
# Run sweep with MPI (4 cores per case)
./runParameterSweep.sh --mpi
```

### Post-processing

```bash
# Run post-processing on one or more cases
./runPostProcess-Ncases.sh 1000 1001
```

```bash
# Show available options
./runPostProcess-Ncases.sh --help
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
│   ├── dropImpact.c      Main simulation case
│   ├── dropImpact_legacy.c  Legacy simulation source
│   └── runSnellius_legacy.sbatch  Legacy batch script
├── runSimulation.sh       Single case runner
├── runParameterSweep.sh   Parameter sweep runner
├── runPostProcess-Ncases.sh Post-process multiple cases
├── runSweepHamilton.sbatch Hamilton batch script
├── runSweepSnellius.sbatch Snellius batch script
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

- **Basilisk Framework**: Install via the ref-locked script above (upstream docs: [basilisk.fr](https://basilisk.fr))
- **MPI** (optional): For parallel execution
  - macOS: `brew install open-mpi`
  - Linux: `sudo apt-get install libopenmpi-dev`

## References

### Key Publications

- **Sanjay, V. & Lohse, D.** (2025). *Unifying theory of scaling in drop impact: forces and maximum spreading diameter*. Physical Review Letters, 134(10), 104003. [DOI](https://doi.org/10.1103/PhysRevLett.134.104003)
  - Comprehensive scaling theory for drop impact forces and maximum spreading

- **Josserand, C. & Thoroddsen, S. T.** (2016). *Drop impact on a solid surface*. Annual Review of Fluid Mechanics, 48, 365-391. [DOI](https://doi.org/10.1146/annurev-fluid-122414-034401)
  - Comprehensive review of drop impact phenomena

- **Yarin, A. L.** (2006). *Drop impact dynamics: Splashing, spreading, receding, bouncing...*. Annual Review of Fluid Mechanics, 38, 159-192. [DOI](https://doi.org/10.1146/annurev.fluid.38.050304.092144)
  - Classical review of drop impact dynamics

## Documentation

This README provides the user-facing overview and usage. Additional
details live in the runnable scripts (`runSimulation.sh`,
`runParameterSweep.sh`) and in the simulation source
(`simulationCases/dropImpact.c`) and headers in `src-local/`.

## License

See [LICENSE](LICENSE) file for details.

## Contributing

Follow the existing code style and keep changes consistent with the
repository's structure and scripts.

## Contact

For questions or collaboration inquiries, please contact the [CoMPhy Lab](https://comphy-lab.org).
