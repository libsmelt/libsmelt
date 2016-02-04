/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_SMLT_H_
#define SMLT_SMLT_H_ 1

/*
 * ===========================================================================
 * type declarations
 * ===========================================================================
 */


/**
 * represents a smelt message handed by the system
 */
struct smlt_msg
{
    uint32_t offset;    ///< offset into the data region
    uint32_t datalen;   ///< length of the data region in bytes
    uint32_t maxlen;    ///< maximum length of the data region
    uint32_t _pad;      ///< unused padding for now
    uint64_t data[];    ///< the data region
};


/*
 * ===========================================================================
 * type declarations
 * ===========================================================================
 */


errval_t smlt_init();

errval_t smlt_thread_init();

#endif /* SMLT_SMLT_H_ */