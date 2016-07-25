#!/bin/sh

if [ -z "$1" ]; then
    echo "Usage: $0 num_threads"
    exit 1
fi

MAXTH=$1
export OMP_PROC_BIND=true
export OMP_NUM_THREADS=$1
./omp_red_bench
./omp_bar_bench 1
