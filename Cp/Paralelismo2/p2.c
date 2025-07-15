#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <mpi.h>

int MPI_FlattreeColectiva(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
	int rango, nprocs, out;
	int *recv;

	MPI_Comm_rank(comm, &rango);
	MPI_Comm_size(comm, &nprocs);

	out = MPI_SUCCESS;

	if (rango == root) {
        recv = malloc(sizeof(int) * count);
		memcpy(recvbuf, sendbuf, count * sizeof(int));

		for (int i = 0; i < nprocs; i++) {
			if (i == root)
				continue;

			out = MPI_Recv(recv, count, datatype, i, 1, comm, MPI_STATUS_IGNORE);
			for (int j = 0; j < count; j++) {
				((int *)recvbuf)[j] += recv[j];
			}
		}
		free(recv);
	} else {
		out = MPI_Send(sendbuf, count, datatype, root, 1, comm);
	}

	return out;
}

int MPI_BinomialColectiva(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    int rank, size, err;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
        
    for (int step = 0; step < (size/2 + 1); step++) {
        int distancia = pow(2,step);  // 2^step
        
        if (rank < distancia) {
            int target = rank + distancia;
            if (target < size) {
                err = MPI_Send(buffer, count, datatype, target, 0, comm);
                if (err != MPI_SUCCESS) return err;
            }
        } else if (rank >= distancia && rank < (distancia * 2)) {
            int source = rank - distancia;
            err = MPI_Recv(buffer, count, datatype, source, 0, comm, MPI_STATUS_IGNORE);
            if (err != MPI_SUCCESS) return err;
        }
    }
    
    return MPI_SUCCESS;
}


int main(int argc, char *argv[])
{
    int n, done = 0, count, rank, total_count, size;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    while (!done) {
        if (rank == 0) {
            printf("Enter the number of points: (0 quits)\n");
            scanf("%d", &n);
        }

        //MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_BinomialColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (n == 0) break;

        srand(time(NULL) + rank);
        count = 0;

        for (int i = rank; i <= n; i += size) {
            x = ((double) rand()) / RAND_MAX;
            y = ((double) rand()) / RAND_MAX;
            z = sqrt(x * x + y * y);
            if (z <= 1.0) count++;
        }

        //MPI_Reduce(&count, &total_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_FlattreeColectiva(&count, &total_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        if (rank == 0) {
            pi = ((double) total_count / (double) n) * 4.0;
            printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
        }
    }

    MPI_Finalize();
    return 0;
}
