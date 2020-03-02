#!/bin/bash

set -e

echo "SMELT BUILD MODEL SCRIPT"

MODEL_ROOT="model"
MAKE_NPROC=$(nproc)

export SMLT_HOSTNAME=$(hostname)
export SMLT_MACHINE=$(hostname -s)

if [[ "$1" == "-d" ]]; then
    LOGFILE=/dev/stdout
else
    LOGFILE=$(readlink -f $MODEL_ROOT/create_overlays_${SMLT_MACHINE}.log)
fi

LIKWID_PATH="$MODEL_ROOT/likwid"
LIKWID_REPOSITORY="https://github.com/RRZE-HPC/likwid.git"

SIM_REPOSITORY="https://github.com/libsmelt/Simulator.git"
SIM_PATH="$MODEL_ROOT/Simulator"

MACHINEDB=$SIM_PATH/machinedb
OUTDIR=$MACHINEDB/machine-data/$SMLT_MACHINE

export PATH=$PATH:$LIKWID_PATH:$LIKWID_PATH/ext/lua/:$LIKWID_PATH/ext/hwloc/:.
export LD_LIBRARY_PATH=$LIKWID_PATH:.

echo "SMLT_MACHINE=$SMLT_MACHINE"
echo "SMLT_HOSTNAME=$SMLT_HOSTNAME"
echo "MODEL_ROOT=$MODEL_ROOT"
echo "Creating overlays for $SMLT_MACHINE. logfile: $LOGFILE"

pushd $SIM_PATH

MODELS="adaptivetree badtree mst bintree cluster fibonacci sequential"


for m in $MODELS; do
    echo $m
    ./simulator.py  $SMLT_MACHINE $m 
done
popd


