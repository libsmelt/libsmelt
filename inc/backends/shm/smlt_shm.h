/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SYNC_SHM_H
#define SYNC_SHM_H 1

#include <inttypes.h>

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
        smlt_platform_lock_t __attribute__ ((aligned(64))) lock;

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

        /**
         * \brief Internal barrier used within libsync.
         */
        pthread_barrier_t sync_barrier;
    } data;
};

/*
 * ==============================================================================
 * Reductions
 * ==============================================================================
 */

/**
 * @brief does a reduction on a shared memory region
 *
 * @param input     input message for this node
 * @param output    the returned aggregated value
 *
 * @return SMLT_SUCCESS or error avalue
 */
errval_t smlt_shm_reduce(struct smlt_msg *input, struct smlt_msg *output);

/**
 * @brief does a reduction of notifications on a shared memory region
 *
 *
 * @return SMLT_SUCCESS or error avalue
 */
errval_t smlt_shm_reduce_notify(void);


/*
 * ==============================================================================
 * Management of the shared frame
 * ==============================================================================
 */

/**
 * @brief initializes the shared frame for a cluster
 *
 * @return SMLT_SUCCESS
 */
errval_t smlt_shm_init_master_share(void);

/**
 * @brief obtains a pointer to the shared frame
 *
 * @return pointer to the shared frame
 */
union quorum_share*smlt_shm_get_master_share(void);

#if 0

int init_master_share(void);
int map_master_share(void);

// Layout of the shared page



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

/**
 * \brief Per-thread datastructure representing a SHM queue.
 */
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

struct shm_reduction {

    // shared memory for reduction
    uint64_t __attribute__((aligned(64))) reduction_aggregate;

    // shared memory for reduction (counter to indicate how many finished)
    uint64_t __attribute__((aligned(64))) reduction_counter;

    // shared memory for reduction (counter to indicate round)
    uint8_t __attribute__((aligned(64))) reduction_round; // XXX has to be uint8
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


uintptr_t shm_reduce(uintptr_t);
#endif
#endif /* SYNC_SHM_H */
