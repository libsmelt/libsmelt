/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_PLATFORM_H_
#define SMLT_PLATFORM_H_ 1


/*
 * ===========================================================================
 * Operating System
 * ===========================================================================
 */
#if defined(BARRELFISH)
#include <platforms/barrelfish.h>
#elif defined(__linux__) || defined(__LINUX__)
#include <platforms/linux.h>
#else
#error "unsupported OS"
#endif


/*
 * ===========================================================================
 * Architecture
 * ===========================================================================
 */
#if defined(__x86_64__)
#include <arch/x86_64.h>
#elif defined(__arm__)
#include <arch/arm.h>
#else
#error "unsupported architecture"
#endif


/**
 * @brief initializes the platform specific backend
 *
 * @param num_proc  the requested number of processors
 *
 * @returns the number of available processors
 */
uint32_t smlt_platform_init(uint32_t num_proc);


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

 * @returns SMELT_SUCCESS or error value
 */
errval_t smlt_platform_barrier_init(smlt_platform_barrier_t *bar,
                                    smlt_platform_barrierattr_t *attr,
                                    uint32_t count);

/**
 * @brief destroys a created barrier
 *
 * @param bar     pointer to the barrier 

 * @returns SMELT_SUCCESS or error value
 */
errval_t smlt_platform_barrier_destroy(smlt_platform_barrier_t *barrier);

/**
 * @brief waits on the barrier untill all threads have entered the barrier
 *
 * @param bar     pointer to the barrier 

 * @returns SMELT_SUCCESS or error value
 */
errval_t smlt_platform_barrier_wait(smlt_platform_barrier_t *barrier);


errval_t smlt_platform_node_start(void);  //int  __backend_thread_start(void);
errval_t smlt_platform_node_end();          ///int __backend_thread_end(void);



#endif /* SMLT_PLATFORM_H_ */
