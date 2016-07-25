gcc -O0 -std=c99 -D_GNU_SOURCE -fopenmp ./omp_bar_bench.c -lnuma -lm -lpthread -o omp_bar_bench
gcc -O0 -std=c99 -D_GNU_SOURCE -fopenmp ./omp_red_bench.c -lnuma -lm -lpthread -o omp_red_bench
