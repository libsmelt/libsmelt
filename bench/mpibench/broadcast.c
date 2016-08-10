/*
 * Copyright (c) 2013-2016, ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Universitaetstr. 6, CH-8092 Zurich. Attn: Systems Group.
 */
#include <mpi.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include "measurement_framework.h"

#include <stdio.h>
#define NITERS 3000
#define NVALUES 1000

struct sk_measurement mes;

void dissem_bar(void) 
{
    int m = 1;
    int nprocs;
    int rounds;
    int rank;
    int step = 1;
    int peer = 0;
    MPI_Status stat;
    MPI_Request req;

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    rounds = ceil(log2(nprocs));
    
    char payload = 0;
    for (int r = 0; r < rounds; r++) {
        for (int i = 1; i <= m; i++) {
            peer = (rank + i*step) % nprocs;
            MPI_Isend(&payload, 0, MPI_BYTE, peer, 0, MPI_COMM_WORLD, &req);
        }

        for (int i = 1; i <= m; i++) {
            peer = (rank - i*step);
            while (peer < 0) {
                peer = nprocs+peer;
            }
            MPI_Recv(&payload, 0, MPI_BYTE, peer, 0, MPI_COMM_WORLD, &stat);
        }

        MPI_Wait(&req, &stat);

        step = step*(m+1);
    }
}

int main(int argc, char **argv){
	int rank, size, root=0;	
 	
	MPI_Init(&argc,&argv);

	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);

    char outname[128];
    if (argc == 2) {
        sprintf(outname, "ab_mpisync_%d", size);
    } else {
        sprintf(outname, "ab_mpi_%d", size);
    }
    uint64_t *buf = (uint64_t*) malloc(sizeof(uint64_t)*NITERS);
    sk_m_init(&mes, NVALUES, outname, buf);

    char a = 0;
    char global = 0;
	for(int n=0; n<NITERS; n++){
        // synchro
        if (argc == 2) {
            dissem_bar();
            dissem_bar();
        }

        sk_m_restart_tsc(&mes);
        MPI_Bcast((void*) &a, 1, MPI_BYTE, 0, MPI_COMM_WORLD);
        sk_m_add(&mes);
	}

    uint64_t *buf2 = (uint64_t*) malloc(sizeof(uint64_t)*NITERS);
    if (rank == 0) {
        sk_m_print(&mes);
        for (int i = 1; i < size; i++) {
            MPI_Recv(buf2, NVALUES, MPI_UINT64_T, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (uint32_t j=0; j< NVALUES; j++) {
                char name[100];
                printf("sk_m_print(%d,%s) idx= %d tscdiff= %ld\n",
                       i, outname, j, buf2[j]);
            }   
        }
    } else {
        MPI_Send(buf, NVALUES,MPI_UINT64_T, 0, 0, MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);

	MPI_Finalize();
}

