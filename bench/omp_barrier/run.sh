export OMP_NUM_THREADS=$1
export OMP_PROC_BIND=true
export GOMP_CPU_AFFINITY="?"


./a.out > results.dat



