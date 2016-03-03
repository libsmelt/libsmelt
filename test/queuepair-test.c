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
#include <smlt_queuepair.h>

#define NUM_RUNS 10000000

void* thr_worker1(void* arg)
{
    struct smlt_qp* qp = (struct smlt_qp*) arg;;
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(0, &cpu_mask);

    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);    

    struct smlt_msg* msg = smlt_message_alloc(56);

    uint64_t data[2];
    int num_wrong = 0;
    for (uint64_t i = 0; i < NUM_RUNS; i++) {
        data[0] = i; 
        smlt_message_write(msg, (void*) data, 8);
        smlt_queuepair_send(qp, msg);
        smlt_queuepair_recv(qp, msg);
        smlt_message_read(msg, (void*) data, 8);
        if (data[0] != i) {
           num_wrong++;
        }

        // Test notify
        smlt_queuepair_notify(qp);
        smlt_queuepair_recv(qp,msg);
        //smlt_queuepair_recv0(qp); // TODO does not work for UMP
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
    struct smlt_qp* qp = (struct smlt_qp*) arg;;
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(1, &cpu_mask);

    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);    

    struct smlt_msg* msg = smlt_message_alloc(56);
   
    uint64_t data[2];
    int num_wrong = 0;
    for (uint64_t i = 0; i < NUM_RUNS; i++) {
        // test send/receive
        smlt_queuepair_recv(qp, msg);
        smlt_message_read(msg, (void*) data, 8);  
        if (data[0] != i) {
           num_wrong++;
        }
        smlt_message_write(msg, (void*) data, 8);
        smlt_queuepair_send(qp, msg);
        
        // Test notify
        smlt_queuepair_recv(qp, msg);
        smlt_queuepair_notify(qp);
        //smlt_queuepair_recv0(qp); // TODO does not work for UMP
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

    struct smlt_qp* qp1 = malloc(sizeof(struct smlt_qp));
    struct smlt_qp* qp2 = malloc(sizeof(struct smlt_qp));
    smlt_queuepair_create(SMLT_QP_TYPE_FFQ, &qp1,
                          &qp2, 0, 1);
    pthread_t *tids = (pthread_t*) malloc(sizeof(pthread_t));
    printf("##################################################\n");
    printf("Starting FFQ QP test \n");
    printf("##################################################\n");
    pthread_create(&tids[0], NULL, thr_worker1, (void*) qp1);
    pthread_create(&tids[1], NULL, thr_worker2, (void*) qp2);

    for (uint64_t i = 0; i < (uint64_t) 2; i++) {
        pthread_join(tids[i], NULL);
    }

    smlt_queuepair_destroy(qp1);
    smlt_queuepair_destroy(qp2);

    sleep(1);

    smlt_queuepair_create(SMLT_QP_TYPE_UMP, &qp1,
                          &qp2, 0, 1);
    printf("##################################################\n");
    printf("Starting UMP QP test \n");
    printf("##################################################\n");
    pthread_create(&tids[0], NULL, thr_worker1, (void*) qp1);
    pthread_create(&tids[1], NULL, thr_worker2, (void*) qp2);

    for (uint64_t i = 0; i < (uint64_t) 2; i++) {
        pthread_join(tids[i], NULL);
    }

    smlt_queuepair_destroy(qp1);
    smlt_queuepair_destroy(qp2);

    sleep(1);
    smlt_queuepair_create(SMLT_QP_TYPE_SHM, &qp1,
                          &qp2, 0, 1);
    printf("##################################################\n");
    printf("Starting SHM QP test \n");
    printf("##################################################\n");
    pthread_create(&tids[0], NULL, thr_worker1, (void*) qp1);
    pthread_create(&tids[1], NULL, thr_worker2, (void*) qp2);
    for (uint64_t i = 0; i < (uint64_t) 2; i++) {
        pthread_join(tids[i], NULL);
    }

    smlt_queuepair_destroy(qp1);
    smlt_queuepair_destroy(qp2);

}

