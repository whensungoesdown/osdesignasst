#ifndef _VIRT_H_
#define _VIRT_H_

#define PAGE_SIZE 	0x1000

// virtual heap memory space, size 64KB
#define VIRTUAL_HEAP_START 	(0x60000000)
#define VIRTUAL_HEAP_END	(0x60010000)

#define MAX_VIRTUAL_PAGES	(VIRTUAL_HEAP_END - VIRTUAL_HEAP_START) / PAGE_SIZE

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

#define MAX_PHYSICAL_PAGES 	(MAX_PHYSICAL_MEMORY / PAGE_SIZE)

// later need to be extened to a more complicated data structure
extern char g_pfn[MAX_PHYSICAL_PAGES];

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
