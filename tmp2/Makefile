CC = gcc
CFLAGS = -g -Wall


test: test.o my_pthread.o my_malloc.o virt.o swap.o
	$(CC) $(CFLAGS) -o test test.c my_pthread.c my_malloc.c virt.c swap.c

clean:
	rm -rf test *.o 
