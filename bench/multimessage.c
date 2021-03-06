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
#include "smlt_debug.h"

#include <platforms/measurement_framework.h>

/**
 * \brief 2x#cpus matrix - stores full mesh of queuepairs from core 0
 * to all other cores
 *
 * [0] -> Forward channels
 * [1] -> Backward channels
 */
struct smlt_qp **queue_pairs[2]; // WHY two?

/** Array storing how often a core/thread has to receive messages */
static size_t *n_p_core;

size_t chan_per_core = 0;
cycles_t tsc_overhead = 0;

#define NUM_CHANNELS 1024

#define NUM_THREADS 1

#define NUM_DATA 100
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

struct thr_args {
    /**
     * \brief Pointers to queue pairs
     *
     * Sender:
     */
    struct smlt_qp **queue_pairs; // Only used for sender thread
    coreid_t *cores;
    uint32_t num_cores;
    enum mode mode;
};

static size_t parse_cores(const char* str,
                          coreid_t cores_max,
                          coreid_t *_cores,
                          size_t *_n_p_core)
{
    if (strcmp(str, "none")==0) {
        printf("none given, setting to 0 cores\n");
        return 0;
    }

    printf("cores: ");
    size_t idx = 0;
    const char * tmp = str;
    do {
        size_t l = strcspn (tmp, ",");
        printf("%.*s ", (int) l, tmp);

        coreid_t curr_core = atoi(tmp);
        assert (curr_core<cores_max);

        _cores[idx++] = curr_core;
        _n_p_core[curr_core] += 1;

        tmp += l + 1;

    } while (tmp[-1] && idx<cores_max);
    printf("\n");

    // Make sure we reached the end of the comma separated list when
    // leaving the loop, and not because we are running out of spaces
    // in the array of cores. If the latter is the case, the argument
    // contains more cores tain evailable in the system.
    if (tmp[-1]) {
        printf("Too many cores given\n"); abort();
    }

    return idx;
}

void* thr_write(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    cycles_t tsc_start;

    uint32_t nc = arg->num_cores;

    for (size_t iter=0; iter<NUM_EXP; iter++) {

        // Wait a while to ensure that receivers already polled the
        // line, so that it is in shared state
        //
        // Also, ensure that the write-buffer is empty
        tsc_start = bench_tsc();
        while (bench_tsc()<tsc_start + 1000) ; // wait 100'000 cycles

        // Array of queue pairs we have to send to
        struct smlt_qp **qp = arg->queue_pairs;

        struct sk_measurement skm;
        char title[1000];

        int num =snprintf(title, 1000, "mm-%s", modestring[arg->mode]);

        char *tmp = title + num;
        size_t max = 1000 - num;

        for (coreid_t i = 0; i <arg->num_cores; ++i) {

            num = snprintf(tmp, max, "-%d", arg->cores[i]);
            tmp += num;
            max -= num;
        }


        sk_m_init(&skm, NUM_DATA, title, tsc_measurements);

        switch (arg->mode) {
        case BENCH_MODE_LAST:
            // Measure cost of LAST message (includes one TSC)
            // --------------------------------------------------
            // Send n-1 messages
            for (uint32_t i = 0; i < nc-1; ++i) {
                smlt_queuepair_send(*qp, msg);
                qp++;
            }
            sk_m_restart_tsc(&skm);
            smlt_queuepair_send(*qp, msg);  // send last message
            sk_m_add(&skm);
            break;
        case BENCH_MODE_TOTAL:
            // Measure total time of the batch
            // --------------------------------------------------
            sk_m_restart_tsc(&skm);
            for (uint32_t i = 0; i < nc; ++i) {
                smlt_queuepair_send(*qp, msg);
                qp++;
            }
            sk_m_add(&skm);
            break;
        case BENCH_MODE_ALL:
            // Measure cost of last message, but have one rdtsc()
            // between each of them
            // --------------------------------------------------
            sk_m_restart_tsc(&skm);
            for (uint32_t i = 0; i < nc; ++i) {
                tsc_start = bench_tsc();
                smlt_queuepair_send(*qp, msg);
                qp++;
            }
            sk_m_add(&skm);
            break;
        default:
            printf("ERROR: UNKNOWN MODE\n");
        }
        sk_m_print(&skm);
    }

    return NULL;
}



