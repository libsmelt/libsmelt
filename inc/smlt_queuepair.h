/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_QUEUEPAIR_H_
#define SMLT_QUEUEPAIR_H_ 1

#include <smlt_endpoint.h>

/*
 * ===========================================================================
 * Smelt queue pair MACROS
 * ===========================================================================
 */
 #define SMLT_QP_CHECK(qp) \
    assert(qp)

/*
 * ===========================================================================
 * Smelt queuepair type definitions
 * ===========================================================================
 */

/* forward declaration */

/**
 * represents a Smelt queuepair
 */
struct smlt_qp
{
    struct smlt_ep tx_ep;
    struct smlt_ep rx_ep;

    errval_t error;                 ///< error value that occured
    /* type specific queue pair */
};


/*
 * ===========================================================================
 * Smelt queuepair creation and destruction
 * ===========================================================================
 */


 /**
  * @brief creates the queue pair 
  *
  * @param TODO: information specification
  *
  * @returns 0
  */
errval_t smlt_queuepair_create(smlt_ep_type_t type,
                               struct smlt_ep **ret_ep);

 /**
  * @brief destroys the queuepair 
  *
  * @param
  *
  * @returns 0
  */
errval_t smlt_queuepair_destroy(struct smlt_qp *qp);


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
static inline errval_t smlt_queuepair_send(struct smlt_qp *qp,
                                           struct smlt_msg *msg)
{
    SMLT_QP_CHECK(qp);
    
    return smlt_endpoint_send(&qp->tx_ep, msg);
}

/**
 * @brief sends a notification (zero payload message) 
 * 
 * @param ep    the Smelt queuepair to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 */
static inline errval_t smlt_queuepair_notify(struct smlt_qp *qp)
{
    SMLT_QP_CHECK(qp);

    /* XXX: maybe provide another function */
    return smlt_endpoint_notify(&qp->tx_ep);
}

/**
 * @brief checks if the a message can be sent on the queuepair
 * 
 * @param ep    the Smelt queuepair to call the check function on
 * 
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 */
static inline bool smlt_queuepair_can_send(struct smlt_qp *qp)
{
    SMLT_QP_CHECK(qp);
    
    return smlt_endpoint_can_send(&qp->tx_ep);
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
static inline errval_t smlt_queuepair_recv(struct smlt_qp *qp, 
                                           struct smlt_msg *msg)
{
    SMLT_QP_CHECK(qp);
    
    return smlt_endpoint_recv(&qp->rx_ep, msg);
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
static inline bool smlt_queuepair_can_recv(struct smlt_qp *qp)
{
    SMLT_QP_CHECK(qp);

    return smlt_endpoint_can_recv(&qp->rx_ep);
}


/*
 * ===========================================================================
 * state queries
 * ===========================================================================
 */


#endif /* SMLT_ENDPOINT_H_ */
