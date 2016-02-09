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
struct smlt_node;

/**
 * arguments passed to the the new smelt node
 */
struct smlt_node_args
{
    smlt_nid_t id;          ///< the node ID to set
    coreid_t core;          ///< ID of the core to start the thread on
    void (*start)(void *)   ///< the start function
    void *arg;              ///< argument for the start function
};


/*
 * ===========================================================================
 * node management functions
 * ===========================================================================
 */ 


/**
 * @brief creates a new smelt node 
 * 
 * @param args
 */
errval_t smlt_node_create(struct smlt_node_args *args);

/**
 * @brief waits for the other node to terminate
 *
 * @param node the other Smelt node
 *
 * @returns  TODO: errval
 */
errval_t smlt_node_join(struct smlt_node *node);


/**
 * @brief terminates the othern node and waits for termination
 *
 * @param node the other Smelt node
 *
 * @returns  TODO: errval
 */
errval_t smlt_node_cancel(struct smlt_node *node);


/*
 * ===========================================================================
 * node state functions
 * ===========================================================================
 */ 


/**
 * @brief gets the ID of the current node
 *
 * @returns integer value representing the node id
 */
smlt_nid_t smlt_node_get_id(void);

/**
 * @brief gets the ID of the current node
 *
 * @param node  the smelt node
 *
 * @returns integer value representing the node id
 */
smlt_nid_t smlt_node_get_id_of_node(struct smelt_node *node);

/**
 * @brief gets the core ID of the current node
 *
 * @returns integer value representing the coreid
 */
smlt_nid_t smlt_node_get_coreid(void);

/**
 * @brief gets the ID of the current node
 *
 * @param node  the smelt node
 *
 * @returns integer value representing the node id
 */
smlt_nid_t smlt_node_get_coreid_of_node(struct smelt_node *node);

/**
 * @brief checks whether the calling node is the root of the tree
 *
 * @returns TRUE if the node is the root, FALSE otherwise
 */
bool smlt_node_is_root(void);

/**
 * @brief checks whether the callnig node is a leaf in the tree
 *
 * @returns TRUE if the node is a leaf, FALSE otherwise
 */
bool smlt_node_is_leaf(void);

/**
 * @brief gets the parent of the calling node
 *
 * @returns pointer to the parent, NULL if the root
 */
struct smlt_node *smlt_node_get_parent(void);

/**
 * @brief gets the child nodes of the calling node
 *
 * @param count returns the number of children
 *
 * @returns array of pointer to childnodes
 */
struct smlt_node **smlt_node_get_children(uint32_t *count);

/**
 * @brief gets the number of nodes in the system
 *
 * @returns integer
 */
uint32_t smlt_node_get_num_nodes();


/*
 * ===========================================================================
 * sending function
 * ===========================================================================
 */


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