/**
 * \brief Testing Shared memory queue
 */

/*
 * Copyright (c) 2015, ETH Zurich and Mircosoft Corporation.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <numa.h>
#include <sched.h>
#include <smlt.h>
#include <backends/ffq/ff_queuepair.h>
#include <backends/ffq/ff_queue.h>

#define NUM_RUNS 10000

struct ff_queuepair qp[2];

void* thr_worker1(void* arg)
{
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(0, &cpu_mask);
    uintptr_t r[8];
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);    

    int num_wrong = 0;
    for (int i = 0; i < NUM_RUNS; i++) {
        ffq_enqueue_full(&(qp[0].src), i, 0, 0, 0, 0, 0,0);
        ffq_dequeue_full(&(qp[1].dst),r);

        if (r[0] != i) {
           num_wrong++;
        }
    }   

    if (!num_wrong) {
       printf("Core 0 Test Success \n");
    } else {
       printf("Core 0 Test Failed \n");
    }
    return 0;
}

void* thr_worker2(void* arg)
{
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(1, &cpu_mask);
    uintptr_t r[8];

    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);    
   
    int num_wrong = 0;
    for (int i = 0; i < NUM_RUNS; i++) {
        ffq_dequeue_full(&(qp[0].dst),r);
        ffq_enqueue_full(&(qp[1].src), i, 0, 0, 0, 0, 0,0);
        if (r[0] != i) {
           num_wrong++;
        }
    }
    
    if (!num_wrong) {
       printf("Core 1 Test Success \n");
    } else {
       printf("Core 1 Test Failed \n");
    }
    return 0;
}

int main(int argc, char ** argv)
{
    qp[0] = *ff_queuepair_create(1);

    qp[1] = *ff_queuepair_create(0);

    pthread_t *tids = (pthread_t*) malloc(sizeof(pthread_t)*2);
    pthread_create(&tids[0], NULL, thr_worker1, (void*) 0);
    pthread_create(&tids[1], NULL, thr_worker2, (void*) 1);

    for (uint64_t i = 0; i < (uint64_t) 2; i++) {
        pthread_join(tids[i], NULL);
    }

}

