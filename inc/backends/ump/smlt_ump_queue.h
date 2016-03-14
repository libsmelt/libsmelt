/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_UMP_QUEUE_H_
#define SMLT_UMP_QUEUE_H_ 1

#include <stdbool.h>
#include <stdint.h>
#include <arch/x86_64.h>
#include <smlt.h>

#include <stdio.h>


/*
 * ===========================================================================
 * type definitions
 * ===========================================================================
 */


/**
 * the UMP control word for the header.
 */
typedef  uint32_t smlt_ump_ctrl_word_t;

/**
 * size of a payload word in the message
 */
typedef uint64_t smlt_ump_payload_word_t;

/* the size of a contorl word had to be smaller than the payload word */
SMLT_STATIC_ASSERT(sizeof(smlt_ump_ctrl_word_t) <= sizeof(smlt_ump_payload_word_t));

/**
 * the size of a UMP message in bytes
 * this should be a multiple of the archteicture's cacheline
 */
#define SMLT_UMP_MSG_BYTES  (1 * SMLT_ARCH_CACHELINE_SIZE)

#define SMLT_UMP_DEFAULT_SLOTS (BASE_PAGE_SIZE / SMLT_UMP_MSG_BYTES)

/**
 * the number of (payload) words a message consists of.
 */
#define SMLT_UMP_MSG_WORDS  (SMLT_UMP_MSG_BYTES / sizeof(smlt_ump_payload_word_t))

/**
 * maximum number of words that can be sent in a single message
 * this is the number of words per message minus the header word
 */
#define SMLT_UMP_PAYLOAD_WORDS  (SMLT_UMP_MSG_WORDS - 1)

#define SMLT_UMP_PAYLOAD_BYTES (SMLT_UMP_PAYLOAD_WORDS * sizeof(smlt_ump_payload_word_t))


/*
 * UMP control structure
 * ---------------------------------------------------------------------------
 */

/**
 * represents a slot of the UMP channel. this limits the maximum number of
 * messages available in the channel
 */
typedef uint16_t smlt_ump_idx_t;

///< number of bits of the index
#define SMLT_UMP_IDX_BITS         (sizeof(smlt_ump_idx_t) * SMLT_ARCH_CHAR_BITS)

///< mask for index bits
#define SMLT_UMP_IDX_MASK         ((((uintptr_t)1) << SMLT_UMP_IDX_BITS) - 1)

/**
 * number of bits for the epoch
 */
#define SMLT_UMP_EPOCH_BITS  1

/**
 * number of bits available to encode the header
 */
#define SMLT_UMP_HEADER_BITS (sizeof(smlt_ump_ctrl_word_t) * SMLT_ARCH_CHAR_BITS \
                            - SMLT_UMP_EPOCH_BITS)

/**
 * the UMP control word containing the message header and epoch bits
 */
union smlt_ump_ctrl {
    struct {

        smlt_ump_idx_t epoch;    ///< UMP epoch
        smlt_ump_idx_t last_ack; ///< UMP header
    } c;
    smlt_ump_ctrl_word_t raw;   ///<< raw  field
};

/* the size of the contorl structure must be of size control word */
SMLT_STATIC_ASSERT(sizeof(union smlt_ump_ctrl) == sizeof(smlt_ump_ctrl_word_t));


/*
 * UMP message
 * ---------------------------------------------------------------------------
 */

/**
 * the UMP message format on the channel
 */
struct smlt_ump_message {
    union smlt_ump_ctrl ctrl;                               ///< control header
    smlt_ump_payload_word_t data[SMLT_UMP_PAYLOAD_WORDS];   ///< message payload
};

/* the size of the message has to be the right size */
SMLT_STATIC_ASSERT(sizeof(struct smlt_ump_message) == SMLT_UMP_MSG_BYTES);


/*
 * Channel
 * ---------------------------------------------------------------------------
 */




/**
 * direction of the UMP queue
 */
typedef enum {
    SMLT_UMP_DIRECTION_RECV,
    SMLT_UMP_DIRECTION_SEND
} smlt_ump_direction_t;

