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

#define NUM_THREADS 4

struct smlt_context *context = NULL;

static const char *name = "binary_tree";

errval_t operation(struct smlt_msg* m1, struct smlt_msg* m2) 
{
    return 0;
}

void* thr_worker(void* arg)
{
    struct smlt_msg* msg = smlt_message_alloc(56);
    uintptr_t r = 0;
    uint64_t id = (uint64_t) arg;
    for(int i = 0; i < 100; i++) {
        if (id == 0) {
            r++;
            smlt_message_write(msg, &r, 8);
        }
        smlt_broadcast(context, msg);
        smlt_message_read(msg, (void*) &r, 8);
        if (r != (i+1)) {
           printf("Tesf failed \n");
        }
    }

    printf("%ld :Before Reduce \n", (uint64_t) arg);
    smlt_reduce(context, msg, msg, operation);
    printf("%ld :Reduce \n", (uint64_t) arg);
    return 0;
}

int main(int argc, char **argv)
{
    errval_t err;
    err = smlt_init(NUM_THREADS, true);
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
    for (uint64_t i = 0; i < NUM_THREADS; i++) {
        node = smlt_get_node_by_id(i);
        err = smlt_node_start(node, thr_worker, (void*) i);
        if (smlt_err_is_fail(err)) {
            printf("Staring node failed \n");
        }
    }

    for (int i=0; i < NUM_THREADS; i++) {
        node = smlt_get_node_by_id(i);
        smlt_node_join(node);
    }   
}
