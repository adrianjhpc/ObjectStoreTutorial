#!/bin/bash
# Source this file:  source 00_env.sh
# Sets up the DAOS client environment + Python venv for the pydaos.torch tutorial.

INSTALL=/home/chaarawi/install
DAOS=$INSTALL/daos

# Python: venv with torch + numpy (activate first: it rewrites PATH)
source /home/chaarawi/my_venv/bin/activate

# DAOS client libraries and tools, plus the pydaos.torch module installed by the DAOS build
export PATH=$DAOS/bin:$PATH
export LD_LIBRARY_PATH=$DAOS/lib64:$DAOS/lib:$LD_LIBRARY_PATH
export DAOS_AGENT_DRPC_DIR=/tmp/chaarawi/daos_agent
export PYTHONPATH=$DAOS/lib64/python3.11/site-packages:$PYTHONPATH

# Tutorial parameters (used by all the example scripts)
export TUT_POOL=mypool
export TUT_CONT=pytorch_tut
export TUT_NSAMPLES=10000

# Quieter DAOS client logging for demos; raise to DEBUG when troubleshooting
export D_LOG_MASK=ERR
export D_LOG_FILE=/tmp/chaarawi/pytorch_tut.log

echo "pool=$TUT_POOL cont=$TUT_CONT python=$(python --version 2>&1)"
