/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */


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
    struct smlt_node *node = smlt_get_node(nid);
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
    struct smlt_node *node = smlt_get_node(nid);
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
    struct smlt_node *node = smlt_get_node(nid);
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
    assert(!"NUI");
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
    struct smlt_node *node = smlt_get_node(nid);
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
    struct smlt_node *node = smlt_get_node(nid);
    if (node==NULL) {
        return false;
    }

    return smlt_can_receive(node);
}

