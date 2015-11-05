/*
 *
 */
#include <stdio.h>
#include "sync.h"
#include "topo.h"
#include "mp.h"
#include <pthread.h>

#define NUM_THREADS 4
void* worker1(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    if (tid == SEQUENTIALIZER) {

        for (unsigned int i=0; i<topo_num_cores(); i++) {
            if (topo_is_parent(get_thread_id(), i)) {
                mp_send(i, get_thread_id());
            }
        }
    }

    else {

        for (unsigned int i=0; i<topo_num_cores(); i++) {
            if (i==SEQUENTIALIZER && topo_is_parent(i, get_thread_id())) {
                assert (mp_receive(i)==i);
            }
        }
    }

    printf("Thread %d completed\n", tid);
    
    return NULL;
}

void* worker2(void* a)
{
    int tid = *((int*) a);
    __thread_init(tid, NUM_THREADS);

    if (tid == SEQUENTIALIZER) {
        mp_send_ab(tid);
    } else {
        mp_receive_forward(tid);
    }
    printf("Thread %d completed\n", tid);
    
    return NULL;
}

int main(int argc, char **argv)
{
    __sync_init(NUM_THREADS);

    pthread_t ptds[NUM_THREADS];
    int tids[NUM_THREADS];

    // Create
    for (int i=0; i<NUM_THREADS; i++) {
        tids[i] = i;
        pthread_create(ptds+i, NULL, &worker1, (void*) (tids+i));
    }

    // Join
    for (int i=0; i<NUM_THREADS; i++) {
        pthread_join(ptds[i], NULL);
    }

    // Create
    for (int i=0; i<NUM_THREADS; i++) {
        tids[i] = i;
        pthread_create(ptds+i, NULL, &worker2, (void*) (tids+i));
    }

    // Join
    for (int i=0; i<NUM_THREADS; i++) {
        pthread_join(ptds[i], NULL);
    }
    
}
