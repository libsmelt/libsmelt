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

/* forward declaration */
struct smlt_node;

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
errval_t smlt_platform_pin_thread(coreid_t core);


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
errval_t smlt_platform_node_create(struct smlt_node *node);

/**
 * @brief Starts the platform specific execution of the Smelt node
 *
 * @param node  the Smelt node
 *
 * @return SMELT_SUCCESS or error value
 */
errval_t smlt_platform_node_start(struct smlt_node *node);

/**
 * @brief waits for the Smelt node to be done with execution
 *
 * @param node  the Smelt node
 *
 * @return SMELT_SUCCESS or error value
 */
errval_t smlt_platform_node_join(struct smlt_node *node);

/**
 * @brief cancles the execution of the Smelt node
 *
 * @param node  The Smelt node
 *
 * @return SMELT_SUCCESS or error value
 */
errval_t smlt_platform_node_cancel(struct smlt_node *node);


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
