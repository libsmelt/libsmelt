#ifndef __LIBSYNC_LINUX
#define __LIBSYNC_LINUX 1

#include <cstdint>
#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cinttypes>
#include <cstring>

extern "C" {
#include "ump_conf.h"
#include "ump_common.h"
}
#include "cycle.h"

typedef pthread_spinlock_t spinlock_t;
typedef uint32_t coreid_t;

typedef uint64_t cycles_t;
cycles_t bench_tscoverhead(void);

cycles_t bench_tsc(void);
coreid_t disp_get_core_id(void);
void domain_span_all_cores(void);

void bench_init(void);

#define BASE_PAGE_SIZE 4096U

typedef struct ump_pair_state mp_binding;

#define USER_PANIC(x) {                         \
        printf("PANIC: " x "\n");               \
        exit(1);                                \
    }

#endif

void debug_printf(const char *fmt, ...);

static inline uint64_t rdtscp(void)
{
    uint32_t eax, edx;
    __asm volatile ("rdtscp" : "=a" (eax), "=d" (edx) :: "ecx");
    return ((uint64_t)edx << 32) | eax;
}

#define bench_tsc() rdtscp()


