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

#define NUM_RUNS 100

struct smlt_dissem_barrier* bar;

void* worker1(void* arg)
{
    uint64_t id = (uint64_t) arg;
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(id, &cpu_mask);
    //uintptr_t r[8];
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);    

    for (uint64_t i = 0; i < NUM_RUNS; i++) {
        if (smlt_node_get_id() == 0) {
            sleep(1);
        }
        printf("Node %d \n", smlt_node_get_id());
        smlt_dissem_barrier_wait(bar);
    }
    return 0;
}

#define NUM_THREADS 3

int main(int argc, char ** argv)
{
    errval_t err;
    
    err = smlt_init(4, true);
    if (smlt_err_is_fail(err)) {
        printf("SMLT init failed \n");
    }        

    uint32_t cores[NUM_THREADS] = {0,1,2};
    err = smlt_dissem_barrier_init(cores, NUM_THREADS, &bar);
    if (smlt_err_is_fail(err)) {
        printf("SMLT init failed \n");
    }        


    struct smlt_node* node;
    for (uint64_t i=0; i<NUM_THREADS; i++) {
       node = smlt_get_node_by_id(cores[i]);
       err = smlt_node_start(node, worker1, (void*)(uint64_t) cores[i]);
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

