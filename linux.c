#include "sync.h"
#include "model_defs.h"
#include "topo.h"

extern "C" {
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "ump_conf.h"
}
    
#include <sched.h>

#include <numa.h>

/**
 * \brief Get core ID of the current thread/process
 *
 * Assumption: On Linux, the core_id equals the thread_id
 */
int get_core_id(void)
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
    return 0;
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

/**
 * \brief Store UMP bindings
 *
 * This is currently not clean, as it requires n^2 entries in the
 * buffer for n cores.
 */
typedef struct ump_pair_state mp_binding;
mp_binding* bindings[TOPO_NUM_CORES][TOPO_NUM_CORES];

int
numa_cpu_to_node(int cpu)
{
    int ret    = -1;
    int ncpus  = numa_num_possible_cpus();
    int node_max = numa_max_node();
    struct bitmask *cpus = numa_bitmask_alloc(ncpus);

    for (int node=0; node <= node_max; node++) {
        numa_bitmask_clearall(cpus);
        if (numa_node_to_cpus(node, cpus) < 0) {
            perror("numa_node_to_cpus");
            fprintf(stderr, "numa_node_to_cpus() failed for node %d\n", node);
            abort();
        }

        if (numa_bitmask_isbitset(cpus, cpu)) {
            ret = node;
        }
    }

    numa_bitmask_free(cpus);
    if (ret == -1) {
        fprintf(stderr, "%s failed to find node for cpu %d\n",
                __FUNCTION__, cpu);
        abort();
    }

    return ret;
}


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
static void _setup_ump_chanels(int src, int dst)
{
    printf("Establishing connection between %d and %d\n", src, dst);
    
    // Benchmarks send 1024 messages of one cache-line(?) = 64 Bytes each
    const int shm_size = (64*1024);

    struct ump_pair_conf fwr_conf = UMP_CONF_INIT(src, dst, shm_size);
    struct ump_pair_conf rev_conf = UMP_CONF_INIT(dst, src, shm_size);

    bindings[src][dst] = ump_pair_state_create(&fwr_conf);
    bindings[dst][src] = ump_pair_state_create(&rev_conf);
}

/**
 * \brief Establish connections as given by the model.
 */
void tree_connect(const char *qrm_my_name)
{
    uint64_t nproc = topo_num_cores();
    
    for (int i=0; i<nproc; i++) {

        for (int j=0; j<nproc; j++) {

            if (topo_is_parent(i, j)) {
                printf("setup: %d %d\n", i, j);
                _setup_ump_chanels(i, j);
            }
        }
    }
}


