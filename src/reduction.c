#include <smlt.h>
#include <smlt_reduce.h>

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


/**
 * @brief performs a reduction on the current instance
 * 
 * @param msg       input for the reduction
 * @param result    returns the result of the reduction
 * 
 * @returns TODO:errval
 */
errval_t smlt_reduce(struct smlt_msg *input,
                     struct smlt_msg *result)
{
    if (smlt_current_instance()->has_shm) {
        shm_reduce(input, result);
        if (smlt_err_fail(err)) {
            return err;
        }
    }

    return smlt_current_instance()->reduce(input, result);
}

/**
 * @brief performs a reduction without any payload on teh current instance
 *
 * @returns TODO:errval
 */
errval_t smlt_reduce_notify(void)
{
    errval_t err;

    if (smlt_current_instance()->has_shm) {
        err = shm_reduce_notify();
        if (smlt_err_fail(err)) {
            return err;
        }
    }

    return smlt_current_instance()->reduce_notify();
}

/**
 * @brief performs a reduction and distributes the result to all nodes
 * 
 * @param msg       input for the reduction
 * @param result    returns the result of the reduction
 * 
 * @returns TODO:errval
 */
errval_t smlt_reduce_all(struct smlt_msg *input,
                         struct smlt_msg *result)
{
    errval_t err;

    err = smlt_reduce(input, result);
    if (smlt_err_fail(err)) {
        return err;
    }

    return smlt_broadcast(result);
}