/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#include <smlt.h>
#include <smlt_topology.h>
#include <smlt_generator.h>
#include "debug.h"
#include <stdio.h>

/**
 * this is a node in the current topology
 */
struct smlt_topology_node
{
    struct smlt_topology *topology;         ///< backpointer to the topology
    struct smlt_topology_node *parent;      ///< pointer to the parent node
    struct smlt_topology_node **children;   ///< array of children
    uint32_t chanel_type;                   ///< queuepairs 
    smlt_nid_t node_id;                     ///< qp[0]->parent qp[1..n] child
    uint32_t array_index;                   ///< Invalid if root
    uint32_t num_children;                  ///<
    bool is_leaf;
};


/**
 * represents a smelt topology
 */
struct smlt_topology
{
    const char *name;                             ///< name
    struct smlt_topology_node *root;        ///< pointer to the root node
    uint32_t num_nodes;
    struct smlt_topology_node all_nodes[];   ///< array of all nodes
};

//prototypes
static void smlt_topology_create_binary_tree(struct smlt_topology **topology,
                                             uint32_t num_threads);
static void smlt_topology_parse_model(struct smlt_generated_model* model,
                                      struct smlt_topology** topo);
/**
 * @brief initializes the topology subsystem
 *
 * @return SMLT_SUCCESS or error value
 */
errval_t smlt_topology_init(void)
{

    /*
    _tree_prepare();

    if (get_thread_id()==0) {

        shl_barrier_init(&tree_setup_barrier, NULL, get_num_threads());
    }

    _tree_export(qrm_my_name);
     */
    return SMLT_SUCCESS;
}

/**
 * @brief creates a new Smelt topology out of the model parameter
 *
 * @param model         the model input data from the simulator
 * @param length        length of the model input
 * @param name          name of the topology
 * @param ret_topology  returned pointer to the topology
 *
 * @return SMELT_SUCCESS or error value
 *
 * If the model is NULL, then a binary tree will be generated
 */
errval_t smlt_topology_create(struct smlt_generated_model* model, 
                              const char *name,
                              struct smlt_topology **ret_topology)
{
    
    if (model == NULL) {

        *ret_topology = (struct smlt_topology*) 
                        smlt_platform_alloc(sizeof(struct smlt_topology)+
                                            sizeof(struct smlt_topology_node)*
                                            smlt_get_num_proc(),
                                            SMLT_DEFAULT_ALIGNMENT, true);
        smlt_topology_create_binary_tree(ret_topology, smlt_get_num_proc());    
        (*ret_topology)->num_nodes = smlt_get_num_proc();
    } else {
        *ret_topology = (struct smlt_topology*) 
                        smlt_platform_alloc(sizeof(struct smlt_topology)+
                                            sizeof(struct smlt_topology_node)*
                                            model->len,
                                            SMLT_DEFAULT_ALIGNMENT, true);
        smlt_topology_parse_model(model, ret_topology);   
        (*ret_topology)->num_nodes = model->len;
    }

    (*ret_topology)->name = name;
    return SMLT_SUCCESS;
}

/**
 * @brief destroys a smelt topology.
 *
 * @param topology  the Smelt topology to destroy
 *
 * @return SMELT_SUCCESS or error vlaue
 */
errval_t smlt_topology_destroy(struct smlt_topology *topology)
{
    return SMLT_SUCCESS;
}

/**
 * \brief Build a binary tree model for the current machine.
 *
 * @param topology      returned pointer to the topology
 * @param num_threads   number of threads/processes in the model
 * @param name          name of the topology
 *
 * @return SMELT_SUCCESS or error value 
 */
static void smlt_topology_create_binary_tree(struct smlt_topology **topology,
                                             uint32_t num_threads)
{
    assert (topology!=NULL);
    // Fill model    
    (*topology)->root = &((*topology)->all_nodes[0]);    
   
    for (int i = 0; i < num_threads;i++) {
        
        struct smlt_topology_node* node = &(*topology)->all_nodes[i];
        node->topology = *topology;
        if (i == 0) {
            node->parent = NULL;
            node->node_id = 0;
        } else {
            if ((i % 2) == 0) {
                node->parent = &((*topology)->all_nodes[(i/2)-1]);
                node->array_index = 1;
            } else { 
                node->parent = &((*topology)->all_nodes[(i/2)]);
                node->array_index = 0;
            }
        }
        
        if (i*2+1 < num_threads) {
            node->children = (struct smlt_topology_node**)
                             smlt_platform_alloc(sizeof(struct smlt_topology_node*)*2,
                                                 SMLT_DEFAULT_ALIGNMENT, true);
            node->children[0] = &(*topology)->all_nodes[i*2+1];
            node->node_id = i;
            node->num_children =1;
            if (i*2+2 < num_threads) {
                node->children[1] = &(*topology)->all_nodes[i*2+2];
                node->num_children = 2;
            }
            // TODO set chanel type;
        } else {
            node->children = NULL;
            node->num_children = 0;
            node->node_id = i;
            node->is_leaf = true;
        }
    }
    for (int i = 0; i < num_threads; i++) {
        SMLT_DEBUG(SMLT_DBG__INIT, "Parent of node %d %p \n",
        i, (*topology)->all_nodes[i].parent);
    }
}

