#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rados/librados.h>
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
 
  // librados configuration/system data holder
  rados_t cluster;

  // I/O context for librados operations
  rados_ioctx_t ioctx;

  // Character array for object name
  char objname[100];

  // Write operation for the indexing object we will create
  rados_write_op_t write_op;

  // Character array for the key name for each data object written
  char key[100];
 
  // Variables for creating the key value objects
  size_t key_len;
  size_t objname_len;
  char * key_vector;
  char * objname_vector;

  MPI_Init(&argc, &argv);
  
  ierr = MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
  ierr = MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

  if(mpi_rank == 0){
    printf("Running on %d MPI processes\n", num_procs);
  }

  nx = 10;
  ny = 10;

  ng = 1;


  dataArray = (double *) malloc(sizeof(double)*(nx+2*ng)*(ny+2*ng));
  data = (double **) malloc(sizeof(double *)*(ny+2*ng));

  for (i = 0; i < ny+2*ng; i++) {
    data[i] = dataArray + i*(nx+2*ng);
  }

  initialise_array(nx, ny, ng, mpi_rank, data);

  MPI_Barrier(MPI_COMM_WORLD);

  start_time = MPI_Wtime();

  // First we need to create the ceph handle.
  // You can use the rados_create2 function to do this
  // clustername can be "ceph"
  // name can be set as "client.tutorial"
  ierr = rados_create2(&cluster, "ceph", "client.tutorial", 0);

  // Add in error handling to skip the rest if the handle creation doesn't work but ensure we can still clean up safely.
  // Goto is a nasty programming approach, but serves a useful function for this error handling.
  if (ierr < 0) {
    printf("rados_create2 failed\n");
    goto mpi_fini;
  }

  // Read in the Ceph configuration to setup the cluster object
  // For this you can use the rados_conf_read_file function
  // The location of the configuration file "/opt/apps/ceph/ceph.conf"
  ierr = rados_conf_read_file(cluster, "/opt/apps/ceph/ceph.conf");

  if (ierr < 0) {
    printf("rados_conf_read_file failed\n");
    goto mpi_fini;
  }


  // Now connect to the Ceph cluster using the rados_connect function
  ierr = rados_connect(cluster);

  if (ierr < 0) {
    printf("rados_connect failed\n");
    goto mpi_fini;
  }

  // Now we need to connect to a ceph pool to use for I/O. This creates an io context for I/O operations
  // To do this you can use the rados_ioctl_create function, using the cluster variable you have recreated above
  // and the pool name as "default-pool"
  ierr = rados_ioctx_create(cluster, "default-pool", &ioctx);

  if (ierr < 0 ) {
    printf("rados_ioctx_create failed %d %s\n", ierr, strerror(-ierr));
    goto disconnect;
  }


  // Now you need to create a librados namespace for your I/O.
  // This can be done using the rados_ioctx_set_namespace function
  // Use the ioctx variable you created above and use your ssh username, i.e. "tuXXX" for your namespace name
  rados_ioctx_set_namespace(ioctx, "tu001");

  // Create object name to label this as a unique object in the namespace
  snprintf(objname, 100, "process-rank-%d", mpi_rank);

  // This is where you will need to add code to write out the individual process data to the Ceph object store via librados
  // For this you can use the rados_write function, using the ioctx and objname variables you have written above
  for (i = 0; i < ny+2*ng; i++) {
    // This is where to add the rados_write function. The buf variable can be data[i] and the offset can be i*(ny+2*ng) 
    ierr = rados_write(ioctx, objname, (const char *)data[i], ny*sizeof(data), i*(ny+2*ng)*sizeof(data));
    if (ierr < 0) {
      printf("rados_write failed %s\n", strerror(-ierr));
      goto ioctx_destroy;
    }
  }

  // Now we have written the data we need to create some indexes so we can find and search the available data sets at a later data
  // We are going to create and object that gathers together the keys of the individial objects each process has now written  
  write_op = rados_create_write_op();

  // Creating a key name for each MPI process. We are writing a single object per process so we need a single key per process
  snprintf(key, 100, "rank-%d", mpi_rank);
  key_len = strlen(key);
  objname_len = strlen(objname);
  // Get a point to the key and object variables/names to use in the object create
  key_vector = &key[0];
  objname_vector = &objname[0];

  // Now we need to create a key/value pair on a object, i.e. create a key value to index the object we created previously
  // For this you can use the rados_write_op_omap_set2 function. You can use the write_op variable as the operation variable
  // The key_vector and objname_vector variables are the key and value arrays the function needs, along with the key_len and
  // objname_len sizes. The number of key value pairs to set is 1.
  // This links the key (i.e. rank-mpi_rank) to the object we created before (i.e. process-rank-mpi_rank)
  rados_write_op_omap_set2(write_op, (char const * const *) &key_vector, (char const * const *) &objname_vector, &key_len, &objname_len, 1);

  // Now create an object from this key-value pair. This collects together all the key-values pairs each process has created into a single object.
  // You can sue the rados_write_op_operate function to do this, using write_op and ioctx variables we have already created.
  // The object ID you can choose, but we would suggest "username-index", i.e "tu001-index". This will make it unique for your user in the CEph.
  // The mtime variable can be set to NULL and the flags can be set to 0.
  ierr = rados_write_op_operate(write_op, ioctx, "tu004-index", NULL, 0);

  if (ierr < 0) {
    printf("rados rados_write_op_operate failed\n");
    goto release_op;
  }


  MPI_Barrier(MPI_COMM_WORLD);
  
  end_time = MPI_Wtime();

  free(data);
  free(dataArray);

release_op:
  rados_release_write_op(write_op);

ioctx_destroy:
  rados_ioctx_destroy(ioctx);

disconnect:
  rados_shutdown(cluster);

mpi_fini:

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



