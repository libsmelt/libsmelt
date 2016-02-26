#include "tree_config.h"
#include "sim.h"
#include <stdio.h>

extern "C" {
	int smlt_tree_parse(char* json_string,
                        uint32_t ncores,
                        uint16_t** model,
                        uint32_t** leafs,
                        uint32_t* last_node)
	{
        return smlt_tree_parse_wrapper(json_string, ncores,
                                       model, leafs, last_node);
	}

	int smlt_tree_generate(uint32_t ncores, uint32_t* cores, 
                           char* tree_name, uint16_t** model,
                           uint32_t** leafs, uint32_t* last_node)
	{
        return smlt_tree_generate_wrapper(ncores, cores, tree_name,
                                          model, leafs, last_node);
	}
}

