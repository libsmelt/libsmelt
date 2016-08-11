SMLT_DIR=../../

export LD_LIBRARY_PATH=${SMLT_DIR}:$LD_LIBRARY_PATH
gcc -O0 -std=c99 -D_GNU_SOURCE -L${SMLT_DIR} -I${SMLT_DIR}/inc -I${SMLT_DIR}/inc/backends/ -I. -o barbench barrier.c mcs.c -lpthread -lnuma -lsmltrt -lm
gcc -O0 -std=c99 -D_GNU_SOURCE -DTHROUGHPUT -L${SMLT_DIR} -I${SMLT_DIR}/inc -I${SMLT_DIR}/inc/backends/ -I. -o barbench_throughput barrier.c mcs.c -lpthread -lnuma -lsmltrt -lm
