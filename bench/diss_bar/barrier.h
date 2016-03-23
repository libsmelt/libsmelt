/*
 * bcasttree.h
 *
 *  Created on: 5 Dec, 2012
 *      Author: sabela
 */

#ifndef BARRIER_H_
#define BARRIER_H_

#include "sharedmemory.h"


#if (defined DISSEMINATION_2) 

void barrier(int nthreads, sharedMemory_t * shm, int m, int rounds, int my_id, 
             unsigned short int * tag){

//tag is increased by number of rounds per barrier call

    unsigned short int op_tag = *tag;
    unsigned short int index_tag = op_tag % MAXOPS;
	unsigned short int actualFlag;
	if(*tag<10000)       	(*tag)+=rounds; //to the content of my tag, i am the only one accessing 
	else (*tag) = 2;
	int r,i,peer,step;


	notification_t * notification =  &(shm->notification[index_tag]);
	notificationLine_t * notificationLines = shm->notification[index_tag].notifications[my_id].notificationLines;
	//	printf("STARTING BARRIER: %d, m = %d, rounds = %d, op_tag = %d\n", my_id, m, rounds,op_tag);fflush(stdout);
//	while(!__sync_bool_compare_and_swap(&(notificationLine->flag),0,op_tag));//the while is not necessary, is my line and I can't be in two barriers, they're blocking
	actualFlag=op_tag;
	step = 1;
	for (r =0; r<rounds; r++){
		notificationLines[r].flag=actualFlag;; //Im the only one writing
			
		
		for(i=1; i<=m; i++){
			peer = (my_id - i*step); 
			while(peer<0) peer = nthreads+peer;
			//printf("my_id = %d, round = %d, peer = %d\n",my_id,r,peer);fflush(stdout); 
			while(notification->notifications[peer].notificationLines[r].flag<actualFlag);
			//printf("my_id = %d, round = %d, peer = %d, its notification line is %d\n",my_id,r,peer,notification->notificationLines[peer].notification);fflush(stdout); 
		}
		step = step * (m+1);
		actualFlag++;
		//while(!__sync_bool_compare_and_swap(&(notificationLine->notification),m,0));
		 //printf("my_id = %d, round = %d FINISHED\n", my_id,r);fflush(stdout);
	}
}



#endif //DISSEMINATION_2

#ifdef DISSEMINATION_NAIVE

void barrier(int nthreads, sharedMemory_t * shm, int m, int rounds, int my_id, 
             unsigned short int * tag)
{
    unsigned short int op_tag = *tag;
    unsigned short int index_tag = op_tag % MAXOPS;
	unsigned short int actualFlag;
	if(*tag<10000)       	(*tag)+=rounds; //to the content of my tag, i am the only one accessing 
	else (*tag) = 2;
	int r,i,peer,step;


	notification_t * notification =  &(shm->notification[index_tag]);
	notificationLine_t * notificationLines = shm->notification[index_tag].notifications[my_id].notificationLines;
	//	printf("STARTING BARRIER: %d, m = %d, rounds = %d, op_tag = %d\n", my_id, m, rounds,op_tag);fflush(stdout);
//	while(!__sync_bool_compare_and_swap(&(notificationLine->flag),0,op_tag));//the while is not necessary, is my line and I can't be in two barriers, they're blocking
	actualFlag=op_tag;
	step = 1;
	for (r =0; r<rounds; r++){
		notificationLines[0].flag=actualFlag;; //Im the only one writing
		for(i=1; i<=m; i++){
			peer = (my_id - i*step); 
			while(peer<0) peer = nthreads+peer;
			//printf("my_id = %d, round = %d, peer = %d\n",my_id,r,peer);fflush(stdout); 
			while(notification->notifications[peer].notificationLines[0].flag<actualFlag);
			//printf("my_id = %d, round = %d, peer = %d, its notification line is %d\n",my_id,r,peer,notification->notificationLines[peer].notification);fflush(stdout); 
		}
		step = step * (m+1);
		actualFlag++;
		//while(!__sync_bool_compare_and_swap(&(notificationLine->notification),m,0));
		 //printf("my_id = %d, round = %d FINISHED\n", my_id,r);fflush(stdout);
	}
}



#endif //DISSEMINATION_NAIVE



#endif /* BARRIER_H_ */










