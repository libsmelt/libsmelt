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

int init_master_share(void);
int map_master_share(void);

union quorum_share* get_master_share(void);

#endif /* SYNC_SHM_H */
