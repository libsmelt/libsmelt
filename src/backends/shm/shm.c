#include <smlt.h>
#include <shm/smlt_shm.h>
#include "../../internal.h"

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

// Stores pointers to per-cluster structs for reductions. Indexed as
// "clusters"
struct shm_reduction** cluster_reductions;
struct shm_reduction** reductions;

// Stores the buffers to be used for shared memory queues
void** cluster_bufs;

/*
 * We need to remember the round of reductions we are in. Clients can
 * also start a new reduction once the previous is finished.
 *
 * The coordinator increases the current round once it is done summing
 * up all values and resetting counters.
 *
 * Will overflow, but that is okay given that the type of the local
 * and global round variable is the same (uint8)
 */
//static __thread uint8_t red_round = 0; // XXX HAS to be uint8

/**
 * @brief initializes the shared frame for a cluster
 *
 * @return SMLT_SUCCESS
 */
errval_t smlt_shm_init_master_share(void)
{
    SMLT_DEBUG(SMLT_DBG__SHM, "initialize master share\n");
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
    if (master_share) {
        SMLT_DEBUG(SMLT_DBG__SHM, "master share already initialized\n");
        return SMLT_SUCCESS;
    }

    // Sanity checks
    assert (SHM_SIZE % BASE_PAGE_SIZE==0);
    assert (sizeof(union quorum_share)<=SHM_SIZE);

    master_share = smlt_platform_alloc(SHM_SIZE, SMLT_DEFAULT_ALIGNMENT, true);

    if (master_share == NULL) {
        return SMLT_ERR_MALLOC_FAIL;
    }

    SMLT_DEBUG(SMLT_DBG__SHM, "master share initialized to %p\n", master_share);

    return SMLT_SUCCESS;
#endif
}

/**
 * @brief obtains a pointer to the shared frame
 *
 * @return pointer to the shared frame
 */
union quorum_share*smlt_shm_get_master_share(void)
{
    return master_share;
}


int map_master_share(void)
{
    // This is a no-op if we use threads for parallelism.

    // If processes were used, we would have to map the frame here

    return 0;
}


/**
 * \Global initialization
 *
 * Has to be called from exactly one thread.
 */
