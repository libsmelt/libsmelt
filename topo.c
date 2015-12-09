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
    assert (switch_topo_to_idx(topo_idx+1));
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
    if (idx>=NUM_TOPOS) {

        printw("Cannot switch topology: index %d invalid\n", idx);
        return false;
    }
    
    topo_idx = idx;
    if (get_core_id() == get_sequentializer()) {
        debug_printfff(DBG__SWITCH_TOPO, "Available topos: %d out of %d\n",
                     topo_idx, NUM_TOPOS);

        for (int i=0; i<NUM_TOPOS; i++) {

            debug_printfff(DBG__SWITCH_TOPO, "topo %d %p %s [%c]\n",
                         i, topo_combined[i], topo_names[i],
                         i==topo_idx ? 'X' : ' ' );
        }
        
        debug_printf("Switching topology: \033[1;36m%s\033[0m\n",
                     topo_get_name());
    }

    topo = topo_combined[topo_idx];
    
    debug_printfff(DBG__SWITCH_TOPO, "test-accessing topo .. %d %d\n",
                   topo_get(0, 0),
                   topo_get(TOPO_NUM_CORES-1, TOPO_NUM_CORES-1));

    assert(get_last_node()!=get_sequentializer()); // LAST_NODE switched with topo
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
    for (int i=0; i<TOPO_NUM_CORES; i++) {

        if (topo_is_parent_real(core, i)) {
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
    for (int i=0; i<TOPO_NUM_CORES; i++) {

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
    for (int i=0; i<TOPO_NUM_CORES; i++) {

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
    for (int i=0; i<TOPO_NUM_CORES; i++) {

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
    return mod[x*TOPO_NUM_CORES+y];
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
    return TOPO_NUM_CORES;
}

coreid_t get_sequentializer(void)
{
    return SEQUENTIALIZER;
}
