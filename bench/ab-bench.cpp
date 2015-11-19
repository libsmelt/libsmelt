/*
 *
 */
#include <stdio.h>
#include "sync.h"
#include "topo.h"
#include "mp.h"
#include "measurement_framework.h"
#include <pthread.h>
#include <unistd.h>

#include "model_defs.h"

__thread struct sk_measurement m;

int NUM_THREADS;
#define NUM_RUNS 100 // 10000 // Tested up to 1.000.000

pthread_barrier_t ab_barrier;

/**
 * \brief Message ping-pong between LAST_NODE and SEQUENTIALIZER
 */
void* pingpong(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);


    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RUNS);
    sk_m_init(&m, NUM_RUNS, "pingpong", buf);
    
    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        sk_m_restart_tsc(&m);
        
        if (get_thread_id()==LAST_NODE) {
            mp_send(SEQUENTIALIZER, epoch);
            mp_receive(SEQUENTIALIZER);
        }
        else if (get_thread_id()==SEQUENTIALIZER) {
            mp_receive(LAST_NODE);
            mp_send(LAST_NODE, epoch);
        }

        sk_m_add(&m);
    }

    if (get_thread_id() == LAST_NODE ||
        get_thread_id() == SEQUENTIALIZER) {

        sk_m_print(&m);
    }
    

    __thread_end();
    return NULL;
}

/**
 * \brief Broadcast trigger by LAST_NODE until back to LAST_NODE
 */
void* ab(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RUNS);
    sk_m_init(&m, NUM_RUNS, "ab", buf);
    
    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        sk_m_restart_tsc(&m);
        
        if (get_thread_id()==LAST_NODE) {
            mp_send(SEQUENTIALIZER, 0);
        }

        if (get_thread_id()==SEQUENTIALIZER) {
            mp_send_ab(mp_receive(LAST_NODE));
        }
        else {
            mp_receive_forward(0);
        }

        sk_m_add(&m);
    }

    if (get_thread_id() == LAST_NODE) {
        sk_m_print(&m);
    }
    

    __thread_end();
    return NULL;
}

extern void shl_barrier_shm(int b_count);

/**
 * \brief Reduction
 */
void* reduction(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RUNS);
    sk_m_init(&m, NUM_RUNS, "reduction", buf);
    
    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        sk_m_restart_tsc(&m);
        mp_reduce(tid);
        sk_m_add(&m);

        pthread_barrier_wait(&ab_barrier);
    }

    if (get_thread_id() == SEQUENTIALIZER) {
        sk_m_print(&m);
    }
    

    __thread_end();
    return NULL;
}

/**
 * \brief
 */
void* reduction_ln(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RUNS);
    sk_m_init(&m, NUM_RUNS, "reduction-ln", buf);
    
    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        sk_m_restart_tsc(&m);
        mp_reduce(tid);

        if (tid==SEQUENTIALIZER) {
            mp_send(LAST_NODE, 0);
        } else if (tid==LAST_NODE) {
            mp_receive(SEQUENTIALIZER);
        }
        sk_m_add(&m);
    }

    if (get_thread_id() == LAST_NODE) {
        sk_m_print(&m);
    }
    

    __thread_end();
    return NULL;
}

void* barrier(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RUNS);
    sk_m_init(&m, NUM_RUNS, "barriers", buf);
    
    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        mp_barrier(NULL);
        
        if (tid==LAST_NODE) sk_m_add(&m);
    }

    if (get_thread_id()==SEQUENTIALIZER ||
        get_thread_id()==LAST_NODE) {

        sk_m_print(&m);
    }

    __thread_end();
    return NULL;
}

#define NUM_EXP 5

int main(int argc, char **argv)
{
    NUM_THREADS = sysconf(_SC_NPROCESSORS_CONF);

    pthread_barrier_init(&ab_barrier, NULL, NUM_THREADS);
    
    typedef void* (worker_func_t)(void*);
    worker_func_t* workers[NUM_EXP] = {
        &pingpong,
        &ab,
        &reduction,
        &reduction_ln,
        &barrier
    };

    const char *labels[NUM_EXP] = {
        "Ping pong",
        "Atomic broadcast",
        "Reduction",
        "Reduction (last node)", 
        "barrier"
    };

    __sync_init(NUM_THREADS);

    pthread_t ptds[NUM_THREADS];
    int tids[NUM_THREADS];

    for (int j=0; j<NUM_EXP; j++) {

        printf("----------------------------------------\n");
        printf("Executing experiment %d - %s\n", (j+1), labels[j]);
        printf("----------------------------------------\n");

        // Yield to reduce the risk of getting de-scheduled later
        sched_yield();
        
        // Create
        for (int i=1; i<NUM_THREADS; i++) {
            tids[i] = i;
            pthread_create(ptds+i, NULL, workers[j], (void*) (tids+i));
        }

        // Master thread executes work for node 0
        tids[0] = 0;
        workers[j]((void*) (tids+0));

        // Join
        for (int i=1; i<NUM_THREADS; i++) {
            pthread_join(ptds[i], NULL);
        }
    }

    pthread_barrier_destroy(&ab_barrier);
    
}
