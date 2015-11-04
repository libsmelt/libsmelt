#include "sync.h"

__thread struct waitset setup_ws;
__thread struct waitset *tree_ws;


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
void _tree_export(const char *qrm_my_name)
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


/**
 * \brief Prepare for setting up the tree.
 *
 * Currently:
 *  - Initialize waitsets
 *  - Allocate memory for bindings
 */
void _tree_prepare(void)
{
    waitset_init(&setup_ws);
    tree_ws = &setup_ws;
}

void _tree_register_receive_handler(struct sync_binding *b)
{
    // Set receive handlers
    b->rx_vtbl = syn_rx_vtbl;
}


/**
 * \brief Round-based connection setup of tree
 */
void tree_connect(const char *qrm_my_name)
{
    // Barrier to make sure that tree setup messages will not be
    // processed BEFORE we are ready for them

    // This is only an issue on Barrelfish, where we have processes
    // and they need to rendezvous for binding.
    qrm_exp_round();

    debug_printf("building the tree %s\n", qrm_my_name);

    for (int round=0; round<get_num_threads(); round++) {

        QDBG_INIT("tree_init: core %d in round %d value %d\n",
                     my_core_id, round, model_get(round, my_core_id));

        if (my_core_id==round) {
            QDBG_INIT("tree_init: round %d on %d: connecting to other clients .. \n",
                 round, my_core_id);

            tree_bind_to_children();

        } else if (model_is_parent(round, my_core_id)) {
            QDBG_INIT("tree_init: round %d on %d: waiting for connection .. \n",
                 round, my_core_id);

            tree_wait_connection(round);

        } else {
            QDBG_INIT("tree_init: round %d on %d: nothing to do, sleeping .. \n",
                 round, my_core_id);

        }

        QDBG_INIT("round %d completed, waiting for others .. \n", round);
        qrm_exp_round();
    }

    QDBG_INIT("tree_init: all rounds completed\n");

#if !defined(EXP_HYBRID)
    PRINTFFF("tree_sanity_check\n");
    tree_sanity_check();
#endif

    // Wait for everyone
    char *dummy;
    oct_barrier_enter("qrm_up", &dummy, get_num_threads());
    oct_barrier_leave(dummy);
}
