/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_SMLT_H_
#define SMLT_SMLT_H_ 1


/*
 * ===========================================================================
 * Smelt configuration
 * ===========================================================================
 */


/*
 * ===========================================================================
 * type declarations
 * ===========================================================================
 */



/**
 * represents a handle to a smelt instance.
 */
struct smlt_inst;

/*
 * ===========================================================================
 * type declarations
 * ===========================================================================
 */

/**
 * @brief creates a new smelt instance
 */
errval_t smlt_instance_create();

/**
 * @brief destroys a smelt instance.
 */
errval_t smlt_instance_destroy();


errval_t smlt_thread_init();


struct smlt_inst *smlt_get_current_instance();

errval_t smlt_switch_instance();

#endif /* SMLT_SMLT_H_ */