#ifndef _VIRT_H_
#define _VIRT_H_



#define MI_ROUND_TO_SIZE(LENGTH,ALIGNMENT)     \
                    (((LENGTH) + ((ALIGNMENT) - 1)) & ~((ALIGNMENT) - 1))


#define PAGE_SIZE 	0x1000

// virtual heap memory space, size 64KB
//#define VIRTUAL_HEAP_START 	(0x60000000)
//#define VIRTUAL_HEAP_END	(0x60010000)


// asst2, phase d, virtual memory sapce is 8mb-16k
#define VIRTUAL_HEAP_START 	(0x60000000)
#define VIRTUAL_HEAP_END	(0x607FBFFF)

#define VIRTUAL_SHARE_START	(0x607FC000)
#define VIRTUAL_SHARE_END	(0x607FFFFF)

#define MAX_VIRTUAL_PAGES	(MI_ROUND_TO_SIZE(VIRTUAL_HEAP_END - VIRTUAL_HEAP_START, PAGE_SIZE)) / PAGE_SIZE
#define MAX_SHARE_PAGES		(MI_ROUND_TO_SIZE(VIRTUAL_SHARE_END - VIRTUAL_SHARE_START, PAGE_SIZE)) / PAGE_SIZE


// Each "thread" has a page table for 16 pages
// When switch to another thread, copy valid pages from "physical" pages
// to "virtual" pages (store old "virtual" pages back to "physcal" first). 
// Invalid pages set to NO_ACCESS using mprotect()
//
// We also need to provide two "syscalls" mm_allocate_page() and mm_free_page
// for user malloc library to request pages.
// mm_allocate_page() can only allocate one page at a time, starting from
// VIRTUAL_HEAP_START, user cannot specify address.

// emulated physical memory size  8MB
#define MAX_PHYSICAL_MEMORY 	1024 * 1024 * 8	

// for testing purpose, set physical pages to 2
//#define MAX_PHYSICAL_MEMORY 	1024 * 8	

#define MAX_PHYSICAL_PAGES 	(MAX_PHYSICAL_MEMORY / PAGE_SIZE)

typedef struct _thread_info_t thread_info_t;

typedef struct _pfn_t {
	char used;
	thread_info_t * thread;	
} pfn_t;

extern pfn_t g_pfn[MAX_PHYSICAL_PAGES];

extern int g_pfn_count;


extern void* g_physical_memory;

typedef struct _mmpte_software {
	unsigned long valid : 1;
	unsigned long swapout: 1;
	unsigned long gefilea_pos : 30;
} mmpte_software;


typedef struct _mmpte_hardware {
	unsigned long valid : 1;
	unsigned long write : 1;
        unsigned long reserved : 10;
	unsigned long pageframenumber : 20;
} mmpte_hardware;


typedef struct _mmpte {
	union {
		unsigned long Long;
		mmpte_hardware hard;
		mmpte_software soft;
	} u;
} mmpte;

#define mi_getpteindex(va)  (((unsigned long)(va) - VIRTUAL_HEAP_START) >> 12)

//-----------------------------------------------------------------------------//
int mm_init ();


// called before schedule()
int mm_store_virtual_pages_back();


// called after schedule()
int mm_switch_virtual_pages();

void* mm_allocate_page();
//-----------------------------------------------------------------------------//
#endif
