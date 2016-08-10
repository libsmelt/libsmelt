/*
 * Copyright (c) 2013-2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_PLATFORM_LINUX_H_
#define SMLT_PLATFORM_LINUX_H_ 1

//#define _GNU_SOURCE
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>

//#define SMLT_CONFIG_LINUX_NUMA_ALIGN 1

/*
 * ===========================================================================
 * error values
 * ===========================================================================
 */
typedef uint64_t errval_t;


/* platform specific barrier */
typedef pthread_barrier_t smlt_platform_barrier_t;
typedef pthread_barrierattr_t smlt_platform_barrierattr_t;

/* platform specific handles */
typedef pthread_t smlt_platform_node_handle_t;


/* platform specific lock */
typedef pthread_spinlock_t smlt_platform_lock_t;

typedef uint32_t coreid_t;
#define PRIuCOREID PRIu32

typedef uint64_t cycles_t;

cycles_t bench_tsc(void);
void domain_span_all_cores(void);


#define BASE_PAGE_SIZE 4096U
#define UMP_QUEUE_SIZE 1000U // 64 aka one page does not work well on 815

#define USER_PANIC(x) {                         \
        printf("PANIC: " x "\n");               \
        exit(1);                                \
    }


void debug_printf(const char *fmt, ...);

static inline uint64_t rdtscp(void)
{
    uint32_t eax, edx;
    __asm volatile ("rdtscp" : "=a" (eax), "=d" (edx) :: "ecx");
    return ((uint64_t)edx << 32) | eax;
}


inline void bench_init(void)
{
}

static inline cycles_t bench_tscoverhead(void)
{
    return 0;
}

#define bench_tsc() rdtscp()


#endif /* SMLT_PLATFORM_LINUX_H_ */
