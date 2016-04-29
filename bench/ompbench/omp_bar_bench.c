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
int barrierKernel(int totalReps, int fill){
	int repIter;
	struct sk_measurement * mes;
	char outname[512];
	int i;
    	
	if (fill) {
        	sprintf(outname, "barriers_omp_fill%d", omp_get_max_threads());
    	} else {
        	sprintf(outname, "barriers_omp_rr%d", omp_get_max_threads());
    	}
	mes = (struct sk_measurement*) malloc(sizeof(struct sk_measurement)*omp_get_max_threads());
	for(i=0; i<omp_get_max_threads(); i++){
		uint64_t *buf = (uint64_t*) malloc(sizeof(uint64_t)*totalReps);
		sk_m_init(&(mes[i]), totalReps, outname, buf);
	}
	uint32_t* cores = placement(omp_get_max_threads(), fill);
	/* Open the parallel region */
#pragma omp parallel default(none) \
	private(repIter) \
	shared(totalReps,mes,cores)//,comm)
	{
	int tid =omp_get_thread_num(); 
	//set affinity
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cores[tid],&mask);
	pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t),&mask);
	for (repIter=0; repIter<totalReps; repIter++){

#pragma omp barrier
#pragma omp barrier 
 		sk_m_restart_tsc(&(mes[tid]));	
			{
#pragma omp barrier
			}
		sk_m_add(&(mes[tid]));	
	}
	}
	for(i=0; i<omp_get_max_threads(); i++)
   		sk_m_print(&(mes[i]),i);
	return 0;
}

int runBarrier(int totalReps, int fill, bool sync){
	int repIter;
	struct sk_measurement * mes;
	char outname[512];
	int i;
    	
	if (sync) {
        sprintf(outname, "barriers_ompsync_fill%d", omp_get_max_threads());
    } else {
        sprintf(outname, "barriers_omp_rr%d", omp_get_max_threads());
    }
	mes = (struct sk_measurement*) malloc(sizeof(struct sk_measurement)*omp_get_max_threads());
	for(i=0; i<omp_get_max_threads(); i++){
		uint64_t *buf = (uint64_t*) malloc(sizeof(uint64_t)*totalReps);
		sk_m_init(&(mes[i]), totalReps, outname, buf);
	}
	uint32_t* cores = placement(omp_get_max_threads(), fill);
	/* Open the parallel region */
#pragma omp parallel default(none) \
	private(repIter) \
	shared(totalReps,mes,cores,sync)//,comm)
	{
	int tid =omp_get_thread_num(); 
	//set affinity
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cores[tid],&mask);
	pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t),&mask);
	for (repIter=0; repIter<totalReps; repIter++){
        if (sync){
#pragma omp barrier
#pragma omp barrier
        } 
 		sk_m_restart_tsc(&(mes[tid]));	
			{
#pragma omp barrier
			}
		sk_m_add(&(mes[tid]));	
	}
	}
	for(i=0; i<omp_get_max_threads(); i++)
   		sk_m_print(&(mes[i]),i);
	return 0;
}


int barrierDriver(int totalReps){
	/* initialise repsToDo to defaultReps */
	int repsToDo = totalReps;
	printf("Entering\n");fflush(stdout);
	/* perform warm-up for benchmark */
	/* Initialise the benchmark */
	barrierKernel(repsToDo, 1);

	barrierKernel(repsToDo, 0);
	return 0;
}


int main(int argc, char *argv[]){


    int totalReps = 1000;
    printf("Calling Barrier driver\n");fflush(stdout);
    if (argc != 2) {
        barrierDriver(totalReps);
    } else {
        runBarrier(totalReps, 1, 0);
        runBarrier(totalReps, 1, 1);
    }
}
