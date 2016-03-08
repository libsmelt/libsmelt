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
#include <smlt_channel.h>
#include <smlt_context.h>
#include <smlt_generator.h>
#include <platforms/measurement_framework.h>

#define NUM_THREADS 4
#define NUM_RUNS 10000 //50 // 10000 // Tested up to 1.000.000
#define NUM_RESULTS 1000
#define NUM_EXP 2

struct smlt_context *context = NULL;

static const char *name = "binary_tree";
static pthread_barrier_t bar;
static struct smlt_channel chan[NUM_THREADS][NUM_THREADS];
static struct smlt_topology *active_topo;

__thread struct sk_measurement m;
__thread struct sk_measurement m2;

#define TOPO_NAME(x,y) sprintf(x, "%s_%s", y, smlt_topology_get_name(active_topo));

errval_t operation(struct smlt_msg* m1, struct smlt_msg* m2) 
{
    return 0;
}

static void* pingpong(void* a)
{
    char outname[1024];
    char outname2[1024];
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    cycles_t *buf2 = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    TOPO_NAME(outname, "pingpong");
    TOPO_NAME(outname2, "pingpong_receive");

    sk_m_init(&m, NUM_RESULTS, outname, buf);
    sk_m_init(&m2, NUM_RESULTS, outname2, buf2);

    if (smlt_node_get_id() == NUM_THREADS-1) {
        struct smlt_msg* msg = smlt_message_alloc(56);
        for (int i = 0; i < NUM_RUNS; i++) {
            sk_m_restart_tsc(&m);
            smlt_channel_send(&chan[0][NUM_THREADS-1], msg);

            sk_m_restart_tsc(&m2);
            smlt_channel_recv(&chan[NUM_THREADS-1][0], msg);

            sk_m_add(&m2);            
            sk_m_add(&m);            
        }
    }   

    if (smlt_node_get_id() == 0) {
        struct smlt_msg* msg = smlt_message_alloc(56);
        for (int i = 0; i < NUM_RUNS; i++) {
            sk_m_restart_tsc(&m);
            sk_m_restart_tsc(&m2);
            smlt_channel_recv(&chan[0][NUM_THREADS-1], msg);

            sk_m_add(&m2);

            smlt_channel_send(&chan[NUM_THREADS-1][0], msg);

            sk_m_add(&m2);
        }
    }

    if ((smlt_node_get_id() == NUM_THREADS-1) ||
        (smlt_node_get_id() == 0)) {
        sk_m_print(&m);
        sk_m_print(&m2);

    }

    return 0;   
}



void* thr_worker(void* arg)
{
    pthread_barrier_wait(&bar);
    struct smlt_msg* msg = smlt_message_alloc(56);
    uintptr_t r = 0;
    uint64_t id = (uint64_t) arg;
    for(int i = 0; i < NUM_RUNS; i++) {
        if (id == 0) {
            r++;
            smlt_message_write(msg, &r, 8);
        }
        smlt_broadcast(context, msg);
        smlt_message_read(msg, (void*) &r, 8);
        if (r != (i+1)) {
           printf("Node %ld: Test failed %ld should be %d \n", 
                  id, r, i+1);
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
    typedef void* (worker_func_t)(void*);
    worker_func_t * workers[NUM_EXP] = {
        &pingpong,
        &thr_worker,
    };

    const char *labels[NUM_EXP] = {
        "Ping pong",
        "Worker",
    };

    pthread_barrier_init(&bar, NULL, NUM_THREADS);
    errval_t err;
    err = smlt_init(NUM_THREADS, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    // TODO to many channels
    for (int i = 0; i < NUM_THREADS; i++) {
        for (int j = 0; j < NUM_THREADS; j++) {
            struct smlt_channel* ch = &chan[i][j];
            err = smlt_channel_create(&ch, (uint32_t *)&i, (uint32_t*) &j, 1, 1);
            if (smlt_err_is_fail(err)) {
                printf("FAILED TO INITIALIZE CHANNELS !\n");
                return 1;
            }
        }
    }

    struct smlt_topology *topo = NULL;
    printf("Creating binary tree \n");
    smlt_topology_create(NULL, name, &topo);
    active_topo = topo;

    err = smlt_context_create(topo, &context);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE CONTEXT !\n");
        return 1;
    }
 
    for (int i = 0; i < NUM_EXP; i++){

        printf("----------------------------------------\n");
        printf("Executing experiment %s\n", labels[i]);
        printf("----------------------------------------\n");
        
        struct smlt_node *node;
        for (uint64_t j = 0; j < NUM_THREADS; j++) {
            node = smlt_get_node_by_id(j);
            err = smlt_node_start(node, workers[i], (void*) j);
            if (smlt_err_is_fail(err)) {
                printf("Staring node failed \n");
            }   
        }

        for (int j=0; j < NUM_THREADS; j++) {
            node = smlt_get_node_by_id(j);
            smlt_node_join(node);
        }
   }
}
