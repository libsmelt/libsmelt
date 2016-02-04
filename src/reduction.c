#include "sync.h"
#include "topo.h"
#include "shm.h"
#include "mp.h"

/**
 * \brief
 *
 */
uintptr_t sync_reduce(uintptr_t val)
{
    // --------------------------------------------------
    // Shared memory first

    // Note: tree is processed in backwards direction, i.e. a receive
    // corresponds to a send and vica-versa.

    uintptr_t current_aggregate = val;

    current_aggregate += shm_reduce(current_aggregate);

    // --------------------------------------------------
    // Message passing

    /*
     * Each client receives (potentially from several children) and
     * sends only ONCE. On which level of the tree hierarchy this is
     * does not matter. If a client would send several messages, we
     * would have circles in the tree.
     */
    return mp_reduce(current_aggregate);
}

uintptr_t sync_reduce0(uintptr_t val)
{
    // --------------------------------------------------
    // Shared memory first

    // Note: tree is processed in backwards direction, i.e. a receive
    // corresponds to a send and vica-versa.

    uintptr_t current_aggregate = val;

    current_aggregate += shm_reduce(current_aggregate);

    // --------------------------------------------------
    // Message passing

    /*
     * Each client receives (potentially from several children) and
     * sends only ONCE. On which level of the tree hierarchy this is
     * does not matter. If a client would send several messages, we
     * would have circles in the tree.
     */
    mp_reduce0();
    return 0;
}
