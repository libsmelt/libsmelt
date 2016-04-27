

export LD_LIBRARY_PATH=../smelt-runtime-system/

gcc -O0 -std=c99 -D_GNU_SOURCE -L../smelt-runtime-system -I../smelt-runtime-system/inc -I../smelt-runtime-system/inc/backends/ -I. -o barbench barrier.c mcs.c -lpthread -lnuma -lsmltrt -lm 

