/*
 * Copyright (c) 2013-2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
/*
 *
 */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <smlt.h>
#include <smlt_broadcast.h>
#include <smlt_reduction.h>
#include <smlt_topology.h>
#include <smlt_channel.h>
#include <smlt_context.h>
#include <smlt_generator.h>
#include <smlt_barrier.h>
#include "platforms/measurement_framework.h"

#include "diss_bar/mcs.h"

__thread struct sk_measurement m;
__thread struct sk_measurement m2;

unsigned num_threads;
struct smlt_context* ctx;
#define NUM_RUNS 10000
#define NUM_RESULTS 100
#define ADAPTIVETREE "adaptivetree-nomm-shuffle-sort"

mcs_barrier_t mcs_b;

static void* mcs_barrier(void* a)
{
    coreid_t tid = smlt_node_get_id();

    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    assert (buf!=NULL);
    sk_m_init(&m, NUM_RESULTS, "barriers_mcs-barrier", buf);

    for (unsigned j=0; j<NUM_RESULTS; j++) {
        sk_m_restart_tsc(&m);
        for (unsigned i=0; i<NUM_RUNS; i++) {

            mcs_barrier_wait(&mcs_b, tid);
        }

        sk_m_add(&m);
    }
    sk_m_print(&m);

    return NULL;
}

static void* barrier(void* a)
{
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    assert (buf!=NULL);
    sk_m_init(&m, NUM_RESULTS, "barriers_sync-barrier", buf);

    for (unsigned j=0; j<NUM_RESULTS; j++) {
        sk_m_restart_tsc(&m);
        for (int epoch=0; epoch<NUM_RUNS; epoch++) {
            smlt_barrier_wait(ctx);
        }
        sk_m_add(&m);
    }
    sk_m_print(&m);

    return NULL;
}

#define NUM_EXP 2

#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a > _b ? _a : _b; })

int main(int argc, char **argv)
{
    errval_t err;

    unsigned nthreads = sysconf(_SC_NPROCESSORS_CONF);
    num_threads = nthreads;

    if (argc>1) {
        num_threads = atoi(argv[1]);
        fprintf(stderr, "Setting number of threads to %d\n", num_threads);
    }

    mcs_barrier_init(&mcs_b, num_threads);

    typedef void* (worker_func_t)(void*);
    worker_func_t* workers[NUM_EXP] = {
        &barrier,
        &mcs_barrier,
    };

    const char *labels[NUM_EXP] = {
        "Smelt barrier",
        "MCS barrier",
    };


    err = smlt_init(num_threads, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }


    uint32_t*  cores = (uint32_t*) malloc(sizeof(uint32_t)*num_threads);
    assert (cores!=NULL);

    for (int i = 0; i < num_threads; i++) {
        cores[i] = i;
    }

    struct smlt_generated_model* model = NULL;
    err = smlt_generate_model(cores, num_threads, ADAPTIVETREE, &model);

    if (smlt_err_is_fail(err)) {
        printf("Failed to generated model, aborting\n");
        return 1;
    }

    struct smlt_topology *topo = NULL;
    smlt_topology_create(model, ADAPTIVETREE, &topo);

    err = smlt_context_create(topo, &ctx);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE CONTEXT !\n");
        return 1;
    }


    for (int j=0; j<NUM_EXP; j++) {
        printf("----------------------------------------\n");
        printf("Executing experiment %d - %s\n", (j+1), labels[j]);
        printf("----------------------------------------\n");

        // Yield to reduce the risk of getting de-scheduled later
        sched_yield();

        struct smlt_node *node;
        for (uint64_t i = 0; i < num_threads; i++) {
            node = smlt_get_node_by_id(i);
            err = smlt_node_start(node, workers[j], (void*) (i));
            if (smlt_err_is_fail(err)) {
                printf("Staring node failed \n");
            }
        }

        for (int i=0; i < num_threads; i++) {
            node = smlt_get_node_by_id(i);
            smlt_node_join(node);
        }   
    }
}
