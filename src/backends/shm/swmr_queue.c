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
#include <stdbool.h>
#include <assert.h>

#include <shm/swmr.h>


//#define DEBUG_SHM

/**
 * \brief Initalize a shared memory context
 *
 * The memory must already be shared between the
 * cluster cores
 *
 * \param shm Pointer to memory that should be used for the
 * queue. Presumably, this has to be SHMQ_SIZE*CACHELINE bytes.
 *
 * \param num_readers The number of readers using this queue. Each
 * reader has a read pointer, which has to be allocated.
 *
 * \param id An ID representing this reader, has to unique within a
 * cluster, starting at 0, i.e. for three readers, the id's would be
 * 0, 1 and 2.
 */
struct swmr_queue* swmr_init_context(void* shm,
                                   uint8_t num_readers,
                                   uint8_t id)
{

    struct swmr_queue* queue = (struct swmr_queue*) calloc(1, sizeof(struct swmr_queue));
    assert (queue!=NULL);

    queue->shm = (uint8_t*) shm;
    queue->num_readers = num_readers;
    queue->id = id;

    queue->num_slots = (SWMRQ_SIZE)-(num_readers+2);
#ifdef DEBUG_SHM
    queue->num_slots = 10;
#endif
    queue->readers_pos = (union pos_pointer*) shm;
    queue->l_pos = 0;
    queue->data = (uint8_t*) shm+((num_readers+2)*sizeof(union pos_pointer));
    queue->next_sync = queue->num_slots-1;
    queue->next_seq = 1;
    queue->r_mask = 0xF;

    return queue;
}

// get the minimum of the readers pointer
void swmr_get_next_sync(struct swmr_queue* context, 
                   uint64_t* next)
{
    uint64_t min = 0xFFFFFFFF;
    for (int i = 0; i < context->num_readers; i++) {
        if (min > context->readers_pos[i].p[0]) {
            min = context->readers_pos[i].p[0];
        }
    }

    *next = min+(context->num_slots-1);
}



bool swmr_can_send(struct swmr_queue* context)
{
    if (context->next_seq == context->next_sync) {
        uint64_t next_sync;
        swmr_get_next_sync(context, &next_sync);
        if (context->next_sync < next_sync) {
            return true;
        }
        return false;
    } else {    
        return true;
    }
}

void swmr_send_raw(struct swmr_queue* context,
                  uintptr_t p1,
                  uintptr_t p2,
                  uintptr_t p3,
                  uintptr_t p4,
                  uintptr_t p5,
                  uintptr_t p6,
                  uintptr_t p7)
{

    uint64_t next_sync;
    //assert (context!=NULL);
    // if we reached the end reset local queue buffer pointer
    if ((context->l_pos) == context->num_slots) {
        context->l_pos = 0;
    }

    // get next sync at the sync point
    if (context->next_seq == context->next_sync) {
       swmr_get_next_sync(context, &next_sync);
       // block if there is no empty slot
       while(context->next_seq == next_sync) {
            swmr_get_next_sync(context, &next_sync);
       }
       context->next_sync = next_sync;
    }

    uintptr_t offset =  (context->l_pos*(CACHELINE_SIZE/sizeof(uintptr_t)));
    //assert (offset<SHMQ_SIZE*CACHELINE_SIZE);

    uintptr_t* slot_start = (uintptr_t*) context->data + offset;

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
        printf("Shm writer %d: write pos %d val %lu \n", 
                sched_getcpu(), context->l_pos, 
                context->next_seq);
#endif
    slot_start[0] = context->next_seq;
    context->next_seq++;

    // increse write pointer..
    context->l_pos = context->l_pos+1;
}

bool swmr_can_receive(struct swmr_queue* context)
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

// returns NULL if reader reached writers pos
bool swmr_receive_non_blocking(struct swmr_queue* context,
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
/*
        printf("Shm %d w %d: read pos %" PRIu16 " val1 %lu slot_start %p \n",
               sched_getcpu(), (sched_getcpu() % 4), context->l_pos,
                *((uintptr_t *) start) , start); 
*/
#ifdef DEBUG_SHM
        printf("Shm %d w %d: read pos %" PRIu16 " val1 %lu \n",
               sched_getcpu(), (sched_getcpu() % 4), context->l_pos,
                *((uintptr_t *) start)); 
    
#endif
        context->l_pos = (context->l_pos+1);
        context->next_seq++;
        if (context->l_pos == context->num_slots) {
            context->l_pos = 0;
        }

	// only update read pointer every 16th read
        if ((context->next_seq & 0xF) == 0) {
            context->readers_pos[context->id].p[0] = (context->next_seq -1);
        }
        return true;
    }
    return false;
}


// blocks
// TODO make this smarter?
void swmr_receive_raw(struct swmr_queue* context,
                     uintptr_t *p1,
                     uintptr_t *p2,
                     uintptr_t *p3,
                     uintptr_t *p4,
                     uintptr_t *p5,
                     uintptr_t *p6,
                     uintptr_t *p7)
{
    while(!swmr_receive_non_blocking(context, p1, p2, p3,
                                 p4, p5, p6, p7)){};

}

