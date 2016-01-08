#include <stdio.h>

#include "internal.h"
#include "sync.h"
#include "topo.h"
#include "barrier.h"
#include "topo.h"
#include "mp.h"
#include "model_defs.h"

enum state {
    STATE_NONE,
    STATE_EXPORTED,
};

enum state qrm_st[Q_MAX_CORES];


/**
 * \brief Store UMP bindings
 *
 * This is currently not clean, as it requires n^2 entries in the
 * buffer for n cores.
 */
mp_binding** bindings = NULL;

__thread uint32_t tree_num_peers;

__thread int num_acks;

struct shl_barrier_t tree_setup_barrier;

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
 * \brief Initialize empty bindings.
 *
 */
void tree_init_bindings(void)
{
    if (bindings==NULL) {

        debug_printfff(DBG__BINDING, "Allocating memory for bindings\n");
        bindings = (mp_binding**) calloc(sizeof(mp_binding*),
                                         (get_num_threads()*get_num_threads()));
        assert (bindings!=NULL);
    }
}

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

void add_binding(coreid_t sender, coreid_t receiver, mp_binding *b)
{
    if (bindings==NULL) {

        debug_printfff(DBG__BINDING, "Allocating memory for bindings\n");
        bindings = (mp_binding**) calloc(sizeof(mp_binding*), (get_num_threads()*get_num_threads()));
        assert (bindings!=NULL);
    }
    
    debug_printfff(DBG__BINDING,
                   "Adding binding for %d, %d\n", sender, receiver);
    bindings[sender*get_num_threads()+receiver] = b;
}

mp_binding* get_binding(coreid_t sender, coreid_t receiver)
{
    debug_printfff(DBG__BINDING,
                   "%d Getting binding for %d, %d\n", get_thread_id(), sender, receiver);
    
    if (!bindings)
        return NULL;
    
    return bindings[sender*get_num_threads()+receiver];
}

struct binding_lst *child_bindings = NULL;
struct binding_lst *parent_bindings = NULL;

/**
 * \brief Build a broadcast tree based on the model
 *
 * The model is currently given as a matrix, where an integer value
 * !=0 at position x,y represents a link in the overlay network.
 *
 * Parse model array. The list of children will be setup according to
 * the numbers given in the model matrix. The neighbor with the
 * smallest integer value will be scheduled first. Parents are encoded
 * with number 99 in the matrix.
 */
void setup_tree_from_model(void)
{
    // Allocate memory for child and parent binding storage
    if (!child_bindings) {

        // Children
        child_bindings = (struct binding_lst*)
            malloc(sizeof(struct binding_lst)*topo_num_cores());
        assert (child_bindings!=NULL);

        // Parents
        parent_bindings = (struct binding_lst*)
            malloc(sizeof(struct binding_lst)*topo_num_cores());
        assert(parent_bindings!=NULL);
    }
    
    // Sanity check
    if (get_num_threads()!=topo_num_cores()) {
        printf("threads: %d/ topo: %d\n", get_num_threads(), topo_num_cores());
        USER_PANIC("Cannot parse model. Number of cores does not match\n");
    }

    assert (is_coordinator(get_thread_id()));
    
    int* tmp = new int[topo_num_cores()];
    int tmp_parent = -1;
    
    debug_printfff(DBG__BINDING, "tree setup: start ------------------------------\n");
  
    // Find children
    for (coreid_t s=0; s<topo_num_cores(); s++) { // foreach sender

        unsigned num = 0;
        
        for (coreid_t i=1; i<topo_num_cores(); i++) { // + possible outward index
            for (coreid_t r=0; r<topo_num_cores(); r++) {

                // Is this the i-th outgoing connection?
                if (topo_get(s, r) == (int64_t) i) {

                    assert (r>=0);
                    debug_printfff(DBG__BINDING,
                                   "tree setup: found binding(%d) %d->%d with %d\n",
                                   num, s, r, i);

                    tmp[num++] = r;
                }

                if (topo_is_parent_real(r, s)) {

                    tmp_parent = r;
                }
            }
        }

        // Otherwise parent was not found
        assert (tmp_parent >= 0 || s == get_sequentializer()); 

        // Create permanent list of bindings for that core
        mp_binding **_bindings = (mp_binding**) malloc(sizeof(mp_binding*)*num);
        mp_binding **_r_bindings = (mp_binding**) malloc(sizeof(mp_binding*)*num);
        assert (_bindings!=NULL && _r_bindings!=NULL);

        // .. and core IDs
        int *_idx = (int*) malloc(sizeof(int)*num);
        assert (_idx!=NULL);

        // Retrieve and store bindings
        for (unsigned j=0; j<num; j++) {
            
            debug_printfff(DBG__BINDING,
                           "tree setup: adding binding(%d) %d->%d\n",
                           j, s, tmp[j]);
            
            _bindings[j] =   get_binding(s, tmp[j]);
            _r_bindings[j] = get_binding(tmp[j], s);

            assert(_bindings[j]!=NULL && _r_bindings[j]!=NULL);

            _idx[j] = tmp[j];
        }

        // Remember children
        child_bindings[s] = {
            .num = num,
            .b = _bindings,
            .b_reverse = _r_bindings,
            .idx = _idx
        };

        if (tmp_parent>=0) {
            debug_printfff(DBG__INIT, "Setting parent of %d to %d\n", s, tmp_parent);

            mp_binding **_bindings_p = (mp_binding**) malloc(sizeof(mp_binding*)*2);
            assert (_bindings!=NULL);

            int *_idx_p = (int*) malloc(sizeof(int)*1);
            assert (_idx_p!=NULL);

            // Binding to parent in both directions
            _bindings_p[0] = get_binding(tmp_parent, s);
            _bindings_p[1] = get_binding(s, tmp_parent);
            _idx_p[0] = tmp_parent;
            
            parent_bindings[s] = (struct binding_lst) {
                .num = 1,
                .b = _bindings_p,
                .b_reverse = _bindings_p + 1,
                .idx = _idx_p
            };
        }
        else {
            int *_idx_p = (int*) malloc(sizeof(int)*1);
            assert (_idx_p!=NULL);

            _idx_p[0] = -1;
            
            parent_bindings[s] = (struct binding_lst) {
                .num = 0,
                .b = NULL,
                .b_reverse = NULL,
                .idx = _idx_p
            };

        }
    }
    debug_printfff(DBG__BINDING, "tree setup: end --------------------\n");
    delete[] tmp;
}

mp_binding **mp_get_children(coreid_t c, int *num, int **nidx)
{
    *num = child_bindings[c].num;
    if (nidx!=NULL) {
        *nidx = child_bindings[c].idx;
    }
    return child_bindings[c].b;
}

mp_binding *mp_get_parent(coreid_t c, int *nidx)
{
    if (nidx!=NULL) {
        *nidx = parent_bindings[c].idx[0];
    }
    return parent_bindings[c].b[0];
}

binding_lst *_mp_get_children_raw(coreid_t c)
{
    return child_bindings+c;
}

binding_lst *_mp_get_parent_raw(coreid_t c)
{
    return parent_bindings+c;
}
