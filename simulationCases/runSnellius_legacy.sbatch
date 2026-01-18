#!/bin/bash
#SBATCH --job-name=jumpingDrops
#SBATCH --nodes=1
#SBATCH --ntasks=48
#SBATCH --cpus-per-task=1
#SBATCH --time=1:00:00
#SBATCH --partition=genoa
#SBATCH --mail-type=ALL
#SBATCH --mail-user=vatsal.sanjay@comphy-lab.org
#SBATCH --output=slurm-%j.out
#SBATCH --error=slurm-%j.err

# # runSnellius_legacy.sbatch
#
# Legacy SLURM submission script for Snellius. Compiles and runs
# `dropImpact_legacy.c` with fixed parameters.
#
# ## Usage
# `sbatch runSnellius_legacy.sbatch`
#
# ## Outputs
# - SLURM logs: `slurm-%j.out` and `slurm-%j.err`
#
set -euo pipefail

# ============================================================
# Paths
# ============================================================
SCRIPT_FILE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_FILE_DIR}/.." && pwd)"

# Load required modules
module purge
module load 2024
module load OpenMPI/5.0.3-GCC-13.3.0

# ============================================================
# Setup Basilisk Environment
# ============================================================
echo "Setting up Basilisk environment..."
cd "$PROJECT_ROOT"

if ! command -v curl &> /dev/null; then
    echo "ERROR: curl not found (required for Basilisk install)" >&2
    exit 1
fi

BASILISK_REF="v2026-01-13"
BASILISK_INSTALL_URL="https://raw.githubusercontent.com/comphy-lab/basilisk-C/main/reset_install_basilisk-ref-locked.sh"
LOCK_FILE="${PROJECT_ROOT}/basilisk/.comphy-lock"
LOCK_REF=""
if [ -f "$LOCK_FILE" ]; then
    LOCK_REF="$(awk -F= '$1=="ref"{print $2; exit}' "$LOCK_FILE" 2>/dev/null || true)"
fi
if [ ! -f "$LOCK_FILE" ] || [ "$LOCK_REF" != "$BASILISK_REF" ]; then
    echo "Lock mismatch or missing (found ref='${LOCK_REF:-none}') - performing hard install..."
    curl -fsSL "$BASILISK_INSTALL_URL" | bash -s -- --ref="$BASILISK_REF" --hard
else
    curl -fsSL "$BASILISK_INSTALL_URL" | bash -s -- --ref="$BASILISK_REF"
fi

if [ -f "${PROJECT_ROOT}/.project_config" ]; then
    source "${PROJECT_ROOT}/.project_config"
else
    echo "ERROR: .project_config not found after Basilisk install" >&2
    exit 1
fi

# Print diagnostic information
echo "Job started at: $(date)"
echo "Running on node: $(hostname)"
echo "Working directory: $(pwd)"
echo "Number of MPI tasks: ${SLURM_NTASKS}"

LEVEL="10"
tmax="8e0"
We="1e1"
Ohd="4e-3"
Ohs="1e-5"
Ldomain="4"

cd "$SCRIPT_FILE_DIR"

CC99='mpicc -std=c99 -D_GNU_SOURCE=1' qcc -Wall -O2 -D_MPI=1 -disable-dimensions dropImpact_legacy.c -o dropImpact_legacy -lm 

# Run the simulation
srun JumpingDrops_Snellius $LEVEL $tmax $We $Ohd $Ohs $Ldomain 

echo "Job completed at: $(date)"