void shm_init(void)
{
    // Should be called from sequentializer
    //assert (get_thread_id() == get_sequentializer());

#if 0
    // --> per-thread state:
    // Allocate space for shared-memory data structures
    clusters = new struct shm_queue* [topo_num_cores()];
    cluster_reductions = new struct shm_reduction* [topo_num_cores()];

    // --> per-cluster state:
    // Allocate space for buffers
    cluster_bufs = new void* [get_max_num_clusters()];
    reductions = new struct shm_reduction* [get_max_num_clusters()];

    // Allocate memory for shared memory queues
    int num_clusters;
    int *model_ids;
    int *cluster_ids;

    // Find all clusters this core is part of in ALL models
    shm_get_clusters (&num_clusters, &model_ids, &cluster_ids);

    // Allocate shared-memory datastructures, one per cluster
    for (int cidx=0; cidx<num_clusters; cidx++) {

        debug_printfff(DBG__SWITCH_TOPO,
                       "allocating cluster resources for cluster %d\n", cidx);

        // SHM queue writer
        // --------------------------------------------------
        coreid_t coord;
        coord = shm_get_coordinator_for_cluster_in_model(cluster_ids[cidx],
                                                         model_ids[cidx]);


        // Allocate queue - on NUMA node of coordinator
        // That's exactly one page (4096 bytes) and libnuma guarantees that
        // memory will be cache aligned
#ifdef BARRELFISH
        void *buf = numa_alloc_onnode(SHMQ_SIZE*CACHELINE_SIZE,
                                              numa_node_of_cpu(coord), BASE_PAGE_SIZE);
#else
        void *buf = numa_alloc_onnode(SHMQ_SIZE*CACHELINE_SIZE,
                                      numa_node_of_cpu(coord));
#endif
        assert (buf!=NULL);
        cluster_bufs[cidx] = buf;

        // Reductions
        // --------------------------------------------------
        reductions[cidx] = (struct shm_reduction*)
#ifdef BARRELFISH
                numa_alloc_onnode(sizeof(struct shm_reduction),
                                              numa_node_of_cpu(coord),
                                              BASE_PAGE_SIZE);
#else
            numa_alloc_onnode(sizeof(struct shm_reduction),
                              numa_node_of_cpu(coord));
#endif
        assert (reductions[cidx]!=NULL);
    }
#endif
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
#if 0
    int num_clusters;
    int *model_ids;
    int *cluster_ids;

    if (get_thread_id()==get_sequentializer()) {
        shm_init();
    }

    pthread_barrier_wait(&get_master_share()->data.sync_barrier);

    // Find all clusters this core is part of in ALL models
    shm_get_clusters_for_core(get_thread_id(), &num_clusters,
                              &model_ids, &cluster_ids);

    debug_printfff(DBG__SWITCH_TOPO, "shm_switch_topo: number of clusters %d\n",
                   num_clusters);

    // Iterate all clusters this thread is part of
    for (int cidx=0; cidx<num_clusters; cidx++) {

        int mid = model_ids[cidx];
        int cid = cluster_ids[cidx];

        debug_printfff(DBG__SWITCH_TOPO, "shm_switch_topo: cidx %d mid %d cid %d\n",
                      cidx, mid, cid);

        // Is this cluster part of the current model?
        if (mid==get_topo_idx()) {

            coreid_t coord = shm_get_coordinator_for_cluster(cid);

            //TODO topo_mp_cluster_size only correct for coordinator?
            int num_readers = topo_mp_cluster_size(coord, cid);
            num_readers -= 1; // coordinator is writer

            int reader_id = shm_cluster_get_unique_reader_id(cid,
                                                             get_thread_id());

            // <cid>, despite being a model-local index for the
            // cluster is fine here, since there is only one model
            // active at the time, and the cid is unique within that
            // model. The fact that it is not globally unique does not
            // matter here.

            // Queue
            clusters[get_thread_id()] = shm_init_context(cluster_bufs[cid],
                                                         num_readers,
                                                         reader_id);
            // Reduction
            cluster_reductions[get_thread_id()] = reductions[cid];
            debug_printfff(DBG__SWITCH_TOPO, "Mapping core %d to cluster %d\n",
                           get_thread_id(), cid);


            // We need to initialize the shared datastructures here,
            // presumably by the coordinator. A barrier following the
            // call to this function will ensure that no one uses any
            // of these uninitialized.
            if (coord==get_core_id()) {

                debug_printfff(DBG__SWITCH_TOPO, "Resetting data structures of cluster %d\n", cid);

                // SHM queues
                // XXX Roni?

                // SHM reductions
                *(reductions[cid]) = {
                    .reduction_aggregate = 0,
                    .reduction_counter = 0,
                    .reduction_round = 0
                };
            }

            assert (clusters[get_thread_id()]!=NULL);

            debug_printfff(DBG__SWITCH_TOPO, "shm_switch_topo: clusters[%d] = %p\n",
                           cid, clusters[cid]);

        }
    }

    debug_printfff(DBG__SWITCH_TOPO, "shm_switch_topo done\n");
#endif
}

int shm_does_shm(coreid_t core)
{
#if 0
    for (unsigned x=0; x<topo_num_cores(); x++) {

        int value = topo_get(core, x);

        if (value>=SHM_MASTER_START && value<SHM_MASTER_MAX) {
            return 1;
        }
        if (value>=SHM_SLAVE_START && value<SHM_SLAVE_MAX) {
            return 1;
        }
    }
#endif
    return 0;

}


/**
 * \brief Determine from model if given core is a cluster coordinator
 *
 * \return ID of cluster the given node is coordinator of, otherwise -1
 */
int shm_is_cluster_coordinator(coreid_t core)
{
#if 0
    int value;
    for (unsigned x=0; x<topo_num_cores(); x++) {

        value = topo_get(core, x);

        if ((unsigned) value>=SHM_MASTER_START &&
            (unsigned) value<SHM_MASTER_MAX && core!=x) {
            return value-SHM_MASTER_START;
        }
    }
#endif
    return -1;

}

