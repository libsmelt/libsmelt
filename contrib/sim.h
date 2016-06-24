#ifndef SIM_H
#define SIM_H 1

#include <stdint.h>

int smlt_tree_generate_wrapper(uint32_t ncores,
                               uint32_t *cores,
                               const char* tree_name,
                               uint16_t** model,
                               uint32_t** leafs,
                               uint32_t* t_root,
                               uint32_t* len_model);
int smlt_tree_parse_wrapper(const char* json_string,
                            uint32_t ncores,
                            uint16_t** model,
                            uint32_t** leafs,
                            uint32_t* t_root,
                            uint32_t* len_model);

#endif
