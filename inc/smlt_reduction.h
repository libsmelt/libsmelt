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

/* forward declaration */
struct smlt_context;
struct smlt_msg;

/**
 * @brief operation to be called when performing the aggregate
 *
 * @param dest  destination message pointer.
 * @param src   source message
 *
 */
typedef errval_t (*smlt_reduce_fn_t)(struct smlt_msg *dest, struct smlt_msg *src);


/**
 * @brief performs a reduction on the current instance
 * 
 * @param ctx       The smelt context
 * @param msg        input for the reduction
 * @param result     returns the result of the reduction
 * @param operation  function to be called to calculate the aggregate
 *
 * @returns TODO:errval
 */
errval_t smlt_reduce(struct smlt_context *ctx,
                     struct smlt_msg *input,
                     struct smlt_msg *result,
                     smlt_reduce_fn_t operation);


/**
 * @brief checks if the children already send something for the reduction
 *
 * @param ctx       The smelt context
 *
 * @returns bool if there is something to receive
 */
bool smlt_reduce_can_recv(struct smlt_context *ctx);

/**
 * @brief performs a reduction without any payload on teh current instance
 *
 * @param ctx       The smelt context
 *
 * @returns TODO:errval
 */
errval_t smlt_reduce_notify(struct smlt_context *ctx);

/**
 * @brief performs a reduction and distributes the result to all nodes
 * 
 * @param ctx       The smelt context
 * @param msg       input for the reduction
 * @param result    returns the result of the reduction
 * @param operation  function to be called to calculate the aggregate
 * 
 * @returns TODO:errval
 */
errval_t smlt_reduce_all(struct smlt_context *ctx,
                         struct smlt_msg *input,
                         struct smlt_msg *result,
                         smlt_reduce_fn_t operation);


//uintptr_t sync_reduce(uintptr_t);
//uintptr_t sync_reduce0(uintptr_t);


#endif /* SMLT_REDUCTION_H_ */
