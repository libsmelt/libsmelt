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
 *
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
 *
 * @returns SMELT_SUCCESS or error value
 */
errval_t smlt_platform_barrier_wait(smlt_platform_barrier_t *barrier);


/*
 * ===========================================================================
 * Mutex
 * ===========================================================================
 */

errval_t smlt_platform_lock_init(smlt_platform_lock_t *lock);
void smlt_platform_lock_acquire(smlt_platform_lock_t *lock);
void smlt_platfrom_lock_release(smlt_platform_lock_t *lock);


/*
 * ===========================================================================
 * Starting of nodes
 * ===========================================================================
 */


errval_t smlt_platform_node_start(void);  //int  __backend_thread_start(void);
errval_t smlt_platform_node_end();          ///int __backend_thread_end(void);

/*
 * ===========================================================================
 * Memory allocation abstraction
 * ===========================================================================
 */

#define SMLT_DEFAULT_ALIGNMENT 0

/**
 * @brief allocates a buffer reachable by all nodes on this machine
 *
 * @param bytes         number of bytes to allocate
 * @param align         align the buffer to a multiple bytes
 * @param do_clear      if TRUE clear the buffer (zero it)
 *
 * @returns pointer to newly allocated buffer
 *
 * When processes are used this buffer will be mapped to all processes.
 * The memory is allocated from any numa node
 */
void *smlt_platform_alloc(uintptr_t bytes, uintptr_t align, bool do_clear);

/**
 * @brief allocates a buffer reachable by all nodes on this machine on a NUMA node
 *
 * @param bytes     number of bytes to allocate
 * @param align     align the buffer to a multiple bytes
 * @param node      which numa node to allocate the buffer
 * @param do_clear  if TRUE clear the buffer (zero it)
 *
 * @returns pointer to newly allocated buffer
 *
 * When processes are used this buffer will be mapped to all processes.
 * The memory is allocated form the specified NUMA node
 */
void *smlt_platform_alloc_on_node(uint64_t bytes, uintptr_t align, uint8_t node,
                                  bool do_clear);

/**
 * @brief frees the buffer
 *
 * @param buf   the buffer to be freed
 */
void smlt_platform_free(void *buf);

#endif /* SMLT_PLATFORM_H_ */
