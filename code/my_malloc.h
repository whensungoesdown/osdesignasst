#ifndef _MY_MALLOC_H_
#define _MY_MALLOC_H_

#define THREADREQ	0
#define LIBRARYREQ	1


void *my_malloc(size_t size);
void my_free(void *ptr);

void reclaim_current_heap();

#define myallocate(x, y, z, q) my_malloc(x)
#define mydeallocate(x, y, z, q) my_free(x)

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)


#endif
