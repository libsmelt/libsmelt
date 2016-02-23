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

#include <smlt_queuepair.h>

/**
 * @brief
 * @param arg
 */
typedef void *(*smlt_node_start_fn_t)(void *arg);


/*
 * ===========================================================================
 * type declarations
 * ===========================================================================
 */
struct smlt_node
{
    smlt_nid_t id;
    coreid_t core;

    smlt_platform_node_handle_t handle;
    smlt_node_start_fn_t fn;
    void *arg;
 //   cycles_t tsc_start;

    /* flags */
    uint8_t mp_recv;
    uint8_t mp_send;
    uint8_t shm_send;
    uint8_t shm_recv;

    struct smlt_qp qp[];  // XXX: we need multiple queue pairs here
};

extern __thread struct smlt_node *smlt_node_self;
extern __thread smlt_nid_t smlt_node_self_id; ///< caches the node id

/**
 * arguments passed to the the new smelt node
 */
struct smlt_node_args
{
    smlt_nid_t id;          ///< the node ID to set
    coreid_t core;          ///< ID of the core to start the thread on
    uint32_t num_nodes;     ///< number of nodes in the system
};

#define SMLT_NODE_CHECK(_node)

#define SMLT_NODE_SIZE(_num) ((sizeof(struct smlt_node)          \
                                 + _num * sizeof(struct smlt_qp)))

/*
 * ===========================================================================
 * node management functions
 * ===========================================================================
 */ 


/**
 * @brief creates a new smelt node 
 *
 * @param node  Smelt node to initialize
 * @param args  arguments for the creation
 *
 */
errval_t smlt_node_create(struct smlt_node **node,
                          struct smlt_node_args *args);

/**
 * @brief starts the execution of the Smelt node
 *
 * @param node  the Smelt node
 * @param fn    function to call
 * @param arg   argument of the function
 *
 * @return SMELT_SUCCESS if the node has been started
 *         error value otherwise
 */
errval_t smlt_node_start(struct smlt_node *node, smlt_node_start_fn_t fn, void *arg);

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

/**
 * @brief runs the start function of the node
 *
 * @param node  the Smelt ndoe
 *
 * @return foo bar
 */
static inline void *smlt_node_run(struct smlt_node *node)
{
    return node->fn(node);
}

/**
 * @brief this function has to be called when the thread is executed first
 *
 * @param node  the Smelt node
 *
 * @return SMLT_SUCCESS
 */
errval_t smlt_node_exec_start(struct smlt_node *node); // int  __thread_init(coreid_t,int);

/**
 * @brief this function has to be called when the thread is executed first
 *
 * @param node  the Smelt node
 *
 * @return SMLT_SUCCESS
 */
errval_t smlt_node_exec_end(struct smlt_node *node); //int  __thread_end(void);


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
static inline smlt_nid_t smlt_node_get_id_of_node(struct smlt_node *node)
{
    return node->id;
}

/**
 * @brief gets the ID of the current node
 *
 * @param node  the smelt node
 *
 * @returns integer value representing the node id
 */
static inline smlt_nid_t smlt_node_get_coreid_of_node(struct smlt_node *node)
{
    return node->core;
}

/**
 * @brief gets the core ID of the current node
 *
 * @returns integer value representing the coreid
 */
static inline smlt_nid_t smlt_node_get_coreid(void)
{
    return smlt_node_get_coreid_of_node(smlt_node_self);
}



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
    
    return smlt_queuepair_send(&node->qp[smlt_node_self_id], msg);
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
    return smlt_queuepair_notify(&node->qp[smlt_node_self_id]);
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
    
    return smlt_queuepair_can_send(&node->qp[smlt_node_self_id]);
}

/* TODO: include also non blocking variants ? */


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
    
    return smlt_queuepair_recv(&node->qp[smlt_node_self_id], msg);
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

    return smlt_queuepair_can_recv(&node->qp[smlt_node_self_id]);
}


#endif /* SMLT_NODE_H_ */
