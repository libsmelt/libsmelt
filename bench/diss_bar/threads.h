/*
 * threads.h
 *
 *  Created on: 4 Dec, 2012
 *      Author: sabela
 */

#ifndef THREADS_H_
#define THREADS_H_


#include <math.h>
#include "barrier.h"
#include "measurement_framework.h"

#include <smlt.h>
#include <smlt_barrier.h>

extern struct smlt_context* context;

__thread struct sk_measurement mes;

/* data needed by each thread */
typedef struct threaddata
{
	unsigned int thread_id;			 // 4 byte
	volatile unsigned int ack;	     // 8 byte
	unsigned short int tag;		 //10 byte
	sharedMemory_t* shm;				 // 18 byte
	int bcastID;						 // 22 byte
	int core_id;						 // 26 byte
	unsigned long long g_timerfreq;	 // 34 byte
	UINT64_T * rdtsc_synchro;			 // 42 byte
	int rdtsc_latency;					 // 46 byte
	int m;						//50
	int rounds;					//54
	int naccesses;						 // 58 byte
	int *accesses;						 //66
    int num_threads;                    // 70
    bool fill;                          // 71
	unsigned char padding[57];  // 128 byte
} threaddata_t;

typedef struct timer{
	HRT_TIMESTAMP_T t1;
	HRT_TIMESTAMP_T t2;
	UINT64_T elapsed_ticks;
} my_timer_t;

void initThreads(int num_threads, threaddata_t * threaddata, sharedMemory_t * shm, 
                 unsigned long long g_timerfreq, UINT64_T * rdtsc_synchro, 
                 int rdtsc_latency, int naccesses, int *accesses, int m, 
                 int rounds, uint32_t* cores, bool fill){
	for(int i=0; i< num_threads; i++){
		threaddata[i].thread_id = i;
		threaddata[i].ack = 0;
		threaddata[i].tag = 2;
		threaddata[i].shm = shm;
		threaddata[i].bcastID = 0;
		threaddata[i].core_id = cores[i];//*4;
		threaddata[i].g_timerfreq = g_timerfreq;
		threaddata[i].rdtsc_synchro = rdtsc_synchro;
		threaddata[i].rdtsc_latency = rdtsc_latency;
		threaddata[i].naccesses = naccesses;
		threaddata[i].accesses = accesses;
		threaddata[i].m = m;
		threaddata[i].rounds = rounds;
        threaddata[i].fill = fill;
        threaddata[i].num_threads = num_threads;
	}
}


void* function_thread (void * arg_threaddata){
	my_timer_t timer;
	threaddata_t * threaddata = ((threaddata_t*)arg_threaddata);
	unsigned int thread_id = threaddata->thread_id;

	int i,n,j,m;

	double tmp_usecs;

	int ell;
	HRT_TIMESTAMP_T actual;
	UINT64_T ticks;

    char outname[512];
	//set affinity
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(threaddata->core_id,&mask);
	pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t),&mask);

    if (threaddata->fill) {
        sprintf(outname, "barriers_dis_fill%d", threaddata->num_threads);
    } else {
        sprintf(outname, "barriers_dis_rr%d", threaddata->num_threads);
    }

    uint64_t *buf = (uint64_t*) malloc(sizeof(uint64_t)*NITERS);
    sk_m_init(&mes, NITERS, outname, buf);
	threaddata->ack = 1;

	while(threaddata->ack);
       

	int index_rdtsc=0;
	for (n=0; n<NITERS; n++){
		//TSC syncrho
		HRT_GET_TIMESTAMP(actual);
		ticks = (((( UINT64_T ) actual.h) << 32) | actual.l);
		ell =  HRT_GET_USEC(threaddata->rdtsc_synchro[index_rdtsc]-ticks, threaddata->g_timerfreq);
		while(ticks<threaddata->rdtsc_synchro[index_rdtsc]){
			HRT_GET_TIMESTAMP(actual);
			ticks = (((( UINT64_T ) actual.h) << 32) | actual.l);
		}

		HRT_GET_TIMESTAMP (timer.t1);
 	    sk_m_restart_tsc(&mes);
		barrier(threaddata->num_threads, threaddata->shm,threaddata->m,threaddata->rounds,
                thread_id,&(threaddata->tag));
        sk_m_add(&mes);

		HRT_GET_TIMESTAMP (timer.t2);
		HRT_GET_ELAPSED_TICKS(timer.t1,timer.t2, &(timer.elapsed_ticks));
		timer.elapsed_ticks -= threaddata->rdtsc_latency;
		tmp_usecs = HRT_GET_USEC(timer.elapsed_ticks, threaddata->g_timerfreq);
		index_rdtsc++;
		
	}
    sk_m_print(&mes);
}


void* function_thread_smlt(void * arg_threaddata){
	my_timer_t timer;
	threaddata_t * threaddata = ((threaddata_t*)arg_threaddata);
	unsigned int thread_id = threaddata->thread_id;

	int i,n,j,m;

	double tmp_usecs;

	int ell;
	HRT_TIMESTAMP_T actual;
	UINT64_T ticks;

    char outname[512];
	//set affinity
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(threaddata->core_id,&mask);
	pthread_setaffinity_np(pthread_self(),sizeof(cpu_set_t),&mask);

    if (threaddata->fill) {
        sprintf(outname, "barriers_smlt_fill%d", threaddata->num_threads);
    } else {
        sprintf(outname, "barriers_smlt_rr%d", threaddata->num_threads);
    }

    uint64_t *buf = (uint64_t*) malloc(sizeof(uint64_t)*NITERS);
    sk_m_init(&mes, NITERS, outname, buf);
	threaddata->ack = 1;

	while(threaddata->ack);
       

	int index_rdtsc=0;
	for (n=0; n<NITERS; n++){
		//TSC syncrho
		HRT_GET_TIMESTAMP(actual);
		ticks = (((( UINT64_T ) actual.h) << 32) | actual.l);
		ell =  HRT_GET_USEC(threaddata->rdtsc_synchro[index_rdtsc]-ticks, 
                            threaddata->g_timerfreq);
		while(ticks<threaddata->rdtsc_synchro[index_rdtsc]){
			HRT_GET_TIMESTAMP(actual);
			ticks = (((( UINT64_T ) actual.h) << 32) | actual.l);
		}

 	    sk_m_restart_tsc(&mes);
        smlt_barrier_wait(context);
        sk_m_add(&mes);

		index_rdtsc++;
		
	}
    sk_m_print(&mes);
}




#endif /* THREADS_H_ */
