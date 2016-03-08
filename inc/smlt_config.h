/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_CONFIG_H_
#define SMLT_CONFIG_H_ 1


/*
 * ===========================================================================
 * Smelt configuration
 * ===========================================================================
 */
#define Q_MAX_CORES         64
#define QRM_ROUND_MAX        3 // >= 3 - barrier reinitialization problem
#define SHM_Q_MAX          600
#define SHM_SIZE     (16*4096) // 4 KB
#define SEQUENTIALIZER       0 // node that acts as the sequentializer

#define USE_THREADS          1 // switch threads vs. processes
#define BACK_CHAN            1 // Backward channel from "last node" to 


#define SMLT_EAGER_NODE_CREATION 1

#endif /* SMLT_CONFIG_H_ */
