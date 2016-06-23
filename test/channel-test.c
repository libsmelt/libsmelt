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
#include <platforms/measurement_framework.h>

#define NUM_RUNS 100000
#define NUM_VALUES 1000

#define NUM_READERS 3

struct smlt_channel chan;

__thread struct sk_measurement m;
__thread cycles_t buf[NUM_VALUES];

void* worker1(void* arg)
{
    uint64_t id = (uint64_t) arg;
    struct smlt_msg* msg = smlt_message_alloc(56);
    int num_wrong = 0;


    if (id == 0) {
        sk_m_init(&m, NUM_VALUES, "writer_shm", buf);
        for (uint64_t i = 0; i < NUM_RUNS; i++) {
            msg->data[0] = i;
            sk_m_restart_tsc(&m);
            smlt_channel_send(&chan, msg);
            sk_m_add(&m);

            smlt_channel_recv(&chan, msg);
            if (msg->data[0] != i) {
                num_wrong++;
            }
        }
    } else {
        sk_m_init(&m, NUM_VALUES, "reader_shm", buf);
        for (uint64_t i = 0; i < NUM_RUNS; i++) {
            sk_m_restart_tsc(&m);
            smlt_channel_recv(&chan, msg);
            sk_m_add(&m);
            if (msg->data[0] != i) {
                num_wrong++;
            }
            smlt_channel_send(&chan, msg);
        }
    }

    if (num_wrong) {
        printf("Node %d: Test Failed \n", smlt_node_get_id());
    } else {
        printf("Node %d: Test Succeeded \n", smlt_node_get_id());
    }
    sk_m_print_analysis(&m);   

    return 0;
}


int main(int argc, char ** argv)
{

    size_t nproc = sysconf( _SC_NPROCESSORS_ONLN );;

    errval_t err;
    err = smlt_init(nproc, true);
    if (smlt_err_is_fail(err)) {
        printf("SMLT init failed \n");
        return 1;
    }


    struct smlt_channel* c = &chan;
    uint32_t src = 0;
    /*
    uint32_t dst[NUM_READERS] = {4, 8, 12, 16, 20, 24, 28,
                                 32, 36, 40, 44, 48, 52, 56,
                                 60};
    */
    //uint32_t dst[NUM_READERS] = {4, 8, 12, 16, 20, 24, 28};
    //uint32_t dst[NUM_READERS] = {4,8,12};
    uint32_t dst[NUM_READERS] = {1,2,3};
    smlt_channel_create(&c, &src, dst, 1, NUM_READERS);   

    struct smlt_node* node;
    // writer
    node = smlt_get_node_by_id(0);
    err = smlt_node_start(node, worker1, (void*) 0);
    if (smlt_err_is_fail(err)) {
       // XXX
    }

    // readers
    for (uint64_t i=0; i<NUM_READERS; i++) {
       node = smlt_get_node_by_id(dst[i]);
       err = smlt_node_start(node, worker1, (void*) ((uint64_t) i+1));
       if (smlt_err_is_fail(err)) {
           // XXX
       }
    }

    node = smlt_get_node_by_id(0);
    err = smlt_node_join(node);
    if (smlt_err_is_fail(err)) {
           // XXX
    }

    for (uint64_t i=0; i<NUM_READERS; i++) {
       node = smlt_get_node_by_id(dst[i]);
       err = smlt_node_join(node);
       if (smlt_err_is_fail(err)) {
           // XXX
       }
    }

    return 0;
}
