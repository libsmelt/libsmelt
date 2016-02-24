/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#include <smlt.h>
#include <smlt_channel.h>
#include <smlt_context.h>
#include <smlt_broadcast.h>

/**
 * @brief performs a broadcast to the nodes of the subtree routed at the 
 *        calling node
 *
 * @param ctx   the Smelt context to broadcast on
 * @param msg   input for the broadcast
 * 
 * @returns TODO:errval
 */
errval_t smlt_broadcast_subtree(struct smlt_context *ctx,
                                struct smlt_msg *msg)
{
    errval_t err;

    uint32_t count = 0;
    struct smlt_channel *children;
    err =  smlt_context_get_children_channels(ctx, &children, &count);
    if (smlt_err_is_fail(err)) {
        return err; // TODO: adding more error values
    }

    for (uint32_t i = 0; i < count; ++i) {
        err = smlt_channel_send(&children[i], msg);
        if (smlt_err_is_fail(err)) {
            // TODO: error handling
        }
    }

    return SMLT_SUCCESS;
}

/**
 * @brief performs a broadcast without any payload to the subtree routed at
 *        the calling node
 *
 * @param ctx   the Smelt context to broadcast on
 *
 * @returns TODO:errval
 */
errval_t smlt_broadcast_notify_subtree(struct smlt_context *ctx)
{
    errval_t err;

    uint32_t count = 0;
    struct smlt_channel *children;
    err =  smlt_context_get_children_channels(ctx, &children, &count);
    if (smlt_err_is_fail(err)) {
        return err; // TODO: adding more error values
    }

    for (uint32_t i = 0; i < count; ++i) {
        err = smlt_channel_notify(&children[i]);
        if (smlt_err_is_fail(err)) {
            // TODO: error handling
        }
    }

    return SMLT_SUCCESS;
}

/**
 * @brief performs a broadcast to all nodes on the current active instance
 * 
 * @param ctx   the Smelt context to broadcast on
 * @param msg   input for the reduction
 * 
 * @returns TODO:errval
 */
errval_t smlt_broadcast(struct smlt_context *ctx,
                        struct smlt_msg *msg)
{
    errval_t err;

    if (smlt_context_is_root(ctx)) {
        return smlt_broadcast_subtree(ctx, msg);
    } else {
        struct smlt_channel *parent;
        err =  smlt_context_get_parent_channel(ctx, &parent);
        if (smlt_err_is_fail(err)) {
            return err; // TODO: adding more error values
        }

        err = smlt_channel_recv(parent, NULL);
        if (smlt_err_is_fail(err)) {
            // TODO: error handling
        }
        return smlt_broadcast_subtree(ctx, msg);
    }
}

/**
 * @brief performs a broadcast without any payload to all nodes
 *
 * @param ctx   the Smelt context to broadcast on
 *
 * @returns TODO:errval
 */
errval_t smlt_broadcast_notify(struct smlt_context *ctx)
{
    errval_t err;

    if (smlt_context_is_root(ctx)) {
        return smlt_broadcast_notify_subtree(ctx);
    } else {
        struct smlt_channel *parent;
        err =  smlt_context_get_parent_channel(ctx, &parent);
        if (smlt_err_is_fail(err)) {
            return err; // TODO: adding more error values
        }

        err = smlt_channel_recv(parent, NULL);
        if (smlt_err_is_fail(err)) {
            // TODO: error handling
        }
        return smlt_broadcast_notify_subtree(ctx);
    }
}

#if 0
/**
 * \brief
 *  XXX: this should be mergedw ith smlt_broadcast...
 */
uintptr_t ab_forward(uintptr_t val, coreid_t sender)
{
    debug_printfff(DBG__HYBRID_AC, "hybrid_ac entered\n");

    coreid_t my_core_id = get_thread_id();
    bool does_mp = topo_does_mp_receive(my_core_id, false) ||
        (topo_does_mp_send(my_core_id, false));

    // Sender forwards message to the root
    if (get_thread_id()==sender && sender!=get_sequentializer()) {

        mp_send(get_sequentializer(), val);
    }

    // Message passing
    // --------------------------------------------------
    if (does_mp) {

        debug_printfff(DBG__HYBRID_AC, "Starting message passing .. "
                       "does_recv=%d does_send=%d - val=%d\n",
                       topo_does_mp_receive(my_core_id, false),
                       topo_does_mp_send(my_core_id, false),
                       val);

        if (my_core_id == SEQUENTIALIZER) {
            if (sender!=get_sequentializer()) {
                val = mp_receive(sender);
            }
            mp_send_ab(val);
        } else {
            val = mp_receive_forward(0);
        }
    }

    // Shared memory
    // --------------------------------------------------
    if (topo_does_shm_send(my_core_id) || topo_does_shm_receive(my_core_id)) {

        if (topo_does_shm_send(my_core_id)) {

            // send
            // --------------------------------------------------

            debug_printfff(DBG__HYBRID_AC, "Starting SHM .. -- send, \n");

            shm_send(val, 0, 0, 0, 0, 0, 0);
        }
        if (topo_does_shm_receive(my_core_id)) {

            // receive
            // --------------------------------------------------

            debug_printfff(DBG__HYBRID_AC, "Starting SHM .. -- receive, \n");

            uintptr_t val1, val2, val3, val4, val5, val6, val7;
            shm_receive(&val1, &val2, &val3, &val4, &val5, &val6, &val7);
        }

        debug_printfff(DBG__HYBRID_AC, "End SHM\n");
    }

    return val;

}


/**
 * \brief Receive a message from the broadcast tree and forward
 *
 * \param val The value to be added to the received value before
 * forwarding. Useful for reductions, but should probably not be here.
 *
 * \return The message received from the broadcast tree
 */
uintptr_t mp_receive_forward(uintptr_t val)
{
    int parent_core;

    mp_binding *b = mp_get_parent(get_thread_id(), &parent_core);

    debug_printfff(DBG__AB, "Receiving from parent %d\n", parent_core);
    uintptr_t v = mp_receive_raw(b);

    mp_send_ab(v + val);

    return v;
}

#endif
