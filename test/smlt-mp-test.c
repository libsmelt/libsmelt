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
#include <smlt_node.h>


#define NUM_THREADS 4

unsigned num_runs = 10000000;

void* worker(void* a)
{
    uint64_t my_id = (uint64_t) a;
    
    struct smlt_msg* msg = smlt_message_alloc(56);
    for (int i = 0; i < NUM_THREADS; i++) {
        // If its my turn send to all nodes
        if (i == my_id) {
            for (int z = 0; z < NUM_THREADS; z++) {
                if (z == my_id) {
                    continue;
                }

                for (int j = 0; j < num_runs; j++) {
                    //printf("Node %ld: Sending to %d\n", my_id, z);
                    smlt_send(z, msg);
                    smlt_recv(z, msg);
                }
            }
        } else {
            for (int j = 0; j < num_runs; j++) {
                smlt_recv(i, msg);
                //printf("Node %ld: Recving from %d \n", my_id, i);
                smlt_send(i, msg);
            }
        }

    }

    return NULL;
}

int main(int argc, char **argv)
{
    if (argc>1) {
        num_runs = (unsigned) atoi(argv[1]);
    }
    printf("num_runs = %d (from first argument)\n", num_runs);


    errval_t err;
    err = smlt_init(NUM_THREADS, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }
    struct smlt_node *node;


    // Create
    for (uint64_t i=0; i<NUM_THREADS; i++) {
       node = smlt_get_node_by_id(i);
       err = smlt_node_start(node, worker, (void*) i);
       if (smlt_err_is_fail(err)) {
           // XXX
       }
    }

    // Join
    for (int i=0; i<NUM_THREADS; i++) {
        node = smlt_get_node_by_id(i);
        smlt_node_join(node);
    }

}
