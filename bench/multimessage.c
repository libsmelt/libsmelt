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
#include <string.h>
#include <unistd.h>
#include <smlt.h>
#include <smlt_node.h>
#include <smlt_queuepair.h>

#include <platforms/measurement_framework.h>

struct smlt_qp **queue_pairs[2];
size_t chan_per_core = 0;
cycles_t tsc_overhead = 0;

#define NUM_CHANNELS 1024

#define NUM_THREADS 1

#define NUM_DATA 2000
#define NUM_EXP 5000
#define NUM_WARMUP 5000


enum mode {
    BENCH_MODE_TOTAL=0,
    BENCH_MODE_LAST=1,
    BENCH_MODE_ALL=2,
    BENCH_MODE_MAX=3
};

const char *modestring [4] = {
    "sum ",
    "last",
    "all ",
    "sumA"
};

#define STR(X) #X

cycles_t tsc_measurements[NUM_DATA];

//#define bench_tsc() rdtscp()

struct thr_args {
    struct smlt_qp **queue_pairs;
    coreid_t *cores;
    uint32_t num_cores;
    enum mode mode;
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

static void eval_data(struct thr_args*arg)
{
    cycles_t sum = 0, max = 0, min = (cycles_t)-1;
    size_t count = 0;

    cycles_t *sorted = do_sorting(tsc_measurements, NUM_DATA);

    for (size_t i=0; i<(NUM_DATA * 0.90); i++) {
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

    printf("r=%u,l=%u, mode=%s, avg=%lu, stdev=%lu, med=%lu, min=%lu, max=%lu cycles, count=%lu, ignored=%lu\n",
           0, 0, modestring[arg->mode], avg, (cycles_t)sqrt(sum),sorted[count/2], min, max, count, NUM_EXP - count);


}

void* thr_write(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    cycles_t tsc_start, tsc_end;

    uint32_t nc = arg->num_cores;

    for (size_t iter=0; iter<NUM_EXP; iter++) {

        // Wait a while to ensure that receivers already polled the
        // line, so that it is in shared state
        assert (tsc_overhead>0);
        for (size_t i=0;; i++) {
            bench_tsc(); // Waste some time
            if (i*tsc_overhead>1000) {
                break;
            }
        }

        struct smlt_qp **qp = arg->queue_pairs;
        switch (arg->mode) {
        case BENCH_MODE_LAST :
            for (uint32_t i = 1; i < nc; ++i) {
                smlt_queuepair_send(*qp, msg);
                qp++;
            }
            tsc_start = bench_tsc();
            smlt_queuepair_send(*qp, msg);
            tsc_end = bench_tsc();
            tsc_measurements[iter % NUM_DATA] = (tsc_end - tsc_start);
            break;
        case BENCH_MODE_TOTAL :
            tsc_start = bench_tsc();
            for (uint32_t i = 0; i < nc; ++i) {
                smlt_queuepair_send(*qp, msg);
                qp++;
            }
            tsc_end = bench_tsc();
            tsc_measurements[iter % NUM_DATA] = (tsc_end - tsc_start);
            break;
        case BENCH_MODE_ALL :
            tsc_start = bench_tsc();
            for (uint32_t i = 0; i < nc; ++i) {
                tsc_start = bench_tsc();
                smlt_queuepair_send(*qp, msg);
                qp++;
            }
            tsc_end = bench_tsc();
            tsc_measurements[iter % NUM_DATA] = (tsc_end - tsc_start);
            break;
        default:
            printf("ERROR: UNKNOWN MODE\n");
        }

        qp = arg->queue_pairs;
        for (uint32_t i = 0; i < nc; ++i) {
            smlt_queuepair_recv(*qp, msg);
            qp++;
        }
    }

    eval_data(arg);

    return NULL;
}



void* thr_receiver(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    assert (arg->num_cores == 1); // Receivers only need one core ID (sender)

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    struct smlt_qp *qp = queue_pairs[1][arg->cores[0]];

    for (size_t j=0; j<NUM_EXP; j++) {

        smlt_queuepair_recv(qp, msg);
        smlt_queuepair_send(qp, msg);
    }

    return NULL;
}


/**
 * \brief Run experiments
 *
 * \param num_cores: Number of cores and size of array cores
 * \param cores: List of cores to send a message to
 * \param m: Mode of measurement - one of enum mode
 */
int run_experiment(size_t num_cores, coreid_t* cores, enum mode m)
{
    errval_t err;
    struct thr_args args[num_cores + 1];

    args[0].queue_pairs = (struct smlt_qp **) calloc(num_cores, sizeof(void *));
    assert (args[0].queue_pairs);

    // Copy preinitialized queue pairs
    for (size_t i=0; i<num_cores; i++) {
        args[0].queue_pairs[i] = queue_pairs[0][cores[i]];
    }


    // Start receiver threads
    for (coreid_t i = 0; i < num_cores; ++i) {

        struct smlt_node *dst = smlt_get_node_by_id(cores[i]);
        args[i].cores = cores + i;
        args[i].num_cores = 1;
        err = smlt_node_start(dst, thr_receiver, args + i);
        if (smlt_err_is_fail(err)) {
            printf("Staring node failed \n");
            abort();
        }
    }

    // Start sender thread
    args[0].num_cores = num_cores;
    args[0].cores = cores;
    args[0].mode = m;

    thr_write(args);

    // Join threads
    for (coreid_t i = 0; i < num_cores; ++i) {
        struct smlt_node *dst = smlt_get_node_by_id(cores[i]);
        smlt_node_join(dst);
    }

    free(args[0].queue_pairs);

    return 0;
}

int main(int argc, char **argv)
{
    if (argc<2) {

        printf("Usage: %s <num_cores> <cores>\n", argv[0]);
        printf("\n");
        printf("cores: comma-separate list of receiving cores\n");

        return 1;
    }

    size_t cores_max = sysconf(_SC_NPROCESSORS_CONF);

    coreid_t cores[cores_max];

    // Parse cores
    printf("cores: ");
    size_t idx = 0;
    char * tmp = (char *) argv[1];
    do {
        size_t l = strcspn (tmp, ",");
        printf("%.*s ", (int) l, tmp);
        tmp += l + 1;

        cores[idx++] = atoi(tmp);
        assert (atoi(tmp)<cores_max);

    } while (tmp[-1] && idx<cores_max);
    printf("\n");

    // Make sure we reached the end of the comma separated list when
    // leaving the loop, and not because we are running out of spaces
    // in the array of cores. If the latter is the case, the argument
    // contains more cores tain evailable in the system.
    if (tmp[-1]) {
        printf("Too many cores given\n"); abort();
    }

    // Parse arguments
    size_t num_cores = idx;
    printf("num_cores: %zu\n", num_cores);

    errval_t err;
    if (num_cores>cores_max) {

        printf("Machine does not have enough cores: %zu %zu\n",
               num_cores, cores_max);
        abort();
    }

    printf("Running with %zu cores\n", num_cores);

    // Calibrate TSC overhead
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

    // Initalize Smelt
    err = smlt_init(num_cores, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE smlt !\n");
        return 1;
    }

    queue_pairs[0] = (struct smlt_qp**) calloc(num_cores, sizeof(void *));
    queue_pairs[1] = (struct smlt_qp**) calloc(num_cores, sizeof(void *));

    if (queue_pairs[0] == NULL || queue_pairs[1] == NULL) {
        printf("FAILED TO INITIALIZE calloc!\n");
        abort();
    }

    // Initialize message channels. One channel from the master thread
    // to each other core
    for (size_t s=0; s<num_cores; s++) {
        struct smlt_qp **src = &(queue_pairs[0][s]);
        struct smlt_qp **dst = &(queue_pairs[1][s]);

        err = smlt_queuepair_create(SMLT_QP_TYPE_UMP,
                                    src, dst, 0, s);
        if (smlt_err_is_fail(err)) {
            printf("FAILED TO INITIALIZE queuepair! for core %lu\n", s);
            abort();
        }
    }

    smlt_platform_pin_thread(0); // << pin master thread

    // Execute benchmark
    for (coreid_t i = 0; i < num_cores; ++i) {

        memset(tsc_measurements, 0, sizeof(tsc_measurements));
        run_experiment(i, cores, BENCH_MODE_TOTAL);

        memset(tsc_measurements, 0, sizeof(tsc_measurements));
        run_experiment(i, cores, BENCH_MODE_LAST);

        memset(tsc_measurements, 0, sizeof(tsc_measurements));
        run_experiment(i, cores, BENCH_MODE_ALL);
    }

    return 0;

}
