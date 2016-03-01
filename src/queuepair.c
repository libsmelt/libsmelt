/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#include <smlt.h>
#include <smlt_queuepair.h>
#include <smlt_platform.h>

/* backends */
#include <shm/smlt_shm.h>
#include <backends/ffq/ff_queuepair.h>
#include <backends/ump/ump_queuepair.h>
#include "qp_func_wrapper.h"
/**
  * @brief creates the queue pair
  *
  * @param TODO: information specification
  *
  * @returns 0
  */
errval_t smlt_queuepair_create(smlt_qp_type_t type,
                               struct smlt_qp **qp,
                               coreid_t src,
                               coreid_t dst)
{
    // TODO what is really a queuepair ?
    *qp = (struct smlt_qp*) smlt_platform_alloc(sizeof(struct smlt_qp), 
                                                SMLT_DEFAULT_ALIGNMENT,
                                                true);
    int ret;
    switch(type) {
        case SMLT_QP_TYPE_UMP :
            // create two queues
            ret = ump_queuepair_create(&((*qp)->queue_tx.ump), src, dst);
            if (ret) {
                return SMLT_ERR_ALLOC_UMP;
            }
            ret = ump_queuepair_create(&((*qp)->queue_rx.ump), dst, src);
            if (ret) {
                return SMLT_ERR_ALLOC_UMP;
            }

            // set function pointers
            (*qp)->f.send.do_send = smlt_ump_send;
            (*qp)->f.send.notify = smlt_ump_send0;
            (*qp)->f.send.can_send = smlt_ump_can_send;
            (*qp)->f.recv.do_recv = smlt_ump_recv;
            (*qp)->f.recv.can_recv = smlt_ump_can_recv;
            (*qp)->f.recv.notify = smlt_ump_recv0;
            break;
        case SMLT_QP_TYPE_FFQ :
            // create two queues
            ret = ff_queuepair_create(&((*qp)->queue_tx.ffq), dst);
            if (ret) {
                return SMLT_ERR_ALLOC_FFQ;
            }
            ret = ff_queuepair_create(&((*qp)->queue_rx.ffq), src);
            if (ret) {
                return SMLT_ERR_ALLOC_FFQ;
            }
        
            // set function pointers
            (*qp)->f.send.do_send = smlt_ffq_send;
            (*qp)->f.send.notify = smlt_ffq_send0;
            (*qp)->f.send.can_send = smlt_ffq_can_send;
            (*qp)->f.recv.do_recv = smlt_ffq_recv;
            (*qp)->f.recv.can_recv = smlt_ffq_can_recv;
            (*qp)->f.recv.notify = smlt_ffq_recv0;
            break;
        case SMLT_QP_TYPE_SHM :
            assert(!"NIY");
            break;
        default:
            break;
    }

    return SMLT_SUCCESS;
}

/**
 * @brief destroys the queuepair
 *
 * @param
 *
 * @returns 0
 */
errval_t smlt_queuepair_destroy(struct smlt_qp *qp)
{
    int ret;
    switch(qp->type) {
        case SMLT_QP_TYPE_UMP :
            // TODO destroy always returns 0
            ret = ump_queuepair_destroy(&(qp->queue_tx.ump));
            ret = ump_queuepair_destroy(&(qp->queue_rx.ump));
            if (ret) {
                return SMLT_ERR_DESTROY_UMP;
            }
            break;
        case SMLT_QP_TYPE_FFQ :
            // create two queues
            ret = ff_queuepair_destroy(&(qp->queue_tx.ffq));
            ret = ff_queuepair_destroy(&(qp->queue_rx.ffq));
            if (ret) {
                return SMLT_ERR_DESTROY_FFQ;
            }
            break;
        case SMLT_QP_TYPE_SHM :
            assert(!"NIY");
            break;
        default:
            break;
    }

    smlt_platform_free(qp);       
    return SMLT_SUCCESS;
}

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
errval_t smlt_queuepair_send(struct smlt_qp *qp,
                             struct smlt_msg *msg)
{
    return qp->f.send.do_send(qp, msg);
}
/**
 * @brief sends a notification (zero payload message) 
 * 
 * @param ep    the Smelt queuepair to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 */
errval_t smlt_queuepair_notify(struct smlt_qp *qp)
{
    return qp->f.send.notify(qp);
}
/**
 * @brief checks if the a message can be sent on the queuepair
 * 
 * @param ep    the Smelt queuepair to call the check function on
 * 
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 */
bool smlt_queuepair_can_send(struct smlt_qp *qp)
{
    return qp->f.send.can_send(qp);
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
 * @param ep    the Smelt queuepair to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 *
 * this function is BLOCKING if there is no message on the queuepair
 */
errval_t smlt_queuepair_recv(struct smlt_qp *qp, 
                             struct smlt_msg *msg)
{
    return qp->f.recv.do_recv(qp, msg);
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
bool smlt_queuepair_can_recv(struct smlt_qp *qp)
{
    return qp->f.recv.can_recv(qp);
}

