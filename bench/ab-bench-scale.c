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

#define NUM_TOPO  8
#define NUM_EXP   4
#define INC_CORES 2
#define INC_STEPS 2

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

struct smlt_msg **gl_msg;

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
    // For convenience, allows to lookup 2*n for n in 0..n/2
    if (req_step==0)
        req_step=1;

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
                                cores_per_node-choose_per_node[curr_numa_idx]);

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

                dbg_printf("Choosing %" PRIuCOREID " on node %zu\n", p, i);
            }
        }
    }

    assert (idx == req_cores);

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
                if (smlt_topo_is_model_leaf(tn) && (i == cores[j])) {
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
                if (smlt_topo_is_model_leaf(tn) && (i == cores[j])) {
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

    struct smlt_msg* msg = gl_msg[smlt_node_get_id()];

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


static void* reduction(void* a)
{
    char outname[1024];
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    TOPO_NAME(outname, "reduction");

    sk_m_init(&m, NUM_RESULTS, outname, buf);

    uint32_t count = 0;
    uint32_t* leafs;
    leafs = get_leafs(active_topo, &count, gl_cores);

    struct smlt_msg* msg = smlt_message_alloc(56);
    for (unsigned int i = 0; i < count; i++) {
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
        sk_m_add(&m);
    }

#ifdef PRINT_SUMMARY
        sk_m_print_analysis(&m);
#else
        sk_m_print(&m);
#endif

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
    leafs = get_leafs(active_topo, &count, gl_cores);

    struct smlt_msg* msg = smlt_message_alloc(56);

    smlt_nid_t root = smlt_topology_get_root_id(active_topo);

    pthread_barrier_wait(&bar);

    for (unsigned int i = 0; i < count; i++) {
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
    int core_max = -1;
    if (argc>1) {
        core_max = atoi(argv[1]);
        fprintf(stderr, "Limiting number of cores to %d\n", core_max);
    }

    size_t num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    size_t total = num_cores;
    if (core_max>0 && core_max<total) {
        total = core_max;
    }
    fprintf(stderr, "Running %d threads\n", core_max);
    gl_total = total;

    // Allocate memory for Smelt messages
    gl_msg = (struct smlt_msg**) malloc(sizeof(struct smlt_msg*)*num_cores);
    COND_PANIC(gl_msg!=NULL, "Failed to allocate gl_msg");

    for (size_t i=0; i<num_cores; i++) {
        gl_msg[i] = smlt_message_alloc(56);
    }

    // Determine NUMA properties
    size_t max_node = numa_max_node();
    size_t cores_per_node = num_cores/(max_node+1);
    if (total<num_cores) {
        size_t tmp = cores_per_node;
        cores_per_node = tmp/(num_cores/total);
        fprintf(stderr, "Scaling down cores_per_node from %zu to %zu\n",
                tmp, cores_per_node);
    }

    chan = (struct smlt_channel**) malloc(sizeof(struct smlt_channel*)*num_cores);
    COND_PANIC(chan!=NULL, "Failed to allocate chan");

    for (size_t i = 0; i < num_cores; i++) {
        chan[i] = (struct smlt_channel*) malloc(sizeof(struct smlt_channel)*num_cores);
    }

    typedef void* (worker_func_t)(void*);
    worker_func_t * workers[NUM_EXP] = {
        &ab,
        &reduction,
        &agreement,
        &barrier,
    };

    const char *labels[NUM_EXP] = {
        "Atomic Broadcast",
        "Reduction",
        "Agreement",
        "Barrier",
    };

    const char *topo_names[NUM_TOPO] = {
        "mst",
        "bintree",
        "cluster",
        "badtree",
        "fibonacci",
        "sequential",
        "adaptivetree-shuffle-sort",
        "adaptivetree",
    };

    errval_t err;
    err = smlt_init(num_cores, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    // Full mesh of channels
    for (unsigned int i = 0; i < num_cores; i++) {
        for (unsigned int j = 0; j < num_cores; j++) {
            struct smlt_channel* ch = &chan[i][j];
            err = smlt_channel_create(&ch, (uint32_t *)&i, (uint32_t*) &j, 1, 1);
            if (smlt_err_is_fail(err)) {
                printf("FAILED TO INITIALIZE CHANNELS !\n");
                return 1;
            }
        }
    }

    // Foreach topology
    for (int top = 0; top < NUM_TOPO; top++) {

        // For an increasing number of threads
        for (size_t num_threads = 2; num_threads<total+1;
             num_threads+=INC_CORES) {

            // For an increasing round-robin batch size
            for (size_t step=0; step<=cores_per_node; step+=INC_STEPS) {

                coreid_t cores[num_threads];

                // Make available globally
                gl_num_threads = num_threads;
                gl_cores = cores;
                gl_step = step;

                placement(num_threads, step, cores);

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
                        assert(node!=NULL);
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
    }

    // Free space for messages
    for (size_t i=0; i<num_cores; i++) {
        smlt_message_free(gl_msg[i]);
    }

    return 0;
}
