/*
 * Copyright (c) 2015 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SYNC_DEBUG_H
#define SYNC_DEBUG_H 1
// --------------------------------------------------
// DEBUG

/**
 * \brief Selective debug output according to subsystem.
 */
#define DBG__NONE          (0)
#define DBG__GENERAL       (1<<0)
#define DBG__HYBRID_AC     (1<<1)
#define DBG__QRM_BARRIER   (1<<2)
#define DBG__SHM           (1<<3)
#define DBG__AB            (1<<4)
#define DBG__SWITCH_TOPO   (1<<5)
#define DBG__REDUCE        (1<<6)
#define DBG__INIT          (1<<7)
#define DBG__BINDING       (1<<8)
#define DBG__UMP           (1<<9)
#define DBG__FFQ          (1<<10)

#define DBG__BARRIER      (DBG__AB | DBG__REDUCE)

// Mask for selectively enabling debug output
#define qrm_debug_mask (DBG__GENERAL | DBG__INIT)
//#define QRM_DBG_ENABLED

// Debug UMP interconnect
//#define UMP_DBG_COUNT 1

#ifdef QRM_DBG_ENABLED  /* ------------------------------*/
#define QDBG(...)              qrm_debug(DBG__GENERAL, __VA_ARGS__)
#define debug_printff(...)     qrm_debug(DBG__GENERAL, __VA_ARGS__)
#define debug_printfff(x, ...) qrm_debug(x,            __VA_ARGS__)
#define dbg_assert(x) assert(x)
#else /* QRM_DBG_ENABLED --------------------------------*/
#define QDBG(...) void()
#define debug_printff(...) void()
#define debug_printfff(...) void()
#define dbg_assert(...) void()
#endif /* QRM_DBG_ENABLED -------------------------------*/

#define qrm_debug(_subs, _fmt, ...) \
    do {                                                                \
        if (((_subs) & qrm_debug_mask) )                                \
            debug_printf(_fmt, ## __VA_ARGS__);                         \
    } while(0)


void printw(const char *fmt, ...);

#endif /* SYNC_DEBUG_H */