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

