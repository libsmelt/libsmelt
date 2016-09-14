#!/bin/bash

set -e

MODEL_ROOT="model"
MAKE_NPROC=$(nproc)

export SMLT_HOSTNAME=$(hostname)
export SMLT_MACHINE=$(hostname -s)

if [ "$1" == "-d" ]; then
    LOGFILE=$(tty)
else
    LOGFILE=$MODEL_ROOT/create_model.log
fi

LIKWID_PATH="$MODEL_ROOT/likwid"
LIKWID_REPOSITORY="https://github.com/RRZE-HPC/likwid.git"

SIM_REPOSITORY="git@gitlab.inf.ethz.ch:skaestle/Simulator.git"
SIM_PATH="$MODEL_ROOT/Simulator"
SIM_MODEL="http://people.inf.ethz.ch/skaestle/machinemodel.gz"

MACHINEDB=$SIM_PATH/machinedb
OUTDIR=$MACHINEDB/$SMLT_MACHINE

export PATH=$PATH:$LIKWID_PATH:$LIKWID_PATH/ext/lua/:$LIKWID_PATH/ext/hwloc/:.
export LD_LIBRARY_PATH=$LIKWID_PATH:.

echo "SMLT_MACHINE=$SMLT_MACHINE"
echo "SMLT_HOSTNAME=$SMLT_HOSTNAME"
echo "MODEL_ROOT=$MODEL_ROOT"
echo "Creating model for $SMLT_MACHINE. logfile: $LOGFILE"
mkdir -p $MODEL_ROOT


if [[ ! -d $SIM_PATH ]]; then
    echo -e " \nSIMULATOR: setting up simulator in $SIM_PATH..."
    echo -n " - cloning simulator repository $SIM_REPOSITORY..."
    git clone $SIM_REPOSITORY $SIM_PATH > $LOGFILE 2>&1
    echo "OK."
    mkdir $SIM_PATH/graphs > $LOGFILE 2>&1
    mkdir $SIM_PATH/visu > $LOGFILE 2>&1
    mkdir $MACHINEDB > $LOGFILE 2>&1
    echo -n " - obtaining machinemodel from $SIM_MODEL..."
    wget $SIM_MODEL -O "$SIM_PATH/machinemodel.gz" > $LOGFILE 2>&1
    echo "OK."
    echo -n " - extracting model to $MACHINEDB..."
    tar -xzf "$SIM_PATH/machinemodel.gz" -C "$MACHINEDB" > $LOGFILE 2>&1
    echo "OK."
    echo "Simulator is successfully setup."
else
    echo -e "\nSIMULATOR: already set up in $SIM_PATH"
fi

if [[ ! -d $LIKWID_PATH ]]; then
    echo -e "\nLIKWID: setting up likwid in $LIKWID_PATH..."
    echo -n " - cloning likwid repository $LIKWID_REPOSITORY..."
    git clone $LIKWID_REPOSITORY $LIKWID_PATH > $LOGFILE 2>&1
    echo "OK."
    echo -n " - building likwid in $LIKWID_PATH..."
    (cd $LIKWID_PATH; make -j$MAKE_NPROC clean) > $LOGFILE 2>&1
    (cd $LIKWID_PATH; make -j$MAKE_NPROC) > $LOGFILE 2>&1
    echo "OK."
    echo -n " - building local installation of likwid in $LIKWID_PATH..."
    (cd $LIKWID_PATH; make -j$MAKE_NPROC local) > $LOGFILE 2>&1
    echo "OK"
    echo -e "likwid is successfully setup.\n"
else
    echo -e "\nLIKWID: already setup in $LIKWID_PATH\n"
fi


echo -n "SMELT: building bench/pairwise..."
make -j$MAKE_NPROC bench/pairwise  2>&1 > $LOGFILE
echo -e "OK.\n"

echo "MODEL: gathering data..."

mkdir -p $OUTDIR

echo " - running lscpu on $SMLT_MACHINE"
lscpu > $OUTDIR/lscpu.txt

echo " - running cpuid on $SMLT_MACHINE"
cpuid > $OUTDIR/cpuid.txt

echo " - running proc/cpuinfo"
cat /proc/cpuinfo > $OUTDIR/proc_cpuinfo.txt

echo " - running likwid on $SMLT_MACHINE"
likwid-topology > $OUTDIR/likwid.txt

echo " - running pairwise on $SMLT_MACHINE"
./bench/pairwise | gzip > $OUTDIR/pairwise.gz

echo -e "\nMODEL: parsing files"

echo " - parsing pairwise data for $SMLT_MACHINE"
zcat $OUTDIR/pairwise.gz | ./scripts/evaluate-pairwise.py --machine "$OUTDIR" --basename pairwise-nsend

echo -e "\nDONE. Model successfully created"
