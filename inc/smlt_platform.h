/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_PLATFORM_H_
#define SMLT_PLATFORM_H_ 1


/*
 * ===========================================================================
 * Operating System
 * ===========================================================================
 */
#if defined(BARRELFISH)
#include <platforms/barrelfish.h>
#elif defined(__linux__) || defined(__LINUX__)
#include <platforms/linux.h>
#else
#error "unsupported OS"
#endif


/*
 * ===========================================================================
 * Architecture
 * ===========================================================================
 */
#if defined(__x86_64__)
#include <arch/x86_64.h>
#elif defined(__arm__)
#include <arch/arm.h>
#else
#error "unsupported architecture"
#endif


struct sync_binding;
int _tree_prepare(void);
void _tree_export(const char *);
void _tree_register_receive_handlers(struct sync_binding *, void*);

void tree_connect(const char*);

#endif /* SMLT_PLATFORM_H_ */