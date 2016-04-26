#!/bin/bash

echo "compile and generate rank files"
rm -r rank_files
mkdir rank_files
./compile.sh

CPUFILE=/proc/cpuinfo

NUMPHY=`grep "physical id" $CPUFILE | sort -u | wc -l`
NUMLOG=`grep "processor" $CPUFILE | wc -l`

if [ $NUMLOG > $NUMPHY ]
then
    echo "Hyperthreads enabled"
    ./parse_cores
else
    echo "Hyperthreads disabled"
    ./parse_cores
fi

echo "run benchmark"
FILES=rank_files/*
for f in $FILES
do
    
    IFS='_' read -ra SPLIT <<< $f

    >&2 echo "Running Rank File $f"

   # /home/haeckir/Desktop/openmpi-1.10.2/bin/mpirun -H localhost -n ${SPLIT[3]} -rf $f -mca rmaps_rank_file_physical 1 -mca --bind-to core -mca blt sm,self barrier
done
rm 1
rm -r rank_files
