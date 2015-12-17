#!/bin/bash

function error() {
	echo $1
	exit 1
}

function usage() {

    me=`basename "$BASH_SOURCE"`
    echo "Usage: $me <options> <Simulator args>"
	echo ""
	echo "Options (to be listed BEFORE Simulator args):"
	echo " -b: benchmark to execute (defaults to $BENCH)"
	echo " -s: do not execute the Simulator"
    exit 1
}

BENCH=bench/ab-bench
SIM=1

# Select benchmark
# ------------------------------
while [[ 1 ]]; do
	case $1 in
		"-b")
			BENCH=$2
			shift; shift
			[[ -f "$BENCH" ]] || error "Benchmark does not exist"
			;;

		"-s")
			SIM=0
			shift
			;;

		*)
			break
	esac
done

[[ -n "$1" ]] || usage

MACHINE=$1
shift

# Run Simulator
# ------------------------------
if [[ $SIM -eq 1 ]]; then
	scripts/run_simulator.sh $MACHINE $@
fi

# Compile
# ------------------------------
make clean || exit 1
make $BENCH || error "Compilation failed"

echo "Assuming machine to be $1" # This might not work in the future, grep from model then

set +x

# Copy executable
scp $BENCH $MACHINE:  || error "Failed to copy program"

# Run and log ..
TMP=$(mktemp)
ssh $MACHINE killall -s KILL $(basename $BENCH) &>/dev/null
echo "Running benchmark .."
ssh $MACHINE "./$(basename $BENCH) > /tmp/log"; RC=$?

[[ $RC -eq 0 ]] || exit $RC

scp $MACHINE:/tmp/log $TMP
cat $TMP | log_wrapper
rm $TMP

# Evalute .. 
DATE=$(date +"%Y-%m-%d-%H-%M")
FILE="$HOME/projects/phd/thesis/measurements/mp/mbench_${MACHINE}_$DATE"
log_show_last | sk_m_parser.py --crop .4
log_show_last > $FILE
echo "Wrote result to $FILE"

exit 0
