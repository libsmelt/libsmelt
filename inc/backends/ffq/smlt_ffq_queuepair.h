/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_FFQ_QUEUEPAIR_H_
#define SMLT_FFQ_QUEUEPAIR_H_ 1

#include <ffq/smlt_ffq_queue.h>

struct smlt_qp;
struct smlt_message;


#define SMLT_FFQ_DEFAULT_SLOTS 64

/**
 * represents an bidirectional FastForward queue
 */
struct smlt_ffq_queuepair
{
    struct smlt_ffq_queue tx SMLT_ARCH_ATTR_ALIGN;
    struct smlt_ffq_queue rx SMLT_ARCH_ATTR_ALIGN;
};


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
                                  struct smlt_ffq_queuepair *dst);

/**
 * @brief destorys a FFQ queuepair
 *
 * @param qp    the FFQ queuepair
 *
 * @returns SMLT_SUCCESS or error value
 */
errval_t smlt_ffq_queuepair_destroy(struct smlt_ffq_queuepair *qp);


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
                                 struct smlt_msg *msg);

/**
 * @brief sends a message on the queuepair
 *
 * @param qp     The smelt queuepair to send on
 * @param msg    the Smelt message to send
 *
 * @returns SMELT_SUCCESS of the messessage could be sent.
 */
static inline errval_t smlt_ffq_queuepair_send_raw(struct smlt_ffq_queuepair *qp,
                                                   smlt_ffq_payload_t *val_ptr,
                                                   smlt_ffq_idx_t num)
{
    return smlt_ffq_queue_send_raw(&qp->tx, val_ptr, num);
}

/**
 * @brief sends a notification on the queuepair
 *
 * @param qp     The smelt queuepair to send on
 *
 * @returns SMELT_SUCCESS of the messessage could be sent.
 */
static inline errval_t smlt_ffq_queuepair_notify_raw(struct smlt_ffq_queuepair *qp)
{
    return smlt_ffq_queue_notify(&qp->tx);
}

/**
 * @brief sends a notification on the queuepair
 *
 * @param qp     The smelt queuepair to send on
 *
 * @returns SMELT_SUCCESS of the messessage could be sent.
 */
errval_t smlt_ffq_queuepair_notify(struct smlt_qp *qp);

/**
 * @brief checks if a message can be setn
 *
 * @param qp     The smelt queuepair to send on
 *
 * @returns TRUE if a message can be sent, FALSE otherwise
 */
static inline bool smlt_ffq_queuepair_can_send_raw(struct smlt_ffq_queuepair *qp)
{
    return smlt_ffq_queue_can_send(&qp->tx);
}

/**
 * @brief checks if a message can be setn
 *
 * @param qp     The smelt queuepair to send on
 *
 * @returns TRUE if a message can be sent, FALSE otherwise
 */
bool smlt_ffq_queuepair_can_send(struct smlt_qp *qp);


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
static inline errval_t smlt_ffq_queuepair_recv_raw(struct smlt_ffq_queuepair *qp,
                                                   smlt_ffq_payload_t *val_ptr,
                                                   smlt_ffq_idx_t num)
{
    return smlt_ffq_queue_recv_raw(&qp->rx, val_ptr, num);
}

 /**
  * @brief receives a message on the queuepair
  *
  * @param qp     The smelt queuepair to send on
  * @param msg    the Smelt message to receive in
  *
  * @returns SMELT_SUCCESS of the messessage could be received.
  */
 errval_t smlt_ffq_queuepair_recv(struct smlt_qp *qp,
                                  struct smlt_msg *msg);



/**
 * @brief receives a notification on the queuepair
 *
 * @param qp     The smelt queuepair to send on
 *
 * @returns SMELT_SUCCESS of the messessage could be received.
 */
static inline errval_t smlt_ffq_queuepair_recv_notify_raw(struct smlt_ffq_queuepair *qp)
{
    return smlt_ffq_queue_recv_raw(&qp->rx, NULL, 0);
}

/**
 * @brief receives a notification on the queuepair
 *
 * @param qp     The smelt queuepair to send on
 *
 * @returns SMELT_SUCCESS of the messessage could be received.
 */
 errval_t smlt_ffq_queuepair_recv_notify(struct smlt_qp *qp);

/**
 * @brief checks if a message can be received (polling)
 *
 * @param qp     The smelt queuepair to send on
 *
 * @returns TRUE if a message can be received, FALSE otherwise
 */
bool smlt_ffq_queuepair_can_recv(struct smlt_qp *qp);

/**
 * @brief checks if a message can be received (polling)
 *
 * @param qp     The smelt queuepair to send on
 *
 * @returns TRUE if a message can be received, FALSE otherwise
 */
static inline bool smlt_ffq_queuepair_can_recv_raw(struct smlt_ffq_queuepair *qp)
{
    return smlt_ffq_queue_can_recv(&qp->rx);
}

#endif /*SMLT_FFQ_QUEUEPAIR_H_ */
