#include "stdio.h"
#ifdef BARRELFISH
#include <barrelfish/barrelfish.h>
#endif
#include <numa.h>

#include "shm.h"
#include "sync.h"
#include "topo.h"
#include "internal.h"
#include "mp.h"

__thread int tid;
static coreid_t nproc;

/**
 * \brief Initialization of sync library.
 *
 * This has to be executed once per address space. If threads are used
 * for parallelism, call this only once. With processes, it has to be
 * executed on each process.
 *
 * \param _nproc Number of prcesses participating in the synchronization
 *
 * \param init_model Should the model be initalized?
 */
void __sync_init(int _nproc, bool init_model)
{

    __sys_init();

    if (_nproc == -1) {
        debug_printf("Getting number of processors from libnuma\n");
        _nproc = numa_num_configured_cpus();
    }

    nproc = _nproc;
    debug_printf("Initializing libsync: model: %d nodes, %d threads\n",
                 topo_num_cores(), nproc);
    char *ic_driver = const_cast<char*>("UMPQ");
#ifdef FFQ
    ic_driver = const_cast<char*>("FFQ");
#endif

    debug_printf("Sequentializer is: %d\n", get_sequentializer());
    debug_printf("Interconnect driver is: %s\n", ic_driver);
    debug_printf("Buffer size (#entries): %d\n", UMP_QUEUE_SIZE);

    // Debug output
#ifdef QRM_DBG_ENABLED
    printw("Debug flag (QRM_DBG_ENABLED) is set - peformace will be reduced\n");
#endif
#ifdef SYNC_DEBUG
    printw("Compiler optimizations are off - "
        "performance will suffer if  BUILDTYPE set to debug (in Makefile)\n");
#endif

    // Master share allows simple barriers; needed for boot-strapping
    debug_printfff(DBG__INIT, "Initializing master share .. \n");
    init_master_share();

    // Initialize model
    if (init_model) {
        assert (topo_num_cores()==nproc || topo_num_cores()==0);
        if (get_topo_idx()<0) {

            // Enable a model
            debug_printfff(DBG__INIT, "No topology initalized, activating default .. \n");
            switch_topo();

        } else {
            debug_printfff(DBG__INIT, "Topology already initialized to %d\n",
                           get_topo_idx());
        }
    }

    // Initialize barrier
    pthread_barrier_init(&get_master_share()->data.sync_barrier, NULL, _nproc);
}


/**
 * \brief Initialization of sync library without tree setup in order to
 *        use mp_send etc.
 *
 * This has to be executed once per address space. If threads are used
 * for parallelism, call this only once. With processes, it has to be
 * executed on each process.
 *
 * \param _nproc Number of prcesses participating
 */

void __sync_init_no_tree(int _nproc)
{
    nproc = _nproc;
    tree_init_bindings();
}

/**
 * \brief Initialize sync library.
 *
 * Has to be executed by each thread exactly once. One of the threads
 * has to have id 0, since this causes additional initialization.
 *
 * Internally calls __lowlevel_thread_init. In addition to that,
 * initializes a broadcast tree from given model.
 *
 * \param _tid The id of the thread
 *
 * \param nproc Number of participants in synchronization - in shared
 *     memory implementations, this is redundant, as already given by
 *     __sync_init.
 */
 int __thread_init(coreid_t _tid, int _nproc)
{
    __lowlevel_thread_init(_tid);

#if !defined(USE_THREADS)
    assert (!"NYI: do initialization in EACH process");
    __sync_init(); // XXX untested
#endif

    // Busy wait for master share to be mounted
    while (!get_master_share()) ;

    // Message passing part
    // --------------------------------------------------

    char *_tmp = (char*) malloc(1000);
    sprintf(_tmp, "sync%d", _tid);

    if (_tid == get_sequentializer()) {

        tree_init(_tmp);

        tree_reset();
        tree_connect("DUMMY");

        // Build associated broadcast tree
        setup_tree_from_model();
    }

    pthread_barrier_wait(&get_master_share()->data.sync_barrier);
    return 0;
}


/**
 * \brief Initialize thread for use of sync library.
 *
 * Setup of the bare minimum of functionality required for threads to
 * work with message passing.
 *
 * \param _tid The id of the thread
 * \return Always returns 0
 */
int __lowlevel_thread_init(int _tid)
{
    __backend_thread_start();

    // Store thread ID and pin to core
    tid = _tid;
    coreid_t coreid = get_core_id();
    pin_thread(coreid); // XXX For now .. use Shoal code for that
    debug_printfff(DBG__INIT, "Hello world from thread %d on core %d \n",
                   tid, coreid);

    return 0;
}


/**
 * \brief Return current thread's id.
 */
unsigned int get_thread_id(void)
{
    return tid;
}

/**
 * \brief De-initialzie threads
 *
 */
int __thread_end(void)
{
    __backend_thread_end();

    debug_printfff(DBG__INIT, "Thread %d ending %d\n", tid, mp_get_counter("barriers"));
    return 0;
}

unsigned int get_num_threads(void)
{
    return nproc;
}

/**
 * \brief Check whether given node is the coordinator.
 *
 * The coordinator is exactly one arbitrarily choosen node
 * participating in each libsync instance.
 */
bool is_coordinator(coreid_t c)
{
    return c == get_sequentializer();
}
