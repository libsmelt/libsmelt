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

#include "shm.h"


//#define DEBUG_SHM
#define END 1
#define RESET 9

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
struct shm_queue* shm_init_context(void* shm,
                                   uint8_t num_readers,
                                   uint8_t id)
{

    struct shm_queue* queue = (struct shm_queue*) calloc(1, sizeof(struct shm_queue));
    assert (queue!=NULL);

    queue->shm = (uint8_t*) shm;
    queue->num_readers = num_readers;
    queue->id = id;

    queue->num_slots = (SHMQ_SIZE)-(num_readers+2);
#ifdef DEBUG_SHM
    queue->num_slots = 10;
#endif
    queue->write_pos = (union pos_pointer*) shm;
    queue->readers_pos = (union pos_pointer*) shm+1;
    queue->l_pos = 0;
    queue->data = (uint8_t*) shm+((num_readers+2)*sizeof(union pos_pointer));

    return queue;
}

static bool check_readers_end(struct shm_queue* queue)
{

    for (int i = 0; i < queue->num_readers; i++) {
        volatile uint64_t pos = queue->readers_pos[i].pos;
        if (pos == END) {

        } else {
            return false;
        }
    }
    return true;
}

void shm_send_raw(struct shm_queue* context,
                  uintptr_t p1,
                  uintptr_t p2,
                  uintptr_t p3,
                  uintptr_t p4,
                  uintptr_t p5,
                  uintptr_t p6,
                  uintptr_t p7)
{
    assert (context!=NULL);
    // if we reached the end sync with readers
    if ((context->write_pos[0].pos) == context->num_slots) {
            //printf("Writer waiting %d \n", sched_getcpu());
            while(!check_readers_end(context)){};

#ifdef DEBUG_SHM
            printf("#################################################### \n");
            printf("Synced %d \n", sched_getcpu());
            printf("#################################################### \n");
#endif       
            // reset new pointer
            context->write_pos[0].pos = 0;
            // reset readers
            for (int i = 0; i < context->num_readers; i++) {
                context->readers_pos[i].pos = RESET;
            }
    }

    uintptr_t offset =  (context->write_pos[0].pos*
                         (CACHELINE_SIZE/sizeof(uintptr_t)));
    assert (offset<SHMQ_SIZE*CACHELINE_SIZE);

    uintptr_t* slot_start = (uintptr_t*) context->data + offset;

    slot_start[0] = p1;
    slot_start[1] = p2; 
    slot_start[2] = p3; 
    slot_start[3] = p4;
    slot_start[4] = p5; 
    slot_start[5] = p6;
    slot_start[6] = p7; 

#ifdef DEBUG_SHM
       /* printf("Shm writer %d epoch %d: write pos %" PRIu64 " addr %p value1 %lu \n",
            sched_getcpu(), context->epoch, context->write_pos[0].pos[context->epoch],
            slot_start, slot_start[0]); */
        printf("Shm writer %d: write pos %" PRIu64 " val %lu \n", 
                sched_getcpu(), context->write_pos[0].pos, 
                slot_start[0]);
#endif

    // increse write pointer..
    uint64_t pos = context->write_pos[0].pos+1;
    context->write_pos[0].pos = pos;
}


bool check_reader_p(struct shm_queue* context)
{
    if (context->readers_pos[context->id].pos == 9) {
        return false;
    } else {
        return true;
    }
}

// returns NULL if reader reached writers pos
bool shm_receive_non_blocking(struct shm_queue* context,
              uintptr_t *p1,
              uintptr_t *p2,
              uintptr_t *p3,
              uintptr_t *p4,
              uintptr_t *p5,
              uintptr_t *p6,
              uintptr_t *p7)
{
    assert (context!=NULL);

    if (context->l_pos == context->num_slots) {
        // at end
        context->readers_pos[context->id].pos = END;
        context->l_pos = 0;

        // wait until writer resets
        while (check_reader_p(context)){};

#ifdef DEBUG_SHM
        printf("Reader %d: reset id %d\n", 
                    sched_getcpu(), context->id);
#endif
    }

    if ((context->l_pos == (context->write_pos[0].pos))) {
        return false;
    } else {

        uintptr_t* start;
        start = (uintptr_t*) context->data + ((context->l_pos)*
                 CACHELINE_SIZE/(sizeof(uintptr_t)));
        *p1 = start[0];
        *p2 = start[1];
        *p3 = start[2]; 
        *p4 = start[3]; 
        *p5 = start[4]; 
        *p6 = start[5]; 
        *p7 = start[6]; 
/*
        printf("Shm %d w %d: read pos %" PRIu16 " val1 %lu slot_start %p \n",
               sched_getcpu(), (sched_getcpu() % 4), context->l_pos,
                *((uintptr_t *) start) , start); 
*/
#ifdef DEBUG_SHM
        printf("Shm %d w %d: read pos %" PRIu16 " val1 %lu slot_start %p \n",
               sched_getcpu(), (sched_getcpu() % 4), context->l_pos,
                *((uintptr_t *) start) , start); 
    
#endif
        context->l_pos = (context->l_pos+1);
        return true;
    }
}


// blocks
// TODO make this smarter?
void shm_receive_raw(struct shm_queue* context,
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