void shm_get_clusters_for_core (int core,
                                int* num_clusters,
                                int **model_ids,
                                int **cluster_ids) {

#if 0
     int i = 0;

    *model_ids = (int*) malloc(sizeof(int)*topo_num_topos()*get_max_num_clusters());
    *cluster_ids = (int*) malloc(sizeof(int)*topo_num_topos()*get_max_num_clusters());

    // Make sure we count each cluster only once. For this, we need
    // state, once bit per possible cluster

    for (unsigned mod=0; mod<topo_num_topos(); mod++) {

        bool clusters_added[get_max_num_clusters()] = {0};

        for (unsigned x=0; x<topo_num_cores(); x++) {

            int value = topos_get(mod, core, x);

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

                debug_printfff(DBG__SHM,
                               "adding cluster model=%d cluster id=%d\n",
                               mod, value);
            }
        }
    }

    *num_clusters = i;
#endif
}



void shm_get_clusters (int* num_clusters,
                       int **model_ids,
                       int **cluster_ids) {
#if 0
     int i = 0;

    *model_ids = (int*) malloc(sizeof(int)*topo_num_topos()*get_max_num_clusters());
    *cluster_ids = (int*) malloc(sizeof(int)*topo_num_topos()*get_max_num_clusters());

    // Make sure we count each cluster only once. For this, we need
    // state, once bit per possible cluster

    for (unsigned mod=0; mod<topo_num_topos(); mod++) {

        bool clusters_added[get_max_num_clusters()] = {0};

        for (coreid_t core=0; core<topo_num_cores(); core++) {

            for (unsigned x=0; x<topo_num_cores(); x++) {

                int value = topos_get(mod, core, x);

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
                    debug_printfff(DBG__SWITCH_TOPO,
                                   "adding cluster model=%d cluster id=%d\n",
                                   mod, value);
                }
            }
        }
    }

    *num_clusters = i;
#endif
}

coreid_t shm_get_coordinator_for_cluster(int cluster)
{
#if 0
    for (unsigned x=0; x<topo_num_cores(); x++) {
        for (unsigned y=0; y<topo_num_cores(); y++) {

            int value = topo_get(x, y);
            if (value>=SHM_MASTER_START && value<SHM_MASTER_MAX) {

                if (value-SHM_MASTER_START == cluster)
                    return x;
            }
        }
    }
#endif
    return -1;
}

coreid_t shm_get_coordinator_for_cluster_in_model(int cluster, int model)
{
#if 0
    for (unsigned x=0; x<topo_num_cores(); x++) {
        for (unsigned y=0; y<topo_num_cores(); y++) {

            int value = topos_get(model, x, y);
            if (value>=SHM_MASTER_START && value<SHM_MASTER_MAX) {

                if (value-SHM_MASTER_START == cluster)
                    return x;
            }
        }
    }
#endif
    return -1;
}

void shm_receive(uintptr_t *p1,
                 uintptr_t *p2,
                 uintptr_t *p3,
                 uintptr_t *p4,
                 uintptr_t *p5,
                 uintptr_t *p6,
                 uintptr_t *p7) {
#if 0
    // Find the cluster
    struct shm_queue *q = clusters[get_thread_id()];
    shm_receive_raw (q, p1, p2, p3, p4, p5, p6, p7);
#endif
}

void shm_send(uintptr_t p1,
              uintptr_t p2,
              uintptr_t p3,
              uintptr_t p4,
              uintptr_t p5,
              uintptr_t p6,
              uintptr_t p7) {
#if 0
    struct shm_queue *q = clusters[get_thread_id()];
    shm_send_raw (q, p1, p2, p3, p4, p5, p6, p7);
#endif
}

/**
 * \brief Return a unique reader for the SHM context for the given reader
 *
 * Refers to the currently active model.
 *
 * \param cid Cluster ID
 *
 * \param reader Thread ID of the reader for which a unique ID should
 * be returned.
 */
int shm_cluster_get_unique_reader_id(unsigned cid,
                                     coreid_t reader)
{
#if 0
    unsigned val = cid + SHM_MASTER_START;
    int ret = -1;

    coreid_t coord =  shm_get_coordinator_for_cluster(cid);

    for (coreid_t i=0; i<topo_num_cores(); i++) {

        if ((unsigned) topo_get(coord, i)==val) {
            ret++;
        }

        if (i==reader) {

            return ret;
        }
    }
#endif
    return -1;
}

