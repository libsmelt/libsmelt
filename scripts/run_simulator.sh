#!/bin/bash

function error() {
	echo $1
	exit 1
}

function usage() {

    me=`basename "$BASH_SOURCE"`
    echo "Usage: $me <machine> [<simulator_args>]"
	echo ""
	echo "simulator_args: default is \"adaptivetree bintree cluster mst badtree fibonacci\""
    exit 1
}

[[ -n "$1" ]] || usage

# set -x # <<<< Enable for Debug

MODEL_ROOT="model"
MAKE_NPROC=$(nproc)

export SMLT_HOSTNAME=$(hostname)
export SMLT_MACHINE=$(hostname -s)

if [[ "$1" == "-d" ]]; then
    LOGFILE=/dev/stdout
else
    LOGFILE=$MODEL_ROOT/create_model_${SMLT_MACHINE}.log
fi

LIKWID_PATH="$MODEL_ROOT/likwid"
LIKWID_REPOSITORY="https://github.com/RRZE-HPC/likwid.git"

SIM_REPOSITORY="https://github.com/libsmelt/Simulator.git"
SIM_PATH="$MODEL_ROOT/Simulator"

MACHINEDB=$SIM_PATH/machinedb
OUTDIR=$MACHINEDB/machine-data/$SMLT_MACHINE


pushd $SIM_PATH

M=$1; shift

if [[ -z "$1" ]]; then
	SIMARGS="adaptivetree bintree cluster mst badtree fibonacci"
	echo "Setting default Simulator arguments $SIMARGS"
else
	SIMARGS=$@
fi

mkdir -p measurements

DATE=$(date +"%Y-%m-%d-%H-%M")
FILE="measurements/sim_${MACHINE}_$DATE"
echo "Writing Simulator output to: $FILE"
./simulator.py $M $SIMARGS &> "$FILE"; RC=$?

if [[ $RC -ne 0 ]]; then
	cat $FILE
fi 

popd

[[ $RC -eq 0 ]] || error "Generating model failed, aborting"
cp $SIM_PATH/model.h $SIM_PATH/model_defs.h inc/

exit $RC
