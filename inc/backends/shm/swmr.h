/*
 * Copyright (c) 2015 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SWMR_H
#define SWMR_H 1

#include <inttypes.h>
#include <stdbool.h>

/*
 * Definitions for shared memory queue
 */

#define SWMRQ_SIZE 64 // Number of slots in shared memory queue
#define CACHELINE_SIZE 64

union __attribute__((aligned(64))) pos_point{
    uint64_t pos;
    uint8_t padding[CACHELINE_SIZE];
};

/**
 * \brief Per-thread datastructure representing a SHM queue.
 */
struct swmr_context {

    // The shared memory itself
    uint8_t* shm;
    uint8_t* data;

    // Positions of readers/writer within shared queue
    volatile union pos_point* write_pos;
    volatile union pos_point* readers_pos;
    // Number of readers
    uint8_t num_readers;
    // Reader id, only unique within cluster
    uint8_t id;
    // Number of slots (depends on num_readers)
    uint64_t num_slots;
    /* local position pointer, global will be updated
     * when the end of the queue is reached
     */
    uint16_t l_pos;

    // avoid bug
    uint8_t epoch;
    uint64_t r_mask;
    uint64_t w_mask;

    // shm4 impl
    uint64_t next_sync;
    uint64_t next_seq;
};

struct swmr_queue {
    struct swmr_context src;
    struct swmr_context* dst;
};

void swmr_init_context(void* shm, struct swmr_context* queue,
                       uint8_t num_readers, uint8_t id);

struct swmr_queue* swmr_init_queue(uint32_t src,
                                   uint32_t* dst,
                                   uint16_t count);

void swmr_send_raw(struct swmr_context* context,
                  uintptr_t p1,
                  uintptr_t p2,
                  uintptr_t p3,
                  uintptr_t p4,
                  uintptr_t p5,
                  uintptr_t p6,
                  uintptr_t p7);

void swmr_receive_raw(struct swmr_context* context,
              uintptr_t *p1,
              uintptr_t *p2,
              uintptr_t *p3,
              uintptr_t *p4,
              uintptr_t *p5,
              uintptr_t *p6,
              uintptr_t *p7);

bool swmr_can_send(struct swmr_context* context);
bool swmr_can_receive(struct swmr_context* context);

#endif /* SYNC_SHM_H */
