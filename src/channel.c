/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <smlt_platform.h>
#include <smlt_error.h>
#include <smlt_queuepair.h>
#include <smlt_channel.h>

/*
 * ===========================================================================
 * Smelt channel creation and destruction
 * ===========================================================================
 */

static errval_t smlt_channel_o2o_create(struct smlt_channel** chan,
                                        uint32_t src,
                                        uint32_t dst)
{
    // TODO only ump channels for now 
    errval_t err;
    struct smlt_qp* qp1;
    struct smlt_qp* qp2;

    qp1 = &((*chan)->c.o2o.send);    
    qp2 = &((*chan)->c.o2o.recv);    

    err = smlt_queuepair_create(SMLT_QP_TYPE_UMP,
                                &qp1, &qp2, src, dst);
    
    if (smlt_err_is_fail(err)) {
        return smlt_err_push(err, SMLT_ERR_CHAN_CREATE);
    }
    
    (*chan)->c.o2o.send = *qp1;
    (*chan)->c.o2o.recv = *qp2;
    return SMLT_SUCCESS;
}

static errval_t smlt_channel_o2m_create(struct smlt_channel** chan,
                                        uint32_t src,
                                        uint32_t* dst,
                                        uint16_t count)
{
    (*chan)->c.o2m.m = count;
    (*chan)->c.o2m.backend = SMLT_CHAN_BACKEND_MP;
    (*chan)->c.o2m.last_polled = 0;
    // TODO only ump channels for now 

    struct smlt_qp* qp2;
    struct smlt_qp* qp1;
    struct smlt_qp* qp_array1;
    struct smlt_qp* qp_array2;
    errval_t err;

    qp_array1 = (*chan)->c.o2m.send.qp;
    qp_array2 = (*chan)->c.o2m.send.qp;
    // TODO for now allocate only space for pointers, otherwise
    // have to change smlt_queuepair_create to not allocate memory
    qp_array1 = (struct smlt_qp *) smlt_platform_alloc(sizeof(struct smlt_qp)*count,
                                                     SMLT_DEFAULT_ALIGNMENT, false);
    qp_array2 = (struct smlt_qp *) smlt_platform_alloc(sizeof(struct smlt_qp)*count,
                                                     SMLT_DEFAULT_ALIGNMENT, false);
    for (int i = 0 ; i < count; i++) {
        qp1 = &(qp_array1[i]);
        qp2 = &(qp_array2[i]);
        err = smlt_queuepair_create(SMLT_QP_TYPE_UMP,
                                    &qp1, &qp2, src, dst[i]);
        if (smlt_err_is_fail(err)) {
            return smlt_err_push(err, SMLT_ERR_CHAN_CREATE);
        }

    }

    return SMLT_SUCCESS;
}

 /**
  * @brief creates the queue pair
  *
  * @param type     type of the channel (o2o o2m m2o m2m)
  * @param chan     return pointer to the channel
  * @param src      src core id // TODO use nid ? 
  * @param dst      array of core ids to desinations
  * @param count    length of array dst;   
  *
  * @returns SMLT_SUCCESS or failure 
  */
errval_t smlt_channel_create(smlt_chan_type_t type,
                             struct smlt_channel **chan,
                             uint32_t src,
                             uint32_t* dst,
                             uint16_t count)
{
    *chan = smlt_platform_alloc(sizeof(struct smlt_channel),
                               SMLT_DEFAULT_ALIGNMENT,      
                               true);
    if (!chan) {
        return SMLT_ERR_MALLOC_FAIL;
    }
    (*chan)->type = type;
    switch(type) {
        case SMLT_CHAN_TYPE_ONE_TO_ONE :
            return smlt_channel_o2o_create(chan, src, *dst);
            break;
        case SMLT_CHAN_TYPE_ONE_TO_MANY :
            return smlt_channel_o2m_create(chan, src, dst, count);
            break;
        case SMLT_CHAN_TYPE_MANY_TO_ONE :
            assert(!"NIY");
            break;
        case SMLT_CHAN_TYPE_MANY_TO_MANY :
            assert(!"NIY");
            break;
        default :
            return SMLT_ERR_CHAN_UNKNOWN_TYPE;
            break;
    }
    return SMLT_ERR_CHAN_UNKNOWN_TYPE;
}

 /**
  * @brief destroys the channel
  *
  * @param
  *
  * @returns 0
  */
errval_t smlt_channel_destroy(struct smlt_channel *chan)
{
    switch(chan->type) {
        case SMLT_CHAN_TYPE_ONE_TO_ONE :
            smlt_queuepair_destroy(&chan->c.o2o.send);
            smlt_platform_free(chan);
            break;
        case SMLT_CHAN_TYPE_ONE_TO_MANY :
            for (int i = 0; i < chan->c.o2m.m; i++) {
                smlt_queuepair_destroy(&chan->c.o2m.send.qp[i]);
            }
            smlt_platform_free(chan->c.o2m.send.qp);
            smlt_platform_free(chan->c.o2m.recv.qp);
            smlt_platform_free(chan);
            break;
        case SMLT_CHAN_TYPE_MANY_TO_ONE :
            assert(!"NIY");
            break;
        case SMLT_CHAN_TYPE_MANY_TO_MANY :
            assert(!"NIY");
            break;
        default :
            return SMLT_ERR_CHAN_UNKNOWN_TYPE;
            break;
    }
    return SMLT_SUCCESS;
}
