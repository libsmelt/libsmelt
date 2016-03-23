/*
 * common.h
 *
 *  Created on: 4 Dec, 2012
 *      Author: sabela
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <malloc.h>
#include <stdio.h>
#include <pthread.h>
#include "hrtimer_x86_64.h"
#include "cache.h"
#include <sys/time.h>


#define MAXTHREADS 16
#define MAXROUNDS 4 //ceil(log2(MAXTHREADS)) Be careful. Sharedmemory.h assumes that this occupies less than 60 bytes
#define FREE 0
#define BUSY 1
#define MAXOPS 10
#define ALIGNMENT 64
#define CORE0 0

#define NITERS 1000
#define TIMEINTERVAL 0.05 //seconds
#define FILLVALUE 1

#define CACHE_MODE MODE_E
#define DISSEMINATION_2
//#define DISSEMINATION_NAIVE


int get_rdtsc_latency(){
	/** used to store Registers {R|E}AX, {R|E}BX, {R|E}CX and {R|E}DX */
	unsigned long long a,b,c,d;
	unsigned long long reg_a,reg_b,reg_c,reg_d;
	unsigned int latency=0xffffffff,i;
	double tmp;


	/*
	 * Output : EDX:EAX stop timestamp
	 *          ECX:EBX start timestamp
	 */
	for(i=0;i<100;i++)
	{
		__asm__ __volatile__(
				//start timestamp
				"rdtsc;"
				"mov %%rax,%%rbx;"
				"mov %%rdx,%%rcx;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				"rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;rdtsc;"
				//stop timestamp
				"rdtsc;"
				: "=a" (reg_a), "=b" (reg_b), "=c" (reg_c), "=d" (reg_d)
		);
		a=(reg_d<<32)+(reg_a&0xffffffffULL);
		b=(reg_c<<32)+(reg_b&0xffffffffULL);
		tmp=rint(((double)(a-b))/((double)257));
		if (tmp<latency) latency=(int)tmp;
	}
	return latency;
}

void init_time(UINT64_T * g_timerfreq, int * rdtsc_latency){
	HRT_INIT(0,*g_timerfreq);
	*rdtsc_latency = get_rdtsc_latency();
}

void generate_rdtscsynchro (UINT64_T * rdtsc_synchro, int length, UINT64_T g_timerfreq ){
	int i,j, index = 0;
	UINT64_T interval, actual;
	HRT_TIMESTAMP_T tmp;
	HRT_GET_TIMESTAMP(tmp);
	actual = (((( UINT64_T ) tmp.h) << 32) | tmp.l);
	interval = TIMEINTERVAL*g_timerfreq;
	actual = actual+interval+interval; //esperamos dos intervalos al principio
	for(j=0;j<length;j++){
		rdtsc_synchro[index] = actual+ interval*index;
		index++;
	}
	//printf("generated %d intervals\n",j);
}



int mylog2(int n) {//truncated //assumes n>0
	int logValue = 0;
	if (n==0) return -1;
	else if (n==1) return 0;
	n >>= 1;
	while (n) {//
		logValue++;
		n >>= 1;
	}
	return logValue;
}

int mypow2(int n){ //not safe (overflow and so on)
	if (n<0) n*=(-1);

	int powValue = 1;
	powValue <<=n;
	return powValue;
}


int mylogk(int n, int k) {//truncated //assumes n>0
        
        return (int) (log(n)/(log(k)));
}

int ceilmylogk(int n, int k) {//truncated //assumes n>0

        return (int) (ceil(log(n)/(log(k))));
}



#endif /* COMMON_H_ */