/**
 * \brief build topology from model.
 *
 * @param topology      returned pointer to the topology
 *
 * @return SMELT_SUCCESS or error value 
 */
static void smlt_topology_parse_model(struct smlt_generated_model* model,
                                      struct smlt_topology** topo)
{
    
    assert (topo!=NULL);
    // Fill model    
    (*topo)->root = &((*topo)->all_nodes[model->root]);    
    (*topo)->root->parent = NULL;
    // TODO use node ids instead of cores ids
    for (int x = 0; x < model->len;x++){
        struct smlt_topology_node* node = &((*topo)->all_nodes[x]);

        // find number of children and allocate accordingly
        int max_child = 0;
        for(int y = 0; y < model->len; y++){
            int tmp = model->model[x*model->len+y];
            if ((tmp > max_child) && (tmp != 99)){
               max_child = model->model[x*model->len+y];
            }
        }
        node->node_id = x;   
        node->children = (struct smlt_topology_node**)
                             smlt_platform_alloc(sizeof(struct smlt_topology_node*)*
                                                 max_child, SMLT_DEFAULT_ALIGNMENT, 
                                                 true);
        // set model
        for(int y = 0; y < model->len; y++){
            int val = model->model[x*(model->len)+y];
            
            if (val > 0) {
                if (val == 99) {
                    SMLT_DEBUG(SMLT_DBG__INIT,"Parent of %d is %d \n", x, y);
                    node->parent = &((*topo)->all_nodes[y]);
                } else {
                    // TODO add channel type
                    SMLT_DEBUG(SMLT_DBG__INIT,"Child of %d is %d at pos %d \n", 
                               x, y, val-1);
                    node->topology = *topo;
                    node->children[val-1] = &((*topo)->all_nodes[y]);
                    node->node_id = x; // TODO change to real node ID
                    (*topo)->all_nodes[y].array_index = val-1;
                }
            }

        }   
    
        node->num_children = max_child;
    }


    for (int i = 0; i < model->len; i++) {
        for (int j = 0; j < model->len; j++) {
            if ((model->leafs[j] == i) && (i != 0)) {
                SMLT_DEBUG(SMLT_DBG__INIT,"%d is a leaf \n", i) 
                (*topo)->all_nodes[i].is_leaf = true;
            }
        }
    }
}
/*
 * ===========================================================================
 * topology nodes
 * ===========================================================================
 */


/**
 * @brief returns the first topology node
 *
 * @param topo  the Smelt topology
 *
 * @return Pointer to the smelt topology node
 */
struct smlt_topology_node *smlt_topology_get_first_node(struct smlt_topology *topo)
{
    return topo->all_nodes;
}

/**
 * @brief gets the next topology node in the topology
 *
 * @param node the current topology node
 *
 * @return
 */
struct smlt_topology_node *smlt_topology_node_next(struct smlt_topology_node *node)
{
    return node+1;
}

/**
 * @brief gets the parent topology ndoe
 *
 * @param node the current topology node
 *
 * @return
 */
struct smlt_topology_node *smlt_topology_node_parent(struct smlt_topology_node *node)
{
    return node->parent;
}


/**
 * @brief gets the parent topology ndoe
 *
 * @param node the current topology node
 *
 * @return 
 */
struct smlt_topology_node **smlt_topology_node_children(struct smlt_topology_node *node,
                                                        uint32_t* children)
{
    *children = node->num_children;
    return node->children;
}

/**
 * @brief checks if the topology node is the last
 *
 * @param node the topology node
 *
 * @return TRUE if the node is the last, false otherwise
 */
bool smlt_topology_node_is_last(struct smlt_topology_node *node)
{
    return (node == &node->topology->all_nodes[node->topology->num_nodes-1]);
}

/**
 * @brief checks if the topology node is the root
 *
 * @param node the topology node
 *
 * @return TRUE if the node is the root, false otherwise
 */
bool smlt_topology_node_is_root(struct smlt_topology_node *node)
{
    return (node->parent == NULL);
}

/**
 * @brief checks if the topology node is a leaf
 *
 * @param node the topology node
 *
 * @return TRUE if the node is a leaf, false otherwise
 */
