/*
 * sharedmemory.h
 *
 *  Created on: 4 Dec, 2012
 *      Author: sabela
 */

#ifndef SHAREDMEMORY_H_
#define SHAREDMEMORY_H_


#include "common.h"


//shared structures for the management of the shared memory
//

//notification system
typedef struct __attribute__((__packed__)) notificationLine{
	unsigned int flag; //4 bytes
	unsigned char padding[60];
} notificationLine_t;


typedef struct __attribute__((__packed__)) notificationOne{
	notificationLine_t * notificationLines;
	unsigned char padding [56];
}notificationOne_t;

typedef struct __attribute__((__packed__)) notification{
	notificationOne_t* notifications;
}notification_t;


void initnotification(int num_threads, notification_t *notification, int rounds){
    for (int i = 0; i < MAXOPS; i++) {
    	void *addr;
        if(posix_memalign(&addr, ALIGNMENT, sizeof(notificationOne_t)*num_threads)) {
            //returns 0 on success
            fprintf(stderr, "Error allocation of flag structures failed\n"); 
            fflush(stderr);
            exit(127);
        } 
        notification[i].notifications = addr;
    }    

	for (int i=0; i<MAXOPS; i++){
		for (int j=0; j<num_threads; j++){
			void *addr;
			if(posix_memalign(&addr, ALIGNMENT, sizeof(notificationOne_t)*rounds)){
				//returns 0 on success
				fprintf(stderr, "Error allocation of flag structures failed\n"); fflush(stderr);
				exit(127);
			}

			notification[i].notifications[j].notificationLines = addr;

			for(int k=0; k<rounds; k++){
				notification[i].notifications[j].notificationLines[k].flag = 0;
			}
		}
	}
}

typedef struct __attribute__((__packed__))  sharedMemory{
	int rounds;
	int m;
	unsigned char padding[56];
	notification_t notification[MAXOPS];
}sharedMemory_t;


void readFile(int *m, int * rounds, int nthreads){
    (*m) = 1;//m_read;
	(*rounds) = ceil(log2(nthreads));
}
void initSharedMemory(sharedMemory_t * shm, int nthreads){
    int m,rounds; 
	readFile(&m, &rounds, nthreads);
	shm->m = m;
	shm->rounds = rounds;
    initnotification(nthreads, shm->notification,rounds);

}

      
#endif /* SHAREDMEMORY_H_ */
