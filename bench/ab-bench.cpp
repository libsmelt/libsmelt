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

#include "model_defs.h"


//#define SEND7

#ifdef BARRELFISH
#include <barrelfish/barrelfish.h>
#include <posixcompat.h>
#endif

__thread struct sk_measurement m;
__thread struct sk_measurement m2;

unsigned num_threads;
#define NUM_RUNS 1000000 //50 // 10000 // Tested up to 1.000.000
#define NUM_RESULTS 1000


pthread_barrier_t ab_barrier;

#define TOPO_NAME(x,y) sprintf(x, "%s_%s", y, topo_get_name());

/**
 * \brief Message ping-pong between topo_last_node() and get_sequentializer()
 */
extern __thread uint64_t ump_num_acks_sent;
static void* pingpong(void* a)
{
    char outname[1024];
    char outname2[1024];

     coreid_t tid = *((int*) a);
    __thread_init(tid, num_threads);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    cycles_t *buf2 = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);

    uintptr_t* v = (uintptr_t*) malloc(sizeof(uintptr_t)*8);
    v[0] = 0;

    TOPO_NAME(outname, "pingpong");
    TOPO_NAME(outname2, "pingpong_receive");
    sk_m_init(&m, NUM_RESULTS, outname, buf);
    sk_m_init(&m2, NUM_RESULTS, outname2, buf2);

    if (get_thread_id()==topo_last_node()) {

        mp_binding *b = get_binding(get_sequentializer(), get_thread_id());

        for (unsigned epoch=0; epoch<NUM_RUNS; epoch++) {
            sk_m_restart_tsc(&m);
            debug_printff("send %d\n", epoch);
#ifdef SEND7
            mp_send7(get_sequentializer(), epoch, 1, 2, 3, 4,
             	     5, 6);

            mp_send7(get_sequentializer(), epoch, 1, 2, 3, 4,
             	     5, 6);
#else
            mp_send(get_sequentializer(), epoch);
            mp_send(get_sequentializer(), epoch);
#endif
            debug_printff("receive %d\n", epoch);
            sk_m_restart_tsc(&m2);
#ifdef SEND7
            mp_receive_raw7(b, v);
            assert(v[0]==epoch);
            assert(v[1]==1);
            assert(v[2]==2);
            assert(v[3]==3);
            assert(v[4]==4);
            assert(v[5]==5);
            assert(v[6]==6);

            mp_receive_raw7(b, v);
            assert(v[0]==epoch);
            assert(v[1]==1);
            assert(v[2]==2);
            assert(v[3]==3);
            assert(v[4]==4);
            assert(v[5]==5);
            assert(v[6]==6);
#else
            if (mp_receive_raw(b) !=epoch) {
                printf("mp_receive_raw(b) != epoch");
                exit(1);
            }
            if (mp_receive_raw(b) !=epoch) {
                printf("mp_receive_raw(b) != epoch");
                exit(1);
            }
#endif
            sk_m_add(&m2);
            sk_m_add(&m);
        }

    }
    if (get_thread_id()==get_sequentializer()) {

        for (unsigned epoch=0; epoch<NUM_RUNS; epoch++) {
            mp_binding *b = get_binding(topo_last_node(), get_thread_id());

            debug_printff("receive %d\n", epoch);
            sk_m_restart_tsc(&m);
            sk_m_restart_tsc(&m2);
#ifdef SEND7
            mp_receive_raw7(b, v);
            assert(v[0]==epoch);
            assert(v[1]==1);
            assert(v[2]==2);
            assert(v[3]==3);
            assert(v[4]==4);
            assert(v[5]==5);
            assert(v[6]==6);

            mp_receive_raw7(b, v);
            assert(v[0]==epoch);
            assert(v[1]==1);
            assert(v[2]==2);
            assert(v[3]==3);
            assert(v[4]==4);
            assert(v[5]==5);
            assert(v[6]==6);
#else
            if (mp_receive_raw(b) != epoch) {
                printf("mp_receive_raw(b) != epoch");
                exit(1);
            }

            if (mp_receive_raw(b) != epoch) {
                printf("mp_receive_raw(b) != epoch");
                exit(1);
            }
#endif
            sk_m_add(&m2);

            debug_printff("send %d\n", epoch);
#ifdef SEND7
            mp_send7(topo_last_node(), epoch,
                    1,
                    2,
                    3,
                    4,
                    5,
                    6);
            mp_send7(topo_last_node(), epoch,
                    1,
                    2,
                    3,
                    4,
                    5,
                    6);
#else
            mp_send(topo_last_node(), epoch);
            mp_send(topo_last_node(), epoch);
#endif
            sk_m_add(&m);
        }
    }


    if (get_thread_id() == topo_last_node() ||
        get_thread_id() == get_sequentializer()) {

        sk_m_print(&m);
        sk_m_print(&m2);
    }


    __thread_end();
    return NULL;
}

