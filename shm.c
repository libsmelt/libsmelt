#include "sync.h"
#include "shm.h"

#ifdef BARRELFISH
#include <barrelfish/barrelfish.h>
#endif

// Master share is shared between all threads.
union quorum_share *master_share = NULL;
#ifdef BARRELFISH
struct capref shared_frame;
#endif

int init_master_share(void)
{
#ifdef BARRELFISH    
    errval_t err;

    if (master_share)
        return 0;

    // Sanity checks
    assert (SHM_SIZE % BASE_PAGE_SIZE==0);
    assert (sizeof(union quorum_share)<=SHM_SIZE);

    err = frame_alloc(&shared_frame, SHM_SIZE, NULL);
    assert (err_is_ok(err));

    err = vspace_map_one_frame_attr(((void**)&master_share), SHM_SIZE,
                                    shared_frame, VREGION_FLAGS_READ_WRITE,
                                    NULL, NULL);
    assert(err_is_ok(err));

    return !err_is_ok(err);
#else
    if (master_share)
        return 0;

    // Sanity checks
    assert (SHM_SIZE % BASE_PAGE_SIZE==0);
    assert (sizeof(union quorum_share)<=SHM_SIZE);

    master_share = (union quorum_share*) malloc(SHM_SIZE);
    assert (master_share!=NULL);
    
    return 0;
#endif
}

int map_master_share(void)
{
    // This is a no-op if we use threads for parallelism.

    // If processes were used, we would have to map the frame here

    return 0;
}

union quorum_share* get_master_share(void)
{
    return master_share;
}
