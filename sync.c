#include "stdio.h"

#include "sync/shm.h"
#include "sync/sync.h"
#include "sync/topo.h"
#include "sync/internal.h"

__thread int tid;
static int nproc;

/**
 * \brief Initialization of sync library.
 *
 * This has to be executed once per address space. If threads are used
 * for parallelism, call this only once. With processes, it has to be
 * executed on each process.
 */
void __sync_init(void)
{
    // Master share allows simple barriers; needed for boot-strapping
    debug_printfff(DBG__INIT, "Initializing master share .. \n");
    init_master_share();

    // Enable a model
    debug_printfff(DBG__INIT, "Switching topology .. \n");
    switch_topo();
}

/**
 * \brief Initialize sync library.
 *
 * Has to be executed by each thread exactly once. One of the threads
 * has to have id 0, since this causes additional initialization.
 *
 * \param _tid The id of the thread
 * \param nproc Number of participants in synchronization
 */
int __thread_init(int _tid, int _nproc)
{
    tid = _tid;
    printf("Hello world from thread %d .. \n", tid);

#ifdef USE_THREADS
    // In case of threads, we need to initialize everything only once
    if (tid==0) {
        __sync_init();
        nproc = _nproc;
    }
#else
    assert (!"NYI: do initialization in EACH process");
#endif

    // Busy wait for master share to be mounted
    while (!get_master_share()) ;

    // Message passing part
    // --------------------------------------------------

    char *_tmp = malloc(1000);
    sprintf(_tmp, "sync%d", _tid);

    tree_init(_tmp);

    //tree_reset();
    /* tree_connect(qrm_my_name); */

    return 0;
}

/**
 * \brief Return current thread's id.
 */
int get_thread_id(void)
{
    return tid;
}

/**
 * \brief De-initialzie threads
 *
 */
int __thread_end(void)
{
    printf("Thread %d ending\n", tid);
    return 0;
}

/**
 * \brief Get core ID of the current thread/process
 *
 */
int get_core_id(void)
{
    return disp_get_core_id();
}

int get_num_threads(void)
{
    return nproc;
}
