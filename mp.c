#include "mp.h"
#include "topo.h"

/**
 * \brief Send a message
 */
void mp_send(coreid_t receiver, uintptr_t val)
{
    debug_printfff(DBG__AB, "mp_send on %d\n", get_thread_id());
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
    debug_printfff(DBG__AB, "mp_receive on %d\n", get_thread_id());
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
    int *nidx;
    mp_binding** b = mp_get_children(get_thread_id(), &mp_max, &nidx);

    for (int i=0; i<mp_max; i++) {
    
        debug_printfff(DBG__AB, "message(req%d): %d->%d\n",
                       num_requests, get_thread_id(), nidx[i]);

#ifdef MEASURE_SEND_OVERHEAD
        sk_m_restart_tsc(&m_send_overhead);
#endif
        assert (b[i]!=NULL);

        debug_printfff(DBG__AB, "sending .. \n");
        mp_send_raw(b[i], payload);
        num_requests++;

#ifdef MEASURE_SEND_OVERHEAD
        sk_m_add(&m_send_overhead);
#endif

    }

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
    debug_printfff(DBG__AB, "Receiving from parent\n");
    uintptr_t v = mp_receive_raw(mp_get_parent(get_thread_id(), NULL));

    debug_printfff(DBG__AB, "Sending now .. \n");
    mp_send_ab(v + val);

    return v;
}

bool _mp_is_reduction_root(void)
{
    return get_thread_id()==SEQUENTIALIZER;
}

/**
 * \brief Implement reductions
 *
 * \param val The value to be reduce by this node.
 *
 * \return The result of the reduction, which is only meaningful for
 * the root of the reduction, as given by the tree.
 */
uintptr_t mp_reduce(uintptr_t val)
{
    coreid_t my_core_id = get_thread_id();
    uintptr_t current_aggregate = val;
    
    // Receive (this will be from several children)
    // --------------------------------------------------
    if (topo_does_mp_send(my_core_id)) {

        debug_printfff(DBG__REDUCE, "Receiving on core %d\n", my_core_id);
        
        // XXX In which order to receive from clients? Select? Polling
        // serveral channels is expensive.

        // Decide to parents
        for (unsigned i=0; i<topo_num_cores(); i++) {

            if (topo_is_parent_real(my_core_id, i)) {

                debug_printfff(DBG__REDUCE, "Receiving from %d\n", i);
                uintptr_t v = mp_receive(i);
                current_aggregate += v;
                debug_printfff(DBG__REDUCE, "Receiving %" PRIu64 " from %d\n", v, i);
            }
        }

        debug_printfff(DBG__REDUCE, "Receiving done, value is now %" PRIu64 "\n",
                       current_aggregate);

        // Received message from all channels, finishing up reduction
        if (_mp_is_reduction_root()) {
            debug_printff("reduction done\n");
        }
    }

    // Send (this should only be sending one message)
    // --------------------------------------------------
    if (topo_does_mp_receive(my_core_id)) {

        int pidx;
        mp_get_parent(get_thread_id(), &pidx);
        
        mp_binding *b = get_binding(get_thread_id(), pidx);

        debug_printfff(DBG__REDUCE, "sending %" PRIu64 " to parent %d\n",
                       current_aggregate, pidx);

        mp_send_raw(b, current_aggregate);
    }

    return current_aggregate;
}

void mp_barrier(void)
{
    coreid_t tid = get_core_id();
    
    // Broadcast
    if (tid == SEQUENTIALIZER) {
        mp_send_ab(tid);
        
    } else {
        mp_receive_forward(tid);
    }

    // Recution
    mp_reduce(0);
}
