#include "mp.h"
#include "topo.h"
#include "sync.h"
#include "internal.h"

extern void shl_barrier_shm(int b_count);
#ifdef QRM_DBG_ENABLED
#endif

/**
 * \brief Connect two nodes
 */
void mp_connect(coreid_t src, coreid_t dst)
{
    _setup_ump_chanels(src, dst);
}


/**
 * \brief Send a message
 */
__thread uint64_t num_mp_send = 0;
void mp_send(coreid_t r, uintptr_t val)
{
    num_mp_send++;
    debug_printfff(DBG__AB, "mp_send to %d - %d\n", r, num_mp_send);
    
    coreid_t s = get_thread_id();
    mp_binding *b = get_binding(s, r);
    
    if (b==NULL) {
        printf("Failed to get binding for %d %d\n", s, r);
    }
    assert (b!=NULL);
    mp_send_raw(b, val);
}

void mp_send7(coreid_t r, 
             uintptr_t val1,
             uintptr_t val2,
             uintptr_t val3,
             uintptr_t val4,
             uintptr_t val5,
             uintptr_t val6,
             uintptr_t val7)
{
    num_mp_send++;
    debug_printfff(DBG__AB, "mp_send to %d - %d\n", r, num_mp_send);
    
    coreid_t s = get_thread_id();
    mp_binding *b = get_binding(s, r);
    
    if (b==NULL) {
        printf("Failed to get binding for %d %d\n", s, r);
    }
    assert (b!=NULL);
    mp_send_raw7(b, val1, val2, val3, val4,
                val5, val6, val7);
}


/**
 * \brief Receive a message from s
 */
__thread uint64_t num_mp_receive = 0;
uintptr_t mp_receive(coreid_t s)
{
    num_mp_receive++;
    debug_printfff(DBG__AB, "mp_receive from %d - %d\n", s, num_mp_receive);
    
    coreid_t r = get_thread_id();
    mp_binding *b = get_binding(s, r);
    
    if (b==NULL) {
        printf("Failed to get binding for %d %d\n", s, get_thread_id());
    }
    assert (b!=NULL);
    return mp_receive_raw(b);
}

uintptr_t* mp_receive7(coreid_t s)
{
    num_mp_receive++;
    debug_printfff(DBG__AB, "mp_receive from %d - %d\n", s, num_mp_receive);
    
    // TODO back to get thread_id
    coreid_t r = get_thread_id();
    mp_binding *b = get_binding(s, r);
    
    if (b==NULL) {
        printf("Failed to get binding for %d %d\n", s, get_thread_id());
    }
    assert (b!=NULL);
    return mp_receive_raw7(b);
}

