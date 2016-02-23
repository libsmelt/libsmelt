#include <stdio.h>

#if 0
#include "cwrapper.h"
#include "mp.h"





__thread int th_initialized = 0;

void c_mp_barrier0(void)
{
    mp_barrier0();
}
int c__thread_init(unsigned _tid, int _nproc)
{
    if (!th_initialized) {
        th_initialized = 1;
        return __thread_init(_tid, _nproc);
    }
    return 0;
}

void c__sync_init(int _nproc, bool init_model)
{
    return __sync_init(_nproc, init_model);
}
#endif