#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 num_threads"
    exit 1
fi

NUMPROC=$1

echo "compile and generate rank files"
rm -r rank_files
mkdir rank_files

CPUFILE=/proc/cpuinfo

NUMPHY=`grep "physical id" $CPUFILE | sort -u | wc -l`
NUMLOG=`grep "processor" $CPUFILE | wc -l`

if [[ $NUMPROC -lt $NUMPHY ]]
then
    echo "Hyperthreads enabled"
    ./parse_cores 1
else
    echo "Hyperthreads disabled"
    ./parse_cores
fi

DIR=/home/haeckir/openmpi-1.10.2/bin
if [[ -d $DIR ]]; then

	echo "Could not find MPI, trying local directory"
	export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
	DIR=.

	./mpirun -h
fi

ARGS="-H localhost -n $NUMPROC -rf rank_files/rfile_fill_$NUMPROC -mca rmaps_rank_file_physical 1 -mca --bind-to core -mca blt sm,self"

$DIR/mpirun $ARGS broadcast 1
$DIR/mpirun $ARGS broadcast
$DIR/mpirun $ARGS barrier 1 2
$DIR/mpirun $ARGS barrier 1 1
$DIR/mpirun $ARGS reduction 1
$DIR/mpirun $ARGS reduction

rm -r rank_files
