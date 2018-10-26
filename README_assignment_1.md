# README

Members: 

Jumana Dakka (jdakka)
Mingbo Zhang (mz353)

#### Scheduler implementation 

This is a pthread library that uses the FIFO scheduling algorithm. Two queues
are implemented, a ready and waiting queue. If a thread requests a mutex, it 
will be moved to the waiting queue. Once the mutex is released, the thread
moves to the ready queue. Only threads that are in the ready queue are 
scheduled for execution, in FIFO order. Threads execute for 25 ms before they 
are swapped with the next available thread in the ready queue. The scheduler 
moves the thread that finished to the back of the ready queue. 

#### Scheduler parameters 

There is a 32 thread limit that the user can create. We maintain the same limit 
on the number of mutexes. The maximum stack size of each thread is limited to 
8192 bits. 

#### Running the `parallelCal` benchmark with our library we obtain:

Average running time: `2104 ms` (amortized over 10 trials) for 6 threads
sum is: `83842816`
verified sum is: `83842816`

The benchmark executed with p_thread library produces:

Average running time: `2092 ms` (amortized over 10 trials) for 6 threads


#### Running the `vectorMultiply` benchmark with our library we obtain:

Average running time: `729 ms` (amortized over 10 trials ) for 30 threads

res is: `631560480`
verified res is: `631560480`

The benchmark executed with p_thread library produces:

Average running time: `946 ms` (amortized over 10 trials) for 30 threads


#### Implementation details

`my_pthread_create()` allocates a context for each thread and places the thread 
on the ready queue. Every time the 25 ms alarm is initiated, the scheduler runs
the next context in the ready queue. 

`my_pthread_mutex_lock` will look at the mutex table to see if the currently 
requested mutex is used. If not, we assign the mutex to the thread and 
swap the context to put that thread on the wait queue and schedule the next
thread. To unlock the mutex, the mutex table is updated so it reflects that 
the current thread is no longer using the mutex, and moves that thread to the 
ready queue. 

`my_pthread_join` will move the thread (making sure it does not hold the mutex) 
and move it to the waiting queue for thread termination. 

`my_pthread_mutex_destroy` will check to see if the mutex is held by any thread. 
If not, it will change the thread_info structures back to the initial conditions,
where `mutex.used` is set back to false, and the `count = 0`.  






