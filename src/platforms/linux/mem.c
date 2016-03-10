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
    if (align < sizeof(void *)) {
        align = sizeof(void *);
    }

    if (!SMLT_MEM_IS_POWER_OF_TWO(align)) {
        SMLT_ERROR("Alignment is not a power of two.\n");
        return NULL;
    }

#ifdef SMLT_CONFIG_LINUX_NUMA_ALIGN
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
#else
    void *buf = numa_alloc_local(bytes + align + 2* sizeof(void *));
    if (!buf) {
        return NULL;
    }

    void *buf_aligned = SMLT_MEM_ALIGN((char *)buf + 2 * sizeof(uintptr_t), align);

    uintptr_t *ret_buf = buf_aligned;

    *(ret_buf - 1) = (uintptr_t)buf;
    *(ret_buf - 2) = (uintptr_t)(bytes + align + 2* sizeof(void *));

    if (do_clear) {
        memset(ret_buf, 0, bytes);
    }

    return ret_buf;
#endif
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
    if (align < sizeof(void *)) {
        align = sizeof(void *);
    }

    if (node > numa_max_node()) {
        return NULL;
    }

    if (!SMLT_MEM_IS_POWER_OF_TWO(align)) {
        SMLT_ERROR("Alignment is not a power of two.\n");
        return NULL;
    }

#ifdef SMLT_CONFIG_LINUX_NUMA_ALIGN
    struct bitmask *current_mask = numa_get_membind();
    struct bitmask *new_nodes = numa_allocate_nodemask();

    numa_bitmask_setbit(new_nodes, node);

    numa_set_membind(new_nodes);
    numa_set_bind_policy(1);

    void *buf = smlt_platform_alloc(bytes, align, do_clear);

    numa_set_membind(current_mask);
    numa_set_bind_policy(0);
    numa_free_nodemask(new_nodes);
    // allso free current mask ?
    return buf;
#else
    void *buf = numa_alloc_onnode(bytes + align + 2* sizeof(void *), node);
    if (!buf) {
        return NULL;
    }

    void *buf_aligned =  SMLT_MEM_ALIGN((char *)buf + 2 * sizeof(uintptr_t), align);

    uintptr_t *ret_buf = buf_aligned;

    *(ret_buf - 1) = (uintptr_t)buf;
    *(ret_buf - 2) = (uintptr_t)(bytes + align + 2* sizeof(void *));

    if (do_clear) {
        memset(buf_aligned, 0, bytes);
    }
    return buf_aligned;
#endif
}

/**
 * @brief frees the buffer
 *
 * @param buf   the buffer to be freed
 */
void smlt_platform_free(void *buf)
{
#ifdef SMLT_CONFIG_LINUX_NUMA_ALIGN
    free(buf);
#else
    uintptr_t *hdr = buf;
    numa_free((void *)hdr[-1], hdr[-2]);
#endif
}
