/*
 * Copyright (c) 2015 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SYNC_SHM_H
#define SYNC_SHM_H 1

#include <inttypes.h>

int init_master_share(void);
int map_master_share(void);

union quorum_share* get_master_share(void);

/*
 * Definitions for shared memory queue
 */

#define SHMQ_SIZE 4096
#define CACHELINE_SIZE 64

union __attribute__((aligned(64))) pos_pointer{
    uint64_t pos;
    uint8_t padding[CACHELINE_SIZE];
};

struct shm_queue{	
    // The shared memory itself
    uint8_t* shm;
    uint8_t* data;
    // Positions of readers/writer within shared queue
    union pos_pointer* write_pos;
    union pos_pointer* readers_pos;
    // Number of readers
    uint8_t num_readers;
    // Reader id, only unique within cluster
    uint8_t id;
    // Number of slots (depends on num_readers)
    uint16_t num_slots;
    /* local position pointer, global will be updated
     * when the end of the queue is reached
     */
    uint16_t l_pos;

};

/*
 * \brief Initializ a shared memory context
 *
 * The memory must already be shared between the 
 * cluster cores
 * 
 * \param shm           the shared memory itself
 * \param num_readers   the number of readers
 * \param id            the readers id, unique within cluster  
 *
 * \return the initialized per thread struct
 */

struct shm_queue* shm_init_context(void* shm,
                                   uint8_t num_readers,
                                   uint8_t id);


void shm_send(struct shm_queue* context,
              uintptr_t p1,
              uintptr_t p2,
              uintptr_t p3,
              uintptr_t p4,
              uintptr_t p5,
              uintptr_t p6,
              uintptr_t p7);

/*
 * Receiver
 *
 * void bla(uintptr_t d1, ... ) {
 * uintptr_t d1, d2, d3 .. 
 * uintptr_t d[7];
 * shm_receive(..., &d1, &d2, .. )
 * sth(d);
 * }
 * 
 */

void shm_receive(struct shm_queue* context,
              uintptr_t *p1,
              uintptr_t *p2,
              uintptr_t *p3,
              uintptr_t *p4,
              uintptr_t *p5,
              uintptr_t *p6,
              uintptr_t *p7);
#endif /* SYNC_SHM_H */
