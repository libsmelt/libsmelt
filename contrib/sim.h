#ifndef SIM_H
#define SIM_H 1

#include <stdint.h>

void smlt_tree_generate_wrapper(unsigned ncores, 
                                uint32_t *cores, 
                                char* tree_name,
                                uint16_t** model,
                                uint32_t** leafs,
                                uint32_t* last_node);
void smlt_tree_parse_wrapper(char* json_string,
                             uint16_t** model,
                             uint32_t** leafs,
                             uint32_t* last_node);

#endif
