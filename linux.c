#include "sync.h"
#include "model_defs.h"
#include "topo.h"

#include "debug.h"

#ifdef FFQ

#else
#include "ump_conf.h"
#endif

#include <sched.h>
#include <errno.h>

#include <cstdarg>
#include <numa.h>

/**
 * \brief Get core ID of the current thread/process
 *
 * Assumption: On Linux, the core_id equals the thread_id
 */
coreid_t get_core_id(void)
{
    return get_thread_id();
}

int _tree_prepare(void)
{
    return 0;
}

void _tree_export(const char *qrm_my_name)
{
    // Linux does not need any export
}

void _tree_register_receive_handler(struct sync_binding *)
{
}

/* --------------------------------------------------
 * Barrelfish compatibility
 * --------------------------------------------------
 */
coreid_t disp_get_core_id(void)
{
    return get_thread_id();
}

void bench_init(void)
{
}

cycles_t bench_tscoverhead(void)
{
    return 0;
}

/* --------------------------------------------------
 * Tree operations
 * --------------------------------------------------
 */

int
numa_cpu_to_node(int cpu)
{
    int ret    = -1;
    int node_max = numa_max_node();
    struct bitmask *cpus = numa_allocate_cpumask();

    for (int node=0; node <= node_max; node++) {
        numa_bitmask_clearall(cpus);
        if (numa_node_to_cpus(node, cpus) < 0) {
            perror("numa_node_to_cpus");
            fprintf(stderr, "numa_node_to_cpus() failed for node %d\n", node);
            if (errno== ERANGE) {
                fprintf(stderr, "Given range is too small\n");
            }
            abort();
        }

        if (numa_bitmask_isbitset(cpus, cpu)) {
            ret = node;
        }
    }

    numa_free_cpumask(cpus);

    if (ret == -1) {
        fprintf(stderr, "%s failed to find node for cpu %d\n",
                __FUNCTION__, cpu);
        abort();
    }

    return ret;
}


#ifdef FFQ
// --------------------------------------------------

#define FF_CONF_INIT(src_, dst_)                        \
    {                                                   \
        .src = {                                        \
            .core_id = src_,                            \
            .delay = 0,                                 \
        },                                              \
        .dst = {                                        \
            .core_id = dst_,                            \
            .delay = 0                                  \
        },                                              \
        .next = NULL,                                   \
        .qsize = UMP_QUEUE_SIZE,                        \
        .shm_numa_node = -1                             \
    }

/**
 * \brief Setup a pair of UMP channels.
 *
 * This is borrowed from UMPQ's pt_bench_pairs_ump program.
 */
void _setup_chanels(int src, int dst)
{
    debug_printfff(DBG__INIT, "FF: setting up channel between %d and %d\n",
                   src, dst);
    
    struct ffq_pair_conf ffpc = FF_CONF_INIT(src, dst);
    struct ffq_pair_state *ffps = ffq_pair_state_create(&ffpc);

    add_binding(src, dst, ffps);

    struct ffq_pair_conf ffpc_r = FF_CONF_INIT(dst, src);
    struct ffq_pair_state *ffps_r = ffq_pair_state_create(&ffpc_r);
    
    add_binding(dst, src, ffps_r);
}

void mp_send_raw(mp_binding *b, uintptr_t val)
{
    ffq_enqueue(&b->src, val);
}

uintptr_t mp_receive_raw(mp_binding *b)
{
    uintptr_t r;
    ffq_dequeue(&b->dst, &r);

    return r;
}

void mp_send_raw7(mp_binding *b,
                  uintptr_t val1,
                  uintptr_t val2,
                  uintptr_t val3,
                  uintptr_t val4,
                  uintptr_t val5,
                  uintptr_t val6,
                  uintptr_t val7)
{
    ffq_enqueue_full(&b->src, val1, val2, val3, 
                val4, val5, val6, val7);
}

void mp_receive_raw7(mp_binding *b, uintptr_t* buf)
{
    ffq_dequeue_full(&b->dst, buf);
}

bool mp_can_receive_raw(mp_binding *b)
{
    return ffq_can_dequeue(&b->dst);
}

#else // UMP
// --------------------------------------------------

#define UMP_CONF_INIT(src_, dst_, shm_size_)      \
{                                                 \
    .src = {                                      \
         .core_id = src_,                         \
         .shm_size = shm_size_,                   \
         .shm_numa_node = numa_cpu_to_node(src_)  \
    },                                            \
    .dst = {                                      \
         .core_id = dst_,                         \
         .shm_size = shm_size_,                   \
         .shm_numa_node = numa_cpu_to_node(src_)  \
    },                                            \
    .shared_numa_node = -1,                       \
    .nonblock = 1                                 \
}

