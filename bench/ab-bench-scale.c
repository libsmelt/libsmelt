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
#include <smlt_debug.h>
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
size_t gl_step;

struct smlt_context *context = NULL;

static pthread_barrier_t bar;
static struct smlt_channel** chan;
static struct smlt_topology *active_topo;

__thread struct sk_measurement m;
__thread struct sk_measurement m2;

#define TOPO_NAME(x,y) sprintf(x, "%s_%s%zu-%zu", y, \
                               smlt_topology_get_name(active_topo), \
                               gl_num_threads, gl_step);

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))

/**
 * @brief Returns an array of cores of size req_cores choosen
 *     round-robin from NUMA nodes in batches of req_step.
 *
 * @param req_step The step with - how many cores should be picked
 *     from each NUMA node in each iteration. Use a negative value
 *     for a "fill"-strategy, where NUMA nodes are completely filled
 *     before moving on to the next one.
 */
void placement(size_t req_cores, size_t req_step, coreid_t *cores)
{
    size_t max_node = numa_max_node();
    size_t num_cores = numa_num_configured_cpus();
    size_t cores_per_node = num_cores/(max_node+1);

    printf("req_cores: %zu\n", req_cores);
    printf("req_step: %zu\n", req_step);
    printf("cores / NUMA node: %zu\n", cores_per_node);
    printf("max_node: %zu\n", max_node);

    size_t num_selected = 0;
    size_t curr_numa_idx = 0;

    // How many nodes to choose from each NUMA node
    size_t choose_per_node[max_node+1];
    memset(choose_per_node, 0, sizeof(size_t)*(max_node+1));

    // Step 1:
    // Figure out how many cores to choose from each node

    while (num_selected<req_cores) {

        // Determine number of cores of that node

        // How many cores should be choosen in this step?
        // At max req_step
        size_t num_choose = min(min(req_step, req_cores-num_selected),
                                cores_per_node);

        // Increment counter indicating how many to choose from this node
        choose_per_node[curr_numa_idx] += num_choose;
        num_selected += num_choose;

        // Move on to the next NUMA node
        curr_numa_idx = (curr_numa_idx + 1) % (max_node+1);
    }

    // Step 2:
    // Get the cores from each NUMA node
    //
    // hyperthreads? -> should have higher core IDs, and hence picked in
    // the end.

    struct bitmask *mask = numa_allocate_cpumask();

    size_t idx = 0;

    for (size_t i=0; i<=max_node; i++) {

        dbg_printf("node %2zu choosing %2zu\n", i, choose_per_node[i]);

        // Determine which cores are on node i
        numa_node_to_cpus(i, mask);

        size_t choosen = 0;

        for (coreid_t p=0; p<num_cores && choosen<choose_per_node[i]; p++) {

            // Is processor p on node i
            if (numa_bitmask_isbitset(mask, p)) {

                cores[idx++] = p;
                choosen++;

                dbg_printf("Choosing %" PRIuCOREID "\n", p);
            }
        }
    }

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

    // Full mesh of channels
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
        for (size_t num_threads = 2; num_threads < total+1; num_threads++) {

            coreid_t cores[num_threads];

            // Make available globally
            gl_num_threads = num_threads;
            gl_cores = cores;

            placement(num_threads, -1, cores);

            printf("Cores (%zu threads): ", num_threads);
            for (size_t dbg=0; dbg<num_threads; dbg++) {
                printf(" %" PRIuCOREID, cores[dbg]);
            }
            printf("\n");

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
