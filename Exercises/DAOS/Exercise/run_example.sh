#!/bin/bash
#SBATCH -J run_daos
#SBATCH --nodes=1
#SBATCH --time=0:10:0
#SBATCH --nvram-options=1LM:1000

./run.sh
