#include <smlt.h>
#include <smlt_reduce.h>




/**
 * @brief performs a reduction on the current instance
 * 
 * @param msg        input for the reduction
 * @param result     returns the result of the reduction
 * @param operation  function to be called to calculate the aggregate
 *
 * @returns TODO:errval
 */
errval_t smlt_reduce(struct smlt_msg *input,
                     struct smlt_msg *result,
                     smlt_reduce_fn_t operation)
{
    if (!operation) {
        return smlt_reduce_notify();
    }

    if (smlt_current_instance()->has_shm) {
        smlt_shm_reduce(input, result);
        if (smlt_err_fail(err)) {
            return err;
        }
    }
    
    struct smlt_node *p = smlt_node_get_parent();
    if (smlt_node_is_leaf()) {    
        smlt_node_send(p, input);
    } else {
        uint32_t count;
        struct smlt_node **nl = smlt_node_get_children(&count);

        result = operation(input, NULL);
        for (uint32_t i = 0; i < count; ++i) {
            err = smlt_node_recv(nl[i], result);
            // TODO: error handling
            result = operation(input, result);
        }

        if (!smlt_node_is_root()) {
            smlt_node_send(p, result);
        }
    }

    return SMLT_SUCCESS;
}

/**
 * @brief performs a reduction without any payload on teh current instance
 *
 * @returns TODO:errval
 */
errval_t smlt_reduce_notify(void)
{
    errval_t err;

    if (smlt_current_instance()->has_shm) {
        err = shm_reduce_notify();
        if (smlt_err_fail(err)) {
            return err;
        }
    }

    struct smlt_node *p = smlt_node_get_parent();
    if (smlt_node_is_leaf()) {    
        smlt_node_notify(p);
    } else {
        uint32_t count;
        struct smlt_node **nl = smlt_node_get_children(&count);

        for (uint32_t i = 0; i < count; ++i) {
            err = smlt_node_recv(nl[i], NULL);
            // TODO: error handling
        }

        if (!smlt_node_is_root()) {
            smlt_node_notify(p);
        }
    }
}


/**
 * @brief performs a reduction and distributes the result to all nodes
 * 
 * @param msg       input for the reduction
 * @param result    returns the result of the reduction
 * @param operation  function to be called to calculate the aggregate
 * 
 * @returns TODO:errval
 */
errval_t smlt_reduce_all(struct smlt_msg *input,
                         struct smlt_msg *result,
                         smlt_reduce_fn_t operation)
{
    errval_t err;

    err = smlt_reduce(input, result);
    if (smlt_err_fail(err)) {
        return err;
    }

    return smlt_broadcast(result);
}



/**
 * \brief
 *
 */
uintptr_t sync_reduce(uintptr_t val)
{
    // --------------------------------------------------
    // Shared memory first

    // Note: tree is processed in backwards direction, i.e. a receive
    // corresponds to a send and vica-versa.

    uintptr_t current_aggregate = val;

    current_aggregate += shm_reduce(current_aggregate);

    // --------------------------------------------------
    // Message passing

    /*
     * Each client receives (potentially from several children) and
     * sends only ONCE. On which level of the tree hierarchy this is
     * does not matter. If a client would send several messages, we
     * would have circles in the tree.
     */
    return mp_reduce(current_aggregate);
}

uintptr_t sync_reduce0(uintptr_t val)
{
    // --------------------------------------------------
    // Shared memory first

    // Note: tree is processed in backwards direction, i.e. a receive
    // corresponds to a send and vica-versa.

    uintptr_t current_aggregate = val;

    current_aggregate += shm_reduce(current_aggregate);

    // --------------------------------------------------
    // Message passing

    /*
     * Each client receives (potentially from several children) and
     * sends only ONCE. On which level of the tree hierarchy this is
     * does not matter. If a client would send several messages, we
     * would have circles in the tree.
     */
    mp_reduce0();
    return 0;
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