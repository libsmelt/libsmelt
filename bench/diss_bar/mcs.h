/*
 * Copyright (c) 2011 The Regents of the University of California
 * Andrew Waterman <waterman@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 *
 * This file is part of Parlib.
 * 
 * Parlib is free software: you can redistribute it and/or modify
 * it under the terms of the Lesser GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Parlib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Lesser GNU General Public License for more details.
 * 
 * See COPYING.LESSER for details on the GNU Lesser General Public License.
 * See COPYING for details on the GNU General Public License.
 */

#ifndef _MCS_H
#define _MCS_H

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wgnu"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MCS_LOCK_INIT {0}
#define MCS_PDRLOCK_INIT {0}
#define MCS_QNODE_INIT {0,0}

static __inline void
cpu_relax(void)
{
	__asm volatile("pause" : : : "memory");
}

/* Note that we need no lock prefix.  */
#define atomic_exchange_acq(mem, newvalue) \
  ({ __typeof (*mem) result;						      \
     if (sizeof (*mem) == 1)						      \
       __asm __volatile ("xchgb %b0, %1"				      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (newvalue), "m" (*mem));			      \
     else if (sizeof (*mem) == 2)					      \
       __asm __volatile ("xchgw %w0, %1"				      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (newvalue), "m" (*mem));			      \
     else if (sizeof (*mem) == 4)					      \
       __asm __volatile ("xchgl %0, %1"					      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" (newvalue), "m" (*mem));			      \
     else								      \
       __asm __volatile ("xchgq %q0, %1"				      \
			 : "=r" (result), "=m" (*mem)			      \
			 : "0" ((long) (newvalue)), "m" (*mem));	      \
     result; })

#ifndef BARRELFISH
  //typedef unsigned int coreid_t;
#endif
#define MCS_LOG2_MAX_CORES 8
#define CACHE_LINE_SIZE 64

typedef struct mcs_lock_qnode
{
	volatile struct mcs_lock_qnode* volatile next;
	volatile int locked;
} mcs_lock_qnode_t;

typedef struct mcs_lock
{
	mcs_lock_qnode_t* lock;
} mcs_lock_t;

typedef struct mcs_pdr_lock
{
	mcs_lock_qnode_t* lock;
} mcs_pdr_lock_t;

typedef struct mcs_dissem_flags
{
	volatile int myflags[2][MCS_LOG2_MAX_CORES];
	volatile int* partnerflags[2][MCS_LOG2_MAX_CORES];
	int parity;
	int sense;
	char pad[CACHE_LINE_SIZE];
} __attribute__((aligned(CACHE_LINE_SIZE))) mcs_dissem_flags_t;

typedef struct mcs_barrier_t
{
	size_t nprocs;
	mcs_dissem_flags_t* allnodes;
	size_t logp;
} mcs_barrier_t;

void mcs_lock_init(struct mcs_lock *lock);
void mcs_pdr_init(struct mcs_pdr_lock *pdr_lock);
/* Caller needs to alloc (and zero) their own qnode to spin on.  The memory
 * should be on a cacheline that is 'per-thread'.  This could be on the stack,
 * in a thread control block, etc. */
void mcs_lock_lock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode);
void mcs_lock_unlock(struct mcs_lock *lock, struct mcs_lock_qnode *qnode);
void mcs_pdr_lock(struct mcs_pdr_lock *pdr_lock, struct mcs_lock_qnode *qnode);
void mcs_pdr_unlock(struct mcs_pdr_lock *pdr_lock, struct mcs_lock_qnode *qnode);

void mcs_barrier_init(mcs_barrier_t* b, size_t nprocs);
void mcs_barrier_wait(mcs_barrier_t* b, size_t vcoreid);

#ifdef __cplusplus
}
#endif

#endif
