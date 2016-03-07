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


/*
 * ===========================================================================
 * Barrier implementation for initialization purposes
 * ===========================================================================
 */

/**
 * @brief initializes a platform independent barrier for init synchro
 *
 * @param bar     pointer to the barrier memory area
 * @param attr    attributes for the barrier if any
 * @param count   number of threads
 *
 * @returns SMELT_SUCCESS or error value
 */
errval_t smlt_platform_barrier_init(smlt_platform_barrier_t *bar,
                                    smlt_platform_barrierattr_t *attr,
                                    uint32_t count)
{
    return pthread_barrier_init(bar, attr, count);
}

/**
 * @brief destroys a created barrier
 *
 * @param bar     pointer to the barrier

 * @returns SMELT_SUCCESS or error value
 */
errval_t smlt_platform_barrier_destroy(smlt_platform_barrier_t *barrier)
{
    return pthread_barrier_destroy(barrier);
}

/**
 * @brief waits on the barrier untill all threads have entered the barrier
 *
 * @param bar     pointer to the barrier
 *
 * @returns SMELT_SUCCESS or error value
 */
errval_t smlt_platform_barrier_wait(smlt_platform_barrier_t *barrier)
{
    return pthread_barrier_wait(barrier);
}
