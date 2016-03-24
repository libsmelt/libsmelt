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

#include <ump/smlt_ump_queuepair.h>
#include <ffq/smlt_ffq_queuepair.h>
#include <shm/shm_qp.h>
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
struct smlt_qp;
struct smlt_msg;
struct smlt_ump_queuepair;
struct smlt_ffq_queuepair;
struct shm_qp;
struct swmr_queue;
/**
 * encodes the backend type of the Smelt queuepair
 */
typedef enum {
    SMLT_QP_TYPE_INVALID=0,     ///< qp type is invalid
    SMLT_QP_TYPE_UMP,           ///< qp uses the UMP backend
    SMLT_QP_TYPE_FFQ,           ///< qp uses the FFQ backend
    SMLT_QP_TYPE_SHM,            ///< qp uses shared memory backend
} smlt_qp_type_t;


/**
 * @brief type definition for the OP function of the queuepair.
 *
 * @param qp    the Smelt queuepair to call the operation on
 * @param msg   Smelt message argument
 *
 * @returns error value
 *
 * this invokes either the send or recv function
 */
typedef errval_t (*smlt_qp_op_fn_t)(struct smlt_qp *qp, struct smlt_msg *msg);

typedef errval_t (*smlt_qp_notify_fn_t)(struct smlt_qp *qp);

/**
 * @brief type definition for the CHECK function of the queuepair.
 *
 * @param ep    the Smelt endpoint to call the check function on
 *
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 *
 * this invokes either the can_send or can_receive function
 */
typedef bool (*smlt_qp_check_fn_t)(struct smlt_qp *qp);

/**
 * represents a Smelt queuepair
 */
struct smlt_qp
{
    errval_t error;                 ///< error value that occured
    smlt_qp_type_t type;

    union {
        struct smlt_ump_queuepair ump;
        struct smlt_ffq_queuepair ffq;
        struct shm_qp shm;
    } q;

    union {
        struct shm_qp shm;
    } queue_tx;

    union {
        struct shm_qp shm;
    } queue_rx;

    struct {
        struct {
            smlt_qp_op_fn_t try_send;           ///< send operation
            smlt_qp_notify_fn_t notify;
            smlt_qp_check_fn_t can_send;    ///< checks if can be send
        } send;
        struct {
            smlt_qp_op_fn_t try_recv;           ///< recv operation
            smlt_qp_notify_fn_t notify; // TODO change name
            smlt_qp_check_fn_t can_recv;    ///< checksi if can be received
        } recv;
    } f;
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
errval_t smlt_queuepair_create(smlt_qp_type_t type,
                               struct smlt_qp **qp_src,
                               struct smlt_qp **qp_dst,
                               coreid_t src,
                               coreid_t dst);

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
 */
static inline errval_t smlt_queuepair_try_send(struct smlt_qp *qp,
                                               struct smlt_msg *msg)
{
    return qp->f.send.try_send(qp, msg);
}

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
    errval_t err;

    do {
        err = smlt_queuepair_try_send(qp, msg);
    } while(err == SMLT_ERR_QUEUE_FULL);

    return err;
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
static inline bool smlt_queuepair_can_send(struct smlt_qp *qp)
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
  * @param qp    the Smelt queuepair to call the operation on
  * @param msg   Smelt message argument
  *
  * @returns error value
  *
  * this function is BLOCKING if there is no message on the queuepair
  */
 static inline errval_t smlt_queuepair_try_recv(struct smlt_qp *qp,
                                                struct smlt_msg *msg)
 {
     return qp->f.recv.try_recv(qp, msg);
 }

/**
 * @brief receives a message from the queuepair
 *
 * @param qp    the Smelt queuepair to call the operation on
 * @param msg   Smelt message argument
 *
 * @returns error value
 *
 * this function is BLOCKING if there is no message on the queuepair
 */
static inline errval_t smlt_queuepair_recv(struct smlt_qp *qp,
                                           struct smlt_msg *msg)
{
    errval_t err;
    do {
        err = smlt_queuepair_try_recv(qp, msg);
    } while(err == SMLT_ERR_QUEUE_EMPTY);

    return err;
}

/**
 * @brief receives a notification from the queuepair
 *
 * @param qp    the Smelt queuepair to call the operation on
 *
 * @returns error value
 *
 * this function is BLOCKING if there is no message on the queuepair
 */
static inline errval_t smlt_queuepair_recv0(struct smlt_qp *qp)
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
static inline bool smlt_queuepair_can_recv(struct smlt_qp *qp)
{
    return qp->f.recv.can_recv(qp);
}


/*
 * ===========================================================================
 * state queries
 * ===========================================================================
 */


#endif /* SMLT_ENDPOINT_H_ */
