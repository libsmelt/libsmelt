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

#include "ump_conf.h"
#include "ump_common.h"

#define NUM_EXP 100

#define INIT_SKM(func, sender, receiver)                                \
        char _str_buf[1024];                                            \
        cycles_t _buf[NUM_EXP];                                         \
        struct sk_measurement m;                                        \
        snprintf(_str_buf, 1024, "%s-%d-%d", func, sender, receiver);   \
        sk_m_init(&m, NUM_EXP, _str_buf, _buf);

/**
 * \brief Busy wait for message.
 *
 * Receiving thread will not sleep. This will be bad for machines
 * using hyper-threads.
 */
uintptr_t raw_receive(mp_binding *b, sk_measurement *_m)
{
    struct ump_pair_state *ups = (struct ump_pair_state*) b;
    struct ump_queue *q = &ups->dst.queue;

    uintptr_t ret;
    bool success;

    assert (q!=NULL || !"Invalid channel given - check channel setup");

    do {

        if (_m) sk_m_restart_tsc(_m);
        success = ump_dequeue_word_nonblock(q, &ret);

    } while(!success);

    if (_m) sk_m_add(_m);

    return ret;
}

struct thr_args {
    coreid_t s;
    coreid_t r;
};

void* thr_sender(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;
    __lowlevel_thread_init(arg->s);

    INIT_SKM("send", arg->s, arg->r);

    mp_binding *bsend = get_binding(get_thread_id(), arg->r);
    mp_binding *brecv = get_binding(arg->r, get_thread_id());

    for (int i=0; i<NUM_EXP; i++) {

        sk_m_restart_tsc(&m);
        mp_send_raw(bsend, i);
        sk_m_add(&m);

        raw_receive(brecv, NULL);
    }

    sk_m_print(&m);
    __thread_end();

    return NULL;
}


void* thr_receiver(void* a)
{
    struct thr_args* arg = (struct thr_args*) a;
    __lowlevel_thread_init(arg->r);

    INIT_SKM("receive", arg->s, arg->r);

    mp_binding *bsend = get_binding(get_thread_id(), arg->s);
    mp_binding *brecv = get_binding(arg->s, get_thread_id());

    for (unsigned i=0; i<NUM_EXP; i++) {

        assert(raw_receive(brecv, &m)==i);
        mp_send_raw(bsend, i);
    }

    sk_m_print(&m);
    __thread_end();

    return NULL;
}


int main(int argc, char **argv)
{
    coreid_t num_cores = (coreid_t) sysconf(_SC_NPROCESSORS_CONF);
    printf("Running with %d cores\n", num_cores);

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

            _setup_chanels(s, r);

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