/**
 * \brief Setup a pair of UMP channels.
 *
 * This is borrowed from UMPQ's pt_bench_pairs_ump program.
 */
void _setup_chanels(int src, int dst)
{
    debug_printfff(DBG__INIT, "Establishing connection between %d and %d\n",
                   src, dst);

    const int shm_size = (64*UMP_QUEUE_SIZE);

    struct ump_pair_conf fwr_conf = UMP_CONF_INIT(src, dst, shm_size);
    struct ump_pair_conf rev_conf = UMP_CONF_INIT(dst, src, shm_size);

    add_binding(src, dst, ump_pair_state_create(&fwr_conf));
    add_binding(dst, src, ump_pair_state_create(&rev_conf));
}

void mp_send_raw(mp_binding *b, uintptr_t val)
{
    struct ump_pair_state *ups = (struct ump_pair_state*) b;
    struct ump_queue *q = &ups->src.queue;

    ump_enqueue_word(q, val);
}

void mp_send_raw7(mp_binding *b, 
                  uintptr_t val1,
                  uintptr_t val2,
                  uintptr_t val3,
                  uintptr_t val4,
                  uintptr_t val5,
                  uintptr_t val6,
                  uintptr_t val7)
{
    struct ump_pair_state *ups = (struct ump_pair_state*) b;
    struct ump_queue *q = &ups->src.queue;

    ump_enqueue(q, val1, val2, val3, val4,
                val5, val6, val7);
}


uintptr_t mp_receive_raw(mp_binding *b)
{
    struct ump_pair_state *ups = (struct ump_pair_state*) b;
    struct ump_queue *q = &ups->dst.queue;

    uintptr_t r;
    ump_dequeue_word(q, &r);

    return r;
}


void mp_receive_raw7(mp_binding *b, uintptr_t* buf)
{
    struct ump_pair_state *ups = (struct ump_pair_state*) b;
    struct ump_queue *q = &ups->dst.queue;

    ump_dequeue(q, buf);
}

bool mp_can_receive_raw(mp_binding *b)
{
    struct ump_pair_state *ups = (struct ump_pair_state*) b;
    struct ump_queue *q = &ups->dst.queue;
    return ump_can_dequeue(q);
}

#endif 

/**
 * \brief Establish connections as given by the model.
 */
void tree_connect(const char *qrm_my_name)
{
    uint64_t nproc = topo_num_cores();

    for (unsigned int i=0; i<nproc; i++) {

        for (unsigned int j=0; j<nproc; j++) {

            if (topo_is_parent_real(i, j) || (j==get_sequentializer() && i!=j)) {
                debug_printfff(DBG__SWITCH_TOPO, "setup: %d %d\n", i, j);
                _setup_chanels(i, j);
            }
        }
    }
}

void debug_printf(const char *fmt, ...)
{
    va_list argptr;
    char str[1024];
    size_t len;

    len = snprintf(str, sizeof(str), "\033[34m%d\033[0m: ",
                   get_thread_id());
    if (len < sizeof(str)) {
        va_start(argptr, fmt);
        vsnprintf(str + len, sizeof(str) - len, fmt, argptr);
        va_end(argptr);
    }
    printf(str, sizeof(str));
}

void pin_thread(coreid_t cpu)
{
    debug_printfff(DBG__INIT, "Pinning thread %d to core %d\n",
                   get_thread_id(), cpu);

    cpu_set_t cpu_mask;
    int err;

    CPU_ZERO(&cpu_mask);
    CPU_SET(cpu, &cpu_mask);

    err = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);

    if (err) {
        perror("sched_setaffinity");
        exit(1);
    }
}

void __sys_init(void)
{
#ifdef UMP_ENABLE_SLEEP
    printf("\033[1;31mWarning:\033[0m UMP sleep is enabled - this is buggy\n");
#else
    printf("UMP sleep disabled\n");
#endif
    assert (numa_available()==0);

}

int __backend_thread_start(void)
{
#ifdef UMP_DBG_COUNT
#ifdef FFQ
#else    
    ump_start();
#endif    
#endif
    return 0;
}

int __backend_thread_end(void)
{
#ifdef UMP_DBG_COUNT
#ifdef FFQ
#else
    ump_end();
#endif    
#endif
    return 0;
}
