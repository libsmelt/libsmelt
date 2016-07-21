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
#include <numa.h>
#include <smlt.h>
#include <smlt_broadcast.h>
#include <smlt_reduction.h>
#include <smlt_topology.h>
#include <smlt_channel.h>
#include <smlt_context.h>
#include <smlt_generator.h>
#include <smlt_barrier.h>
#include <platforms/measurement_framework.h>

//#define PRINT_SUMMARY
#ifdef PRINT_SUMMARY
#define NUM_RUNS 15000 //50 // 10000 // Tested up to 1.000.000
#define NUM_RESULTS 5000
#else
#define NUM_RUNS 10000 //50 // 10000 // Tested up to 1.000.000
#define NUM_RESULTS 1000
#endif

#define NUM_TOPO 14
#define NUM_EXP 1

// --------------------------------------------------
// Global state
//
// Is used from within functions
// --------------------------------------------------

uint32_t gl_num_topos = NUM_TOPO;
size_t gl_num_threads; // set before starting teh benchmarks
uint32_t gl_total;
coreid_t* gl_cores;


struct smlt_context *context = NULL;

static pthread_barrier_t bar;
static struct smlt_channel** chan;
static struct smlt_topology *active_topo;

__thread struct sk_measurement m;
__thread struct sk_measurement m2;

#define TOPO_NAME(x,y) sprintf(x, "%s_%s%zu", y, \
                               smlt_topology_get_name(active_topo), \
                               gl_num_threads);


static uint32_t* placement(uint32_t n, bool do_fill)
{
    uint32_t* result = (uint32_t*) malloc(sizeof(uint32_t)*n);
    uint32_t numa_nodes = numa_max_node()+1;
    uint32_t num_cores = numa_num_configured_cpus();
    struct bitmask* nodes[numa_nodes];

    for (unsigned i = 0; i < numa_nodes; i++) {
        nodes[i] = numa_allocate_cpumask();
        numa_node_to_cpus(i, nodes[i]);
    }

    unsigned num_taken = 0;
    if (numa_available() == 0) {
        if (do_fill) {
            for (unsigned i = 0; i < numa_nodes; i++) {
                for (unsigned j = 0; j < num_cores; j++) {
                    if (numa_bitmask_isbitset(nodes[i], j)) {
                        result[num_taken] = j;
                        num_taken++;
                    }

                    if (num_taken == n) {
                        return result;
                    }
                }
           }
        } else {
            int cores_per_node = n/numa_nodes;
            int rest = n - (cores_per_node*numa_nodes);
            int taken_per_node = 0;

            fprintf(stderr, "Cores per node %d \n", cores_per_node);
            fprintf(stderr, "rest %d \n", rest);
            for (unsigned i = 0; i < numa_nodes; i++) {
                for (unsigned j = 0; j < num_cores; j++) {
                    if (numa_bitmask_isbitset(nodes[i], j)) {
                        if (taken_per_node == cores_per_node) {
                            if (rest > 0) {
                                result[num_taken] = j;
                                num_taken++;
                                rest--;
                                if (num_taken == n) {
                                    return result;
                                }
                            }
                            break;
                        }
                        result[num_taken] = j;
                        num_taken++;
                        taken_per_node++;

                        if (num_taken == n) {
                            return result;
                        }
                    }
                }
                taken_per_node = 0;
            }
        }
    } else {
        printf("Libnuma not available \n");
        return NULL;
    }
    return NULL;
}

errval_t operation(struct smlt_msg* m1, struct smlt_msg* m2)
{
    return 0;
}

