/*
 * Copyright (c) 2015 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SHM_QP_H
#define SHM_QP_H 1

#include <inttypes.h>

/*
 * Definitions for shared memory queue
 */
#define DEFAULT_SHM_Q_SIZE 64 // Number of slots in shared memory queue
#define CACHELINE_SIZE 64
#define DEFAULT_SHM_SIZE DEFAULT_SHM_Q_SIZE*CACHELINE_SIZE

union __attribute__((aligned(64))) pos_pointer{
    uint64_t pos;
    uint8_t padding[CACHELINE_SIZE];
};


/**
 * \brief Per-thread datastructure representing a SHM queue.
 */
struct shm_context {
    // Number of slots (depends on num_readers)
    uint64_t num_slots;
    /* local position pointer, global will be updated
     * when the end of the queue is reached
     */
    uint16_t l_pos;

    // shm impl
    uint64_t next_sync;
    uint64_t next_seq;

    // The shared memory itself
    uint8_t* shm;
    uint8_t* data;

    // Positions of readers/writer within shared queue
    volatile union pos_pointer* reader_pos;
};

struct shm_qp {
    struct shm_context* src;
    struct shm_context* dst;
};

struct shm_qp* shm_queuepair_create(uint32_t src,
                                    uint32_t dst);

void shm_q_send(struct shm_context* context,
                uintptr_t p1,
                uintptr_t p2,
                uintptr_t p3,
                uintptr_t p4,
                uintptr_t p5,
                uintptr_t p6,
                uintptr_t p7);
void shm_q_send0(struct shm_context* context);

void shm_q_recv(struct shm_context* context,
                   uintptr_t *p1,
                   uintptr_t *p2,
                   uintptr_t *p3,
                   uintptr_t *p4,
                   uintptr_t *p5,
                   uintptr_t *p6,
                   uintptr_t *p7);
void shm_q_recv0(struct shm_context* context);

bool shm_q_can_send(struct shm_context* context);
bool shm_q_can_recv(struct shm_context* context);

#endif /* SYNC_SHM_H */
