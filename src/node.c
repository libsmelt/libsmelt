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
#include "internal.h"

__thread struct smlt_node *smlt_node_self;


/*
 * ==============================================================================
 *
 * ==============================================================================
 */

/**
 * @brief creates a new smelt node
 *
 * @param node  Smelt node to initialize
 * @param args  arguments for the creation
 *
 */
errval_t smlt_node_create(struct smlt_node *node,
                          struct smlt_node_args *args)
{
    SMLT_DEBUG(SMLT_DBG__NODE, "creating new node id=%" PRIu32 "\n", args->id);

    if (!node) {
        return SMLT_ERR_INVAL;
    }

    node->id = args->id;
    node->core = args->core;

    return smlt_platform_node_create(node);
}


/**
 * @brief starts the execution of the Smelt node
 *
 * @param node  the Smelt node
 * @param fn    function to call
 * @param arg   argument of the function
 *
 * @return SMELT_SUCCESS if the node has been started
 *         error value otherwise
 */
errval_t smlt_node_start(struct smlt_node *node, smlt_node_start_fn_t fn, void *arg)
{
    SMLT_DEBUG(SMLT_DBG__NODE, "smlt_node_start with node=%p, fn=%p\n", node, fn);

    /* set the start function and the arguments */
    node->fn = fn;
    node->arg = arg;

   // node->tsc_start = rdtsc();

    return smlt_platform_node_start(node);
}

/**
 * @brief waits for the other node to terminate
 *
 * @param node the other Smelt node
 *
 * @returns  TODO: errval
 */
errval_t smlt_node_join(struct smlt_node *node)
{
    return smlt_platform_node_join(node);
}


/**
 * @brief terminates the othern node and waits for termination
 *
 * @param node the other Smelt node
 *
 * @returns  TODO: errval
 */
errval_t smlt_node_cancel(struct smlt_node *node)
{
    return SMLT_SUCCESS;
}

/**
 * \brief Initialize thread for use of sync library.
 *
 * Setup of the bare minimum of functionality required for threads to
 * work with message passing.
 *
 * \param _tid The id of the thread
 * \return Always returns 0
 */
errval_t smlt_node_lowlevel_init(smlt_nid_t nid)
{
    errval_t err;

    err = smlt_platform_thread_start_hook();
    if (smlt_err_is_fail(err)) {
        // XXX:
        return err;
    }

    coreid_t core = nid;
    if (smlt_node_self != NULL) {
        core = smlt_node_self->core;
    }

    SMLT_DEBUG(SMLT_DBG__NODE, "lowlevel thread init \n");

    err = smlt_platform_pin_thread(core);
    if (smlt_err_is_fail(err)) {
        // XXX:
        return err;
    }

    return SMLT_SUCCESS;
}

/**
 * @brief this function has to be called when the thread is executed first
 *
 * @param node  the Smelt node
 *
 * @return SMLT_SUCCESS
 *
 * Has to be executed by each thread exactly once. One of the threads
 * has to have id 0, since this causes additional initialization.
 *
 * Internally calls __lowlevel_thread_init. In addition to that,
 * initializes a broadcast tree from given model.
 */
errval_t smlt_node_exec_start(struct smlt_node *node)
{
    smlt_node_self = node;

    smlt_node_lowlevel_init(node->id);

#if !defined(USE_THREADS)
    assert (!"NYI: do initialization in EACH process");
    __sync_init(); // XXX untested
#endif

    // Busy wait for master share to be mounted
    //while (!smlt_shm_get_master_share()) ;

    // Message passing part
    // --------------------------------------------------


    //char *_tmp = (char*) malloc(1000);
    //sprintf(_tmp, "smlt-%d", node->id);

    if (smlt_node_is_root()) {
        // Message passing initialization
        // --------------------------------------------------

       // tree_init(_tmp);

       //tree_reset();
       // tree_connect("DUMMY");

        // Build associated broadcast tree
        //setup_tree_from_model();
    }

    // Shared memory initialization
    // --------------------------------------------------
    //smlt_shm_switch_topo();

  //  smlt_platform_barrier_wait(&smlt_shm_get_master_share()->data.sync_barrier)

    return 0;

}

/**
 * @brief this function has to be called when the thread is executed first
 *
 * @param node  the Smelt node
 *
 * @return SMLT_SUCCESS
 */
errval_t smlt_node_exec_end(struct smlt_node *node) //int  __thread_end(void)
{
    smlt_node_self = NULL;

    smlt_platform_thread_end_hook();

    //debug_printfff(DBG__INIT, "Thread %d ending %d\n", tid, mp_get_counter("barriers"));
    return 0;
}


#if 0

/**
 * \brief Connect two nodes
 */
void mp_connect(coreid_t src, coreid_t dst)
{
    debug_printf("Opening channels manually: %d -> %d\n", src, dst);
    assert (get_binding(src, dst)==NULL);

    _setup_chanels(src, dst);

    assert (get_binding(src,dst)!=NULL);
}


bool topo_does_mp(coreid_t core)
{
    return topo_does_mp_send(core, false) || topo_does_mp_receive(core, false);
}


bool topo_does_shm_send(coreid_t core)
{
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_get(core, i)>=SHM_MASTER_START &&
            topo_get(core, i)<SHM_MASTER_MAX) {

            return true;
        }
    }

    return false;
}

bool topo_does_shm_receive(coreid_t core)
{
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_get(core, i)>=SHM_SLAVE_START &&
            topo_get(core, i)<SHM_SLAVE_MAX) {

            return true;
        }
    }

    return false;
}

/**
 * \brief Check whether a node sends messages according to the current
 * model.
 *
 * If <core> does an mp_send, there is at least one other node that
 * has <core> as a parent.
 *
 * \param include_leafs Consider communication from leaf nodes to
 * sequentializer as message passing links
 */
bool topo_does_mp_send(coreid_t core, bool include_leafs)
{
    // All last nodes will send messages
    if (topo_is_leaf_node(core) && include_leafs) {
        return true;
    }

    // Is the node a parent of any other node?
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_is_child(core, i)) {
            return true;
        }
    }

    return false;
}

/**
 * \brief Check whether a node receives messages according to the
 * current model.
 *
 * \param include_leafs Consider communication from leaf nodes to
 * sequentializer as message passing links
 */
bool topo_does_mp_receive(coreid_t core, bool include_leafs)
{
    // The sequentializer has to receive messages
    if (core == get_sequentializer() && include_leafs) {
        return true;
    }

    // Is the node receiving from any other node
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_is_child(i, core)) {

            return true;
        }
    }

    return false;
}
#endif
