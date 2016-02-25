#include "tree_config.h"
#include "sim.h"
#include <stdio.h>

extern "C" {
	void smlt_tree_parse(char* json_string,
                         uint16_t** model,
                         uint32_t** leafs,
                         uint32_t* last_node)
	{
        smlt_tree_parse_wrapper(json_string,
                                model, leafs, last_node);
	}

	void smlt_tree_generate(unsigned ncores, uint32_t* cores, 
                            char* tree_name, uint16_t** model,
                            uint32_t** leafs, uint32_t* last_node)
	{
        smlt_tree_generate_wrapper(ncores, cores, tree_name,
                                   model, leafs, last_node);
	}
}

