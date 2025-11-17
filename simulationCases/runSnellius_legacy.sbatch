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

# Load required modules
module purge
module load 2024
module load OpenMPI/5.0.3-GCC-13.3.0

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

CC99='mpicc -std=c99 -D_GNU_SOURCE=1' qcc -Wall -O2 -D_MPI=1 -disable-dimensions dropImpact_legacy.c -o dropImpact_legacy -lm 

# Run the simulation
srun JumpingDrops_Snellius $LEVEL $tmax $We $Ohd $Ohs $Ldomain 

echo "Job completed at: $(date)"

