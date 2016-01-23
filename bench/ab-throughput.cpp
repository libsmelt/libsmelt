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
#include <vector>
#include "shl.h"

#include "model_defs.h"
#include "barrier.h"

#ifdef PARLIB
#include "mcs.h"
#endif

//#define SEND7

#ifdef BARRELFISH
#include <barrelfish/barrelfish.h>
#include <posixcompat.h>
#endif

__thread struct sk_measurement m;
__thread struct sk_measurement m2;

unsigned num_threads;
#define NUM_RUNS 1000 //50 // 10000 // Tested up to 1.000.000
#define NUM_RESULTS 1000

pthread_barrier_t ab_barrier;

#define TOPO_NAME(x,y) sprintf(x, "%s_%s", y, topo_get_name());

mcs_barrier_t mcs_b;
static void* mcs_barrier(void* a)
{
    coreid_t tid = *((int*) a);
    shl__init_thread(tid);
    __thread_init(tid, num_threads); // will bind threads

    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    char outname[1024];
    TOPO_NAME(outname, "mcs-barrier");
    sk_m_init(&m, NUM_RESULTS, outname, buf);

    if (get_thread_id() == get_sequentializer())
        papi_start();
    sk_m_restart_tsc(&m);

    for (unsigned i=0; i<NUM_RUNS; i++) {
        mcs_barrier_wait(&mcs_b, tid);
    }

    sk_m_add(&m);
    if (get_thread_id() == get_sequentializer())
        papi_stop();

    sk_m_print(&m);

    return NULL;
}

static void* barrier(void* a)
{

    coreid_t tid = *((int*) a);
    shl__init_thread(tid);
    __thread_init(tid, num_threads);

    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    char outname[1024];
    TOPO_NAME(outname, "syc-barrier");
    sk_m_init(&m, NUM_RESULTS, outname, buf);

    if (get_thread_id() == get_sequentializer())
        papi_start();
    sk_m_restart_tsc(&m);
    for (unsigned i=0; i<NUM_RUNS; i++) {
        shl_hybrid_barrier(NULL);
    }
    sk_m_add(&m);
    if (get_thread_id() == get_sequentializer())
        papi_stop();


    //    if (get_thread_id() == get_sequentializer()) {
        sk_m_print(&m);
        //    }

    __thread_end();
    return NULL;
}

#define NUM_EXP 2

#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a > _b ? _a : _b; })

int main(int argc, char **argv)
{
    unsigned nthreads = sysconf(_SC_NPROCESSORS_CONF);

    shl__init(nthreads, false);
    __sync_init(nthreads, true);

    num_threads = get_num_threads();

    mcs_barrier_init(&mcs_b, num_threads);
    pthread_barrier_init(&ab_barrier, NULL, num_threads);

    typedef void* (worker_func_t)(void*);
    worker_func_t* workers[NUM_EXP] = {
        &mcs_barrier,
        &barrier,
    };

    const char *labels[NUM_EXP] = {
        "MCS barrier",
        "libsync barrier"
    };

    pthread_t ptds[num_threads];
    int tids[num_threads];

    printf("%d models\n", max(1U, (topo_num_topos()-1)));

    for (unsigned e=0; e<max(1U, (topo_num_topos()-1)); e++) {
        for (int j=0; j<NUM_EXP; j++) {
            printf("----------------------------------------\n");
            printf("Executing experiment %d - %s\n", (j+1), labels[j]);
            printf("----------------------------------------\n");

            // Yield to reduce the risk of getting de-scheduled later
            sched_yield();

            // Create
            for (unsigned i=1; i<num_threads; i++) {
                tids[i] = i;

                pthread_create(ptds+i, NULL, workers[j], (void*) (tids+i));
            }

            // Master thread executes work for node 0
            tids[0] = 0;
            workers[j]((void*) (tids+0));

            // Join
            for (unsigned i=1; i<num_threads; i++) {
                pthread_join(ptds[i], NULL);
            }
        }

        if( e<topo_num_topos()-1) switch_topo();
    }

    pthread_barrier_destroy(&ab_barrier);
}
