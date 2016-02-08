/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_NODE_H_
#define SMLT_NODE_H_ 1

/*
 * ===========================================================================
 * type declarations
 * ===========================================================================
 */

///< the smelt thread id        XXX or is this the endpoint id ?
typedef uint32_t smlt_nid_t;

/*
 * ===========================================================================
 * thread management functions
 * ===========================================================================
 */ 
errval_t smlt_node_create();

errval_t smlt_node_join();

errval_t smlt_node_cancel();

/*
 * ===========================================================================
 * thread state functions
 * ===========================================================================
 */ 

smlt_tid_t smlt_node_get_id(void);

/// xxx we need to have a way to get the <machine:core>
coreid_t smlt_node_get_core_id();

bool smlt_node_is_coordinator();

uint32_t smlt_node_get_num_nodes();



mp_binding *mp_get_parent(coreid_t, int*);
mp_binding **mp_get_children(coreid_t, int*, int**);
void mp_connect(coreid_t src, coreid_t dst);
coreid_t get_sequentializer(void);


/**
 * @brief sends a message on the to the node
 * 
 * @param ep    the Smelt node to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 *
 * This function is BLOCKING if the node cannot take new messages
 */
static inline errval_t smlt_node_send(struct smlt_node *node, 
                                      struct smlt_msg *msg)
{
    SMLT_NODE_CHECK(node);
    
    return smlt_queuepair_send(&node->qp, msg);
}

/**
 * @brief sends a notification (zero payload message) 
 * 
 * @param node    the Smelt node to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 */
static inline errval_t smlt_node_notify(struct smlt_node *node)
{
    SMLT_NODE_CHECK(node);

    /* XXX: maybe provide another function */
    return smlt_queuepair_notify(&node->qp);
}

/**
 * @brief checks if the a message can be sent on the node
 * 
 * @param node    the Smelt node to call the check function on
 * 
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 */
static inline bool smlt_node_can_send(struct smlt_node *node)
{
    SMLT_NODE_CHECK(node);
    
    return smlt_queuepair_can_send(&node->qp);
}

/* TODO: include also non blocking variants ?

/*
 * ===========================================================================
 * receiving functions
 * ===========================================================================
 */


/**
 * @brief receives a message or a notification from the node
 * 
 * @param node    the Smelt node to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 *
 * this function is BLOCKING if there is no message on the node
 */
static inline errval_t smlt_node_recv(struct smlt_node *node, 
                                      struct smlt_msg *msg)
{
    SMLT_NODE_CHECK(node);
    
    return smlt_queuepair_recv(&node->qp, msg);
}

/**
 * @brief checks if there is a message to be received
 * 
 * @param node    the Smelt node to call the check function on
 * 
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 *
 * this invokes either the can_send or can_receive function
 */
static inline bool smlt_node_can_recv(struct smlt_node *node)
{
    SMLT_NODE_CHECK(node);

    return smlt_queuepair_can_recv(&node->qp);
}


#endif /* SMLT_NODE_H_ */