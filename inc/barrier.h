/*
 * Copyright (c) 2015 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */


#ifndef BARRIER_H
#define BARRIER_H 1

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

#endif /* BARRIER_H */
