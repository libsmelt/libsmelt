#ifndef MULTICORE_MODEL_DEFS
#define MULTICORE_MODEL_DEFS 1

#define MACHINE "<class 'gruyere.Gruyere'>"
#define TOPO_NUM_CORES 4

#define TOPOLOGY "multi-model"
#define SHM_SLAVE_START 50
#define SHM_SLAVE_MAX 69
#define SHM_MASTER_START 70
#define SHM_MASTER_MAX 89
#define NUM_TOPOS 1
#define ALL_LAST_NODES ((coreid_t[]) {2})
#define LAST_NODE ALL_LAST_NODES[get_topo_idx()]
#endif
