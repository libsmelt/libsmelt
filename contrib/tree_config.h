#ifndef TREE_CONFIG
#define TREE_CONFIG 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void smlt_tree_generate(unsigned ncores, uint32_t *cores, char* tree_name,
                        uint16_t** model, uint32_t** leafs, uint32_t* last_node);
void smlt_tree_parse(char* json_string, uint16_t** model, 
                     uint32_t** leafs, uint32_t* last_node);

#ifdef __cplusplus
}
#endif
#endif
