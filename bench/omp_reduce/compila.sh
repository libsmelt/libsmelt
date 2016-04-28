
gcc -O0 -std=c99 -D_GNU_SOURCE -fopenmp ./omp_red_bench.c -lnuma -lm -lpthread -o omp_red_bench
#gcc -std=c99 parse_cores.c -lnuma -lm -o parse_cores
