/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_BROADCAST_H_
#define SMLT_BROADCAST_H_ 1


/*
 * ===========================================================================
 * Smelt broadcast: higher level functions
 * ===========================================================================
 */


/**
 * @brief performs a broadcast to the nodes of the subtree routed at the 
 *        calling node
 * 
 * @param msg       input for the reduction
 * @param result    returns the result of the reduction
 * 
 * @returns TODO:errval
 */
errval_t smlt_broadcast_subtree(struct smlt_msg *msg);

/**
 * @brief performs a broadcast without any payload to the subtree routed at
 *        the calling node
 *
 * @returns TODO:errval
 */
errval_t smlt_broadcast_notify_subtree(void);

/**
 * @brief performs a broadcast to all nodes on the current active instance 
 * 
 * @param msg       input for the reduction
 * @param result    returns the result of the reduction
 * 
 * @returns TODO:errval
 */
errval_t smlt_broadcast(struct smlt_msg *msg);

/**
 * @brief performs a broadcast without any payload to all nodes
 *
 * @returns TODO:errval
 */
errval_t smlt_broadcast_notify(void);


/*
 * ===========================================================================
 * Smelt broadcast: lower level functions
 * ===========================================================================
 */


/*
  TODO
  uintptr_t mp_receive_forward(uintptr_t);  
  uintptr_t ab_forward(uintptr_t, coreid_t);
*/
#endif /* SMLT_BROADCAST_H_ */
