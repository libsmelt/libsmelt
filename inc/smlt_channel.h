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

/**
 * represents a Smelt channel
 */
struct smlt_channel
{
    uint32_t m;
    uint32_t n;

    /* type specific queue pair */
    struct smlt_qp* send;  // pointer to send qps
    struct smlt_qp* recv; // pointer to recv qps
};


/*
 * ===========================================================================
 * Smelt channel creation and destruction
 * ===========================================================================
 */


 /**
  * @brief creates the queue pair
  *
  * @param chan     return pointer to the channel
  * @param src      src core ids // TODO use nid ? 
  * @param dst      array of core ids to desinations
  * @param count_src    length of array src;   
  * @param count_dst    length of array dst;   
  *
  * @returns SMLT_SUCCESS or failure 
  */
errval_t smlt_channel_create(struct smlt_channel **chan,
                             uint32_t* src,
                             uint32_t* dst,
                             uint16_t count_src,
                             uint16_t count_dst);

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
    errval_t err;
    uint32_t num_chan = chan->m > chan->n ? chan->m : chan->n;
    for (int i = 0; i < num_chan; i++) {
        err = smlt_queuepair_send(&chan->send[i], msg);
        if (smlt_err_is_fail(err)){
            return smlt_err_push(err, SMLT_ERR_SEND);
        }   
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
    errval_t err;
    uint32_t num_chan = chan->m > chan->n ? chan->m : chan->n;
    for (int i = 0; i < num_chan; i++) {
        err = smlt_queuepair_notify(&chan->send[i]); 
        if (smlt_err_is_fail(err)){
            return smlt_err_push(err, SMLT_ERR_SEND);
        }   
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
    bool result = true;
    uint32_t num_chan = chan->m > chan->n ? chan->m : chan->n;
    for (int i = 0; i < num_chan; i++) {
        if (!smlt_queuepair_can_send(&chan->send[i])) {
            result = false;
        }   
    }
    return result;
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
    // 1:1
    errval_t err;
    if (chan->n == chan->m) {
       err = smlt_queuepair_recv(&chan->recv[0], msg);
    } else {
       assert(!"1:N recv NIY");
    }
    return err;
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
    bool result = true;
    uint32_t num_chan = chan->m > chan->n ? chan->m : chan->n;
    for (int i = 0; i < num_chan; i++) {
        if (!smlt_queuepair_can_recv(&chan->send[i])) {
            result = false;
        }   
    }
    return result;
}


/*
 * ===========================================================================
 * state queries
 * ===========================================================================
 */

#endif /* SMLT_ENDPOINT_H_ */
