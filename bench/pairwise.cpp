/*
 *
 */
#include <stdio.h>
#include "sync.h"
#include "topo.h"
#include "mp.h"
#include "measurement_framework.h"
#include "internal.h"
#include <pthread.h>
#include <unistd.h>

#include "model_defs.h"

#define NUM_EXP 1000

#define INIT_SKM(func, sender, receiver)                                \
        char _str_buf[1024];                                            \
        cycles_t _buf[NUM_EXP];                                         \
        struct sk_measurement m;                                        \
        snprintf(_str_buf, 1024, "%s-%d-%d", func, sender, receiver);   \
        sk_m_init(&m, NUM_EXP, _str_buf, _buf);                         \
                                                                        \


struct thr_args {
    coreid_t s;
    coreid_t r;
};

void* thr_sender(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;
    __lowlevel_thread_init(arg->s);

    INIT_SKM("send", arg->s, arg->r);

    for (int i=0; i<NUM_EXP; i++) {

        sk_m_restart_tsc(&m);
        mp_send(arg->r, i);
        sk_m_add(&m);

        mp_receive(arg->r);
    }

    sk_m_print(&m);

    return NULL;
}


void* thr_receiver(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;
    __lowlevel_thread_init(arg->r);

    INIT_SKM("receive", arg->s, arg->r);

    for (int i=0; i<NUM_EXP; i++) {

        sk_m_restart_tsc(&m);
        mp_receive(arg->s);
        sk_m_add(&m);

        mp_send(arg->s, i);
    }

    sk_m_print(&m);

    return NULL;
}


int main(int argc, char **argv)
{
    coreid_t num_cores = (coreid_t) sysconf(_SC_NPROCESSORS_CONF);

    __sync_init(num_cores, false);

    for (coreid_t s=0; s<num_cores; s++) {
        for (coreid_t r=0; r<num_cores; r++) {

            // Measurement does not make sense if sender == receiver
            if (s==r) continue;

            pthread_t ptd1;
            struct thr_args arg = {
                .s = s,
                .r = r
            };

            _setup_ump_chanels(s, r);

            // Thread for the sender
            pthread_create(&ptd1, NULL, thr_sender, (void*) &arg);

            // Receive on THIS thread: we don't want another thread
            // for this, as the master would then busy-wait for both
            // to complete, and we would have to guarantee that it
            // does not do so on any of the cores involved in the
            // benchmark, or their hyper-threads, which seems like a
            // hassle.
            thr_receiver((void*) &arg);

            // Wait for sender to complete
            pthread_join(ptd1, NULL);

        }
    }
}
