/*
 * Copyright (c) 2016 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#ifndef SMLT_ARCH_H_
#define SMLT_ARCH_H_ 1

#include <stdbool.h>

/**
 * the architecture specific cache line size
 */
#define SMLT_ARCH_CACHELINE_SIZE 64

/**
 * size of an char type in bits.
 */
#define SMLT_ARCH_CHAR_BITS 8




/// Emit memory barrier needed between writing UMP payload and header
static inline void smlt_arch_write_barrier(void)
{
#if defined(__i386__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_IX86)
    /* the x86 memory model ensures ordering of stores, so all we need to do
     * is prevent the compiler from reordering the instructions */
# if defined(__GNUC__)
    __asm volatile ("" : : : "memory");
# elif defined(_MSC_VER)
    _WriteBarrier();
# else
# error force ordering here!
# endif
#else // !x86 (TODO: the x86 optimisation may be applicable to other architectures)
# ifdef __GNUC__
    /* use conservative GCC intrinsic */
    __sync_synchronize();
# else
#  error memory barrier here!
# endif
#endif
}


/// compare and swap helper for sleep/wakeup
static inline  bool smlt_arch_cas(int volatile *p, int old, int new_)
{
#if defined(__GNUC__)
    return __sync_bool_compare_and_swap(p, old, new_);
#elif defined(_MSC_VER)
    return _InterlockedCompareExchange(p, new_, old) == old;
#else
# error implement CAS here
#endif
}

#endif /* SMLT_ARCH_H_ */
