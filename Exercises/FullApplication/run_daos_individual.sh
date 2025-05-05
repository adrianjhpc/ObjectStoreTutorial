#!/bin/bash

#SBATCH --nodes=2
#SBATCH --time=00:10:00
#SBATCH --job-name=daos_ind_example
#SBATCH -o daos_individual_example.%A.out
#SBATCH -e daos_individual_example.%A.err
#SBATCH --tasks-per-node=48
#SBATCH --cpus-per-task=1
#SBATCH --nvram-options=1LM:1000

module load compiler
module load mpi
module load gnu/11.2.0
module load libfabric/latest

export PSM2_MULTI_EP=1
export PSM2_MULTIRAIL=1
export PSM2_MULTIRAIL_MAP=0:1,1:1

export PSM2_DEVICES=self,hfi,shm

export DAOS_AGENT_DRPC_DIR=/var/daos_agent

export FI_PROVIDER_PATH=/home/software/libfabric/latest/lib

srun ./daos_individual

