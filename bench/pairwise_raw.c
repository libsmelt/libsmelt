/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <smlt.h>
#include <smlt_node.h>
#include <smlt_queuepair.h>

#include <platforms/measurement_framework.h>

struct smlt_qp **queue_pairs;

#define NUM_EXP 10000

#define STR(X) #X

#define INIT_SKM(func, id, sender, receiver)                                \
        char _str_buf_##func[1024];                                                \
        cycles_t _buf_##func[NUM_EXP];                                             \
        struct sk_measurement m_##func;                                            \
        snprintf(_str_buf_##func, 1024, "%s%zu-%d-%d", STR(func), id, sender, receiver); \
        sk_m_init(&m_##func, NUM_EXP, _str_buf_##func, _buf_##func);


struct thr_args {
    coreid_t s;
    coreid_t r;
    size_t num_messages;
};

void* thr_sender(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 1;

    struct smlt_qp *qp = &queue_pairs[arg->s][arg->r];

    INIT_SKM(send, arg->num_messages, arg->s, arg->r);
    INIT_SKM(rtt, arg->num_messages, arg->s, arg->r);

    for (size_t i=0; i<NUM_EXP; i++) {
        sk_m_restart_tsc(&m_rtt);
        sk_m_restart_tsc(&m_send);
        for (size_t j = 0; j <= arg->num_messages; j++) {
            msg->data[0] = i;
            smlt_queuepair_send(qp, msg);
        }
        sk_m_add(&m_send);

        for (size_t j = 0; j <= arg->num_messages; j++) {
            smlt_queuepair_recv(qp, msg);
            assert(msg->data[0] == i);
        }
        sk_m_add(&m_rtt);
    }

    sk_m_print(&m_send);
    sk_m_print(&m_rtt);

    return NULL;
}


void* thr_receiver(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;
    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 1;

    struct smlt_qp *qp = &queue_pairs[arg->r][arg->s];

    INIT_SKM(receive, arg->num_messages, arg->s, arg->r);

    for (size_t i=0; i<NUM_EXP; i++) {

        for (size_t j = 0; j <= arg->num_messages; j++) {
            smlt_queuepair_recv(qp, msg);
            assert(msg->data[0] == i);
        }
        sk_m_add(&m_receive);
        for (size_t j = 0; j <= arg->num_messages; j++) {
            msg->data[0] = i;
            smlt_queuepair_send(qp, msg);
        }
    }

    sk_m_print(&m_receive);

    return NULL;
}


int main(int argc, char **argv)
{
    errval_t err;
    coreid_t num_cores = (coreid_t) sysconf(_SC_NPROCESSORS_CONF);
    printf("Running with %d cores\n", num_cores);

    err = smlt_init(num_cores, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    queue_pairs = calloc(num_cores, sizeof(*queue_pairs));
    if (queue_pairs == NULL) {
        printf("FAILED TO INITIALIZE !\n");
        return -1;
    }

    for (coreid_t s=0; s<num_cores; s++) {
        queue_pairs[s] = calloc(num_cores, sizeof(**queue_pairs));
        if (queue_pairs[s] == NULL) {
            printf("FAILED TO INITIALIZE !\n");
            return -1;
        }
    }

    for (coreid_t s=0; s<num_cores; s++) {
        for (coreid_t r=s+1; r<num_cores; r++) {
            struct smlt_qp* src = &(queue_pairs[s][r]);
            struct smlt_qp* dst = &(queue_pairs[r][s]);

            err = smlt_queuepair_create(SMLT_QP_TYPE_UMP,
                                        &src, &dst, s, r);
            if (smlt_err_is_fail(err)) {
                printf("FAILED TO INITIALIZE !\n");
                return -1;
            }
        }
    }

    for (coreid_t s=0; s<num_cores; s++) {
        for (coreid_t r=0; r<num_cores; r++) {

            // Measurement does not make sense if sender == receiver
            if (s==r) continue;

            struct smlt_node *src = smlt_get_node_by_id(s);
            struct smlt_node *dst = smlt_get_node_by_id(r);

            for (size_t num_messages = 1; num_messages <= num_cores; num_messages++) {
                struct thr_args arg = {
                    .s = s,
                    .r = r,
                    .num_messages = num_messages,
                };

                err = smlt_node_start(src, thr_sender, &arg);
                if (smlt_err_is_fail(err)) {
                    printf("Staring node failed \n");
                }

                err = smlt_node_start(dst, thr_receiver, &arg);
                if (smlt_err_is_fail(err)) {
                    printf("Staring node failed \n");
                }


                // Wait for sender to complete
                smlt_node_join(src);
                smlt_node_join(dst);
            }
        }
    }
}
