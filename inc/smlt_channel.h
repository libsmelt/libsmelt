/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_CHANNEL_H_
#define SMLT_CHANNEL_H_ 1

#include <smlt_queuepair.h>
/*
 * ===========================================================================
 * Smelt channel MACROS
 * ===========================================================================
 */
 #define SMLT_CHAN_CHECK(_chan) \
    assert(_chan)


/*
 * ===========================================================================
 * Smelt queuepair type definitions
 * ===========================================================================
 */

typedef enum {
    SMLT_CHAN_BACKEND_MP, // Message passing backend
    SMLT_CHAN_BACKEND_SHM, // Single writer multiple reader backend
} smlt_chan_backend_t;

typedef enum {
    SMLT_CHAN_TYPE_INVALID,
    SMLT_CHAN_TYPE_ONE_TO_ONE,
    SMLT_CHAN_TYPE_ONE_TO_MANY,
    SMLT_CHAN_TYPE_MANY_TO_ONE,
    SMLT_CHAN_TYPE_MANY_TO_MANY,
} smlt_chan_type_t;


struct smlt_chan_o2o
{
    struct smlt_qp send;
    struct smlt_qp recv;
};

struct smlt_chan_o2m
{
    uint32_t m;
    smlt_chan_backend_t backend;
    uint32_t last_polled;
    union {
        struct smlt_qp* qp; // can not make array (size would be undefined)
        uint32_t dummy; // TODO add shm SW/MR queue
    } send;
    union {
        struct smlt_qp* qp; // can not make array (size would be undefined)
        uint32_t dummy; // TODO add shm SW/MR queue
    } recv;
};

struct smlt_chan_m2o
{
    uint32_t dummy;
};

struct smlt_chan_m2m
{
    uint32_t dummy;
};

/**
 * represents a Smelt channel
 */
struct smlt_channel
{
    smlt_chan_type_t type;
    union {
        struct smlt_chan_o2o o2o;
        struct smlt_chan_o2m o2m;
        struct smlt_chan_m2o m2o;
        struct smlt_chan_m2m m2m;
    } c;
    /* type specific queue pair */
};


/*
 * ===========================================================================
 * Smelt channel creation and destruction
 * ===========================================================================
 */


 /**
  * @brief creates the queue pair
  *
  * @param type     type of the channel (o2o o2m m2o m2m)
  * @param chan     return pointer to the channel
  * @param src      src core id // TODO use nid ? 
  * @param dst      array of core ids to desinations
  * @param count    length of array dst;   
  *
  * @returns SMLT_SUCCESS or failure 
  */
errval_t smlt_channel_create(smlt_chan_type_t  type,
                             struct smlt_channel *chan,
                             uint32_t src,
                             uint32_t* dst,
                             uint16_t count);

 /**
  * @brief destroys the channel
  *
  * @param
  *
  * @returns 0
  */
errval_t smlt_channel_destroy(struct smlt_channel *chan);


/*
 * ===========================================================================
 * sending functions
 * ===========================================================================
 */


/**
 * @brief sends a message on the to the queuepair
 *
 * @param ep    the Smelt queuepair to call the operation on
 * @param msg   Smelt message argument
 *
 * @returns error value
 *
 * This function is BLOCKING if the queuepair cannot take new messages
 */
static inline errval_t smlt_channel_send(struct smlt_channel *chan,
                                         struct smlt_msg *msg)
{
    switch (chan->type) {
        case SMLT_CHAN_TYPE_ONE_TO_ONE:
            smlt_queuepair_send(&chan->c.o2o.send, msg);
            break;
        case SMLT_CHAN_TYPE_ONE_TO_MANY:
            if (chan->c.o2m.backend == SMLT_CHAN_BACKEND_MP) {
                for (int i = 0; i < chan->c.o2m.m; i++) {
                    smlt_queuepair_send(&chan->c.o2m.send.qp[i], msg);
                }
            } else {
                // TODO
                assert(!"NIY");
            }
            break;
        case SMLT_CHAN_TYPE_MANY_TO_ONE:   
            assert(!"NIY");
            break;

        case SMLT_CHAN_TYPE_MANY_TO_MANY:
            assert(!"NIY");
            break;

        default:
            break;
    }
    return SMLT_SUCCESS;
}

