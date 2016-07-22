/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_DEBUG_H_
#define SMLT_DEBUG_H_ 1

#include "smlt_platform.h"

#ifdef SYNC_DEBUG_BUILD
// Enable debug output for buildtype "debug"
#define SMLT_DEBUG_ENABLED 1
#endif

#define SMLT_DBG_ERR       (1 << 31)
#define SMLT_DBG_WARN      (1 << 30)
#define SMLT_DBG_NOTICE    (1 << 29)

/**
 * \brief Selective debug output according to subsystem.
 */
#define SMLT_DBG__NONE          (0)
#define SMLT_DBG__GENERAL       (1<<0)
#define SMLT_DBG__HYBRID_AC     (1<<1)
#define SMLT_DBG__QRM_BARRIER   (1<<2)
#define SMLT_DBG__SHM           (1<<3)
#define SMLT_DBG__AB            (1<<4)
#define SMLT_DBG__SWITCH_TOPO   (1<<5)
#define SMLT_DBG__REDUCE        (1<<6)
#define SMLT_DBG__INIT          (1<<7)
#define SMLT_DBG__BINDING       (1<<8)
#define SMLT_DBG__UMP           (1<<9)
#define SMLT_DBG__FFQ          (1<<10)
#define SMLT_DBG__PLATFORM     (1<<11)
#define SMLT_DBG__NODE          (1<<12)
#define SMLT_DBG__ALL          ((1 << 13) -1)
#define SMLT_DBG__BARRIER      (DBG__AB | DBG__REDUCE)

// Mask for selectively enabling debug output
#define smlt_debug_mask (SMLT_DBG__ALL)

// Debug UMP interconnect
//#define UMP_DBG_COUNT 1


#ifdef SMLT_DEBUG_ENABLED
#define SMLT_DEBUG(_subs, ... )                                    \
    do {                                                                \
        if (((_subs) & smlt_debug_mask))                                \
            smlt_debug_print(_subs,  __VA_ARGS__);              \
    } while(0);
#else
#define SMLT_DEBUG(_subs, ...) 
#endif

#define SMLT_ERROR(...) smlt_debug_print(SMLT_DBG_ERR,  __VA_ARGS__);
#define SMLT_WARNING(...) smlt_debug_print(SMLT_DBG_WARN,  __VA_ARGS__);
#define SMLT_NOTICE(...) smlt_debug_print(SMLT_DBG_NOTICE,  __VA_ARGS__);
#define SMLT_ABORT(...)             \
    do {                            \
        SMLT_ERROR(__VA_ARGS__);    \
        exit(1);                    \
    } while(0);                     \

#define dbg_printf(...) SMLT_DEBUG(SMLT_DBG__GENERAL, __VA_ARGS__);
#define COND_PANIC(cond, msg) \
    if (!(cond)) {            \
        panic(msg);           \
    }

/*
 * ===========================================================================
 * print functionality
 * ===========================================================================
 */

/**
 * @brief prints an message to stdout
 *
 * @param fmt   format string to print
 */
void smlt_debug_print(uint32_t subs, const char *fmt, ...);

/**
 * @brief Print message and abort
 */
void panic(const char *str);

#endif /* SMLT_DEBUG_H_ */
