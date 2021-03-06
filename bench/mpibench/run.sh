#!/bin/bash

export LD_LIBRARY_PATH=/usr/local/lib/
<<comment2
for i in {30000..45000}
do
    kill -9 $i
done
comment2

echo "compile and generate rank files"
rm -r rank_files
rm -r barrier
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

<<comment3
echo "run benchmark"
FILES=rank_files/*
for f in $FILES
do
    IFS='_' read -ra SPLIT <<< $f

    >&2 echo "Running Rank File $f"

    S1='rr'
    S2=${SPLIT[2]}
    echo ${SPLIT[2]}
<<c
    if [${SPLIT[3]} = 32]
    then
        if [ $S1 -eq $S2 ]
        then
            /home/haeckir/openmpi-1.10.2/bin/mpirun -H localhost -n ${SPLIT[3]} -rf $f -mca --bind-to core -mca blt sm,self barrier 1
        else
            /home/haeckir/openmpi-1.10.2/bin/mpirun -H localhost -n ${SPLIT[3]} -rf $f -mca rmaps_rank_file_physical 1 -mca --bind-to core -mca blt sm,self barrier 
        fi
    fi
c
    if [ $S1 = $S2 ]
    then
        /home/haeckir/openmpi-1.7.5/bin/mpirun -H localhost -n ${SPLIT[3]} -rf $f -mca rmaps_rank_file_physical 1 -mca --bind-to core -mca blt sm,self barrier 1
    else
        /home/haeckir/openmpi-1.7.5/bin/mpirun -H localhost -n ${SPLIT[3]} -rf $f -mca rmaps_rank_file_physical 1 -mca --bind-to core -mca blt sm,self barrier 
    fi
done
comment3
#<<comment
for i in {2..32}
do
    >&2 echo "Running with $i cores"
    /home/haeckir/openmpi-1.10.2/bin/mpirun -H localhost -n $i -rf rank_files/rfile_fill_$i -mca rmaps_rank_file_physical 1 -mca --bind-to core -mca blt sm,self barrier 
    /home/haeckir/openmpi-1.10.2/bin/mpirun -H localhost -n $i -rf rank_files/rfile_rr_$i -mca rmaps_rank_file_physical 1 -mca --bind-to core -mca blt sm,self barrier 1
done
#comment
