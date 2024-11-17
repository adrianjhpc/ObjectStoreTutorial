CC=mpicc
CFLAGS=-I/home/tu001/hdf5/1.14.5/include
LDFLAGS=-L/home/tu001/hdf5/1.14.5/lib -lhdf5

.PHONY: default
default: hdf5 daos ceph

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

hdf5: hdf5.o
	$(CC) -o $@ hdf5.o $(LDFLAGS)

daos: daos.o
	$(CC) -o $@ daos.o

ceph: ceph.o
	$(CC) -o $@ ceph.o $(LDFLAGS)

clean:
	rm -fr hdf5 daos ceph *.o
