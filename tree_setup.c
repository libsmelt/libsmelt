#include <stdio.h>

#include "internal.h"
#include "sync.h"
#include "topo.h"
#include "barrier.h"

enum state {
    STATE_NONE,
    STATE_EXPORTED,
};

enum state qrm_st[Q_MAX_CORES];

__thread struct sync_binding *_bindings[Q_MAX_CORES];
__thread uint32_t tree_num_peers;

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
}

/* // Add a binding */
/* void add_binding(struct sync_binding *b, coreid_t core) */
/* { */
/*     QDBG("ADD_BINDING on core %d for partner %d\n", get_thread_id(), core); */
/*     printf("EDGE: %d -> %d\n", get_thread_id(), core); */

/*     assert(core>=0 && core<get_num_threads()); */
/*     assert(core!=get_thread_id()); */

/*     _tree_register_receive_handler(b); */
    
/*     // Remember the binding */
/*     _bindings[core] = b; */
/* } */
