/*
 *
 */
#include <stdio.h>
#include "sync.h"
#include <pthread.h>

#define NUM_THREADS 4
void* worker(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    return NULL;
}

int main(int argc, char **argv)
{
    __sync_init();

    pthread_t ptds[NUM_THREADS];
    int tids[NUM_THREADS];

    // Create
    for (int i=0; i<NUM_THREADS; i++) {
        tids[i] = i;
        pthread_create(ptds+i, NULL, &worker, (void*) (tids+i));
    }

    // Join
    for (int i=0; i<NUM_THREADS; i++) {
        pthread_join(ptds[i], NULL);
    }
    
}
