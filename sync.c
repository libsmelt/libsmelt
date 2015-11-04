#include "stdio.h"

#include "shm.h"
#include "sync.h"
#include "topo.h"
#include "internal.h"

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
    printf("Initializing libsync .. model has %d nodes\n", topo_num_cores());
    
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

    tree_init(_tmp);

    tree_reset();
    tree_connect("DUMMY");

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
    printf("Thread %d ending\n", tid);
    return 0;
}

unsigned int get_num_threads(void)
{
    return nproc;
}
