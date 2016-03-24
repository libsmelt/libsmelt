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
#include <ffq/smlt_ffq_queue.h>

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
errval_t smlt_ffq_queue_init_tx(struct smlt_ffq_queue *q, void *buf,
                                smlt_ffq_idx_t slots)
{
    q->direction = SMLT_FFQ_DIRECTION_SEND;
    q->size = slots;
    q->slots = (volatile struct smlt_ffq_slot *)buf;
    q->pos = 0;
    for (smlt_ffq_idx_t i = 0; i < slots; ++i) {
        q->slots[i].data[0] = SMLT_FFQ_SLOT_EMPTY;
    }

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
errval_t smlt_ffq_queue_init_rx(struct smlt_ffq_queue *q, void *buf,
                                smlt_ffq_idx_t slots)
{
    q->direction = SMLT_FFQ_DIRECTION_RECV;
    q->size = slots;
    q->slots = (volatile struct smlt_ffq_slot *)buf;
    q->pos = 0;

    return SMLT_SUCCESS;
}