/**
 * \brief Broadcast trigger by topo_last_node() until back to LAST_NODE
 */
static void* ab(void* a)
{
    char outname[1024];

   coreid_t tid = *((int*) a);
    __thread_init(tid, num_threads);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    uintptr_t* v = (uintptr_t*) malloc(sizeof(uintptr_t)*8);
    v[0] = 0;


    TOPO_NAME(outname, "ab");
    sk_m_init(&m, NUM_RESULTS, outname, buf);

    std::vector<int> *leafs = topo_all_leaf_nodes()[0];

    for (std::vector<int>::iterator i=leafs->begin(); i!=leafs->end(); ++i) {

        coreid_t last_node = (coreid_t) *i;
        sk_m_reset(&m);

        for (int epoch=0; epoch<NUM_RUNS; epoch++) {

            sk_m_restart_tsc(&m);

            if (get_thread_id()==last_node) {
#ifdef SEND7
                mp_send7(get_sequentializer(), 1, 2, 3, 4, 5, 6, 7);
#else
                mp_send(get_sequentializer(), 0);
#endif
            }

            if (get_thread_id()==get_sequentializer()) {
#ifdef SEND7
		        mp_receive7(last_node, v);
		        mp_send_ab7(v[0], v[1], v[2], v[3], v[4],
			                v[5], v[6]);
#else
                mp_send_ab(mp_receive(last_node));
#endif
            }
            else {
#ifdef SEND7
                mp_receive_forward7(v);
#else
                mp_receive_forward(0);
#endif
            }

            sk_m_add(&m);
/*
            if (get_thread_id() == last_node) {
                printf("Round %d finished \n", epoch);
            }
*/
        }

        if (get_thread_id() == last_node) {
            sk_m_print(&m);
        }
    }

    __thread_end();
    return NULL;
}

extern void shl_barrier_shm(int b_count);


/**
 * \brief
 */
static void* reduction(void* a)
{
    char outname[1024];

    coreid_t tid = *((int*) a);
    __thread_init(tid, num_threads);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    uintptr_t* v = (uintptr_t*) malloc(sizeof(uintptr_t)*8);

    TOPO_NAME(outname, "reduction");
    sk_m_init(&m, NUM_RESULTS, outname, buf);

    std::vector<int> *leafs = topo_all_leaf_nodes()[0];

    for (std::vector<int>::iterator i=leafs->begin(); i!=leafs->end(); ++i) {

        coreid_t last_node = (coreid_t) *i;
        sk_m_reset(&m);

        for (int epoch=0; epoch<NUM_RUNS; epoch++) {

            sk_m_restart_tsc(&m);
#ifdef SEND7
            mp_reduce7(v, 
                       1, 1, 1, 1, 1, 1, 1);
#else
            mp_reduce(tid);
#endif

            if (tid==get_sequentializer()) {
#ifdef SEND7
                mp_send7(last_node, 0, 1, 2, 3, 4, 5, 6);
#else
                mp_send(last_node, 0);
#endif
            } else if (tid==last_node) {
#ifdef SEND7
                mp_receive7(get_sequentializer(), v);
#else
                v[0] = mp_receive(get_sequentializer());
#endif
                sk_m_add(&m);
            }
        }

        if (get_thread_id() == last_node) {
            sk_m_print(&m);
        }

    }

    __thread_end();
    return NULL;
}

static void* barrier(void* a)
{
    // XXX Perhaps we also want all leaf nodes here?

    char outname[1024];

    coreid_t tid = *((int*) a);
    __thread_init(tid, num_threads);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);

    TOPO_NAME(outname, "barriers");
    sk_m_init(&m, NUM_RESULTS, outname, buf);

    for (int epoch=0; epoch<NUM_RUNS; epoch++) {

        mp_barrier(NULL);

        if (tid==topo_last_node()) sk_m_add(&m);
    }

    if (get_thread_id()==get_sequentializer() ||
        get_thread_id()==topo_last_node()) {

        sk_m_print(&m);
    }

    __thread_end();
    return NULL;
}

