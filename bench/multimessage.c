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

#define NUM_THREADS 1

#define NUM_DATA 2000
#define NUM_EXP 5000
#define NUM_WARMUP 5000



#define STR(X) #X

char *glb_label = NULL;

cycles_t tsc_measurements[NUM_DATA];

//#define bench_tsc() rdtscp()

struct thr_args {
    struct smlt_qp **queue_pairs;
    coreid_t *cores;
    uint32_t num_cores;
    uint32_t num_local;
    uint32_t num_remote;
    bool just_last;
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

    printf("r=%u,l=%u,l=%u, avg=%lu, stdev=%lu, med=%lu, min=%lu, max=%lu cycles, count=%lu, ignored=%lu\n",
            arg->num_remote, arg->num_local, arg->just_last, avg, (cycles_t)sqrt(sum),sorted[count/2], min, max, count, NUM_EXP - count);


}

void* thr_write(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    cycles_t tsc_start, tsc_end;

    uint32_t nc = arg->num_cores;

    for (size_t i=0; i<NUM_EXP; i++) {
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
        if (arg->just_last) {
            uint32_t i;
            for (i = 1; i < nc; ++i) {
                smlt_queuepair_send(*qp, msg);
                qp++;
            }
            tsc_start = bench_tsc();
            smlt_queuepair_send(*qp, msg);

            tsc_end = bench_tsc();
            tsc_measurements[i % NUM_DATA] = (tsc_end - tsc_start);
        } else {
            tsc_start = bench_tsc();
            for (uint32_t i = 0; i < nc; ++i) {
                tsc_start = bench_tsc(); // update TSC - measure only LAST send
                smlt_queuepair_send(*qp, msg);
                qp++;
            }
            tsc_end = bench_tsc();
            tsc_measurements[i % NUM_DATA] = (tsc_end - tsc_start);
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
    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    //printf("receiver: queue_pairs[1][%u]\n", arg->cores[0]);

    for (size_t j=0; j<NUM_EXP; j++) {
        struct smlt_qp *qp = queue_pairs[1][arg->cores[0]];
        smlt_queuepair_recv(qp, msg);

        smlt_queuepair_send(qp, msg);
    }

    return NULL;
}


int run_experiment(uint32_t num_local, uint32_t num_remote, bool just_last, bool print)
{
    errval_t err;

    //printf("run_experiment(nl=%u, nr=%u)\n", num_local,num_remote);

    struct thr_args args[num_local + num_remote + 1];

    args[0].queue_pairs = calloc(num_local + num_remote, sizeof(void *));

    coreid_t *cores = calloc(num_local + num_remote, sizeof(coreid_t));
    uint32_t idx = 0;

    coreid_t *cluster_cores;
    uint32_t num_cores;

    if (print) printf("# remote cores: ");
    for (uint32_t i = 1; i <= num_remote; ++i) {
        /* */
        err = smlt_platform_cores_of_cluster(i, &cluster_cores, &num_cores);
        if (smlt_err_is_fail(err)) { printf("error getting cores of cluster\n");}
        args[0].queue_pairs[idx] = queue_pairs[0][cluster_cores[0]];

        cores[idx++] = cluster_cores[0];
        if (print)  printf("%u ", cluster_cores[0]);

        free(cluster_cores);
    }

    if (print) printf("\tlocal cores: ");
    err = smlt_platform_cores_of_cluster(0, &cluster_cores, &num_cores);
    if (smlt_err_is_fail(err)) { printf("error getting cores of cluster\n");}

    for (uint32_t i = 1; i <= num_local; ++i) {
        args[0].queue_pairs[idx] = queue_pairs[0][cluster_cores[i]];
        cores[idx++] = cluster_cores[i];
        if (print) printf("%u ", cluster_cores[i]);
    }
    free(cluster_cores);
    if (print) printf("\n");


    for (coreid_t i = 0; i < num_local + num_remote; ++i) {


        struct smlt_node *dst = smlt_get_node_by_id(cores[i]);
        args[i].cores = cores + i;
        args[i].num_cores = 1;
        err = smlt_node_start(dst, thr_receiver, args + i);
        if (smlt_err_is_fail(err)) {
            printf("Staring node failed \n");
        }
    }

    args[0].num_cores = num_local + num_remote;
    args[0].cores = cores;
    args[0].num_local = num_local;
    args[0].num_remote = num_remote;
    args[0].just_last = just_last;

    thr_write(args);

    for (coreid_t i = 0; i < num_local + num_remote; ++i) {
        struct smlt_node *dst = smlt_get_node_by_id(cores[i]);
        smlt_node_join(dst);
    }

    free(cores);
    free(args[0].queue_pairs);

    return 0;
}


int main(int argc, char **argv)
{
    errval_t err;
    coreid_t num_cores = (coreid_t) sysconf(_SC_NPROCESSORS_CONF) / NUM_THREADS;
    printf("Running with %d cores\n", num_cores);

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

    queue_pairs[0] = calloc(num_cores, sizeof(void *));
    queue_pairs[1] = calloc(num_cores, sizeof(void *));
    if (queue_pairs[0] == NULL || queue_pairs[1] == NULL) {
        printf("FAILED TO INITIALIZE calloc!\n");
        return -1;
    }

    glb_label = "WRITEN";

    for (size_t s=0; s<num_cores; s++) {
        struct smlt_qp **src = &(queue_pairs[0][s]);
        struct smlt_qp **dst = &(queue_pairs[1][s]);

        err = smlt_queuepair_create(SMLT_QP_TYPE_UMP,
                                    src, dst, 0, s);
        if (smlt_err_is_fail(err)) {
            printf("FAILED TO INITIALIZE queuepair! for core %lu\n", s);
            return -1;
        }
    }

    smlt_platform_pin_thread(0);

    coreid_t num_local_cores = smlt_platform_num_cores_of_cluster(0) / NUM_THREADS;
    uint32_t num_cluster = smlt_platform_num_clusters();

    printf("num_local_cores=%u, num_cluster=%u\n", num_local_cores, num_cluster);

    printf("=============================================\n");


    for (coreid_t nr = 0; nr < num_cluster; ++nr) {
        for (coreid_t nl = 0; nl < num_local_cores; ++nl) {
            if (nl == 0 && nr == 0) {
                continue;
            }
            printf("\n\n--------------\n\n");
            run_experiment(nl, nr, false, true);
            run_experiment(nl, nr, true, false);
        }
    }



    printf("=============================================\n");



}
