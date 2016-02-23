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
errval_t smlt_topology_create(void *model, uint32_t length, const char *name,
                              struct smlt_topology **ret_topology);

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
 * switching and obtainig the current topology
 * ===========================================================================
 */


/**
 * @brief switches the Smelt tree topology
 *
 * @param new_topo  new topology to be set
 * @param ret_old   returns the old topology if non-null
 *
 * @return SMELT_SUCCESS or error value
 */
errval_t smlt_topology_switch(struct smlt_topology *new_topo,
                              struct smlt_topology **ret_old);

/**
 * @brief gets the current active Smelt topology
 *
 * @return pointer to the current Smelt topology
 */
struct smlt_topology *smlt_topology_get_active(void);


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
 * @brief gets an array of nodes
 *
 * @param topo  the Smelt topology
 *
 * @return pointer to a nodes array
 */
struct smelt_node **smlt_topology_get_nodes(struct smlt_topology *topo);

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
 * @brief gets the number of nodes in the system
 *
 * @returns integer
 */
uint32_t smlt_topology_get_num_nodes();

/**
 * @brief checks whether the calling node is the root of the tree
 *
 * @returns TRUE if the node is the root, FALSE otherwise
 */
static inline bool smlt_topology_is_root(void)
{
    return 0;
    //return (smlt_node_self->parent == NULL);
}

/**
 * @brief checks whether the callnig node is a leaf in the tree
 *
 * @returns TRUE if the node is a leaf, FALSE otherwise
 */
static inline bool smlt_topology_is_leaf(void)
{
    return 0;
    //return (smlt_node_self->children == NULL);
}

/**
 * @brief checks if the node does message passing
 *
 * @return TRUE if the node sends or receives messages
 */
static inline bool smlt_topology_does_message_passing(void)
{
    return (smlt_node_self->mp_send || smlt_node_self->mp_recv);
}

/**
 * @brief checks fi the nodes does shared memory operations
 *
 * @return TRUE if the node uses a shared memory queue
 */
static inline bool smlt_topology_does_shared_memory(void)
{
    return (smlt_node_self->shm_send || smlt_node_self->shm_recv);
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
static inline struct smlt_node **smlt_node_get_children(uint32_t *count)
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
