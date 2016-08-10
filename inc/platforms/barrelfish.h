/*
 * Copyright (c) 2013-2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#include <barrelfish/barrelfish.h>
#include <barrelfish_kpi/types.h>

//#include <barrelfish/waitset.h>
//#include <barrelfish/ump_impl.h>

#include <bench/bench.h>

//#include <if/sync_defs.h>
//#include <barrelfish/nameservice_client.h>

#ifdef FFQ

#include "ff_queue.h"
#include "ffq_conf.h"

typedef struct ffq_pair_state mp_binding;

#else // UMP
#include "ump_conf.h"
#include "ump_common.h"

typedef struct ump_pair_state mp_binding;

#endif

#define UMP_QUEUE_SIZE 1000U
