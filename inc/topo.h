/*
 * Copyright (c) 2015 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef TOPO_H
#define TOPO_H 1

#include "model_defs.h"
#include <vector>

void switch_topo(void);
bool switch_topo_to_idx(int);
int topo_get(int,int);
int topos_get(int,int,int);
bool topo_is_leaf_node(coreid_t);
bool topo_is_child(coreid_t, coreid_t);
int topo_is_parent(coreid_t, coreid_t);
int topo_is_parent_real(coreid_t, coreid_t);
unsigned int topo_num_cores(void);
unsigned int topo_num_topos(void);

const char* topo_get_name(void);

bool topo_does_mp_send(coreid_t, bool);
bool topo_does_mp_receive(coreid_t, bool);
bool topo_does_shm_send(coreid_t core);
bool topo_does_shm_receive(coreid_t core);
int get_topo_idx(void);
coreid_t topo_last_node(void);
std::vector<int> **topo_all_leaf_nodes(void);
int topo_mp_cluster_size(coreid_t coordinator, int clusterid);

#endif /* TOPO_H */
