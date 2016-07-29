/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

/**
 * \brief This benchmark is a variant of ab-bench where the rdtsc() is sent.
 *
 * The idea is to determine the real time of send and receive events on
 * hardware. This currently assumes a machine with synchronized clocks.
 *
 * XXX Need to add a tsc synchronization protocol here.
 *
 * Tested on:
 * - tomme1
 * - sbrinz1
 * - gruyere
 *
 * All of them doe not have synchronized TSCs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <smlt.h>
#include <smlt_broadcast.h>
#include <smlt_reduction.h>
#include <smlt_topology.h>
#include <smlt_channel.h>
#include <smlt_context.h>
#include <smlt_generator.h>
#include <smlt_barrier.h>
#include "smlt_debug.h"
#include <platforms/measurement_framework.h>

#ifdef PRINT_SUMMARY
#define NUM_RUNS 15000 //50 // 10000 // Tested up to 1.000.000
#define NUM_RESULTS 5000
#else
#define NUM_RUNS 1
#define NUM_RESULTS 1000
#endif

#define NUM_TOPO 1
#define NUM_EXP 1

uint32_t num_topos = NUM_TOPO;
uint32_t num_threads;

struct smlt_context *context = NULL;

static pthread_barrier_t bar;
static struct smlt_channel** chan;
static struct smlt_topology *active_topo;

__thread struct sk_measurement m;

#define TOPO_NAME(x,y) sprintf(x, "%s_%s", y, smlt_topology_get_name(active_topo));

errval_t operation(struct smlt_msg* m1, struct smlt_msg* m2)
{
    return 0;
}

static uint32_t* get_leafs(struct smlt_topology* topo, uint32_t* count)
{
        struct smlt_topology_node* tn;
        tn = smlt_topology_get_first_node(active_topo);
        int num_leafs = 0;
        for (unsigned int i = 0; i < num_threads; i++) {
            if (smlt_topology_node_is_leaf(tn)) {
                num_leafs++;
            }
            tn = smlt_topology_node_next(tn);
        }

        uint32_t* ret = (uint32_t*) malloc(sizeof(uint32_t)*num_leafs);

        int index = 0;
        tn = smlt_topology_get_first_node(active_topo);
        for (unsigned int i = 0; i < num_threads; i++) {
            if (smlt_topology_node_is_leaf(tn)) {
               ret[index] = smlt_topology_node_get_id(tn);
               index++;
            }
            tn = smlt_topology_node_next(tn);
        }
        *count = num_leafs;
        return ret;
}


static void* ab(void* a)
{
    char outname[1024];
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    TOPO_NAME(outname, "ab-rdtsc");

    sk_m_init(&m, NUM_RESULTS, outname, buf);

    uint32_t count = 0;
    uint32_t* leafs;
    leafs = get_leafs(active_topo, &count);

    struct smlt_msg* msg = smlt_message_alloc(56);

    coreid_t last_node = (coreid_t) leafs[0];

    for (int j = 0; j < NUM_RUNS; j++) {

        if (smlt_node_get_id() == last_node) {
            dbg_printf("last node sending\n");
            smlt_channel_send(&chan[last_node][0], msg);
        }

        if (smlt_context_is_root(context)) {
            // Receive the notification severing as a trigger
            smlt_channel_recv(&chan[last_node][0], msg);

            cycles_t rdtsc = bench_tsc();
            (*msg->data) = rdtsc;

            smlt_broadcast(context, msg);
            dbg_printf("root done\n");
        } else {

            // Receive
            smlt_broadcast(context, msg);
            dbg_printf("forwarding\n");

            // Store the time since receiver send the message
            cycles_t rdtsc_sender = *(msg->data);
            cycles_t rdtsc = bench_tsc() - rdtsc_sender;
            sk_m_add_value(&m, rdtsc);
        }
    }

    sk_m_print(&m);

    return 0;
}


int main(int argc, char **argv)
{
    num_threads = sysconf(_SC_NPROCESSORS_ONLN);

    chan = (struct smlt_channel**) malloc(sizeof(struct smlt_channel*)*num_threads);
    for (unsigned int i = 0; i < num_threads; i++) {
        chan[i] = (struct smlt_channel*) malloc(sizeof(struct smlt_channel)*num_threads);
    }

    typedef void* (worker_func_t)(void*);
    worker_func_t * workers[NUM_EXP] = {
        &ab,
    };

    const char *labels[NUM_EXP] = {
        "Atomic Broadcast",
    };

    const char *default_topos[NUM_TOPO] = {
        "cluster"
    };

    // Parse arguments
    // --------------------------------------------------
    const char** topo_names;
    if (argc>1) {

        topo_names = (const char**) malloc(sizeof(char*)); assert (topo_names);
        topo_names[0] = argv[1];
        num_topos = 1;
    } else {

        topo_names = default_topos;
    }


    pthread_barrier_init(&bar, NULL, num_threads);
    errval_t err;
    err = smlt_init(num_threads, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    // TODO to many channels
    for (unsigned int i = 0; i < num_threads; i++) {
        for (unsigned int j = 0; j < num_threads; j++) {
            struct smlt_channel* ch = &chan[i][j];
            err = smlt_channel_create(&ch, (uint32_t *)&i, (uint32_t*) &j, 1, 1);
            if (smlt_err_is_fail(err)) {
                printf("FAILED TO INITIALIZE CHANNELS !\n");
                return 1;
            }
        }
    }

    uint32_t cores[num_threads];
    for (unsigned int i = 0; i < num_threads; i++) {
        cores[i] = i;
    }

    for (unsigned int j = 0; j < num_topos; j++) {
        struct smlt_generated_model* model = NULL;

        err = smlt_generate_model(cores, num_threads, topo_names[j], &model);

        if (smlt_err_is_fail(err)) {
            printf("Failed to generated model, aborting\n");
            return 1;
        }

        struct smlt_topology *topo = NULL;
        smlt_topology_create(model, topo_names[j], &topo);
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
            for (uint64_t j = 0; j < num_threads; j++) {
                node = smlt_get_node_by_id(j);
                err = smlt_node_start(node, workers[i], (void*) j);
                if (smlt_err_is_fail(err)) {
                    printf("Staring node failed \n");
                }
            }

            for (unsigned int j=0; j < num_threads; j++) {
                node = smlt_get_node_by_id(j);
                smlt_node_join(node);
            }
       }
    }
}
