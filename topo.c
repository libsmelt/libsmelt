#include "sync.h"
#include "topo.h"

// Include the pre-generated model
#include "model.h"

// --------------------------------------------------
// Data structures - shared

int *topo = NULL;
static int topo_idx = -1;


// --------------------------------------------------
// Functions

unsigned int get_topo_idx(void);
void init_topo(void);
int topo_is_edge(coreid_t src, coreid_t dest);
int topo_is_real_edge(coreid_t src, coreid_t dest);
int topo_is_parent_real(coreid_t core, coreid_t parent);
int topo_is_parent(coreid_t core, coreid_t parent);
bool topo_does_mp_send(coreid_t core);
bool topo_does_mp_receive(coreid_t core);
bool topo_does_shm_send(coreid_t core);
bool topo_does_shm_receive(coreid_t core);

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
    topo_idx++;
    assert (topo_idx<NUM_TOPOS);

    if (get_core_id() == SEQUENTIALIZER) {
        debug_printfff(DBG__SWITCH_TOPO, "Available topos: %d out of %d\n",
                     topo_idx, NUM_TOPOS);

        for (int i=0; i<NUM_TOPOS; i++) {

            debug_printfff(DBG__SWITCH_TOPO, "topo %d %p %s [%c]\n",
                         i, topo_combined[i], topo_names[i],
                         i==topo_idx ? 'X' : ' ' );
        }
    }

    topo = topo_combined[topo_idx];
    debug_printfff(DBG__SWITCH_TOPO, "test-accessing topo .. %d %d\n", topo_get(0, 0),
                 topo_get(TOPO_NUM_CORES-1, TOPO_NUM_CORES-1));

    assert(LAST_NODE!=SEQUENTIALIZER); // LAST_NODE switched with topo
}

unsigned int get_topo_idx(void)
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

/*
 * \brief Check if there is message-passing connection in the topo
 * for the given pair of cores
 *
 * Also returns true for connection LAST_NODE <-> SEQUENTIALIZER
 */
int topo_is_edge(coreid_t src, coreid_t dest)
{
    assert(src<get_num_threads());
    assert(dest<get_num_threads());

    // For MP: Node s -> r implies node r -> s
    assert(topo_get(src, dest) == 0 || topo_get(src, dest)>=SHM_SLAVE_START || topo_get(dest, src)>0);
    assert(topo_get(dest, src) == 0 || topo_get(dest, src)>=SHM_SLAVE_START || topo_get(src, dest)>0);

    return (topo_get(src, dest)>0 && topo_get(src, dest)<SHM_SLAVE_START) ||
        (topo_get(src, dest)==99) ||
        (src==SEQUENTIALIZER && dest==LAST_NODE) ||
        (dest==SEQUENTIALIZER && src==LAST_NODE);
}

/*
 * \brief Check if there is message-passing connection in the topo
 * for the given pair of cores
 *
 * Does NOT return true for connection LAST_NODE <-> SEQUENTIALIZER
 */
int topo_is_real_edge(coreid_t src, coreid_t dest)
{
    assert(src<get_num_threads());
    assert(dest<get_num_threads());

    // For MP: Node s -> r implies node r -> s
    assert(topo_get(src, dest) == 0 || topo_get(src, dest)>=SHM_SLAVE_START || topo_get(dest, src)>0);
    assert(topo_get(dest, src) == 0 || topo_get(dest, src)>=SHM_SLAVE_START || topo_get(src, dest)>0);

    return (topo_get(src, dest)>0 && topo_get(src, dest)<SHM_SLAVE_START) ||
        (topo_get(src, dest)==99);
}

int topo_is_parent_real(coreid_t core, coreid_t parent)
{
    return topo_get(parent, core) == 99;
}

/*
 * \brief Check if parent is parent of core.
 *
 * Includes the pseudo-link between LAST_NODE and SEQUENTIALIZER as a link.
 */
int topo_is_parent(coreid_t core, coreid_t parent)
{
    return topo_is_parent_real(core,parent) ||
        (parent==LAST_NODE && core==SEQUENTIALIZER);
}

bool topo_does_mp_send(coreid_t core)
{
    for (int i=0; i<TOPO_NUM_CORES; i++) {

        if (topo_is_real_edge(core, i) && topo_is_parent(core, i)) {

            return true;
        }
    }

    return false;
}

bool topo_does_mp_receive(coreid_t core)
{
    for (int i=0; i<TOPO_NUM_CORES; i++) {

        if (topo_is_real_edge(core, i) && !topo_is_parent(core, i)) {

            return true;
        }
    }

    return false;
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

/*
 * \brief Check if there is an edge in the topo to the given core
 *
 */
int topo_has_edge(coreid_t dest)
{
    return topo_is_edge(get_core_id(), dest);
}

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

int topo_num_cores(void)
{
    return TOPO_NUM_CORES;
}
