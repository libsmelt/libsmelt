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
#include <smlt_node.h>
#include <smlt_barrier.h>
#include <platforms/measurement_framework.h>
#include <platforms/linux.h>

#define NUM_RUNS_TEST 10
#define NUM_RUNS_BENCH 10000
#define NUM_VALUES 1000

struct smlt_dissem_barrier* bar;



__thread struct sk_measurement m;
__thread cycles_t buf[NUM_VALUES];

void* worker_test(void* arg)
{
    uint64_t id = (uint64_t) arg;
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(id, &cpu_mask);
    //uintptr_t r[8];
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);    

    for (uint64_t i = 0; i < NUM_RUNS_TEST; i++) {
        if (smlt_node_get_id() == 0) {
            sleep(1);
            printf("#################################\n");
        }
        smlt_dissem_barrier_wait(bar);
        printf("Node %d \n", smlt_node_get_id());
    }
    return 0;
}


void* worker_bench(void* arg)
{
    uint64_t id = (uint64_t) arg;
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(id, &cpu_mask);
    //uintptr_t r[8];
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);    

    sk_m_init(&m, NUM_VALUES, "barrier", buf);

    for (uint64_t i = 0; i < NUM_RUNS_BENCH; i++) {
        smlt_dissem_barrier_wait(bar);
        smlt_dissem_barrier_wait(bar);
        sk_m_restart_tsc(&m);
        smlt_dissem_barrier_wait(bar);
        sk_m_add(&m);
    }
    sk_m_print_analysis(&m);
    return 0;
}


#define NUM_THREADS 4

int main(int argc, char ** argv)
{
    errval_t err;
    
    err = smlt_init(sysconf(_SC_NPROCESSORS_ONLN), true);
    if (smlt_err_is_fail(err)) {
        printf("SMLT init failed \n");
    }        

    uint32_t cores[NUM_THREADS] = {0,1,2,3};
    err = smlt_dissem_barrier_init(cores, NUM_THREADS, &bar);
    if (smlt_err_is_fail(err)) {
        printf("SMLT init failed \n");
    }        


    struct smlt_node* node;
    for (uint64_t i=0; i<NUM_THREADS; i++) {
       node = smlt_get_node_by_id(cores[i]);
       err = smlt_node_start(node, worker_test, (void*)(uint64_t) cores[i]);
       if (smlt_err_is_fail(err)) {
           // XXX
       }
    }

    for (uint64_t i=0; i<NUM_THREADS; i++) {
       node = smlt_get_node_by_id(cores[i]);
       err = smlt_node_join(node);
       if (smlt_err_is_fail(err)) {
           // XXX
       }
    }


    for (uint64_t i=0; i<NUM_THREADS; i++) {
       node = smlt_get_node_by_id(cores[i]);
       err = smlt_node_start(node, worker_bench, (void*)(uint64_t) cores[i]);
       if (smlt_err_is_fail(err)) {
           // XXX
       }
    }

    for (uint64_t i=0; i<NUM_THREADS; i++) {
       node = smlt_get_node_by_id(cores[i]);
       err = smlt_node_join(node);
       if (smlt_err_is_fail(err)) {
           // XXX
       }
    }
}

