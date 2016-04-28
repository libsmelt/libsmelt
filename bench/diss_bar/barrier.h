/*
 * bcasttree.h
 *
 *  Created on: 5 Dec, 2012
 *      Author: sabela
 */

#ifndef BARRIER_H_
#define BARRIER_H_

#include "sharedmemory.h"
#include <limits.h>

#if (defined DISSEMINATION_2)

void barrier(int nthreads, sharedMemory_t * shm, int m, int rounds, int my_id,
             unsigned int * tag){

//tag is increased by number of rounds per barrier call

    unsigned  int op_tag = *tag;
    unsigned  int index_tag = op_tag % MAXOPS;
	unsigned int actualFlag;
	int r,i,peer,step;

    if(*tag < INT_MAX) {
        (*tag)+=rounds;
    } else {
        (*tag) = 2;
    }
    
	notification_t * notification =  &(shm->notification[index_tag]);
	notificationLine_t * notificationLines = 
                shm->notification[index_tag].notifications[my_id].notificationLines;
	actualFlag=op_tag;
	step = 1;
	for (r =0; r<rounds; r++){
		notificationLines[r].flag=actualFlag;; //Im the only one writing


		for(i=1; i<=m; i++){
			peer = (my_id - i*step);
			while(peer<0) peer = nthreads+peer;

			while(notification->notifications[peer].notificationLines[r].flag<actualFlag);
		}

		step = step * (m+1);
		actualFlag++;
	}
}



#endif //DISSEMINATION_2

#ifdef DISSEMINATION_NAIVE

void barrier(int nthreads, sharedMemory_t * shm, int m, int rounds, int my_id,
             unsigned int * tag)
{
    unsigned  int op_tag = *tag;
    unsigned int index_tag = op_tag % MAXOPS;
	unsigned int actualFlag;
	int r,i,peer,step;

    if(*tag < INT_MAX) {
        (*tag)+=rounds;
    } else {
        (*tag) = 2;
    }

	notification_t * notification =  &(shm->notification[index_tag]);
	notificationLine_t * notificationLines = 
            shm->notification[index_tag].notifications[my_id].notificationLines;
	actualFlag=op_tag;
	step = 1;
	for (r =0; r<rounds; r++){
		notificationLines[0].flag=actualFlag;; //Im the only one writing
		for(i=1; i<=m; i++){
			peer = (my_id - i*step);

			while(peer<0) peer = nthreads+peer;

			while(notification->notifications[peer].notificationLines[0].flag<actualFlag);
		}
		step = step * (m+1);
		actualFlag++;
	}
}



#endif //DISSEMINATION_NAIVE



#endif /* BARRIER_H_ */