static void* agreement(void* a)
{
    char outname[1024];

    coreid_t tid = *((int*) a);
    __thread_init(tid, num_threads);

    // Setup buffer for measurements
    cycles_t *buf = (cycles_t*) malloc(sizeof(cycles_t)*NUM_RESULTS);
    uintptr_t* v = (uintptr_t*) malloc(sizeof(uintptr_t)*8);
    v[0] = 0;

    TOPO_NAME(outname, "agreement");
    sk_m_init(&m, NUM_RESULTS, outname, buf);

    uintptr_t payload = 7;

    std::vector<int> *_leaf_nodes = topo_all_leaf_nodes()[0];

    for (std::vector<int>::iterator i=_leaf_nodes->begin();
         i!=_leaf_nodes->end(); ++i) {

        coreid_t last_node = (coreid_t) *i;
        assert (last_node>=0 && last_node<=num_threads);

        sk_m_reset(&m);

        for (int epoch =0; epoch < NUM_RUNS; epoch++) {

            /*
             * Phase one of 2PC is a broadcast followed by
             * a reduction
             */
            sk_m_restart_tsc(&m);

            //Synchronize
#ifndef SEND7
            uintptr_t val = 0;
#endif
            if (get_thread_id() == last_node) {
#ifdef SEND7
               mp_send7(get_sequentializer(), 0, 0, 0, 0, 0, 0, payload);
#else
               mp_send(get_sequentializer(), payload);
#endif
            }

            // broadcast to all

            if (tid == get_sequentializer()) {
#ifdef SEND7
                mp_receive7(last_node, v);
                mp_send_ab7(v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
#else
                mp_send_ab(mp_receive(last_node));
#endif
            } else {
#ifdef SEND7
                mp_receive_forward7(v);
#else
                val = mp_receive_forward(0);
#endif
            }
            // Reduction
#ifdef SEND7
            mp_reduce7(v, v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
#else
            mp_reduce(val);
#endif

            /*
             * Phase two of 2PC is a broadcast to inform
             * of the commit
             */
            //Broadcast to all
            if (tid == get_sequentializer()) {
#ifdef SEND7
                mp_send_ab7(0, 0, 0, 0, 0, 0, payload);
#else
                mp_send_ab(val);
#endif
            } else {
#ifdef SEND7
                mp_receive_forward7(v);
#else
                val = mp_receive_forward(0);
#endif
            }

            sk_m_add(&m);

        }
        if (get_thread_id() == last_node) {
            sk_m_print(&m);
        }
    }
    __thread_end();
    return NULL;
}


#define NUM_EXP 5

#ifdef BARRELFISH
static void domain_init_done(void *arg, errval_t err)
{
    debug_printf("SPANNED!\n");
}
#endif

int main(int argc, char **argv)
{
    unsigned nthreads = sysconf(_SC_NPROCESSORS_CONF);

    __sync_init(nthreads, true);

    num_threads = get_num_threads();

    pthread_barrier_init(&ab_barrier, NULL, num_threads);

    typedef void* (worker_func_t)(void*);
    worker_func_t* workers[NUM_EXP] = {
        &pingpong,
        &ab,
        &reduction,
        &barrier,
        &agreement,
    };

    const char *labels[NUM_EXP] = {
        "Ping pong",
        "Atomic broadcast",
        "Reduction",
        "barrier",
        "Agreement",
    };



    pthread_t ptds[num_threads];
    int tids[num_threads];


#ifdef BARRELFISH
    cpu_set_t *cpuset = CPU_ALLOC(num_threads);

    for (unsigned i=1; i<num_threads; i++) {
        //for (int i = my_core_id + BOMP_DEFAULT_CORE_STRIDE; i < nos_threads + my_core_id; i++) {
        coreid_t core = i;
        errval_t err = domain_new_dispatcher(core, domain_init_done, NULL);
        if (err_is_fail(err)) {
            DEBUG_ERR(err, "failed to span domain");
            printf("Failed to span domain to %d\n", i);
            assert(err_is_ok(err));
        }
    }

#endif
    for (unsigned e=0; e<(topo_num_topos()-1); e++) {
        for (int j=0; j<NUM_EXP; j++) {
            printf("----------------------------------------\n");
            printf("Executing experiment %d - %s\n", (j+1), labels[j]);
            printf("----------------------------------------\n");

#ifdef BARRELFISH
            thread_yield();
#else
            // Yield to reduce the risk of getting de-scheduled later
            sched_yield();
#endif
            // Create
            for (unsigned i=1; i<num_threads; i++) {
                tids[i] = i;

#ifdef BARRELFISH
                pthread_attr_t attr;
                pthread_attr_init(&attr);

                CPU_ZERO(cpuset);
                CPU_SET(i, cpuset);
                pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), cpuset);

                pthread_create(ptds+i, &attr, workers[j], (void*) (tids+i));
#else
                pthread_create(ptds+i, NULL, workers[j], (void*) (tids+i));
#endif
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

#ifdef SEND7
    printf("Send7 defined \n");
#endif

#ifdef BARRELFISH
    debug_printf("done!");
    while(1)
        ;
#endif
}
