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

#include <string.h>

#include "smlt_queuepair.h"
#include "smlt_debug.h"
#include "backends/shm/swmr.h"

/*
 * ===========================================================================
 * Smelt channel MACROS
 * ===========================================================================
 */
 #define SMLT_CHAN_CHECK(_chan) \
    assert(_chan)

extern __thread smlt_nid_t smlt_node_self_id; ///< caches the node id

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
    smlt_nid_t owner;
    uint32_t m;
    uint32_t n;
    bool use_shm;
    /* type specific queue pair */
    union {
        struct mp {
            struct smlt_qp* send;  // pointer to send qps
            struct smlt_qp* recv; // pointer to recv qps
        } mp;
        struct shm {
            struct swmr_queue send_owner; // send swmr
            struct smlt_qp **recv_owner; // dst to src
            struct smlt_qp **recv; // dst to src
            uint32_t* dst;
        } shm;
   } c;
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
    if (!chan->use_shm) {
        for (unsigned int i = 0; i < num_chan; i++) {
            if (chan->owner == smlt_node_self_id) {
                err = smlt_queuepair_send(&chan->c.mp.send[i], msg);
            } else {
                err = smlt_queuepair_send(&chan->c.mp.recv[i], msg);
            }
            if (smlt_err_is_fail(err)){
                return smlt_err_push(err, SMLT_ERR_SEND);
            }
        }

    } else {
        if (chan->owner == smlt_node_self_id) {
           smlt_swmr_send(&chan->c.shm.send_owner, msg);
        } else {
            for (unsigned int i = 0; i < chan->m; i++) {
                if (chan->c.shm.dst[i] == smlt_node_self_id) {
                    smlt_queuepair_send(chan->c.shm.recv[i], msg);
                }
            }
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
    if (!chan->use_shm) {
        for (unsigned i = 0; i < num_chan; i++) {
            if (chan->owner == smlt_node_self_id) {
                err = smlt_queuepair_notify(&chan->c.mp.send[i]);
            } else {
                err = smlt_queuepair_notify(&chan->c.mp.recv[i]);
            }
            if (smlt_err_is_fail(err)){
                return smlt_err_push(err, SMLT_ERR_SEND);
            }

        }
    } else {
        if (chan->owner == smlt_node_self_id) {
           smlt_swmr_send0(&chan->c.shm.send_owner);
        } else {
            for (unsigned i = 0; i < chan->m; i++) {
                if (chan->c.shm.dst[i] == smlt_node_self_id) {
                    smlt_queuepair_notify(chan->c.shm.recv[i]);
                }
            }
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
    for (unsigned int i = 0; i < num_chan; i++) {
        if (chan->owner == smlt_node_self_id) {
            if (!smlt_queuepair_can_send(&chan->c.mp.send[i])) {
                result = false;
            }
        } else {
            if (!smlt_queuepair_can_send(&chan->c.mp.recv[i])) {
                result = false;
            }
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
    errval_t err = SMLT_SUCCESS;
    if (chan->n == chan->m) {
        if (chan->owner == smlt_node_self_id){
            err = smlt_queuepair_recv(&chan->c.mp.send[0], msg);
        } else {
            err = smlt_queuepair_recv(&chan->c.mp.recv[0], msg);
        }
        // TODO error checking
    } else {
        if (chan->owner == smlt_node_self_id){
            // recv from all channels
            /*
            for (int i = 0; i < chan->m; i++) {
                err = smlt_queuepair_recv(chan->c.shm.recv_owner[i], msg);
            }
            */
            bool recv[chan->m];
            memset(recv, 0, sizeof(bool)*chan->m);
            unsigned int num_recv = 0;
            unsigned int i = 0;
            while( num_recv < chan->m) {
                if (!recv[i] && smlt_queuepair_can_recv(chan->c.shm.recv_owner[i])) {
                    err = smlt_queuepair_recv0(chan->c.shm.recv_owner[i]);
                    if (smlt_err_is_fail(err)) {
                        return err;
                    }
                    recv[i] = true;
                    num_recv++;
                }

                i++;

                if (i == chan->m) {
                    i = 0;
                }
            }
        } else {
            for (unsigned int i = 0; i < chan->m; i++) {
                if (chan->c.shm.dst[i] == smlt_node_self_id) {
                    smlt_swmr_recv(&chan->c.shm.send_owner.dst[i], msg);
                }
            }
            //smlt_swmr_recv(&chan->c.shm.send_owner.dst[smlt_node_self_id-1], msg);
        }
    }
    return err;
}

/**
 * @brief receives a message or a notification from the queuepair
 *
 * @param chan      the Smelt channel to call he operation on
 * @param msg       Smelt message argument
 * @param core_id   the core id of child on which the msg is received
 * @param index     for 1:n channels we need to know on which receieing end
 *                  we receive otherwise ignored
 *
 * @returns error value
 *
 * this function is BLOCKING if there is no message on the queuepair
 */
static inline errval_t smlt_channel_recv_index(struct smlt_channel *chan,
                                               struct smlt_msg *msg,
                                               uint32_t index)
{
    // 1:1
    errval_t err = SMLT_SUCCESS;
    if (chan->n == chan->m) {
        if (chan->owner == smlt_node_self_id){
            err = smlt_queuepair_recv(&chan->c.mp.send[0], msg);
        } else {
            err = smlt_queuepair_recv(&chan->c.mp.recv[0], msg);
        }
        // TODO error checking
    } else {
        if (chan->owner == smlt_node_self_id){
            // recv from all channels
            for (unsigned i = 0; i < chan->m; i++) {
                err = smlt_queuepair_recv(chan->c.shm.recv_owner[i], msg);
            }
        } else {
            smlt_swmr_recv(&chan->c.shm.send_owner.dst[index], msg);
        }
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
    for (unsigned int i = 0; i < num_chan; i++) {
        if (!chan->use_shm) {
            if (chan->owner == smlt_node_self_id) {
                if (!smlt_queuepair_can_recv(&chan->c.mp.send[i])) {
                    result = false;
                }
            } else {
                if (!smlt_queuepair_can_recv(&chan->c.mp.recv[i])) {
                    result = false;
                }
            }
        } else {
            // TODO how to handle can recv on multiple channels
            return true;
        }
    }
    return result;
}

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
static inline errval_t smlt_channel_recv_notification(struct smlt_channel *chan)
{
    // 1:1
    errval_t err = SMLT_SUCCESS;
    if (chan->n == chan->m) {
        if (chan->owner == smlt_node_self_id){
            err = smlt_queuepair_recv0(&chan->c.mp.send[0]);
        } else {
            err = smlt_queuepair_recv0(&chan->c.mp.recv[0]);
        }
        // TODO error checking
    } else {
        if (chan->owner == smlt_node_self_id){
            // recv from all channels
            /*for (int i = 0; i < chan->m; i++) {
            /    err = smlt_queuepair_recv0(chan->c.shm.recv_owner[i]);
            } */
            bool recv[chan->m];
            memset(recv, 0, sizeof(bool)*chan->m);
            unsigned int num_recv = 0;
            unsigned int i = 0;
            while( num_recv < chan->m) {
                if (!recv[i] && smlt_queuepair_can_recv(chan->c.shm.recv_owner[i])) {
                    err = smlt_queuepair_recv0(chan->c.shm.recv_owner[i]);
                    if (smlt_err_is_fail(err)) {
                        return err;
                    }
                    recv[i] = true;
                    num_recv++;
                }

                i++;

                if (i == chan->m) {
                    i = 0;
                }
            }

        } else {
            for (unsigned i = 0; i < chan->m; i++) {
                if (chan->c.shm.dst[i] == smlt_node_self_id) {
                    smlt_swmr_recv0(&chan->c.shm.send_owner.dst[i]);
                }
            }
        }
    }
    return err;
}

/*
 * ===========================================================================
 * state queries
 * ===========================================================================
 */

#endif /* SMLT_ENDPOINT_H_ */
