/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#include <stdio.h>
#include <string.h>
#include <math.h>


#include <smlt.h>
#include <smlt_bench.h>

cycles_t smlt_bench_tsc_overhead = 0;

 /*
  * ===========================================================================
  * Initialization
  * ===========================================================================
  */

 errval_t smlt_bench_ctl_init(struct smlt_bench_ctl *ctl,
                              char *label,
                              uint32_t num_measurements)
{
    return SMLT_SUCCESS;
}

void smlt_bench_ctl_reset(struct smlt_bench_ctl *ctl)
{
    memset(&ctl->a, 0, sizeof(ctl->a));
    ctl->idx = 0;
    ctl->count = 0;
    ctl->last_tsc = smlt_arch_tsc();
}

 /*
  * ===========================================================================
  * Analysis
  * ===========================================================================
  */

static void smlt_bench_sort(cycles_t *data, uint32_t len)
{
    cycles_t tmp, pivot;
    uint32_t i, j;
    if (len < 2) {
        return;
    }
    pivot = data[len / 2];
    for (i = 0, j = len - 1;;i++, j--) {
        while (data[i] < pivot) {
            i++;
        }
        while (pivot < data[j]) {
            j--;
        }
        if (i >= j) {
            break;
        }

        tmp = data[i];
        data[i] = data[j];
        data[j] = tmp;
    }
    smlt_bench_sort(data, i);
    smlt_bench_sort(data + i, len - i);
}

static void smlt_bench_avg(struct smlt_bench_ctl *ctl)
{
    ctl->a.avg = 0;
    for (uint32_t i = 0; i < ctl->a.count; ++i) {
        ctl->a.avg += ctl->data[i];
    }
    ctl->a.avg /= ctl->a.count;
}

static void smlt_bench_stderr(struct smlt_bench_ctl *ctl)
{
    smlt_bench_avg(ctl);

    cycles_t s = 0;
    for (uint32_t i = 0; i < ctl->a.count; ++i) {
        cycles_t tmp = (ctl->data[i] - ctl->a.avg);
        s += (tmp * tmp);
    }

    s /= ctl->a.count;
    ctl->a.stderr = (cycles_t)sqrt(s);
}

 /**
  * @brief prepares the analysis of the measurements
  *
  * @param ctl       bench contorl structure
  * @param ignore    percentage to ignore
  */
void smlt_bench_ctl_prepare_analysis(struct smlt_bench_ctl *ctl,
                                     double ignore)
{
    ctl->a.count = ctl->count;
    if (ctl->count > ctl->max_data) {
        ctl->a.count = ctl->max_data;
    }

    smlt_bench_sort(ctl->data, ctl->a.count);

    ctl->a.ignored = (uint32_t)(ctl->a.count * ignore);
    ctl->a.count = ctl->a.count - ctl->a.ignored;
    ctl->a.median = ctl->data[ctl->a.count/2];
    ctl->a.min = ctl->data[0];
    ctl->a.max = ctl->data[ctl->a.count -1];
    smlt_bench_stderr(ctl);
    ctl->a.valid = 1;
}


/*
 * ===========================================================================
 * Display
 * ===========================================================================
 */

//#define SMLT_BENCH_PRINT "sk_m_print(%d,%s) idx= %" PRIu32 " tscdiff= %" PRIu64 "\n"

#define SMLT_BENCH_PRINT "%s %" PRIu64

void smlt_bench_ctl_print_analysis(struct smlt_bench_ctl *ctl)
{
    if (!ctl->a.valid) {
        smlt_bench_ctl_prepare_analysis(ctl, SMLT_BENCH_IGNORE_DEFAULT);
    }
    printf("%s, count=%" PRIu32 ", avg=%" PRIu64 ", med=%" PRIu64 ", stderr=%"
           PRIu64 ", min=%" PRIu64 ", max=%" PRIu64 "\n", ctl->label, ctl->a.count,
           ctl->a.avg, ctl->a.median, ctl->a.stderr, ctl->a.min, ctl->a.max);

}

void smlt_bench_clt_print_data(struct smlt_bench_ctl *ctl)
{
    if (ctl->count > ctl->max_data) {
        ctl->count = ctl->max_data;
    }

    for (uint32_t i=0; i<ctl->count ; i++) {
        printf("sk_m_print(%d,%s) idx= %" PRIu32 " tscdiff= %" PRIu64 "\n",
               smlt_platform_get_core_id(), ctl->label, i,
               (ctl->data[i]));
    }

}
