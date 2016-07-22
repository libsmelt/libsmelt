#include "tree_config.h"
#include "sim.h"
#include <stdio.h>
#include <unistd.h>

#define MAX_ATTEMPTS 10

#include <iostream>

using namespace std;

/**
 * \brief Retrieve a model from the simulator
 *
 * Retry up to MAX_ATTEMPTS times in case of failure.
 */
int smlt_tree_parse(const char* json_string,
                    uint32_t ncores,
                    uint16_t** model,
                    uint32_t** leafs,
                    uint32_t* last_node,
                    uint32_t* len_model)
{
    int res = -1;
    int attempts = 0;

    while (res!=0 && attempts<MAX_ATTEMPTS) {

            res = smlt_tree_parse_wrapper(json_string, ncores,
                                          model, leafs,
                                          last_node, len_model);
            if (res!=0) {
                sleep(2);
            }
    }

    return res;
}

extern "C" {
	int smlt_tree_generate(uint32_t ncores, uint32_t* cores,
                           const char* tree_name, uint16_t** model,
                           uint32_t** leafs, uint32_t* last_node,
                           uint32_t* len_model)
	{
        return smlt_tree_generate_wrapper(ncores, cores, tree_name,
                                          model, leafs, last_node, len_model);
	}
}
