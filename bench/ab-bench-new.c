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
#include <string.h>
#include <smlt.h>
#include <smlt_broadcast.h>
#include <smlt_reduction.h>
#include <smlt_topology.h>
#include <smlt_channel.h>
#include <smlt_context.h>
#include <smlt_generator.h>
#include <smlt_barrier.h>
#include <platforms/measurement_framework.h>

#define NUM_RUNS 100000 //50 // 10000 // Tested up to 1.000.000
#define NUM_RESULTS 1000
#define NUM_EXP 5

uint32_t num_topos = 7;
uint32_t num_threads;

struct smlt_context *context = NULL;

static pthread_barrier_t bar;
static struct smlt_channel** chan;
static struct smlt_topology *active_topo;

__thread struct sk_measurement m;
__thread struct sk_measurement m2;

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
        for (int i = 0; i < num_threads; i++) {
            if (smlt_topology_node_is_leaf(tn)) {
                num_leafs++;
            }
            tn = smlt_topology_node_next(tn);
        }

        uint32_t* ret = (uint32_t*) malloc(sizeof(uint32_t)*num_leafs);

        int index = 0;
        tn = smlt_topology_get_first_node(active_topo);
        for (int i = 0; i < num_threads; i++) {
            if (smlt_topology_node_is_leaf(tn)) {
               ret[index] = smlt_topology_node_get_id(tn);
               index++;
            }
            tn = smlt_topology_node_next(tn);
        }
        *count = num_leafs;
        return ret;
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

    if (smlt_node_get_id() == num_threads-1) {
        struct smlt_msg* msg = smlt_message_alloc(56);
        for (int i = 0; i < NUM_RUNS; i++) {
            sk_m_restart_tsc(&m);
            smlt_channel_send(&chan[0][num_threads-1], msg);

            sk_m_restart_tsc(&m2);
            smlt_channel_recv(&chan[0][num_threads-1], msg);

            sk_m_add(&m2);
            sk_m_add(&m);
        }
    }

    if (smlt_node_get_id() == 0) {
        struct smlt_msg* msg = smlt_message_alloc(56);
        for (int i = 0; i < NUM_RUNS; i++) {
            sk_m_restart_tsc(&m);
            sk_m_restart_tsc(&m2);
            smlt_channel_recv(&chan[0][num_threads-1], msg);

            sk_m_add(&m2);

            smlt_channel_send(&chan[0][num_threads-1], msg);

            sk_m_add(&m2);
        }
    }

    if ((smlt_node_get_id() == num_threads-1) ||
        (smlt_node_get_id() == 0)) {
#ifdef PRINT_SUMMARY
        sk_m_print_analysis(&m);
        sk_m_print_analysis(&m2);
#else
        sk_m_print(&m);
        sk_m_print(&m2);
#endif

    }

    return 0;
}


static void* ab(void* a)
{
    char outname[1024];
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    TOPO_NAME(outname, "ab");

    sk_m_init(&m, NUM_RESULTS, outname, buf);

    uint32_t count = 0;
    uint32_t* leafs;
    leafs = get_leafs(active_topo, &count);

    struct smlt_msg* msg = smlt_message_alloc(56);
    for (int i = 0; i < count; i++) {
        coreid_t last_node = (coreid_t) leafs[i];
        sk_m_reset(&m);

        for (int j = 0; j < NUM_RUNS; j++) {
            sk_m_restart_tsc(&m);


            if (smlt_node_get_id() == last_node) {
                smlt_channel_send(&chan[last_node][0], msg);
            }

            if (smlt_context_is_root(context)) {
                smlt_channel_recv(&chan[last_node][0], msg);
                smlt_broadcast(context, msg);
            } else {
                smlt_broadcast(context, msg);
            }
            sk_m_add(&m);
        }

        if (smlt_node_get_id() == last_node) {
#ifdef PRINT_SUMMARY
            sk_m_print_analysis(&m);
#else
            sk_m_print(&m);
#endif
        }
    }
    return 0;
}

