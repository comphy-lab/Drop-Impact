# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This repository contains computational fluid dynamics simulations for drop impact studies using the Basilisk C framework. The project focuses on axisymmetric two-phase flow simulations with adaptive mesh refinement.

**Recent Update (2025)**: The codebase has been refactored for improved maintainability, modularity, and HPC compatibility. See [Modular Structure](#modular-structure) below.

## Purpose

This document outlines the coding standards, project structure, and best practices for computational fluid dynamics simulations using the Basilisk framework. Following these standards ensures code readability, maintainability, and reproducibility across the CoMPhy Lab's research projects.

## Modular Structure (2025 Restructuring)

The code has been completely reorganized with case-based folder management and root-level execution:

### Directory Structure

```
Drop-Impact/
├── .project_config          # Basilisk environment setup
├── CLAUDE.md               # This file
├── basilisk/               # Basilisk framework (submodule/copy)
│
├── src-local/              # Modular header files
│   ├── params.h           # Parameter structures and parsing with CaseNo
│   ├── geometry.h         # Drop geometry and initialization
│   ├── diagnostics.h      # Statistics and output handling
│   └── parse_params.sh    # Shell parameter parsing library
│
├── runSimulation.sh        # Single case runner (from root)
├── runParameterSweep.sh    # Parameter sweep runner (from root)
├── default.params          # Single-case configuration (edit this)
├── sweep.params            # Sweep configuration (edit this)
│
└── SimulationCases/
    ├── dropImpact.c       # Main simulation (refactored with markdown docs)
    ├── dropImpact_legacy.c # Original version (archived)
    └── 1000/              # Case folders created by scripts
        ├── dropImpact     # Compiled executable
        ├── dropImpact.c   # Source copy
        ├── case.params    # Parameter copy
        ├── log            # Time series data
        ├── restart        # Restart checkpoint
        └── intermediate/  # Snapshot files
```

### Key Improvements

1. **Case-Based Organization** (`CaseNo` system):
   - Each simulation runs in `SimulationCases/<CaseNo>/` folder
   - CaseNo is a 4-digit number (1000-9999) set in parameter files
   - Auto-incrementing CaseNo for parameter sweeps
   - Restart-aware (continues from checkpoint if folder exists)

2. **Root-Level Execution**:
   - All scripts run from project root
   - Edit `default.params` or `sweep.params` at root
   - Output folders automatically created in `SimulationCases/`

3. **Parameter Management** (`src-local/params.h`):
   - Added `CaseNo` field for folder naming
   - Centralized configuration structure
   - Parameter file support (key=value format)
   - Input validation with clear error messages
   - Command-line and file-based modes

4. **Modular Code** (`src-local/geometry.h`, `diagnostics.h`):
   - Separated concerns (geometry, statistics, I/O)
   - Reusable components
   - Fixed file I/O performance bug (no repeated open/close)
   - Extensible for additional features

5. **Shell-Based Workflows**:
   - Pure shell scripting (no Python dependencies)
   - Parameter sweep support (sequential execution)
   - Comprehensive error handling

6. **Documentation**:
   - Literate programming style (markdown in code)
   - Self-documenting parameter files
   - Complete documentation in CLAUDE.md
   - Correct Reynolds number formula: Re = √We/Oh

### Quick Start

```bash
# From project root directory

# Single simulation
vim default.params      # Set CaseNo=1000, We, Oh, etc.
./runSimulation.sh

# Parameter sweep
vim sweep.params        # Set CASE_START, CASE_END, sweep variables
./runParameterSweep.sh
```

## Code Style

- **Indentation**: 2 spaces (no tabs).
- **Line Length**: Maximum 80 characters per line.
- **Comments**: Use markdown in comments starting with `/**`; avoid bare `*` in comments.
- **Spacing**: Include spaces after commas and around operators (`+`, `-`).
- **File Organization**:
  - Place core functionality in `.h` headers
  - Implement tests in `.c` files
- **Naming Conventions**:
  - Use `snake_case` for variables and parameters
  - Use `camelCase` for functions and methods
- **Error Handling**: Return meaningful values and provide descriptive `stderr` messages.

## Build & Test Commands

**Standard Compilation**:

```bash
qcc -autolink file.c -o executable -lm
```

**Compilation with Custom Headers**:

```bash
qcc -I$PWD/src-local -autolink file.c -o executable -lm
```

## Best Practices

- Keep simulations modular and reusable
- Document physical assumptions and numerical methods
- Perform mesh refinement studies to ensure solution convergence
- Include visualization scripts in the postProcess directory

## Basilisk Framework

Basilisk is a computational fluid dynamics solver that uses:
- **qcc**: A custom C preprocessor/compiler that extends C with grid abstractions
- **Adaptive mesh refinement**: Quad/octree-based grids with automatic refinement
- **Two-phase flow**: Volume-of-Fluid (VOF) method with surface tension
- **Navier-Stokes**: Centered formulation for incompressible flows

Key framework files are located in `basilisk/src/`.

## Project Configuration

The `.project_config` file sets up the Basilisk environment:
```bash
export BASILISK=/path/to/basilisk/src
export PATH=$PATH:$BASILISK
```

This must be sourced before running simulations to access the `qcc` compiler.

## Building and Running Simulations

### Workflow

Run simulations from the project root directory:

```bash
# Edit parameter file
vim default.params          # Set CaseNo, We, Oh, etc.

# Run single simulation
./runSimulation.sh

# Compile only (check for errors)
./runSimulation.sh --compile-only

# Debug mode
./runSimulation.sh --debug

# Parameter sweep
vim sweep.params           # Set CASE_START, CASE_END, sweep variables
./runParameterSweep.sh
```

### Legacy Command-Line Mode (Still Supported)

For backward compatibility, you can still pass parameters directly:

1. **Run using old script**:
   ```bash
   ./runCases.sh dropImpact
   ```

2. **Command-line arguments** (legacy mode):
   ```bash
   ./runSimulation.sh 10 4.0 10.0 1e-3 1e-5 8.0
   ```

### Compilation Details

Simulations are compiled using:
```bash
qcc -I${ORIG_DIR}/src-local -I${ORIG_DIR}/../src-local -O2 -Wall -disable-dimensions <file>.c -o <executable> -lm
```

Key flags:
- `-I${ORIG_DIR}/src-local`: Include local header directory (if it exists)
- `-O2`: Optimization level 2
- `-Wall`: All warnings
- `-disable-dimensions`: Disable dimensional analysis (Basilisk feature)
- `-lm`: Link math library

### Simulation Parameters

The `dropImpact.c` simulation requires 6 command-line arguments:
```bash
./dropImpact <MAXlevel> <tmax> <We> <Ohd> <Ohs> <Ldomain>
```

Parameters:
- `MAXlevel`: Maximum adaptive mesh refinement level (e.g., 10)
- `tmax`: Maximum simulation time (e.g., 4e0)
- `We`: Weber number - ratio of inertial to surface tension forces (e.g., 1e1)
- `Ohd`: Ohnesorge number for drop phase - viscous/surface tension ratio (e.g., 1e-3)
- `Ohs`: Ohnesorge number for surrounding phase (e.g., 1e-5)
- `Ldomain`: Domain size in drop radii (e.g., 8e0)

Default values are set in `runCases.sh`:
```bash
LEVEL="10"
tmax="4e0"
We="1e1"
Ohd="1e-3"
Ohs="1e-5"
Ldomain="8e0"
```

## Simulation Code Structure

### Key Components in dropImpact.c

1. **Includes**: Basilisk modules for axisymmetric flow, Navier-Stokes, two-phase flow, and surface tension
   ```c
   #include "axi.h"
   #include "navier-stokes/centered.h"
   #include "two-phase.h"
   #include "tension.h"
   ```

2. **Error Tolerances**: Control adaptive mesh refinement
   ```c
   #define fErr (1e-3)    // VOF error tolerance
   #define KErr (1e-6)    // Curvature error tolerance
   #define VelErr (1e-2)  // Velocity error tolerance
   ```

3. **Physical Parameters**:
   - `Rho21`: Density ratio (1e-3 for air-water)
   - `Xdist`: Initial drop position (1.02 radii from axis)

4. **Boundary Conditions**:
   - Left (axis): No-slip, no fluid
   - Right/Top: Neumann for velocity, Dirichlet for pressure (open boundaries)

5. **Event System**: Basilisk uses events for temporal control
   - `init(t=0)`: Initial conditions with refinement
   - `adapt(i++)`: Called every timestep for mesh adaptation
   - `writingFiles(t+=tsnap)`: Save snapshots every 0.01 time units
   - `logWriting(i++)`: Write kinetic energy to log every timestep

### Output Files

Each simulation run produces:
- **log**: Time series of iteration, timestep, time, and kinetic energy
- **restart**: Full simulation state for restarts
- **intermediate/snapshot-X.XXXX**: Periodic dump files at intervals of `tsnap=0.01`

## Cleanup

Remove a simulation directory:
```bash
./cleanup.sh dropImpact
```

This removes the entire simulation output directory.

## Common Modifications

### Changing Physics

- **Density ratio**: Modify `Rho21` in the code
- **Surface tension**: Set via Weber number `We` (lower = stronger tension)
- **Viscosity**: Set via Ohnesorge numbers `Ohd` and `Ohs`
- **Initial velocity**: Modified in `init` event (currently `-1.0*f[]`)

### Mesh Refinement

- `MAXlevel`: Maximum refinement level (10 = 2^10 = 1024 cells per direction at finest)
- `MINlevel`: Minimum level (currently hardcoded to 4)
- Refinement criteria in `adapt` event based on VOF, curvature, and velocity

### Domain Size

- `Ldomain`: Controls computational domain size
- Unrefinement at boundaries prevents spurious refinement near outflow

## Notes

- The simulation uses axisymmetric coordinates (cylindrical with azimuthal symmetry)
- `x` is radial coordinate, `y` is axial coordinate in the code
- VOF field `f` represents the drop (f=1 inside drop, f=0 in surrounding fluid)
- Adaptive mesh refinement focuses resolution on interfaces and high-velocity regions
- The `display.html` file in output directories can visualize Basilisk dump files

## References

### Key Publications

- **Sanjay, V. & Lohse, D.** (2025). *Unifying theory of scaling in drop impact: forces and maximum spreading diameter*. Physical Review Letters, 134(10), 104003. [https://doi.org/10.1103/PhysRevLett.134.104003](https://doi.org/10.1103/PhysRevLett.134.104003)
  - Comprehensive scaling theory for drop impact forces and maximum spreading

- **Josserand, C. & Thoroddsen, S. T.** (2016). *Drop impact on a solid surface*. Annual Review of Fluid Mechanics, 48, 365-391. [https://doi.org/10.1146/annurev-fluid-122414-034401](https://doi.org/10.1146/annurev-fluid-122414-034401)
  - Comprehensive review of drop impact phenomena

- **Yarin, A. L.** (2006). *Drop impact dynamics: Splashing, spreading, receding, bouncing...*. Annual Review of Fluid Mechanics, 38, 159-192. [https://doi.org/10.1146/annurev.fluid.38.050304.092144](https://doi.org/10.1146/annurev.fluid.38.050304.092144)
  - Classical review of drop impact dynamics

### Software

- **Basilisk**: Open-source CFD solver - [http://basilisk.fr](http://basilisk.fr)
- **qcc**: Basilisk's C preprocessor with grid abstractions

## Documentation Generation

This rule provides guidance for maintaining and generating documentation for code repositories in the CoMPhy Lab, ensuring consistency and proper workflow for website generation.

### Documentation Guidelines

- Read `.github/Website-generator-readme.md` for the website generation process.
- Do not auto-deploy the website; generating HTML is permitted using `.github/scripts/build.sh`.
- Avoid editing HTML files directly; they are generated using `.github/scripts/build.sh`, which utilizes `.github/scripts/generate_docs.py`.
- The website is deployed at `https://comphy-lab.org/repositoryName`; refer to the `CNAME` file for configuration. Update if not done already.

### Process Details

The documentation generation process utilizes Python scripts to convert source code files into HTML documentation. The process handles C/C++, Python, Shell, and Markdown files, generating a complete documentation website with navigation, search functionality, and code highlighting.

### Documentation Best Practices

- Always use the build script for generating documentation rather than manually editing HTML files
- Customize styling through CSS files in `.github/assets/css/`
- Modify functionality through JavaScript files in `.github/assets/js/`
- For template changes, edit `.github/assets/custom_template.html`
- Troubleshoot generation failures by checking error messages and verifying paths and dependencies
