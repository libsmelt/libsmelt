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

#define SMLT_DEBUG_ENABLED 1

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
#define SMLT_DBG__ALL          ((1 << 11) -1)
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
#define SMLT_DEBUG(_subs, _fmt, ...) void()
#endif


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


#endif /* SMLT_DEBUG_H_ */
