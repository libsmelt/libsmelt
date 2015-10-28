/* #include <octopus/octopus.h> */
#include <stdio.h>
/* #include <tools/tools.h> */
/* #include "model_defs.h" */

#include <barrelfish_kpi/types.h>
#include <barrelfish/barrelfish.h>
#include <barrelfish/waitset.h>
#include <barrelfish/nameservice_client.h>

#include "sync/internal.h"
#include "sync/sync.h"
#include "sync/topo.h"
#include "sync/barrier.h"

#include <if/sync_defs.h>


enum state {
    STATE_NONE,
    STATE_EXPORTED,
};

enum state qrm_st[Q_MAX_CORES];

__thread struct sync_binding *_bindings[Q_MAX_CORES];
__thread uint32_t tree_num_peers;

__thread struct waitset setup_ws;
__thread struct waitset *tree_ws;

__thread int num_acks;

struct shl_barrier_t tree_setup_barrier;

void add_binding(struct sync_binding *b, coreid_t core);

/**
 * \brief This file provides functionality to setup the tree.
 *
 * It parses the tree model as given in quorum.h and
 * quorum_def.h. Note that here, we are only concerned with setting up
 * all message channels.
 *
 * The policy given in the implementation later on (e.g. exp_ab) will
 * then decide the scheduling, i.e. the order in which to send and
 * receive messages from children/parents in the tree. That is also
 * where waitsets are going to be set up.
 *
 */

/* /\* */
/*  * List of bindings, element i is the binding to the quorum app on core i */
/*  *\/ */
/* uint8_t client_up = 0; */


__thread coreid_t connect_cb_core_id;

/**
 * \brief Prepare for setting up the tree.
 *
 * Currently:
 *  - Initialize waitsets
 *  - Allocate memory for bindings
 */
static void _tree_prepare(void)
{
    waitset_init(&setup_ws);
    tree_ws = &setup_ws;
}

static errval_t connect_cb(void *st, struct sync_binding *b)
{
    debug_printf("connect cb called .. assuming it came from %d\n",
                 connect_cb_core_id);

    assert(topo_has_edge(connect_cb_core_id));

    add_binding(b, connect_cb_core_id);
    tree_num_peers++;

    return SYS_ERR_OK;
};

__thread iref_t tree_iref;

static void export_cb(void *st, errval_t err, iref_t iref)
{
    debug_printf("Export successful on \n");
    int _tid = *((int*) st);

    // Mark as exported AFTER talking to the nameserver
    tree_iref = iref;
    qrm_st[_tid] = STATE_EXPORTED;

    debug_printf("Export successful on %d -%d - %p - %p\n",
                 _tid,
                 qrm_st[_tid], &qrm_st, (void*) thread_self);
}


static errval_t sync_export_ump(void *st,
                                idc_export_callback_fn *export_cb_,
                                sync_connect_fn *connect_cb_,
                                struct waitset *ws,
                                idc_export_flags_t flags)
{
    struct sync_export_ *e = malloc(sizeof(struct sync_export_ ));
    if (e == NULL) {
        return(LIB_ERR_MALLOC_FAIL);
    }

    // fill in common parts of export struct
    e->connect_cb = connect_cb_;
    e->waitset = ws;
    e->st = st;
    (e->common).export_callback = export_cb_;
    (e->common).flags = flags;
    (e->common).connect_cb_st = e;
    (e->common).export_cb_st = st;

    (e->common).ump_connect_callback = sync_ump_connect_handler;

    return(idc_export_service(&(e->common)));
}


/**
 * \brief Export quorum instance to nameserver
 */
static void _tree_export(const char *qrm_my_name)
{
    errval_t err;

    struct waitset ws;
    waitset_init(&ws);

    int _tid = get_thread_id();

    if (_tid == 0)
        _tid = 12;

    qrm_st[_tid] = STATE_NONE;

    void *st = &_tid;
    debug_printf("Doing tree export with state being %p\n", st);

    err = sync_export_ump(st,
                          export_cb,
                          connect_cb,
                          &ws,
                          IDC_EXPORT_FLAGS_DEFAULT);
    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "export failed");
    }
    debug_printf("Exported sync interface - tid %d - %d - %p - %p\n",
                 _tid, qrm_st[_tid], &qrm_st[_tid], thread_self());

    while (qrm_st[_tid]!=STATE_EXPORTED) {
        err = event_dispatch(&ws);
        if (err_is_fail(err)) {
            DEBUG_ERR(err, "in event_dispatch");
            break;
        }
    }

    debug_printf("Export successfull, registering with nameservice\n");
    err = nameservice_register(qrm_my_name, tree_iref);

    if (err_is_fail(err)) {
        USER_PANIC_ERR(err, "nameservice_register failed");
    }

    // Wait for everyone to export ..
    shl_barrier_wait(&tree_setup_barrier);

    debug_printf("tree_export completed .. \n");
}

