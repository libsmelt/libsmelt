#ifndef SMLT_PLATFORM_LINUX_H_
#define SMLT_PLATFORM_LINUX_H_ 1


#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>

/*
 * ===========================================================================
 * error values
 * ===========================================================================
 */
typedef uint64_t errval_t;


/* platform specific barrier */
typedef pthread_barrier_t smlt_platform_barrier_t;
typedef pthread_barrierattr_t smlt_platform_barrierattr_t;

/* platform specific lock */
typedef pthread_spinlock_t smlt_platform_lock_t;

typedef uint32_t coreid_t;

typedef uint64_t cycles_t;
cycles_t bench_tscoverhead(void);

cycles_t bench_tsc(void);
coreid_t disp_get_core_id(void);
void domain_span_all_cores(void);

void bench_init(void);

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

#define bench_tsc() rdtscp()


#endif /* SMLT_PLATFORM_LINUX_H_ */
