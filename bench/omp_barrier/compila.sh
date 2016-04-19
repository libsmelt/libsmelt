
gcc -O0 -std=c99 -fopenmp ./omp_bar_bench.c -o omp_bar_bench
gcc -std=c99 parse_cores.c -lnuma -lm -o parse_cores
