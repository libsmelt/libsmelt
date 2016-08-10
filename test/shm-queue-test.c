/**
 * \brief Testing Shared memory queue
 */

/*
 * Copyright (c) 2013-2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
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
#include <backends/shm/swmr.h>
#include <platforms/measurement_framework.h>

#define QUEUE_SIZE 128

//#define DEBUG
#ifdef DEBUG
#define NUM_WRITES 40
#define NUM_VALUES 10
#else
#define NUM_WRITES 1000000
#define NUM_VALUES 1000
#endif

#define LEAF 0

int sleep_time = 0;
void* shared_mem;
static const int num_readers = 3;

__thread struct sk_measurement m;
__thread cycles_t buf[NUM_WRITES];

void* thr_writer(void* arg)
{
    struct swmr_context* queue = (struct swmr_context*)
                        malloc(sizeof(struct swmr_context));
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(0, &cpu_mask);

    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);

    swmr_init_context(shared_mem, queue, num_readers, 0, 
		      false, QUEUE_SIZE);
    sk_m_init(&m, NUM_VALUES, "writer", buf);

    uint64_t rid = 1;
    while (1) {
        sk_m_restart_tsc(&m);
        swmr_send_raw(queue, rid, 0, 0, 0, 0, 0, 0);
        sk_m_add(&m);

        rid++;
        if (rid == NUM_WRITES) {
            break;
        }
    }

    sleep(1);

    printf("###################################################\n");
    printf("Writer Test finished\n");
    printf("###################################################\n");
    sk_m_print_analysis(&m);
    return 0;
}

void* thr_reader(void* arg)
{
    struct swmr_context* queue = (struct swmr_context*)
                                malloc(sizeof(struct swmr_context));
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    uint64_t core = (uint64_t) arg;
    core++;
    //core = core*4;
    CPU_SET(core, &cpu_mask);

    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);

    swmr_init_context(shared_mem, queue,num_readers, (uint64_t) arg, 
                      false, QUEUE_SIZE);
    uint64_t previous = 0;
    uint64_t num_wrong = 0;
    uintptr_t r[8];

    sk_m_init(&m, NUM_VALUES, "reader", buf);

    while(1) {
        sk_m_restart_tsc(&m);
        swmr_receive_raw(queue, &r[0], &r[1], &r[2], &r[3],
                        &r[4], &r[5], &r[6]);

        sk_m_add(&m);

        if (r[0] != (previous+1)) {
            num_wrong++;
        }

        previous++;

        if (previous == NUM_WRITES-1) {
            break;
        }
    }

    printf("###################################################\n");
    if (num_wrong) {
        printf("Reader %" PRIu64 ": Test Failed \n", ((uint64_t)arg));
    } else {
        printf("Reader %" PRIu64 ": Test Succeeded \n", ((uint64_t)arg));
    }
    printf("###################################################\n");
    sk_m_print_analysis(&m);
    return 0;
}


int main(int argc, char ** argv)
{
    shared_mem = calloc(1,64*64*2);
    pthread_t *tids = (pthread_t*) malloc((num_readers+1)*sizeof(pthread_t));
    printf("###################################################\n");
    printf("SHM test started (%d writes/reads) \n", NUM_WRITES);
    printf("###################################################\n");
    for (uint64_t i = 0; i < (uint64_t) num_readers; i++) {
        pthread_create(&tids[i], NULL, thr_reader, (void*) i);
    }
    sleep(1);

    pthread_create(&tids[num_readers], NULL, thr_writer, (void*) 0);
    for (uint64_t i = 0; i < (uint64_t) num_readers+1; i++) {
        pthread_join(tids[i], NULL);
    }

    return 0;
}
