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

[[ -n "$1" ]] || usage

SIMPATH=~/projects/Simulator/
BENCH=bench/ab-bench

MACHINE=$1
echo "Assuming machine to be $1" # This might not work in the future, grep from model then

set +x

# Copy executable
scp $BENCH $MACHINE:

# Run and log ..
TMP=$(mktemp)
ssh $MACHINE killall -s KILL $(basename $BENCH)
ssh -t $MACHINE "./$(basename $BENCH) | tee /tmp/log"
scp $MACHINE:/tmp/log $TMP
cat $TMP | log_wrapper
rm $TMP

# Evalute .. 
log_show_last | sk_m_parser.py --crop .4


