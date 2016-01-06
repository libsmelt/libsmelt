#include "sync.h"
#include "topo.h"

// Include the pre-generated model
#include "model.h"

#include <vector>
#include <algorithm>

// --------------------------------------------------
// Data structures - shared

int *topo = NULL;
static int topo_idx = -1;
static int model_generated = 0;

// --------------------------------------------------
// Functions

void init_topo(void);

/**
 * \brief Move on to the next topo
 *
 * Switching the topo is a global operations. It requires a barrier
 * before and after switching.
 *
 * This does not actually reinitialize any of the algorithms working
 * on top of the topo. In order for the topo switch to be
 * successful, the message passing tree as well as the shared memory
 * queues have to be reinitialized.
 *
 * When using threads, all threads share the same topology. In that
 * case, only the sequentializer thread is going to do the switch.
 */
void switch_topo(void)
{
    bool res = switch_topo_to_idx(topo_idx+1);
    if (!res) {
        assert(!res);
    }
}

static void _debug_print_curr_model(void)
{
#ifdef QRM_DBG_ENABLED

    printf("Model start:");
    for (unsigned i=0; i<topo_num_cores(); i++) {
        for (unsigned j=0; j<topo_num_cores(); j++) {
            if (j==0)
                printf("\n");
            printf("%2d ", topo_get(i, j));
        }
    }
    printf("\nModel end\n");

#endif
}

/**
 * \brief Build a binary tree model for the current machine.
 *
 */
static void _build_model(coreid_t num_threads)
{
    int *_topo = (int*) (malloc(sizeof(int)*(num_threads*num_threads)));

    printw("No model given, building a binary tree for machine\n");

    assert (_topo!=NULL);
    memset(_topo, 0, sizeof(int)*num_threads*num_threads);

    // Fill model
    for (unsigned i=0; i<num_threads; i++) {

        coreid_t id1 = 2*i+1;
        coreid_t id2 = 2*i+2;

        if (id1<num_threads)
            _topo[i*num_threads + id1] = 1;
        if (id2<num_threads)
            _topo[i*num_threads + id2] = 2;

        debug_printfff(DBG__SWITCH_TOPO,
                       "Node %d sending to %d and %d\n", i, id1, id2);

        coreid_t parent = (i-1) / 2;
        debug_printfff(DBG__SWITCH_TOPO,
                       "Parent of %d is %d\n", i, parent);

        if (i!=0) {

            _topo[i*num_threads + parent] = 99;
        }
    }

    // Debug output of model
    printf("Model start:");
    for (unsigned i=0; i<num_threads; i++) {
        for (unsigned j=0; j<num_threads; j++) {
            if (j==0)
                printf("\n");
            printf("%2d ", _topo[i*num_threads + j]);
        }
    }
    printf("\nModel end\n");

    // Topology names
    topo_names = new char*[1];
    topo_names[0] = const_cast<char*>("binary (auto-generated)");

    topo_combined = (int**) (malloc(sizeof(int*)));
    assert (topo_combined!=NULL);
    topo_combined[0] = _topo;

    // Leaf nodes
    std::vector<int> *_leaf_nodes = new std::vector<int>;
    _leaf_nodes->push_back(num_threads-1);

    all_leaf_nodes = (std::vector<int>**) malloc(sizeof(std::vector<int>*));
    assert (all_leaf_nodes!=NULL);

    all_leaf_nodes[0] = _leaf_nodes;

    // Last node
    last_nodes.push_back(num_threads-1);

    model_generated = 1;
}

/**
 * \brief Switch topology according to given index.
 *
 * \param idx Index of the topology to be used.
 *
 * \return true if successful, false otherwise (e.g. in case of
 * invalid index idx)
 */
bool switch_topo_to_idx(int idx)
{
    // Build model if non is configured
    if (NUM_TOPOS==0) {

        _build_model(get_num_threads());
    }


    if ((unsigned) idx>=topo_num_topos()) {

        printw("Cannot switch topology: index %d invalid\n", idx);
        return false;
    }

    topo_idx = idx;
    if (get_core_id() == get_sequentializer()) {
        debug_printfff(DBG__SWITCH_TOPO, "Available topos: %d out of %d\n",
                     topo_idx, NUM_TOPOS);

        for (unsigned i=0; i<topo_num_topos(); i++) {

            debug_printfff(DBG__SWITCH_TOPO, "topo %d %p %s [%c]\n",
                         i, topo_combined[i], topo_names[i],
                         i==topo_idx ? 'X' : ' ' );
        }

        debug_printf("Switching topology: \033[1;36m%s\033[0m\n",
                     topo_get_name());
        debug_printf("last node is: %d\n", topo_last_node());
    }

    topo = topo_combined[topo_idx];
    _debug_print_curr_model();

    debug_printfff(DBG__SWITCH_TOPO, "test-accessing topo .. %d %d\n",
                   topo_get(0, 0),
                   topo_get(topo_num_cores()-1, topo_num_cores()-1));

    assert(topo_last_node()!=get_sequentializer()); // LAST_NODE switched with topo
    return true;
}