bool mp_can_receive(coreid_t s)
{
    coreid_t r = get_thread_id();
    mp_binding *b = get_binding(s, r);

    if (b==NULL) {
        return false;
    }

    return mp_can_receive_raw(b);

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

uintptr_t mp_send_ab7(uintptr_t val1,
                      uintptr_t val2,
                      uintptr_t val3,
                      uintptr_t val4,
                      uintptr_t val5,
                      uintptr_t val6,
                      uintptr_t val7)
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
        mp_send_raw7(b[i], val1, val2, val3, val4,
                     val5, val6, val7);
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

uintptr_t* mp_receive_forward7(uintptr_t val)
{
    int parent_core;

    mp_binding *b = mp_get_parent(get_thread_id(), &parent_core);
    
    debug_printfff(DBG__AB, "Receiving from parent %d\n", parent_core);
    uintptr_t* v = mp_receive_raw7(b);

    mp_send_ab7(v[0],
                v[1],
                v[2],
                v[3],
                v[4],
                v[5],
                v[6]);

    return v;
}

bool _mp_is_reduction_root(void)
{
    return get_thread_id()==get_sequentializer();
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
        
    // Determine child bindings
    struct binding_lst *blst = _mp_get_children_raw(get_thread_id());
    int numbindings = blst->num;

    assert ((numbindings==0 && !topo_does_mp_send(my_core_id, false)) ||
            (numbindings>0 && topo_does_mp_send(my_core_id, false)));

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
    
    assert ((pidx!=-1 && topo_does_mp_receive(my_core_id, false)) ||
            (pidx==-1 && !topo_does_mp_receive(my_core_id, false)));

    assert (pidx!=-1 || my_core_id == get_sequentializer());
    
    if (pidx!=-1) {

        mp_binding *b_parent = blst_parent->b_reverse[0];
        debug_printfff(DBG__REDUCE, "sending %" PRIu64 " to parent %d\n",
                       current_aggregate, pidx);

        mp_send_raw(b_parent, current_aggregate);
    }

    return current_aggregate;
}

/*
 * Only reduceds the first value !
 *
 * If the node is a leaf, the node dose not receive
 * anything but will send val1 - val7 up the tree
 *
 * If the node is not a leaf, the node will 
 * aggregate the val1, and send the values received
 * up the tree 
 * 
 * 
 */
static __thread uintptr_t vals[7];
uintptr_t* mp_reduce7(uintptr_t val1,
                      uintptr_t val2,
                      uintptr_t val3,
                      uintptr_t val4,
                      uintptr_t val5,
                      uintptr_t val6,
                      uintptr_t val7)
{
    coreid_t my_core_id = get_thread_id();
    uintptr_t current_aggregate = val1;
    // Receive (this will be from several children)
    // --------------------------------------------------
        
    // Determine child bindings
    struct binding_lst *blst = _mp_get_children_raw(get_thread_id());
    int numbindings = blst->num;

    assert ((numbindings==0 && !topo_does_mp_send(my_core_id, false)) ||
            (numbindings>0 && topo_does_mp_send(my_core_id, false)));

    if (numbindings!=0) {
        debug_printfff(DBG__REDUCE, "Receiving on core %d\n", my_core_id);
    }
    
    // Decide to parents
    uintptr_t* v;
    for (int i=0; i<numbindings; i++) {

        v = mp_receive_raw7(blst->b_reverse[i]);
        current_aggregate += v[0];
        debug_printfff(DBG__REDUCE, "Receiving %" PRIu64 " from %d\n", v, i);
        if (i == 0) {
            vals[1] = v[1];
            vals[2] = v[2];
            vals[3] = v[3];
            vals[4] = v[4];
            vals[5] = v[5];
            vals[6] = v[6];
        }   
    }
    
    if (numbindings == 0) {
        vals[1] = val2;
        vals[2] = val3;
        vals[3] = val4;
        vals[4] = val5;
        vals[5] = val6;
        vals[6] = val7;
    }

    debug_printfff(DBG__REDUCE, "Receiving done, value is now %" PRIu64 "\n",
                       current_aggregate);

    // Send (this should only be sending one message)
    // --------------------------------------------------

    binding_lst *blst_parent = _mp_get_parent_raw(get_thread_id());
    int pidx = blst_parent->idx[0];
    
    assert ((pidx!=-1 && topo_does_mp_receive(my_core_id, false)) ||
            (pidx==-1 && !topo_does_mp_receive(my_core_id, false)));

    assert (pidx!=-1 || my_core_id == get_sequentializer());
    
    if (pidx!=-1) {

        mp_binding *b_parent = blst_parent->b_reverse[0];
        debug_printfff(DBG__REDUCE, "sending %" PRIu64 " to parent %d\n",
                       current_aggregate, pidx);
        mp_send_raw7(b_parent, current_aggregate, 
			             vals[1], vals[2], vals[3], vals[4], vals[5],
			             vals[6]);
    }

    vals[0] = current_aggregate;
    return vals;
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

#ifdef QRM_DBG_ENABLED
    ++_num_barrier;
    uint32_t _num_barrier_recv = _num_barrier;
#endif    

    debug_printfff(DBG__REDUCE, "barrier enter #%d\n", _num_barrier);
    
    // Recution
    // --------------------------------------------------
#ifdef QRM_DBG_ENABLED    
    uint32_t _tmp =
#endif
    mp_reduce(_num_barrier);

#ifdef QRM_DBG_ENABLED    
    // Sanity check
    if (tid==get_sequentializer()) {
        assert (_tmp == get_num_threads()*_num_barrier);
    }
#endif
    
    if (measurement)
        *measurement = bench_tsc();
    
    // Broadcast
    // --------------------------------------------------
    if (tid == get_sequentializer()) {
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
