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
#include "../../internal.h"

#include <sched.h>
#include <stdarg.h>



/*
 * ===========================================================================
 * Thread Control
 * ===========================================================================
 */

/**
 * @brief pins the calling thread to the given core
 *
 * @param core  ID of the core to pint the thread to
 *
 * @return SMLT_SUCCESS or error value
 */
errval_t smlt_platform_pin_thread(coreid_t core)
{
    SMLT_DEBUG(SMLT_DBG__PLATFORM, "platform: pinning thread to core %" PRIu32 " \n",
               core);

    cpu_set_t cpu_mask;
    int err;

    CPU_ZERO(&cpu_mask);
    CPU_SET(core, &cpu_mask);

    err = sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);
    if (err) {
        SMLT_ERROR("sched_setaffinity");
        exit(1);
    }

    return SMLT_SUCCESS;
}

/**
 * @brief executed when the Smelt thread is initialized
 *
 * @return SMLT_SUCCESS on ok
 */
errval_t smlt_platform_thread_start_hook(void)
{
#ifdef UMP_DBG_COUNT
#ifdef FFQ
#else
    ump_start();
#endif
#endif
    return SMLT_SUCCESS;
}

/**
 * @brief executed when the Smelt thread terminates
 *
 * @return SMLT_SUCCESS on ok
 */
errval_t smlt_platform_thread_end_hook(void)
{
#ifdef UMP_DBG_COUNT
#ifdef FFQ
#else
    ump_end();
#endif
#endif
    return SMLT_SUCCESS;
}


/*
 * ===========================================================================
 * Platform specific Smelt node management
 * ===========================================================================
 */

/**
 * @brief handles the platform specific initialization of the Smelt node
 *
 * @param node  the node to be created
 *
 * @return SMELT_SUCCESS or error value
 */
errval_t smlt_platform_node_create(struct smlt_node *node)
{
    /*
     * Create the platform specific data structures here
     * i.e. bindings / sockets / file descriptors etc.
     */
    return SMLT_SUCCESS;
}

static void *smlt_platform_node_start_wrapper(void *arg)
{
    struct smlt_node *node = arg;

    SMLT_DEBUG(SMLT_DBG__PLATFORM, "platform: thread started execution id=%" PRIu32 "\n",
               smlt_node_get_id_of_node(node));


    /* initializing smelt node */
    smlt_node_exec_start(node);

    void *result = smlt_node_run(node);

    /* ending Smelt node */
    smlt_node_exec_end(node);

    SMLT_DEBUG(SMLT_DBG__PLATFORM, "thread ended execution\n");
    return result;
}

/**
 * @brief Starts the platform specific execution of the Smelt node
 *
 * @param node  the Smelt node
 *
 * @return SMELT_SUCCESS or error value
 */
errval_t smlt_platform_node_start(struct smlt_node *node)
{
    SMLT_DEBUG(SMLT_DBG__PLATFORM, "platform: starting node with id=%" PRIu32 "\n",
               smlt_node_get_id_of_node(node));

    int err = pthread_create(&node->handle, NULL, smlt_platform_node_start_wrapper,
                             node);
    if (err) {
        return -1;
    }

    return SMLT_SUCCESS;
}

/**
 * @brief waits for the Smelt node to be done with execution
 *
 * @param node  the Smelt node
 *
 * @return SMELT_SUCCESS or error value
 */
errval_t smlt_platform_node_join(struct smlt_node *node)
{
    SMLT_DEBUG(SMLT_DBG__PLATFORM, "platform: joining node with id=%" PRIu32 "\n",
               smlt_node_get_id_of_node(node));
    int err = pthread_join(node->handle, NULL);
    if (err) {
        return -1;
    }
    return SMLT_SUCCESS;
}

/**
 * @brief cancles the execution of the Smelt node
 *
 * @param node  The Smelt node
 *
 * @return SMELT_SUCCESS or error value
 */
errval_t smlt_platform_node_cancel(struct smlt_node *node)
{

    return SMLT_SUCCESS;
}
