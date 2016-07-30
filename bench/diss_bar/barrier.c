/*
 * broadcast.c
 *
 *  Created on: 4 Dec, 2012
 *      Author: sabela
 */


#include <pthread.h>
#include <numa.h>
#include <stdbool.h>
#include "threads.h"
#include "mcs.h"


#include <smlt.h>
#include <smlt_generator.h>
#include <smlt_topology.h>
#include <smlt_context.h>
#include <smlt_node.h>
struct smlt_context* context;
struct smlt_dissem_barrier* bar;
mcs_barrier_t* bar_mcs;

static uint32_t* placement(uint32_t n, bool do_fill, bool hyperthreads)
{
    uint32_t* result = (uint32_t*) malloc(sizeof(uint32_t)*n);
    uint32_t numa_nodes = numa_max_node()+1;
    uint32_t num_cores = 0;
    if (hyperthreads){
        num_cores = numa_num_configured_cpus()/2;
    } else {
        num_cores = numa_num_configured_cpus();
    }
    struct bitmask* nodes[numa_nodes];

    for (int i = 0; i < numa_nodes; i++) {
        nodes[i] = numa_allocate_cpumask();
        numa_node_to_cpus(i, nodes[i]);
    }

    int num_taken = 0;
    if (numa_available() == 0) {
        if (do_fill) {
            for (int i = 0; i < numa_nodes; i++) {
                for (int j = 0; j < num_cores; j++) {
                    if (numa_bitmask_isbitset(nodes[i], j)) {
                        result[num_taken] = j;
                        num_taken++;
                    }

                    if (num_taken == n) {
                        return result;
                    }
                }
           }
        } else {
            int cores_per_node = n/numa_nodes;
            int rest = n - (cores_per_node*numa_nodes);
            int taken_per_node = 0;

            fprintf(stderr, "Cores per node %d \n", cores_per_node);
            fprintf(stderr, "rest %d \n", rest);
            for (int i = 0; i < numa_nodes; i++) {
                for (int j = 0; j < num_cores; j++) {
                    if (numa_bitmask_isbitset(nodes[i], j)) {
                        if (taken_per_node == cores_per_node) {
                            if (rest > 0) {
                                result[num_taken] = j;
                                num_taken++;
                                rest--;
                                if (num_taken == n) {
                                    return result;
                                }
                            }
                            break;
                        }
                        result[num_taken] = j;
                        num_taken++;
                        taken_per_node++;

                        if (num_taken == n) {
                            return result;
                        }
                    }
                }
                taken_per_node = 0;
            }
        }
    } else {
        printf("Libnuma not available \n");
        return NULL;
    }
    return NULL;
}

void run_diss_tp(bool fill, bool hyperthreads) {

	sharedMemory_t * shm;
	threaddata_t * threaddata;
	pthread_t * threads;
	UINT64_T g_timerfreq;
	UINT64_T * rdtsc_synchro;
	int rdtsc_latency;
    int num_threads = 0;
    if (hyperthreads) {
	    num_threads = sysconf(_SC_NPROCESSORS_ONLN)/2;
    } else {
	    num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    }
    int naccesses; //for no use in barrier, inherited from the bcast benchmark.
    int accesses;
    uint32_t* cores;
    int n_thr;
    n_thr = num_threads;
    if (fill) {
        fprintf(stderr, "Running Dissemination %d fill\n", n_thr);
    } else {
        fprintf(stderr, "Running Dissemination %d rr\n", n_thr);
    }

    if(posix_memalign((void**)(&(shm)), ALIGNMENT,
                      sizeof(sharedMemory_t))){
        //returns 0 on success
        fprintf(stderr, "Error allocation of shared memory structure failed\n");
        fflush(stderr);
        exit(127);
    }

    if(posix_memalign((void**)(&threads),
                      ALIGNMENT,n_thr*sizeof(pthread_t))){
        //returns 0 on success
        fprintf(stderr, "Error allocation of threads failed\n");
        fflush(stderr);
        exit(127);
    }

    if(posix_memalign((void**)(&(threaddata)), ALIGNMENT,
                      n_thr*sizeof(threaddata_t))){
        //returns 0 on success
        fprintf(stderr, "Error allocation of thread data structure failed\n");
        fflush(stderr);
        exit(127);
    }

    initSharedMemory(shm, n_thr);

    cores = placement(n_thr, fill, hyperthreads);

    g_timerfreq = 0;      // RH: the are variables that should be cleaned up 
                          //     and are not used anymore since we do dnot synchro
                          //     based on rdtsc but on a dissemination barrier...
    rdtsc_synchro = NULL; // SK: can somebody verify this?
    rdtsc_latency = 0;    // SK: can somebody verify this?
    naccesses = 0;        // SK: can somebody verify this?
    initThreads(n_thr, threaddata, shm, g_timerfreq, rdtsc_synchro,
                rdtsc_latency, naccesses, &accesses, shm->m,
                shm->rounds, cores, fill, false);

    for(int t=0 ; t < n_thr; t++){
        pthread_create(&threads[t],NULL,function_thread_tp,
                       (void *)(&(threaddata[t])));
        while (!threaddata[t].ack); // initialized thread
    }

    for (int t=0; t < n_thr; t++) {
        fprintf(stderr, "Core[%d]=%d\n", t, cores[t]);
    }

    //let them start
    for(int t=0; t < n_thr; t++)
        threaddata[t].ack = 0;

    // wait for them to finish

    for (int t=0; t < n_thr; t++){
        pthread_join(threads[t],NULL);
    }

    free(threads);
    free(threaddata);
    free(shm);
}


