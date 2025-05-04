CC=mpicc
CFLAGS=

.PHONY: default
default: hdf5 daos_individual daos_array 

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

hdf5: hdf5.o
	$(CC) -o $@ hdf5.o -lhdf5

daos_individual: daos_individual.o
	$(CC) -o $@ daos_individual.o -ldaos -ldaos_common -luuid

daos_array: daos_array.o
	$(CC) -o $@ daos_array.o -ldaos -ldaos_common -luuid

clean:
	rm -fr hdf5 daos_individual daos_array *.o