/* static void bind_cb(void *st, errval_t binderr, struct sync_binding *b) */
/* { */
/*     DEBUG_ERR(binderr, "tree_bind_cb"); */
/*     debug_printf("bind_cb\n"); */

/*     int c = *((int*) st); */
/*     add_binding(b, c); */

/*     tree_num_peers++; */

/*     QDBG("bind succeeded on %d for core %d\n", my_core_id, c); */
/* } */

/* /\** */
/*  * \brief Connect to children in tree */
/*  * */
/*  * - nameserver lookup */
/*  * - bind */
/*  * - wait for connect event */
/*  *\/ */
/* static void tree_bind_and_wait(coreid_t core) */
/* { */
/*     debug_printf("connecting to %d\n", core); */

/*     int num_before = tree_num_peers; */

/*     // Nameserver lookup */
/*     iref_t iref; errval_t err; */
/*     char q_name[1000]; */
/*     sprintf(q_name, "sync%d", core); */
/*     err = nameservice_blocking_lookup(q_name, &iref); */
/*     if (err_is_fail(err)) { */
/*         DEBUG_ERR(err, "nameservice_blocking_lookup failed"); */
/*         abort(); */
/*     } */

/*     debug_printf("lookup to nameserver succeeded .. binding \n"); */

/*     // Bind */
/*     err = sync_bind(iref, bind_cb, &core, tree_ws, */
/*                       IDC_BIND_FLAGS_DEFAULT);//|IDC_BIND_FLAG_NUMA_AWARE); */
/*     if (err_is_fail(err)) { */
/*         DEBUG_ERR(err, "bind failed"); */
/*         abort(); */
/*     } */

/*     debug_printf("bind returned .. \n"); */

/*     // Wait for client to connect */
/*     do { */
/*         err = event_dispatch(tree_ws); */
/*         if (err_is_fail(err)) { */
/*             USER_PANIC_ERR(err, "error in qrm_init when binding\n"); */
/*         } */
/*     } while(num_before == tree_num_peers); */
/* } */

/* /\* */
/*  * \brief Find children and connect to them */
/*  * */
/*  *\/ */
/* static void tree_bind_to_children(void) */
/* { */
/*     for (coreid_t node=0; node<get_num_threads(); node++) { */

/*         if (model_is_edge(my_core_id, node) && */
/*             !model_is_parent(node, my_core_id)) { */

/*             debug_printf(("tree_bind_to_children: my_core_id %d " */
/*                           "node %d model_is_edge %d model_is_parent %d\n"), */
/*                          my_core_id, node, model_is_edge(my_core_id, node), */
/*                          !model_is_parent(node, my_core_id)); */
/*             tree_bind_and_wait(node); */
/*         } */
/*     } */
/* } */

/* /\* */
/*  * \brief Wait for incoming connection */
/*  * */
/*  * Wait for incoming connection. The binding will be stored as binding */
/*  * to given core. This seems to be a limitation of flunder. There is */
/*  * no sender coreid associated with incoming connections, so we need */
/*  * to get that information from the round of the state machine. */
/*  *\/ */
/* static void tree_wait_connection(coreid_t core) */
/* { */
/*     errval_t err; */
/*     coreid_t old_num = tree_num_peers; */

/*     connect_cb_core_id = core; */

/*     debug_printf("waiting for connections (from %d).. \n", connect_cb_core_id); */

/*     struct waitset *ws = get_default_waitset(); */
/*     while (old_num==tree_num_peers) { */

/*         // XXX is this happening asynchronously? i.e. will the C */
/*         // closure be finished before returning? Otherwise, there is */
/*         // no guarantee that we will actually see the update of */
/*         // tree_num_peers .. and then, we are deadlocked forever */
/*         err = event_dispatch_non_block(ws); */

/*         // Error handling */
/*         if (err_is_fail(err)) { */

/*             if (err_no(err) == LIB_ERR_NO_EVENT) { */

/*                 // No event, just try again .. */
/*             } */

