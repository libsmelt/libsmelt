/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_TREE_CONFIG
#define SMLT_TREE_CONFIG 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief generates a new model for the current host machine
 *
 * @param ncores        number of cores in the cores array
 * @param cores         array of core ids to be considered
 * @param tree_name     name of the tree to obtain
 * @param model         returns the model
 * @param leafs         returns the leafs in the tree
 * @param last_node     returns the last node
 *
 * @return 0 if successful otherwise > 0
 */
int smlt_tree_generate(uint32_t ncores, uint32_t *cores, char* tree_name,
                       uint16_t** model, uint32_t** leafs, uint32_t* t_root);

/**
 * @brief parses a JSON string into the model
 *
 * @param json_string
 * @param ncores        number of cores in the cores array
 * @param model         returns the model
 * @param leafs         returns the leafs in the tree
 * @param last_node     returns the last node
 *
 * @return 0 if successful otherwise > 0
 */
int smlt_tree_parse(char* json_string, uint32_t ncores, uint16_t** model, 
                    uint32_t** leafs, uint32_t* t_root);

#ifdef __cplusplus
}
#endif
#endif
