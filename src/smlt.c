/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <smlt.h>
#include <smlt_node.h>
#include <smlt_topology.h>
#include <shm/smlt_shm.h>
#include  "debug.h"

uint8_t smlt_initialized = 0;

uint32_t smlt_gbl_num_proc = 0;

static struct smlt_node **smlt_gbl_all_nodes;
static uint32_t smlt_gbl_all_node_count;


/**
 * @brief initializes the Smelt library
 *
 * @param num_proc  the number of processors
 * @param eagerly   create an all-to-all connection mesh
 *
 * @returns SMLT_SUCCESS on success
 *
 * This has to be executed once per address space. If threads are used
 * for parallelism, call this only once. With processes, it has to be
 * executed on each process.
 */
errval_t smlt_init(uint32_t num_proc, bool eagerly)
{
    errval_t err;

    if (num_proc > sysconf(_SC_NPROCESSORS_ONLN)) {

        SMLT_NOTICE("Not enough cores to initialize Smelt, exiting\n");
        return SMLT_ERR_PLATFORM_INIT;
    }

    SMLT_NOTICE("Initializing Smelt RT version=%s, num_proc=%" PRIu32 "\n",
                 SMLT_VERSION, num_proc);

    /* platform specific initializiation */
    smlt_gbl_num_proc = smlt_platform_init(num_proc);

    SMLT_DEBUG(SMLT_DBG__INIT, "Setting number of nodes to %" PRIu32
               ", requested = %" PRIu32 "\n", smlt_gbl_num_proc, num_proc);

    if (smlt_gbl_num_proc == 0) {
        SMLT_ERROR("Platform initialization failed\n");
        /* there was an error while initializing */
        return SMLT_ERR_PLATFORM_INIT;
    }

    // Debug output
#ifdef SMLT_DEBUG_ENABLED
    SMLT_WARNING("Debug flag (SMLT_DEBUG_ENABLED) is set.\n");
#endif

#ifdef SYNC_DEBUG_BUILD
    SMLT_WARNING("Compiler optimizations are off\n");
#endif

    // Master share allows simple barriers; needed for boot-strapping
    SMLT_DEBUG(SMLT_DBG__INIT, "Initializing master share .. \n");

    err = smlt_shm_init_master_share();
    if (smlt_err_is_fail(err)) {
        SMLT_ERROR("failed to initialize the master share\n");
        return smlt_err_push(err, SMLT_ERR_SHM_INIT);
    }

    // Initialize barrier
    smlt_platform_barrier_init(&smlt_shm_get_master_share()->data.sync_barrier,
                               NULL, smlt_gbl_num_proc);

    if (!eagerly) {
        return SMLT_SUCCESS;
    }

    SMLT_DEBUG(SMLT_DBG__INIT, "Allocating %" PRIu64 " bytes for %" PRIu32 " nodes "
               "smlt_node=%" PRIu64 ", smlt_qp=%" PRIu64 "\n",
               SMLT_NODE_SIZE(smlt_gbl_num_proc), smlt_gbl_num_proc,
               sizeof(struct smlt_node), sizeof(struct smlt_qp));

    smlt_gbl_all_nodes = smlt_platform_alloc(smlt_gbl_num_proc * sizeof(void *),
                                             SMLT_ARCH_CACHELINE_SIZE, true);
    if (smlt_gbl_all_nodes == NULL) {
        /* TODO: cleanup master share */
        SMLT_ERROR("failed to allocate the node\n");
        return SMLT_ERR_MALLOC_FAIL;
    }

    /* creating the nodes */

    SMLT_DEBUG(SMLT_DBG__INIT, "Creating %" PRIu32 " nodes\n", smlt_gbl_num_proc);
    for (uint32_t i=0; i<smlt_gbl_num_proc; i++) {
        struct smlt_node_args args = {
            .id = i,
            .core = i,
            .num_nodes = smlt_gbl_num_proc,
        };
        err = smlt_node_create(&smlt_gbl_all_nodes[i], &args);
        if (smlt_err_is_fail(err)) {
            SMLT_ERROR("failed to create the node\n");
            return smlt_err_push(err, SMLT_ERR_NODE_CREATE);
        }
        smlt_gbl_all_node_count++;
    }


    // setup channels
    for (uint32_t i = 0; i < smlt_gbl_num_proc; i++) {
        for (uint32_t j = i+1; j < smlt_gbl_num_proc; j++) {
            struct smlt_channel* chan = &(smlt_gbl_all_nodes[i]->chan[j]);
            err = smlt_channel_create(&chan , &i, &j, 1, 1);
            smlt_gbl_all_nodes[j]->chan[i] = smlt_gbl_all_nodes[i]->chan[j];
        }
    }

    /* initialize the topology subsystem */
    err = smlt_topology_init();
    if (smlt_err_is_fail(err)) {
        return smlt_err_push(err, SMLT_ERR_TOPOLOGY_INIT);
    }

    smlt_initialized = 1;

    return SMLT_SUCCESS;
}

