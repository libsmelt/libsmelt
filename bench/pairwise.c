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

// disables printing of mesasurements, adds other prints
//#define DEBUG
//#define PRINT_SUMMARY

static coreid_t step_size = 1;

// defines the batch size
#define NUM_MESSAGES 8
static size_t num_messages = NUM_MESSAGES;

struct smlt_qp ***queue_pairs;

#ifdef PRINT_SUMMARY
#define NUM_EXP 15000
#define NUM_DATA 5000
#else
#define NUM_EXP 10000
#define NUM_DATA 2000
#endif


#define STR(X) #X

/**
 * Generate the label for the measurements.
 *
 * Currently, this is {send,rtt,receive}-batchsize-sender-receiver
 */
#define INIT_SKM(func, nummsg, sender, receiver)                        \
    char _str_buf_##func[1024];                                         \
    cycles_t _buf_##func[NUM_DATA];                                     \
    struct sk_measurement m_##func;                                     \
    snprintf(_str_buf_##func, 1024, "%s-%zu-%d-%d",                     \
             STR(func), nummsg, sender, receiver);                      \
    sk_m_init(&m_##func, NUM_DATA, _str_buf_##func, _buf_##func);


struct thr_args {
    coreid_t s;
    coreid_t r;
    size_t num_messages;
};

void* thr_sender(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    struct smlt_qp *qp = queue_pairs[arg->s][arg->r];

    INIT_SKM(send, arg->num_messages, arg->s, arg->r);
    INIT_SKM(rtt, arg->num_messages, arg->s, arg->r);

    for (size_t i=0; i<NUM_EXP; i++) {
        sk_m_restart_tsc(&m_rtt);
        sk_m_restart_tsc(&m_send);
        for (size_t j = 0; j < arg->num_messages; j++) {
            smlt_queuepair_send(qp, msg);
        }
        sk_m_add(&m_send);

        for (size_t j = 0; j < arg->num_messages; j++) {
            smlt_queuepair_recv(qp, msg);
        }
        sk_m_add(&m_rtt);
    }

    smlt_message_free(msg);
#ifndef DEBUG
#ifdef PRINT_SUMMARY
    sk_m_print_analysis(&m_send);
    sk_m_print_analysis(&m_rtt);
#else
    sk_m_print(&m_send);
    sk_m_print(&m_rtt);
#endif
#endif
    return NULL;
}


void* thr_receiver(void* a)
{
    errval_t err;
    struct thr_args* arg = (struct thr_args*) a;
    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    struct smlt_qp *qp = queue_pairs[arg->r][arg->s];

    INIT_SKM(receive, arg->num_messages, arg->s, arg->r);

    for (size_t i=0; i<NUM_EXP; i++) {
        do {
            sk_m_restart_tsc(&m_receive);
            err = smlt_queuepair_try_recv(qp, msg);
        } while (err != SMLT_SUCCESS) ;
        for (size_t j = 1; j < arg->num_messages; j++) {
            err = smlt_queuepair_recv(qp, msg);
        }
        sk_m_add(&m_receive);
        for (size_t j = 0; j < arg->num_messages; j++) {
            smlt_queuepair_send(qp, msg);
        }
    }

    smlt_message_free(msg);

#ifndef DEBUG
#ifdef PRINT_SUMMARY
    sk_m_print_analysis(&m_receive);
#else
    sk_m_print(&m_receive);
#endif
#endif
    return NULL;
}


int main(int argc, char **argv)
{
    errval_t err;
    coreid_t num_cores = (coreid_t) sysconf(_SC_NPROCESSORS_CONF);
    printf("NUM_CORES=%d\n", num_cores);

    if (argc == 2) {
        step_size = atoi(argv[1]);
    }

    printf("STEP_SIZE=%d\n", step_size);


// enable this, if you want to make NUM_MSG depend ot the number of cores per cluster
#if 0
    uint32_t num_cores_per_cluster = smlt_platform_num_cores_of_cluster(0);
    assert(num_cores_per_cluster >= 1);
    if (num_messages > ((num_cores_per_cluster / step_size) - 2))  {
        num_messages = (num_cores_per_cluster / step_size) - 2;
    }
#endif

    printf("NUM_MSG=%zu\n", num_messages);
    assert(num_messages > 1);

    err = smlt_init(num_cores, false);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    struct smlt_node **nodes = (struct smlt_node**) calloc(num_cores, sizeof(void *));
    if (!nodes) {
        printf("FAILED TO INITIALIZE !\n");
        return -1;
    }


    queue_pairs = (struct smlt_qp***) calloc(num_cores, sizeof(*queue_pairs));
    if (queue_pairs == NULL) {
        printf("FAILED TO INITIALIZE !\n");
        return -1;
    }

    for (coreid_t s=0; s<num_cores; s++) {
        queue_pairs[s] = (struct smlt_qp**) calloc(num_cores, sizeof(void *));
        if (queue_pairs[s] == NULL) {
            printf("FAILED TO INITIALIZE !\n");
            return -1;
        }

        struct smlt_node_args args = {
                .id = s,
                .core = s,
                .num_nodes = num_cores
        };

        err = smlt_node_create(&nodes[s], &args);
        if (smlt_err_is_fail(err)) {
            printf("FAILED TO INITIALIZE !\n");
            return -1;
        }
    }

    for (coreid_t s=0; s<num_cores; s++) {
        for (coreid_t r=s+1; r<num_cores; r++) {
            struct smlt_qp **src = &(queue_pairs[s][r]);
            struct smlt_qp **dst = &(queue_pairs[r][s]);

            err = smlt_queuepair_create(SMLT_QP_TYPE_UMP,
                                        src, dst, s, r);
            if (smlt_err_is_fail(err)) {
                printf("FAILED TO INITIALIZE !\n");
                return -1;
            }
        }
    }

    fprintf(stderr, "# starting measurements...\n");

    for (coreid_t s=0; s<num_cores; s += step_size) {
        err = smlt_platform_pin_thread(s);
        if (smlt_err_is_fail(err)) {
            printf("FAILED TO INITIALIZE !\n");
            return -1;
        }

        fprintf(stderr, "# measuring [%u] -> [", s);

        for (coreid_t r=0; r<num_cores; r += step_size) {

            // Measurement does not make sense if sender == receiver
            if (s==r) continue;

            struct smlt_node *dst = nodes[r];
            fprintf(stderr, "%3u", r);

            for (size_t num_msg = 1; num_msg <= num_messages; num_msg *= 2) {

                struct thr_args arg = {
                    .s = s,
                    .r = r,
                    .num_messages = num_msg,
                };

                err = smlt_node_start(dst, thr_receiver, &arg);
                if (smlt_err_is_fail(err)) {
                    printf("Staring node failed \n");
                }

                thr_sender(&arg);

                // Wait for sender to complete
                smlt_node_join(dst);

            }
        }
	fprintf(stderr, "]\n");
    }
    fprintf(stderr, "# done.\n");
}