void run_diss(bool fill, bool hyperthreads) {

	sharedMemory_t * shm;
	threaddata_t * threaddata;
	pthread_t * threads;
	UINT64_T g_timerfreq;
	UINT64_T * rdtsc_synchro;
	int rdtsc_latency;
    int num_threads = 0;
    if (hyperthreads) {
	    num_threads = sysconf(_SC_NPROCESSORS_ONLN)/2;
    } else {
	    num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    }
    int naccesses; //for no use in barrier, inherited from the bcast benchmark.
    int accesses;
    uint32_t* cores;

    for (int n_thr = 2; n_thr < num_threads+1; n_thr++) {

        if (fill) {
            fprintf(stderr, "Running Dissemination %d fill\n", n_thr);
        } else {
            fprintf(stderr, "Running Dissemination %d rr\n", n_thr);
        }

        if(posix_memalign((void**)(&(shm)), ALIGNMENT,
                          sizeof(sharedMemory_t))){
            //returns 0 on success
            fprintf(stderr, "Error allocation of shared memory structure failed\n");
            fflush(stderr);
            exit(127);
        }

        if(posix_memalign((void**)(&threads),
                          ALIGNMENT,n_thr*sizeof(pthread_t))){
            //returns 0 on success
            fprintf(stderr, "Error allocation of threads failed\n");
            fflush(stderr);
            exit(127);
        }

        if(posix_memalign((void**)(&(threaddata)), ALIGNMENT,
                          n_thr*sizeof(threaddata_t))){
            //returns 0 on success
            fprintf(stderr, "Error allocation of thread data structure failed\n");
            fflush(stderr);
            exit(127);
        }

        initSharedMemory(shm, n_thr);

        cores = placement(n_thr, fill, hyperthreads);

        g_timerfreq = 0;      // SK: can somebody verify this?
        rdtsc_synchro = NULL; // SK: can somebody verify this?
        rdtsc_latency = 0;    // SK: can somebody verify this?
        naccesses = 0;        // SK: can somebody verify this?
        initThreads(n_thr, threaddata, shm, g_timerfreq, rdtsc_synchro,
                    rdtsc_latency, naccesses, &accesses, shm->m,
                    shm->rounds, cores, fill, false);

        for(int t=0 ; t < n_thr; t++){
            pthread_create(&threads[t],NULL,function_thread,
                           (void *)(&(threaddata[t])));
            while (!threaddata[t].ack); // initialized thread
        }

        for (int t=0; t < n_thr; t++) {
            fprintf(stderr, "Core[%d]=%d\n", t, cores[t]);
        }

        //let them start
        for(int t=0; t < n_thr; t++)
            threaddata[t].ack = 0;

        // wait for them to finish

        for (int t=0; t < n_thr; t++){
            pthread_join(threads[t],NULL);
        }

        free(threads);
        free(threaddata);
        free(shm);
    }
}