/**
 * represents an uni-directional UMP channel.
 */
struct smlt_ump_queue
{
    volatile struct smlt_ump_message *buf;   ///< the messages ring buffer`
    smlt_ump_idx_t *last_ack;       ///< memory to store the last ACK
    smlt_ump_idx_t pos;             ///< current position on the buffer`
    smlt_ump_idx_t num_msg;         ///< buffer size in message
    bool epoch;                     ///< next message epoch
    smlt_ump_direction_t direction; ///< direction of the channel

};


/*
 * ===========================================================================
 * UMP channel initialization
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
 errval_t smlt_ump_queue_init_tx(struct smlt_ump_queue *q, void *buf,
                                 smlt_ump_idx_t slots);

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
                                smlt_ump_idx_t slots);


static inline smlt_ump_idx_t smlt_ump_queue_last_ack(struct smlt_ump_queue *q)
{
    return *q->last_ack;
}

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
struct smlt_ump_message *smlt_ump_queue_get_next(struct smlt_ump_queue *c)
{
    SMLT_ASSERT(c->direction == SMLT_UMP_DIRECTION_SEND);
    return c->buf + c->pos;
}


/**
 * @brief sends a message on the UMP channel
 *
 * @param c     the UMP channel to send on
 * @param msg   pointer to the message slot in the channel`
 * @param ctrl  partial control word for the message
 */
static inline void smlt_ump_queue_send(struct smlt_ump_queue *c,
                                       struct smlt_ump_message *msg,
                                       union smlt_ump_ctrl ctrl)
{
    // must submit in order
    SMLT_ASSERT(msg == &c->buf[c->pos]);

    // write barrier for message payload
    smlt_arch_write_barrier();

    // write control word (thus sending the message)
    ctrl.c.epoch = c->epoch;
    msg->ctrl.raw = ctrl.raw;

    // update pos
    if (++c->pos == c->num_msg) {
        c->pos = 0;
        c->epoch = !c->epoch;
    }

}


/**
 * @brief sends a notification on the UMP channel
 *
 * @param c     the UMP channel to send on
 * @param msg   pointer to the message slot in the channel`
 * @param ctrl  partial control word for the message
 */
static inline void smlt_ump_queue_notify(struct smlt_ump_queue *c,
                                         union smlt_ump_ctrl ctrl)
{
    // write the contorl block triggers the sending operation
    ctrl.c.epoch = c->epoch;
    c->buf[c->pos].ctrl.raw = ctrl.raw;

    // update index state
    if (++c->pos == c->num_msg) {
        c->pos = 0;
        c->epoch = !c->epoch;
    }
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
static inline volatile bool smlt_ump_queue_can_recv(struct smlt_ump_queue *c)
{
    SMLT_ASSERT(c);
    SMLT_ASSERT(c->buf);
    SMLT_ASSERT(c->direction == SMLT_UMP_DIRECTION_RECV);


    /* check the direction */
    /*
    if (c->direction != SMLT_UMP_DIRECTION_RECV) {
        return false;
    }
    */

    volatile struct smlt_ump_message *m = c->buf + c->pos;

    return (m->ctrl.c.epoch == c->epoch);
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
static inline errval_t smlt_ump_queue_recv_raw(struct smlt_ump_queue *q,
                                               struct smlt_ump_message **msg)
{
    union smlt_ump_ctrl ctrl;
    volatile struct smlt_ump_message *m;

    SMLT_ASSERT(q);
    SMLT_ASSERT(q->direction != SMLT_UMP_DIRECTION_RECV);

    m = q->buf + q->pos;

    ctrl.raw = m->ctrl.raw;
    if (ctrl.c.epoch != q->epoch) {
        return SMLT_ERR_QUEUE_EMPTY;
    }

    if (++q->pos == q->num_msg) {
        q->pos = 0;
        q->epoch = !q->epoch;
        *(q->last_ack) = ctrl.c.last_ack;
    }

    if (msg) {
        *msg = (struct smlt_ump_message *)m;
    }

    return SMLT_SUCCESS;
}

#endif // SMLT_UMP_QUEUE_H_
