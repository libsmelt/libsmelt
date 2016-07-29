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
#include <smlt_barrier.h>
#include <numa.h>
#include <platforms/measurement_framework.h>

#define NUM_RUNS 10000 //50 // 10000 // Tested up to 1.000.000
#define NUM_RESULTS 1000


uint32_t num_topos = 7;
uint32_t num_threads;
uint32_t active_threads;
uint32_t* active_cores;


struct smlt_context *context = NULL;
static struct smlt_topology *active_topo;
static pthread_barrier_t bar;

__thread struct sk_measurement m;
__thread struct sk_measurement m2;

#define TOPO_NAME(x,y) sprintf(x, "%s_%s", y, smlt_topology_get_name(active_topo));
/**
 * \brief get a array of cores with a ceartain placement
 */
static coreid_t* placement(uint32_t n, bool do_fill) 
{
    coreid_t* result = malloc(sizeof(coreid_t)*n);
    uint32_t numa_nodes = numa_max_node()+1;
    uint32_t num_cores = numa_num_configured_cpus();
    struct bitmask* nodes[numa_nodes];

    for (int i = 0; i < numa_nodes; i++) {
        nodes[i] = numa_allocate_cpumask();
        numa_node_to_cpus(i, nodes[i]);
    }

    int num_taken = 0;
    if (numa_available() == 0) {
        if (do_fill) {
            for (int i = 0; i < numa_nodes; i++) {
                for (int j = 0; j < num_cores; j++) {
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
            uint8_t ith_of_node = 0;
            // go through numa nodes
            for (int i = 0; i < numa_nodes; i++) {
                // go through cores and see if part of numa node
                for (int j = 0; j < num_cores; j++) {
                    // take the ith core of the node
                    if (numa_bitmask_isbitset(nodes[i], j)){
                        int index = i+ith_of_node*numa_nodes;
                        if (index < n) {
                            result[i+ith_of_node*numa_nodes] = j;
                            num_taken++;
                            ith_of_node++;
                        }
                    }
                    if (num_taken == n) {
                        return result;
                    }
                }
                ith_of_node = 0;
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
    TOPO_NAME(outname, "barriers");

    sk_m_init(&m, NUM_RESULTS, outname, buf);

    pthread_barrier_wait(&bar);

    for (int j = 0; j < NUM_RUNS; j++) {
        sk_m_restart_tsc(&m);
        smlt_barrier_wait(context);

        if (smlt_node_get_id() == (active_cores[active_threads-1])) {
            sk_m_add(&m);
        }
    }

    if (smlt_node_get_id() == 0 || 
        smlt_node_get_id() == (active_cores[active_threads-1])) {
        sk_m_print(&m);
    }

    return 0;
}

int main(int argc, char **argv)
{
    char name[100];
    num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    errval_t err;

    err = smlt_init(num_threads, true);
    if (smlt_err_is_fail(err)) {
	    printf("FAILED TO INITIALIZE !\n");
	    return 1;
    }

    for (int i = 4; i < num_threads+1; i++) {

        pthread_barrier_init(&bar, NULL, i);
        printf("Running %d cores\n", i);

        active_threads = i;
        uint32_t* cores = placement(i, false);
        for (int j = 0; j < i; j++) {
            printf("Cores[%d]=%d\n",j,cores[j]);
        }

        active_cores = cores;

        printf("Generating model\n");
        struct smlt_generated_model* model = NULL;
        err = smlt_generate_model(cores, i, NULL, &model);
        if (smlt_err_is_fail(err)) {
            exit(0);
        }

        printf("Generating topology\n");
        struct smlt_topology *topo = NULL;
        sprintf(name, "adaptivetree_rr%d",i);
        smlt_topology_create(model, name, &topo);
        active_topo = topo;

        printf("Generating context\n");
        err = smlt_context_create(topo, &context);
        if (smlt_err_is_fail(err)) {
            printf("FAILED TO INITIALIZE CONTEXT !\n");
            return 1;
        }
       
        printf("----------------------------------------\n");
        printf("Executing Barrier with %d cores rr\n", i);
        printf("----------------------------------------\n");
        
        struct smlt_node *node;
        for (uint64_t j = 0; j < i; j++) {
            node = smlt_get_node_by_id(cores[j]);
            err = smlt_node_start(node, barrier, (void*) j);
            if (smlt_err_is_fail(err)) {
                printf("Staring node failed \n");
            }   
        }

        for (int j=0; j < i; j++) {
            node = smlt_get_node_by_id(cores[j]);
            smlt_node_join(node);
        }
    }

    for (int i = 4; i < num_threads+1; i++) {

        pthread_barrier_init(&bar, NULL, i);

        active_threads = i;
        uint32_t* cores = placement(i, true);
        for (int j = 0; j < i; j++) {
            printf("Cores[%d]=%d\n",j,cores[j]);
        }

        active_cores = cores;

        struct smlt_generated_model* model = NULL;
        err = smlt_generate_model(cores, i, NULL, &model);
        if (smlt_err_is_fail(err)) {
            exit(0);
        }

        struct smlt_topology *topo = NULL;
        sprintf(name, "adaptivetree_fill%d",i);
        smlt_topology_create(model, name, &topo);
        active_topo = topo;

        err = smlt_context_create(topo, &context);
        if (smlt_err_is_fail(err)) {
            printf("FAILED TO INITIALIZE CONTEXT !\n");
            return 1;
        }
       
        printf("----------------------------------------\n");
        printf("Executing Barrier with %d cores fill\n", i);
        printf("----------------------------------------\n");
        
        struct smlt_node *node;
        for (uint64_t j = 0; j < i; j++) {
            node = smlt_get_node_by_id(cores[j]);
            err = smlt_node_start(node, barrier, (void*) j);
            if (smlt_err_is_fail(err)) {
                printf("Staring node failed \n");
            }   
        }

        for (int j=0; j < i; j++) {
            node = smlt_get_node_by_id(cores[j]);
            smlt_node_join(node);
        }
    }

}
