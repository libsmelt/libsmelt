#include <omp.h>
#include "measurement_framework.h"




/*-----------------------------------------------------------*/
/* barrierKernel                                             */
/* 															 */
/* Main kernel for barrier benchmark.                        */
/* First threads under each process synchronise with an      */
/* OMP BARRIER. Then a MPI barrier synchronises each MPI     */
/* process. MPI barrier is called within a OpenMP master     */
/* directive.                                                */
/*-----------------------------------------------------------*/
int barrierKernel(int totalReps){
	int repIter;
	struct sk_measurement * mes;
	char outname[512];
	int i;
    	
	sprintf(outname, "barriers_omp%d", omp_get_max_threads());
	printf("Init mes (nthreads = %d)\n",omp_get_max_threads());
	mes = (struct sk_measurement*) malloc(sizeof(struct sk_measurement)*omp_get_max_threads());
	for(i=0; i<omp_get_max_threads(); i++){
		uint64_t *buf = (uint64_t*) malloc(sizeof(uint64_t)*totalReps);
		sk_m_init(&(mes[i]), totalReps, outname, buf);
	}
	
	/* Open the parallel region */
#pragma omp parallel default(none) \
	private(repIter) \
	shared(totalReps,mes)//,comm)
	{
	int tid =omp_get_thread_num(); 
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
int barrierDriver(int totalReps){
	/* initialise repsToDo to defaultReps */
	int repsToDo = totalReps;
//	int warmUpIters = 10;
	printf("Entering\n");fflush(stdout);
	/* perform warm-up for benchmark */
//	barrierKernel(warmUpIters,1);
//	printf("Warmup done\n");fflush(stdout);
	/* Initialise the benchmark */
	barrierKernel(repsToDo);

	
	return 0;
}


int main(int argc, char *argv[]){


	int totalReps = 1000;
	printf("Calling Barrier driver\n");fflush(stdout);
	barrierDriver(totalReps);

}
