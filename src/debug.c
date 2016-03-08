/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include "debug.h"

/**
 * @brief prints an message to stdout
 *
 * @param fmt   format string to print
 */
void smlt_debug_print(uint32_t subs, const char *fmt, ...)
{
    va_list argptr;
    char str[1024];
    size_t len = 0;

    if (subs & SMLT_DBG_ERR) {
        len = snprintf(str, sizeof(str), "\033[1;31mERROR:\033[0m ");
    } else if (subs & SMLT_DBG_WARN) {
        len = snprintf(str, sizeof(str), "\033[1;33mWARNING:\033[0m ");
    } else if (subs & SMLT_DBG_NOTICE) {
        len = snprintf(str, sizeof(str), "\033[1;36mNOTICE:\033[0m ");
    } else {
        len = snprintf(str, sizeof(str), "smlt: ");
    }

    /* TODO: include submodule */
    if (len < sizeof(str)) {
        va_start(argptr, fmt);
        vsnprintf(str + len, sizeof(str) - len, fmt, argptr);
        va_end(argptr);
    }

    printf(str, sizeof(str));
}
