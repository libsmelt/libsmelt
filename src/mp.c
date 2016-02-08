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
    debug_printf("Opening channels manually: %d -> %d\n", src, dst);
    assert (get_binding(src, dst)==NULL);

    _setup_chanels(src, dst);

    assert (get_binding(src,dst)!=NULL);
}




/**
 * \brief Send a multicast
 *
 * Send a message to all children in the tree given by the
 * topology. The order is given by qrm_children.
 */
uintptr_t mp_send_ab(uintptr_t payload)
{
#ifdef QRM_DBG_ENABLED
    num_requests = 0;
#endif

    // Walk children and send a message each
    int mp_max;
    int *nidx;
    mp_binding** b = mp_get_children(get_thread_id(), &mp_max, &nidx);

    for (int i=0; i<mp_max; i++) {

        debug_printfff(DBG__AB, "message(req%d): %d->%d - %d\n",
                       num_requests, get_thread_id(), nidx[i], payload);

        dbg_assert (b[i]!=NULL);

        debug_printfff(DBG__AB, "sending .. \n");
        mp_send_raw(b[i], payload);

#ifdef QRM_DBG_ENABLED
        num_requests++;
#endif

    }

    return 0;
}

void mp_send_ab0(void)
{
    // Walk children and send a message each
    int mp_max;
    int *nidx;
    mp_binding** b = mp_get_children(get_thread_id(), &mp_max, &nidx);

    for (int i=0; i<mp_max; i++) {
        mp_send_raw0(b[i]);
    }

    return;
}


/**
 * \brief Receive a message from the broadcast tree and forward
 *
 * \param val The value to be added to the received value before
 * forwarding. Useful for reductions, but should probably not be here.
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

void mp_receive_forward0(void)
{
    int parent_core;

    mp_binding *b = mp_get_parent(get_thread_id(), &parent_core);

    mp_receive_raw0(b);

    mp_send_ab0();
}


#if 0
bool _mp_is_reduction_root(void)
{
    return get_thread_id()==get_sequentializer();
}
#endif

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
#ifdef HYB
    if (!topo_does_mp(get_thread_id()))
        return val;
#endif

    uintptr_t current_aggregate = val;

    // Receive (this will be from several children)
    // --------------------------------------------------

    // Determine child bindings
    struct binding_lst *blst = _mp_get_children_raw(get_thread_id());
    int numbindings = blst->num;

#ifdef QRM_DBG_ENABLED
    coreid_t my_core_id = get_thread_id();
    assert ((numbindings==0 && !topo_does_mp_send(my_core_id, false)) ||
            (numbindings>0 && topo_does_mp_send(my_core_id, false)));

    if (numbindings!=0) {
        debug_printfff(DBG__REDUCE, "Receiving on core %d\n", my_core_id);
    }
#endif

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

#ifdef QRM_DBG_ENABLED
    assert ((pidx!=-1 && topo_does_mp_receive(my_core_id, false)) ||
            (pidx==-1 && !topo_does_mp_receive(my_core_id, false)));

    assert (pidx!=-1 || my_core_id == get_sequentializer());
#endif

    if (pidx!=-1) {

        mp_binding *b_parent = blst_parent->b_reverse[0];
        debug_printfff(DBG__REDUCE, "sending %" PRIu64 " to parent %d\n",
                       current_aggregate, pidx);

        mp_send_raw(b_parent, current_aggregate);
    }

    return current_aggregate;
}


void mp_reduce0(void)
{
    coreid_t my_core_id = get_thread_id();

    if (!topo_does_mp(my_core_id))
        return;


    // Receive (this will be from several children)
    // --------------------------------------------------

    // Determine child bindings
    struct binding_lst *blst = _mp_get_children_raw(my_core_id);
    int numbindings = blst->num;

    // Decide to parents
    for (int i=0; i<numbindings; i++) {
        mp_receive_raw0(blst->b_reverse[i]);
    }

    // Send (this should only be sending one message)
    // --------------------------------------------------

    binding_lst *blst_parent = _mp_get_parent_raw(my_core_id);
    int pidx = blst_parent->idx[0];

    if (pidx!=-1) {

        mp_binding *b_parent = blst_parent->b_reverse[0];

        mp_send_raw0(b_parent);
    }
}



/*
 * Only reduceds the first value !
 *
 * If the node is a leaf, the node dose not receive
 * anything but will send val1 - val7 up the tree
 *
 * If the node is not a leaf, the node will
 * not aggregate anything, and send the values received
 * up the tree
 *
 *
 * */

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
    if (measurement)
        *measurement = bench_tsc();

#endif

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


void mp_barrier0(void)
{
    coreid_t tid = get_core_id();

    // Recution
    // --------------------------------------------------
    mp_reduce0();


    // Broadcast
    // --------------------------------------------------
    if (tid == get_sequentializer()) {
        mp_send_ab0();

    } else {
        mp_receive_forward0();
    }
}
