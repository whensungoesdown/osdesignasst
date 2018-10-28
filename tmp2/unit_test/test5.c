// test case
// test mutex
// result should be like the following

// u@dumb:~/prjs/asst1/tmp$ ./test
// my_pthread_create
// my_pthread_create return
// my_pthread_create
// my_pthread_create return
// my_pthread_create
// my_pthread_create return
// my_pthread_create
// my_pthread_create return
// my_pthread_create
// my_pthread_create return
// 
//  Job 1 started
// 
//  Job 1 finished
// user thread returned to thread stub
// 
//  Job 2 started
// 
//  Job 2 finished
// user thread returned to thread stub
// 
//  Job 3 started
// 
//  Job 3 finished
// user thread returned to thread stub
// 
//  Job 4 started
// 
//  Job 4 finished
// user thread returned to thread stub
// 
//  Job 5 started
// 
//  Job 5 finished

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#include "my_pthread_t.h"

my_pthread_t tid[5];
int counter;
my_pthread_mutex_t lock;

void* doSomeThing(void *arg)
{
	my_pthread_mutex_lock(&lock);

	unsigned long i = 0;
	counter += 1;
	printf("\n Job %d started\n", counter);

	for(i=0; i<(0xFFFFFFFF);i++);

	printf("\n Job %d finished\n", counter);

	my_pthread_mutex_unlock(&lock);

	return NULL;
}

int main(void)
{
	int i = 0;
	int err;

	if (my_pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("\n mutex init failed\n");
		return 1;
	}

	while(i < 5)
	{
		err = my_pthread_create(&(tid[i]), NULL, &doSomeThing, NULL);
		if (err != 0)
			printf("\ncan't create thread :[%s]", strerror(err));
		i++;
	}

	while(1) {}

	//pthread_join(tid[0], NULL);
	//pthread_join(tid[1], NULL);
	pthread_mutex_destroy(&lock);

	return 0;
}
