#ifndef MULTICORE_MODEL
#define MULTICORE_MODEL 1

#include "model_defs.h"

#include <vector>

int **topo_combined = NULL;
char** topo_names = NULL;
std::vector<int> **all_leaf_nodes = {};
std::vector<coreid_t> last_nodes = {};

#endif
