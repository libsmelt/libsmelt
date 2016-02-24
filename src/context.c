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
};

/**
 * represents a handle to a smelt context.  XXX: having a typedef ?
 */
struct smlt_context
{
    struct smlt_topology *topology;
    char name[SMLT_CONTEXT_NAME_MAX];
    uint32_t num_nodes;
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

    if (ret_ctx == NULL || topo == NULL) {
        return SMLT_ERR_INVAL;
    }

    uint32_t num_nodes = smlt_topology_get_num_nodes(topo);

    ctx = smlt_platform_alloc(sizeof(*ctx) + num_nodes * sizeof(struct smlt_context_node),
                              SMLT_CACHELINE_SIZE, true);

    if (ctx == NULL) {
        return SMLT_ERR_MALLOC_FAIL;
    }

    ctx->num_nodes = num_nodes;

    /* loop over the nodes and allocate resources */
    smlt_nid_t max_nid = 0;
    struct smlt_topology_node *tn = smlt_topology_get_first_node(topo);
    for (uint32_t i = 0; i < num_nodes; ++i) {
        struct smlt_context_node *n = &ctx->all_nodes[i];
        smlt_nid_t current_nid = smlt_topology_node_get_id(tn);
        uint32_t num_children = smlt_topology_node_get_num_children(tn);

        n->node_id = current_nid;
        n->num_children = num_children;

        if (num_children) {
            n->children = smlt_platform_alloc(num_children * sizeof(*(n->children)),
                                       SMLT_CACHELINE_SIZE, true);
            if (!n->children) {
                /* PANIC !*/
            }

            /* setup channels */
        }

        /* update maximum node id */
        if (max_nid < current_nid) {
            max_nid = current_nid;
        }

        tn = smlt_topology_node_next(tn);
    }

    ctx->nid_to_node = smlt_platform_alloc(max_nid * sizeof(void *),
                                           SMLT_CACHELINE_SIZE, true);
    if (!ctx->nid_to_node) {

    }

    for (uint32_t i = 0; i < num_nodes; ++i) {
        struct smlt_context_node *n = &ctx->all_nodes[i];
        ctx->nid_to_node[n->node_id] = n;
    }

    tn = smlt_topology_get_first_node(topo);

    for (uint32_t i = 0; i < num_nodes; ++i) {

        smlt_nid_t current_nid = smlt_topology_node_get_id(tn);

        if (smlt_topology_node_is_root(tn)) {
            continue;
        }

        struct smlt_topology_node *tp = smlt_topology_node_parent(tn);

        smlt_nid_t parent_nid = smlt_topology_node_get_id(tp);

        struct smlt_context_node *parent = ctx->nid_to_node[parent_nid];
        struct smlt_context_node *n = ctx->nid_to_node[current_nid];

        n->parent = &parent->children[smlt_topology_node_get_child_idx(tn)];

        //ctx->nid_to_node[current_nid]->parent = ctx->nid_to_node[parent_nid];

        tn = smlt_topology_node_next(tn);
    }

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
        /* todo: create the bindings */
    }

    smlt_platform_free(ctx);

    return SMLT_SUCCESS;
}
