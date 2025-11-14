#include <stdio.h>
#include <mpi.h>
#include <librados.h>
#include <stdlib.h>  // for rand

int main() {

	// initialise MPI

	MPI_Init(NULL, NULL);

	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	
	char hostname[MPI_MAX_PROCESSOR_NAME];
	int hostname_len;
	MPI_Get_processor_name(hostname, &hostname_len);
	
	printf("Starting host %s, rank %d / %d\n", hostname, world_rank, world_size);

	// initialise librados

	int rc;

	rados_t cluster;

	rc = rados_create2(&cluster, "ceph", "client.tutorial", 0);

	if (rc < 0) {
		printf("rados_create2 failed\n");
		goto mpi_fini;
	}

	rc = rados_conf_read_file(cluster, "/opt/apps/ceph/ceph.conf");

	if (rc < 0) {
		printf("rados_conf_read_file failed\n");
		goto mpi_fini;
	}

	rc = rados_connect(cluster);

	if (rc < 0) {
		printf("rados_connect failed\n");
		goto mpi_fini;
	}

	rados_ioctx_t ioctx;

	rc = rados_ioctx_create(cluster, "default-pool", &ioctx);

	if (rc < 0 ) {
		printf("rados_ioctx_create failed\n");
		goto disconnect;
	}

	rados_ioctx_set_namespace(ioctx, "tu004");

	// deindex object name from  Omap

	rados_read_op_t read_op = rados_create_read_op();

	char key[100];
	snprintf(key, 100, "rank-%d", world_rank);

	size_t key_len = strlen(key);

	char * key_vector = &key[0];

	rados_omap_iter_t iter;

	int prval;

	rados_read_op_omap_get_vals_by_keys2(read_op, (char const * const *) &key_vector, 1, &key_len, &iter, &prval);

	rc = rados_read_op_operate(read_op, ioctx, "tu004-index", 0);

	if (rc < 0 || prval < 0) {
		printf("rados omap get failed\n");
		goto release_iter;
	}

	if (rados_omap_iter_size(iter) != 1) {
		printf("rados omap get returned more values than expected\n");
		goto release_iter;
	}

	char * found_key;
	char * found_val;
	size_t found_key_len;
	size_t found_val_len;

	rc = rados_omap_get_next2(iter, &found_key, &found_val, &found_key_len, &found_val_len);

	if (rc < 0) {
		printf("rados_omap_get_next2 failed\n");
		goto release_iter;
	}

	if (found_val_len >= 100) {
		printf("found value in Omap larger than buffer\n");
		goto release_iter;
	}

	// copy value from iterator to buffer before releasing iterator

	char objname[100] = "";  // initialise with all NULL terminators

	memcpy(&objname[0], found_val, found_val_len);

	rados_omap_get_end(iter);

	// read object

	unsigned char data[1024*1024];
	size_t len = 1024*1024;

	rc = rados_read(ioctx, objname, &data[0], len, 0);

	if (rc < 0) {
		printf("rados_read failed\n");
		goto release_op;
	}

	if (rc != len) {
		printf("read less bytes than expected\n");
		goto release_op;
	}

	// done!

	printf("Host %s, rank %d / %d deindexed and read 1 MiB\n", hostname, world_rank, world_size);

	// tear-down

	rados_release_read_op(read_op);

	rados_ioctx_destroy(ioctx);

	rados_shutdown(cluster);

	MPI_Finalize();

	return 0;

release_iter:
	rados_omap_get_end(iter);

release_op:
	rados_release_read_op(read_op);

ioctx_destroy:
	rados_ioctx_destroy(ioctx);

disconnect:
	rados_shutdown(cluster);

mpi_fini:
	MPI_Finalize();

	return 1;

}