/**
 * @brief sends a notification (zero payload message)
 *
 * @param ep    the Smelt queuepair to call the operation on
 * @param msg   Smelt message argument
 *
 * @returns error value
 */
static inline errval_t smlt_channel_notify(struct smlt_channel *chan)
{
    switch (chan->type) {
        case SMLT_CHAN_TYPE_ONE_TO_ONE:
            smlt_queuepair_notify(&chan->c.o2o.send);
            break;
        case SMLT_CHAN_TYPE_ONE_TO_MANY:
            if (chan->c.o2m.backend == SMLT_CHAN_BACKEND_MP) {
                for (int i = 0; i < chan->c.o2m.m; i++) {
                    smlt_queuepair_notify(&chan->c.o2m.send.qp[i]);
                }
            } else {
                // TODO
                assert(!"NIY");
            }
            break;
        case SMLT_CHAN_TYPE_MANY_TO_ONE:   
            assert(!"NIY");
            break;

        case SMLT_CHAN_TYPE_MANY_TO_MANY:
            assert(!"NIY");
            break;

        default:
            break;
    }
    return SMLT_SUCCESS;
}

/**
 * @brief checks if the a message can be sent on the channel
 *
 * @param chan    the Smelt channel to call the check function on
 *
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 */
static inline bool smlt_channel_can_send(struct smlt_channel *chan)
{
    switch (chan->type) {
        case SMLT_CHAN_TYPE_ONE_TO_ONE:
            return smlt_queuepair_can_send(&chan->c.o2o.send);
            break;
        case SMLT_CHAN_TYPE_ONE_TO_MANY:
            if (chan->c.o2m.backend == SMLT_CHAN_BACKEND_MP) {
                for (int i = 0; i < chan->c.o2m.m; i++) {
                    if (!smlt_queuepair_can_send(&chan->c.o2m.send.qp[i])) {
                        return false;
                    }
                }
                return true;
            } else {
                // TODO
                assert(!"NIY");
            }
            break;
        case SMLT_CHAN_TYPE_MANY_TO_ONE:   
            assert(!"NIY");
            break;

        case SMLT_CHAN_TYPE_MANY_TO_MANY:
            assert(!"NIY");
            break;

        default:
            break;
    }
}

/* TODO: include also non blocking variants ? */

/*
 * ===========================================================================
 * receiving functions
 * ===========================================================================
 */


/**
 * @brief receives a message or a notification from the queuepair
 *
 * @param chan      the Smelt channel to call he operation on
 * @param msg       Smelt message argument
 * @param core_id   the core id of child on which the msg is received
 *
 * @returns error value
 *
 * this function is BLOCKING if there is no message on the queuepair
 */
static inline errval_t smlt_channel_recv(struct smlt_channel *chan,
                                         struct smlt_msg *msg)
{
    switch (chan->type) {
        case SMLT_CHAN_TYPE_ONE_TO_ONE:
            smlt_queuepair_recv(&chan->c.o2o.recv, msg);
            break;
        case SMLT_CHAN_TYPE_ONE_TO_MANY:
            assert(!"NIY");
            break;
        case SMLT_CHAN_TYPE_MANY_TO_ONE:   
            assert(!"NIY");
            break;

        case SMLT_CHAN_TYPE_MANY_TO_MANY:
            assert(!"NIY");
            break;

        default:
            break;
    }
    return SMLT_SUCCESS;
    return SMLT_SUCCESS;
}

/**
 * @brief checks if there is a message to be received
 *
 * @param ep    the Smelt queuepair to call the check function on
 *
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 *
 * this invokes either the can_send or can_receive function
 */
static inline bool smlt_channel_can_recv(struct smlt_channel *chan)
{
    return SMLT_SUCCESS;
}


/*
 * ===========================================================================
 * state queries
 * ===========================================================================
 */

#endif /* SMLT_ENDPOINT_H_ */
