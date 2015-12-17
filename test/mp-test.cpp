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

__thread struct sk_measurement m;

#define NUM_THREADS 4
#define NUM_RUNS 10000 // Tested up to 1.000.000
void* worker2(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        if (tid == get_sequentializer()) {
            mp_send_ab(tid);

            // Wait for message from get_last_node()
            mp_receive(get_last_node());

        } else {
            mp_receive_forward(tid);

            if (get_thread_id()==get_last_node()) {

                mp_send(get_sequentializer(), 0);
            }
        }
    }
    printf("Thread %d completed\n", tid);

    __thread_end();
    return NULL;
}

void* worker3(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    debug_printf("Reduction complete: %d\n", mp_reduce(tid));

    __thread_end();
    return NULL;
}

int barrier_rounds[NUM_THREADS];

void* worker4(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RUNS);
    sk_m_init(&m, NUM_RUNS, "barriers", buf);

    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

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

    if (get_thread_id()==get_sequentializer()) {

        sk_m_print(&m);
    }


    debug_printf("All done :-)\n");

    __thread_end();
    return NULL;
}

#define NUM_EXP 3

int main(int argc, char **argv)
{
    typedef void* (worker_func_t)(void*);
    worker_func_t* workers[NUM_EXP] = {
        //        &worker1,
        &worker2,
        &worker3,
        &worker4
    };

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
