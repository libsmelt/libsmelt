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
#include <smlt_reduction.h>
#include <smlt_broadcast.h>
#include <shm/smlt_shm.h>
#include "debug.h"
#include <string.h>


/**
 * @brief performs a reduction on the current instance
 * 
 * @param ctx       The smelt context
 * @param msg        input for the reduction
 * @param result     returns the result of the reduction
 * @param operation  function to be called to calculate the aggregate
 *
 * @returns TODO:errval
 */
errval_t smlt_reduce(struct smlt_context *ctx,
                     struct smlt_msg *input,
                     struct smlt_msg *result,
                     smlt_reduce_fn_t operation)
{
    errval_t err;

    if (!operation) {
        return smlt_reduce_notify(ctx);
    }

    // --------------------------------------------------
    // Message passing

    /*
     * Each client receives (potentially from several children) and
     * sends only ONCE. On which level of the tree hierarchy this is
     * does not matter. If a client would send several messages, we
     * would have circles in the tree.
     */


    operation(result, input);

    uint32_t count = 0;
    struct smlt_channel *children;
    err =  smlt_context_get_children_channels(ctx, &children, &count);
    if (smlt_err_is_fail(err)) {
        return err; // TODO: adding more error values
    }

    // Receive (this will be from several children)
    // --------------------------------------------------
    bool recv[count];
    memset(recv, 0, sizeof(bool)*count);
    int num_recv = 0;
    int i = 0;
    while( num_recv < count) {
        if (!recv[i] && smlt_channel_can_recv(&children[i])) {
            err = smlt_channel_recv(&children[i], result);
            if (smlt_err_is_fail(err)) {
                return err;
            }
            recv[i] = true;
            num_recv++;
        }

        i++;

        if (i == count) {
            i = 0;
        }
    }
    /*
    for (uint32_t i = 0; i < count; ++i) {

        err = smlt_channel_recv(&children[i], result);
        if (smlt_err_is_fail(err)) {
            // TODO: error handling
        }

        err = operation(input, result);
        if (smlt_err_is_fail(err)) {
            // TODO: error handling
        }
    }
    */
    // Receive (this will be from several children)
    // --------------------------------------------------
    struct smlt_channel *parent;
    err =  smlt_context_get_parent_channel(ctx, &parent);
    if (smlt_err_is_fail(err)) {
        return err; // TODO: adding more error values
    }

    if (parent) {
        smlt_channel_send(parent, result);
    }

    return SMLT_SUCCESS;
}

/**
 * @brief checks if the children already send something for the reduction
 *
 * @param ctx       The smelt context
 *
 * @returns bool if there is something to receive
 */
bool smlt_reduce_can_recv(struct smlt_context *ctx)
{
    errval_t err;
    uint32_t count = 0;
    struct smlt_channel *children;
    err =  smlt_context_get_children_channels(ctx, &children, &count);
    if (smlt_err_is_fail(err)) {
        return err; // TODO: adding more error values
    }

    for (int i = 0; i < count; i++) {
        if (smlt_channel_can_recv(&children[i])) {
            return true;
        }
    }
    return false;
}
/**
 * @brief performs a reduction without any payload on teh current instance
 *
 * @param ctx       The smelt context
 *
 * @returns TODO:errval
 */
errval_t smlt_reduce_notify(struct smlt_context *ctx)
{
    errval_t err;

    // --------------------------------------------------
    // Message passing

    /*
     * Each client receives (potentially from several children) and
     * sends only ONCE. On which level of the tree hierarchy this is
     * does not matter. If a client would send several messages, we
     * would have circles in the tree.
     */


    // Receive (this will be from several children)
    // --------------------------------------------------
    uint32_t count = 0;
    struct smlt_channel *children;
    err =  smlt_context_get_children_channels(ctx, &children, &count);
    if (smlt_err_is_fail(err)) {
        return err; // TODO: adding more error values
    }


    bool recv[count];
    memset(recv, 0, sizeof(bool)*count);
    int num_recv = 0;
    int i = 0;
    while( num_recv < count) {
        if (!recv[i] && smlt_channel_can_recv(&children[i])) {
            err = smlt_channel_recv_notification(&children[i]);
            if (smlt_err_is_fail(err)) {
                return err;
            }
            recv[i] = true;
            num_recv++;
        }

        i++;

        if (i == count) {
            i = 0;
        }
    }
/*
    for (uint32_t i = 0; i < count; ++i) {

 //       SMLT_DEBUG(SMLT_DBG__REDUCE, "Node %d: reduction recv from chan %p \n",
 //                  smlt_node_get_id(), (void*) &children[i].recv->queue_rx);

        err = smlt_channel_recv_notification(&children[i]);
        // TODO: error handling
    }
*/
    // Send (this should only be sending one message)
    // --------------------------------------------------
    struct smlt_channel *parent;
    err =  smlt_context_get_parent_channel(ctx, &parent);
    if (smlt_err_is_fail(err)) {
        return err; // TODO: adding more error values
    }

    if (parent) {

   //     SMLT_DEBUG(SMLT_DBG__REDUCE, "Node %d: reduction send to chan %p \n",
   //                smlt_node_get_id(), (void*) &parent->send->queue_tx);
        smlt_channel_notify(parent);
    }

    return SMLT_SUCCESS;
}


/**
 * @brief performs a reduction and distributes the result to all nodes
 * 
 * @param ctx       The smelt context
 * @param msg       input for the reduction
 * @param result    returns the result of the reduction
 * @param operation  function to be called to calculate the aggregate
 * 
 * @returns TODO:errval
 */
errval_t smlt_reduce_all(struct smlt_context *ctx,
                         struct smlt_msg *input,
                         struct smlt_msg *result,
                         smlt_reduce_fn_t operation)
{
    errval_t err;

    err = smlt_reduce(ctx, input, result, operation);
    if (smlt_err_is_fail(err)) {
        return err;
    }

    return smlt_broadcast(ctx, result);
}
