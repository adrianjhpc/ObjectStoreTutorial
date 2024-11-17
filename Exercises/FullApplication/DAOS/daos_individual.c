#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <daos.h>
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

  // Hard code the pool name
  const char *pool_string = "default-pool"; 

  // Hard code the container name
  const char *container_name = "default-container";

  // Data object name
  char objname[100];

  // Array name
  char array_name[100];

  // DAOS handles and variables
  daos_handle_t pool_handle;
  daos_handle_t container_handle;
  uuid_t container_uuid;
  d_iov_t global_handle;
  daos_cont_info_t container_info;
  daos_prop_t *container_properties;
  daos_obj_id_t array_obj_id;
  uint64_t container_obj_id;
  uuid_t array_uuid;
  daos_handle_t array_handle;
  size_t local_block_size, cell_size, total_size;
  daos_array_iod_t iod;
  d_sg_list_t sgl;
  daos_range_t rg;
  d_iov_t iov;

  uuid_t seed;

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

  // Initialise DAOS to start with
  ierr = daos_init();

  ierr = uuid_parse("00000000-0000-0000-0000-000000000000", seed);
  if(ierr != 0){
    printf("Error doing the initial seed parse");
    perror("uuid_parse");
    MPI_Abort(MPI_COMM_WORLD, 0);
    return 0;
  }


  MPI_Barrier(MPI_COMM_WORLD);

  start_time = MPI_Wtime();

  // First we need to connect to the DAOS pool. We will do this on a single and then share the DAOS pool handle using MPI as this can improve performance
  if(mpi_rank == 0){

    ierr = daos_pool_connect(pool_string, NULL, DAOS_PC_RW, &pool_handle, NULL, NULL);
    if(ierr){
      printf("Problem connecting to the daos pool %s (%d)\n", pool_string, ierr);
      perror("daos_pool_connect");
      MPI_Abort(MPI_COMM_WORLD, 0);
      return 0;
    }

  }

  // Distribute the DAOS pool handle. This is an optimisation and isn't strictly necessary
  // You could simply open the pool (connect to it) from all processes in parallel, but it can 
  // be expensive at scale
  global_handle.iov_buf = NULL;
  global_handle.iov_buf_len = 0;
  global_handle.iov_len = 0;

  // Convert the local pool handle on the rank 0 process to a global handle that can be used by all processes
  if(mpi_rank == 0){
    ierr = daos_pool_local2global(pool_handle, &global_handle);
    global_handle.iov_len = global_handle.iov_buf_len;
    global_handle.iov_buf = malloc(global_handle.iov_buf_len);
    ierr = daos_pool_local2global(pool_handle, &global_handle);

    if(ierr != 0){
      printf("Error converting local pool handle to global pool handle\n");
      perror("daos_cont_local2global");
      MPI_Abort(MPI_COMM_WORLD, 0);
      return 0;
    }
  }

  // Send the size of the handle to all nodes
  ierr = MPI_Bcast(&global_handle.iov_buf_len, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);

  // Allocate space for the global handle for the receipt of the data on all processes
  if(mpi_rank != 0){
    global_handle.iov_len = global_handle.iov_buf_len;
    global_handle.iov_buf = malloc(global_handle.iov_buf_len);
  } 

  // Now send the global hand to all processes
  ierr = MPI_Bcast(global_handle.iov_buf, global_handle.iov_buf_len, MPI_BYTE, 0, MPI_COMM_WORLD);

  // Convert the global handle that has been received into a local handle the process can use
  if (mpi_rank != 0) {
    ierr = daos_pool_global2local(global_handle, &pool_handle);
    if(ierr != 0){
      printf("Error converting global pool handle to local pool handle\n");
      perror("daos_pool_global2local");
      MPI_Abort(MPI_COMM_WORLD, 0);
      return 0;
    }
  }

  free(global_handle.iov_buf);


  // Now we have a DAOS pool we can use on all processes we need to create or open a container to use for our I/O
  // For this we can use the daos_cont_open function, we have already created the pool handle you need (above)
  // and the container name is hard coded in the container_name variable. You can open using read/write permissions
  // (i.e. the flag variable can be DAOS_COO_RW) and you can set the info and event handles to NULL
  if(mpi_rank == 0){
    ierr = daos_cont_open(pool_handle, container_name, DAOS_COO_RW, &container_handle, NULL, NULL);

    // Below is code that will deal with any container open problems and try to address them. You can just use this as provided
    if (ierr == -1005) {

      container_properties = daos_prop_alloc(1);
      container_properties->dpp_entries[0].dpe_type = DAOS_PROP_CO_LABEL;
      ierr = daos_prop_set_str(container_properties, DAOS_PROP_CO_LABEL, container_name, strlen(container_name));
      if(ierr != 0){
        printf("Error doing the property set for the container %d\n",ierr);
        perror("daos_prop_set_str");
        MPI_Abort(MPI_COMM_WORLD, 0);
        return 0;
      }

      ierr = daos_cont_create(pool_handle, NULL, container_properties, NULL);
      if(ierr != 0 && ierr != -1004){
        printf("Error doing the container create %d\n", ierr);
        perror("daos_cont_create");
        MPI_Abort(MPI_COMM_WORLD, 0);
        return 0;
      }

      ierr = daos_cont_open(pool_handle, container_name, DAOS_COO_RW, &container_handle, NULL, NULL);
      if(ierr != 0){
        printf("Error opening the container %d\n", ierr);
        perror("daos_cont_open");
        MPI_Abort(MPI_COMM_WORLD, 0);
        return 0;
      }

    }else if(ierr != 0){

      printf("Error opening the container %d\n", ierr);
      perror("daos_cont_open");
      MPI_Abort(MPI_COMM_WORLD, 0);
      return 0;

    }

  }

  // Distribute the DAOS container handle. This is an optimisation and isn't strictly necessary
  // You could simply open the container (connect to it) from all processes in parallel, but it can
  // be expensive at scale
  global_handle.iov_buf = NULL;
  global_handle.iov_buf_len = 0;
  global_handle.iov_len = 0;

  if(mpi_rank == 0){
    ierr = daos_cont_local2global(container_handle, &global_handle);
    global_handle.iov_len = global_handle.iov_buf_len;
    global_handle.iov_buf = malloc(global_handle.iov_buf_len);
    ierr = daos_cont_local2global(container_handle, &global_handle);
    if(ierr != 0){
      printf("Error converting local container handle to global container handle\n");
      perror("daos_cont_local2global");
      MPI_Abort(MPI_COMM_WORLD, 0);
      return 0;
    }
  }

  ierr = MPI_Bcast(&global_handle.iov_buf_len, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);

  if(mpi_rank != 0){
    global_handle.iov_len = global_handle.iov_buf_len;
    global_handle.iov_buf = malloc(global_handle.iov_buf_len);
  }

  ierr = MPI_Bcast(global_handle.iov_buf, global_handle.iov_buf_len, MPI_BYTE, 0, MPI_COMM_WORLD);

  if (mpi_rank != 0) {
    ierr = daos_cont_global2local(pool_handle, global_handle, &container_handle);
    if(ierr != 0){
      printf("Error converting global container handle to local container handle\n");
      perror("daos_cont_global2local");
      MPI_Abort(MPI_COMM_WORLD, 0);
      return 0;
    }
  }

  // Create the name for the data object to be written
  snprintf(objname, 100, "tu004-process-rank-%d", mpi_rank);

  // Next we create an object id (based on the objname) for the DAOS data object we are creating
  array_obj_id.hi = 0;
  array_obj_id.lo = 0;

  uuid_generate_md5(array_uuid, seed, array_name, strlen(array_name));

  memcpy(&(array_obj_id.hi), &(array_uuid[0]) + sizeof(uint64_t), sizeof(uint64_t));
  memcpy(&(array_obj_id.lo), &(array_uuid[0]), sizeof(uint64_t));


  // Now we need to create an object ID for our data we will be writing out to DAOS
  // The function to do this is daos_array_generate_oid, using the container_handle 
  // and array_obj_id you have already crrated, then the object attribute of DAOS_OT_ARRAY_BYTE, 
  // the object class we have hard coded to OC_EC_2P1G8 at the moment to match the container we are using
  // The hints and args can both be set to 0
  daos_array_generate_oid(container_handle, &array_obj_id, DAOS_OT_ARRAY_BYTE, OC_EC_2P1G8, 0, 0);

  // Now create the array to use for storing out data. This can be done with the daos_array_create function
  // using the container handle and array_obj_id we have already created. You can use DAOS_TX_NONE for the transaction
  // handle, a cell size of 1 and a chunk size of the size of data we want to save, i.e. nx*ng*ny*ng 
  ierr = daos_array_create(container_handle, array_obj_id, DAOS_TX_NONE, 1, nx*ng*ny*ng, &array_handle, NULL);


  if (ierr == -1004) {
    ierr = daos_array_open(container_handle, array_obj_id, DAOS_TX_NONE, DAOS_OO_RW, &cell_size, &local_block_size, &array_handle, NULL);
    if (ierr != 0) {
      printf("array open failed with %d", ierr);
      perror("daos_array_open");
      MPI_Abort(MPI_COMM_WORLD, 0);
    }
  } else if (ierr != 0) {
    printf("array create failed with %d", ierr);
    perror("daos_array_create");
    MPI_Abort(MPI_COMM_WORLD, 0);
  }

  // Now we need to define how much data we want to write
  // We will create this for you
  total_size = sizeof(double)*ny*ng*nx*ng;
  
  // This is where you will need to add code to write out the individual process data to the DAOS object store via libdaos
  // For this you can use the daos_array_write function
  for (i = 1; i < (ny+1)*ng; i++) {

    // These data structures hold the size and shape of the data you want to write
    // iod variables (ranges) represent the position in the DAOS array (i.e. the DAOS array where the data will be stored)
    // sgl variables (scatter/gather) represent the position in the source array (i.e. in memory array the data is coming from)
    iod.arr_nr = 1;
    rg.rg_len = nx*ng;
    rg.rg_idx = 0;
    iod.arr_rgs = &rg;
    sgl.sg_nr = 1;
    d_iov_set(&iov, &data[i], nx*ng);
    sgl.sg_iovs = &iov;

    // Now write the data, using the daos_array_write function
    // You can use the array handle crrated in the daos_array_create call and the iod and sgl
    // variables above. The transaction variable can be set to DAOS_TX_NONE, and the event handle can be NULL
    ierr = daos_array_write(array_handle, DAOS_TX_NONE, &iod, &sgl, NULL);
  }


  // Now we have finished writing the data we can close the array
  // Use the daos_array_close to close the array_handle and free the associated resources
  ierr = daos_array_close(array_handle, NULL);


  MPI_Barrier(MPI_COMM_WORLD);
  
  end_time = MPI_Wtime();

  free(data);
  free(dataArray);

  if(ierr != 0){
    printf("Error destroying array %d\n", ierr);
    perror("daos_array_destroy");
    MPI_Abort(MPI_COMM_WORLD, 0);
  }



  // Close the DAOS container. To do this you can use the daos_cont_close function
  ierr = daos_cont_close(container_handle, NULL);
  if(ierr != 0){
    printf("Error closing container %d\n", ierr);
    perror("daos_cont_close");
    MPI_Abort(MPI_COMM_WORLD, 0);
  }


  // Close the DAOS pool. To do this you can use the daos_pool_disconnect handle with the pool_handle variable
  ierr = daos_pool_disconnect(pool_handle, NULL);
  if(ierr != 0){
    printf("Error closing pool\n");
    perror("daos_pool_close");
    MPI_Abort(MPI_COMM_WORLD, 0);
  }


  // Finalise DAOS after everything is done
  ierr = daos_fini();
  if(ierr != 0){
    printf("Error finalising DAOS\n");
    perror("daos_fini");
    MPI_Abort(MPI_COMM_WORLD, 0);
  }


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



