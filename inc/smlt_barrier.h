/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */


#ifndef SMLT_BARRIER_H
#define SMLT_BARRIER_H 1

/* forward declaration */
struct smlt_context;
struct smlt_channel;
struct smlt_dissem_barrier;

/*
 * ===========================================================================
 * Smelt barrier MACROS
 * ===========================================================================
 */


/*
 * ===========================================================================
 * Smelt barrier type declarations
 * ===========================================================================
 */

/**
 * the smelt barrier data structure
 */
struct smlt_barrier
{
    bool use_payload;

};


/*
 * ===========================================================================
 * Barrier interface
 * ===========================================================================
 */

/**
 * @brief destroys a smlt barrier
 *
 * @param ctx the Smelt context
 *
 * @returns TODO:errval
 */
errval_t smlt_barrier_init(struct smlt_context *ctx);

/**
 * @brief destroys a smlt barrier
 *
 * @param ctx the Smelt context
 *
 * @returns TODO:errval
 */
errval_t smlt_barrier_destroy(struct smlt_context *ctx);

/**
 * @brief waits on the supplied barrier
 *
 * @param ctx the Smelt context
 *
 * @returns TODO:errval
 */
errval_t smlt_barrier_wait(struct smlt_context *ctx);





/**
 * @brief initalized a dissemination barrier based on smelt 
 *        channels
 *
 * @param cores     the core_ids that participate in the barrier
 * @param num_cores the number of cores participating
 * @param bar       return struct of the barrier context
 *
 * @returns TODO:errval
 */
errval_t smlt_dissem_barrier_init(uint32_t* cores, uint32_t num_cores,
                                  struct smlt_dissem_barrier** bar);

/**
 * @brief waits on the supplied barrier
 *
 * @param bar context for the barrier
 *
 * @returns TODO:errval
 */
errval_t smlt_dissem_barrier_wait(struct smlt_dissem_barrier *bar);


struct shl_barrier_t {
    int num;
    int idx; //
};
typedef struct shl_barrier_t  shl_barrier_t;

int shl_barrier_destroy(shl_barrier_t *barrier);
int shl_barrier_init(shl_barrier_t *barrier,
                     const void *attr, unsigned count);

int shl_barrier_wait(shl_barrier_t *barrier);
int shl_hybrid_barrier(void*);
int shl_hybrid_barrier0(void*);

#endif /* SMLT_BARRIER_H */
