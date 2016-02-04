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
