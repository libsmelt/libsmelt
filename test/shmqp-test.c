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

#define _GNU_SOURCE
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
#include <backends/shm/shm_qp.h>

#define NUM_RUNS 10000000

struct shm_qp qp[2];

void* thr_worker1(void* arg)
{
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(0, &cpu_mask);
    uintptr_t r[8];
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);    

    int num_wrong = 0;
    for (int i = 0; i < NUM_RUNS; i++) {

        shm_q_send(&qp[0].src, i, 0, 0, 0, 0, 0,0);
        shm_q_recv(&qp[1].dst, &r[0], &r[1], &r[2],
                   &r[3], &r[4], &r[5], &r[6]);
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
        shm_q_recv(&qp[0].dst, &r[0], &r[1], &r[2],
                   &r[3], &r[4], &r[5], &r[6]);
        shm_q_send(&qp[1].src, i, 0, 0, 0, 0, 0,0);
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
    qp[0] = *shm_queuepair_create(0,1);

    qp[1] = *shm_queuepair_create(1,0);

    pthread_t *tids = (pthread_t*) malloc(sizeof(pthread_t)*2);
    pthread_create(&tids[0], NULL, thr_worker1, (void*) 0);
    pthread_create(&tids[1], NULL, thr_worker2, (void*) 1);

    for (uint64_t i = 0; i < (uint64_t) 2; i++) {
        pthread_join(tids[i], NULL);
    }

}

