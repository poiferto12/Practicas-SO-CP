#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mpi.h> 

int main(int argc, char *argv[])
{
    int  n, done = 0, count, rank, total_count, size;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z;
    MPI_Status status;

    MPI_Init(&argc, &argv);  // Inicializamos MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Identificador del proceso
    MPI_Comm_size(MPI_COMM_WORLD, &size);  // Total de procesos
    
    while(!done){
        count = 0; 
        if (rank == 0){
            printf("Enter the number of points: (0 quits) \n");
            scanf("%d",&n);

            for(int i = 1; i<size; i++){
                MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);  // Enviamos el número de puntos a cada proceso
            }
        } else{
            MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);  // Recibimos el número de puntos del proceso 0
        }
        
        if (n == 0) {
            MPI_Finalize();
            return 0;
        }

        srand(time(NULL) + rank);  

        //Cada proceso calcula su trozo
        for (int i = rank; i <= n; i+=size) {
            x = ((double) rand()) / ((double) RAND_MAX);
            y = ((double) rand()) / ((double) RAND_MAX);
            z = sqrt((x*x)+(y*y));
            if(z <= 1.0)
                    count++;
        }

        // Si es un proceso distinto de 0, enviar resultados
        if(rank != 0){
            MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }else{ // Si es 0, recibir resultados
            total_count = count;
            for(int i = 1; i<size; i++){
                MPI_Recv(&count, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
                total_count += count;
            }
            // Calcular el resultado final y mostrarlo
            pi = ((double) total_count/(double) n)*4.0;
            printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
        }
    }

    MPI_Finalize();
    return 0;
}