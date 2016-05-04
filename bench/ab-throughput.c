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
#include "diss_bar/measurement_framework.h"

#include "diss_bar/mcs.h"

__thread struct sk_measurement m;
__thread struct sk_measurement m2;

unsigned num_threads;
struct smlt_context* ctx;
#define NUM_RUNS 100000 //50 // 10000 // Tested up to 1.000.000
#define NUM_RESULTS 1000

mcs_barrier_t mcs_b;

static void* mcs_barrier(void* a)
{
    coreid_t tid = smlt_node_get_id();
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    sk_m_init(&m, NUM_RESULTS, "mcs-barrier", buf);

    sk_m_restart_tsc(&m);
    for (unsigned i=0; i<NUM_RUNS; i++) {

        mcs_barrier_wait(&mcs_b, tid);
    }

    sk_m_add(&m);
    if (tid == 0) {
        sk_m_print(&m);
    }

    return NULL;
}

static void* barrier(void* a)
{
    coreid_t tid = smlt_node_get_id();
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    sk_m_init(&m, NUM_RESULTS, "sync-barrier", buf);

    sk_m_restart_tsc(&m);
    for (int epoch=0; epoch<NUM_RUNS; epoch++) {
        smlt_barrier_wait(ctx);
    }
    sk_m_add(&m);

    if (tid == 0) {
        sk_m_print(&m);
    }

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
    int tids[num_threads];

    unsigned nthreads = sysconf(_SC_NPROCESSORS_CONF);
    num_threads = nthreads;

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
    
    
    uint32_t*  cores = malloc(sizeof(uint32_t)*num_threads);

    for (int i = 0; i < num_threads; i++) {
        cores[i] = i;
    }

    struct smlt_generated_model* model = NULL;
    err = smlt_generate_model(cores, num_threads, "adaptivetree", &model);

    if (smlt_err_is_fail(err)) {
        printf("Failed to generated model, aborting\n");
        return 1;
    }

    struct smlt_topology *topo = NULL;
    smlt_topology_create(model, "adaptivetree", &topo);

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
