CC=mpicc
CFLAGS=-I/home/tu001/hdf5/1.14.5/include

.PHONY: default
default: hdf5 daos ceph

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

hdf5: hdf5.o
	$(CC) -o $@ hdf5.o -L/home/tu001/hdf5/1.14.5/lib -lhdf5

daos: daos.o
	$(CC) -o $@ daos.o -ldaos

ceph: ceph.o
	$(CC) -o $@ ceph.o -lrados

clean:
	rm -fr hdf5 daos ceph *.o
