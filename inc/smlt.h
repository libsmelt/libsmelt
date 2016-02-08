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


#include <smlt_config.h>
#include <smlt_platform.h>



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
 * function declarations
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


/* TODO :*/
void __sync_init(int, bool);
void __sync_init_no_tree(int);
void __sys_init(void);

int  __thread_init(coreid_t,int);
int  __thread_end(void);

int  __backend_thread_end(void);
int  __backend_thread_start(void);

unsigned int  get_thread_id(void);
coreid_t  get_core_id(void);
unsigned int  get_num_threads(void);
bool is_coordinator(coreid_t);
void add_binding(coreid_t sender, coreid_t receiver, mp_binding *mp);
mp_binding* get_binding(coreid_t sender, coreid_t receiver);

void pin_thread(coreid_t);
int __lowlevel_thread_init(int tid);
#endif /* SMLT_SMLT_H_ */