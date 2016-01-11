#include "sync.h"
#include "shm.h"
#include "model_defs.h"
#include "topo.h"

#ifdef BARRELFISH
#include <barrelfish/barrelfish.h>
#endif

#include <numa.h>

// Master share is shared between all threads.
union quorum_share *master_share = NULL;
#ifdef BARRELFISH
struct capref shared_frame;
#endif

#define get_max_num_clusters(x) (SHM_MASTER_MAX-SHM_MASTER_START)

// Stores the meta data for each clusters. This is indexed by the
// cluster ID. E.g. 53 in the model corresponds to cluster 3, so the
// corresponding meta data stored at index 3 in that array.
struct shm_queue** clusters;

// Stores a reference ID each thread belongs to. Assumes that there is
// only one cluster per thread.
int* thread_clusters;

int init_master_share(void)
{
#ifdef BARRELFISH    
    errval_t err;

    if (master_share)
        return 0;

    // Sanity checks
    assert (SHM_SIZE % BASE_PAGE_SIZE==0);
    assert (sizeof(union quorum_share)<=SHM_SIZE);

    err = frame_alloc(&shared_frame, SHM_SIZE, NULL);
    assert (err_is_ok(err));

    err = vspace_map_one_frame_attr(((void**)&master_share), SHM_SIZE,
                                    shared_frame, VREGION_FLAGS_READ_WRITE,
                                    NULL, NULL);
    assert(err_is_ok(err));

    return !err_is_ok(err);
#else
    if (master_share)
        return 0;

    // Sanity checks
    assert (SHM_SIZE % BASE_PAGE_SIZE==0);
    assert (sizeof(union quorum_share)<=SHM_SIZE);

    master_share = (union quorum_share*) malloc(SHM_SIZE);
    assert (master_share!=NULL);
    
    return 0;
#endif
}

int map_master_share(void)
{
    // This is a no-op if we use threads for parallelism.

    // If processes were used, we would have to map the frame here

    return 0;
}

union quorum_share* get_master_share(void)
{
    return master_share;
}

/**
 * \brief Activate shared memory clusters.
 *
 * This should be executed once on each thread after changing the model.
 *
 * Gets all clusters in all models for this core.
 */
void shm_switch_topo(void)
{
    int num_clusters;
    int *model_ids;
    int *cluster_ids;

    bool mapped = false;

    // Allocate space for mappings
    clusters = (struct shm_queue**) malloc(sizeof(struct shm_queue*)*get_max_num_clusters());
    assert (clusters!=NULL);

    // Store one cluster per thread
    thread_clusters = (int*) malloc(sizeof(int)*topo_num_cores());
    assert (thread_clusters!=NULL);

    // Find all clusters this core is part of in ALL models
    shm_get_clusters_for_core(get_thread_id(), &num_clusters,
                              &model_ids, &cluster_ids);

    debug_printf("shm_switch_topo: number of clusters %d\n", num_clusters);

    for (int cidx=0; cidx<num_clusters; cidx++) {

        int mid = model_ids[cidx];
        int cid = cluster_ids[cidx];

        debug_printfff(DBG__SWITCH_TOPO, "shm_switch_topo: cidx %d mid %d cid %d\n",
                      cidx, mid, cid);

        // Is this cluster part of the current model?
        if (mid==get_topo_idx()) {

            if (shm_get_coordinator_for_cluster(cid)==get_thread_id()) {
                debug_printfff(DBG__SWITCH_TOPO, "shm_switch_topo: Switching model %d master\n",
                               cid);

                // Allocate queue - on NUMA node of coordinator
                void *buf = numa_alloc_local(SHMQ_SIZE);
                assert (buf!=NULL);
                
                int num_readers = topo_mp_cluster_size(get_thread_id(), cid);
                num_readers -= 1; // coordinator is writer

                assert (cid<=num_clusters);
                clusters[cid] = shm_init_context(buf, num_readers, cid);

                thread_clusters[get_thread_id()] == cid;
                
            } else {
                debug_printfff(DBG__SWITCH_TOPO, "shm_switch_topo: Switching model %d client\n",
                               cid);
                
                thread_clusters[get_thread_id()] == cid;
            }
        }
    }

    debug_printf("shm_switch_topo done\n");
}

int shm_does_shm(coreid_t core)
{
    for (int x=0; x<topo_num_cores(); x++) {

        int value = topo_get(core, x);

        if (value>=SHM_MASTER_START && value<SHM_MASTER_MAX) {
            return 1;
        }
        if (value>=SHM_SLAVE_START && value<SHM_SLAVE_MAX) {
            return 1;
        }
    }

    return 0;
}


/**
 * \brief Determine from model if given core is a cluster coordinator
 *
 * \return ID of cluster the given node is coordinator of, otherwise -1
 */
int shm_is_cluster_coordinator(coreid_t core)
{
    int value;
    for (int x=0; x<topo_num_cores(); x++) {

        value = topo_get(core, x);

        if (value>=SHM_MASTER_START && value<SHM_MASTER_MAX && core!=x) {
            return value-SHM_MASTER_START;
        }
    }

    return -1;
}

void shm_get_clusters_for_core (int core, int *num_clusters,
                                int **model_ids, int **cluster_ids)
{
    int i = 0;

    *model_ids = (int*) malloc(sizeof(int)*topo_num_topos()*get_max_num_clusters());
    *cluster_ids = (int*) malloc(sizeof(int)*topo_num_topos()*get_max_num_clusters());

    // Make sure we count each cluster only once. For this, we need
    // state, once bit per possible cluster

    for (int mod=0; mod<topo_num_topos(); mod++) {

        bool clusters_added[get_max_num_clusters()] = {0};

        for (int x=0; x<topo_num_cores(); x++) {

            int value = topos_get(mod, core, x);
            debug_printf("Looking at value %d\n", value);

            if (value>=SHM_MASTER_START && value<SHM_MASTER_MAX) {
                value -= SHM_MASTER_START;
            }

            else if (value>=SHM_SLAVE_START && value<SHM_SLAVE_MAX) {
                value -= SHM_SLAVE_START;
            }

            else {
                value = -1;
            }

            if (value>=0 && !clusters_added[value]) {

                (*model_ids)[i] = mod;
                (*cluster_ids)[i] = value;
                i++;

                clusters_added[value] = true;
                debug_printf("adding cluster model=%d cluster id=%d\n",
                             mod, value);
            }
        }
    }

    *num_clusters = i;
}

int shm_get_coordinator_for_cluster(int cluster)
{
    for (int x=0; x<topo_num_cores(); x++) {
        for (int y=0; y<topo_num_cores(); y++) {

            int value = topo_get(x, y);
            if (value>=SHM_MASTER_START && value<SHM_MASTER_MAX) {

                if (value-SHM_MASTER_START == cluster)
                    return x;
            }
        }
    }
    return -1;
}

void shm_receive(uintptr_t *p1,
                 uintptr_t *p2,
                 uintptr_t *p3,
                 uintptr_t *p4,
                 uintptr_t *p5,
                 uintptr_t *p6,
                 uintptr_t *p7) {

    // Find the cluster
    
    
    shm_receive_raw (clusters[thread_clusters[get_thread_id()]],
                     p1, p2, p3, p4, p5, p6, p7);
}

void shm_send(uintptr_t p1,
              uintptr_t p2,
              uintptr_t p3,
              uintptr_t p4,
              uintptr_t p5,
              uintptr_t p6,
              uintptr_t p7) {

    shm_send_raw (clusters[thread_clusters[get_sequentializer()]],
                     p1, p2, p3, p4, p5, p6, p7);
}
