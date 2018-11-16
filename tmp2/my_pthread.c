// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

#include <signal.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <ucontext.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "my_pthread_t.h"

//
// Don't include my_malloc.h 
// don't want to use mymalloc in the pthread lib
void reclaim_current_heap();

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
} while (0)


#define STACK_SIZE		8192

int g_threads_cnt = 0;

int g_pthread_init = false;
//-----------------------------------------------------------------------------//
int g_queue_spinlock = 0;

int acquire_queue_spinlock()
{
	// mask interrupt
	// Since we can't execute privileged instructions such as STI CLI
	// simply disable timer signal to emulate it.

	stop_timer(); // CLI

	while (0 != g_queue_spinlock) {}

	g_queue_spinlock = 1;

	start_timer(); // STI
	
	return 0;
}

void release_queue_spinlock()
{
	g_queue_spinlock = 0;
}
//-----------------------------------------------------------------------------//
thread_info_t* CURRENT;

STAILQ_HEAD(_thread_info_queue, _thread_info_t);
struct _thread_info_queue g_queue_ready = STAILQ_HEAD_INITIALIZER(g_queue_ready);
struct _thread_info_queue g_queue_wait = STAILQ_HEAD_INITIALIZER(g_queue_wait);
//-----------------------------------------------------------------------------//
#define MAX_MUTEX_NUM 	32

typedef struct _mutex {

	bool used;
	unsigned int count;
} mutex_t;

mutex_t g_mutex_table[MAX_MUTEX_NUM] = {0};
//-----------------------------------------------------------------------------//
void* alloc_thread_info ()
{
	thread_info_t* ti = malloc(sizeof(thread_info_t));
	if (NULL == ti ) return NULL;
	
	memset(ti, 0, sizeof(thread_info_t));

	ti->run_ms = 0;

	ti->context = malloc(sizeof(ucontext_t));
	if (NULL == ti->context) {
		free(ti);
		return NULL;
	}

	ti->stack = malloc(STACK_SIZE);
	if (NULL == ti->stack ) {
		free(ti->context);
		free(ti);
		return NULL;
	}

	return ti;
}

void free_thread_info (thread_info_t* ti)
{
	free(ti->stack);
	free(ti->context);
	free(ti);
}
//-----------------------------------------------------------------------------//
void schedule()
{
	thread_info_t* tmp;

	//printf("schedule()\n");

	if (STAILQ_EMPTY(&g_queue_ready)) 
		return;

	if (NULL == CURRENT) { 

		// schedule after one thread exit
		acquire_queue_spinlock();

		CURRENT = STAILQ_FIRST(&g_queue_ready);
		STAILQ_REMOVE_HEAD(&g_queue_ready, list);

		release_queue_spinlock();

		setcontext(CURRENT->context);
		
		mm_switch_virtual_pages();

	} else {
		tmp = CURRENT;

		mm_store_virtual_pages_back();

		acquire_queue_spinlock();

		CURRENT = STAILQ_FIRST(&g_queue_ready);
		STAILQ_REMOVE_HEAD(&g_queue_ready, list);

		STAILQ_INSERT_TAIL(&g_queue_ready, tmp, list);
		
		release_queue_spinlock();

		swapcontext(tmp->context, CURRENT->context);

		mm_switch_virtual_pages();
	}
}

void schedule_swap_current_to_wait()
{

	thread_info_t* tmp;
	
	//printf("schedule_swap_current_to_wait()\n");
	
	// no other thread to schedule, don't wait let the current thread go
	if (STAILQ_EMPTY(&g_queue_ready)) 
		return;
	
	tmp = CURRENT;

	mm_store_virtual_pages_back();

	acquire_queue_spinlock();

	CURRENT = STAILQ_FIRST(&g_queue_ready);
	STAILQ_REMOVE_HEAD(&g_queue_ready, list);

	STAILQ_INSERT_TAIL(&g_queue_wait, tmp, list);

	release_queue_spinlock();

	swapcontext(tmp->context, CURRENT->context);

	mm_switch_virtual_pages();
}

