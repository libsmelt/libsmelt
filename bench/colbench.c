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
#include <smlt_topology.h>
#include <smlt_context.h>
#include <smlt_generator.h>
#include <smlt_barrier.h>
#include <smlt_broadcast.h>
#include <smlt_reduction.h>
#include <numa.h>
#include <platforms/measurement_framework.h>

#define NUM_RUNS 10000 //50 // 10000 // Tested up to 1.000.000
#define NUM_RESULTS 1000

#define NUM_EXP 3

uint32_t num_topos = 7;
uint32_t num_threads;
uint32_t active_threads;

bool use_bar = false;

struct smlt_context *context = NULL;
static struct smlt_dissem_barrier* bar;

__thread struct sk_measurement m;
__thread struct sk_measurement m2;


errval_t operation(struct smlt_msg* m1, struct smlt_msg* m2)
{
    m1->data[0] = m1->data[0] + m2->data[0];
    return 0;
}

/**
 * \brief get a array of cores with a ceartain placement
 */
static coreid_t* placement(uint32_t n, bool do_fill, bool hyper) 
{
    uint32_t* result = (uint32_t*) malloc(sizeof(uint32_t)*n);
    uint32_t numa_nodes = numa_max_node()+1;
    uint32_t num_cores = 0;
    if (hyper) {
        num_cores = numa_num_configured_cpus()/2;
    } else {
        num_cores = numa_num_configured_cpus();
    }
    struct bitmask* nodes[numa_nodes];

    for (unsigned int i = 0; i < numa_nodes; i++) {
        nodes[i] = numa_allocate_cpumask();
        numa_node_to_cpus(i, nodes[i]);
    }

    unsigned int num_taken = 0;
    if (numa_available() == 0) {
        if (do_fill) {
            for (unsigned int i = 0; i < numa_nodes; i++) {
                for (unsigned int j = 0; j < num_cores; j++) {
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

            for (unsigned int i = 0; i < numa_nodes; i++) {
                for (unsigned int j = 0; j < num_cores; j++) {
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

static void* barrier(void* a) 
{
    char outname[1024];
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    if (use_bar) {
        sprintf(outname, "barriers_smltsync_%d", num_threads);
    } else {
        sprintf(outname, "barriers_smlt_%d", num_threads);
    }

    sk_m_init(&m, NUM_RESULTS, outname, buf);

    for (int j = 0; j < NUM_RUNS; j++) {

        if (use_bar) {
            smlt_dissem_barrier_wait(bar);
            smlt_dissem_barrier_wait(bar);
        }

        sk_m_restart_tsc(&m);
        smlt_barrier_wait(context);
        sk_m_add(&m);
    }

    sk_m_print(&m);

    return 0;
}


static void* broadcast(void* a) 
{
    char outname[1024];
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    if (use_bar) {
        sprintf(outname, "ab_smltsync_%d", num_threads);
    } else {
        sprintf(outname, "ab_smlt_%d", num_threads);
    }

    sk_m_init(&m, NUM_RESULTS, outname, buf);

    struct smlt_msg* msg = smlt_message_alloc(1);

    for (int j = 0; j < NUM_RUNS; j++) {

        if (use_bar) {
            smlt_dissem_barrier_wait(bar);
            smlt_dissem_barrier_wait(bar);
        }

        sk_m_restart_tsc(&m);
        smlt_broadcast(context, msg);
        sk_m_add(&m);
    }

    sk_m_print(&m);

    return 0;
}


static void* reduction(void* a) 
{
    char outname[1024];
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    if (use_bar) {
        sprintf(outname, "reduction_smltsync_%d", num_threads);
    } else {
        sprintf(outname, "reduction_smlt_%d", num_threads);
    }


    struct smlt_msg* msg = smlt_message_alloc(56);

    sk_m_init(&m, NUM_RESULTS, outname, buf);

    for (int j = 0; j < NUM_RUNS; j++) {

        if (use_bar) {
            smlt_dissem_barrier_wait(bar);
            smlt_dissem_barrier_wait(bar);
        }

        sk_m_restart_tsc(&m);
        smlt_reduce(context, msg, msg, operation);
        sk_m_add(&m);
    }

    sk_m_print(&m);

    return 0;
}


int main(int argc, char **argv)
{
    bool hyper = false;
    if (argc == 2) {
        fprintf(stderr,"Hyperthreads Enabled \n");
        num_threads = sysconf(_SC_NPROCESSORS_ONLN)/2;
        hyper = true;
    } else {
        fprintf(stderr,"Hyperthreads Disabled \n");
        num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    }

    errval_t err;

    err = smlt_init(num_threads, true);
    if (smlt_err_is_fail(err)) {
	    printf("FAILED TO INITIALIZE !\n");
	    return 1;
    }

    uint32_t* cores = placement(num_threads, true, hyper);
    err = smlt_dissem_barrier_init(cores, num_threads,
                                   &bar);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE SYNCHRO BARRIER \n");
        exit(0);
    }

    struct smlt_generated_model* model = NULL;
    err = smlt_generate_model(cores, num_threads, NULL, &model);
    if (smlt_err_is_fail(err)) {
        exit(0);
    }

    struct smlt_topology *topo = NULL;
    smlt_topology_create(model, "adaptivetree", &topo);

    err = smlt_context_create(topo, &context);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE CONTEXT !\n");
        return 1;
    }
    
    typedef void* (worker_func_t)(void*);
    worker_func_t * workers[NUM_EXP] = {
        &broadcast,
        &reduction,
        &barrier,
    };
   
    for (int i = 0; i < NUM_EXP; i++) {      
        struct smlt_node *node;
        for (uint64_t j = 0; j < num_threads; j++) {
            node = smlt_get_node_by_id(cores[j]);
            err = smlt_node_start(node, workers[i], (void*) j);
            if (smlt_err_is_fail(err)) {
                printf("Staring node failed \n");
            }   
        }

        for (unsigned int j=0; j < num_threads; j++) {
            node = smlt_get_node_by_id(cores[j]);
            smlt_node_join(node);
        }
    }

    use_bar = true;

    for (int i = 0; i < NUM_EXP; i++) {      
        struct smlt_node *node;
        for (uint64_t j = 0; j < num_threads; j++) {
            node = smlt_get_node_by_id(cores[j]);
            err = smlt_node_start(node, workers[i], (void*) j);
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
