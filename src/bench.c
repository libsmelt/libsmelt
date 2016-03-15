/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#include <stdio.h>
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

    ctl->min = 0;
    ctl->max = 0;
    ctl->median = 0;
    ctl->avg = 0;
    ctl->stderr = 0;
    ctl->num = 0;
    ctl->ignored = 0;
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
    ctl->avg = 0;
    for (uint32_t i = 0; i < ctl->num; ++i) {
        ctl->avg += ctl->data[i];
    }
    ctl->avg /= ctl->num;
}

static void smlt_bench_stderr(struct smlt_bench_ctl *ctl)
{
    smlt_bench_avg(ctl);

    cycles_t s = 0;
    for (uint32_t i = 0; i < ctl->num; ++i) {
        cycles_t tmp = (ctl->data[i] - ctl->avg);
        s += (tmp * tmp);
    }

    s /= ctl->num;
    ctl->stderr = sqrt(s);
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
    if (ctl->count > ctl->max_data) {
        ctl->count = ctl->max_data;
    }

    smlt_bench_sort(ctl->data, ctl->count);

    ctl->ignored = (uint32_t)(ctl->count * ignore);
    ctl->num = ctl->count - ctl->ignored;
    ctl->median = ctl->data[ctl->num/2];
    ctl->min = ctl->data[0];
    ctl->max = ctl->data[ctl->num -1];
    smlt_bench_stderr(ctl);
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
    printf("%s, count=%" PRIu32 ", avg=%" PRIu64 ", med=%" PRIu64 ", stderr=%"
           PRIu64 ", min=%" PRIu64 ", max=%" PRIu64 "\n", ctl->label, ctl->num,
           ctl->avg, ctl->median, ctl->stderr, ctl->min, ctl->max);

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
