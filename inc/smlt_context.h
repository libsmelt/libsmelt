/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_CONTEXT_H_
#define SMLT_CONTEXT_H_ 1


#define SMLT_CONTEXT_CHECK(_ctx)

/*
 * ===========================================================================
 * type declarations
 * ===========================================================================
 */


/**
 * represents a handle to a smelt context.  XXX: having a typedef ?
 */
struct smlt_context;


/*
 * ===========================================================================
 * Smelt context creation
 * ===========================================================================
 */


/**
 * @brief creates a new smelt context from the topology
 *
 * @param topo      Smelt topology to create the context from
 * @param ret_ctx   returns the new Smelt context
 *
 * @return  SMLT_ERR_MALLOC_FAIL
 *          SMLT_SUCCESS
 */
errval_t smlt_context_create(struct smlt_topology *topo,
                             struct smlt_context **ret_ctx);

/**
 * @brief destroys the Smelt context and frees its resources
 *
 * @param ctx   Smelt context to destroy
 *
 * @return  SMLT_SUCCESS
 */
errval_t smlt_context_destroy(struct smlt_context *ctx);


/*
 * ===========================================================================
 * Smelt context creation
 * ===========================================================================
 */

struct smlt_channel *smlt_context_get_parent_chan(struct smlt_context *ctx);

struct smlt_channel *smlt_context_get_parent_of_node(struct smlt_context *ctx,
                                                     struct smlt_node *node);

bool smlt_context_is_root(struct smlt_context *ctx);

bool smlt_context_is_leaf(struct smlt_context *ctx);



#endif /* SMLT_CONTEXT_H_ */
