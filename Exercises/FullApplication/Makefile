CC=mpicc
CFLAGS=-I/home/software/phdf5/1.12.0/intelmpi/include

.PHONY: default
default: hdf5 daos_individual daos_array 

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

hdf5: hdf5.o
	$(CC) -o $@ hdf5.o -L/home/software/phdf5/1.12.0/intelmpi/lib -lhdf5

daos_individual: daos_individual.o
	$(CC) -o $@ daos_individual.o -ldaos -ldaos_common -L. -luuid

daos_array: daos_array.o
	$(CC) -o $@ daos_array.o -ldaos -ldaos_common -L. -luuid

clean:
	rm -fr hdf5 daos_individual daos_array  *.o
