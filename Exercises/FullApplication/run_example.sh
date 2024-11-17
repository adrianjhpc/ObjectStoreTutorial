#!/bin/bash

#SBATCH --nodes=2
#SBATCH --time=00:10:00
#SBATCH --job-name=hdf5_example
#SBATCH -o hdf5_example.%A.out
#SBATCH -e hdf5_example.%A.err
#SBATCH --tasks-per-node=32
#SBATCH --cpus-per-task=1

module load openmpi 

export LD_LIBRARY_PATH=/home/tu001/hdf5/1.14.5/lib:$LD_LIBRARY_PATH

pool=default-pool
cont=default-container
dfuse_path=/tmp/parallelstore

export pool=default-pool
export cont=default-container
export dfuse_path=/tmp/parallelstore

srun -N $SLURM_NNODES -n $SLURM_NNODES sudo umount $dfuse_path > /dev/null 2>&1

srun -N $SLURM_NNODES -n $SLURM_NNODES mkdir -p $dfuse_path

srun -N $SLURM_NNODES -n $SLURM_NNODES  dfuse -f -m $dfuse_path --pool $pool --container $cont --multi-user \
    --disable-caching --thread-count=24 --eq-count=12 &

sleep 2

srun --oversubscribe -N 1 -n 1 mkdir -p $dfuse_path/$USER/data

cp example $dfuse_path/$USER/data
cd $dfuse_path/$USER/data

srun --overlap --oversubscribe ./example

ls -lhrt $dfuse_path/$USER/data

srun --oversubscribe -N 1 -n 1 rm -fr $dfuse_path/$USER

srun --oversubscribe -N $SLURM_NNODES -n $SLURM_NNODES sudo umount $dfuse_path > /dev/null 2>&1

killall -u $USER

wait

