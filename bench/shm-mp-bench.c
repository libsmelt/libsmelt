/**
 * \brief Testing Shared memory queue
 */

/*
 * Copyright (c) 2015, ETH Zurich and Mircosoft Corporation.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <numa.h>
#include <sched.h>
#include <smlt.h>
#include <smlt_node.h>
#include <smlt_channel.h>
#include <smlt_message.h>
#include <smlt_barrier.h>
#include <platforms/measurement_framework.h>

#define NUM_RUNS 1000000
#define NUM_VALUES 1000

struct smlt_channel shm;
struct smlt_channel* mp;
struct smlt_dissem_barrier* bar;

int num_threads = 0;


__thread struct sk_measurement m;
__thread cycles_t buf[NUM_VALUES];

void* shm_func(void* arg)
{
    uint64_t id = (uint64_t) arg;
    struct smlt_msg* msg = smlt_message_alloc(56);

    char writer[128];
    char reader[128];

    sprintf(writer, "writer_shm%d", num_threads);
    sprintf(reader, "reader_shm%d", num_threads);

    if (id == 0) {
        sk_m_init(&m, NUM_VALUES, writer, buf);
        for (uint64_t i = 0; i < NUM_RUNS; i++) {
            msg->data[0] = i;
            // synchro
            smlt_dissem_barrier_wait(bar);
            smlt_dissem_barrier_wait(bar);
    
            sk_m_restart_tsc(&m);
            smlt_channel_send(&shm, msg);
            sk_m_add(&m);

        }
    } else {

        sk_m_init(&m, NUM_VALUES, reader, buf);
        for (uint64_t i = 0; i < NUM_RUNS; i++) {
            // synchro
            smlt_dissem_barrier_wait(bar);
            smlt_dissem_barrier_wait(bar);

            sk_m_restart_tsc(&m);
            smlt_channel_recv(&shm, msg);
            sk_m_add(&m);
        }
    }

    sk_m_print(&m);   

    return 0;
}

// mp
void* mp_func(void* arg)
{
    uint64_t id = (uint64_t) arg;
    struct smlt_msg* msg = smlt_message_alloc(56);

    char writer[128];
    char reader[128];

    sprintf(writer, "writer_mp%d", num_threads);
    sprintf(reader, "reader_mp%d", num_threads);

    if (id == 0) {
        sk_m_init(&m, NUM_VALUES, writer, buf);
        for (uint64_t i = 0; i < NUM_RUNS; i++) {
            msg->data[0] = i;
            // synchro
            smlt_dissem_barrier_wait(bar);
            smlt_dissem_barrier_wait(bar);
    
            sk_m_restart_tsc(&m);
            for (int j = 0; j < num_threads-1; j++) {
                smlt_channel_send(&mp[j], msg);
            }
            sk_m_add(&m);

        }
    } else {

        sk_m_init(&m, NUM_VALUES, reader, buf);
        for (uint64_t i = 0; i < NUM_RUNS; i++) {
            // synchro
            smlt_dissem_barrier_wait(bar);
            smlt_dissem_barrier_wait(bar);

            sk_m_restart_tsc(&m);
            smlt_channel_recv(&mp[id-1], msg);
            sk_m_add(&m);
        }
    }

    sk_m_print(&m);   

    return 0;
}


// multiple readers
void run(bool use_mp)
{
    errval_t err;
    struct smlt_channel* c;;
    uint32_t num_cores;
    uint32_t* cores;

    err = smlt_platform_cores_of_cluster(0, &cores, &num_cores);
    if (smlt_err_is_fail(err)) {
        printf("Failed to get cores of cluster 0 \n");
        exit(1);
    }

    mp = smlt_platform_alloc(sizeof(struct smlt_channel)*num_cores,
                             SMLT_ARCH_CACHELINE_SIZE, true);
    for (int i = 2; i < num_cores+1; i++) {

        num_threads = i;
        for (int j = 0; j < i; j++) {
            fprintf(stderr,"%d ", j);
        }
        fprintf(stderr,"\n");

        err = smlt_dissem_barrier_init(cores, i, &bar);
        if (smlt_err_is_fail(err)) {
            printf("Init Barrier failed \n");
            exit(1);
        }

        if (use_mp) {
            for (int j = 0; j < i; j++) {
                c = &mp[j];
                smlt_channel_create(&c, &cores[0], &cores[j], 1, 1);  
            }
        } else {
            c = &shm;
            smlt_channel_create(&c, &cores[0], &cores[1], 1, i-1);  
        } 

        struct smlt_node* node;
        // readers
        for (uint64_t j=0; j < i; j++) {
           node = smlt_get_node_by_id(cores[j]);
           if (use_mp) {
               err = smlt_node_start(node, mp_func, (void*) j);
           } else {
               err = smlt_node_start(node, shm_func, (void*) j);
           }

           if (smlt_err_is_fail(err)) {
               // XXX
           }
        }

        for (uint64_t j=0; j < i; j++) {
           node = smlt_get_node_by_id(cores[j]);
           err = smlt_node_join(node);
           if (smlt_err_is_fail(err)) {
               // XXX
           }
        }

    }
}

int main(int argc, char ** argv)
{
    errval_t err;
    err = smlt_init(sysconf(_SC_NPROCESSORS_ONLN), true);
    if (smlt_err_is_fail(err)) {
        printf("SMLT init failed \n");
    }        

    run(false);
    run(true);
    
}



