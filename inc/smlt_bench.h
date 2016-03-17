/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */

#ifndef SMLT_BENCH_H_
#define SMLT_BENCH_H_ 1

#define SMLT_BENCH_IGNORE_DEFAULT 0.05

struct smlt_bench_analyzed
{
    cycles_t min;           ///< minimum of the samples
    cycles_t max;           ///< maximum value of the samples
    cycles_t median;        ///< median of the samples
    cycles_t avg;           ///< average over the samples
    cycles_t stderr;        ///< standard errors
    uint32_t count;         ///< number of values considered
    uint32_t ignored;       ///< number of ignored values
    bool valid;             ///< flag indicating that this data is valid
};

struct smlt_bench_ctl
{
    cycles_t *data;         ///< the array of measured values
    cycles_t last_tsc;      ///< the last measured tsc
    uint32_t count;         ///< number of measured values
    uint32_t idx;           ///< current index
    uint32_t max_data;      ///< maximum number of measturements
    char *label;            ///< label for the measurement

    struct smlt_bench_analyzed a;
};

extern cycles_t smlt_bench_tsc_overhead;

/*
 * ===========================================================================
 * Initialization
 * ===========================================================================
 */

errval_t smlt_bench_ctl_init(struct smlt_bench_ctl *ctl,
                             char *label,
                             uint32_t num_measurements);

void smlt_bench_ctl_reset(struct smlt_bench_ctl *ctl);


/*
 * ===========================================================================
 * Measuring
 * ===========================================================================
 */


static inline void smlt_bench_ctl_start(struct smlt_bench_ctl *ctl)
{
    ctl->last_tsc = smlt_arch_tsc();
}

static inline void smlt_bench_clt_add_value(struct smlt_bench_ctl *ctl,
                                            cycles_t value)
{
    ctl->count++;
    ctl->data[ctl->idx] = value;

    if (++ctl->idx == ctl->max_data) {
        ctl->idx = 0;
    }
}

static inline void smlt_bench_ctl_add_measurement(struct smlt_bench_ctl *ctl)
{
    cycles_t tsc = smlt_arch_tsc();
    smlt_bench_clt_add_value(ctl, tsc - ctl->last_tsc - smlt_bench_tsc_overhead);
    ctl->last_tsc = tsc;
}



/*
 * ===========================================================================
 * Analysis
 * ===========================================================================
 */

/**
 * @brief prepares the analysis of the measurements
 *
 * @param ctl       bench contorl structure
 * @param ignore    percentage to ignore
 */
void smlt_bench_ctl_prepare_analysis(struct smlt_bench_ctl *ctl,
                                     double ignore);

void smlt_bench_ctl_get_analysis(struct smlt_bench_ctl *ctl,
                                 struct smlt_bench_analyzed *a);


/*
 * ===========================================================================
 * Display
 * ===========================================================================
 */

void smlt_bench_ctl_print_analysis(struct smlt_bench_ctl *ctl);

void smlt_bench_clt_print_data(struct smlt_bench_ctl *ctl);

#endif /* SMLT_BENCH_H_ */
