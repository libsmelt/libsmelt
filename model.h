#ifndef MULTICORE_MODEL
#define MULTICORE_MODEL 1

#include "model_defs.h"

int model0[TOPO_NUM_CORES][TOPO_NUM_CORES] = {
//    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
    { 0, 1, 0, 2},
    {99, 0, 1, 0},
    { 0,99, 0, 0},
    {99, 0, 0, 0},
};

int *topo_combined[NUM_TOPOS] = {(int*) model0};
const char* topo_names[NUM_TOPOS] = {"mst"};
#endif
