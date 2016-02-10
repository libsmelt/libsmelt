/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */


#include <smlt.h>
#include <smlt_node.h>


#if 0

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


bool topo_does_mp(coreid_t core)
{
    return topo_does_mp_send(core, false) || topo_does_mp_receive(core, false);
}


bool topo_does_shm_send(coreid_t core)
{
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_get(core, i)>=SHM_MASTER_START &&
            topo_get(core, i)<SHM_MASTER_MAX) {

            return true;
        }
    }

    return false;
}

bool topo_does_shm_receive(coreid_t core)
{
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_get(core, i)>=SHM_SLAVE_START &&
            topo_get(core, i)<SHM_SLAVE_MAX) {

            return true;
        }
    }

    return false;
}

/**
 * \brief Check whether a node sends messages according to the current
 * model.
 *
 * If <core> does an mp_send, there is at least one other node that
 * has <core> as a parent.
 *
 * \param include_leafs Consider communication from leaf nodes to
 * sequentializer as message passing links
 */
bool topo_does_mp_send(coreid_t core, bool include_leafs)
{
    // All last nodes will send messages
    if (topo_is_leaf_node(core) && include_leafs) {
        return true;
    }

    // Is the node a parent of any other node?
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_is_child(core, i)) {
            return true;
        }
    }

    return false;
}

/**
 * \brief Check whether a node receives messages according to the
 * current model.
 *
 * \param include_leafs Consider communication from leaf nodes to
 * sequentializer as message passing links
 */
bool topo_does_mp_receive(coreid_t core, bool include_leafs)
{
    // The sequentializer has to receive messages
    if (core == get_sequentializer() && include_leafs) {
        return true;
    }

    // Is the node receiving from any other node
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_is_child(i, core)) {

            return true;
        }
    }

    return false;
}
#endif
