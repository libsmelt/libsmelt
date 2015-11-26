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
#define NUM_RUNS 1000000 //50 // 10000 // Tested up to 1.000.000
#define NUM_RESULTS 1000

pthread_barrier_t ab_barrier;

#define TOPO_NAME(x,y) sprintf(x, "%s_%s", y, topo_get_name());

/**
 * \brief Message ping-pong between get_last_node() and get_sequentializer()
 */
extern __thread uint64_t ump_num_acks_sent;
void* pingpong(void* a)
{
    char outname[1024];
   
     coreid_t tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    
    TOPO_NAME(outname, "pingpong");
    sk_m_init(&m, NUM_RESULTS, outname, buf);
     
    for (unsigned epoch=0; epoch<NUM_RUNS; epoch++) {

        // assert (ump_num_acks_sent==0); // We should NEVER have to sent
        //                                // ACKs in this benchmark, as
        //                                // they should get piggy-backed
        //                                // with the response.
        
        sk_m_restart_tsc(&m);
        
        if (get_thread_id()==get_last_node()) {

            debug_printff("send %d\n", epoch);
            mp_send(get_sequentializer(), epoch);

            debug_printff("receive %d\n", epoch);
            assert(mp_receive(get_sequentializer())==epoch);
        }
        else if (get_thread_id()==get_sequentializer()) {

            debug_printff("receive %d\n", epoch);
            assert(mp_receive(get_last_node())==epoch);

            debug_printff("send %d\n", epoch);
            mp_send(get_last_node(), epoch);
        }

        sk_m_add(&m);
    }

    if (get_thread_id() == get_last_node() ||
        get_thread_id() == get_sequentializer()) {

        sk_m_print(&m);
    }
    

    __thread_end();
    return NULL;
}

/**
 * \brief Broadcast trigger by get_last_node() until back to LAST_NODE
 */
void* ab(void* a)
{
    char outname[1024];
    
   coreid_t tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);

    TOPO_NAME(outname, "ab");
    sk_m_init(&m, NUM_RESULTS, outname, buf);

    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        sk_m_restart_tsc(&m);
        
        if (get_thread_id()==get_last_node()) {
            mp_send(get_sequentializer(), 0);
        }

        if (get_thread_id()==get_sequentializer()) {
            mp_send_ab(mp_receive(get_last_node()));
        }
        else {
            mp_receive_forward(0);
        }

        sk_m_add(&m);
    }

    if (get_thread_id() == get_last_node()) {
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
    char outname[1024];
    
    coreid_t tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);

    TOPO_NAME(outname, "reduction");
    sk_m_init(&m, NUM_RESULTS, outname, buf);
     
    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        sk_m_restart_tsc(&m);
        mp_reduce(tid);
        sk_m_add(&m);

        pthread_barrier_wait(&ab_barrier);
    }

    if (get_thread_id() == get_sequentializer()) {
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
    char outname[1024];
    
    coreid_t tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);

    TOPO_NAME(outname, "reduction-ln");
    sk_m_init(&m, NUM_RESULTS, outname, buf);
    
    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        sk_m_restart_tsc(&m);
        mp_reduce(tid);

        if (tid==get_sequentializer()) {
            mp_send(get_last_node(), 0);
        } else if (tid==get_last_node()) {
            mp_receive(get_sequentializer());
        }
        sk_m_add(&m);
    }

    if (get_thread_id() == get_last_node()) {
        sk_m_print(&m);
    }
    

    __thread_end();
    return NULL;
}

void* barrier(void* a)
{
    char outname[1024];
        
    coreid_t tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    
    TOPO_NAME(outname, "barriers");
    sk_m_init(&m, NUM_RESULTS, outname, buf);
    
    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        mp_barrier(NULL);
        
        if (tid==get_last_node()) sk_m_add(&m);
    }

    if (get_thread_id()==get_sequentializer() ||
        get_thread_id()==get_last_node()) {

        sk_m_print(&m);
    }

    __thread_end();
    return NULL;
}

#define NUM_EXP 1

int main(int argc, char **argv)
{
    NUM_THREADS = sysconf(_SC_NPROCESSORS_CONF);

    pthread_barrier_init(&ab_barrier, NULL, NUM_THREADS);
    
    typedef void* (worker_func_t)(void*);
    worker_func_t* workers[NUM_EXP] = {
        &pingpong,
        // &ab,
        // //        &reduction,
        // &reduction_ln,
        // &barrier
    };

    const char *labels[NUM_EXP] = {
        "Ping pong",
        // "Atomic broadcast",
        // //        "Reduction",
        // "Reduction (last node)", 
        // "barrier"
    };

    __sync_init(NUM_THREADS);

    pthread_t ptds[NUM_THREADS];
    int tids[NUM_THREADS];

    for (int e=0; e<NUM_TOPOS; e++) {
    
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

        if( e<NUM_TOPOS-1) switch_topo();
    }

    pthread_barrier_destroy(&ab_barrier);
    
}
