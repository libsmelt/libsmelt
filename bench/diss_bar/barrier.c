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

#include <smlt.h>
#include <smlt_generator.h>
#include <smlt_topology.h>
#include <smlt_context.h>
#include <smlt_node.h>
struct smlt_context* context;

static uint32_t* placement(uint32_t n, bool do_fill)
{
    uint32_t* result = (uint32_t*) malloc(sizeof(uint32_t)*n);
    uint32_t numa_nodes = numa_max_node()+1;
    uint32_t num_cores = numa_num_configured_cpus();
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

            /*
            uint8_t ith_of_node = 0;
            // go through numa nodes
            for (int i = 0; i < numa_nodes; i++) {
                // go through cores and see if part of numa node
                for (int j = 0; j < num_cores; j++) {
                    // take the ith core of the node
                    if (numa_bitmask_isbitset(nodes[i], j)){
                        int index = i+ith_of_node*numa_nodes;
                        if (index < n) {
                            result[i+ith_of_node*numa_nodes] = j;
                            num_taken++;
                            ith_of_node++;
                        }
                    }
                    if (num_taken == n) {
                        return result;
                    }
                }
                ith_of_node = 0;
            }
            */
        }
    } else {
        printf("Libnuma not available \n");
        return NULL;
    }
    return NULL;
}


void run_diss(bool fill) {

	sharedMemory_t * shm;
	threaddata_t * threaddata;
	pthread_t * threads;
	UINT64_T g_timerfreq;
	UINT64_T * rdtsc_synchro;
	int rdtsc_latency;
	int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    int naccesses; //for no use in barrier, inherited from the bcast benchmark.
    int accesses;
    int step_size = 4;        
    uint32_t* cores;

    if (numa_available() == 0) {
        step_size = numa_max_node()+1;
    }

    if (step_size < 2) {
        step_size = 2;
    }


    for (int n_thr = step_size; n_thr < num_threads+1; n_thr += step_size) {

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
        if(posix_memalign((void **)(&rdtsc_synchro), ALIGNMENT,
                                    sizeof(rdtsc_synchro)*NITERS)){
            //returns 0 on success
            fprintf(stderr, "Error allocation of rdtsc_synchro failed\n");
            fflush(stderr);
            exit(127);
        }

        initSharedMemory(shm, n_thr);
        init_time(&g_timerfreq, &rdtsc_latency);
        fprintf(stderr,"Frequency %lu\n", g_timerfreq);


        cores = placement(n_thr, fill);

        initThreads(n_thr, threaddata, shm, g_timerfreq, rdtsc_synchro,
                    rdtsc_latency, naccesses, &accesses, shm->m,
                    shm->rounds, cores, fill);

        for(int t=0 ; t < n_thr; t++){
            pthread_create(&threads[t],NULL,function_thread, 
                           (void *)(&(threaddata[t])));
            while (!threaddata[t].ack); // initialized thread
        }

        for (int t=0; t < n_thr; t++) {
            fprintf(stderr, "Core[%d]=%d\n", t, cores[t]);
        }

        generate_rdtscsynchro(rdtsc_synchro, NITERS, g_timerfreq);

        //let them start
        for(int t=0; t < n_thr; t++)
            threaddata[t].ack = 0;

        // wait for them to finish

        for (int t=0; t < n_thr; t++){
            pthread_join(threads[t],NULL);
        }

        free(threads);
        free(threaddata);
        free(rdtsc_synchro);
        free(shm);
    }
}


void run_smlt(bool fill) {

	sharedMemory_t * shm;
	threaddata_t * threaddata;
	pthread_t * threads;
	UINT64_T g_timerfreq;
	UINT64_T * rdtsc_synchro;
	int rdtsc_latency;
	int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    int naccesses; //for no use in barrier, inherited from the bcast benchmark.
    int accesses;
    int step_size = 4;        
    uint32_t* cores;
    errval_t err;
    struct smlt_node* node;

    if (numa_available() == 0) {
        step_size = numa_max_node()+1;
    }

    if (step_size < 2) {
        step_size = 2;
    }

    for (int n_thr = step_size; n_thr < num_threads+1; n_thr += step_size) {
        if (fill) {
            fprintf(stderr, "Running SMLT %d fill\n", n_thr);
        } else {
            fprintf(stderr, "Running SMLT %d rr\n", n_thr);
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
        if(posix_memalign((void **)(&rdtsc_synchro), ALIGNMENT,
                                    sizeof(rdtsc_synchro)*NITERS)){
            //returns 0 on success
            fprintf(stderr, "Error allocation of rdtsc_synchro failed\n");
            fflush(stderr);
            exit(127);
        }

        initSharedMemory(shm, n_thr);
        init_time(&g_timerfreq, &rdtsc_latency);
        fprintf(stderr,"Frequency %lu\n", g_timerfreq);

        cores = placement(n_thr, fill);

        for (int t=0; t < n_thr; t++) {
            fprintf(stderr, "Core[%d]=%d\n", t, cores[t]);
        }

        initThreads(n_thr, threaddata, shm, g_timerfreq, rdtsc_synchro,
                    rdtsc_latency, naccesses, &accesses, shm->m, shm->rounds,
                    cores, fill);

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

        struct smlt_node *node;
        for (uint64_t t = 0; t < n_thr; t++) {
            node = smlt_get_node_by_id(cores[t]);
            err = smlt_node_start(node, function_thread, (void*) (&(threaddata[t])));
            if (smlt_err_is_fail(err)) {
                printf("Failed setting up nodes \n");
            }
            while (!threaddata[t].ack); // initialized thread
        }

        generate_rdtscsynchro(rdtsc_synchro, NITERS, g_timerfreq);

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
        free(rdtsc_synchro);
        free(shm);
    }
}



int main(int argc, char** argv){

    run_diss(true);
    run_diss(false);

    int total_threads = sysconf(_SC_NPROCESSORS_ONLN);
    errval_t err;

    err = smlt_init(total_threads, true);
    if (smlt_err_is_fail(err)) {
        printf("Faild to init SMLT \n");
        exit(1);
    }
    
    run_smlt(true);
    run_smlt(false);
}




