// test case for asst2 phase C
// There are two threads, bigspender and tester.
// For testing purpose, set physical memory to 8KB, that's 2 pages
// Bigspender will exhaust physical memory by calling malloc
// Next tester call one malloc() will trigger physical memory swap to file 


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "my_pthread_t.h"
#include "my_malloc.h"
#include "syscall.h"


int counter;
my_pthread_mutex_t lock;

void fake_sleep()
{
	for (int i = 0; i < 0x1FFFFFFF; i ++) {}
}

void* bigspender (void *arg)
{
	void* p = NULL;
	int i = 0;

	printf("hello from bigspender\n");

	while(1) {
		p = malloc(512);
		printf("bigspender: malloc buffer 512 byte, address 0x%p\n", p);

		fake_sleep();

		printf("bigspender: copy a string to the buffer\n");
		memset(p, 0, 512);
		strncpy(p, "This buffer belongs to the bigspender\n", strlen("This buffer belongs to the bigspender\n"));

		fake_sleep();

		printf("bigspender: read buffer again\n");
		printf("bigspender: %s\n", (char*)p);

		fake_sleep();
		
		printf("bigspender: free buffer\n");
		free(p);
		
		fake_sleep();
	}
	
	printf("\n Thread bigspender exit\n");
	return 0;
}

void* tester (void *arg)
{
	void* p = NULL;
	int i = 0;

	printf("hello from tester\n");

	while(1) {
		p = malloc(512);
		printf("tester: malloc buffer 512 byte, address 0x%p\n", p);

		fake_sleep();

		printf("tester: copy a string to the buffer\n");
		memset(p, 0, 512);
		strncpy(p, "This buffer belongs to the tester\n", strlen("This buffer belongs to the tester\n"));

		fake_sleep();

		printf("tester: read buffer again\n");
		printf("tester: %s\n", (char*)p);

		fake_sleep();
		
		printf("tester: free buffer\n");
		free(p);
		
		fake_sleep();
	}
	
	printf("\n Thread tester exit\n");
	return 0;

}

int main(void)
{
	my_pthread_t tid_bigspender;
	my_pthread_t tid_tester;
	
	int i = 0;
	int err;


	my_pthread_init();


	err = my_pthread_create(&tid_bigspender, NULL, bigspender, NULL);
	if (err != 0)
		printf("\ncan't create thread :[%s]", strerror(err));

	err = my_pthread_create(&tid_tester, NULL, tester, NULL);
	if (err != 0)
		printf("\ncan't create thread :[%s]", strerror(err));
	
	my_pthread_join(tid_bigspender, NULL);
	my_pthread_join(tid_tester, NULL);

	return 0;
}
