/*
 * Copyright (c) 2015 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef INTERNAL_DEBUG_H
#define INTERNAL_DEBUG_H 1

#include "sync.h"

/**
 * File: tree_setup.c
 *
 * This header should never be imported by anything outside of libsync.
 */

void tree_reset(void);
int  tree_init(const char*);
void tree_init_bindings(void);

void setup_tree_from_model(void);

/**
 * List of bindings.
 */
struct binding_lst {
    unsigned int num;
    mp_binding **b;
    mp_binding **b_reverse;
    int *idx;
};

binding_lst *_mp_get_parent_raw(coreid_t c);
binding_lst *_mp_get_children_raw(coreid_t c);

void _setup_chanels(int src, int dst);

#endif /* INTERNAL_DEBUG_H */
