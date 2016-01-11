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


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include "shm.h"


#define DEBUG_SHM

struct shm_queue* shm_init_context(void* shm,
                                   uint8_t num_readers,
                                   uint8_t id)
{

    struct shm_queue* queue = (struct shm_queue*) malloc(sizeof(struct shm_queue));
    queue->shm = (uint8_t*) shm;
    queue->num_readers = num_readers;
    queue->id = id;

    queue->num_slots = ((SHMQ_SIZE-((num_readers+2)*
                           sizeof(union pos_pointer)))/CACHELINE_SIZE) -1;
    queue->write_pos = (union pos_pointer*) shm;
    queue->readers_pos = (union pos_pointer*) shm+1;
    queue->l_pos = 0;
    queue->data = (uint8_t*) shm+((num_readers+1)*sizeof(union pos_pointer));   

    return queue;
}

static bool check_readers_end(struct shm_queue* queue)
{
    for (int i = 0; i < queue->num_readers; i++) {
        if (queue->readers_pos[i].pos == 0) {

        } else {
            return false;
        }
    }	
    return true;    
}

void shm_send(struct shm_queue* context,
              uintptr_t p1,
              uintptr_t p2,
              uintptr_t p3,
              uintptr_t p4,
              uintptr_t p5,
              uintptr_t p6,
              uintptr_t p7)
{
    // if we reached the end sync with readers    
    if ((context->write_pos[0].pos) == 0) {
        while(!check_readers_end(context)){};
#ifdef DEBUG_SHM
        printf("#################################################### \n");
        printf("Synced \n");
        printf("#################################################### \n");
#endif
        for (int i = 0; i < context->num_readers; i++) {
            context->readers_pos[i].pos = 9;
        }
    }

    uintptr_t* slot_start = (uintptr_t*) context->data + 
                            (context->write_pos[0].pos*CACHELINE_SIZE);

    slot_start[0] = p1;
    slot_start[1] = p2;
    slot_start[2] = p3;
    slot_start[3] = p4;
    slot_start[4] = p5;
    slot_start[5] = p6;
    slot_start[6] = p7;

#ifdef DEBUG_SHM
    printf("Shm writer %d: write pos %" PRIu64 " addr %p \n", 
            sched_getcpu(), context->write_pos[0].pos,
            slot_start);
#endif

    // increse write pointer..
    uint64_t pos = context->write_pos[0].pos+1;
    pos = pos % context->num_slots;
    context->write_pos[0].pos = pos;
}

// returns NULL if reader reached writers pos
static bool shm_receive_non_blocking(struct shm_queue* context,
              uintptr_t *p1,
              uintptr_t *p2,
              uintptr_t *p3,
              uintptr_t *p4,
              uintptr_t *p5,
              uintptr_t *p6,
              uintptr_t *p7)
{

    if (context->l_pos == context->write_pos[0].pos) {
        return false;
    } else {

        uintptr_t* start;
        start = (uintptr_t*) context->data + ((context->l_pos)*CACHELINE_SIZE);
        *p1 = start[0];
        *p2 = start[1];
        *p3 = start[2];
        *p4 = start[3];
        *p5 = start[4];
        *p6 = start[5];
        *p7 = start[6];
#ifdef DEBUG_SHM
        printf("Shm %d: read pos %" PRIu16 " val1 %lu slot_start %p \n", 
               sched_getcpu(), context->l_pos, 
                *((uintptr_t *) start) , start);
#endif
        context->l_pos = (context->l_pos+1) % context->num_slots;
        if (context->l_pos == 0) {
            context->readers_pos[context->id].pos = 0;
#ifdef DEBUG_SHM
            printf("Reader %d: reset \n", context->id);
#endif
        }
        return true;
    }
}


// blocks
// TODO make this smarter?
void shm_receive(struct shm_queue* context,
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