void* thr_receiver(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;

    assert (arg->num_cores == 1); // Receivers only need one core ID (sender)
    assert (arg->queue_pairs == NULL); // Not used, queuepair lookup
                                       // based on core ID given

    coreid_t tid = arg->cores[0];

    struct smlt_msg* msg = smlt_message_alloc(8);
    msg->words = 0;

    // Fetch Backward queue pair
    dbg_printf("Fetching backward queue pair for %" PRIuCOREID "\n", tid);
    struct smlt_qp *qp = queue_pairs[1][arg->cores[0]];


    assert (n_p_core[tid] != 0);
    printf("core %" PRIuCOREID " receives %zu times\n", tid, n_p_core[tid]);
    size_t num_rep = NUM_EXP * n_p_core[tid];

    for (size_t j=0; j<num_rep; j++) {

        smlt_queuepair_recv(qp, msg);
    }

    return NULL;
}


/**
 * \brief Run experiments
 *
 * \param num_local: Number of local cores and size of array cores
 * \param local cores: List of local cores to send a message to
 * \param num_remote: Number of remote cores and size of array cores
 * \param remote cores: List of remote cores to send a message to
 * \param m: Mode of measurement - one of enum mode
 */
int run_experiment(size_t num_local, coreid_t* local_cores,
                   size_t num_remote, coreid_t* remote_cores, enum mode m)
{
    printf("Running experiment for %zu %zu cores\n", num_local, num_remote);

    coreid_t num_cores = num_local + num_remote;

    errval_t err;
    struct thr_args args[num_cores + 1];

    // Initialize queue pairs for sender
    args[0].queue_pairs = (struct smlt_qp **) calloc(num_cores, sizeof(void *));
    assert (args[0].queue_pairs);

    // Determine the cores to run in this iteration
    coreid_t cores[num_cores]; size_t idx = 0;
    for (coreid_t r = 0; r < num_remote; r++) { // remote first
        cores[idx++] = remote_cores[r];
        dbg_printf("adding remote core %" PRIuCOREID " %" PRIuCOREID "\n",
                   r, remote_cores[r]);
    }
    for (coreid_t l = 0; l < num_local; l++) { // followed by local
        cores[idx++] = local_cores[l];
        dbg_printf("adding local core %" PRIuCOREID " %" PRIuCOREID "\n",
                   l, local_cores[l]);
    }

    // Copy preinitialized queue pairs
    for (size_t i=0; i<num_cores; i++) {

        // Get forward channel to ith core given as argument
        dbg_printf("Setting forward channel %" PRIuCOREID "\n", cores[i]);
        assert (queue_pairs[0][cores[i]] != NULL);
        args[0].queue_pairs[i] = queue_pairs[0][cores[i]];
    }

    size_t c_max = sysconf(_SC_NPROCESSORS_CONF);

    // Reset array
    memset(n_p_core, 0, sizeof(size_t)*c_max);
    for (coreid_t i = 0; i < num_cores; ++i) {
        n_p_core[cores[i]] += 1;
    }

    // Start receiver threads
    int already_started[c_max]; // initialize to zero
    memset(already_started, 0, sizeof(int)*(c_max));
    for (coreid_t i = 0; i < num_cores; ++i) {

        // Start only one thread per receiver core
        if (already_started[cores[i]])
            continue;

        already_started[cores[i]] = 1;
        printf("Starting receiver on core %d - num %zu\n",
               cores[i], n_p_core[cores[i]]);

        struct smlt_node *dst = smlt_get_node_by_id(cores[i]);
        struct thr_args *a = args + (i+1); // args[0] is sender

        if (!dst) panic("smlt_get_node_by_id failed");

        a->cores = cores + i;
        a->num_cores = 1;
        a->queue_pairs = NULL; // Receiver fetches queuepair
                               // depending on core ID
        err = smlt_node_start(dst, thr_receiver, a);
        if (smlt_err_is_fail(err)) {
            printf("Staring node failed \n");
            abort();
        }
    }

    // Start sender thread
    args[0].num_cores = num_cores;
    args[0].cores = cores;
    args[0].mode = m;

    assert (args[0].queue_pairs != NULL);
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
    if (argc<4) {

        printf("Usage: %s <sender> <local_cores> <remote_cores>\n", argv[0]);
        printf("\n");
        printf("sender core: sender of the batch\n");
        printf("local cores: comma-separate list of local receiving cores\n");
        printf("remote cores: comma-separate list of remote receiving cores\n");

        return 1;
    }

    size_t cores_max = sysconf(_SC_NPROCESSORS_CONF);

    coreid_t local_cores[cores_max];
    coreid_t remote_cores[cores_max];
    n_p_core = malloc(sizeof(size_t)*cores_max);
    COND_PANIC(n_p_core!=NULL, "n_p_core alloc failed");
    memset(n_p_core, 0, sizeof(size_t)*cores_max);

    // Parse cores
    coreid_t sender = atoi(argv[1]);
    size_t num_local  = parse_cores(argv[2], cores_max, local_cores, n_p_core);
    size_t num_remote = parse_cores(argv[3], cores_max, remote_cores, n_p_core);
    size_t num_cores = num_local + num_remote;

    // Parse arguments
    printf("sender: %" PRIuCOREID "\n", sender);
    printf("num_cores: local=%zu remote=%zu\n", num_local, num_remote);

    // Debug output of cores
    for (coreid_t c = 0; c<num_local; c++) {
        printf("Core %" PRIuCOREID ": %" PRIuCOREID " num=%zu\n",
               c, local_cores[c], n_p_core[local_cores[c]]);
    }
    for (coreid_t c = 0; c<num_remote; c++) {
        printf("Core %" PRIuCOREID ": %" PRIuCOREID " num=%zu\n",
               c, remote_cores[c], n_p_core[remote_cores[c]]);
    }

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
        bench_tsc(); //5
        bench_tsc();
        bench_tsc();
        bench_tsc();
        bench_tsc();
        bench_tsc(); //10
    }
    cycles_t tsc_end = bench_tsc();

    tsc_overhead = (tsc_end - tsc_start) / (TSC_ROUNDS * 10);

    printf("Calibrating TSC overhead is %lu cycles\n", tsc_overhead);

    // Initalize Smelt
    err = smlt_init(cores_max, true);
    if (smlt_err_is_fail(err)) {
        printf("FAILED TO INITIALIZE smlt !\n");
        return 1;
    }

    queue_pairs[0] = (struct smlt_qp**) calloc(cores_max, sizeof(void *));
    queue_pairs[1] = (struct smlt_qp**) calloc(cores_max, sizeof(void *));

    if (queue_pairs[0] == NULL || queue_pairs[1] == NULL) {
        printf("FAILED TO INITIALIZE calloc!\n");
        abort();
    }

    // Initialize message channels. One channel from the master thread
    // to each other core: full mesh
    for (size_t s=0; s<cores_max; s++) {
        struct smlt_qp **src = &(queue_pairs[0][s]);
        struct smlt_qp **dst = &(queue_pairs[1][s]);

        err = smlt_queuepair_create(SMLT_QP_TYPE_UMP,
                                    src, dst, 0, s);
        if (smlt_err_is_fail(err)) {
            printf("FAILED TO INITIALIZE queuepair! for core %lu\n", s);
            abort();
        }
    }

    smlt_platform_pin_thread(sender); // << pin master thread

    // Execute benchmark
    for (coreid_t n_local = 0; n_local <= num_local; n_local++) {
        for (coreid_t n_remote = 0; n_remote <= num_remote; n_remote++) {

            if (n_local==0 && n_remote==0) continue;

            memset(tsc_measurements, 0, sizeof(tsc_measurements));
            run_experiment(n_local, local_cores,
                           n_remote, remote_cores, BENCH_MODE_TOTAL);

            memset(tsc_measurements, 0, sizeof(tsc_measurements));
            run_experiment(n_local, local_cores,
                           n_remote, remote_cores, BENCH_MODE_LAST);

            memset(tsc_measurements, 0, sizeof(tsc_measurements));
            run_experiment(n_local, local_cores,
                           n_remote, remote_cores, BENCH_MODE_ALL);
        }
    }

    free(n_p_core);

    return 0;

}