void run_mcs(bool fill, bool hyperthreads) {


	sharedMemory_t * shm;
	threaddata_t * threaddata;
	pthread_t * threads;
    int naccesses; //for no use in barrier, inherited from the bcast benchmark.
    int accesses;
    uint32_t* cores;
    int num_threads;

    if (hyperthreads) {
	    num_threads = sysconf(_SC_NPROCESSORS_ONLN)/2;
    } else {
	    num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    }

    for (int n_thr = 2; n_thr < num_threads+1; n_thr++) {

        if (fill) {
            fprintf(stderr, "Running MCS %d fill\n", n_thr);
        } else {
            fprintf(stderr, "Running MCS %d rr\n", n_thr);
        }

        if(posix_memalign((void**)(&(shm)), ALIGNMENT,
                          sizeof(sharedMemory_t))){
            //returns 0 on success
            fprintf(stderr, "Error allocation of shared memory structure failed\n");
            fflush(stderr);
            exit(127);
        }

        if(posix_memalign((void**)(&threads),
                          ALIGNMENT,n_thr*sizeof(pthread_t))){
            //returns 0 on success
            fprintf(stderr, "Error allocation of threads failed\n");
            fflush(stderr);
            exit(127);
        }

        if(posix_memalign((void**)(&(threaddata)), ALIGNMENT,
                          n_thr*sizeof(threaddata_t))){
            //returns 0 on success
            fprintf(stderr, "Error allocation of thread data structure failed\n");
            fflush(stderr);
            exit(127);
        }

        if(posix_memalign((void**)(&(bar_mcs)), ALIGNMENT,
                          sizeof(bar_mcs))){
            //returns 0 on success
            fprintf(stderr, "Error allocation of thread data structure failed\n");
            fflush(stderr);
            exit(127);
        }

        initSharedMemory(shm, n_thr);
        cores = placement(n_thr, fill, hyperthreads);

        mcs_barrier_init(bar_mcs, n_thr);

        naccesses = 0;        // SK: can somebody verify this?
        initThreads(n_thr, threaddata, shm, 0, 0, 0, naccesses, &accesses,
                    shm->m, shm->rounds,
                    cores, fill, false);

        for(int t=0 ; t < n_thr; t++){
            pthread_create(&threads[t],NULL,function_thread_mcs,
                           (void *)(&(threaddata[t])));
            while (!threaddata[t].ack); // initialized thread
        }

        for (int t=0; t < n_thr; t++) {
            fprintf(stderr, "Core[%d]=%d\n", t, cores[t]);
        }

        //let them start
        for(int t=0; t < n_thr; t++)
            threaddata[t].ack = 0;

        // wait for them to finish

        for (int t=0; t < n_thr; t++){
            pthread_join(threads[t],NULL);
        }

        free(threads);
        free(threaddata);
        free(bar_mcs);
    }
}


