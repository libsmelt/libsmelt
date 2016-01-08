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
#include <inttypes.h>

#include "shm/shm_queue.h"
#include "client/consensus_client.h"
#include "common/incremental_stats.h"

#define SHM_SIZE 4096
#define CACHELINE_SIZE 64


union __attribute__((aligned(64))) pos_pointer{
    uint64_t pos;
    uint8_t padding[CACHELINE_SIZE];
};

typedef struct shm_queue_t{	
    uint8_t* shm;
    uint64_t shm_size;
    struct pos_pointer* write_pos;
    struct pos_pointer* readers_pos;

    // for which replica is this shared memory
    uint8_t replica_id;
    uint8_t shm_id;
    uint8_t num_readers;
    uint8_t num_cores;
    uint64_t slot_size;
    uint16_t num_slots;
    bool node_level;
    void (*execute) (void* addr);
} shm_queue_t;

/* shared mutex/ memory */
static pthread_mutex_t mutex;
static bool init_done;
static void* shm_buffer;

static __thread shm_queue_t shm_queue;

/*
 * Sets up the memory layout starting from buf
 */
static void setup_memory(void* buf)
{
    shm_queue.num_slots = ((SHM_SIZE-((shm_queue.num_readers+1)*sizeof(struct pos_pointer)))
                           /shm_queue.slot_size) -1;

#ifdef DEBUG_SHM
    shm_queue.num_slots = 10;
#endif
    shm_queue.shm = (uint8_t*) buf+((shm_queue.num_readers+1)*sizeof(struct pos_pointer));
    shm_queue.write_pos = (struct pos_pointer*) buf;
    shm_queue.readers_pos = (struct pos_pointer*) buf+1;
}

static void default_exec_fn(void* addr);
static void default_exec_fn(void* addr)
{
    return;
}

void set_execution_fn_shm(void (*execute)(void * addr))
{
    shm_queue.execute = execute;
}


void init_shm_writer(uint8_t replica_id, 
        uint8_t current_core,
        uint8_t num_clients,
        uint8_t num_readers,
        uint64_t slot_size,
        bool node_level,	
        void* shared_mem, 
        void (*exec_fn)(void *))
{

    shm_queue.replica_id = replica_id;
    shm_queue.slot_size = slot_size;
    shm_queue.num_readers = num_readers;
    if (exec_fn == NULL) {
        shm_queue.execute = &default_exec_fn;
    } else {
        shm_queue.execute = exec_fn;
    }
    // -1 just to be sure
    shm_queue.num_slots = ((SHM_SIZE-((num_readers+2)*sizeof(struct pos_pointer)))/slot_size) -1;
#ifdef DEBUG_SHM
    shm_queue.num_slots = 10;
#endif
#ifdef MEASURE_TP
    tsc_per_ms = 2400000;
#endif

    void* buf;
    if (shared_mem != NULL) {
        buf = shared_mem;
    } else {

        pthread_mutex_lock(&mutex);
        if (!init_done) {
            shm_buffer = malloc(SHM_SIZE);
            init_done = true;
        }
        pthread_mutex_unlock(&mutex);         
        buf = shm_buffer;
    }

    setup_memory(buf);
    shm_queue.write_pos[0].pos = 0;
    if (node_level) {
        // TODO periodic event for measuring TP
    }

    printf("Writer on core %d: mapped at %p \n", current_core, buf);
    // TODO setup client stuff
}

void init_shm_reader(uint8_t id, 
                     uint8_t current_core,
                     uint8_t num_readers,
                     uint64_t slot_size,
                     bool node_level,
                     uint8_t started_from,
                     void* shared_mem, 
                     void (*exec_fn)(void* addr))
{
    shm_queue.replica_id = started_from;
    shm_queue.num_readers = num_readers;
    shm_queue.slot_size = slot_size;
    shm_queue.node_level = node_level;
    shm_queue.shm_id = id;
    if (exec_fn != NULL) {
        shm_queue.execute = exec_fn;
    } else {
        shm_queue.execute = &default_exec_fn;
    }

    void* buf;
    if (shared_mem != NULL) {
        buf = shared_mem;
    } else {

        pthread_mutex_lock(&mutex);
        if (!init_done) {
            shm_buffer = malloc(SHM_SIZE);
            init_done = true;
        }
        pthread_mutex_unlock(&mutex);
        buf = shm_buffer;
    }
    setup_memory(buf);
    printf("Reader on core %d: mapped at %p \n", current_core, buf);
    // TODO SET AFFINITY
#ifndef BARRELFISH
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(current_core, &set);
    sched_setaffinity(0, sizeof(set), &set);
#endif
}

static bool check_readers_end(void)
{
    for (int i = 0; i < shm_queue.num_readers; i++) {
        if (shm_queue.readers_pos[i].pos == 0) {

        } else {
            return false;
        }
    }	
    return true;    
}


#ifdef DEBUG_SHM
struct command {
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
};
#endif
// only single writer no need to lock
void shm_write(void* addr)
{
    // if we reached the end sync with readers
    if ((shm_queue.write_pos[0].pos) == 0) {
        while(!check_readers_end()){};
#ifdef DEBUG_SHM
        printf("#################################################### \n");
        printf("Synced \n");
        printf("#################################################### \n");
#endif
    }

    memcpy((shm_queue.shm+ (shm_queue.write_pos[0].pos*shm_queue.slot_size)), 
            addr, shm_queue.slot_size);
#ifdef DEBUG_SHM
    uint64_t* val = addr;
    printf("Shm writer %d: write pos %"PRIu64" val %"PRIu64" addr %p \n",
            sched_getcpu(), shm_queue.write_pos[0].pos, *val,
            shm_queue.shm +(shm_queue.write_pos[0].pos*shm_queue.slot_size));
#endif

    // avoid undefined behaviour warning ..
    uint64_t pos = shm_queue.write_pos[0].pos+1;
    pos = pos % shm_queue.num_slots;
    shm_queue.write_pos[0].pos = pos;
}


// returns NULL if reader reached writers pos
void* shm_read(void)
{
    void* ret_val = 0;
    ret_val = shm_queue.shm + 
        ((shm_queue.readers_pos[shm_queue.shm_id].pos)*shm_queue.slot_size);
    if (shm_queue.readers_pos[shm_queue.shm_id].pos == shm_queue.write_pos[0].pos) {
        return NULL;
    } else {
#ifdef DEBUG_SHM
        printf("Shm %d: read pos %"PRIu64" val %"PRIu64" ret_val %p \n", sched_getcpu(),
                shm_queue.readers_pos[shm_queue.shm_id].pos,
                *((uint64_t *) ret_val) ,ret_val);
#endif
        uint64_t pos = shm_queue.readers_pos[shm_queue.shm_id].pos+1;
        pos = pos % shm_queue.num_slots;
        shm_queue.readers_pos[shm_queue.shm_id].pos = pos;

        return ret_val;
    }
}

void poll_and_execute(void)
{   
    while(true) {
        void* cmd = NULL;
        while (cmd == NULL) {
            cmd = shm_read();
            if (cmd == NULL) {
                // thread_yield();
            } else {
                shm_queue.execute(cmd);
#ifdef DEBUG_SHM
     //           printf("Shm %d: read %"PRIu64" \n", sched_getcpu(), ((struct command *) cmd)->arg1);
#endif
            }
        }
    }
}


