#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>
#include "my_pthread_t.h"
#include "virt.h"
#include "syscall.h"

//#define PAGE_SIZE	0x1000

//
// A simple jemalloc-like malloc library
//
// 8MB for malloc pool, the first page contains metadata
//

// Only one page window for each thread.

// Size classes are:
//
// 32  bytes    1kb  (32 slots)
// 64  bytes    1kb  (16 slots)
// 128 bytes    1kb  (8  slots)
// 512 bytes    1k   (2  slots)

// Since MAX_MUTEX_NUM is 32, and we use mutex handle as tid
// Maximum thread count is also 32 



#define MAX_REGION32_NUM	32
#define MAX_REGION64_NUM	16
#define MAX_REGION128_NUM	8
#define MAX_REGION512_NUM	2

typedef struct _malloc_metadata_t {
	char region32[MAX_REGION32_NUM];
	int region32_count;

	char region64[MAX_REGION64_NUM];
	int region64_count;

	char region128[MAX_REGION128_NUM];
	int region128_count;

	char region512[MAX_REGION512_NUM];
	int region512_count;
} heap_metadata_t;

//void* g_pages = NULL;
heap_metadata_t* g_metadata = NULL;

void* g_heap_pages = NULL;

//-----------------------------------------------------------------------------//
int my_malloc_init()
{

	// hack, we assume heap start from VIRTUAL_HEAP_START
	// metadata use the first page
	// each page called a trunk,
	// at leat have 1 trunk
	
	printf("Debug: my_malloc_init 1\n");
	mm_allocate_page();
	printf("\nDebug: my_malloc_init 2\n");
	mm_allocate_page();

	g_metadata = VIRTUAL_HEAP_START;
	printf("tid %d, malloc: g_metadata 0x%p\n", get_tid(), g_metadata);

	g_heap_pages = VIRTUAL_HEAP_START + PAGE_SIZE;
	printf("tid %d, malloc: g_heap_pages 0x%p\n", get_tid(), g_heap_pages);


	set_heap_trunk_count(1);
	set_heap_init();

	return 0;
}

void* _malloc(heap_metadata_t* metadata, void* base, size_t size)
{
	int i = 0;

	if (0 == size) {
		printf("_malloc: request 0 byte\n");
		return NULL;
	}

	if (size <= 32) {
		if (metadata->region32_count >= MAX_REGION32_NUM) {
			printf("_malloc: 32 region full\n");
			return NULL;
		}
		for (i = 0; i < MAX_REGION32_NUM; i++) {
			if (0 == metadata->region32[i]) {
				metadata->region32[i] = 1;
				metadata->region32_count += 1;
				return (void*)((char*)base + 0 + i * 32);
			}
		}
	
	} else if (size <= 64) {
		if (metadata->region64_count >= MAX_REGION64_NUM) {
			printf("_malloc: 64 region full\n");
			return NULL;
		}
		for (i = 0; i < MAX_REGION64_NUM; i++) {
			if (0 == metadata->region64[i]) {
				metadata->region64[i] = 1;
				metadata->region64_count += 1;
				return (void*)((char*)base + 0x400 + i * 64);
			}
		}

	} else if (size <= 128) {
		if (metadata->region128_count >= MAX_REGION128_NUM) {
			printf("_malloc: 128 region full\n");
			return NULL;
		}
		for (i = 0; i < MAX_REGION128_NUM; i++) {
			if (0 == metadata->region128[i]) {
				metadata->region128[i] = 1;
				metadata->region128_count += 1;
				return (void*)((char*)base + 0x800 + i * 128);
			}
		}
	
	} else if (size <= 512) {
		if (metadata->region512_count >= MAX_REGION512_NUM) {
			printf("_malloc: 512 region full\n");
			return NULL;
		}
		for (i = 0; i < MAX_REGION512_NUM; i++) {
			if (0 == metadata->region512[i]) {
				metadata->region512[i] = 1;
				metadata->region512_count += 1;
				return (void*)((char*)base + 0xc00 + i * 512);
			}
		}
	} else {
		printf("_malloc: request too large buffer");
		return NULL;
	}
	
	return NULL;
}

