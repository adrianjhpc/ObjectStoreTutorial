#!/bin/bash

#SBATCH --nodes=2
#SBATCH --time=00:10:00
#SBATCH --job-name=daos_ind_example
#SBATCH -o daos_individual_example.%A.out
#SBATCH -e daos_individual_example.%A.err
#SBATCH --tasks-per-node=48
#SBATCH --cpus-per-task=1

module load mpi
module load compiler


srun ./daos_individual

