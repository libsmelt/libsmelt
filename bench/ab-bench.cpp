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
#define NUM_RUNS 30 // 10000 // Tested up to 1.000.000

/**
 * \brief Message ping-pong between LAST_NODE and SEQUENTIALIZER
 */
void* pingpong(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);


    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RUNS);
    sk_m_init(&m, NUM_RUNS, "ab", buf);
    
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

/**
 * \brief Reduction
 */
void* reduction(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RUNS);
    sk_m_init(&m, NUM_RUNS, "ab", buf);
    
    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        sk_m_restart_tsc(&m);
        mp_reduce(tid);
        sk_m_add(&m);

        mp_barrier(NULL);
    }

    if (get_thread_id() == SEQUENTIALIZER) {
        sk_m_print(&m);
    }
    

    __thread_end();
    return NULL;
}

void* worker4(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RUNS);
    sk_m_init(&m, NUM_RUNS, "barriers", buf);
    
    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        cycles_t r;
        mp_barrier(&r);
        sk_m_add_value(&m, r-m.last_tsc);
    }

    if (get_thread_id()==SEQUENTIALIZER) {

        sk_m_print(&m);
    }

    __thread_end();
    return NULL;
}

#define NUM_EXP 4

int main(int argc, char **argv)
{
    NUM_THREADS = sysconf(_SC_NPROCESSORS_CONF);

    typedef void* (worker_func_t)(void*);
    worker_func_t* workers[NUM_EXP] = {
        &pingpong,
        &ab,
        &reduction,
        &worker4
    };

    const char *labels[NUM_EXP] = {
        "Ping pong",
        "Atomic broadcast",
        "Reduction",
        "barrier"
    };

    __sync_init(NUM_THREADS);

    pthread_t ptds[NUM_THREADS];
    int tids[NUM_THREADS];

    for (int j=0; j<NUM_EXP; j++) {

        printf("----------------------------------------\n");
        printf("Executing experiment %d - %s\n", (j+1), labels[j]);
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
