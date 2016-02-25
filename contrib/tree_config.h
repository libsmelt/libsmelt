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
 */
void smlt_tree_generate(unsigned ncores, uint32_t *cores, char* tree_name,
                        uint16_t** model, uint32_t** leafs, uint32_t* last_node);

/**
 * @brief parses a JSON string into the model
 *
 * @param json_string
 * @param model         returns the model
 * @param leafs         returns the leafs in the tree
 * @param last_node     returns the last node
 */
void smlt_tree_parse(char* json_string, uint16_t** model, 
                     uint32_t** leafs, uint32_t* last_node);

#ifdef __cplusplus
}
#endif
#endif
