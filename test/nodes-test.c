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

unsigned num_runs = 50;

void* worker2(void* a)
{
    unsigned tid = *((unsigned*) a);
#if 0
    __thread_init(tid, NUM_THREADS);

    for (unsigned epoch=0; epoch<num_runs; epoch++) {

        if (tid == get_sequentializer()) {
            mp_send_ab(tid);

            // Wait for message from topo_last_node()
            mp_receive(topo_last_node());

        } else {
            mp_receive_forward(tid);

            if (get_thread_id()==topo_last_node()) {

                mp_send(get_sequentializer(), 0);
            }
        }
    }
    __thread_end();
#endif
    printf("Thread %d completed\n", tid);
    return NULL;
}



unsigned barrier_rounds[NUM_THREADS];

void* worker4(void* a)
{
#if 0
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    for (unsigned epoch=0; epoch<num_runs; epoch++) {

        mp_barrier(NULL);
        barrier_rounds[tid] = epoch;

        mp_barrier(NULL);

        // Verify that every thread is in the same round
        for (int j=0; j<NUM_THREADS; j++) {

            if (barrier_rounds[j] != epoch) {
                debug_printf("assertion fail %d %d %d\n",
                             j, barrier_rounds[j], epoch);
            }
            assert (barrier_rounds[j] == epoch);
        }

    }
    __thread_end();
#endif
    printf("All done :-)\n");


    return NULL;
}

#define NUM_EXP 2

int main(int argc, char **argv)
{
    typedef void* (worker_func_t)(void*);
    worker_func_t* workers[NUM_EXP] = {
        &worker2,
        &worker4,
    };

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


    for (int j=0; j<NUM_EXP; j++) {

        printf("----------------------------------------\n");
        printf("Executing experiment %d\n", (j+1));
        printf("----------------------------------------\n");

        // Create
        for (int i=0; i<NUM_THREADS; i++) {
           node = smlt_get_node_by_id(i);
           err = smlt_node_start(node, workers[j], NULL);
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

}
