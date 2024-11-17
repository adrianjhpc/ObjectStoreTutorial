CC=mpicc
CFLAGS=-I/home/tu001/hdf5/1.14.5/include

.PHONY: default
default: hdf5 daos_individual daos_array ceph

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

hdf5: hdf5.o
	$(CC) -o $@ hdf5.o -L/home/tu001/hdf5/1.14.5/lib -lhdf5

daos_individual: daos_individual.o
	$(CC) -o $@ daos_individual.o -ldaos -ldaos_common -luuid

daos_array: daos_array.o
	$(CC) -o $@ daos_array.o -ldaos -ldaos_common -luuid

ceph: ceph.o
	$(CC) -o $@ ceph.o -lrados

clean:
	rm -fr hdf5 daos_individual daos_array ceph *.o
