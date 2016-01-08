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


void shm_send(struct libsync_context* context,
              uintptr_t p1,
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

void shm_receive(struct libsync_context* context,
              uintptr_t *p1,
              uintptr_t *p2,
              uintptr_t *p3,
              uintptr_t *p4,
              uintptr_t *p5,
              uintptr_t *p6,
              uintptr_t *p7);

#endif /* SYNC_SHM_H */
