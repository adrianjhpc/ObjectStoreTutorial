#!/bin/bash

#SBATCH --nodes=2
#SBATCH --time=00:10:00
#SBATCH --job-name=daos_array_example
#SBATCH -o daos_array_example.%A.out
#SBATCH -e daos_array_example.%A.err
#SBATCH --tasks-per-node=48
#SBATCH --cpus-per-task=1
#SBATCH --nvram-options=1LM:1000

module load compiler/2021.1.1
module load mpi
module load gnu/11.2.0
module load mkl
module load libfabric/latest
module load phdf5/1.12.0/intel-intelmpi

export PSM2_MULTI_EP=1
export PSM2_MULTIRAIL=1
export PSM2_MULTIRAIL_MAP=0:1,1:1

export FI_PSM2_LAZY_CONN=0

export FI_PROVIDER=tcp

srun ./daos_array

