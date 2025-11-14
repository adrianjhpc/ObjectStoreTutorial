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

	// generate object name and content, and write

	char objname[100];
	snprintf(objname, 100, "tu004-rank-%d", world_rank);

	unsigned char data[1024*1024];
	size_t len = 1024*1024;
	for (int i = 0; i < len; i++) {
		data[i] = rand();
	}

	rc = rados_write(ioctx, objname, data, len, 0);

	if (rc < 0) {
		printf("rados_wrtie failed\n");
		goto ioctx_destroy;
	}

	// insert entry in Omap

	rados_write_op_t write_op = rados_create_write_op();

	char key[100];
	snprintf(key, 100, "rank-%d", world_rank);

	size_t key_len = strlen(key);
	size_t objname_len = strlen(objname);

	char * key_vector = &key[0];
	char * objname_vector = &objname[0];

	rados_write_op_omap_set2(write_op, (char const * const *) &key_vector, (char const * const *) &objname_vector, &key_len, &objname_len, 1);

	rc = rados_write_op_operate(write_op, ioctx, "tu004-index", NULL, 0);

	if (rc < 0) {
		printf("rados omap set failed\n");
		goto release_op;
	}

	// done!

	printf("Host %s, rank %d / %d wrote and indexed 1 MiB\n", hostname, world_rank, world_size);

	// tear-down

	rados_release_write_op(write_op);

	rados_ioctx_destroy(ioctx);

	rados_shutdown(cluster);

	MPI_Finalize();

	return 0;

release_op:
	rados_release_write_op(write_op);

ioctx_destroy:
	rados_ioctx_destroy(ioctx);

disconnect:
	rados_shutdown(cluster);

mpi_fini:
	MPI_Finalize();

	return 1;

}
