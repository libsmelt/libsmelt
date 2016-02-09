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

