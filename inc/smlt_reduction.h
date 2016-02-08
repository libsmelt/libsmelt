/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_REDUCTION_H_
#define SMLT_REDUCTION_H_ 1


/**
 * @brief performs a reduction 
 * 
 * @param msg       input for the reduction
 * @param result    returns the result of the reduction
 * 
 * @returns TODO:errval
 */
errval_t smlt_reduce(struct smlt_msg *input,
                     struct smlt_msg *result);

/**
 * @brief performs a reduction without any payload
 *
 * @returns TODO:errval
 */
errval_t smlt_reduce_notify(void);

/**
 * @brief performs a reduction and distributes the result to all nodes
 * 
 * @param msg       input for the reduction
 * @param result    returns the result of the reduction
 * 
 * @returns TODO:errval
 */
errval_t smlt_reduce_all(struct smlt_msg *input,
                         struct smlt_msg *result);


//uintptr_t sync_reduce(uintptr_t);
//uintptr_t sync_reduce0(uintptr_t);


#endif /* SMLT_REDUCTION_H_ */