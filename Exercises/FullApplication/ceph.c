#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdf5.h"
#include "mpi.h"

void initialise_array(int, int, int, int, double **);


int main(int argc, char *argv[]) {

  int i, j;

  int proc;

  int nx, ny;
  int ng;

  double *dataArray;
  double **data;

  int mpi_rank, num_procs;
  int ierr;

  double start_time, end_time;
 
  MPI_Status stat;

  MPI_Init(&argc, &argv);
  
  ierr = MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
  ierr = MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

  if(mpi_rank == 0){
    printf("Running on %d MPI processes\n", num_procs);
  }

  nx = 10;
  ny = 10;

  ng = 2;


  dataArray = (double *) malloc(sizeof(double)*(nx+2*ng)*(ny+2*ng));
  data = (double **) malloc(sizeof(double *)*(ny+2*ng));

  for (i = 0; i < ny+2*ng; i++) {
    data[i] = dataArray + i*(nx+2*ng);
  }

  initialise_array(nx, ny, ng, mpi_rank, data);

  MPI_Barrier(MPI_COMM_WORLD);

  start_time = MPI_Wtime();

  // This is where you will need to add code to write out the individual process data to the Ceph object store via librados
  
  //
  

  MPI_Barrier(MPI_COMM_WORLD);
  
  end_time = MPI_Wtime();

  free(data);
  free(dataArray);

  MPI_Finalize();

  if(mpi_rank == 0){
    printf("Created and written data in %lf seconds\n", end_time-start_time);
    printf("Finished\n");
    fflush(stdout);
  }

  return 0;
}
  


  
void initialise_array(int nx, int ny, int ng, int mpi_rank, double **array) {

  int i, j;

  for (j = 0; j < ny+2*ng; j++) {
    for (i = 0; i < nx+2*ng; i++) {

      array[j][i] = 0.0;

    }
  }

  
  for (j = ng; j < ny+ng; j++) {
    for (i = ng; i < nx+ng; i++) {

      array[j][i] = (double) (i+j+100*mpi_rank);

    }
  }

}



