#!/bin/sh

#FILL="0,2,4" #fill with list of cores given by parse_cores
#RR="1,2,3" 

./compila.sh

MAXTH=$1
export OMP_PROC_BIND=true
export OMP_NUM_THREADS=$i
./omp_red_bench
./omp_bar_bench 1

