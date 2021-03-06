/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <smlt.h>
#include <smlt_topology.h>
#include <smlt_context.h>
#include <smlt_channel.h>
#include "smlt_debug.h"

#include <stdio.h>

#define SMLT_CONTEXT_NAME_MAX 16

/*
 * ===========================================================================
 * type definitions
 * ===========================================================================
 */


struct smlt_context_node
{
    smlt_nid_t node_id;
    struct smlt_channel *parent;
    struct smlt_channel *children;
    uint32_t num_children;
    uint32_t index;
};

/**
 * represents a handle to a smelt context.  XXX: having a typedef ?
 */
struct smlt_context
{
    struct smlt_topology *topology;
    char name[SMLT_CONTEXT_NAME_MAX];
    uint32_t num_nodes;
    smlt_nid_t max_nid;
    struct smlt_context_node **nid_to_node;
    struct smlt_context_node all_nodes[];
};


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
                             struct smlt_context **ret_ctx)
{

    struct smlt_context *ctx;

    if (topo == NULL) {
        return SMLT_ERR_INVAL;
    }

    uint32_t num_nodes = smlt_topology_get_num_nodes(topo);

    ctx = (struct smlt_context*) smlt_platform_alloc(sizeof(*ctx) + num_nodes * sizeof(struct smlt_context_node),
                              SMLT_ARCH_CACHELINE_SIZE, true);

    if (ctx == NULL) {
        return SMLT_ERR_MALLOC_FAIL;
    }
    ctx->num_nodes = num_nodes;
    ctx->max_nid = 0;
    /* loop over the nodes and allocate resources */

    struct smlt_topology_node *tn = smlt_topology_get_first_node(topo);
    for (uint32_t i = 0; i < num_nodes; ++i) {
        struct smlt_context_node *n = &ctx->all_nodes[i];
        smlt_nid_t current_nid = smlt_topology_node_get_id(tn);
        uint32_t num_children = smlt_topology_node_get_num_children(tn);

        n->node_id = current_nid;
        n->num_children = num_children;
        if (num_children) {

            // if we use shared memory need to allocate additional channel
            if (smlt_topology_node_use_shm(tn)) {
                n->children = (struct smlt_channel*) smlt_platform_alloc\
                    ((num_children+1)* sizeof(*(n->children)),
                     SMLT_ARCH_CACHELINE_SIZE, true);
                assert (n->children != NULL);
            } else {
                n->children = (struct smlt_channel*) smlt_platform_alloc\
                    (num_children * sizeof(*(n->children)),
                     SMLT_ARCH_CACHELINE_SIZE, true);
                assert (n->children != NULL);
            }

            /* setup channels */
            struct smlt_topology_node **children;
            children = smlt_topology_node_children(tn, &num_children);

            for (unsigned int i = 0; i < num_children; i++) {
                uint32_t dst = smlt_topology_node_get_id(children[i]);
                uint32_t src = smlt_topology_node_get_id(tn);
                struct smlt_channel * chan = &(n->children[i]);
                smlt_channel_create(&chan, &src,
                                    &dst, 1, 1);
            }
        }

        if ((num_children == 0) && smlt_topology_node_use_shm(tn)) {
            n->children = (struct smlt_channel*)
                smlt_platform_alloc(sizeof(*(n->children)),
                                    SMLT_ARCH_CACHELINE_SIZE, true);
            assert (n->children!=NULL);
        }

        // shared memory children (TODO adds the shm channel at the end)
        if (smlt_topology_node_use_shm(tn)) {
            uint32_t num_children_shm = smlt_topology_node_get_num_children_shm(tn);
            if (num_children_shm) {
                /* setup channel */
                struct smlt_topology_node **children;
                children = smlt_topology_node_children_shm(tn, &num_children_shm);
                uint32_t* dst = (uint32_t*) smlt_platform_alloc(num_children_shm*sizeof(uint32_t),
                                                    SMLT_ARCH_CACHELINE_SIZE, true);

                uint32_t src = smlt_topology_node_get_id(tn);
                // num_children is of the MP children
                struct smlt_channel * chan = &(n->children[n->num_children]);
                for (unsigned i = 0; i < num_children_shm; i++) {
                    dst[i] = smlt_topology_node_get_id(children[i]);
                }

                smlt_channel_create(&chan, &src,
                                    dst, 1, num_children_shm);
                n->num_children++;
            }
        }

        /* update maximum node id */
        if (ctx->max_nid  < current_nid) {
            ctx->max_nid  = current_nid;
        }

        tn = smlt_topology_node_next(tn);
    }

