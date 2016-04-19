

FILL="0,2,4" #fill with list of cores given by parse_cores
RR="1,2,3" 

MAXTH=$1
export OMP_PROC_BIND=true

for ((i=2; i<=$MAXTH; i+=1))
do
	export OMP_NUM_THREADS=$1
	export GOMP_CPU_AFFINITY=$FILL
	./omp_var_bench > omp_fill_$i.dat
	export GOMP_CPU_AFFINITY=$RR
	./omp_var_bench > omp_rr_$i.dat
done