void *my_malloc(size_t size)
{
	int tid = 0;
	void* thread_heap_base = NULL;
	heap_metadata_t* thread_heap_metadata = NULL;

	int i = 0;
	void* ptr = NULL;

	int trunk_count = 0;

	printf("malloc %d bytes\n", (int)size);

	if (false == get_heap_init()) {
		my_malloc_init();
		//set_heap_init();
	}
	
	//
	//  Hack, CURRENT == NULL, means that the main thread call malloc before
	//  pthread init
	//  Set tid to 0
	//
	if (NULL == CURRENT) {
		tid = 0;
	} else {
		tid  = CURRENT->tid;
	}

	//thread_heap_base = (void*)((char*)g_heap_pages + tid * PAGE_SIZE);

	//thread_heap_metadata = &g_metadata[tid];

	trunk_count = get_heap_trunk_count();

	printf("tid %d, malloc: trunk_count %d\n", get_tid(), trunk_count);


	for (i = 0; i < trunk_count; i++) {
		thread_heap_metadata = &g_metadata[i];
		thread_heap_base = (void*)((char*)g_heap_pages + i * PAGE_SIZE);

		ptr = _malloc(thread_heap_metadata, thread_heap_base, size);
		if (NULL != ptr) {
			return ptr;
		}
	}

	// need new trunk, a new page

	printf("tid %d, malloc: allocate a new trunk\n", get_tid());

	mm_allocate_page();
	trunk_count += 1;
	set_heap_trunk_count(trunk_count);

	// go though again
	thread_heap_metadata = &g_metadata[trunk_count - 1];
	thread_heap_base = (void*)((char*)g_heap_pages + (trunk_count - 1) * PAGE_SIZE);

	return  _malloc(thread_heap_metadata, thread_heap_base, size);
}


void _free(heap_metadata_t* metadata, void* base, void *ptr)
{
	int offset = 0;
	int slot = 0;

	offset = (int)(ptr - base);

	if (offset < 0 || offset >= PAGE_SIZE) {
		printf("_free() error, out of range\n");
		return;
	}

	if ((0 == (offset / 0x400)) && (0 == (offset % 32))) {

		if (0 == metadata->region32_count) {
			printf("_free error\n");
			return;
		}

		slot = (offset - 0) / 32;
		if (1 == metadata->region32[slot]) {
			metadata->region32[slot] = 0;
			metadata->region32_count -= 1;
		} else {
			printf("_free error\n");
		}
	
	} else if ((1 == (offset / 0x400)) && (0 == (offset % 64))) {
		if (0 == metadata->region64_count) {
			printf("_free error\n");
			return;
		}

		slot = (offset - 0x400) / 64;
		if (1 == metadata->region64[slot]) {
			metadata->region64[slot] = 0;
			metadata->region64_count -= 1;
		} else {
			printf("_free error\n");
		}
	} else if ((2 == (offset / 0x400)) && (0 == (offset % 128))) {
		if (0 == metadata->region128_count) {
			printf("_free error\n");
			return;
		}

		slot = (offset - 0x800) / 128;
		if (1 == metadata->region128[slot]) {
			metadata->region128[slot] = 0;
			metadata->region128_count -= 1;
		} else {
			printf("_free error\n");
		}
	} else if ((3 == (offset / 0x400)) && (0 == (offset % 512))) {
		if (0 == metadata->region512_count) {
			printf("_free error\n");
			return;
		}

		slot = (offset - 0xc00) / 512;
		if (1 == metadata->region512[slot]) {
			metadata->region512[slot] = 0;
			metadata->region512_count -= 1;
		} else {
			printf("_free error\n");
		}
	} else {
		printf("_free error, ptr misalign\n");
		return;
	}
}

void my_free(void *ptr)
{
	int tid = 0;
	void* thread_heap_base = NULL;
	heap_metadata_t* thread_heap_metadata = NULL;

	int offset = 0;
	int trunk = 0;

	printf("free buffer 0x%p\n", ptr);

	//
	//  Hack, CURRENT == NULL, means that the main thread call malloc before
	//  pthread init
	//  Set tid to 0
	//
	if (NULL == CURRENT) {
		tid = 0;
	} else {
		tid  = CURRENT->tid;
	}

	// find the trunk

	offset = (int)((char*)ptr - (char*)g_heap_pages);

	if (offset < 0) {
		return;
	}
	
	trunk = offset / PAGE_SIZE;

	thread_heap_metadata = &g_metadata[trunk];
	thread_heap_base = (void*)((char*)g_heap_pages + trunk * PAGE_SIZE);

	//thread_heap_base = (void*)((char*)g_heap_pages + tid * PAGE_SIZE);

	//thread_heap_metadata = &g_metadata[tid];

	_free(thread_heap_metadata, thread_heap_base, ptr);
}

//void reclaim_current_heap()
//{
//	int tid = 0;
//	void* thread_heap_base = 0;
//	heap_metadata_t* thread_heap_metadata = NULL;
//
//	tid  = CURRENT->tid;
//
//	thread_heap_base = (void*)((char*)g_heap_pages + tid * PAGE_SIZE);
//
//	thread_heap_metadata = &g_metadata[tid];
//
//	memset(thread_heap_metadata, 0, sizeof(heap_metadata_t));
//
//	memset((void*)thread_heap_base, 0, PAGE_SIZE);
//}
//-----------------------------------------------------------------------------//
