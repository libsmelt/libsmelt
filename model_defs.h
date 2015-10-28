#ifndef MULTICORE_MODEL_DEFS
#define MULTICORE_MODEL_DEFS 1

#define MACHINE "<class 'gruyere.Gruyere'>"
#define MODEL_NUM_CORES 32

#define TOPOLOGY "multi-model"
#define SHM_SLAVE_START 50
#define SHM_SLAVE_MAX 69
#define SHM_MASTER_START 70
#define SHM_MASTER_MAX 89
#define NUM_MODELS 1
#define ALL_LAST_NODES ((int[]) {7})
#define LAST_NODE ALL_LAST_NODES[get_model_idx()]
#endif
