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
#include <sched.h>

#ifdef __linux__
#include "linux.h"
#endif

#ifdef BARRELFISH
#include <barrelfish/domain.h>
#include <bench/bench.h>
#endif

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
           smlt_platform_get_core_id(), m->label, sum);
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
               smlt_platform_get_core_id(), m->label, i,
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


static cycles_t *sk_m_sort(struct sk_measurement *m)
{
    uint32_t len = sk_m_get_max(m);

    size_t i, j;
    cycles_t *sorted_array = m->buffer;
    cycles_t temp_holder;


    // sort the array
    for (i = 0; i < len; ++i) {
        for (j = i; j < len; ++j) {
            if (sorted_array[i] > sorted_array[j]) {
                temp_holder = sorted_array[i];
                sorted_array[i] = sorted_array[j];
                sorted_array[j] = temp_holder;
            }
        } // end for: j
    } // end for: i
    return sorted_array;
} // end function: do_sorting

#ifndef SK_M_CUTOFF
#define SK_M_CUTOFF 0.90
#endif


inline static void sk_m_print_analysis(struct sk_measurement *m)
{
    sk_m_sort(m);
    size_t count = sk_m_get_max(m) * SK_M_CUTOFF;

    cycles_t sum = 0;
    for (size_t i=0; i<count; i++) {
        sum += m->buffer[i];
    }
    cycles_t avg = sum / count;

    sum = 0;
    for (size_t i=0; i<count; i++) {
        if (m->buffer[i] > avg) {
            sum += (m->buffer[i] - avg) * (m->buffer[i] - avg);
        } else {
            sum += (avg- m->buffer[i]) * (avg - m->buffer[i]);
        }
    }

    sum /= count;

    printf("sk_m_analysis(%d,%s) n=%" PRIu64 ", %" PRIu64 ", %" PRIu64
           ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 "\n",
           smlt_platform_get_core_id(), m->label, count, avg, sum,
           m->buffer[0], m->buffer[count / 2], m->buffer[count-1]);

}

#endif /* MEASUREMENT_FRAMEWORK_H */
