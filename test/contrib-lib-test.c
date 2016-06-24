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
#include <smlt_generator.h>

#define NUM_THREADS 32

static const char *name = "adaptivetree";
static uint32_t cores[NUM_THREADS];
int main(int argc, char **argv)
{
    for (int i = 0; i < NUM_THREADS; i++) {
        cores[i] = i;
    }
    struct smlt_generated_model* model;
    smlt_generate_model(cores, NUM_THREADS, name, &model);

/*
    smlt_tree_generate(NUM_THREADS, cores, name,
                       &model, &leafs, &last_node);
*/
}