/**
 * \brief Get the currently activated topology index
 *
 * \return Topology index or -1 if nothing selected
 */
int get_topo_idx(void)
{
    return topo_idx;
}

void init_topo(void)
{

#if defined(EXP_MP) || defined(EXP_HYBRID)
    debug_printfff(DBG__SWITCH_TOPO, "init_topo: doing MP init\n");
    tree_init(qrm_my_name);

    // Reset tree
    tree_reset();
    tree_connect(qrm_my_name);
#endif /* EXP_MP */

#if defined(EXP_SHM)
    // Reset shared memory part
    shm_switch_topo(topo_idx);
#endif
}

/**
 * \brief Determine if a node is a leaf node in the current model.
 *
 * Warning: Iterates C++ vectors, might be slow.
 */
bool topo_is_leaf_node(coreid_t core)
{
    // Leaf nodes of current model
    std::vector<int> *leafs = all_leaf_nodes[topo_idx];

    return std::find(leafs->begin(), leafs->end(), core) != leafs->end();
}

/**
 * \brief Determine if node2 is a child of node1
 *
 * Is there an edge node1 -> node2
 */
bool topo_is_child(coreid_t node1, coreid_t node2)
{
    return (topo_get(node1, node2)>0 && topo_get(node1, node2)<SHM_SLAVE_START);
}


int topo_is_parent_real(coreid_t core, coreid_t parent)
{
    bool is_parent = topo_get(parent, core) == 99;

    // Sanity check for model consistency
    assert (!is_parent || topo_is_child(core, parent));

    return is_parent;
}


/**
 * \brief Check whether a node sends messages according to the current
 * model.
 *
 * If <core> does an mp_send, there is at least one other node that
 * has <core> as a parent.
 *
 * \param include_leafs Consider communication from leaf nodes to
 * sequentializer as message passing links
 */
bool topo_does_mp_send(coreid_t core, bool include_leafs)
{
    // All last nodes will send messages
    if (topo_is_leaf_node(core) && include_leafs) {
        return true;
    }

    // Is the node a parent of any other node?
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_is_child(core, i)) {
            return true;
        }
    }

    return false;
}

/**
 * \brief Check whether a node receives messages according to the
 * current model.
 *
 * \param include_leafs Consider communication from leaf nodes to
 * sequentializer as message passing links
 */
bool topo_does_mp_receive(coreid_t core, bool include_leafs)
{
    // The sequentializer has to receive messages
    if (core == get_sequentializer() && include_leafs) {
        return true;
    }

    // Is the node receiving from any other node
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_is_child(i, core)) {

            return true;
        }
    }

    return false;
}

const char* topo_get_name(void)
{
    return topo_names[get_topo_idx()];
}

/*
bool topo_does_shm_send(coreid_t core)
{
    for (int i=0; i<topo_num_cores(); i++) {

        if (topo_get(core, i)>=SHM_MASTER_START &&
            topo_get(core, i)<SHM_MASTER_MAX) {

            assert(cluster_share != NULL);
            return true;
        }
    }

    return false;
}

bool topo_does_shm_receive(coreid_t core)
{
    for (int i=0; i<topo_num_cores(); i++) {

        if (topo_get(core, i)>=SHM_SLAVE_START &&
            topo_get(core, i)<SHM_SLAVE_MAX) {

            assert(cluster_share != NULL);
            return true;
        }
    }

    return false;
}
*/

static int __topo_get(int* mod, int x, int y)
{
#ifdef QRM_DBG_ENABLED
    assert ((x*topo_num_cores()+y)<(topo_num_cores()*topo_num_cores()));
#endif
    return mod[x*topo_num_cores()+y];
}

int topo_get(int x, int y)
{
    assert (topo);
    return __topo_get(topo, x, y);
}

int topos_get(int mod, int x, int y)
{
    return __topo_get(topo_combined[mod], x, y);
}

unsigned int topo_num_cores(void)
{
    return model_generated ? get_num_threads() : TOPO_NUM_CORES;
}

unsigned int topo_num_topos(void)
{
    return model_generated ? 1 : NUM_TOPOS;
}

coreid_t get_sequentializer(void)
{
    return SEQUENTIALIZER;
}

coreid_t topo_last_node(void)
{
    return last_nodes[get_topo_idx()];
}

std::vector<int> **topo_all_leaf_nodes(void)
{
    return all_leaf_nodes;
}
