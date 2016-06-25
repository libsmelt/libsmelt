/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <smlt.h>
#include <smlt_broadcast.h>
#include <smlt_reduction.h>
#include <smlt_topology.h>
#include <smlt_context.h>
#include <smlt_generator.h>
#include <pthread.h>

#define NUM_RUNS 10000000

struct smlt_context *context = NULL;

static const char *name = "binary_tree";
static pthread_barrier_t bar;


errval_t operation(struct smlt_msg* m1, struct smlt_msg* m2)
{
    return 0;
}

void* thr_worker(void* arg)
{
    pthread_barrier_wait(&bar);
    struct smlt_msg* msg = smlt_message_alloc(56);
    uintptr_t r = 0;
    uint64_t id = (uint64_t) arg;
    for(unsigned int i = 0; i < NUM_RUNS; i++) {
        if (id == 0) {
            r++;
            msg->data[0] = r;
        }
        smlt_broadcast(context, msg);
        r = msg->data[0];
        if (r != (i+1)) {
           printf("Node %ld: Test failed %ld should be %d \n",
                  id, r, i+1);
           exit(1);
        }
    }

    printf("%ld :Broadcast Finished \n", (uint64_t) arg);
    pthread_barrier_wait(&bar);
    struct smlt_msg* msg2 = smlt_message_alloc(56);
    for(int i = 0; i < NUM_RUNS; i++) {
        smlt_reduce(context, msg2, msg2, operation);
    }
    printf("%ld :Reduction Finished \n", (uint64_t) arg);
    pthread_barrier_wait(&bar);
    for(int i = 0; i < NUM_RUNS; i++) {
        smlt_reduce(context, NULL, NULL, NULL);
    }
    printf("%ld :Reduction 0 Payload Finished \n", (uint64_t) arg);
    pthread_barrier_wait(&bar);
    return 0;
}

int main(int argc, char **argv)
{
    size_t num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_barrier_init(&bar, NULL, num_threads);
    errval_t err;
    err = smlt_init(num_threads, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    struct smlt_topology *topo = NULL;
    printf("Creating binary tree \n");
    smlt_topology_create(NULL, name, &topo);

    err = smlt_context_create(topo, &context);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE CONTEXT !\n");
        return 1;
    }

    struct smlt_node *node;
    for (uint64_t i = 0; i < num_threads; i++) {
        node = smlt_get_node_by_id(i);
        err = smlt_node_start(node, thr_worker, (void*) i);
        if (smlt_err_is_fail(err)) {
            printf("Staring node failed \n");
        }
    }

    for (unsigned int i=0; i < num_threads; i++) {
        node = smlt_get_node_by_id(i);
        smlt_node_join(node);
    }
}
