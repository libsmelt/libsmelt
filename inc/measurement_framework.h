/**
 * \file
 * \brief Measurement framework
 */

/*
 * Copyright (c) 2007, 2008, 2009, 2010, 2011, 2012, 2013, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef MEASUREMENT_FRAMEWORK_H
#define MEASUREMENT_FRAMEWORK_H

#include <stdlib.h>
#include <stdio.h>

#include "cycle.h"

#define bench_tsc() getticks()

static inline uint64_t rdtscp(void)
{
    uint32_t eax, edx;
    __asm volatile ("rdtscp" : "=a" (eax), "=d" (edx) :: "ecx");
    return ((uint64_t)edx << 32) | eax;
}

#include "inttypes.h"

static inline void cpuid(uint32_t function, uint32_t *eax, uint32_t *ebx,
                         uint32_t *ecx, uint32_t *edx)
{
    // make it possible to omit certain return registers
    uint32_t a, b, c, d;
    if (eax == NULL) {
        eax = &a;
    }
    if (ebx == NULL) {
        ebx = &b;
    }
    if (ecx == NULL) {
        ecx = &c;
    }
    if (edx == NULL) {
        edx = &d;
    }
    __asm volatile("cpuid"
                   : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
                   : "a" (function)
                   );
}

typedef uint64_t cycles_t;


struct sk_measurement {

    // Pointer to the buffer
    cycles_t *buffer;

    // Store the last tcp value
    cycles_t last_tsc;

    // Current index
    uint32_t idx;

    // Total number of measurements.
    // The idx might point to the beginning of the buffer after an overflow,
    // but the data in the rest of the buffer is still valid
    uint32_t total;

    // Size of buffers
    uint32_t max;

    // Name of the measurement
    char *label;
};

inline static cycles_t sk_m_get_max(struct sk_measurement *m)
{
    return (m->total >= m->max) ? m->max : m->total;
}

/*
 * \brief Initialize the data structures
 */
inline static void sk_m_init(struct sk_measurement *m,
                             uint32_t max,
                             char *name,
                             cycles_t *buf)
{
    m->max = max;
    m->last_tsc = bench_tsc();
    m->idx = 0;
    m->total = 0;
    m->label = name;

    uint32_t eax, ebx, ecx, edx;
    bool rdtscp_flag;
    
    cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
    if ((edx >> 27) & 1) {
        rdtscp_flag = true;
    } else {
        rdtscp_flag = false;
    }

    // Otherwise, we need the other tsc function (see BF's bench_tsc)
    assert(rdtscp_flag==1);

    
    m->buffer = buf;
    assert(buf!=NULL);
}

/*
 * \brief Initialize the data structures
 */
inline static void sk_m_reset(struct sk_measurement *m)
{
    m->last_tsc = bench_tsc();
    m->idx = 0;
    m->total = 0;
}

/*
 * \brief Restart the measurement
 *
 * Updates the currently stored tsc value indicating the start of the measurement.
 */
inline static void sk_m_restart_tsc(struct sk_measurement *m)
{
    m->last_tsc = bench_tsc();
}

/*
 * \brief Add a new RTC difference to the measurement
 *
 * The measurement buffer is handled as a ring buffer. The oldest data will
 * overwritten.
 */
inline static void sk_m_add(struct sk_measurement *m)
{
    cycles_t tsc = bench_tsc();

    assert(tsc>m->last_tsc);
    assert(m->idx<m->max);
    assert(m->buffer!=NULL);

    m->buffer[m->idx] = (tsc - m->last_tsc - bench_tscoverhead());
    m->last_tsc = tsc;

    m->idx = (m->idx+1) % m->max;
    m->total++;
}


/*
 * \brief Add the given measurement to the buffer
 *
 * This function does _not_ reset the temporary start tsc stored
 * internally.
 */
inline static void sk_m_add_value(struct sk_measurement *m, cycles_t v)
{
    assert(m->idx<m->max);
    assert(m->buffer!=NULL);

    m->buffer[m->idx] = v;

    m->idx = (m->idx+1) % m->max;
    m->total++;
}

/*
 * \brief Sum up all values and  output result
 */
inline static void sk_m_sum(struct sk_measurement *m)
{
    uint32_t max = sk_m_get_max(m);
    assert(max<=m->max);
    assert(m->buffer!=NULL);

    uint64_t sum = 0;

    for (uint32_t i=0; i<max; i++) {
        sum += m->buffer[i];
    }
    printf("sk_m_sum(%d,%s) sum= %" PRIu64 "\n",
           disp_get_core_id(), m->label, sum);
}

/*
 * \brief Print the current measurements
 *
 * \param c A constant to add to each measurement
 */
inline static void sk_m_print_const(struct sk_measurement *m, uint64_t c)
{
    uint32_t max = sk_m_get_max(m);
    assert(max<=m->max);
    assert(m->buffer!=NULL);
    for (uint32_t i=0; i<max; i++) {

        printf("sk_m_print(%d,%s) idx= %" PRIu32 " tscdiff= %" PRIu64 "\n",
               disp_get_core_id(), m->label, i,
               (m->buffer[i]+c));
    }
}

/*
 * \brief Print the current measurements
 */
inline static void sk_m_print(struct sk_measurement *m)
{
    sk_m_print_const(m, 0);
}

#endif /* MEASUREMENT_FRAMEWORK_H */
