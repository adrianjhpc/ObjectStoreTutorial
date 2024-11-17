#!/bin/bash

#SBATCH --nodes=2
#SBATCH --time=00:10:00
#SBATCH --job-name=daos_example
#SBATCH -o daos_example.%A.out
#SBATCH -e daos_example.%A.err
#SBATCH --tasks-per-node=32
#SBATCH --cpus-per-task=1

module load openmpi 

pool=default-pool
cont=default-container
dfuse_path=/tmp/parallelstore

export pool=default-pool
export cont=default-container
export dfuse_path=/tmp/parallelstore

srun ./daos

