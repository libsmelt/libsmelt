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

