/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_SMLT_H_
#define SMLT_SMLT_H_ 1

#include <smlt_config.h>
#include <smlt_platform.h>
#include <smlt_error.h>
#include <smlt_message.h>



/*
 * ===========================================================================
 * type declarations
 * ===========================================================================
 */


///< the smelt node id        XXX or is this the endpoint id ?
typedef uint32_t smlt_nid_t;

/**
 * represents a handle to a smelt instance.
 */
struct smlt_inst;

/*
 * ===========================================================================
 * function declarations
 * ===========================================================================
 */

#define SMLT_USE_ALL_CORES ((uint32_t)-1)

/**
 * @brief initializes the Smelt library
 *
 * @param num_proc  the number of processors
 *
 * @returns SMLT_SUCCESS on success
 * 
 * This has to be executed once per address space. If threads are used
 * for parallelism, call this only once. With processes, it has to be
 * executed on each process.
 */
errval_t smlt_init(uint32_t num_proc);


/**
 * @brief creates a new smelt instance
 */
errval_t smlt_instance_create();

/**
 * @brief destroys a smelt instance.
 */
errval_t smlt_instance_destroy();



struct smlt_inst *smlt_get_current_instance(void);

errval_t smlt_switch_instance(void);

errval_t smlt_add_node(void); //void add_binding(coreid_t sender, coreid_t receiver, mp_binding *mp);

struct smlt_node *smlt_get_node_by_id(smlt_nid_t id);

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
errval_t smlt_send(smlt_nid_t nid, struct smlt_msg *msg);

/**
 * @brief sends a notification (zero payload message) 
 * 
 * @param node    the Smelt node to call the operation on
 * @param msg   Smelt message argument
 * 
 * @returns error value
 */
errval_t smlt_notify(smlt_nid_t nid);

/**
 * @brief checks if the a message can be sent on the node
 * 
 * @param node    the Smelt node to call the check function on
 * 
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 */
bool smlt_can_send(smlt_nid_t nid);

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
errval_t smlt_recv(smlt_nid_t nid, struct smlt_msg *msg);

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
bool smlt_can_recv(smlt_nid_t nid);


#endif /* SMLT_SMLT_H_ */
