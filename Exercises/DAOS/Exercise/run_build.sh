#!/bin/bash
#SBATCH -J build
#SBATCH --nodes=1
#SBATCH --time=0:10:0
#SBATCH --nvram-options=1LM:1000

./build.sh