bool smlt_topology_node_is_leaf(struct smlt_topology_node *node)
{
    return (node->num_children == 0);
}

/**
 * @brief obtains the child index (the order of the children) from the node
 *
 * @param node  the topology ndoe
 *
 * @return child index
 */
uint32_t smlt_topology_node_get_child_idx(struct smlt_topology_node *node)
{
    return node->array_index;
}

/**
 * @brief gets the number of nodes with the the given idx
 *
 * @param node  the topology node
 *
 * @return numebr of children
 */
uint32_t smlt_topology_node_get_num_children(struct smlt_topology_node *node)
{
    return node->num_children;
}

/**
 * @brief obtainst he associated node id of the topology node
 *
 * @param node  the Smelt topology node
 *
 * @return Smelt node id
 */
smlt_nid_t smlt_topology_node_get_id(struct smlt_topology_node *node)
{
    return node->node_id;
}


/*
 * ===========================================================================
 * queries
 * ===========================================================================
 */


/**
 * @brief obtains the name of the topology
 *
 * @param topo  the Smelt topology
 *
 * @return string representation
 */
const char *smlt_topology_get_name(struct smlt_topology *topo)
{
    return topo->name;
}


/**
 * @brief gets the number of nodes in the topology
 *
 * @param topo  the Smelt topology
 *
 * @return number of nodes
 */
uint32_t smlt_topology_get_num_nodes(struct smlt_topology *topo)
{
    return topo->num_nodes;
}



// --------------------------------------------------
// Data structures - shared

int *topo = NULL;
//static int topo_idx = -1;
#if 0

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


static int model_generated = 0;
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

    int *topo = smlt_platform_alloc(sizeof(int) * ( num_threads * num_threads),
                                    SMLT_DEFAULT_ALIGNMENT, true);

    //int *_topo = (int*) (malloc(sizeof(int)*(num_threads*num_threads)));

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
                         i==(unsigned) topo_idx ? 'X' : ' ' );
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




const char* topo_get_name(void)
{
    if (topo_names == NULL) {
        return "unknown-topo";
    }
    return topo_names[get_topo_idx()];
}

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




/**
 * \brief Determine the size of a cluster.
 *
 * The coordinator is an argument to this call for performance (only
 * one row of the model has to be parsed, not the entire n*n one)
 *
 * \return Size of the given cluster
 */
int topo_mp_cluster_size(coreid_t coordinator, int clusterid)
{
    int num = 1;
    int value;

    assert (clusterid>=0);

    for (unsigned x=0; x<topo_num_cores(); x++) {

        value = topo_get(coordinator, x);

        if (value>=SHM_MASTER_START && value<SHM_MASTER_MAX) {
            value -= SHM_MASTER_START;
        } /*
        else if (value>=SHM_SLAVE_START && value<SHM_SLAVE_MAX) {
            value -= SHM_SLAVE_START;
            } */
        else {
            value = -1;
        }

        // node belongs to cluster
        if (value>=0) {
            num++;
        }
    }

    return num;
}

#endif

#if 0
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
void smlt_topology_create(void)
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
        bool does_mp = topo_does_mp_send(s, false) ||
            topo_does_mp_receive(s, false);
        assert (tmp_parent >= 0 || s == get_sequentializer() || !does_mp);

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

void tree_init_bindings(void)
{
    /*
    if (bindings==NULL) {

        debug_printfff(DBG__BINDING, "Allocating memory for bindings\n");
        bindings = (mp_binding**) calloc(sizeof(mp_binding*),
                                         (get_num_threads()*get_num_threads()));
        assert (bindings!=NULL);
    }
    */
}


#endif

#if 0

/**
 * \brief Connect two nodes
 */
void mp_connect(coreid_t src, coreid_t dst)
{
    debug_printf("Opening channels manually: %d -> %d\n", src, dst);
    assert (get_binding(src, dst)==NULL);

    _setup_chanels(src, dst);

    assert (get_binding(src,dst)!=NULL);
}


bool topo_does_mp(coreid_t core)
{
    return topo_does_mp_send(core, false) || topo_does_mp_receive(core, false);
}


bool topo_does_shm_send(coreid_t core)
{
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_get(core, i)>=SHM_MASTER_START &&
            topo_get(core, i)<SHM_MASTER_MAX) {

            return true;
        }
    }

    return false;
}

bool topo_does_shm_receive(coreid_t core)
{
    for (unsigned i=0; i<topo_num_cores(); i++) {

        if (topo_get(core, i)>=SHM_SLAVE_START &&
            topo_get(core, i)<SHM_SLAVE_MAX) {

            return true;
        }
    }

    return false;
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
#endif
