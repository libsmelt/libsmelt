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
#include <math.h>
#include <unistd.h>
#include <smlt.h>
#include <smlt_node.h>
#include <smlt_queuepair.h>

#include <platforms/measurement_framework.h>

struct smlt_qp *queue_pairs;

cycles_t tsc_overhead = 0;

#define IGNORE_THRESHOLD 4000

#define NUM_EXP 10000
#define NUM_WARMUP 100

#define STR(X) #X

#define INIT_SKM(func, id, sender, receiver)                                \
        char _str_buf_##func[1024];                                                \
        cycles_t _buf_##func[NUM_EXP];                                             \
        struct sk_measurement m_##func;                                            \
        snprintf(_str_buf_##func, 1024, "%s%zu-%d-%d", STR(func), id, sender, receiver); \
        sk_m_init(&m_##func, NUM_EXP, _str_buf_##func, _buf_##func);


cycles_t tsc_measurements[NUM_EXP];

//#define bench_tsc() rdtscp()

struct thr_args {
    coreid_t s;
    coreid_t r;
    size_t num_messages;
};

static cycles_t *do_sorting(cycles_t *array,
                            size_t len)
{
    size_t i, j;
    cycles_t *sorted_array = array;
    cycles_t temp_holder;


    // sort the array
    for (i = 0; i < len; ++i) {
        for (j = i; j < len; ++j) {
            if (sorted_array[i] > sorted_array[j]) {
                temp_holder = sorted_array[i];
                sorted_array[i] = sorted_array[j];
                sorted_array[j] = temp_holder;
            }
        } // end for: j
    } // end for: i
    return sorted_array;
} // end function: do_sorting


void* thr_sender(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    struct smlt_qp *qp = &queue_pairs[2*arg->r];

    INIT_SKM(rtt, arg->num_messages, arg->s, arg->r);

    cycles_t tsc_start, tsc_end;

    for (size_t i=0; i<NUM_WARMUP; i++) {
        tsc_start = bench_tsc();
        smlt_queuepair_send(qp, msg);
        smlt_queuepair_recv(qp, msg);
        tsc_end = bench_tsc();
        tsc_measurements[i] = tsc_end - tsc_start - 2 * tsc_overhead;
    }

    for (size_t i=0; i<NUM_EXP; i++) {
        tsc_start = bench_tsc();
        smlt_queuepair_send(qp, msg);
        smlt_queuepair_recv(qp, msg);
        tsc_end = bench_tsc();
        tsc_measurements[i] = tsc_end - tsc_start - 2 * tsc_overhead;
    }

    cycles_t sum = 0;
    cycles_t max = 0;
    cycles_t min = (cycles_t)-1;
    size_t count = 0;

    cycles_t *sorted = do_sorting(tsc_measurements, NUM_EXP);

    for (size_t i=0; i<(NUM_EXP * 0.99); i++) {
        count ++;
        sum += tsc_measurements[i];
        if (tsc_measurements[i] < min) {
            min = tsc_measurements[i];
        }
        if (tsc_measurements[i] > max) {
            max = tsc_measurements[i];
        }
    }
    cycles_t avg = sum / NUM_EXP;

    sum = 0;
    for (size_t i=0; i<count; i++) {
        if (tsc_measurements[i] > avg) {
            sum += (tsc_measurements[i] - avg) * (tsc_measurements[i] - avg);
        } else {
            sum += (avg- tsc_measurements[i]) * (avg - tsc_measurements[i]);
        }
    }

    sum /= count;


    printf("RTT src=0, dst=%02u is avg=%5lu, stdev=%5lu, med=%5lu, min=%5lu, max=%5lu cycles, count=%lu, ignored=%lu\n",
            arg->r, avg, (cycles_t)sqrt(sum),sorted[NUM_EXP/2], min, max, count, NUM_EXP - count);


    return NULL;
}


void* thr_receiver(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;
    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    struct smlt_qp *qp = &queue_pairs[2*arg->r+1];

    for (size_t i=0; i<NUM_WARMUP; i++) {
        smlt_queuepair_recv(qp, msg);
        smlt_queuepair_send(qp, msg);
    }

    for (size_t i=0; i<NUM_EXP; i++) {
        smlt_queuepair_recv(qp, msg);
        smlt_queuepair_send(qp, msg);
    }

    return NULL;
}


int main(int argc, char **argv)
{
    errval_t err;
    coreid_t num_cores = (coreid_t) sysconf(_SC_NPROCESSORS_CONF);
    printf("Running with %d cores\n", num_cores);

    printf("sizeof(struct smlt_ump_queuepair) = %lu\n", sizeof(struct smlt_ump_queuepair));
    printf("sizeof(struct smlt_ump_queue) = %lu\n", sizeof(struct smlt_ump_queue));
    printf("Calibrating TSC overhead\n");

    #define TSC_ROUNDS 1000

    cycles_t tsc_start = bench_tsc();
    for (int i = 0; i < TSC_ROUNDS; ++i) {
        bench_tsc();
        bench_tsc();
        bench_tsc();
        bench_tsc();
        bench_tsc();
        bench_tsc();
        bench_tsc();
        bench_tsc();
        bench_tsc();
        bench_tsc();
    }
    cycles_t tsc_end = bench_tsc();

    tsc_overhead = (tsc_end - tsc_start) / (TSC_ROUNDS * 10);

    printf("Calibrating TSC overhead is %lu cycles\n", tsc_overhead);


    err = smlt_init(num_cores, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE !\n");
        return 1;
    }

    queue_pairs = calloc(2 * num_cores, sizeof(*queue_pairs));
    if (queue_pairs == NULL) {
        printf("FAILED TO INITIALIZE !\n");
        return -1;
    }

    for (coreid_t s=1; s<num_cores; s++) {
        struct smlt_qp* src = &(queue_pairs[2 * s]);
        struct smlt_qp* dst = &(queue_pairs[2 * s + 1]);

        err = smlt_queuepair_create(SMLT_QP_TYPE_UMP,
                                    src, dst, s, 0);
        if (smlt_err_is_fail(err)) {
            printf("FAILED TO INITIALIZE !\n");
            return -1;
        }
    }

    smlt_platform_pin_thread(0);

    for (coreid_t r=1; r<num_cores; r++) {

        //struct smlt_node *src = smlt_get_node_by_id(0);
        struct smlt_node *dst = smlt_get_node_by_id(r);

        struct thr_args arg = {
            .s = 0,
            .r = r,
        };

/*
        err = smlt_node_start(src, thr_sender, &arg);
        if (smlt_err_is_fail(err)) {
            printf("Staring node failed \n");
        }
*/

        err = smlt_node_start(dst, thr_receiver, &arg);
        if (smlt_err_is_fail(err)) {
            printf("Staring node failed \n");
        }

        thr_sender(&arg);

        // Wait for sender to complete
    //    smlt_node_join(src);
        smlt_node_join(dst);
    }
}
