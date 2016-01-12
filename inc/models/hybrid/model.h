#ifndef MULTICORE_MODEL
#define MULTICORE_MODEL 1

#include "model_defs.h"
#include <vector>

int topo0[TOPO_NUM_CORES][TOPO_NUM_CORES] = {
//    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    { 0, 1,71, 0}, //  0
    {99, 0, 0,70}, //  1
    {51, 0, 0, 0}, //  2
    { 0,50, 0, 0}, //  3
};



int *_topo_combined[NUM_TOPOS] = {(int*) topo0};
int **topo_combined = (int**) _topo_combined;

const char* _topo_names[NUM_TOPOS] = {"cluster"};
char **topo_names = (char**) _topo_names;

std::vector<int> leaf_nodes0 {2,3};
std::vector<int> *_all_leaf_nodes[NUM_TOPOS] = {&leaf_nodes0};
std::vector<int> **all_leaf_nodes = _all_leaf_nodes;
std::vector<coreid_t> last_nodes = {3};

#endif
