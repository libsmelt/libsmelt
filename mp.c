#include "mp.h"

/**
 * \brief Send a message
 */
void mp_send(coreid_t receiver, uintptr_t val)
{
    printf("mp_send on %d\n", get_thread_id());
    mp_binding *b = get_binding(get_thread_id(), receiver);
    if (b==NULL) {
        printf("Failed to get binding for %d %d\n", get_thread_id(), receiver);
    }
    assert (b!=NULL);
    mp_send_raw(b, val);
}

/**
 * \brief Receive a message
 */
uintptr_t mp_receive(coreid_t sender)
{
    printf("mp_receive on %d\n", get_thread_id());
    mp_binding *b = get_binding(sender, get_thread_id());
    if (b==NULL) {
        printf("Failed to get binding for %d %d\n", sender, get_thread_id());
    }
    assert (b!=NULL);
    return mp_receive_raw(b);
}


/**
 * \brief Send a multicast
 *
 * Send a message to all children in the tree given by the
 * topology. The order is given by qrm_children.
 */
static __thread int num_tree_acks = 0;
static __thread int num_requests = 0;
uintptr_t mp_send_ab(uintptr_t payload)
{
    num_requests = 0;
    num_tree_acks = 0;

    // Walk children and send a message each
    int mp_max;
    mp_binding** b = mp_get_children(get_thread_id(), &mp_max);

    for (int i=0; i<mp_max; i++) {
    
        debug_printfff(DBG__AB, "message(req%d): %d->%d (sending_core_id=%d)\n",
             num_requests, my_core_id, n->node_idx, sender_core_id);

#ifdef MEASURE_SEND_OVERHEAD
        sk_m_restart_tsc(&m_send_overhead);
#endif
        assert (b[i]!=NULL);

        printf("sending .. \n");
        mp_send_raw(b[i], payload);
        num_requests++;

#ifdef MEASURE_SEND_OVERHEAD
        sk_m_add(&m_send_overhead);
#endif

    }
    free(b);

#ifdef QRM_DO_ACKS
    // If no children, ACK right away
    if (num_requests==0) {

        QDBG("forward_tree_message -> qrm_send_ack\n");
        qrm_send_ack();
    }
#else

#if defined(CONFIG_TRACE)
    trace_flush(MKCLOSURE(trace_callback, NULL));
#endif /* CONFIG_TRACE */

#endif

    return 0;
}

/**
 * \brief Receive a message from the broadcast tree and forward
 *
 * \return The message received from the broadcast tree
 */
uintptr_t mp_receive_forward(uintptr_t val)
{
    debug_printf("Receiving from parent\n");
    uintptr_t v = mp_receive_raw(mp_get_parent(get_thread_id()));

    debug_printf("Sending now .. \n");
    mp_send_ab(v + val);

    return v;
}
