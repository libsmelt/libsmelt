/**
 * \file
 * \brief Implementation of shared memory qeueue writer
 */

/*
 * Copyright (c) 2015, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, CAB F.78, Universitaetstr. 6, CH-8092 Zurich,
 * Attn: Systems Group.
 */

//#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <stdbool.h>
#include <assert.h>
#include <numa.h>

#include "shm_qp.h"

#define SHM_SIZE 4096
#define SHMQ_SIZE 64
//#define DEBUG_SHM

struct shm_context* shm_init_context(void* shm,
                                     uint8_t node)
{
    struct shm_context* q;
    q = (struct shm_context*) numa_alloc_onnode(sizeof(struct shm_context),
                                                node);
    assert (q!=NULL);
    
    q->shm = (uint8_t*) shm;
    q->data = q->shm + (sizeof(union pos_pointer));
    q->num_slots = (SHMQ_SIZE)-1; 
    q->reader_pos = (union pos_pointer*) q->shm;
    q->l_pos = 0;
    q->next_sync = q->num_slots-1;
    q->next_seq = 1;

    return q;
}

struct shm_qp* shm_queuepair_create(uint32_t src,
                                    uint32_t dst)
{
    struct shm_qp* qp = (struct shm_qp*) malloc(sizeof(struct shm_qp));
    void* shm = numa_alloc_onnode(sizeof(SHM_SIZE),
                                  numa_node_of_cpu(dst));
    qp->src = *shm_init_context(shm,
                               numa_node_of_cpu(src));
    qp->dst = *shm_init_context(shm,
                               numa_node_of_cpu(dst));
    return qp;
}

void get_next_sync(struct shm_context* q, 
                   uint64_t* next)
{
    *next = q->reader_pos->pos+(q->num_slots-1);
}

void shm_q_send(struct shm_context* q,
                  uintptr_t p1,
                  uintptr_t p2,
                  uintptr_t p3,
                  uintptr_t p4,
                  uintptr_t p5,
                  uintptr_t p6,
                  uintptr_t p7)
{

    uint64_t next_sync;
    // if we reached the end sync with readers
    if ((q->l_pos) == q->num_slots) {
        q->l_pos = 0;
    }

    // get next sync at the sync point
    if (q->next_seq == q->next_sync) {
       get_next_sync(q, &next_sync);
       while(q->next_seq == next_sync) {
            get_next_sync(q, &next_sync);
       }
       q->next_sync = next_sync;
    }

    uintptr_t offset =  (q->l_pos*(CACHELINE_SIZE/sizeof(uintptr_t)));
    //assert (offset<SHMQ_SIZE*CACHELINE_SIZE);

    uintptr_t* slot_start = (uintptr_t*) q->data + offset;

    slot_start[1] = p1; 
    slot_start[2] = p2; 
    slot_start[3] = p3;
    slot_start[4] = p4; 
    slot_start[5] = p5;
    slot_start[6] = p6; 
    slot_start[7] = p7; 
#ifdef DEBUG_SHM
       /* printf("Shm writer %d epoch %d: write pos %" PRIu64 " addr %p value1 %lu \n",
            sched_getcpu(), context->epoch, context->write_pos[0].pos[context->epoch],
            slot_start, slot_start[0]); */
        printf("Shm writer %d: write pos %d val %lu \n", sched_getcpu(),
                q->l_pos, 
                q->next_seq);
#endif
    slot_start[0] = q->next_seq;
    q->next_seq++;

    // increse write pointer..
    q->l_pos = q->l_pos+1;
}

void shm_q_send0(struct shm_context* q)
{

    uint64_t next_sync;
    // if we reached the end sync with readers
    if ((q->l_pos) == q->num_slots) {
        q->l_pos = 0;
    }

    // get next sync at the sync point
    if (q->next_seq == q->next_sync) {
       get_next_sync(q, &next_sync);
       while(q->next_seq == next_sync) {
            get_next_sync(q, &next_sync);
       }
       q->next_sync = next_sync;
    }

    uintptr_t offset =  (q->l_pos*(CACHELINE_SIZE/sizeof(uintptr_t)));
    //assert (offset<SHMQ_SIZE*CACHELINE_SIZE);

    uintptr_t* slot_start = (uintptr_t*) q->data + offset;

    slot_start[0] = q->next_seq;
    q->next_seq++;

    // increse write pointer..
    q->l_pos = q->l_pos+1;
}

bool shm_q_can_recv(struct shm_context* context)
{
    uintptr_t* start;
    start = (uintptr_t*) context->data + ((context->l_pos)*
                 CACHELINE_SIZE/(sizeof(uintptr_t)));

    if (context->next_seq == start[0]) {
        return true;
    } else {
        return false;
    }
}

bool shm_q_can_send(struct shm_context* context)
{
    if (context->next_seq == context->next_sync) {
        return false;
    } else {
        return true;
    }
}

// returns NULL if reader reached writers pos
bool shm_receive_non_blocking(struct shm_context* context,
              uintptr_t *p1,
              uintptr_t *p2,
              uintptr_t *p3,
              uintptr_t *p4,
              uintptr_t *p5,
              uintptr_t *p6,
              uintptr_t *p7)
{
    //assert (context!=NULL);
    uintptr_t* start;
    start = (uintptr_t*) context->data + ((context->l_pos)*
                 CACHELINE_SIZE/(sizeof(uintptr_t)));

    if (context->next_seq == start[0]) {
        *p1 = start[1];
        *p2 = start[2];
        *p3 = start[3]; 
        *p4 = start[4]; 
        *p5 = start[5]; 
        *p6 = start[6]; 
        *p7 = start[7]; 

#ifdef DEBUG_SHM
        printf("Shm reader %d : read pos %" PRIu16 " val1 %lu \n", 
                sched_getcpu(), context->l_pos,
                *((uintptr_t *) start)); 
    
#endif
        context->l_pos = (context->l_pos+1);
        context->next_seq++;
        if (context->l_pos == context->num_slots) {
            context->l_pos = 0;
        }

        context->reader_pos->pos = (context->next_seq -1);
        return true;
    }
    return false;
}

// returns NULL if reader reached writers pos
bool shm_receive_non_blocking0(struct shm_context* context)
{
    //assert (context!=NULL);
    uintptr_t* start;
    start = (uintptr_t*) context->data + ((context->l_pos)*
                 CACHELINE_SIZE/(sizeof(uintptr_t)));

    if (context->next_seq == start[0]) {
        context->l_pos = (context->l_pos+1);
        context->next_seq++;
        if (context->l_pos == context->num_slots) {
            context->l_pos = 0;
        }

        context->reader_pos->pos = (context->next_seq -1);
        return true;
    }
    return false;
}

// blocks
// TODO make this smarter?
void shm_q_recv(struct shm_context* context,
                     uintptr_t *p1,
                     uintptr_t *p2,
                     uintptr_t *p3,
                     uintptr_t *p4,
                     uintptr_t *p5,
                     uintptr_t *p6,
                     uintptr_t *p7)
{
    while(!shm_receive_non_blocking(context, p1, p2, p3,
                                 p4, p5, p6, p7)){};

}

void shm_q_recv0(struct shm_context* context)
{
    while(!shm_receive_non_blocking0(context)){};

}

#undef SHM_SIZE
#undef SHMQ_SIZE
