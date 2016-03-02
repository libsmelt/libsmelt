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
#include <backends/ump/ump_queuepair.h>
#include <backends/ump/ump_queue.h>
#include <backends/ffq/ff_queuepair.h>
#include <backends/ffq/ff_queue.h>
#include "qp_func_wrapper.h"
#include "debug.h"
/* ===========================================================
 * UMP wrapper functions
 * ===========================================================
 */

errval_t smlt_ump_send(struct smlt_qp *qp, struct smlt_msg *msg)
{
    if (msg->datalen <= 56) {
        uintptr_t* data = (uintptr_t*) msg->data;
        ump_enqueue(&qp->queue_tx.ump.src.queue, 
                    data[0], data[1], data[2],
                    data[3], data[4], data[5], data[6]);
    } else {
        // TODO Fragment ? Or Bulkload style?
    }
    return SMLT_SUCCESS;
}


errval_t smlt_ump_recv(struct smlt_qp *qp, struct smlt_msg *msg)
{
    // TODO msg struct must be allocated i.e. where to encode msg 
    //      size that is allocated
    // TODO Fragmentation
    uintptr_t* data = (uintptr_t*) msg->data;
    ump_dequeue(&qp->queue_rx.ump.dst.queue, data);
    return SMLT_SUCCESS;
}


errval_t smlt_ump_recv0(struct smlt_qp *qp)
{
    ump_dequeue_zero(&qp->queue_rx.ump.dst.queue);
    return SMLT_SUCCESS;
}

errval_t smlt_ump_send0(struct smlt_qp *qp)
{
    ump_enqueue_zero(&qp->queue_tx.ump.src.queue);
    return SMLT_SUCCESS;
}

bool smlt_ump_can_recv(struct smlt_qp *qp)
{
    return ump_can_dequeue(&qp->queue_rx.ump.dst.queue);
}

bool smlt_ump_can_send(struct smlt_qp *qp)
{
    return ump_can_enqueue(&qp->queue_tx.ump.src.queue);
}

/* ===========================================================
 * FFQ wrapper functions
 * ===========================================================
 */

errval_t smlt_ffq_send(struct smlt_qp *qp, struct smlt_msg *msg)
{
    if (msg->datalen <= 56) {
        uintptr_t* data = (uintptr_t*) msg->data;
        ffq_enqueue_full(&qp->queue_tx.ffq.src, 
                         data[0], data[1], data[2],
                         data[3], data[4], data[5], data[6]);
    } else {
        // TODO Fragment ? Or Bulkload style?
    }
    return SMLT_SUCCESS;
}


errval_t smlt_ffq_recv(struct smlt_qp *qp, struct smlt_msg *msg)
{
    // TODO msg struct must be allocated i.e. where to encode msg 
    //      size that is allocated
    // TODO Fragmentation
    uintptr_t* data = (uintptr_t*) msg->data;
    ffq_dequeue_full(&qp->queue_rx.ffq.dst, data);
    return SMLT_SUCCESS;
}

errval_t smlt_ffq_send0(struct smlt_qp *qp)
{
    ffq_enqueue0(&qp->queue_tx.ffq.src);
    return SMLT_SUCCESS;
}


errval_t smlt_ffq_recv0(struct smlt_qp *qp)
{
    ffq_dequeue0(&qp->queue_rx.ffq.dst);
    return SMLT_SUCCESS;
}

bool smlt_ffq_can_recv(struct smlt_qp *qp)
{
    return ffq_can_dequeue(&qp->queue_rx.ffq.dst);
}

bool smlt_ffq_can_send(struct smlt_qp *qp)
{
    return ffq_can_enqueue(&qp->queue_tx.ffq.src);
}

/* ===========================================================
 * SHM wrapper functions
 * ===========================================================
 */