    ctx->nid_to_node = (struct smlt_context_node**) smlt_platform_alloc\
        ((ctx->max_nid+1)  * sizeof(void *),
         SMLT_ARCH_CACHELINE_SIZE, true);
    assert(ctx->nid_to_node!=NULL);

    for (uint32_t i = 0; i < num_nodes; ++i) {
        struct smlt_context_node *n = &ctx->all_nodes[i];
        ctx->nid_to_node[n->node_id] = n;
    }

    tn = smlt_topology_get_first_node(topo);

    for (uint32_t i = 0; i < num_nodes; ++i) {
        smlt_nid_t current_nid = smlt_topology_node_get_id(tn);

        if (smlt_topology_node_is_root(tn)) {
            tn = smlt_topology_node_next(tn);
            continue;
        }

        struct smlt_topology_node *tp = smlt_topology_node_parent(tn);

        smlt_nid_t parent_nid = smlt_topology_node_get_id(tp);

        struct smlt_context_node *parent = ctx->nid_to_node[parent_nid];
        struct smlt_context_node *n = ctx->nid_to_node[current_nid];

        if (smlt_topology_node_use_shm(tp)) {
            bool child_use_shm = false;
            struct smlt_topology_node **children;
            uint32_t num_children;
            children = smlt_topology_node_children_shm(tp, &num_children);
            for (unsigned j = 0; j < num_children; j++) {
                if (smlt_topology_node_get_id(children[j]) == i) {
                    child_use_shm = true;
                }
            }

            if (child_use_shm) {
                n->parent = &parent->children[parent->num_children-1];
                n->index = smlt_topology_node_get_child_idx_shm(tn);
            } else {
                n->parent = &parent->children[smlt_topology_node_get_child_idx(tn)];
                n->index = smlt_topology_node_get_child_idx(tn);
            }
        } else {
            n->parent = &parent->children[smlt_topology_node_get_child_idx(tn)];
        }
        tn = smlt_topology_node_next(tn);
    }

    *ret_ctx = ctx;
    return SMLT_SUCCESS;
}

/**
 * @brief destroys the Smelt context and frees its resources
 *
 * @param ctx   Smelt context to destroy
 *
 * @return  SMLT_SUCCESS
 */
errval_t smlt_context_destroy(struct smlt_context *ctx)
{
    for (uint32_t i = 0; i < ctx->num_nodes; ++i) {
        /* TODO: create the bindings */
    }

    smlt_platform_free(ctx);

    return SMLT_SUCCESS;
}


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
                                                 uint32_t *ret_count)
{
    if (ctx->max_nid < node->id) {
        return -1; /* TODO: ERROR VALUE */
    }

    if (ret_chan) {
        *ret_chan = ctx->nid_to_node[node->id]->children;
    }
    if (ret_count) {
        *ret_count = ctx->nid_to_node[node->id]->num_children;
    }

    return SMLT_SUCCESS;
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
                                              struct smlt_channel **ret_chan)
{
    if (ctx->max_nid < node->id) {
        return -1; /* TODO: ERROR VALUE */
    }

    if (ret_chan) {
        *ret_chan  = ctx->nid_to_node[node->id]->parent;
    }
    return SMLT_SUCCESS;
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
                               struct smlt_node *node)
{
    if (ctx->max_nid < node->id) {
        return 0;
    }
    return (ctx->nid_to_node[node->id]->parent == NULL);
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
                               struct smlt_node *node)
{
    if (ctx->max_nid < node->id) {
        return 0;
    }
    return (ctx->nid_to_node[node->id]->num_children == 0);
}


/**
 * @brief gets the index into receiving array
 *
 * @param ctx   Smelt context
 * @param node  Smelt node
 *
 * @return the index
 */
uint32_t smlt_context_node_get_child_idx(struct smlt_context *ctx)
{
    return ctx->nid_to_node[smlt_node_self_id]->index;
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
                                          struct smlt_node *node)
{
    if (ctx->max_nid < node->id) {
        return 0;
    }

    /* TODO: impelemt */
    assert(!"NYI");
    return 0;
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
                                            struct smlt_node *node)
{
    if (ctx->max_nid < node->id) {
        return 0;
    }
    /* TODO: impelemt */
    assert(!"NYI");
    return 0;
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
                                          struct smlt_node *node)
{
    if (ctx->max_nid < node->id) {
        return 0;
    }
    assert(!"NYI");
    return 0;
}
