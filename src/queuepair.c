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
#include <backends/ffq/ff_queuepair.h>
#include <backends/ump/smlt_ump_queuepair.h>
#include <backends/shm/shm_qp.h>
#include "qp_func_wrapper.h"
#include "debug.h"

/**
  * @brief creates the queue pair
  *
  * @param TODO: information specification
  *
  * @returns 0
  */
errval_t smlt_queuepair_create(smlt_qp_type_t type,
                               struct smlt_qp **qp1,
                               struct smlt_qp **qp2,
                               coreid_t src,
                               coreid_t dst)
{
    errval_t err;

    SMLT_DEBUG(SMLT_DBG__GENERAL, "creating qp src=%" PRIu32 " dst =% " PRIu32 " \n",
               src, dst);
    // TODO what is really a queuepair ?
    assert(*qp1);
    assert(*qp2);

    (*qp1)->type = type;
    (*qp2)->type = type;
    switch(type) {
        case SMLT_QP_TYPE_UMP :
            ;

            struct smlt_ump_queuepair *umpq_src = &(*qp1)->q.ump;
            struct smlt_ump_queuepair *umpq_dst = &(*qp2)->q.ump;

            err = smlt_ump_queuepair_init(SMLT_UMP_DEFAULT_SLOTS,
                                          smlt_platform_cluster_of_core(src),
                                          smlt_platform_cluster_of_core(dst),
                                          umpq_src, umpq_dst);
            if (smlt_err_is_fail(err)) {
                return err;
            }

            // set function pointers
            (*qp1)->f.send.do_send = smlt_ump_queuepair_send;
            (*qp1)->f.send.notify = smlt_ump_queuepair_notify;
            (*qp1)->f.send.can_send = smlt_ump_queuepair_can_send;
            (*qp1)->f.recv.do_recv = smlt_ump_queuepair_recv;
            (*qp1)->f.recv.can_recv = smlt_ump_queuepair_can_recv;
            (*qp1)->f.recv.notify = smlt_ump_queuepair_recv_notify;
            (*qp2)->f = (*qp1)->f;

            break;
        case SMLT_QP_TYPE_FFQ :
            // create two queues
            (*qp1)->queue_tx.ffq = *ff_queuepair_create(dst);
            (*qp1)->queue_rx.ffq = *ff_queuepair_create(src);

            (*qp2)->queue_tx.ffq = (*qp1)->queue_rx.ffq;
            (*qp2)->queue_rx.ffq = (*qp1)->queue_tx.ffq;

            // set function pointers
            (*qp1)->f.send.do_send = smlt_ffq_send;
            (*qp1)->f.send.notify = smlt_ffq_send0;
            (*qp1)->f.send.can_send = smlt_ffq_can_send;
            (*qp1)->f.recv.do_recv = smlt_ffq_recv;
            (*qp1)->f.recv.can_recv = smlt_ffq_can_recv;
            (*qp1)->f.recv.notify = smlt_ffq_recv0;

            (*qp2)->f = (*qp1)->f;
            break;
        case SMLT_QP_TYPE_SHM :
            // create two queues
            (*qp1)->queue_tx.shm = *shm_queuepair_create(src, dst);
            (*qp1)->queue_rx.shm = *shm_queuepair_create(dst, src);

            (*qp2)->queue_rx.shm = (*qp1)->queue_tx.shm;
            (*qp2)->queue_tx.shm = (*qp1)->queue_rx.shm;

            // set function pointers
            (*qp1)->f.send.do_send = smlt_shm_send;
            (*qp1)->f.send.notify = smlt_shm_send0;
            (*qp1)->f.send.can_send = smlt_shm_can_send;
            (*qp1)->f.recv.do_recv = smlt_shm_recv;
            (*qp1)->f.recv.can_recv = smlt_shm_can_recv;
            (*qp1)->f.recv.notify = smlt_shm_recv0;

            (*qp2)->f = (*qp1)->f;
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
    //int ret;
    switch(qp->type) {
        case SMLT_QP_TYPE_UMP :
            // TODO destroy always returns 0
            //ret = ump_queuepair_destroy(&(qp->queue_tx.ump));
            //ret = ump_queuepair_destroy(&(qp->queue_rx.ump));
            //if (ret) {
                return SMLT_ERR_DESTROY_UMP;
            //}
            break;
        case SMLT_QP_TYPE_FFQ :
            // TODO do destroy
            break;
        case SMLT_QP_TYPE_SHM :
            // TODO do destroy
            break;
        default:
            break;
    }

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
 * @brief receives a message from the queuepair
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
 * @brief receives a notification from the queuepair
 *
 * @param ep    the Smelt queuepair to call the operation on
 *
 * @returns error value
 *
 * this function is BLOCKING if there is no message on the queuepair
 */
errval_t smlt_queuepair_recv0(struct smlt_qp *qp)
{
    return qp->f.recv.notify(qp);
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
