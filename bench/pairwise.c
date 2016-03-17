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


#define NUM_EXP 1000

#define INIT_SKM(func, sender, receiver)                                \
        char _str_buf[1024];                                            \
        cycles_t _buf[NUM_EXP];                                         \
        struct sk_measurement m;                                        \
        snprintf(_str_buf, 1024, "%s-%d-%d", func, sender, receiver);   \
        sk_m_init(&m, NUM_EXP, _str_buf, _buf);                         \
                                                                        \

struct smlt_qp**queue_pairs;

struct thr_args {
    coreid_t s;
    coreid_t r;
};

void* thr_sender(void* a)
{
    errval_t err;
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 1;

    struct smlt_qp *qp = &queue_pairs[arg->s][arg->r];

    INIT_SKM("send", arg->s, arg->r);

    for (int i=0; i<NUM_EXP; i++) {

        sk_m_restart_tsc(&m);
        msg->data[0] = i;
        err = smlt_queuepair_send(qp, msg);
        sk_m_add(&m);
        if (smlt_err_is_fail(err)) {
            printf("thr_sender recv\n");
        }

        err = smlt_queuepair_recv(qp, msg);
        if (smlt_err_is_fail(err)) {
            printf("thr_sender recv\n");
        }
    }

    sk_m_print(&m);

    return NULL;
}


void* thr_receiver(void* a)
{
    errval_t err;
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_qp *qp = &queue_pairs[arg->r][arg->s];


    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 1;

    INIT_SKM("receive", arg->s, arg->r);

    for (int i=0; i<NUM_EXP; i++) {

        sk_m_restart_tsc(&m);
        err = smlt_queuepair_recv(qp, msg);
        sk_m_add(&m);
        if (smlt_err_is_fail(err)) {
            printf("thr_sender recv\n");
        }

        msg->data[0] = i;
        err = smlt_queuepair_send(qp, msg);
        if (smlt_err_is_fail(err)) {
            printf("thr_sender recv\n");
        }
    }

    sk_m_print(&m);

    return NULL;
}


int main(int argc, char **argv)
{
    errval_t err;
    coreid_t num_cores = (coreid_t) sysconf(_SC_NPROCESSORS_CONF);

    err = smlt_init(num_cores, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    queue_pairs = calloc(num_cores, sizeof(void *));
    if (queue_pairs == NULL) {
        printf("FAILED TO INITIALIZE !\n");
        return -1;
    }

    for (coreid_t s=0; s<num_cores; s++) {
        queue_pairs[s] = calloc(num_cores, sizeof(struct smlt_qp));
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
                                        src, dst, s, r);
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

            struct thr_args arg = {
                .s = s,
                .r = r
            };

            struct smlt_node *src = smlt_get_node_by_id(s);
            struct smlt_node *dst = smlt_get_node_by_id(r);

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
