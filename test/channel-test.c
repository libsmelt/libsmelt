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

#define NUM_RUNS 10000000
#define NUM_VALUES 1000

struct smlt_channel chan;

__thread struct sk_measurement m;
__thread cycles_t buf[NUM_VALUES];

void* worker1(void* arg)
{
    uint64_t id = (uint64_t) arg;
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(id, &cpu_mask);
    //uintptr_t r[8];
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);    

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
        for (int i = 0; i < NUM_RUNS; i++) {
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
    errval_t err;
    err = smlt_init(4, true);
    if (smlt_err_is_fail(err)) {
        printf("SMLT init failed \n");
    }        


    struct smlt_channel* c = &chan;
    uint32_t src = 0;
    uint32_t dst[3] = {1,2,3};
    smlt_channel_create(&c, &src, dst, 1, 3);   

    struct smlt_node* node;
    for (uint64_t i=0; i<4; i++) {
       node = smlt_get_node_by_id(i);
       err = smlt_node_start(node, worker1, (void*) i);
       if (smlt_err_is_fail(err)) {
           // XXX
       }
    }

    for (uint64_t i=0; i<4; i++) {
       node = smlt_get_node_by_id(i);
       err = smlt_node_join(node);
       if (smlt_err_is_fail(err)) {
           // XXX
       }
    }
}

