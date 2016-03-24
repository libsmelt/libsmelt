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
#include <smlt_queuepair.h>
#include <smlt_message.h>
#include <ffq/smlt_ffq_queuepair.h>


/**
 * @brief creats a FFQ queuepair
 *
 * @param num_slots   the number of slots in the queue
 * @param
 *
 * @returns SMLT_SUCCESS or error value
 */
errval_t smlt_ffq_queuepair_init(smlt_ffq_idx_t num_slots, uint8_t node_src,
                                  uint8_t node_dst,
                                  struct smlt_ffq_queuepair *src,
                                  struct smlt_ffq_queuepair *dst)
{
    errval_t err;


    void *shm_src = NULL;
    void *shm_dst = NULL;

    size_t chan_size = num_slots * SMLT_FFQ_MSG_BYTES;

    /* initialize queue SRC->DST */
    shm_src = smlt_platform_alloc_on_node(chan_size, BASE_PAGE_SIZE,
                                                node_src, true);
    if (shm_src == NULL) {
        return SMLT_ERR_MALLOC_FAIL;
    }

    err = smlt_ffq_queue_init_rx(&src->rx, shm_src, num_slots);
    if (smlt_err_is_fail(err)) {
        goto err_init;
    }
    err = smlt_ffq_queue_init_tx(&dst->tx, shm_src, num_slots);
    if (smlt_err_is_fail(err)) {
        goto err_init;
    }


    /* initialize queue SRC->DST */
    shm_dst = smlt_platform_alloc_on_node(chan_size, BASE_PAGE_SIZE,
                                                node_dst, true);
    if (shm_dst == NULL) {
        goto err_init;
    }

    err = smlt_ffq_queue_init_rx(&dst->rx, shm_dst, num_slots);
    if (smlt_err_is_fail(err)) {
        goto err_init;
    }
    err = smlt_ffq_queue_init_tx(&src->tx, shm_dst, num_slots);
    if (smlt_err_is_fail(err)) {
        goto err_init;
    }

    return SMLT_SUCCESS;

    err_init:
    if (shm_src) {
        smlt_platform_free(shm_src);
    }
    if (shm_dst) {
        smlt_platform_free(shm_dst);
    }
    return SMLT_ERR_QUEUE_INIT;

}

/**
 * @brief destorys a FFQ queuepair
 *
 * @param qp    the FFQ queuepair
 *
 * @returns SMLT_SUCCESS or error value
 */
errval_t smlt_ffq_queuepair_destroy(struct smlt_ffq_queuepair *qp)
{
    return SMLT_SUCCESS;
}


/*
 * ===========================================================================
 * Send Functions
 * ===========================================================================
 */

/**
 * @brief sends a message on the queuepair
 *
 * @param qp     The smelt queuepair to send on
 * @param msg    the Smelt message to send
 *
 * @returns SMELT_SUCCESS of the messessage could be sent.
 */
errval_t smlt_ffq_queuepair_send(struct smlt_qp *qp,
                                 struct smlt_msg *msg)
{
    struct smlt_ffq_queuepair *ffq = &qp->q.ffq;
    return smlt_ffq_queuepair_send_raw(ffq, msg->data, msg->words);
}

/**
* @brief sends a notification on the queuepair
*
* @param qp     The smelt queuepair to send on
*
* @returns SMELT_SUCCESS of the messessage could be sent.
*/
errval_t smlt_ffq_queuepair_notify(struct smlt_qp *qp)
{
    struct smlt_ffq_queuepair *ffq = &qp->q.ffq;
    return smlt_ffq_queuepair_notify_raw(ffq);
}

/**
* @brief checks if a message can be setn
*
* @param qp     The smelt queuepair to send on
*
* @returns TRUE if a message can be sent, FALSE otherwise
*/
bool smlt_ffq_queuepair_can_send(struct smlt_qp *qp)
{
    struct smlt_ffq_queuepair *ffq = &qp->q.ffq;
    return smlt_ffq_queuepair_can_send_raw(ffq);
}


/*
 * ===========================================================================
 * Receive Functions
 * ===========================================================================
 */

 /**
  * @brief receives a message on the queuepair
  *
  * @param qp     The smelt queuepair to send on
  * @param msg    the Smelt message to receive in
  *
  * @returns SMELT_SUCCESS of the messessage could be received.
  */
errval_t smlt_ffq_queuepair_recv(struct smlt_qp *qp,
                                 struct smlt_msg *msg)
{
    struct smlt_ffq_queuepair *ffq = &qp->q.ffq;
    return smlt_ffq_queuepair_recv_raw(ffq, msg->data, msg->words);
}

 /**
 * @brief receives a notification on the queuepair
 *
 * @param qp     The smelt queuepair to send on
 *
 * @returns SMELT_SUCCESS of the messessage could be received.
 */
 errval_t smlt_ffq_queuepair_recv_notify(struct smlt_qp *qp)
 {
    struct smlt_ffq_queuepair *ffq = &qp->q.ffq;
    return smlt_ffq_queuepair_recv_raw(ffq, NULL, 0);
 }

 /**
 * @brief checks if a message can be received (polling)
 *
 * @param qp     The smelt queuepair to send on
 *
 * @returns TRUE if a message can be received, FALSE otherwise
 */
 bool smlt_ffq_queuepair_can_recv(struct smlt_qp *qp)
 {
    struct smlt_ffq_queuepair *ffq = &qp->q.ffq;
    return smlt_ffq_queuepair_can_recv_raw(ffq);
 }
