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

SIMPATH=~/projects/Simulator/

pushd $SIMPATH

M=$1; shift

if [[ -z "$1" ]]; then
	SIMARGS="adaptivetree bintree cluster mst badtree fibonacci"
	echo "Setting default Simulator arguments $SIMARGS"
else
	SIMARGS=$@
fi

[[ $M == "ziger2" ]] && M="ziger1"
DATE=$(date +"%Y-%m-%d-%H-%M")
FILE="$HOME/projects/phd/thesis/measurements/mp/sim_${MACHINE}_$DATE"
echo "Writing Simulator output to: $FILE"
./simulator.py $M $SIMARGS &> "$FILE"; RC=$?

if [[ $RC -ne 0 ]]; then
	cat $FILE
fi 

popd

[[ $RC -eq 0 ]] || error "Generating model failed, aborting"
cp $SIMPATH/model.h $SIMPATH/model_defs.h inc/

exit $RC
