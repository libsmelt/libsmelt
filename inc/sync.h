/*
 * Copyright (c) 2015 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */


#ifndef SYNC_H
#define SYNC_H 1

#ifdef __linux__
#include "backends/linux.h"
#else
#include "backends/barrelfish.h"
#endif

#include "debug.h"
#include "backend.h"

// ==================================================
// Functions

void __sync_init(void);

int  __thread_init(int,int);
int  __thread_end(void);

unsigned int  get_thread_id(void);
int  get_core_id(void);
unsigned int  get_num_threads(void);

// ==================================================
// Defines
#define Q_MAX_CORES         64
#define QRM_ROUND_MAX        3 // >= 3 - barrier reinitialization problem
#define SHM_Q_MAX          600
#define SHM_SIZE     (16*4096) // 4 KB
#define SEQUENTIALIZER       0 // node that acts as the sequentializer

#define USE_THREADS          1 // switch threads vs. processes

// ==================================================
// Data structures

// Layout of the shared page
union quorum_share {
    // Structure
    struct {
        uint32_t __attribute__ ((aligned(32))) dummy; // < test mapping
        uint32_t __attribute__ ((aligned(32))) ctr; // cnt clients
#if defined(FEEDBACK)
        // Feedback: for signalling end of experiment
        uint32_t __attribute__ ((aligned(32))) feedback;
#endif
        uint32_t __attribute__ ((aligned(32))) cores[Q_MAX_CORES]; // core state

        // --- 16 bytes

        // Several counters on the same cache-line, but we don't care
        // as different counters are never used concurrently anyway
        spinlock_t __attribute__ ((aligned(64))) lock;

        // --  8 bytes

        uint8_t __attribute__ ((aligned(32))) rounds[QRM_ROUND_MAX];

        // -- 4 bytes * 500

        // -- SUM: 2024

        // FIFO - message channels: these are used in shared-memory
        // communication channels to transport messages. It's
        // essentially a one-writer multiple-reader queue.

        // Currently shared by all cores participating in a
        // shared-memory island.

        // There used to be four queues here, but only the first one
        // was ever used. I am going to remove the remaining one to
        // save some space.

        // one ump_message should be 64 bytes.
        struct ump_message __attribute__((aligned(64))) fifo0[SHM_Q_MAX];

        // shared memory for reduction
        uint64_t __attribute__((aligned(64))) reduction_aggregate;

        // shared memory for reduction (counter to indicate how many finished)
        uint64_t __attribute__((aligned(64))) reduction_counter;

        // shared memory for reduction (counter to indicate round)
        uint8_t __attribute__((aligned(64))) reduction_round; // XXX has to be uint8

        // Count how often this frame has been mapped. Keep this last
        // to ensure that whole frame is mapped when incrementing
        // counter after mapping.
        uint64_t __attribute__((aligned(64))) num_maps;
    } data;
};

#endif /* SYNC_H */