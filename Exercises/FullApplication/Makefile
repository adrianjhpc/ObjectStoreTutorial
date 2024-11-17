CC=mpicc
CFLAGS=-I/home/tu001/hdf5/1.14.5/include
LDFLAGS=-L/home/tu001/hdf5/1.14.5/lib -lhdf5

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

example: main.o
	$(CC) -o example main.o ${LDFLAGS}

clean:
	rm -fr example main.o
