#include <omp.h>
#include "measurement_framework.h"
#include "placement.h"
#include <sched.h>
#include <pthread.h>

/*-----------------------------------------------------------*/
/* barrierKernel                                             */
/* 															 */
/* Main kernel for barrier benchmark.                        */
/* First threads under each process synchronise with an      */
/* OMP BARRIER. Then a MPI barrier synchronises each MPI     */
/* process. MPI barrier is called within a OpenMP master     */
/* directive.                                                */
/*-----------------------------------------------------------*/
int reduceKernel(int totalReps, int fill){
	int repIter;
	struct sk_measurement * mes;
	char outname[512];
	int i;
    	char address = 1;

	//ensure that master runs in core 0
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(0,&mask);
	pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t),&mask);



	if (fill) {
        	sprintf(outname, "barriers_omp_fill%d", omp_get_max_threads());
    	} else {
        	sprintf(outname, "barriers_omp_rr%d", omp_get_max_threads());
    	}
	mes = (struct sk_measurement*) malloc(sizeof(struct sk_measurement)*(omp_get_max_threads()));
	for(i=0; i<omp_get_max_threads(); i++){
		uint64_t *buf = (uint64_t*) malloc(sizeof(uint64_t)*totalReps);
		sk_m_init(&(mes[i]), totalReps, outname, buf);
	}
	uint32_t* cores = placement(omp_get_max_threads(), fill);
	for (repIter=0; repIter<totalReps; repIter++){	
/* Open the parallel region */
	//	sk_m_restart_tsc(&(mes[omp_get_max_threads()]));	

#pragma omp parallel default(none) \
	private(mask)\
	shared(mes,cores)\
	reduction(+:address)
		{
		int tid =omp_get_thread_num(); 
//		sk_m_restart_tsc(&(mes[tid]));	

	//set affinity
		CPU_ZERO(&mask);
		CPU_SET(cores[tid],&mask);
		pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t),&mask);
		address+=1;
#pragma omp barrier
#pragma omp barrier 
		sk_m_restart_tsc(&(mes[tid]));	
	//	sk_m_add(&(mes[tid]));	
		}
		//sk_m_add(&(mes[omp_get_max_threads()]));	
		sk_m_add(&(mes[tid]));	
	}
//	for(i=0; i<=omp_get_max_threads(); i++)
   		sk_m_print(&(mes[0]),0);
	return 0;
}
int reduceDriver(int totalReps){
	/* initialise repsToDo to defaultReps */
	int repsToDo = totalReps;
//	int warmUpIters = 10;
	printf("Entering\n");fflush(stdout);
	/* perform warm-up for benchmark */
//	barrierKernel(warmUpIters,1);
//	printf("Warmup done\n");fflush(stdout);
	/* Initialise the benchmark */
	reduceKernel(repsToDo, 1);

	reduceKernel(repsToDo, 0);
	return 0;
}


int main(int argc, char *argv[]){


	int totalReps = 1000;
	printf("Calling Reduce driver\n");fflush(stdout);
	reduceDriver(totalReps);

}
