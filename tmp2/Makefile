CC = gcc
CFLAGS = -g -w


test: test.o my_pthread.o my_malloc.o virt.o
	$(CC) $(CFLAGS) -o test test.c my_pthread.c my_malloc.c virt.c

clean:
	rm -rf test *.o 
