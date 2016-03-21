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
#include <smlt_platform.h>

/* backends */
#include <backends/ffq/ff_queuepair.h>
#include <backends/ump/smlt_ump_queuepair.h>
#include <backends/shm/shm_qp.h>
#include "qp_func_wrapper.h"
#include "debug.h"

/**
  * @brief creates the queue pair
  *
  * @param TODO: information specification
  *
  * @returns 0
  */
errval_t smlt_queuepair_create(smlt_qp_type_t type,
                               struct smlt_qp **qp1,
                               struct smlt_qp **qp2,
                               coreid_t core_src,
                               coreid_t core_dst)
{
    errval_t err;

    SMLT_DEBUG(SMLT_DBG__GENERAL, "creating qp src=%" PRIu32 " dst =% " PRIu32 " \n",
               core_src, core_dst);
    // TODO what is really a queuepair ?
    assert(qp1);
    assert(qp2);

    uint8_t src_affinity = smlt_platform_cluster_of_core(core_src);
    uint8_t dst_affinity = smlt_platform_cluster_of_core(core_dst);

    struct smlt_qp *qp_src = smlt_platform_alloc_on_node(sizeof (*qp_src),
                                                         SMLT_ARCH_CACHELINE_SIZE,
                                                         src_affinity, true);

    struct smlt_qp *qp_dst = smlt_platform_alloc_on_node(sizeof (*qp_dst),
                                                         SMLT_ARCH_CACHELINE_SIZE,
                                                         dst_affinity, true);



    (qp_src)->type = type;
    (qp_dst)->type = type;
    switch(type) {
        case SMLT_QP_TYPE_UMP :
            err = smlt_ump_queuepair_init(SMLT_UMP_DEFAULT_SLOTS,
                                          src_affinity, dst_affinity,
                                          &(qp_src)->q.ump, &(qp_dst)->q.ump);
            if (smlt_err_is_fail(err)) {
                return err;
            }

            // set function pointers
            (qp_src)->f.send.try_send = smlt_ump_queuepair_try_send;
            (qp_src)->f.send.notify = smlt_ump_queuepair_notify;
            (qp_src)->f.send.can_send = smlt_ump_queuepair_can_send;
            (qp_src)->f.recv.try_recv = smlt_ump_queuepair_try_recv;
            (qp_src)->f.recv.can_recv = smlt_ump_queuepair_can_recv;
            (qp_src)->f.recv.notify = smlt_ump_queuepair_recv_notify;
            (qp_dst)->f = (qp_src)->f;

            break;
        case SMLT_QP_TYPE_FFQ :
            // create two queues
            (qp_src)->queue_tx.ffq = *ff_queuepair_create(core_dst);
            (qp_src)->queue_rx.ffq = *ff_queuepair_create(core_src);

            (qp_dst)->queue_tx.ffq = (qp_src)->queue_rx.ffq;
            (qp_dst)->queue_rx.ffq = (qp_src)->queue_tx.ffq;

            // set function pointers
            (qp_src)->f.send.try_send = smlt_ffq_send;
            (qp_src)->f.send.notify = smlt_ffq_send0;
            (qp_src)->f.send.can_send = smlt_ffq_can_send;
            (qp_src)->f.recv.try_recv = smlt_ffq_recv;
            (qp_src)->f.recv.can_recv = smlt_ffq_can_recv;
            (qp_src)->f.recv.notify = smlt_ffq_recv0;

            (qp_dst)->f = (qp_src)->f;
            break;
        case SMLT_QP_TYPE_SHM :
            // create two queues
            (qp_src)->queue_tx.shm = *shm_queuepair_create(core_src, core_dst);
            (qp_src)->queue_rx.shm = *shm_queuepair_create(core_dst, core_src);

            (qp_dst)->queue_rx.shm = (qp_src)->queue_tx.shm;
            (qp_dst)->queue_tx.shm = (qp_src)->queue_rx.shm;

            // set function pointers
            (qp_src)->f.send.try_send = smlt_shm_send;
            (qp_src)->f.send.notify = smlt_shm_send0;
            (qp_src)->f.send.can_send = smlt_shm_can_send;
            (qp_src)->f.recv.try_recv = smlt_shm_recv;
            (qp_src)->f.recv.can_recv = smlt_shm_can_recv;
            (qp_src)->f.recv.notify = smlt_shm_recv0;

            (qp_dst)->f = (qp_src)->f;
            break;
        default:
            break;
    }

    *qp1 = qp_src;
    *qp2 = qp_dst;

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
    //int ret;
    switch(qp->type) {
        case SMLT_QP_TYPE_UMP :
            // TODO destroy always returns 0
            //ret = ump_queuepair_destroy(&(qp->queue_tx.ump));
            //ret = ump_queuepair_destroy(&(qp->queue_rx.ump));
            //if (ret) {
                return SMLT_ERR_DESTROY_UMP;
            //}
            break;
        case SMLT_QP_TYPE_FFQ :
            // TODO do destroy
            break;
        case SMLT_QP_TYPE_SHM :
            // TODO do destroy
            break;
        default:
            break;
    }

    return SMLT_SUCCESS;
}
