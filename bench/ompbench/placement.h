/*
 * Copyright (c) 2013-2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#include <stdint.h>
#include <numa.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
//Core Placement
uint32_t* placement(uint32_t n, int do_fill)
{
    uint32_t* result = (uint32_t*) malloc(sizeof(uint32_t)*n);
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
            int cores_per_node = n/numa_nodes;
            int rest = n - (cores_per_node*numa_nodes);
            int taken_per_node = 0;        

            fprintf(stderr, "Cores per node %d \n", cores_per_node);
            fprintf(stderr, "rest %d \n", rest);
            for (int i = 0; i < numa_nodes; i++) {
                for (int j = 0; j < num_cores; j++) {
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
