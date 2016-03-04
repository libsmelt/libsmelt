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
#include "tree_config.h"

#define NUM_THREADS 12

static char *name = "adaptive_tree";
static uint32_t cores[NUM_THREADS] = {0,1,2,3,4,5,6,7,8,9,10,11};
int main(int argc, char **argv)
{
    uint16_t* model;
    uint32_t* leafs;
    uint32_t last_node;
    smlt_tree_generate(NUM_THREADS, cores, name,
                       &model, &leafs, &last_node);

}