void run_smlt(bool fill, bool smlt_dissem, bool hyperthreads) {

	sharedMemory_t * shm;
	threaddata_t * threaddata;
	pthread_t * threads;
	UINT64_T g_timerfreq;
	UINT64_T * rdtsc_synchro;
	int rdtsc_latency;
    int naccesses; //for no use in barrier, inherited from the bcast benchmark.
    int accesses;
    uint32_t* cores;
    errval_t err;
    int num_threads;

    if (hyperthreads) {
	    num_threads = sysconf(_SC_NPROCESSORS_ONLN)/2;
    } else {
	    num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    }

    for (int n_thr = 2; n_thr < num_threads+1; n_thr++) {
        if (fill) {
            if (smlt_dissem) {
                fprintf(stderr, "Running dissemination SMLT %d fill\n", n_thr);
            } else {
                fprintf(stderr, "Running SMLT %d fill\n", n_thr);
            }
        } else {
            if (smlt_dissem) {
                fprintf(stderr, "Running dissemination SMLT %d rr\n", n_thr);
            } else {
                fprintf(stderr, "Running SMLT %d rr\n", n_thr);
            }
        }

        if(posix_memalign((void**)(&(shm)), ALIGNMENT,
                          sizeof(sharedMemory_t))){
            //returns 0 on success
            fprintf(stderr, "Error allocation of shared memory structure failed\n");
            fflush(stderr);
            exit(127);
        }

        if(posix_memalign((void**)(&threads),
                          ALIGNMENT,n_thr*sizeof(pthread_t))){
            //returns 0 on success
            fprintf(stderr, "Error allocation of threads failed\n");
            fflush(stderr);
            exit(127);
        }

        if(posix_memalign((void**)(&(threaddata)), ALIGNMENT,
                          n_thr*sizeof(threaddata_t))){
            //returns 0 on success
            fprintf(stderr, "Error allocation of thread data structure failed\n");
            fflush(stderr);
            exit(127);
        }

        initSharedMemory(shm, n_thr);

        cores = placement(n_thr, fill, hyperthreads);

        for (int t=0; t < n_thr; t++) {
            fprintf(stderr, "Core[%d]=%d\n", t, cores[t]);
        }

        g_timerfreq = 0;          // SK: can somebody verify this?
        rdtsc_synchro = 0;        // SK: can somebody verify this?
        rdtsc_latency = 0;        // SK: can somebody verify this?
        naccesses = 0;            // SK: can somebody verify this?
        initThreads(n_thr, threaddata, shm, g_timerfreq, rdtsc_synchro,
                    rdtsc_latency, naccesses, &accesses, shm->m, shm->rounds,
                    cores, fill, smlt_dissem);

        if (!smlt_dissem) {
            struct smlt_generated_model* model = NULL;
            err = smlt_generate_model(cores, n_thr, NULL, &model);
            if (smlt_err_is_fail(err)) {
                printf("Faild to init SMLT model \n");
                return ;
            }

            struct smlt_topology *topo = NULL;
            err = smlt_topology_create(model, "adaptivetree", &topo);
            if (smlt_err_is_fail(err)) {
                printf("Faild to init SMLT model \n");
                return ;
            }

            err = smlt_context_create(topo, &context);
            if (smlt_err_is_fail(err)) {
                printf("Faild to init SMLT context \n");
                return ;
            }
        } else {
            err = smlt_dissem_barrier_init(cores, n_thr, &bar);
            if (smlt_err_is_fail(err)) {
                printf("Faild to init SMLT context \n");
                return ;
            }
        }
        struct smlt_node *node;
        for (uint64_t t = 0; t < n_thr; t++) {
            node = smlt_get_node_by_id(cores[t]);
            err = smlt_node_start(node, function_thread_smlt, (void*) (&(threaddata[t])));
            if (smlt_err_is_fail(err)) {
                printf("Failed setting up nodes \n");
            }
            while (!threaddata[t].ack); // initialized thread
        }

        //let them start
        for(int t=0; t < n_thr; t++)
            threaddata[t].ack = 0;

        // wait for them to finish

        for (int t = 0; t < n_thr; t++) {
            node = smlt_get_node_by_id(cores[t]);
            err = smlt_node_join(node);
            if (smlt_err_is_fail(err)) {
                printf("Failed setting up nodes \n");
            }
        }

        free(threads);
        free(threaddata);
        free(shm);
        if (smlt_dissem) {
            smlt_dissem_barrier_destroy(bar);
        }
    }
}



int main(int argc, char** argv){

    int total_threads = sysconf(_SC_NPROCESSORS_ONLN);
    errval_t err;

    err = smlt_init(total_threads, true);
    if (smlt_err_is_fail(err)) {
        printf("Faild to init SMLT \n");
        exit(1);
    }

    bool hyper = false;
    if(argc == 2) {
        printf("HYPERTHREADS ENABLED");
        hyper = true;
    }

    run_diss(true, hyper);
    run_diss(false, hyper);
    // smlt tree barrier
    run_smlt(true, false, hyper);
    run_smlt(false, false, hyper);

    run_smlt(true, true, hyper);
    run_smlt(false, true, hyper);

    run_mcs(true, hyper);
    run_mcs(false, hyper);

    //run_diss_tp(true, hyper);
}
