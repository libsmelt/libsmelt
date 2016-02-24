/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#include <smlt.h>
#include <smlt_queuepair.h>

/* backends */
#include <shm/smlt_shm.h>


/**
  * @brief creates the queue pair
  *
  * @param TODO: information specification
  *
  * @returns 0
  */
errval_t smlt_queuepair_create(smlt_ep_type_t type,
                               struct smlt_qp *qp)
{

    switch(type) {
        case SMLT_EP_TYPE_UMP :
            break;
        case SMLT_EP_TYPE_FFQ :
            break;
        case SMLT_EP_TYPE_SHM :
            break;
        default:
            break;
    }

    return SMLT_SUCCESS;
}

/**
 * @brief destroys the queuepair
 *
 * @param
 *
 * @returns 0
 */
errval_t smlt_queuepair_destroy(struct smlt_qp *qp)
{
    return SMLT_SUCCESS;
}
