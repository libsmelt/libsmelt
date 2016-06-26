/*
Copyright (c) 2006 The Trustees of Indiana University and Indiana
                   University Research and Technology Corporation. All
                   rights reserved.
Copyright (c) 2006 The Technical University of Chemnitz. All rights
                   reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

- Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer listed
  in this license in the documentation and/or other materials
  provided with the distribution.

- Neither the name of the copyright holders nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

The copyright holders provide no reassurances that the source code
provided does not infringe any patent, copyright, or any other
intellectual property rights of third parties.  The copyright holders
disclaim any liability to any recipient for claims brought against
recipient by any third party for infringement of that parties
intellectual property rights.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * hrtimer_x86_64.h
 *
 * Based on hrtimer from netgauge
 */

#ifndef HRTIMER_X86_64_H_
#define HRTIMER_X86_64_H_

#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include "calibrate.h"
#define UINT32_T uint32_t
#define UINT64_T uint64_t

#define HRT_INIT(print, freq) do {\
		if(print) printf("# initializing x86-64 timer (takes some seconds)\n"); \
		HRT_CALIBRATE(freq); \
} while(0)


typedef struct {
	UINT32_T l;
	UINT32_T h;
} x86_64_timeval_t;

#define HRT_TIMESTAMP_T x86_64_timeval_t

/* TODO: Do we need a while loop here? aka Is rdtsc atomic? - check in the documentation */
#define HRT_GET_TIMESTAMP(t1)  __asm__ __volatile__ ("rdtsc" : "=a" (t1.l), "=d" (t1.h));

#define HRT_GET_ELAPSED_TICKS(t1, t2, numptr)	*numptr = (((( UINT64_T ) t2.h) << 32) | t2.l) - \
		(((( UINT64_T ) t1.h) << 32) | t1.l);

#define HRT_GET_TIME(t1, time) time = (((( UINT64_T ) t1.h) << 32) | t1.l)


#endif /* HRTIMER_X86_64_H_ */
