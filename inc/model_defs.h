#ifndef MULTICORE_TOPO_DEFS
#define MULTICORE_TOPO_DEFS 1

#define MACHINE "<class 'gruyere.Gruyere'>"
#define TOPO_NUM_CORES 32

#define TOPOLOGY "multi-topo"
#define SHM_SLAVE_START 50
#define SHM_SLAVE_MAX 69
#define SHM_MASTER_START 70
#define SHM_MASTER_MAX 89
#define NUM_TOPOS 5
#define ALL_LAST_NODES ((int[]) {31, 28, 24, 23, 29})
#define LAST_NODE ALL_LAST_NODES[get_topo_idx()]
#endif
