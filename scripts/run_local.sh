#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export LD_LIBRARY_PATH=`dirname $DIR`:$LD_LIBRARY_PATH

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

MACHINE=`hostname -s`
shift

export SMLT_HOSTNAME="localhost"
export SMTL_MACHINE=$MACHINE
export SMLT_TOPO=adaptivetree-shuffle-sort

# Run Simulator
# ------------------------------
if [[ $SIM -eq 1 ]]; then
    scripts/run_simulator.sh $MACHINE $@
fi

# Compile
# ------------------------------
make clean || exit 1
make $BENCH || error "Compilation failed"

echo "Assuming machine to be $MACHINE" # This might not work in the future, grep from model then


killall simulator.py
pushd model/Simulator
./simulator.py --server &
popd
SIM_PID=$!

sleep 2

set +x

# Copy executable
killall -s KILL $(basename $BENCH) &>/dev/null

# Run and log ..
killall -s KILL $(basename $BENCH) &>/dev/null
echo "Running benchmark .. ./$BENCH "

DATE=$(date +"%Y-%m-%d-%H-%M")

BENCH_NAME=$(basename $BENCH)

RESULTS_DIR=measurements/$SMTL_MACHINE/

mkdir -p $RESULTS_DIR
    

./$BENCH | gzip -9 > $RESULTS_DIR/$BENCH_NAME-$DATE.gz 
cp $RESULTS_DIR/$BENCH_NAME-$DATE.gz  $RESULTS_DIR/$BENCH_NAME.last.gz

# Evaluate .. 
zcat $RESULTS_DIR/$BENCH_NAME-$DATE.gz  | ./scripts/sk_m_parser.py --crop .4

./scripts/plot-ab-bench.py --machines $SMTL_MACHINE
#./scripts/plot-ab-bench-heatmap.py  --machines $SMTL_MACHINE




kill -9 $SIM_PID

exit 0
