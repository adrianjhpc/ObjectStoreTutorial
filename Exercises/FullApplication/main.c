#include <stdio.h>
#include <stdlib.h>

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

  int rank;
  hsize_t dim1d, dim2d[2];
  hid_t err;

  hssize_t start_2d[2];
  hsize_t stride_2d[2];
  hsize_t count_2d[2];

  hid_t dataspace, memspace, dataset;

  hid_t string_type;

  hid_t acc_template;

  MPI_Info FILE_INFO_TEMPLATE;

  char *filename = "output_data.hdf5";

  char comment[] = "Dataset in HDF5 format";

  hid_t file_identifier;

  int MyPE, NumPEs;
  int ierr;

  MPI_Status stat;

  MPI_Init(&argc, &argv);
  
  ierr = MPI_Comm_size(MPI_COMM_WORLD, &NumPEs);
  ierr = MPI_Comm_rank(MPI_COMM_WORLD, &MyPE);

  printf("proc %d out of %d\n", MyPE, NumPEs);

  nx = 10;
  ny = 10;

  ng = 2;


  dataArray = (double *) malloc(sizeof(double)*(nx+2*ng)*(ny+2*ng));
  data = (double **) malloc(sizeof(double *)*(ny+2*ng));

  for (i = 0; i < ny+2*ng; i++) {
    data[i] = dataArray + i*(nx+2*ng);
  }

  initialise_array(nx, ny, ng, MyPE, data);

  acc_template = H5Pcreate(H5P_FILE_ACCESS);

  ierr = MPI_Info_create(&FILE_INFO_TEMPLATE);

  ierr = H5Pset_sieve_buf_size(acc_template, 262144); 
  ierr = H5Pset_alignment(acc_template, 524288, 262144);
  
  ierr = MPI_Info_set(FILE_INFO_TEMPLATE, "access_style", "write_once");
  ierr = MPI_Info_set(FILE_INFO_TEMPLATE, "collective_buffering", "true");
  ierr = MPI_Info_set(FILE_INFO_TEMPLATE, "cb_block_size", "1048576");
  ierr = MPI_Info_set(FILE_INFO_TEMPLATE, "cb_buffer_size", "4194304");

  ierr = H5Pset_fapl_mpio(acc_template, MPI_COMM_WORLD, FILE_INFO_TEMPLATE);


  file_identifier = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, acc_template);

  ierr = H5Pclose(acc_template);
  ierr = MPI_Info_free(&FILE_INFO_TEMPLATE);
  
  string_type = H5Tcopy(H5T_C_S1);
  H5Tset_size(string_type, strlen(comment));

  rank = 1;
  dim1d = 1;

  dataspace = H5Screate_simple(rank, &dim1d, NULL);
  
  dataset = H5Dcreate(file_identifier, "comment", string_type, 
		      dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  
  if (MyPE == 0) {
    err = H5Dwrite(dataset, string_type, H5S_ALL, dataspace, 
		   H5P_DEFAULT, comment);
  }

  H5Sclose(dataspace);
  H5Dclose(dataset);

  rank = 2;

  dim2d[0] = ny;
  dim2d[1] = nx*NumPEs;

  dataspace = H5Screate_simple(rank, dim2d, NULL);
  
  start_2d[0] = 0;
  start_2d[1] = MyPE*nx;

  stride_2d[0] = 1;
  stride_2d[1] = 1;

  count_2d[0] = ny;
  count_2d[1] = nx;

  err = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, start_2d, stride_2d, count_2d, NULL);

  dim2d[0] = ny+2*ng;
  dim2d[1] = nx+2*ng;

  memspace = H5Screate_simple(rank, dim2d, NULL);
  
  start_2d[0] = ng;
  start_2d[1] = ng;

  stride_2d[0] = 1;
  stride_2d[1] = 1;

  count_2d[0] = ny;
  count_2d[1] = nx;

  err = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, 
                            start_2d, stride_2d, count_2d, NULL);
     
  dataset = H5Dcreate(file_identifier, "data array", H5T_NATIVE_DOUBLE, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

  err = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, memspace, dataspace, H5P_DEFAULT, &(data[0][0]));

  H5Sclose(memspace);
  H5Sclose(dataspace);
  H5Dclose(dataset);
    
  H5Fclose(file_identifier);

  free(data);
  free(dataArray);

  MPI_Finalize();

}
  


  
void initialise_array(int nx, int ny, int ng, int MyPE, double **array) {

  int i, j;

  for (j = 0; j < ny+2*ng; j++) {
    for (i = 0; i < nx+2*ng; i++) {

      array[j][i] = 0.0;

    }
  }

  
  for (j = ng; j < ny+ng; j++) {
    for (i = ng; i < nx+ng; i++) {

      array[j][i] = (double) (i+j+100*MyPE);

    }
  }

}



