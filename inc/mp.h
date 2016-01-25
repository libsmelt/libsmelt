/*
 * Copyright (c) 2015 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef SYNC_MP_H
#define SYNC_MP_H

#include "sync.h"

// Connection setup
void mp_connect(coreid_t src, coreid_t dst);
bool mp_can_dequeue(coreid_t core);

// Generic
uintptr_t mp_receive(coreid_t);
void mp_receive7(coreid_t, uintptr_t* buf);
bool mp_can_receive(coreid_t);
void mp_send(coreid_t, uintptr_t);
void mp_send7(coreid_t,
              uintptr_t,
              uintptr_t,
              uintptr_t,
              uintptr_t,
              uintptr_t,
              uintptr_t,
              uintptr_t);

// Atomic broadcast
void mp_send_ab0(void);
uintptr_t mp_send_ab(uintptr_t);
uintptr_t mp_send_ab7(uintptr_t,
                      uintptr_t,
                      uintptr_t,
                      uintptr_t,
                      uintptr_t,
                      uintptr_t,
		      uintptr_t);
uintptr_t mp_receive_forward(uintptr_t);
void mp_receive_forward7(uintptr_t*);
void mp_receive_forward0(void);
// Reduce
void mp_reduce0(void);
uintptr_t mp_reduce(uintptr_t);
void mp_reduce7(uintptr_t*,
                uintptr_t,
                uintptr_t,
                uintptr_t,
                uintptr_t,
                uintptr_t,
                uintptr_t,
                uintptr_t);
void mp_barrier(cycles_t*);
void mp_barrier0(void);

#ifdef FFQ
static inline void mp_send_raw(mp_binding *b, uintptr_t val)
{
    ffq_enqueue(&b->src, val);
}

static inline uintptr_t mp_receive_raw(mp_binding *b)
{
    uintptr_t r;
    ffq_dequeue(&b->dst, &r);

    return r;
}

#else

// To be implemented by the backend
uintptr_t mp_receive_raw(mp_binding*);
void mp_send_raw(mp_binding*, uintptr_t);

#endif

static inline void mp_receive_raw0(mp_binding *b)
{
#ifdef FFQ
    mp_receive_raw(b);
#else
    struct ump_pair_state *ups = (struct ump_pair_state*) b;
    struct ump_queue *q = &ups->dst.queue;

    ump_dequeue_zero(q);
#endif
}

static inline void mp_send_raw0(mp_binding *b)
{
#ifdef FFQ
    mp_send_raw(b, 0);
#else
    struct ump_pair_state *ups = (struct ump_pair_state*) b;
    struct ump_queue *q = &ups->src.queue;

    ump_enqueue_zero(q);
#endif
}

void mp_receive_raw7(mp_binding*,
                     uintptr_t* buf);
void mp_send_raw7(mp_binding*,
                 uintptr_t,
                 uintptr_t,
                 uintptr_t,
                 uintptr_t,
                 uintptr_t,
                 uintptr_t,
                 uintptr_t);

bool mp_can_receive_raw(mp_binding*);


// Helpers to find bindings
mp_binding *mp_get_parent(coreid_t, int*);
mp_binding **mp_get_children(coreid_t, int*, int**);

int mp_get_counter(const char*);

#endif
