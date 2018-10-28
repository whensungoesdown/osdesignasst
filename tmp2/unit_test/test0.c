#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "../code/my_pthread_t.h"

void *PrintHello(void * arg)
{
	//while (1) {
		printf("Hello World! It's me, thread !\n");
	//}
	//pthread_exit(NULL);
}

int main (int argc, char *argv[])
{
	pthread_t thread;
	int rc;
	
	rc = my_pthread_create(&thread, NULL, PrintHello, (void *)0);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
	}


	printf("main thread.\n");
	
	while(1) {
		//printf("main thread.\n");
	};
	/* Last thing that main() should do */
	//pthread_exit(NULL);
}
