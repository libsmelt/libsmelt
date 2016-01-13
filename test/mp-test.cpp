/*
 *
 */
#include <stdio.h>
#include "sync.h"
#include "topo.h"
#include "mp.h"
#include "measurement_framework.h"
#include <pthread.h>

#include "model_defs.h"

#define NUM_THREADS 32

unsigned num_runs = 50;

void* worker2(void* a)
{
    unsigned tid = *((unsigned*) a);
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
    printf("Thread %d completed\n", tid);

    __thread_end();
    return NULL;
}

void* hybrid_ab(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);


    for (unsigned i=0; i<num_runs; i++) {

        uintptr_t r = ab_forward(i, topo_last_node());
        assert (r==i);
    }

    debug_printf("hybrid ab complete\n");

    __thread_end();
    return NULL;
}

void* hybrid_reduce(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    debug_printf("Reduction complete: %d\n", sync_reduce(tid));

    __thread_end();
    return NULL;
}

unsigned barrier_rounds[NUM_THREADS];

void* worker4(void* a)
{
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

    debug_printf("All done :-)\n");

    __thread_end();
    return NULL;
}

#define NUM_EXP 1

int main(int argc, char **argv)
{
    typedef void* (worker_func_t)(void*);
    worker_func_t* workers[NUM_EXP] = {
        //        &worker1,
        // &worker2,
        &hybrid_reduce,
        // &worker4,
        //        &hybrid_ab
    };

    if (argc>1) {

        num_runs = (unsigned) atoi(argv[1]);
    }
    printf("num_runs = %d (from first argument)\n", num_runs);

    __sync_init(NUM_THREADS, true);

    pthread_t ptds[NUM_THREADS];
    int tids[NUM_THREADS];

    for (int j=0; j<NUM_EXP; j++) {

        printf("----------------------------------------\n");
        printf("Executing experiment %d\n", (j+1));
        printf("----------------------------------------\n");

        // Create
        for (int i=0; i<NUM_THREADS; i++) {
            tids[i] = i;
            pthread_create(ptds+i, NULL, workers[j], (void*) (tids+i));
        }

        // Join
        for (int i=0; i<NUM_THREADS; i++) {
            pthread_join(ptds[i], NULL);
        }
    }

}