void timer_handler (int signum)
{
	static int count = 0;
	//printf ("timer expired %d times\n", ++count);

	//	 if (1 == (count % 2)) {
	//		 swapcontext(&uctx_thread0, &uctx_main);
	//	 }
	//	 else {
	//		 swapcontext(&uctx_main, &uctx_thread0);
	//	 }
	
	schedule();
}


void stop_timer()
{
	struct itimerval timer;

	/* Configure the timer to expire after 25 msec... */
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	/* ... and every 250 msec after that. */
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	/* Start a virtual timer. It counts down whenever this process is
	 *    executing. */
	setitimer (ITIMER_VIRTUAL, &timer, NULL);


}

void start_timer()
{
	struct itimerval timer;

	/* Configure the timer to expire after 25 msec... */
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 25000;
	/* ... and every 250 msec after that. */
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 25000;
	/* Start a virtual timer. It counts down whenever this process is
	 *    executing. */
	setitimer (ITIMER_VIRTUAL, &timer, NULL);
}

void my_pthread_init(void)
{
	struct sigaction sa;

	g_pthread_init = true;
	
	/* Install timer_handler as the signal handler for SIGVTALRM. */
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &timer_handler;
	sigaction (SIGVTALRM, &sa, NULL);
		
	start_timer();
	
	srand(time(NULL));

	// same as the STAILQ_HEAD_INITIALIZER
	//STAILQ_INIT(&g_queue_ready);

	CURRENT = malloc(sizeof(thread_info_t));
	if (NULL == CURRENT)
		errExit("my_pthread_init");

	CURRENT->context = malloc(sizeof(ucontext_t));
	if (NULL == CURRENT->context)
		errExit("my_pthread_init");

	CURRENT->run_ms = 0;
	CURRENT->stack = NULL;


	mm_init();
}

void thread_stub (void *(*function)(void*), void * arg)
{
	int ret = 0;

	ret = function(arg);
	printf("user thread returned to thread stub, return value 0x%x\n", ret);

	// reclaim resouce for this thread.
	// call scheduler()

	// main thread won't be here
	//while(1) {};
	
	release_all_thread((my_pthread_mutex_t)CURRENT->tid);
	free_mutex(CURRENT->tid);

	free_thread_pages(CURRENT);
	free_thread_info(CURRENT);
	//reclaim_current_heap();
	CURRENT = NULL;

	schedule();
}	

/* create a new thread */
// Every thread has a mutex, to let other threads wait on
// Since I didn't implement tid, now the mutex handle will be used
// as tid
// Different from mutex, once thread return or exit, signal all the
// waiting threads
//
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {

	thread_info_t* ti;

	printf("my_pthread_create\n");

	if (!g_pthread_init) {		
		my_pthread_init();
	}

	//if (g_threads_cnt >=  MAX_THREAD_NUM) return -1;

	ti = alloc_thread_info();
	if (NULL == ti)
		errExit("alloc_thread_info");

	g_threads_cnt ++;

	if (getcontext(ti->context) == -1)
		errExit("getcontext");

	//	uctx_thread0.uc_stack.ss_sp = thread0_stack;
	//	uctx_thread0.uc_stack.ss_size = sizeof(thread0_stack);
	//	uctx_thread0.uc_link = 0; //&uctx_main;
	//	makecontext(&uctx_thread0, thread_stub, 2, function, arg);


	ti->context->uc_stack.ss_sp = ti->stack;
	ti->context->uc_stack.ss_size = STACK_SIZE;
	ti->context->uc_link = 0;
	makecontext(ti->context, thread_stub, 2, function, arg);


	// pthread_join will wait on it
	ti->tid = alloc_mutex();
	*thread = ti->tid;
	g_mutex_table[*thread].count = 1;

	
	acquire_queue_spinlock();
	// After everything is setup, add it to the queue
	STAILQ_INSERT_TAIL(&g_queue_ready, ti, list);
	release_queue_spinlock();

	printf("my_pthread_create return\n");
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	
	schedule();
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	// this is the CURRENT thread
	release_all_thread(CURRENT->tid);
	free_mutex(CURRENT->tid);
	free_thread_pages(CURRENT);
	free_thread_info(CURRENT);
	//reclaim_current_heap();
	CURRENT = NULL;
	schedule();
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	if (false == g_mutex_table[thread].used) 
		return -1;

	if (thread >= MAX_MUTEX_NUM)
		return -1;

	if (0 == g_mutex_table[thread].count) {
		g_mutex_table[thread].count += 1;
		return 0;
	} else {
		g_mutex_table[thread].count += 1;
		
		CURRENT->mu = thread;
		schedule_swap_current_to_wait();
	}
	return 0;
};
//-----------------------------------------------------------------------------//
int alloc_mutex(void)
{
	int i = 0;

	//
	//  Saved 0 for main thead
	//

	for (i = 1; i < MAX_MUTEX_NUM; i++) {
		if (false == g_mutex_table[i].used) {
			g_mutex_table[i].used = true;
			g_mutex_table[i].count = 0;
			return i;
		}
	}

	return -1;
}

