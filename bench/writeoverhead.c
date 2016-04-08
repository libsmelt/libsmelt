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

struct smlt_qp **queue_pairs[2];
size_t chan_per_core = 0;
cycles_t tsc_overhead = 0;

#define NUM_CHANNELS 1024

#define NUM_DATA 10000
#define NUM_EXP 20000
#define NUM_WARMUP 5000

#define STR(X) #X

char *glb_label = NULL;

cycles_t tsc_measurements[NUM_DATA];

//#define bench_tsc() rdtscp()

struct thr_args {
    coreid_t s;
    coreid_t r;
    coreid_t num_cores;
    size_t num_channels;
    size_t num_msg;
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



void* thr_write(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    cycles_t tsc_start, tsc_end;

    size_t num_chan = arg->num_channels;

    if (arg->r == 0) {
        for (size_t i=0; i<NUM_EXP; i++) {
            struct smlt_qp *qp = queue_pairs[0][0];
            tsc_start = bench_tsc();
            for (size_t n = arg->s; n < arg->s+num_chan; ++n) {
                qp = queue_pairs[0][n];
                smlt_queuepair_send(qp, msg);
            }
            tsc_end = bench_tsc();
            tsc_measurements[i % NUM_DATA] = (tsc_end - tsc_start) / num_chan;
            for (size_t n = arg->s; n < arg->s + num_chan; ++n) {
                qp = queue_pairs[0][n];
                smlt_queuepair_recv(qp, msg);
            }
        }
    } else {
        for (size_t i=0; i<NUM_EXP; i++) {
            struct smlt_qp *qp = queue_pairs[0][0];
            tsc_start = bench_tsc();
            for (size_t n = 1; n <= num_chan; ++n) {
                qp = queue_pairs[0][n * arg->r];
                smlt_queuepair_send(qp, msg);
            }
            tsc_end = bench_tsc();
            tsc_measurements[i%NUM_DATA] = (tsc_end - tsc_start) / num_chan;
            for (size_t n = 1; n <= num_chan; ++n) {
                qp = queue_pairs[0][n * arg->r];
                smlt_queuepair_recv(qp, msg);
            }
        }
    }

    cycles_t sum = 0, max = 0, min = (cycles_t)-1;
    size_t count = 0;

    cycles_t *sorted = do_sorting(tsc_measurements, NUM_DATA);

    for (size_t i=0; i<(NUM_DATA * 0.95); i++) {
        count ++;
        sum += tsc_measurements[i];
        if (tsc_measurements[i] < min) {
            min = tsc_measurements[i];
        }
        if (tsc_measurements[i] > max) {
            max = tsc_measurements[i];
        }
    }
    cycles_t avg = sum / count;

    sum = 0;
    for (size_t i=0; i<count; i++) {
        if (tsc_measurements[i] > avg) {
            sum += (tsc_measurements[i] - avg) * (tsc_measurements[i] - avg);
        } else {
            sum += (avg- tsc_measurements[i]) * (avg - tsc_measurements[i]);
        }
    }

    sum /= count;

    printf("%s %3lu, avg=%lu, stdev=%lu, med=%lu, min=%lu, max=%lu cycles, count=%lu, ignored=%lu\n",
            glb_label, arg->num_channels, avg, (cycles_t)sqrt(sum),sorted[count/2], min, max, count, NUM_EXP - count);

    return NULL;
}



void* thr_receiver(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;
    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    for (size_t j=0; j<NUM_EXP; j++) {
        for (size_t i = 0; i < arg->num_channels; ++i) {
            struct smlt_qp *qp = queue_pairs[1][arg->r + i * arg->num_cores];
            smlt_queuepair_recv(qp, msg);
        }
        for (size_t i = 0; i < arg->num_channels; ++i) {
            struct smlt_qp *qp = queue_pairs[1][arg->r + i * arg->num_cores];
            smlt_queuepair_send(qp, msg);
        }

    }

    return NULL;
}


void* thr_write_one(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    cycles_t tsc_start, tsc_end;


    struct smlt_qp *qp = queue_pairs[0][arg->r];

    for (size_t j=0; j<NUM_EXP; j++) {
        tsc_start = bench_tsc();
        for (size_t i = 0; i < arg->num_msg; ++i) {
            smlt_queuepair_send(qp, msg);
        }
        tsc_end = bench_tsc();
        tsc_measurements[j % NUM_DATA] = (tsc_end - tsc_start) / arg->num_msg;

        smlt_queuepair_recv(qp, msg);
    }
    cycles_t sum = 0, max = 0, min = (cycles_t)-1;
    size_t count = 0;

    cycles_t *sorted = do_sorting(tsc_measurements, NUM_DATA);

    for (size_t i=0; i<(NUM_DATA * 0.95); i++) {
        count ++;
        sum += tsc_measurements[i];
        if (tsc_measurements[i] < min) {
            min = tsc_measurements[i];
        }
        if (tsc_measurements[i] > max) {
            max = tsc_measurements[i];
        }
    }
    cycles_t avg = sum / count;

    sum = 0;
    for (size_t i=0; i<count; i++) {
        if (tsc_measurements[i] > avg) {
            sum += (tsc_measurements[i] - avg) * (tsc_measurements[i] - avg);
        } else {
            sum += (avg- tsc_measurements[i]) * (avg - tsc_measurements[i]);
        }
    }

    sum /= count;

    printf("%s %3lu, dst=%u, avg=%lu, stdev=%lu, med=%lu, min=%lu, max=%lu cycles, count=%lu, ignored=%lu\n",
            glb_label, arg->num_msg, arg->r, avg, (cycles_t)sqrt(sum),sorted[count/2], min, max, count, NUM_EXP - count);

    return NULL;
}

void* thr_receiver_one(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;
    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    struct smlt_qp *qp = queue_pairs[1][arg->r];

    for (size_t j=0; j<NUM_EXP; j++) {
        for (size_t i = 0; i < arg->num_msg; ++i) {
            smlt_queuepair_recv(qp, msg);
        }
        smlt_queuepair_send(qp, msg);

    }

    return NULL;
}

int main(int argc, char **argv)
{
    errval_t err;
    coreid_t num_cores = (coreid_t) sysconf(_SC_NPROCESSORS_CONF) / 2;
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

    queue_pairs[0] = calloc(NUM_CHANNELS, sizeof(void *));
    queue_pairs[1] = calloc(NUM_CHANNELS, sizeof(void *));
    if (queue_pairs[0] == NULL || queue_pairs[1] == NULL) {
        printf("FAILED TO INITIALIZE calloc!\n");
        return -1;
    }

    coreid_t core = 1;

    chan_per_core = (NUM_CHANNELS / (num_cores - 1));
    size_t chan_max = chan_per_core * (num_cores - 1);
    for (size_t s=0; s<chan_max; s++) {
        struct smlt_qp **src = &(queue_pairs[0][s]);
        struct smlt_qp **dst = &(queue_pairs[1][s]);

        err = smlt_queuepair_create(SMLT_QP_TYPE_UMP,
                                    src, dst, 0, core);
        if (smlt_err_is_fail(err)) {
            printf("FAILED TO INITIALIZE queuepair! for core %u\n", core);
            return -1;
        }

        if (++core == num_cores) {
            core = 1;
        }
    }

    smlt_platform_pin_thread(0);

    struct thr_args args[num_cores];

    coreid_t num_local_cores = smlt_platform_num_cores_of_cluster(0) / 2;

    printf("=============================================\n");

    glb_label = "WRITEL";

    /* local polling */
    for (coreid_t core = 1; core < num_local_cores; ++core) {
        for (coreid_t i = 1; i <= core; ++i) {
            struct smlt_node *dst = smlt_get_node_by_id(i);
            args[i].s = 0;
            args[i].r = i-1;
            args[i].num_channels = 1;
            args[i].num_cores = core;

            err = smlt_node_start(dst, thr_receiver, args + i);
            if (smlt_err_is_fail(err)) {
                printf("Staring node failed \n");
            }
        }

        args[0].s = 0;
        args[0].r = 0;
        args[0].num_channels = core;
        args[0].num_cores = core;

        thr_write(args);

        for (coreid_t i = 1; i <= core; ++i) {
            struct smlt_node *dst = smlt_get_node_by_id(i);
            smlt_node_join(dst);
        }
    }

    printf("=============================================\n");

    glb_label = "WRITEN";

    for (uint32_t c = 1; c < smlt_platform_num_clusters(); ++c) {
        for (coreid_t i = 1; i <= c; ++i) {
            coreid_t target = num_local_cores * i;
            struct smlt_node *dst = smlt_get_node_by_id(target);
            args[target].s = 0;
            args[target].r = target;
            args[target].num_channels = 1;
            args[target].num_cores = c;

            err = smlt_node_start(dst, thr_receiver, args + target);
            if (smlt_err_is_fail(err)) {
                printf("Staring node failed \n");
            }
        }

        args[0].s = 0;
        args[0].r = num_local_cores;
        args[0].num_channels = c;
        args[0].num_cores = c;

        sleep(1);
        thr_write(args);

        for (coreid_t i = 1; i <= c; ++i) {
            coreid_t target = num_local_cores * i;
            struct smlt_node *dst = smlt_get_node_by_id(target);
            smlt_node_join(dst);
        }
    }

    printf("=============================================\n");

    glb_label = "WRITES";

    for (uint32_t c = 0; c < smlt_platform_num_clusters(); ++c) {
            coreid_t target = num_local_cores * c;
            if (target == 0) target = 1;
            struct smlt_node *dst = smlt_get_node_by_id(target);
         for (uint32_t num_msg = 1; num_msg <= 16; ++num_msg) {
            args[target].s = 0;
            args[target].r = target;
            args[target].num_channels = 1;
            args[target].num_cores = c;
            args[target].num_msg = num_msg;

            err = smlt_node_start(dst, thr_receiver_one, args + target);
            if (smlt_err_is_fail(err)) {
                printf("Staring node failed \n");
            }

        args[0].s = 0;
        args[0].r = target;
        args[0].num_channels = c;
        args[0].num_cores = c;
        args[0].num_msg = num_msg;

        sleep(1);
        thr_write_one(args);

            smlt_node_join(dst);
        }
    }


    printf("=============================================\n");

    glb_label = "WRITEX";

    for (size_t r=1; r<num_cores - num_local_cores;  ++r) {
        for (coreid_t core = 0; core < r; ++core) {
            //struct smlt_node *src = smlt_get_node_by_id(0);i
            coreid_t target = num_local_cores + core;
            struct smlt_node *dst = smlt_get_node_by_id(target);

            args[target].s = 0;
            args[target].r = target - 1;
            args[target].num_channels = 1;
            args[target].num_cores = r;

            err = smlt_node_start(dst, thr_receiver, args + target);
            if (smlt_err_is_fail(err)) {
                printf("Staring node failed \n");
            }
        }

        args[0].s = num_local_cores-1;
        args[0].r = 0;
        args[0].num_channels = r;
        args[0].num_cores = r -1;


        sleep(1);
        thr_write(args);

        // Wait for sender to complete
        for (coreid_t core = 0; core < r; ++core) {
            //struct smlt_node *src = smlt_get_node_by_id(0);
            coreid_t target = num_local_cores +  core;
            struct smlt_node *dst = smlt_get_node_by_id(target);

            smlt_node_join(dst);
        }
    }


    printf("=============================================\n");

    glb_label = "WRITE";

    for (size_t r=1; r<num_cores;  ++r) {
        for (coreid_t core = 1; core <= r; ++core) {
            struct smlt_node *dst = smlt_get_node_by_id(core);

            args[core].s = 0;
            args[core].r = core - 1;
            args[core].num_channels = 1;
            args[core].num_cores = r;

            err = smlt_node_start(dst, thr_receiver, args + core);
            if (smlt_err_is_fail(err)) {
                printf("Staring node failed \n");
            }
        }

        args[0].s = 0;
        args[0].r = 0;
        args[0].num_channels = r;
        args[0].num_cores = r -1;


        sleep(1);
        thr_write(args);

        // Wait for sender to complete
        for (coreid_t core = 1; core <= r; ++core) {
            //struct smlt_node *src = smlt_get_node_by_id(0);
            struct smlt_node *dst = smlt_get_node_by_id(core);
            smlt_node_join(dst);
        }
    }
}
