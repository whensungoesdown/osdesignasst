CC = gcc
CFLAGS = -g -Wall -Wno-unused-but-set-variable -Wno-int-conversion -Wno-implicit-function-declaration -Wno-unused-variable -Wno-incompatible-pointer-types


test: test.o my_pthread.o my_malloc.o virt.o swap.o shm.o
	$(CC) $(CFLAGS) -o test test.c my_pthread.c my_malloc.c virt.c swap.c shm.c

clean:
	rm -rf test *.o 
