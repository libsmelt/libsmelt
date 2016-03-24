/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_FFQ_QUEUE_H_
#define SMLT_FFQ_QUEUE_H_ 1

#include <smlt.h>

///< index into the FastForward queue
typedef uint16_t smlt_ffq_idx_t;

///< payload type of the fast forward queue
typedef uint64_t smlt_ffq_payload_t;

///< an empty FFQ slot has this value
#define SMLT_FFQ_SLOT_EMPTY ((smlt_ffq_payload_t)-1)

#define SMLT_FFQ_SLOT_NOTIFY ((smlt_ffq_payload_t)0)

/**
 * the size of a FFQ message in bytes
 * this should be a multiple of the archteicture's cacheline
 */
#define SMLT_FFQ_MSG_BYTES  (1 * SMLT_ARCH_CACHELINE_SIZE)

/**
 * the number of (payload) words a message consists of.
 */
#define SMLT_FFQ_MSG_WORDS  (SMLT_FFQ_MSG_BYTES / sizeof(smlt_ffq_payload_t))


struct SMLT_ARCH_ATTR_ALIGN smlt_ffq_slot
{
    smlt_ffq_payload_t data[SMLT_FFQ_MSG_WORDS];
};

/* the size of a contorl word had to be smaller than the payload word */
SMLT_STATIC_ASSERT(sizeof(struct smlt_ffq_slot) == SMLT_FFQ_MSG_BYTES);


typedef enum {
    SMLT_FFQ_DIRECTION_SEND,
    SMLT_FFQ_DIRECTION_RECV
} smlt_ffq_direction_t;


struct smlt_ffq_queue
{
    volatile struct smlt_ffq_slot *slots;
    smlt_ffq_idx_t size;
    smlt_ffq_idx_t pos;
    smlt_ffq_direction_t direction;
};

//struct smlt_ffq_rx
//{
//    struct smlt_ffq_slot *slots;
//    smlt_ffq_idx_t size;
//    smlt_ffq_idx_t tail;
//};

/*
 * ===========================================================================
 * FFQ channel initialization
 * ===========================================================================
 */


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
                                smlt_ffq_idx_t slots);

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
                                smlt_ffq_idx_t slots);


/*
 * ===========================================================================
 * Send Functions
 * ===========================================================================
 */

 /**
  * @brief obtains a pointer to the next message to be used for sending
  *
  * @param c     the UMP channel to get the next message for
  *
  * @return pointer to a message slot
  */
static inline volatile
struct smlt_ffq_slot *smlt_ffq_queue_get_next(struct smlt_ffq_queue *q)
{
    SMLT_ASSERT(q->direction == SMLT_FFQ_DIRECTION_SEND);

    return q->slots + q->pos;
}

/**
 * @brief checks if a message can be sent on that queue-state
 *
 * @param q the ffq queue
 *
 * @returns TRUE iff a message can be sent, FALSE otherwise
 */
static inline bool smlt_ffq_queue_can_send(struct smlt_ffq_queue *q)
{
    SMLT_ASSERT(c);
    SMLT_ASSERT(c->slots);

    /* check the direction */
    if (q->direction != SMLT_FFQ_DIRECTION_SEND) {
        return false;
    }

    volatile struct smlt_ffq_slot *slot = q->slots + q->pos;

    return (slot->data[0] == SMLT_FFQ_SLOT_EMPTY);
}


 /**
  * @brief sends a message on the UMP channel
  *
  * @param c     the UMP channel to send on
  * @param msg   pointer to the message slot in the channel`
  * @param ctrl  partial control word for the message
  */
static inline errval_t smlt_ffq_queue_send_raw(struct smlt_ffq_queue *q,
                                               smlt_ffq_payload_t *val_ptr,
                                               smlt_ffq_idx_t num)
{
    if (!smlt_ffq_queue_can_send(q)) {
        return SMLT_ERR_QUEUE_FULL;
    }

    volatile struct smlt_ffq_slot *s = q->slots + q->pos;

    if (num > SMLT_FFQ_MSG_WORDS) {
        num = SMLT_FFQ_MSG_WORDS;
    }

    for (smlt_ffq_idx_t i = 1; i < num; ++i) {
        s->data[i] = val_ptr[i];
    }

    smlt_arch_write_barrier();

    s->data[0] = val_ptr[0];

    if (++q->pos == q->size) {
        q->pos = 0;
    }

    return SMLT_SUCCESS;
}


 /**
  * @brief sends a notification on the UMP channel
  *
  * @param c     the UMP channel to send on
  * @param msg   pointer to the message slot in the channel`
  * @param ctrl  partial control word for the message
  */
 static inline errval_t smlt_ffq_queue_notify(struct smlt_ffq_queue *q)
 {
    if (!smlt_ffq_queue_can_send(q)) {
        return SMLT_ERR_QUEUE_FULL;
    }
    volatile struct smlt_ffq_slot *s = q->slots + q->pos;

    SMLT_ASSERT(s->data[0] == SMLT_FFQ_SLOT_EMTPY);

    s->data[0] = SMLT_FFQ_SLOT_NOTIFY;

    if (++q->pos == q->size) {
        q->pos = 0;
    }

    return SMLT_SUCCESS;
 }


 /*
  * ===========================================================================
  * Recveive Functions
  * ===========================================================================
  */

/**
* @brief checks if there is a message to be received
*
* @param c     the UMP channel to be polled
*
* @returns TRUE if there is a pending message, false otherwise
*/
static inline volatile bool smlt_ffq_queue_can_recv(struct smlt_ffq_queue *c)
{
    SMLT_ASSERT(c);
    SMLT_ASSERT(c->slots);

    /* check the direction */
    if (c->direction != SMLT_FFQ_DIRECTION_RECV) {
        return false;
    }

    volatile struct smlt_ffq_slot *slot = c->slots + c->pos;

    return (slot->data[0] != SMLT_FFQ_SLOT_EMPTY);
}

/**
 * @brief Receives a pointer to an outsanding message
 *
 * @param c     the UMP channel to be received on
 *
 * @returns pointer to the message or NULL otherwise
 *
 * the returned pointer is the raw UMP message in the channel and will be
 * overwritten on reuse.
 */
static inline errval_t smlt_ffq_queue_recv_raw(struct smlt_ffq_queue *q,
                                               smlt_ffq_payload_t *val_ptr,
                                               smlt_ffq_idx_t num)
{
    SMLT_ASSERT(q);
    SMLT_ASSERT(q->direction != smlt_ffq_DIRECTION_RECV);

    volatile struct smlt_ffq_slot *s = q->slots + q->pos;

    if (s->data[0] == SMLT_FFQ_SLOT_EMPTY) {
        return SMLT_ERR_QUEUE_EMPTY;
    }

    if (val_ptr) {
        if (num > SMLT_FFQ_MSG_WORDS) {
            num = SMLT_FFQ_MSG_WORDS;
        }

        for (smlt_ffq_idx_t i = 0; i < num; ++i) {
            val_ptr[i] = s->data[i];
        }
    }

    s->data[0] = SMLT_FFQ_SLOT_EMPTY;

    if (++q->pos == q->size) {
        q->pos = 0;
    }

    return SMLT_SUCCESS;
}


#endif /* SMLT_FFQ_QUEUE_H_ */
