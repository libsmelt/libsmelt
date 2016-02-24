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

#include <smlt_node.h>

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
 * Channels
 * ===========================================================================
 */

/**
 * @brief obtains the children channels of the current node
 *
 * @param ctx        Smelt context
 * @param node       Smelt node
 * @param ret_chan   returns an channel array
 * @param ret_count  number of channels in this array
 *
 * @return SMLT_SUCECSS if the returned channel and count are valid
 */
errval_t smlt_context_node_get_children_channels(struct smlt_context *ctx,
                                                 struct smlt_node *node,
                                                 struct smlt_channel **ret_chan,
                                                 uint32_t *ret_count);

/**
 * @brief obtains the children channels of the current node
 *
 * @param ctx        Smelt context
 * @param ret_chan   returns an channel array
 * @param ret_count  number of channels in this array
 *
 * @return SMLT_SUCECSS if the returned channel and count are valid
 */
static inline
errval_t smlt_context_get_children_channels(struct smlt_context *ctx,
                                            struct smlt_channel **ret_chan,
                                            uint32_t *ret_count)
{
    return smlt_context_node_get_children_channels(ctx, smlt_node_self, ret_chan,
                                                   ret_count);
}


/**
 * @brief gets the channel to the parent
 *
 * @param ctx       Smelt context
 * @param node       Smelt node
 * @param ret_chan  returns the channel to the parent, NULL if root
 *
 * @return SMLT_SUCESS if the returned channel is valid
 */
errval_t smlt_context_node_get_parent_channel(struct smlt_context *ctx,
                                              struct smlt_node *node,
                                              struct smlt_channel **ret_chan);

/**
 * @brief gets the channel to the parent
 *
 * @param ctx       Smelt context
 * @param ret_chan  returns the channel to the parent, NULL if root
 *
 * @return SMLT_SUCESS if the returned channel is valid
 */
static inline
errval_t smlt_context_get_parent_channel(struct smlt_context *ctx,
                                         struct smlt_channel **ret_chan)
{
    return smlt_context_node_get_parent_channel(ctx, smlt_node_self, ret_chan);
}



/*
 * ===========================================================================
 * State queries
 * ===========================================================================
 */


/**
 * @brief checks if the node is the root in the context
 *
 * @param ctx   Smelt context
 * @param node  Smelt node
 *
 * @return TRUE if the current node is the root, FALSE otherwise
 */
bool smlt_context_node_is_root(struct smlt_context *ctx,
                               struct smlt_node *node);

/**
 * @brief checks if the current node is the root in the context
 *
 * @param ctx   Smelt context
 *
 * @return TRUE if the current node is the root, FALSE otherwise
 */
static inline bool smlt_context_is_root(struct smlt_context *ctx)
{
    return smlt_context_node_is_root(ctx, smlt_node_self);
}

/**
 * @brief checks if the node is a leaf in the context
 *
 * @param ctx   Smelt context
 * @param node  Smelt node
 *
 * @return TRUE if the current node is a leaf, FALSE otherwise
 */
bool smlt_context_node_is_leaf(struct smlt_context *ctx,
                               struct smlt_node *node);

/**
 * @brief checks if the current node is a leaf in the context
 *
 * @param ctx   Smelt context
 *
 * @return TRUE if the current node is a leaf, FALSE otherwise
 */
static inline bool smlt_context_is_leaf(struct smlt_context *ctx)
{
    return smlt_context_node_is_leaf(ctx, smlt_node_self);
}

/**
 * @brief checks if the node does shared memory operations
 *
 * @param ctx   Smelt context
 * @param node  Smelt node
 *
 * @return TRUE if the node does shm os, FALSE otherwise
 */
bool smlt_context_node_does_shared_memory(struct smlt_context *ctx,
                                          struct smlt_node *node);

/**
 * @brief checks if the current node does shared memory operations
 *
 * @param ctx   Smelt context
 *
 * @return TRUE if the current node does shm os, FALSE otherwise
 */
static inline bool smlt_context_does_shared_memory(struct smlt_context *ctx)
{
    return smlt_context_node_does_shared_memory(ctx, smlt_node_self);
}

/**
 * @brief checks if the node does message passing
 *
 * @param ctx   Smelt context
 * @param node  Smelt node
 *
 * @return TRUE if the node does message passing, FALSE otherwise
 */
bool smlt_context_node_does_message_passing(struct smlt_context *ctx,
                                            struct smlt_node *node);

/**
 * @brief checks if the current node does message passing
 *
 * @param ctx   Smelt context
 *
 * @return TRUE if the current node does message passing, FALSE otherwise
 */
static inline bool smlt_context_does_message_passing(struct smlt_context *ctx)
{
    return smlt_context_node_does_message_passing(ctx, smlt_node_self);
}

/**
 * @brief checks if the node has inter machine links
 *
 * @param ctx   Smelt context
 * @param node  Smelt node
 *
 * @return TRUE if the nodes has inter machine links, FALSE otherwise
 */
bool smlt_context_node_does_inter_machine(struct smlt_context *ctx,
                                          struct smlt_node *node);

/**
 * @brief checks if the current node has inter machine links
 *
 * @param ctx   Smelt context
 *
 * @return TRUE if the nodes has inter machine links, FALSE otherwise
 */
static inline bool smlt_context_does_inter_machine(struct smlt_context *ctx)
{
    return smlt_context_node_does_inter_machine(ctx, smlt_node_self);
}


#endif /* SMLT_CONTEXT_H_ */
