#!/bin/bash

#SBATCH --nodes=2
#SBATCH --time=00:10:00
#SBATCH --job-name=hdf5_example
#SBATCH -o hdf5_example.%A.out
#SBATCH -e hdf5_example.%A.err
#SBATCH --tasks-per-node=48
#SBATCH --cpus-per-task=1
#SBATCH --nvram-options=1LM:1000

module load compiler
module load mpi
module load hdf5

pool=default-pool
cont=default-container
dfuse_path=/tmp/daos

export pool=default-pool
export cont=default-container
export dfuse_path=/tmp/daos

srun -N $SLURM_NNODES -n $SLURM_NNODES sudo umount $dfuse_path > /dev/null 2>&1

srun -N $SLURM_NNODES -n $SLURM_NNODES mkdir -p $dfuse_path

srun -N $SLURM_NNODES -n $SLURM_NNODES  dfuse -f -m $dfuse_path --pool $pool --container $cont --multi-user \
    --disable-caching --thread-count=24 --eq-count=12 &

sleep 2

srun --oversubscribe -N 1 -n 1 mkdir -p $dfuse_path/$USER/data

cp hdf5 $dfuse_path/$USER/data
cd $dfuse_path/$USER/data

srun --overlap --oversubscribe ./hdf5

srun --oversubscribe -N 1 -n 1 rm -fr $dfuse_path/$USER

srun --oversubscribe -N $SLURM_NNODES -n $SLURM_NNODES sudo umount $dfuse_path > /dev/null 2>&1

killall -u $USER

wait

