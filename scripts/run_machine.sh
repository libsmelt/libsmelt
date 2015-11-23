#!/bin/bash

function error() {
	echo $1
	exit 1
}

function usage() {

    me=`basename "$BASH_SOURCE"`
    echo "Usage: $me <Simulator args>"
    exit 1
}

SIMPATH=~/projects/Simulator/
BENCH=bench/ab-bench

pushd $SIMPATH
./simulator.py $@; RC=$?
popd

[[ $RC -eq 0 ]] || error "Generating model failed, aborting"

cp $SIMPATH/model.h $SIMPATH/model_defs.h inc/
make clean
make $BENCH

MACHINE=$1
echo "Assuming machine to be $1" # This might not work in the future, grep from model then

set +x

# Copy executable
scp $BENCH $MACHINE:

# Run and log ..
TMP=$(mktemp)
ssh $MACHINE killall -s KILL $(basename $BENCH)
ssh $MACHINE "./$(basename $BENCH) > log"
scp $MACHINE:log $TMP
cat $TMP | log_wrapper
rm $TMP

# Evalute .. 
log_show_last | sk_m_parser.py --crop .4


