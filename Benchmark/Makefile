CC = gcc
CFLAGS = -g -w

all:: parallelCal vectorMultiply externalCal test

parallelCal: 
	$(CC) $(CFLAGS) -pthread -o parallelCal parallelCal.c -L../code/ -I../code/ -lmy_pthread

vectorMultiply: 
	$(CC) $(CFLAGS) -pthread -o vectorMultiply vectorMultiply.c -L../code/ -I../code/ -lmy_pthread

externalCal: 
	$(CC) $(CFLAGS) -pthread -o externalCal externalCal.c -L../code/ -I../code/ -lmy_pthread

test: 
	$(CC) $(CFLAGS) -o test test.c -L../code/ -I../code/ -lmy_pthread

clean:
#	rm -rf parallelCal vectorMultiply externalCal test *.o ./record/
	rm -rf parallelCal vectorMultiply externalCal test *.o
