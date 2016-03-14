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
#include <ump/smlt_ump_queuepair.h>


/*
 * ===========================================================================
 * creation and destruction of a queue pair
 * ===========================================================================
 */

errval_t smlt_ump_queuepair_init(smlt_ump_idx_t num_slots, uint8_t node_src,
                                  uint8_t node_dst,
                                  struct smlt_ump_queuepair *src,
                                  struct smlt_ump_queuepair *dst)
{
    errval_t err;

    printf("creating UMP queuepair\n");

    /* make sure the number of slots is even */
    if (num_slots < 2) {
        printf("INVAL\n");
        return SMLT_ERR_INVAL;
    }

    memset(src, 0, sizeof(*src));
    memset(dst, 0, sizeof(*dst));

    uint64_t chan_size = (num_slots + 1)  * SMLT_UMP_MSG_BYTES;

    /* initialize queue SRC->DST */
    void *shm_src = smlt_platform_alloc_on_node(chan_size, BASE_PAGE_SIZE,
                                                node_dst, true);
    if (shm_src == NULL) {
        return SMLT_ERR_MALLOC_FAIL;
    }

    err = smlt_ump_queue_init_tx(&src->tx, shm_src, num_slots);
    if (smlt_err_is_fail(err)) {
        smlt_platform_free(shm_src);
        return smlt_err_push(err, SMLT_ERR_QUEUE_INIT);
    }

    err = smlt_ump_queue_init_rx(&dst->rx, shm_src, num_slots);
    if (smlt_err_is_fail(err)) {
        smlt_platform_free(shm_src);
        return smlt_err_push(err, SMLT_ERR_QUEUE_INIT);
    }

    /* initialize queue DST->SRC */
    void *shm_dst = smlt_platform_alloc_on_node(chan_size, BASE_PAGE_SIZE,
                                                node_src, true);
    if (shm_dst == NULL) {
        smlt_platform_free(shm_src);
        return SMLT_ERR_MALLOC_FAIL;
    }


    err = smlt_ump_queue_init_rx(&src->rx, shm_dst, num_slots);
    if (smlt_err_is_fail(err)) {
        smlt_platform_free(shm_src);
        smlt_platform_free(shm_dst);
        return smlt_err_push(err, SMLT_ERR_QUEUE_INIT);
    }

    err = smlt_ump_queue_init_tx(&dst->tx, shm_dst, num_slots);
    if (smlt_err_is_fail(err)) {
        smlt_platform_free(shm_src);
        smlt_platform_free(shm_dst);
        return smlt_err_push(err, SMLT_ERR_QUEUE_INIT);
    }

    src->seq_id = 1;
    dst->seq_id = 1;

    /* do the linkage */
    src->other = dst;
    dst->other = src;

    return SMLT_SUCCESS;
}


errval_t smlt_ump_queuepair_destroy(struct smlt_ump_queuepair *qp)
{
    // TODO how to destroy a ump queuepair?
    return SMLT_SUCCESS;
}




/**
 * @brief sends a message on the queuepair
 *
 * @param qp     The smelt queuepair to send on
 * @param msg    the Smelt message to send
 *
 * @returns SMELT_SUCCESS of the messessage could be sent.
 */
errval_t smlt_ump_queuepair_try_send(struct smlt_qp *qp,
                                     struct smlt_msg *msg)
{
    errval_t err;
    struct smlt_ump_message *m;

    SMLT_ASSERT(qp->type == SMLT_QP_TYPE_UMP);

    struct smlt_ump_queuepair *ump = &qp->q.ump;

    err = smlt_ump_queuepair_prepare_send(ump, &m);
    if (smlt_err_is_fail(err)) {
        return SMLT_ERR_QUEUE_FULL;
    }

    for (uint32_t i = 0; i < msg->words; ++i) {
        m->data[i] = msg->data[i];
    }

    return smlt_ump_queuepair_send_raw(ump, m);
}
/**
* @brief sends a notification on the queuepair
*
* @param qp     The smelt queuepair to send on
*
* @returns SMELT_SUCCESS of the messessage could be sent.
*/
errval_t smlt_ump_queuepair_notify(struct smlt_qp *qp)
{
    struct smlt_ump_queuepair *ump = &qp->q.ump;

    while(!smlt_ump_queuepair_can_send_raw(ump))
        ;

    return smlt_ump_queuepair_notify_raw(ump);
}

/**
* @brief checks if a message can be setn
*
* @param qp     The smelt queuepair to send on
*
* @returns TRUE if a message can be sent, FALSE otherwise
*/
bool smlt_ump_queuepair_can_send(struct smlt_qp *qp)
{
    return smlt_ump_queuepair_can_send_raw(&qp->q.ump);
}


/**
 * @brief receives a message on the queuepair
 *
 * @param qp     The smelt queuepair to send on
 * @param msg    the Smelt message to receive in
 *
 * @returns SMELT_SUCCESS of the messessage could be received.
 */
errval_t smlt_ump_queuepair_try_recv(struct smlt_qp *qp,
                                     struct smlt_msg *msg)
{
    errval_t err;
    struct smlt_ump_message *m;

    SMLT_ASSERT(qp->type == SMLT_QP_TYPE_UMP);

    struct smlt_ump_queuepair *ump = &qp->q.ump;

    err =  smlt_ump_queuepair_recv_raw(ump, &m);
    if (smlt_err_is_fail(err)) {
        return err;
    }

    for (uint32_t i = 0; i < msg->words; ++i) {
        msg->data[i] = m->data[i];
    }

    return SMLT_SUCCESS;
}

/**
* @brief receives a notification on the queuepair
*
* @param qp     The smelt queuepair to send on
*
* @returns SMELT_SUCCESS of the messessage could be received.
*/
errval_t smlt_ump_queuepair_recv_notify(struct smlt_qp *qp)
{
    struct smlt_ump_queuepair *ump = &qp->q.ump;

    while(!smlt_ump_queuepair_can_recv_raw(ump))
        ;
    return smlt_ump_queuepair_recv_raw(ump, NULL);
}

/**
* @brief checks if a message can be received (polling)
*
* @param qp     The smelt queuepair to send on
*
* @returns TRUE if a message can be received, FALSE otherwise
*/
bool smlt_ump_queuepair_can_recv(struct smlt_qp *qp)
{
    return smlt_ump_queuepair_can_recv_raw(&qp->q.ump);
}
