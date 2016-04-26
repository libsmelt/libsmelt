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
#include <smlt.h>
#include <smlt_topology.h>
#include <smlt_generator.h>

#define NUM_THREADS 8

static uint16_t model[64] = { 0, 1, 2, 0, 0, 0, 0, 0,
                             99, 0, 0, 1, 2, 0, 0, 0,
                             99, 0, 0, 0, 0, 1, 2, 3,
                              0,99, 0, 0, 0, 0, 0, 0,
                              0,99, 0, 0, 0, 0, 0, 0,
                              0, 0, 99, 0, 0, 0, 0, 0,
                              0, 0, 99, 0, 0, 0, 0, 0,
                              0, 0, 99, 0, 0, 0, 0, 0};
static uint32_t leafs[NUM_THREADS] = {3,4,5,6,7,0,0,0};
static uint32_t cores[NUM_THREADS];


static uint16_t model2[64] = { 0, 70, 70, 70, 1, 0, 0, 0,
                             50, 0, 0, 1, 2, 0, 0, 0,
                             50, 0, 0, 0, 0, 1, 2, 3,
                             50, 0, 0, 0, 0, 0, 0, 0,
                             99, 0, 0, 0, 0, 71, 71, 71,
                              0, 0, 0, 0, 0, 51, 0, 0,
                              0, 0, 0, 0, 0, 51, 0, 0,
                              0, 0, 0, 0, 0, 51, 0, 0};


static const char *name = "binary_tree";
int main(int argc, char **argv)
{
    errval_t err;
    err = smlt_init(4, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    struct smlt_topology *topo = NULL;
    printf("Creating binary tree \n");
    smlt_topology_create(NULL, name, &topo);

    struct smlt_topology *topo2 = NULL;
    struct smlt_generated_model *m = NULL;
    m = (struct smlt_generated_model*) malloc(sizeof(struct smlt_generated_model));
    m->model = model;
    m->leafs = leafs;
    m->root = 0;
    m->ncores = NUM_THREADS;
    m->len = NUM_THREADS;
    printf("Creating other tree \n");
    smlt_topology_create(m, name, &topo2);


    printf("Creating shared memory based tree \n");
    m->model = model2;
    m->leafs = leafs;
    m->root = 0;
    m->ncores = NUM_THREADS;
    m->len = NUM_THREADS;
    struct smlt_topology *topo3 = NULL;
    name = "hybrid";
    smlt_topology_create(m, name, &topo3);

    printf("Creating adaptivetree \n");
    struct smlt_generated_model *m2 = NULL;
    name = "adaptivetree";
    for (int i = 0; i < NUM_THREADS; i++) {
        cores[i] = i;
    }

    smlt_generate_model(cores, NUM_THREADS, name, &m2);
    smlt_topology_create(m2, name, &topo2);
}
