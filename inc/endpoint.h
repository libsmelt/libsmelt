/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_ENDPOINT_H_
#define SMLT_ENDPOINT_H_ 1

/*
 * ===========================================================================
 * Smelt endpoint MACROS
 * ===========================================================================
 */
 #define SMLT_EP_CHECK(ep, dir) \
    assert(ep && ep->state == SMLT_EP_ST_ONLINE && ep->direction == dir)

/*
 * ===========================================================================
 * Smelt endpoint type definitions
 * ===========================================================================
 */

/* forward declaration */
struct smlt_ep;

/**
 * encodes the Smelt endpoint direction. This can either be send or recv
 */
typedef enum {
    SMLT_EP_DIRECTION_INVALID=0, ///< direction is invalid
    SMLT_EP_DIRECTION_RECV,      ///< this is a receiving endpoint
    SMLT_EP_DIRECTION_SEND       ///< this is a sending endpoint
} smlt_ep_direction_t; 

/**
 * encodes the state a Smelt endpoint can be in
 */
typedef enum {
    SMLT_EP_STATE_INVALID=0,    ///< endpoint state is invalid
    SMLT_EP_STATE_OFFILNE,      ///< endpoint is initialized but not connected
    SMLT_EP_STATE_ONLINE,       ///< endpoint is connected and ready to use
    SMLT_EP_STATE_ERROR         ///< endpoint occurred an error
} smlt_ep_state_t;

/**
 * encodes the backend type of the Smelt endpoint
 */
typedef enum {
    SMLT_EP_TYPE_INVALID=0,     ///< endpoint type is invalid
    SMLT_EP_TYPE_UMP,           ///< endpoint uses the UMP backend
    SMLT_EP_TYPE_FFQ,           ///< endpoint uses the FFQ backend
    SMLT_EP_TYPE_SHM            ///< endpoint uses shared memory backend
} smlt_ep_type_t;

/**
 * @brief type definition for the OP function of the endpoint. 
 * 
 * @param ep    the Smelt endpoint to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 *
 * this invokes either the send or recv function
 */
typedef errval_t (*smlt_ep_op_fn_t)(struct smlt_ep *ep, struct smlt_msg *msg);

/**
 * @brief type definition for the CHECK function of the endpoint. 
 * 
 * @param ep    the Smelt endpoint to call the check function on
 * 
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 *
 * this invokes either the can_send or can_receive function
 */
typedef bool (*smlt_ep_check_fn_t)(struct smlt_ep *ep, struct smlt_msg *msg);

/**
 * represents a Smelt endpoint
 */
struct smlt_ep
{
    smlt_ep_direction_t direction;  ///< direction of this endpoint
    smlt_ep_state_t state;          ///< current state of the endpoint
    smlt_ep_type_t type;            ///< backend type of the endpoint
    union {
        struct {                          
            smlt_ep_op_fn_t send;           ///< send operation
            smlt_ep_check_fn_t can_send;    ///< checks if can be send
        };
        struct {
            smlt_ep_op_fn_t recv;           ///< recv operation
            smlt_ep_check_fn_t can_recv;    ///< checksi if can be received
        } ;
    };
    errval_t error;                 ///< error value that occured
    /* type specific endpoint */
};


/*
 * ===========================================================================
 * Smelt endpoint creation and destruction
 * ===========================================================================
 */


 /**
  * @brief creates the endpoint 
  *
  * @param TODO: information specification
  *
  * @returns 0
  */
errval_t smlt_endpoint_create(smlt_ep_type_t type,
                              struct smlt_ep **ret_ep);

 /**
  * @brief destroys the endpoint 
  *
  * @param
  *
  * @returns 0
  */
errval_t smlt_endpoint_destroy(struct smlt_ep *ep);


/*
 * ===========================================================================
 * sending functions
 * ===========================================================================
 */


/**
 * @brief sends a message on the to the endpoint
 * 
 * @param ep    the Smelt endpoint to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 *
 * This function is BLOCKING if the endpoint cannot take new messages
 */
static inline errval_t smlt_endpoint_send(struct smlt_ep *ep, 
                                          struct smlt_msg *msg)
{
    SMLT_EP_CHECK(ep, SMLT_EP_DIRECTION_SEND);
    
    return ep->send(ep, msg);
}

/**
 * @brief sends a notification (zero payload message) 
 * 
 * @param ep    the Smelt endpoint to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 */
static inline errval_t smlt_endpoint_notify(struct smlt_ep *ep)
{
    SMLT_EP_CHECK(ep, SMLT_EP_DIRECTION_SEND);

    /* XXX: maybe provide another function */
    return ep->send(ep, NULL);
}

/**
 * @brief checks if the a message can be sent on the endpoint
 * 
 * @param ep    the Smelt endpoint to call the check function on
 * 
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 */
static inline bool smlt_endpoint_can_send(struct smlt_ep *ep)
{
    SMLT_EP_CHECK(ep, SMLT_EP_DIRECTION_SEND);
    
    return ep->can_send(ep);
}

/* TODO: include also non blocking variants ?

/*
 * ===========================================================================
 * receiving functions
 * ===========================================================================
 */


/**
 * @brief receives a message or a notification from the endpoint
 * 
 * @param ep    the Smelt endpoint to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 *
 * this function is BLOCKING if there is no message on the endpoint
 */
static inline errval_t smlt_endpoint_recv(struct smlt_ep *ep, 
                                          struct smlt_msg *msg)
{
    SMLT_EP_CHECK(ep, SMLT_EP_DIRECTION_RECV);
    
    return ep->recv(ep, msg);
}

/**
 * @brief checks if there is a message to be received
 * 
 * @param ep    the Smelt endpoint to call the check function on
 * 
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 *
 * this invokes either the can_send or can_receive function
 */
static inline bool smlt_endpoint_can_recv(struct smlt_ep *ep)
{
    SMLT_EP_CHECK(ep, SMLT_EP_DIRECTION_RECV);

    return ep->can_recv(ep);
}


/*
 * ===========================================================================
 * state queries
 * ===========================================================================
 */


/**
 * @brief checks whether the endpoint is ready and usable
 * 
 * @param ep    the smelt endpoint
 * 
 * @returns TRUE if the endpoint is online
 *          FALSE otherwise
 */
static inline bool smlt_endpoint_is_online(struct smlt_ep *ep)
{
    return (ep->state == SMLT_EP_STATE_ONLINE);
}

/**
 * @brief obtains the last occurred error value of the endpoint
 * 
 * @param ep    the smelt endpoint
 * 
 * @returns error value
 */
static inline errval_t smlt_endpoint_get_error(struct smlt_ep *ep)
{
    return ep->error;
}

#endif /* SMLT_ENDPOINT_H_ */