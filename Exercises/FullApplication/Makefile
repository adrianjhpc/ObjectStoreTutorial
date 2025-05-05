CC=mpicc
CFLAGS=-Iuuid/include
.PHONY: default
default: daos_individual daos_array


%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

daos_individual: daos_individual.o
	$(CC) -o $@ daos_individual.o -ldaos -ldaos_common -Luuid/lib -luuid

daos_array: daos_array.o
	$(CC) -o $@ daos_array.o -ldaos -ldaos_common -Luuid/lib -luuid

clean:
	rm -fr daos_individual daos_array *.o
