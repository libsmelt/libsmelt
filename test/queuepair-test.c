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
#include <smlt_queuepair.h>

#define NUM_RUNS 10000000             // << default number of runs
static uint64_t num_runs = NUM_RUNS;  // << can be configured via param #2

volatile uint8_t signal = 0;

void* thr_worker1(void* arg)
{
    struct smlt_qp* qp = (struct smlt_qp*) arg;;
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(0, &cpu_mask);

    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);

    struct smlt_msg* msg = smlt_message_alloc(7);

    int num_wrong = 0;
    for (uint64_t i = 0; i < num_runs; i++) {
        msg->data[0] = i;
        msg->words = 1;
        smlt_queuepair_send(qp, msg);
        signal = 1;
        msg->words = 1;
        smlt_queuepair_recv(qp, msg);
        if (msg->data[0] != i) {
            printf("wrong: %lu / %lu\n", msg->data[0], i);
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

    struct smlt_msg* msg = smlt_message_alloc(7);

    int num_wrong = 0;
    for (uint64_t i = 0; i < num_runs; i++) {
        msg->words = 1;
        // test send/receive
        smlt_queuepair_recv(qp, msg);
        if (msg->data[0] != i) {
            printf("wrong: %lu / %lu\n", msg->data[0], i);
           num_wrong++;
        }
        msg->data[0] = i;
        msg->words = 1;
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

void run(struct smlt_qp* qp1, struct smlt_qp* qp2,
         pthread_t* tids)
{
    printf("##################################################\n");
    switch (qp1->type) {
        case SMLT_QP_TYPE_UMP:
            printf("Staring QP UMP test \n");
            break;

        case SMLT_QP_TYPE_FFQ:
            printf("Staring QP FFQ test \n");
            break;

        case SMLT_QP_TYPE_SHM:
            printf("Staring QP SHM test \n");
            break;
        default:
            break;
    }
    printf("##################################################\n");
    pthread_create(&tids[0], NULL, thr_worker1, (void*) qp1);
    pthread_create(&tids[1], NULL, thr_worker2, (void*) qp2);

    for (uint64_t i = 0; i < (uint64_t) 2; i++) {
        pthread_join(tids[i], NULL);
    }

}


int main(int argc, char **argv)
{

    struct smlt_qp* qp1;
    struct smlt_qp* qp2;

    pthread_t *tids = (pthread_t*) malloc(2*sizeof(pthread_t));
    assert(tids!=NULL);

    if (argc>2) {
        // Accept number of runs as second argument, but only if first
        // argument is the backend
        num_runs = atoi(argv[2]);
    }

    if (argc > 1) {
       if (!strcmp(argv[1], "ump")) {
          smlt_queuepair_create(SMLT_QP_TYPE_UMP, &qp1,
                          &qp2, 0, 1);
          run(qp1, qp2, tids);

          smlt_queuepair_destroy(qp1);
          smlt_queuepair_destroy(qp2);
       }

       else if (!strcmp(argv[1], "ffq")) {
          smlt_queuepair_create(SMLT_QP_TYPE_FFQ, &qp1,
                          &qp2, 0, 1);
          run(qp1, qp2, tids);

          smlt_queuepair_destroy(qp1);
          smlt_queuepair_destroy(qp2);
       }

       else if (!strcmp(argv[1], "shm")) {
          smlt_queuepair_create(SMLT_QP_TYPE_SHM, &qp1,
                          &qp2, 0, 1);
          run(qp1, qp2, tids);

          smlt_queuepair_destroy(qp1);
          smlt_queuepair_destroy(qp2);
       }

       else {

           printf("Usage: %s [backend] [num_runs]\n\n", argv[0]);
           printf("backend: backend to use. One of 'ump', 'ffq' or 'shm' "
                  "(default: all of them)\n"
                  "num_runs: number of runs to be executed (default: %d)\n\n"
                  "Single argument is interpreted as backend.\n", NUM_RUNS);
           return 1;
       }
    } else {
        smlt_queuepair_create(SMLT_QP_TYPE_UMP, &qp1,
                      &qp2, 0, 1);
        run(qp1, qp2, tids);

        smlt_queuepair_destroy(qp1);
        smlt_queuepair_destroy(qp2);

        sleep(1);

        smlt_queuepair_create(SMLT_QP_TYPE_FFQ, &qp1,
                      &qp2, 0, 1);
        run(qp1, qp2, tids);

        smlt_queuepair_destroy(qp1);
        smlt_queuepair_destroy(qp2);

        sleep(1);

        smlt_queuepair_create(SMLT_QP_TYPE_SHM, &qp1,
                      &qp2, 0, 1);
        run(qp1, qp2, tids);

        smlt_queuepair_destroy(qp1);
        smlt_queuepair_destroy(qp2);
    }

    free (tids);

    return 0;
}
