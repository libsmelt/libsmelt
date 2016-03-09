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
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <numa.h>



/*
 * ===========================================================================
 * Memory allocation abstraction
 * ===========================================================================
 */


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
void *smlt_platform_alloc(uintptr_t bytes, uintptr_t align, bool do_clear)
{
    SMLT_WARNING("smlt_platform_alloc() not fully implemented! (alignment)\n");

    if (align < sizeof(void *)) {
        align = sizeof(void *);
    }

    void *buf;
    int ret = posix_memalign(&buf, align, bytes);
    if (ret) {
        if (ret == EINVAL) {
            SMLT_WARNING("smlt_platform_alloc() alignment not a power of two\n");
        }
        return NULL;
    }

    if (do_clear) {
        memset(buf, 0, bytes);
    }

    return buf;
}

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
                                  bool do_clear)
{
    SMLT_WARNING("smlt_platform_alloc_on_node() not fully implemented! (NUMA node)\n");
    return smlt_platform_alloc(bytes, align, do_clear);
}

/**
 * @brief frees the buffer
 *
 * @param buf   the buffer to be freed
 */
void smlt_platform_free(void *buf)
{
    free(buf);
}
