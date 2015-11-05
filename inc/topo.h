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

// Include the pre-generated model
//#include "sync/model_defs.h" // << perhaps better not globally included?

void switch_topo(void);
int topo_get(int,int);
int topos_get(int,int,int);
int topo_has_edge(coreid_t);
int topo_is_edge(coreid_t, coreid_t);
int topo_is_parent(coreid_t, coreid_t);
int topo_is_parent_real(coreid_t, coreid_t);
unsigned int topo_num_cores(void);

#endif /* TOPO_H */