static void* reduction(void* a)
{
    char outname[1024];
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    TOPO_NAME(outname, "reduction");

    sk_m_init(&m, NUM_RESULTS, outname, buf);

    uint32_t count = 0;
    uint32_t* leafs;
    leafs = get_leafs(active_topo, &count);

    struct smlt_msg* msg = smlt_message_alloc(56);
    for (int i = 0; i < count; i++) {
        coreid_t last_node = (coreid_t) leafs[i];
        sk_m_reset(&m);

        for (int j = 0; j < NUM_RUNS; j++) {
            sk_m_restart_tsc(&m);

            smlt_reduce(context, msg, msg, operation);


            if (smlt_context_is_root(context)) {
                smlt_channel_send(&chan[last_node][0], msg);
            } else if (smlt_node_get_id() == last_node) {
                smlt_channel_recv(&chan[last_node][0], msg);
                sk_m_add(&m);
            }
        }

        if (smlt_node_get_id() == last_node) {
#ifdef PRINT_SUMMARY
            sk_m_print_analysis(&m);
#else
            sk_m_print(&m);
#endif
        }
    }
    return 0;
}

static void* barrier(void* a)
{
    char outname[1024];
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    TOPO_NAME(outname, "barriers");

    sk_m_init(&m, NUM_RESULTS, outname, buf);

    sleep(1);
    pthread_barrier_wait(&bar);

    for (int j = 0; j < NUM_RUNS; j++) {
        sk_m_restart_tsc(&m);

        smlt_barrier_wait(context);

        if (smlt_node_get_id() == (num_threads-1)) {
            sk_m_add(&m);
        }
    }

    if (smlt_context_is_root(context) ||
        smlt_node_get_id() == (num_threads-1)) {
#ifdef PRINT_SUMMARY
        sk_m_print_analysis(&m);
#else
        sk_m_print(&m);
#endif
    }

    return 0;
}

static void* agreement(void* a)
{
    char outname[1024];
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    TOPO_NAME(outname, "agreement");

    sk_m_init(&m, NUM_RESULTS, outname, buf);

    uint32_t count = 0;
    uint32_t* leafs;
    leafs = get_leafs(active_topo, &count);

    struct smlt_msg* msg = smlt_message_alloc(56);

    smlt_nid_t root = smlt_topology_get_root_id(active_topo);

    pthread_barrier_wait(&bar);

    for (int i = 0; i < count; i++) {
        coreid_t last_node = (coreid_t) leafs[i];
        sk_m_reset(&m);

        for (int j = 0; j < NUM_RUNS; j++) {
            sk_m_restart_tsc(&m);

            if (smlt_node_get_id() == last_node) {
                smlt_channel_send(&chan[last_node][root], msg);
            }

            if (smlt_context_is_root(context)) {
                smlt_channel_recv(&chan[last_node][root], msg);
                smlt_broadcast(context, msg);
            } else {
                smlt_broadcast(context, msg);
            }

            smlt_reduce(context, msg, msg, operation);
            smlt_broadcast(context, msg);
            sk_m_add(&m);
        }

        if (smlt_node_get_id() == last_node) {
#ifdef PRINT_SUMMARY
            sk_m_print_analysis(&m);
#else
            sk_m_print(&m);
#endif
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    num_threads = sysconf(_SC_NPROCESSORS_ONLN);

    chan = (struct smlt_channel**) malloc(sizeof(struct smlt_channel*)*num_threads);
    for (int i = 0; i < num_threads; i++) {
        chan[i] = (struct smlt_channel*) malloc(sizeof(struct smlt_channel)*num_threads);
    }

    typedef void* (worker_func_t)(void*);
    worker_func_t * workers[NUM_EXP] = {
        &pingpong,
        &ab,
        &reduction,
        &agreement,
        &barrier,
    };

    const char *labels[NUM_EXP] = {
        "Ping pong",
        "Atomic Broadcast",
        "Reduction",
        "Agreement",
        "Barrier",
    };

    char *topo_names[7] = {
        "adaptivetree",
        "mst",
        "bintree",
        "cluster",
        "badtree",
        "fibonacci",
        "sequential",
    };

    pthread_barrier_init(&bar, NULL, num_threads);
    errval_t err;
    err = smlt_init(num_threads, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    // TODO to many channels
    for (int i = 0; i < num_threads; i++) {
        for (int j = 0; j < num_threads; j++) {
            struct smlt_channel* ch = &chan[i][j];
            err = smlt_channel_create(&ch, (uint32_t *)&i, (uint32_t*) &j, 1, 1);
            if (smlt_err_is_fail(err)) {
                printf("FAILED TO INITIALIZE CHANNELS !\n");
                return 1;
            }
        }
    }

    uint32_t cores[num_threads];
    for (int i = 0; i < num_threads; i++) {
        cores[i] = i;
    }

    for (int j = 0; j < num_topos; j++) {
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

            for (int j=0; j < num_threads; j++) {
                node = smlt_get_node_by_id(j);
                smlt_node_join(node);
            }
       }
    }
}