static uint32_t* get_leafs(struct smlt_topology* topo,
                           uint32_t* count,
                           uint32_t* cores)
{
        struct smlt_topology_node* tn;
        tn = smlt_topology_get_first_node(active_topo);
        int num_leafs = 0;
        for (unsigned i = 0; i < gl_total; i++) {
            for (unsigned j = 0; j < gl_num_threads; j++) {
                if (smlt_topology_node_is_leaf(tn) && (i == cores[j])) {
                    num_leafs++;
                }
            }
            tn = smlt_topology_node_next(tn);
        }

        uint32_t* ret = (uint32_t*) malloc(sizeof(uint32_t)*num_leafs);

        int index = 0;
        tn = smlt_topology_get_first_node(active_topo);
        for (unsigned i = 0; i < gl_total; i++) {
            for (unsigned j = 0; j < gl_num_threads; j++) {
                if (smlt_topology_node_is_leaf(tn) && (i == cores[j])) {
                    ret[index] = smlt_topology_node_get_id(tn);
                    index++;
                }
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
    TOPO_NAME(outname, "ab");

    sk_m_init(&m, NUM_RESULTS, outname, buf);

    uint32_t count = 0;
    uint32_t* leafs;
    leafs = get_leafs(active_topo, &count, gl_cores);

    struct smlt_msg* msg = smlt_message_alloc(56);
    for (unsigned i = 0; i < count; i++) {
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

int main(int argc, char **argv)
{
    size_t total = sysconf(_SC_NPROCESSORS_ONLN);
    gl_total = total;

    chan = (struct smlt_channel**) malloc(sizeof(struct smlt_channel*)*total);
    for (size_t i = 0; i < total; i++) {
        chan[i] = (struct smlt_channel*) malloc(sizeof(struct smlt_channel)*total);
    }

    typedef void* (worker_func_t)(void*);
    worker_func_t * workers[NUM_EXP] = {
        &ab,
        //&reduction,
        //&agreement,
        //&barrier,
    };

    const char *labels[NUM_EXP] = {
        "Atomic Broadcast",
        //"Reduction",
        //"Agreement",
        //"Barrier",
    };

    const char *topo_names[NUM_TOPO] = {
        "mst",
        "bintree",
        "cluster",
        "badtree",
        "fibonacci",
        //        "sequential",
        "mst-naive",
        "bintree-naive",
        "cluster-naive",
        "badtree-naive",
        "fibonacci-naive",
        //        "sequential-naive",
        "adaptivetree",
        "adaptivetree-shuffle-sort",
        "adaptivetree-nomm",
        "adaptivetree-nomm-shuffle-sort",
    };

    errval_t err;
    err = smlt_init(total, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    // TODO to many channels
    for (unsigned int i = 0; i < total; i++) {
        for (unsigned int j = 0; j < total; j++) {
            struct smlt_channel* ch = &chan[i][j];
            err = smlt_channel_create(&ch, (uint32_t *)&i, (uint32_t*) &j, 1, 1);
            if (smlt_err_is_fail(err)) {
                printf("FAILED TO INITIALIZE CHANNELS !\n");
                return 1;
            }
        }
    }

    for (int top = 0; top < NUM_TOPO; top++) {
        for (unsigned int j = 2; j < total+1; j++) {
            gl_num_threads = j;
            gl_cores = placement(num_threads, true);
            pthread_barrier_init(&bar, NULL, num_threads);
            struct smlt_generated_model* model = NULL;

            fprintf(stderr, "%s nthreads %zu \n", topo_names[top], num_threads);
            err = smlt_generate_model(cores, num_threads, topo_names[top], &model);

            if (smlt_err_is_fail(err)) {
                printf("Failed to generated model, aborting\n");
                return 1;
            }

            struct smlt_topology *topo = NULL;
            smlt_topology_create(model, topo_names[top], &topo);
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
                    node = smlt_get_node_by_id(cores[j]);
                    err = smlt_node_start(node, workers[i], (void*)(uint64_t) cores[j]);
                    if (smlt_err_is_fail(err)) {
                        printf("Staring node failed \n");
                    }
                }

                for (unsigned int j=0; j < num_threads; j++) {
                    node = smlt_get_node_by_id(cores[j]);
                    smlt_node_join(node);
                }
           }
        }
    }

    return 0;
}
