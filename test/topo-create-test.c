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
#include <smlt.h>
#include <smlt_topology.h>
#include <smlt_generator.h>

#define NUM_THREADS 12

static uint16_t model[64] = { 0, 1, 2, 0, 0, 0, 0, 0,
                             99, 0, 0, 1, 2, 0, 0, 0,
                             99, 0, 0, 0, 0, 1, 2, 3,
                              0,99, 0, 0, 0, 0, 0, 0,
                              0,99, 0, 0, 0, 0, 0, 0,
                              0, 0, 99, 0, 0, 0, 0, 0,
                              0, 0, 99, 0, 0, 0, 0, 0,
                              0, 0, 99, 0, 0, 0, 0, 0};
static uint32_t leafs[8] = {1,2,3,4,5,6,7,0};
static uint32_t last_node = 7; 


static const char *name = "binary_tree";
int main(int argc, char **argv)
{
    errval_t err;
    err = smlt_init(NUM_THREADS, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    struct smlt_topology *topo = NULL;
    printf("Creating binary tree \n");
    smlt_topology_create(NULL, name, topo);

    struct smlt_topology *topo2 = NULL;
    struct smlt_generated_model *m = NULL;
    m = (struct smlt_generated_model*) malloc(sizeof(struct smlt_generated_model));
    m->model = model;
    m->leafs = leafs;
    m->last_node = last_node;
    m->ncores = 8;
    printf("Creating other tree \n");
    smlt_topology_create(m, name, topo2);  
}
