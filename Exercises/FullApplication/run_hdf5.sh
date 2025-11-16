#!/bin/bash

#SBATCH --nodes=2
#SBATCH --time=00:10:00
#SBATCH --job-name=hdf5_example
#SBATCH -o hdf5_example.%A.out
#SBATCH -e hdf5_example.%A.err
#SBATCH --tasks-per-node=48
#SBATCH --cpus-per-task=1
#SBATCH --nvram-options=1LM:1000

module load compiler/2021.1.1
module load mpi
module load gnu/11.2.0
module load mkl
module load libfabric/latest

export PSM2_MULTI_EP=1
export PSM2_MULTIRAIL=1
export PSM2_MULTIRAIL_MAP=0:1,1:1

export FI_PSM2_LAZY_CONN=0

export FI_PROVIDER=tcp

pool=tutorial
cont=tutorial-container
dfuse_path=/tmp/daos

export pool=tutorial
export cont=tutorial-container
export dfuse_path=/tmp/daos

srun --oversubscribe -N 1 -n 1 mkdir -p $dfuse_path/$USER/data

cp hdf5 $dfuse_path/$USER/data
cd $dfuse_path/$USER/data

srun --overlap --oversubscribe ./hdf5

srun --oversubscribe -N 1 -n 1 rm -fr $dfuse_path/$USER

