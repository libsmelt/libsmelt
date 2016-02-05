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


/**
 * @brief performs a broadcast on the current active instance 
 * 
 * @param msg       input for the reduction
 * @param result    returns the result of the reduction
 * 
 * @returns TODO:errval
 */
errval_t smlt_broadcast(struct smlt_msg *input);

/**
 * @brief performs a broadcast without any payload
 *
 * @returns TODO:errval
 */
errval_t smlt_broadcast_notify(void);

/*
  TODO
  uintptr_t mp_receive_forward(uintptr_t);
  void mp_receive_forward7(uintptr_t*);
  void mp_receive_forward0(void);
*/
#endif /* SMLT_BROADCAST_H_ */