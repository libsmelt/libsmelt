#!/bin/sh

#FILL="0,2,4" #fill with list of cores given by parse_cores
#RR="1,2,3" 

MAXTH=$1
export OMP_PROC_BIND=true

for i in $(seq 2 $1)
do
    echo $i
	export OMP_NUM_THREADS=$i
#	export GOMP_CPU_AFFINITY=$FILL
	./omp_bar_bench
#	export GOMP_CPU_AFFINITY=$RR
#	./omp_bar_bench > omp_rr_$i.dat
done


