#!/bin/bash

#SBATCH --nodes=2
#SBATCH --time=00:10:00
#SBATCH --job-name=daos_array_example
#SBATCH -o daos_array_example.%A.out
#SBATCH -e daos_array_example.%A.err
#SBATCH --tasks-per-node=48
#SBATCH --cpus-per-task=1
#SBATCH --nvram-options=1LM:1000

module load compiler
module load mpi

srun ./daos_array