/*             else { */

/*                 // Any other error is fatal .. */
/*                 DEBUG_ERR(err, "in event_dispatch"); */
/*                 USER_PANIC("tree_wait_connection failed\n"); */
/*             } */
/*         } */
/*     } */
/* } */

/* /\** */
/*  * \brief Round-based connection setup of tree */
/*  *\/ */
/* void tree_connect(const char *qrm_my_name) */
/* { */
/*     // Barrier to make sure that tree setup messages will not be */
/*     // processed BEFORE we are ready for them */
/*     qrm_exp_round(); */

/*     debug_printf("building the tree %s\n", qrm_my_name); */

/*     for (int round=0; round<get_num_threads(); round++) { */

/*         QDBG_INIT("tree_init: core %d in round %d value %d\n", */
/*                      my_core_id, round, model_get(round, my_core_id)); */

/*         if (my_core_id==round) { */
/*             QDBG_INIT("tree_init: round %d on %d: connecting to other clients .. \n", */
/*                  round, my_core_id); */

/*             tree_bind_to_children(); */

/*         } else if (model_is_parent(round, my_core_id)) { */
/*             QDBG_INIT("tree_init: round %d on %d: waiting for connection .. \n", */
/*                  round, my_core_id); */

/*             tree_wait_connection(round); */

/*         } else { */
/*             QDBG_INIT("tree_init: round %d on %d: nothing to do, sleeping .. \n", */
/*                  round, my_core_id); */

/*         } */

/*         QDBG_INIT("round %d completed, waiting for others .. \n", round); */
/*         qrm_exp_round(); */
/*     } */

/*     QDBG_INIT("tree_init: all rounds completed\n"); */

/* #if !defined(EXP_HYBRID) */
/*     PRINTFFF("tree_sanity_check\n"); */
/*     tree_sanity_check(); */
/* #endif */

/*     // Wait for everyone */
/*     char *dummy; */
/*     oct_barrier_enter("qrm_up", &dummy, get_num_threads()); */
/*     oct_barrier_leave(dummy); */
/* } */

/* static void __attribute__((unused))  tree_sanity_check(void) */
/* { */
/*     // SANITY CHECK */
/*     // Make sure all bindings as suggested by the model are set up */
/*     for (int i=0; i<get_num_threads(); i++) { */
/*         if (model_has_edge(i)) { */
/*             if (qrm_binding[i]==NULL) { */

/*                 printf("ERROR: model_has_edge(%d), but binding %d " */
/*                        "missing on core %d\n", i, i, my_core_id); */

/*                 assert(!"Binding for edge in model missing"); */
/*             } */
/*         } */
/*     } */
/* } */

/*
 * \brief Initialize a broadcast tree from a model.
 *
 */
int tree_init(const char *qrm_my_name)
{
    _tree_prepare();
    num_acks = 0;

    if (get_thread_id()==0) {

        shl_barrier_init(&tree_setup_barrier, NULL, get_num_threads());
    }

    _tree_export(qrm_my_name);

    return 0;
}

/**
 * \brief Reset the tree.
 *
 * This is useful when switching trees.
 */
void tree_reset(void)
{
    tree_num_peers = 0;
    assert (_bindings!=NULL);

    /* for (int i=0; i<Q_MAX_CORES; i++) { */
    /*     _bindings[i] = NULL; */
    /* } */
}

// --------------------------------------------------
// Binding manipulations
// --------------------------------------------------

static void q_request(struct sync_binding *b, uint32_t id, uint32_t id2)
{
    // Echo it back!
    errval_t err = sync_ack__tx(b, NOP_CONT, get_thread_id(), id);
    assert(err_is_ok(err));

    QDBG("q_request\n");
}

static void q_ack(struct sync_binding *b, uint32_t id, uint32_t id2)
{
    QDBG("q_ack\n");
    num_acks++;
}

struct sync_rx_vtbl syn_rx_vtbl = {
    .request = &q_request,
    .ack = &q_ack
};

// Add a binding
void add_binding(struct sync_binding *b, coreid_t core)
{
    QDBG("ADD_BINDING on core %d for partner %d\n", get_thread_id(), core);
    printf("EDGE: %d -> %d\n", get_thread_id(), core);

    assert(core>=0 && core<get_num_threads());
    assert(core!=get_thread_id());

    // Set receive handlers
    b->rx_vtbl = syn_rx_vtbl;
    // Remember the binding
    _bindings[core] = b;
}
