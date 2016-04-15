typedef struct timer{
	HRT_TIMESTAMP_T t1;
	HRT_TIMESTAMP_T t2;
	UINT64_T elapsed_ticks;
}my_timer_t;


int barrierDriver(){
	/* initialise repsToDo to defaultReps */
	repsToDo = defaultReps;
	printf("Entering\n");fflush(stdout);
	/* perform warm-up for benchmark */
	barrierKernel(warmUpIters);
	printf("Warmup done\n");fflush(stdout);
	/* Initialise the benchmark */
	barrierKernel(repsToDo);

	int i;
	for(i=0; i<repsToDo*numThreads;i++){
		printf("%d\t %f\n",numThreads,benchReport.usecs[i]);	
	}
	return 0;
}

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
	my_timer_t timer;
	double tmp_usecs;
	/* Open the parallel region */
#pragma omp parallel default(none) \
	private(repIter,timer,tmp_usecs) \
	shared(totalReps,benchReport)//,comm)
	{
	int tid =omp_get_thread_num(); 
	for (repIter=0; repIter<totalReps; repIter++){

#pragma omp barrier
#pragma omp barrier 

//	printf("Calling barrier\n");
		/* Threads synchronise with an OpenMP barrier */
			HRT_GET_TIMESTAMP(timer.t1);
			//if(tid>=0){
			{
#pragma omp barrier
			}
			HRT_GET_TIMESTAMP(timer.t2);
			HRT_GET_ELAPSED_TICKS(timer.t1,timer.t2, &(timer.elapsed_ticks));
			timer.elapsed_ticks -= benchReport.rdtsc_latency;
			tmp_usecs = HRT_GET_USEC(timer.elapsed_ticks, benchReport.g_timerfreq);
			benchReport.usecs[totalReps*tid+repIter]=tmp_usecs;

	}
	}
	return 0;
}