/**
 * @brief obtains the node based on the id
 *
 * @param id    node id
 *
 * @return pointer ot the Smelt node
 */
struct smlt_node *smlt_get_node_by_id(smlt_nid_t id)
{
    if (id < smlt_gbl_all_node_count) {
        return smlt_gbl_all_nodes[id];
    }
    return NULL;
}

errval_t smlt_add_node(struct smlt_node *node)
{

    // smlt_node_set_id(smlt_all_node_count++);
    // smlt_all_nodes[smlt_node_get_id(node)] = node;

     return SMLT_SUCCESS;
}

/*
 * ===========================================================================
 * sending functions
 * ===========================================================================
 */


/**
 * @brief sends a message on the to the node
 *
 * @param ep    the Smelt node to call the operation on
 * @param msg   Smelt message argument
 *
 * @returns error value
 *
 * This function is BLOCKING if the node cannot take new messages
 */
errval_t smlt_send(smlt_nid_t nid, struct smlt_msg *msg)
{
    struct smlt_node *node = smlt_get_node_by_id(nid);
    if (node==NULL) {
        return SMLT_ERR_NODE_INVALD;
    }

    return smlt_node_send(node, msg);
}

/**
 * @brief sends a notification (zero payload message)
 *
 * @param node    the Smelt node to call the operation on
 * @param msg   Smelt message argument
 *
 * @returns error value
 */
errval_t smlt_notify(smlt_nid_t nid)
{
    struct smlt_node *node = smlt_get_node_by_id(nid);
    if (node==NULL) {
        return SMLT_ERR_NODE_INVALD;
    }

    return smlt_node_notify(node);
}

/**
 * @brief checks if the a message can be sent on the node
 *
 * @param node    the Smelt node to call the check function on
 *
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 */
bool smlt_can_send(smlt_nid_t nid)
{
    struct smlt_node *node = smlt_get_node_by_id(nid);
    if (node==NULL) {
        return false;
    }

    return smlt_node_can_send(node);;
}


/*
 * ===========================================================================
 * receiving functions
 * ===========================================================================
 */

/**
 * @brief receives a message or a notification from any incoming channel
 *
 * @param msg   Smelt message argument
 *
 * @returns error value
 *
 * this function is BLOCKING if there is no message on the node
 */
errval_t smlt_recv_any(struct smlt_msg *msg)
{
    /* TODO */
    assert(!"NYI");

    return SMLT_SUCCESS;
}

/**
 * @brief receives a message or a notification from the node
 *
 * @param node    the Smelt node to call the operation on
 * @param msg   Smelt message argument
 *
 * @returns error value
 *
 * this function is BLOCKING if there is no message on the node
 */
errval_t smlt_recv(smlt_nid_t nid, struct smlt_msg *msg)
{
    struct smlt_node *node = smlt_get_node_by_id(nid);
    if (node==NULL) {
        return SMLT_ERR_NODE_INVALD;
    }

    return smlt_node_recv(node, msg);
}

/**
 * @brief checks if there is a message to be received
 *
 * @param node    the Smelt node to call the check function on
 *
 * @returns TRUE if the operation can be executed
 *          FALSE otherwise
 *
 * this invokes either the can_send or can_receive function
 */
bool smlt_can_recv(smlt_nid_t nid)
{
    struct smlt_node *node = smlt_get_node_by_id(nid);
    if (node==NULL) {
        return false;
    }

    return smlt_node_can_recv(node);
}

/**
 * @brief returns the number of processes (or threads) running
 *
 * @returns number of processes/threads participating
 *
 * this invokes either the can_send or can_receive function
 */
uint32_t smlt_get_num_proc(void)
{
    return smlt_gbl_num_proc;
}
