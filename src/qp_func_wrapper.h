/*
 * Copyright (c) 2015 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef QP_FUNC_WRAPPER_H
#define QP_FUNC_WRAPPER_H 1

#include <smlt_error.h>

struct smlt_qp;

errval_t smlt_ump_send(struct smlt_qp *qp, struct smlt_msg *msg);
errval_t smlt_ump_recv(struct smlt_qp *qp, struct smlt_msg *msg);
errval_t smlt_ump_send0(struct smlt_qp *qp);
errval_t smlt_ump_recv0(struct smlt_qp *qp);
bool smlt_ump_can_send (struct smlt_qp *qp);
bool smlt_ump_can_recv(struct smlt_qp *qp);


errval_t smlt_ffq_send (struct smlt_qp *qp, struct smlt_msg *msg);
errval_t smlt_ffq_recv(struct smlt_qp *qp, struct smlt_msg *msg);
errval_t smlt_ffq_send0(struct smlt_qp *qp);
errval_t smlt_ffq_recv0(struct smlt_qp *qp);
bool smlt_ffq_can_send (struct smlt_qp *qp);
bool smlt_ffq_can_recv(struct smlt_qp *qp);


errval_t smlt_shm_send (struct smlt_qp *qp, struct smlt_msg *msg);
errval_t smlt_shm_recv(struct smlt_qp *qp, struct smlt_msg *msg);
errval_t smlt_shm_send0(struct smlt_qp *qp);
errval_t smlt_shm_recv0(struct smlt_qp *qp);
bool smlt_shm_can_send (struct smlt_qp *qp);
bool smlt_shm_can_recv(struct smlt_qp *qp);
#endif /* QP_FUNC_WRAPPER_H */
