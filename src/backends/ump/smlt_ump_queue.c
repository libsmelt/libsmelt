/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <stdio.h>

#include <limits.h>
#include <string.h>

#include <smlt.h>
#include <ump/smlt_ump_queue.h>

/**
 * @brief verifies the alginment and size of the buffer
 *
 * @bparam buf      buffer to check
 * @param size_t    size of the buffer to check
 *
 * @returns SMLT_SUCESS or error value on bad alignment
 */
static errval_t check_alignment(void *buf, size_t size)
{
    // check alignment and size of buffer.
    if (size == 0 || (size > UINT16_MAX)) {
        printf("ALIGNMENT SIZE\n");
        return SMLT_ERR_BAD_ALIGNMENT;
    }

    if (buf == NULL || (((uintptr_t)buf) % SMLT_UMP_MSG_BYTES) != 0) {
        printf("ALIGNMENT BUFFER\n");
        return SMLT_ERR_BAD_ALIGNMENT;
    }

    return SMLT_SUCCESS;
}

/**
 * @brief initializes the common part of an UMP queue
 *
 * @param       c       Pointer to queue-state structure to initialize.
 * @param       buf     Pointer to ring buffer for the queue.
 * @param       slots   Size (in slots) of buffer.
 *
 * @returns SMLT_SUCCESS or error value
 */
static errval_t smlt_ump_queue_init_common(struct smlt_ump_queue *q, void *buf,
                                           smlt_ump_idx_t slots)
{
    errval_t err;

    err = check_alignment(buf, slots);
    if (smlt_err_is_fail(err)) {
        return err;
    }

    q->num_msg = slots - 1;
    q->epoch = 1;

    q->mbuf = (struct smlt_ump_message*) ((uintptr_t)buf + SMLT_UMP_MSG_BYTES);
    q->mcurr = (volatile struct smlt_ump_message*)q->mbuf;
    q->mlast = q->mbuf + q->num_msg;
    q->last_ack = (smlt_ump_idx_t *) buf;

    assert(((uintptr_t)q->mbuf & (SMLT_UMP_MSG_BYTES -1)) == 0 );


    return SMLT_SUCCESS;
}

/**
 * @brief Initialize UMP transmit queue state
 *
 * @param       c       Pointer to queue-state structure to initialize.
 * @param       buf     Pointer to ring buffer for the queue.
 * @param       slots   Size (in slots) of buffer.
 *
 * @returns SMLT_SUCCESS or error value
 *
 * The state structure and buffer must already be allocated and appropriately
 * aligned.
 */
errval_t smlt_ump_queue_init_tx(struct smlt_ump_queue *q, void *buf,
                                smlt_ump_idx_t slots)
{
    errval_t err;

    err = smlt_ump_queue_init_common(q, buf, slots);
    if (smlt_err_is_fail(err)) {
        return err;
    }

    q->direction = SMLT_UMP_DIRECTION_SEND;

    /*
     * we clear the buffer of the messages, this has to be done on the sending
     * side
     */
    memset(buf, 0, slots* SMLT_UMP_MSG_BYTES);

    SMLT_ASSERT(q->buf && q->epoch);

    return SMLT_SUCCESS;
}

/**
 * @brief Initialize UMP receive queue state
 *
 * @param       c       Pointer to queue-state structure to initialize.
 * @param       buf     Pointer to ring buffer for the queue.
 * @param       slots   Size (in slots) of buffer.
 *
 * @returns SMLT_SUCCESS or error value
 *
 * The state structure and buffer must already be allocated and appropriately
 * aligned.
 */
errval_t smlt_ump_queue_init_rx(struct smlt_ump_queue *q, void *buf,
                                smlt_ump_idx_t slots)
{
    errval_t err;

    err = smlt_ump_queue_init_common(q, buf, slots);
    if (smlt_err_is_fail(err)) {
        return err;
    }

    q->direction = SMLT_UMP_DIRECTION_RECV;

    SMLT_ASSERT(q->buf);

    return SMLT_SUCCESS;
}
