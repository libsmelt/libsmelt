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

// Generic
uintptr_t mp_receive(coreid_t);
void mp_send(coreid_t, uintptr_t);

// Atomic broadcast
uintptr_t mp_send_ab(uintptr_t);
uintptr_t mp_receive_forward(uintptr_t);

// Reduce
uintptr_t mp_reduce(uintptr_t);
void mp_barrier(void);

// To be implemented by the backend
uintptr_t mp_receive_raw(mp_binding*);
void mp_send_raw(mp_binding*, uintptr_t);

// Helpers to find bindings
mp_binding *mp_get_parent(coreid_t, int*);
mp_binding **mp_get_children(coreid_t, int*, int**);

int mp_get_counter(const char*);

#endif