void free_mutex(int mu)
{
	g_mutex_table[mu].count = 0;
	g_mutex_table[mu].used = false;
}
/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	int i;

	i = alloc_mutex();

	if (-1 == i)
		errExit("alloc_mutex");

	*mutex = i;

	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {

	if (false == g_mutex_table[*mutex].used) 
		return -1;

	if (*mutex >= MAX_MUTEX_NUM)
		return -1;

	if (0 == g_mutex_table[*mutex].count) {
		g_mutex_table[*mutex].count += 1;
		return 0;
	} else {
		g_mutex_table[*mutex].count += 1;
		
		CURRENT->mu = *mutex;
		schedule_swap_current_to_wait();
	}

	return 0;
};

void release_thread(my_pthread_mutex_t mu)
{
	//
	// move one thread from wait queue to ready queue
	//
	
	thread_info_t* ti;
	
	acquire_queue_spinlock();
	STAILQ_FOREACH(ti, &g_queue_wait, list) {

		if (mu  == ti->mu) {
			STAILQ_REMOVE(&g_queue_wait, ti, _thread_info_t, list);
			STAILQ_INSERT_TAIL(&g_queue_ready, ti, list);
			//schedule();
			break;
		}
	}
	release_queue_spinlock();
}

void release_all_thread(my_pthread_mutex_t mu)
{
	//
	// move one thread from wait queue to ready queue
	//
	
	thread_info_t* ti;

	acquire_queue_spinlock();

	STAILQ_FOREACH(ti, &g_queue_wait, list) {

		if (mu  == ti->mu) {
			STAILQ_REMOVE(&g_queue_wait, ti, _thread_info_t, list);
			STAILQ_INSERT_TAIL(&g_queue_ready, ti, list);
		}
	}

	release_queue_spinlock();
}
/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {

	if (false == g_mutex_table[*mutex].used)
		return -1;

	if (*mutex >= MAX_MUTEX_NUM)
		return -1;

	if (0 == g_mutex_table[*mutex].count)
		return -1;

	if (1 == g_mutex_table[*mutex].count) {
		// only one thread holding this mutex
		g_mutex_table[*mutex].count -= 1;
		return 0;
	} else {
		g_mutex_table[*mutex].count -= 1;

		release_thread(*mutex);
		return 0;
	}
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	
	if (false == g_mutex_table[*mutex].used)
		return -1;

	if (*mutex >= MAX_MUTEX_NUM)
		return -1;

	g_mutex_table[*mutex].used = false;
	g_mutex_table[*mutex].count = 0;

	return 0;
};

// syscall
int get_tid ()
{
	return CURRENT->tid;
}

int get_heap_trunk_count()
{
	return CURRENT->heap_trunk_count;
}

void set_heap_trunk_count(int trunk_count)
{
	CURRENT->heap_trunk_count = trunk_count;
}

bool get_heap_init()
{
	return CURRENT->heap_init;
}

void set_heap_init()
{
	CURRENT->heap_init = true;
}
