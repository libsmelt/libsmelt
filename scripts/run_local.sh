#!/bin/bash

set -e

export SMLT_HOSTNAME=$(hostname)

if [ "$1" == "" ]; then
    export SMLT_MACHINE=$(hostname -s)
else
    export SMLT_MACHINE=$1
fi

MODEL_ROOT="model"

MAKE_NPROC=$(nproc)

LIKWID_PATH="$MODEL_ROOT/likwid"
LIKWID_REPOSITORY="https://github.com/RRZE-HPC/likwid.git"

SIM_REPOSITORY="git@gitlab.inf.ethz.ch:skaestle/Simulator.git"
SIM_PATH="$MODEL_ROOT/Simulator"

MACHINEDB=$SIM_PATH/machinedb
OUTDIR=$MACHINEDB/$SMLT_MACHINE

export PATH=$PATH:$LIKWID_PATH:$LIKWID_PATH/ext/lua/:$LIKWID_PATH/ext/hwloc/:.
export LD_LIBRARY_PATH=$LIKWID_PATH:.

echo "SMLT_MACHINE=$SMLT_MACHINE"
echo "SMLT_HOSTNAME=$SMLT_HOSTNAME"
echo "MODEL_ROOT=$MODEL_ROOT"
echo "Creating model for $SMLT_MACHINE. logfile: $MODEL_ROOT/create_model.log"
mkdir -p $MODEL_ROOT

echo "checking simulator in $SIM_PATH"
if [[ ! -d $SIM_PATH ]]; then
    echo -n "   - setting up simulator in $SIM_PATH..."
    git clone $SIM_REPOSITORY $SIM_PATH > $MODEL_ROOT/create_model.log 2>&1
    mkdir $SIM_PATH/graphs > $MODEL_ROOT/create_model.log 2>&1
    mkdir $SIM_PATH/visu > $MODEL_ROOT/create_model.log 2>&1
    mkdir $MACHINEDB > $MODEL_ROOT/create_model.log 2>&1
    wget 'http://people.inf.ethz.ch/skaestle/machinemodel.gz' -O "$SIM_PATH/machinemodel.gz" > $MODEL_ROOT/create_model.log 2>&1
    tar -xzf "$SIM_PATH/machinemodel.gz" -C "$MACHINEDB" > $MODEL_ROOT/create_model.log 2>&1
    echo "OK"
fi

echo "checking likwid in $LIKWID_PATH"
if [[ ! -d $LIKWID_PATH ]]; then
    echo -n "   - setting up likqid in $LIKWID_PATH..."
    git clone $LIKWID_REPOSITORY $LIKWID_PATH > $MODEL_ROOT/create_model.log 2>&1

    (cd $LIKWID_PATH; make -j$MAKE_NPROC clean) > $MODEL_ROOT/create_model.log 2>&1
    (cd $LIKWID_PATH; make -j$MAKE_NPROC) > $MODEL_ROOT/create_model.log 2>&1
    (cd $LIKWID_PATH; make -j$MAKE_NPROC local) > $MODEL_ROOT/create_model.log 2>&1
    echo "OK"
fi

echo -n "building bench/pairwise..."
make -j$MAKE_NPROC bench/pairwise  > $MODEL_ROOT/create_model.log 2>&1
echo "OK."

echo "gathering data..."

mkdir -p $OUTDIR

echo "   - running lscpu on $SMLT_MACHINE"
lscpu > $OUTDIR/lscpu.txt

echo "   - running cpuid on $SMLT_MACHINE"
cpuid > $OUTDIR/cpuid.txt


echo "   - running proc/cpuinfo"
cat /proc/cpuinfo > $OUTDIR/proc_cpuinfo.txt

echo "   - running likwid on $SMLT_MACHINE"
likwid-topology > $OUTDIR/likwid.txt


echo "   - running pairwise on $SMLT_MACHINE"
./bench/pairwise | gzip > $OUTDIR/pairwise.gz
#./pairwise_no_ht | gzip > $OUTDIR/pairwise.gz

echo "   - parsing pairwise data for $SMLT_MACHINE"
zcat $OUTDIR/pairwise.gz | ./scripts/evaluate-pairwise.py --machine "$OUTDIR" --basename pairwise-nsend

echo "DONE. Model successfully created"
