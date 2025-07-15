/*The Mandelbrot set is a fractal that is defined as the set of points c
in the complex plane for which the sequence z_{n+1} = z_n^2 + c
with z_0 = 0 does not tend to infinity.*/

/*This code computes an image of the Mandelbrot set using MPI.*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 1

#define          X_RESN  1024  /* x resolution */
#define          Y_RESN  1024  /* y resolution */

/* Boundaries of the mandelbrot set */
#define           X_MIN  -2.0
#define           X_MAX   2.0
#define           Y_MIN  -2.0
#define           Y_MAX   2.0

/* More iterations -> more detailed image & higher computational cost */
#define   maxIterations  1000

typedef struct complextype
{
  float real, imag;
} Compl;

static inline double get_seconds(struct timeval t_ini, struct timeval t_end)
{
  return (t_end.tv_usec - t_ini.tv_usec) / 1E6 +
         (t_end.tv_sec - t_ini.tv_sec);
}

int main(int argc, char *argv[])
{
  /* MPI variables */
  int myid, numprocs;
  int rows_per_proc;
  double comp_time = 0.0, comm_time = 0.0;

  /* Mandelbrot variables */
  int i, j, k;
  Compl   z, c;
  float   lengthsq, temp;
  int *vres = NULL;      
  int *local_vres = NULL; 

  /* Timestamp variables */
  struct timeval  ti, tf, t_comp_start, t_comp_end, t_comm_start, t_comm_end;

  /* Initialize MPI */
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

  /* Calculate rows per process - ensure all processes have the same number of rows */
  rows_per_proc = (Y_RESN + numprocs - 1) / numprocs;

  /* Start measuring total time */

  gettimeofday(&ti, NULL);

  /* Allocate memory for local results */
  local_vres = (int *) malloc(rows_per_proc * X_RESN * sizeof(int));
  if (!local_vres) {
    fprintf(stderr, "Process %d: Error allocating memory\n", myid);
    MPI_Abort(MPI_COMM_WORLD, 1);
    return 1;
  }

  /* Process 0 allocates memory for the complete result */
  if (myid == 0) {
    vres = (int *) malloc(numprocs * rows_per_proc * X_RESN * sizeof(int));
    if (!vres) {
      fprintf(stderr, "Process 0: Error allocating memory\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
      return 1;
    }
  }

  /* Start measuring computation time */

  gettimeofday(&t_comp_start, NULL);

  /* Calculate the local part of the Mandelbrot set */
  for (i = 0; i < rows_per_proc; i++) {
    /* Calculate the global row index */
    int global_i = myid * rows_per_proc + i;
    
    /* Skip if this row is beyond the image height */
    if (global_i >= Y_RESN) {
      /* Fill with zeros for rows beyond the image height to maintain uniform size */
      for (j = 0; j < X_RESN; j++) {
        local_vres[i * X_RESN + j] = 0;
      }
      continue;
    }

    for (j = 0; j < X_RESN; j++) {
      z.real = z.imag = 0.0;
      c.real = X_MIN + j * (X_MAX - X_MIN) / X_RESN;
      c.imag = Y_MAX - global_i * (Y_MAX - Y_MIN) / Y_RESN;
      k = 0;

      do {    /* iterate for pixel color */
        temp = z.real * z.real - z.imag * z.imag + c.real;
        z.imag = 2.0 * z.real * z.imag + c.imag;
        z.real = temp;
        lengthsq = z.real * z.real + z.imag * z.imag;
        k++;
      } while (lengthsq < 4.0 && k < maxIterations);

      if (k >= maxIterations) local_vres[i * X_RESN + j] = 0;
      else local_vres[i * X_RESN + j] = k;
    }
  }

  /* End measuring computation time */

  gettimeofday(&t_comp_end, NULL);
  comp_time = get_seconds(t_comp_start, t_comp_end);


  /* Start measuring communication time */

  gettimeofday(&t_comm_start, NULL);


  /* Gather results from all processes to process 0 */
  MPI_Gather(local_vres, rows_per_proc * X_RESN, MPI_INT,
             vres, rows_per_proc * X_RESN, MPI_INT,
             0, MPI_COMM_WORLD);

  /* End measuring communication time */

  gettimeofday(&t_comm_end, NULL);
  comm_time = get_seconds(t_comm_start, t_comm_end);


  /* Print computation and communication times for process 0 */

  fprintf(stderr,"Process %d: Computation time = %lf seconds, Communication time = %lf seconds\n", 
           myid, comp_time, comm_time);
  

  /* End measuring total time and print results */
  if (myid == 0) {
    gettimeofday(&tf, NULL);
    fprintf(stderr, "(PERF) Total time (seconds) = %lf\n", get_seconds(ti, tf));

    /* Print result out */
    if (DEBUG) {
      for (i = 0; i < Y_RESN; i++) {
        /* Skip rows beyond the image height (due to padding) */
        if (i >= Y_RESN) continue;
        
        for (j = 0; j < X_RESN; j++) {
          printf("%3d ", vres[i * X_RESN + j]);
        }
        printf("\n");
      }
    }

    /* Free global resources */
    free(vres);
  }

  /* Free local resources */
  free(local_vres);

  /* Finalize MPI */
  MPI_Finalize();

  return 0;
}