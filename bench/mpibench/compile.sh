gcc -O0 -std=c99 -o parse_cores parse_cores.c -lnuma -lm
/home/haeckir/openmpi-1.10.2/bin/mpicc -O0 -std=c99 -D_GNU_SOURCE -o barrier barrier.c -lm
