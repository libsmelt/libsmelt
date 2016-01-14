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
#include "sync.h"

int init_master_share(void);
int map_master_share(void);

union quorum_share* get_master_share(void);

/*
 * Definitions for shared memory queue
 */

#define SHMQ_SIZE 64 // Number of slots in shared memory queue
#define CACHELINE_SIZE 64

union __attribute__((aligned(64))) pos_pointer{
    uint64_t pos;
    uint8_t padding[CACHELINE_SIZE];
};

struct shm_queue {
    // The shared memory itself
    uint8_t* shm;
    uint8_t* data;
    // Positions of readers/writer within shared queue
    volatile union pos_pointer* write_pos;
    volatile union pos_pointer* readers_pos;
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
};

void shm_init(void);

struct shm_queue* shm_init_context(void* shm,
                                   uint8_t num_readers,
                                   uint8_t id);

void shm_switch_topo(void);


void shm_send_raw(struct shm_queue* context,
                  uintptr_t p1,
                  uintptr_t p2,
                  uintptr_t p3,
                  uintptr_t p4,
                  uintptr_t p5,
                  uintptr_t p6,
                  uintptr_t p7);

void shm_send(uintptr_t p1,
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

void shm_receive_raw(struct shm_queue* context,
              uintptr_t *p1,
              uintptr_t *p2,
              uintptr_t *p3,
              uintptr_t *p4,
              uintptr_t *p5,
              uintptr_t *p6,
              uintptr_t *p7);

void shm_receive(uintptr_t *p1,
                 uintptr_t *p2,
                 uintptr_t *p3,
                 uintptr_t *p4,
                 uintptr_t *p5,
                 uintptr_t *p6,
                 uintptr_t *p7);

int shm_does_shm(coreid_t core);
int shm_is_cluster_coordinator(coreid_t core);
void shm_get_clusters_for_core (int core, int *num_clusters,
                                int **model_ids, int **cluster_ids);
void shm_get_clusters (int *num_clusters, int **model_ids, int **cluster_ids);
coreid_t shm_get_coordinator_for_cluster(int cluster);
coreid_t shm_get_coordinator_for_cluster_in_model(int cluster, int model);

int shm_cluster_get_unique_reader_id(unsigned cid,
                                     coreid_t reader);


#endif /* SYNC_SHM_H */
