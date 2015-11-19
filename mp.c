#include "mp.h"
#include "topo.h"
#include "sync.h"
#include "internal.h"

#ifdef QRM_DBG_ENABLED
extern void shl_barrier_shm(int b_count);
#endif

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
    
        debug_printfff(DBG__AB, "message(req%d): %d->%d - %d\n",
                       num_requests, get_thread_id(), nidx[i], payload);

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
    int parent_core;
    mp_binding *b = mp_get_parent(get_thread_id(), &parent_core);
    
    debug_printfff(DBG__AB, "Receiving from parent %d\n", parent_core);
    uintptr_t v = mp_receive_raw(b);

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
        
    // XXX In which order to receive from clients? Select? Polling
    // serveral channels is expensive.

    struct binding_lst *blst = _mp_get_children_raw(get_thread_id());
    int numbindings = blst->num;

    assert ((numbindings==0 && !topo_does_mp_send(my_core_id)) ||
            (numbindings>0 && topo_does_mp_send(my_core_id)));

    if (numbindings!=0) {
        debug_printfff(DBG__REDUCE, "Receiving on core %d\n", my_core_id);
    }
    
    // Decide to parents
    for (int i=0; i<numbindings; i++) {

        uintptr_t v = mp_receive_raw(blst->b_reverse[i]);
        current_aggregate += v;
        debug_printfff(DBG__REDUCE, "Receiving %" PRIu64 " from %d\n", v, i);

    }

    debug_printfff(DBG__REDUCE, "Receiving done, value is now %" PRIu64 "\n",
                       current_aggregate);

    // Send (this should only be sending one message)
    // --------------------------------------------------

    binding_lst *blst_parent = _mp_get_parent_raw(get_thread_id());
    int pidx = blst_parent->idx[0];
    
    assert ((pidx!=-1 && topo_does_mp_receive(my_core_id)) ||
            (pidx==-1 && !topo_does_mp_receive(my_core_id)));

    assert (pidx!=-1 || my_core_id == SEQUENTIALIZER);
    
    if (pidx!=-1) {

        mp_binding *b_parent = blst_parent->b_reverse[0];
        debug_printfff(DBG__REDUCE, "sending %" PRIu64 " to parent %d\n",
                       current_aggregate, pidx);

        mp_send_raw(b_parent, current_aggregate);
    }

    return current_aggregate;
}

static __thread uint32_t _num_barrier = 0;
int mp_get_counter(const char *ctr_name)
{
    if (strcmp(ctr_name, "barriers")==0) {

        return _num_barrier;
    }
    else {

        return -1;
    }
}

void mp_barrier(cycles_t *measurement)
{
    coreid_t tid = get_core_id();

    debug_printfff(DBG__REDUCE, "barrier enter #%d\n", _num_barrier);

#ifdef QRM_DBG_ENABLED
    ++_num_barrier;
    uint32_t _num_barrier_recv = _num_barrier;
#endif    
    
    // Recution
    // --------------------------------------------------
    uint32_t _tmp = mp_reduce(_num_barrier);

#ifdef QRM_DBG_ENABLED    
    // Sanity check
    if (tid==SEQUENTIALIZER) {
        assert (_tmp == get_num_threads()*_num_barrier);
    }
#endif
    
    if (measurement)
        *measurement = bench_tsc();
    
    // Broadcast
    // --------------------------------------------------
    if (tid == SEQUENTIALIZER) {
        mp_send_ab(_num_barrier);
        
    } else {
#ifdef QRM_DBG_ENABLED
        _num_barrier_recv =
#endif
            mp_receive_forward(0);
    }

#ifdef QRM_DBG_ENABLED
    if (_num_barrier_recv != _num_barrier) {
    debug_printf("ASSERTION fail %d != %d\n", _num_barrier_recv, _num_barrier);
    }
    assert (_num_barrier_recv == _num_barrier);

    // Add a shared memory barrier to absolutely make sure that
    // everybody finished the barrier before leaving - this simplifies
    // debugging, as the programm will get stuck if barriers are
    // broken, rather than some threads (wrongly) continuing and
    // causing problems somewhere else
#if 0 // Enable separately
    debug_printfff(DBG_REDUCE, "finished barrier .. waiting for others\n");
    shl_barrier_shm(get_num_threads());
#endif
#endif    

    debug_printfff(DBG__REDUCE, "barrier complete #%d\n", _num_barrier);
}
