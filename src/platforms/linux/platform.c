/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <smlt.h>
#include <smlt_node.h>
#include "../../internal.h"

#include <numa.h>


/**
 * @brief initializes the platform specific backend
 *
 * @param num_proc  the requested number of processors
 *
 * @returns the number of available processors
 */
uint32_t smlt_platform_init(uint32_t num_proc)
{
    SMLT_DEBUG(SMLT_DBG__INIT, "platform specific initialization (Linux)\n");

    if (numa_available() != 0) {
        SMLT_ERROR("NUMA is not available!\n");
        return 0;
    }
#ifdef SMLT_UMP_ENABLE_SLEEP
    SMLT_WARNING("UMP sleep is enabled - this is buggy\n");
#else
    SMLT_NOTICE("UMP sleep disabled\n");
#endif

    if (num_proc > numa_num_configured_cpus()) {
        num_proc = numa_num_configured_cpus();
    }

    return num_proc;
}


/*
 * ===========================================================================
 * Platform specific NUMA implementation
 * ===========================================================================
 */

/**
 * @brief returns the number of clusters
 *
 * @return  the number of clusters
 */
uint32_t smlt_platform_num_clusters(void)
{
    return (uint32_t) numa_num_configured_nodes();
}

/**
 * @brief getting the cores ids of a NUMA cluster
 *
 * @param cluster_id        the id of the NUMA node
 *
 * @return array of core ids that are within the NUMA node
 */
errval_t smlt_platform_cores_of_cluster(uint8_t cluster_id,
                                        coreid_t** cores,
                                        uint32_t* size)
{
    int max_nodes = numa_num_configured_nodes();
    int num_cores = numa_num_configured_cpus();
    int node_size = num_cores/max_nodes;

    coreid_t* result = (coreid_t*) malloc(sizeof(coreid_t)*node_size);

    assert(result != NULL);
    struct bitmask* node = numa_allocate_cpumask();

    if (!numa_node_to_cpus(cluster_id, node)) {
        int j = 0;
        for (int i = 0; i < num_cores;i++) {
            if (numa_bitmask_isbitset(node, i)) {
               result[j] = i;
               j++;
            }
        }
        *cores = result;
        *size = node_size;

        numa_free_cpumask(node);

        return SMLT_SUCCESS;
    } else {
        numa_free_cpumask(node);


        *cores = NULL;
        *size = 0;
        return SMLT_ERR_TOPOLOGY_INIT;
    }
}

/**
 * @brief  obtains the number of cores in the system
 *
 * @retursn     the number of cores on the system
 */
uint32_t smlt_platform_num_cores(void)
{
    return (uint32_t) numa_num_configured_cpus();
}

/**
 * @brief  obtains the number of cores on a cluster
 *
 * @param cluster_id     the id of the cluster
 *
 * @retursn     the number of cores on the cluster
 */
uint32_t smlt_platform_num_cores_of_cluster(uint8_t cluster_id)
{
    struct bitmask* node = numa_allocate_cpumask();
    uint32_t core_count = 0;

    if (!numa_node_to_cpus(cluster_id, node)) {
        core_count = numa_bitmask_weight(node);
    }
    numa_free_cpumask(node);

    return core_count;
}

/**
 * @brief getting the NUMA node id of a core id
 *
 * @param core_id        the id of the core
 *
 * @return id of the NUMA node
 */
uint8_t smlt_platform_cluster_of_core(coreid_t core_id)
{
    return numa_node_of_cpu(core_id);
}
