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

struct smlt_qp *queue_pairs[2];
size_t chan_per_core = 0;
cycles_t tsc_overhead = 0;

#define NUM_CHANNELS 1000

#define IGNORE_THRESHOLD 4000

#define NUM_EXP 5000
#define NUM_WARMUP 64

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
    coreid_t num_cores;
    size_t num_channels;
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



void* thr_poll(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    INIT_SKM(rtt, 1UL, arg->s, arg->r);

    cycles_t tsc_start, tsc_end;

    for (size_t i=0; i<NUM_WARMUP; i++) {
        tsc_start = bench_tsc();
        for (size_t n = 0; n < arg->num_channels; ++n) {
            struct smlt_qp *qp = &queue_pairs[0][n];
            smlt_queuepair_can_recv(qp);
        }
        tsc_end = bench_tsc();
        tsc_measurements[i] = tsc_end - tsc_start - 2 * tsc_overhead;
    }

    for (size_t i=0; i<NUM_EXP; i++) {
        tsc_start = bench_tsc();
        for (size_t n = 0; n < arg->num_channels; ++n) {
            struct smlt_qp *qp = &queue_pairs[0][n];
            smlt_queuepair_can_recv(qp);
        }
        tsc_end = bench_tsc();
        tsc_measurements[i] = tsc_end - tsc_start - 2 * tsc_overhead;
    }

    cycles_t sum = 0;
    cycles_t max = 0;
    cycles_t min = (cycles_t)-1;
    size_t count = 0;

    cycles_t *sorted = do_sorting(tsc_measurements, NUM_EXP);

    for (size_t i=0; i<(NUM_EXP * 0.95); i++) {
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

    printf("POLL %lu, src=0, dst=%02u is avg=%5lu, stdev=%5lu, med=%5lu, min=%5lu, max=%5lu cycles, count=%lu, ignored=%lu\n",
            arg->num_channels, arg->r, avg, (cycles_t)sqrt(sum),sorted[NUM_EXP/2], min, max, count, NUM_EXP - count);


    return NULL;
}

void* thr_poll_recv(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    INIT_SKM(rtt, 1UL, arg->s, arg->r);

    cycles_t tsc_start, tsc_end;

    size_t num_chan = arg->num_channels * arg->num_cores;

    for (size_t i=0; i<NUM_WARMUP; i++) {
        tsc_start = bench_tsc();
        for (size_t n = 0; n < num_chan; ++n) {
            struct smlt_qp *qp = &queue_pairs[0][n];
        //    printf("POLL warmup recv: %lu/%lu, qp[%lu]\n", n, num_chan, n);
            smlt_queuepair_recv(qp, msg);
        }
        tsc_end = bench_tsc();
        tsc_measurements[i] = tsc_end - tsc_start -  tsc_overhead;

        for (size_t n = 0; n < num_chan; ++n) {
            struct smlt_qp *qp = &queue_pairs[0][n];
        //    printf("POLL warmup send: %lu/%lu\n", n, num_chan);
            smlt_queuepair_send(qp, msg);
        }
    }

    for (size_t i=0; i<NUM_EXP; i++) {
        errval_t err;
        struct smlt_qp *qp = &queue_pairs[0][0];
    //    printf("POLL try recv: %u/%lu, qp[0]\n", 0, num_chan);
        do {
            tsc_start = bench_tsc();

            err = smlt_queuepair_try_recv(qp, msg);
        } while(err != SMLT_SUCCESS);

        for (size_t n = 1; n < num_chan; ++n) {
            qp = &queue_pairs[0][n];
        //    printf("POLL recv: %lu/%lu qp[%lu]\n", n, num_chan, n);
            smlt_queuepair_recv(qp, msg);
        }
        tsc_end = bench_tsc();
        tsc_measurements[i] = tsc_end - tsc_start - tsc_overhead;

        for (size_t n = 0; n < num_chan; ++n) {
            struct smlt_qp *qp = &queue_pairs[0][n];
        //    printf("POLL send: %lu/%lu qp[%lu]\n", n, num_chan, n);
            smlt_queuepair_send(qp, msg);
        }
    }

    cycles_t sum = 0;
    cycles_t max = 0;
    cycles_t min = (cycles_t)-1;
    size_t count = 0;

    cycles_t *sorted = do_sorting(tsc_measurements, NUM_EXP);

    for (size_t i=0; i<(NUM_EXP * 0.95); i++) {
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

    printf("POLL %lu, src=0, dst=%02u is avg=%5lu, stdev=%5lu, med=%5lu, min=%5lu, max=%5lu cycles, count=%lu, ignored=%lu\n",
            num_chan, arg->r, avg, (cycles_t)sqrt(sum),sorted[NUM_EXP/2], min, max, count, NUM_EXP - count);


    return NULL;
}



void* thr_receiver(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;
    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

//    printf("recv starated: %lu, arg->r = %u, arg->num_cores=%u, arg->num_chan=%lu\n", arg->num_channels, arg->r, arg->num_cores, arg->num_channels);

    for (size_t j=0; j<NUM_WARMUP; j++) {
        for (size_t i = 0; i < arg->num_channels; ++i) {
            struct smlt_qp *qp = &queue_pairs[1][arg->r + i * arg->num_cores];
        //    printf("warmup send: %lu/%lu qp[%lu]\n", i, arg->num_channels, arg->r + i * arg->num_cores);
            smlt_queuepair_send(qp, msg);
        }

        for (size_t i = 0; i < arg->num_channels; ++i) {
        //    printf("warmup recv: %lu/%lu qp[%lu]\n", i, arg->num_channels, arg->r + i * arg->num_cores);
            struct smlt_qp *qp = &queue_pairs[1][arg->r + i * arg->num_cores];
            smlt_queuepair_recv(qp, msg);
        }
    }

//    printf("real rounds\n");

    for (size_t j=0; j<NUM_EXP; j++) {
        for (size_t i = 0; i < arg->num_channels; ++i) {
        //    printf("send: %lu/%lu qp[%lu]\n", i, arg->num_channels, arg->r + i * arg->num_cores);
            struct smlt_qp *qp = &queue_pairs[1][arg->r + i * arg->num_cores];
            smlt_queuepair_send(qp, msg);
        }

        for (size_t i = 0; i < arg->num_channels; ++i) {
    //        printf("recv: %lu/%lu qp[%lu]\n", i, arg->num_channels, arg->r + i * arg->num_cores);
            struct smlt_qp *qp = &queue_pairs[1][arg->r + i * arg->num_cores];
            smlt_queuepair_recv(qp, msg);
        }
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
        printf("FAILED TO INITIALIZE smlt !\n");
        return 1;
    }

    queue_pairs[0] = calloc(NUM_CHANNELS, sizeof(*queue_pairs[0]));
    queue_pairs[1] = calloc(NUM_CHANNELS, sizeof(*queue_pairs[1]));
    if (queue_pairs[0] == NULL || queue_pairs[1] == NULL) {
        printf("FAILED TO INITIALIZE calloc!\n");
        return -1;
    }

    coreid_t core = 1;

    chan_per_core = (NUM_CHANNELS / (num_cores - 1));
    size_t chan_max = chan_per_core * (num_cores - 1);
    for (size_t s=0; s<chan_max; s++) {
        struct smlt_qp* src = &(queue_pairs[0][s]);
        struct smlt_qp* dst = &(queue_pairs[1][s]);



        err = smlt_queuepair_create(SMLT_QP_TYPE_UMP,
                                    &src, &dst, 0, core);
        if (smlt_err_is_fail(err)) {
            printf("FAILED TO INITIALIZE queuepair! for core %u\n", core);
            return -1;
        }

        if (++core == num_cores) {
            core = 1;
        }
    }

    smlt_platform_pin_thread(0);

#if 0
    for (size_t i = 0; i < chan_max; i += 2*(num_cores - 1)) {
        struct thr_args arg = {
            .s = 0,
            .r = 0,
            .num_channels = i
        };

        thr_poll(&arg);
    }
#endif

    struct thr_args args[num_cores];

    for (size_t r=1; r<chan_per_core;  ++r) {
        for (coreid_t core = 1; core < (num_cores); ++core) {
            //struct smlt_node *src = smlt_get_node_by_id(0);
            struct smlt_node *dst = smlt_get_node_by_id(core);

            args[core].s = 0;
            args[core].r = core - 1;
            args[core].num_channels = r;
            args[core].num_cores = num_cores -1;

        //    printf("starting.. %lu \n", r);

            err = smlt_node_start(dst, thr_receiver, args + core);
            if (smlt_err_is_fail(err)) {
                printf("Staring node failed \n");
            }
        }

        struct thr_args arg = {
            .s = 0,
            .r = core,
            .num_channels = r,
            .num_cores = num_cores -1
        };

        thr_poll_recv(&arg);

        // Wait for sender to complete
        for (coreid_t core = 1; core < (num_cores); ++core) {
            //struct smlt_node *src = smlt_get_node_by_id(0);
            struct smlt_node *dst = smlt_get_node_by_id(core);
            smlt_node_join(dst);
        }
        sleep(1);
    }
}