/**
 * \brief Add value to sum atomically using shared memory
 */
static inline void shm_reduce_add(uint64_t *sum, uint64_t *value)
{
#if 0
    __sync_add_and_fetch(sum, *value);
#endif
}
#if 0
/**
 *\ brief Atomically update shared state of reduction
 *
 * \param value The value to be "added" to the shared state. We should
 * really support other operations at some point.
 */
static uintptr_t shm_reduce_write(uintptr_t value)
{

    debug_printfff(DBG__REDUCE, "core %d waiting to start with round %d\n",
                 get_core_id(), red_round);

    // Find cluster share
    struct shm_reduction *red_state = cluster_reductions[get_thread_id()];

    volatile uint8_t *rnd = &red_state->reduction_round;
    while (*rnd!=red_round)
        ;

    // Add value to aggregate
    shm_reduce_add(&red_state->reduction_aggregate, &value);

    // Track that we added our value
#if defined(QRM_DBG_ENABLED)
    uint64_t ctrval =
#endif
        __sync_add_and_fetch(&red_state->reduction_counter, 1);

    debug_printfff(DBG__REDUCE, "shm_reduce, adding value %" PRIu64 ", "
                   "counter now %" PRIu64 "\n", value, ctrval);

    // Switch to the next round
    red_round++;
    return 0;
}

/**
 * \brief Writer: wait for results of children
 */
static uint64_t shm_reduce_sum_children(void)
{

    coreid_t my_core_id = get_thread_id();

    // Find cluster share
    struct shm_reduction *red_state = cluster_reductions[get_thread_id()];

    uint64_t r;

    // Determine which cluster we are the coordinator of
    int clusterid = shm_is_cluster_coordinator(my_core_id);
    assert (clusterid>=0);

    unsigned clustersize = topo_mp_cluster_size(my_core_id, clusterid);
    assert (clustersize>1); // Otherwise it would be ONLY the coordinator

    debug_printfff(DBG__REDUCE, "wait for cluster %d to finish SHM, size %d\n",
                  clusterid, clustersize);

    // Wait for everybody to write - volatile: prevent storage in registers
    volatile uint64_t *ptr = &red_state->reduction_counter;
    while (*ptr!=(clustersize-1))
        ;

    r = red_state->reduction_aggregate;

    // Cleanup
    red_state->reduction_aggregate = 0;
    red_state->reduction_counter = 0;

    // Start next round
    red_state->reduction_round++;

    return r;

}
#endif

/**
 * @brief does a reduction on a shared memory region
 *
 * @param input     input message for this node
 * @param output    the returned aggregated value
 *
 * @return SMLT_SUCCESS or error avalue
 */
errval_t smlt_shm_reduce(struct smlt_msg *input, struct smlt_msg *output)
{
#if 0
    uintptr_t current_aggregate = val;

    // Leaf nodes send messagas
    if (topo_does_shm_receive(get_thread_id())) {
        shm_reduce_write(val);
        return 0;
    }

    // Coordinator of each cluster
    if (topo_does_shm_send(get_thread_id())) {

        debug_printfff(DBG__REDUCE,
                       "cluster coordinator %d looking for share\n",
                       get_thread_id());

        current_aggregate = shm_reduce_sum_children();

        debug_printfff(DBG__REDUCE,
                       "cluster coordinator %d got value: %" PRIu64 "\n",
                       get_thread_id(),
                       current_aggregate);

    }

    return current_aggregate;
#endif
    return SMLT_SUCCESS;
}

/**
 * @brief does a reduction of notifications on a shared memory region
 *
 *
 * @return SMLT_SUCCESS or error avalue
 */
errval_t smlt_shm_reduce_notify(void)
{
#if 0
    uintptr_t current_aggregate = val;

    // Leaf nodes send messagas
    if (topo_does_shm_receive(get_thread_id())) {

        shm_reduce_write(val);
        return 0;
    }

    // Coordinator of each cluster
    if (topo_does_shm_send(get_thread_id())) {

        debug_printfff(DBG__REDUCE,
                       "cluster coordinator %d looking for share\n",
                       get_thread_id());

        current_aggregate = shm_reduce_sum_children();

    }
#endif
    return SMLT_SUCCESS;
}

