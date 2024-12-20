#!/bin/bash

#SBATCH --nodes=2
#SBATCH --time=00:10:00
#SBATCH --job-name=daos_array_example
#SBATCH -o daos_array_example.%A.out
#SBATCH -e daos_array_example.%A.err
#SBATCH --tasks-per-node=32
#SBATCH --cpus-per-task=1

module load openmpi 

srun ./daos_array

