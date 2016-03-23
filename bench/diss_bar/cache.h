/*
 * cache.h
 *
 *  Created on: 5 Dec, 2012
 *      Author: sabela
 */

#ifndef CACHE_H_
#define CACHE_H_

#include <immintrin.h>

#define MODE_M 0x01
#define MODE_E 0x02
#define MODE_S 0x03
#define MODE_I 0x04
#define MODE_F 0x05
#define CACHELINE 64

int inline clflush(void* buffer,unsigned long long size);
void inline use_memory(void* buffer,unsigned long long memsize,int mode,int repeat);

/*
 *  * use a block of memory to ensure it is in the caches afterwards
 *   */
void inline use_memory(void* buffer,unsigned long long memsize,int mode,int repeat)
{
        int i,j;
        unsigned char tmp;
        unsigned long long stride = memsize>CACHELINE?CACHELINE:1; //just to be used if memsize>CACHELINE
        //printf("use_mode: 0x%x\n",mode);fflush(stdout);
        if ((mode==MODE_M)||(mode==MODE_E)||(mode==MODE_I)){
                j=repeat;
                while(j--){
                        for (i=0;i<=memsize-stride;i+=stride){
                                //this kernel needs the content of the buffer, so the usage must not be destructive
                                //compiler optimisation has to be disabled (-O0), otherwise the compiler could remove the following commands
                                tmp=*((unsigned char*)(buffer)+i);
                                *((unsigned char*)(buffer)+i)=tmp;
                        }
                }
                //now buffer is invalid in other caches, modified in local cache
        }

        if ((mode==MODE_E)||(mode==MODE_S)||(mode==MODE_F)){
                if (mode==MODE_E){
                        clflush(buffer,memsize);
                        //now buffer is invalid in local cache
                }
                j=repeat;
                while(j--){
                        //for (i=0;i<memsize-stride;i+=stride){//just to be used if memsize>CACHELINE
                       for (i=0;i<=memsize-stride;i+=stride){
                                tmp|=*((unsigned char*)(buffer)+i);
                        }
                        //result has to be stored somewhere to prevent the compiler from deleting the hole loop
                        //if compiler optimisation is disabled, the following command is not needed
                        //*((int*)((unsigned long long)buffer+i))=tmp;
                }
                //now buffer is exclusive or shared in local cache
        }

        if (mode==MODE_I){
                clflush(buffer,memsize);
                //now buffer is invalid in local cache too
        }
}


/**
 * * flushes content of buffer from all cache-levels
 *   * @param buffer pointer to the buffer
 *    * @param size size of buffer in Bytes
 *     * @return 0 if successful
 *      *         -1 if not available
 *       */
int inline clflush(void* buffer,unsigned long long size)
{
#if defined (__x86_64__)
        unsigned long long addr,passes,linesize;
	addr = (unsigned long long) buffer;
        linesize = CACHELINE;

        __asm__ __volatile__("mfence;"::);
//        __asm__ __volatile__("lock;""addl $0,(%%rsp)"::);

        for(passes = (size/linesize);passes>0;passes--)
        {
                __asm__ __volatile__("clflush (%%rax);":: "a" (addr));
                addr+=linesize;
                // __cdecl _mm_clevict(buffer,_MM_HINT_T0);
                // __cdecl _mm_clevict(buffer,_MM_HINT_T1);
		// buffer+=linesize;
        }
       __asm__ __volatile__("mfence;"::);
       // __asm__ __volatile__("lock;""addl $0,(%%rsp)"::);
#endif

        return 0;
}

#endif /* CACHE_H_ */
