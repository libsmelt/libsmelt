/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <smlt.h>
#include <smlt_node.h>
#include <shm/smlt_shm.h>
#include  "debug.h"

uint8_t smlt_initialized = 0;

uint32_t smlt_gbl_num_proc = 0;

/**
 * @brief initializes the Smelt library
 *
 * @param num_proc  the number of processors
 * @param eagerly   create an all-to-all connection mesh
 *
 * @returns SMLT_SUCCESS on success
 * 
 * This has to be executed once per address space. If threads are used
 * for parallelism, call this only once. With processes, it has to be
 * executed on each process.
 */
errval_t smlt_init(uint32_t num_proc, bool eagerly)
{
    /* platform specific initializiation */
    smlt_gbl_num_proc = smlt_platform_init(num_proc);

    if (smlt_gbl_num_proc == 0) {
        /* there was an error while initializing */
        return SMLT_ERR_INIT;
    }

    SMLT_DEBUG(SMLT_DBG__INIT, "Initializing Smelt runtime with %PRIu32 nodes\n",
               smlt_gbl_num_proc);

    // Debug output
#ifdef SMLT_DEBUG_ENABLED
    SMLT_DEBUG(SMLT_DBG_WARN, "Debug flag (SMLT_DEBUG_ENABLED) is set. "
                               "May cause performance degradation\n");
#endif

#ifdef SYNC_DEBUG_BUILD
    TODOPRINTF("Compiler optimizations are off - "
        "performance will suffer if  BUILDTYPE set to debug (in Makefile)\n");
#endif

    // Master share allows simple barriers; needed for boot-strapping
    SMLT_DEBUG(SMLT_DBG__INIT, "Initializing master share .. \n");

    errval_t err;
    err = smlt_shm_init_master_share();
    if (smlt_err_is_fail(err)) {
        return err;
    }

    // Initialize barrier
    smlt_platform_barrier_init(&smlt_shm_get_master_share()->data.sync_barrier,
                               NULL, smlt_gbl_num_proc);
    
    smlt_initialized = 1;

    return SMLT_SUCCESS;
}


/*
 * ===========================================================================
 * sending functions
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
errval_t smlt_send(smlt_nid_t nid, struct smlt_msg *msg)
{
    struct smlt_node *node = smlt_get_node_by_id(nid);
    if (node==NULL) {
        return SMLT_ERR_NODE_INVALD;
    }

    return smlt_node_send(node, msg);
}

/**
 * @brief sends a notification (zero payload message) 
 * 
 * @param node    the Smelt node to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 */
errval_t smlt_notify(smlt_nid_t nid)
{
    struct smlt_node *node = smlt_get_node_by_id(nid);
    if (node==NULL) {
        return SMLT_ERR_NODE_INVALD;
    }

    return smlt_node_notify(node);
}

/**
 * @brief checks if the a message can be sent on the node
 * 
 * @param node    the Smelt node to call the check function on
 * 
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 */
bool smlt_can_send(smlt_nid_t nid)
{
    struct smlt_node *node = smlt_get_node_by_id(nid);
    if (node==NULL) {
        return false;
    }

    return smlt_node_can_send(node);;
}


/*
 * ===========================================================================
 * receiving functions
 * ===========================================================================
 */

/**
 * @brief receives a message or a notification from any incoming channel
 * 
 * @param msg   Smelt message argument
 * 
 * @returns error value
 *
 * this function is BLOCKING if there is no message on the node
 */
errval_t smlt_recv_any(struct smlt_msg *msg)
{
    /* TODO */
    assert(!"NYI");

    return SMLT_SUCCESS;
}

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
errval_t smlt_recv(smlt_nid_t nid, struct smlt_msg *msg)
{
    struct smlt_node *node = smlt_get_node_by_id(nid);
    if (node==NULL) {
        return SMLT_ERR_NODE_INVALD;
    }

    return smlt_node_recv(node, msg);
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
bool smlt_can_recv(smlt_nid_t nid)
{
    struct smlt_node *node = smlt_get_node_by_id(nid);
    if (node==NULL) {
        return false;
    }

    return smlt_node_can_recv(node);
}

