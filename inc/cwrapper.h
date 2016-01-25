#ifndef __MYWRAPPER_H
#define __MYWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

int c__thread_init(unsigned _tid, int _nproc);

void c_mp_barrier0(void);

void c__sync_init(int _nproc, bool init_model);

#ifdef __cplusplus
}
#endif
#endif
