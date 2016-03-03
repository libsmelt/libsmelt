/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_TOPOLOGY_H_
#define SMLT_TOPOLOGY_H_ 1

/*
 * ===========================================================================
 * type declarations
 * ===========================================================================
 */


/**
 * represents a handle to a smelt topology.
 */
struct smlt_topology;
struct smlt_topology_node;
struct smlt_generated_model;

///< refer to the current smelt topology
#define SMLT_TOPOLOGY_CURRENT NULL;


/*
 * ===========================================================================
 * creating / destroying Smelt topologies
 * ===========================================================================
 */

/**
 * @brief initializes the topology subsystem
 *
 * @return SMLT_SUCCESS or error value
 */
errval_t smlt_topology_init(void);

/**
 * @brief creates a new Smelt topology out of the model parameter
 *
 * @param model         the model input data from the simulator
 * @param length        length of the model input
 * @param name          name of the topology
 * @param ret_topology  returned pointer to the topology
 *
 * @return SMELT_SUCCESS or error value
 *
 * If the model is NULL, then a binary tree will be generated
 */
errval_t smlt_topology_create(struct smlt_generated_model* model,
                              const char *name,
                              struct smlt_topology *ret_topology);

/**
 * @brief destroys a smelt topology.
 *
 * @param topology  the Smelt topology to destroy
 *
 * @return SMELT_SUCCESS or error vlaue
 */
errval_t smlt_topology_destroy(struct smlt_topology *topology);


/*
 * ===========================================================================
 * topology nodes
 * ===========================================================================
 */


/**
 * @brief returns the first topology node
 *
 * @param topo  the Smelt topology
 *
 * @return Pointer to the smelt topology node
 */
struct smlt_topology_node *smlt_topology_get_first_node(struct smlt_topology *topo);

/**
 * @brief gets the next topology node in the topology
 *
 * @param node the current topology node
 *
 * @return
 */
struct smlt_topology_node *smlt_topology_node_next(struct smlt_topology_node *node);

/**
 * @brief gets the parent topology ndoe
 *
 * @param node the current topology node
 *
 * @return
 */
struct smlt_topology_node *smlt_topology_node_parent(struct smlt_topology_node *node);


/**
 * @brief gets the children of a topology ndoe
 *
 * @param node the current topology node
 *
 * @return
 */
struct smlt_topology_node **smlt_topology_node_children(struct smlt_topology_node *node,
                                                        uint32_t* children);
/**
 * @brief checks if the topology node is the last
 *
 * @param node the topology node
 *
 * @return TRUE if the node is the last, false otherwise
 */
bool smlt_topology_node_is_last(struct smlt_topology_node *node);

/**
 * @brief checks if the topology node is the root
 *
 * @param node the topology node
 *
 * @return TRUE if the node is the root, false otherwise
 */
bool smlt_topology_node_is_root(struct smlt_topology_node *node);

/**
 * @brief checks if the topology node is a leaf
 *
 * @param node the topology node
 *
 * @return TRUE if the node is a leaf, false otherwise
 */
bool smlt_topology_node_is_leaf(struct smlt_topology_node *node);

/**
 * @brief obtains the child index (the order of the children) from the node
 *
 * @param node  the topology ndoe
 *
 * @return child index
 */
uint32_t smlt_topology_node_get_child_idx(struct smlt_topology_node *node);


/**
 * @brief gets the number of nodes with the the given idx
 *
 * @param node  the topology node
 *
 * @return numebr of children
 */
uint32_t smlt_topology_node_get_num_children(struct smlt_topology_node *node);

/**
 * @brief obtainst he associated node id of the topology node
 *
 * @param node  the Smelt topology node
 *
 * @return Smelt node id
 */
smlt_nid_t smlt_topology_node_get_id(struct smlt_topology_node *node);


/*
 * ===========================================================================
 * queries
 * ===========================================================================
 */


/**
 * @brief obtains the name of the topology
 *
 * @param topo  the Smelt topology
 *
 * @return string representation
 */
const char *smlt_topology_get_name(struct smlt_topology *topo);


/**
 * @brief gets the number of nodes in the topology
 *
 * @param topo  the Smelt topology
 *
 * @return number of nodes
 */
uint32_t smlt_topology_get_num_nodes(struct smlt_topology *topo);


/**
 * @brief gets the cluster size
 *
 * @param coordinator   ??
 * @param clusterid     ??
 *
 * @return size of the smelt cluster
 */
uint32_t smlt_topology_get_cluster_size(coreid_t coordinator, int clusterid);


/**
 * @brief checks if the node does message passing
 *
 * @return TRUE if the node sends or receives messages
 */
static inline bool smlt_topology_does_message_passing(void)
{
    return 0;
    //return (smlt_node_self->mp_send || smlt_node_self->mp_recv);
}

/**
 * @brief checks fi the nodes does shared memory operations
 *
 * @return TRUE if the node uses a shared memory queue
 */
static inline bool smlt_topology_does_shared_memory(void)
{
    return 0;
    //return (smlt_node_self->shm_send || smlt_node_self->shm_recv);
}

/**
 * @brief gets the parent of the calling node
 *
 * @returns pointer to the parent, NULL if the root
 */
static inline struct smlt_node *smlt_topology_get_parent(void)
{
    return NULL;
    //return smlt_node_self->parent;
}

/**
 * @brief gets the child nodes of the calling node
 *
 * @param count returns the number of children
 *
 * @returns array of pointer to childnodes
 */
static inline struct smlt_node **smlt_topology_get_children(uint32_t *count)
{
    /*
    if (count) {
        *count = smlt_node_self->num_children;
    }
    return smlt_node_self->children;
    */
    return NULL;
}

#endif /* SMLT_TOPOLOGY_H_ */
