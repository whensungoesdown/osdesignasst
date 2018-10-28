#include <stdio.h>
#include <sys/mman.h>
#include "virt.h"
#include "my_pthread_t.h"
//-----------------------------------------------------------------------------//
void* g_physical_memory = NULL;

char g_pfn[MAX_PHYSICAL_PAGES] = {0};
int g_pfn_count = 0;
//-----------------------------------------------------------------------------//
int mm_init ()
{
	void* addr = NULL;

	printf("mm_init\n");
	
	g_physical_memory = mmap(
                	NULL,
                	MAX_PHYSICAL_MEMORY, // 8mb
                	PROT_READ|PROT_WRITE|PROT_EXEC,
                	MAP_ANONYMOUS|MAP_PRIVATE,
                	-1, 0);
        if (MAP_FAILED == g_physical_memory) {
                printf("mmap failed\n");
		return -1;
        }

	memset(g_physical_memory, 0, MAX_PHYSICAL_MEMORY);


	//
	// Establish virtual address space
	//
	
	addr = mmap(
		VIRTUAL_HEAP_START,
		VIRTUAL_HEAP_END - VIRTUAL_HEAP_START,
		PROT_READ|PROT_WRITE|PROT_EXEC,
		MAP_ANONYMOUS|MAP_PRIVATE,
		-1, 0);
	if (MAP_FAILED == addr) {
		printf("mmap failed\n");
		return -1;
	}
	memset(VIRTUAL_HEAP_START, 0, VIRTUAL_HEAP_END - VIRTUAL_HEAP_START);
	
	return 0;
}


// called before schedule()
int mm_store_virtual_pages_back()
{
	int i = 0;
	for (i = 0; i < MAX_VIRTUAL_PAGES; i++) {
		if (CURRENT->pagetables[i].u.hard.valid) {
			int pfn;
			pfn = CURRENT->pagetables[i].u.hard.pageframenumber;

			printf( "mm_store_virtual_pages_back: "
				"thread %d, store virtual address 0x%x "
				"to physical address 0x%x\n", CURRENT->tid,
				(char*)VIRTUAL_HEAP_START + i * PAGE_SIZE,
				(char*)g_physical_memory + pfn * PAGE_SIZE);

			memcpy((char*)g_physical_memory + pfn * PAGE_SIZE,
				(char*)VIRTUAL_HEAP_START + i * PAGE_SIZE,
				PAGE_SIZE);
	
		} else {
			// swaped out page don't need to bring back here
		}
	}

	return 0;
}

// called after schedule()
int mm_switch_virtual_pages()
{
	int i = 0;
	for (i = 0; i < MAX_VIRTUAL_PAGES; i++) {
		if (CURRENT->pagetables[i].u.hard.valid) {
			int pfn;
			pfn = CURRENT->pagetables[i].u.hard.pageframenumber;

			printf( "mm_switch_virtual_pages: "
				"thread %d, flush physical page 0x%x"
				" to virtual page 0x%x\n", CURRENT->tid,
				(char*)g_physical_memory + pfn * PAGE_SIZE,
				(char*)VIRTUAL_HEAP_START + i * PAGE_SIZE);

			mprotect((char*)VIRTUAL_HEAP_START + i * PAGE_SIZE, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC);

			memcpy((char*)VIRTUAL_HEAP_START + i * PAGE_SIZE,
				(char*)g_physical_memory + pfn * PAGE_SIZE,
				PAGE_SIZE);

		} else {

			mprotect((char*)VIRTUAL_HEAP_START + i * PAGE_SIZE, PAGE_SIZE, PROT_NONE);
			printf( "mm_switch_virtual_pages: "
				"thread %d, virtual page 0x%x invalid\n",
				CURRENT->tid,
				(char*)VIRTUAL_HEAP_START + i * PAGE_SIZE);
		}
	}

	return 0;

}


// return pfn
int allocate_physial_page()
{
	int i = 0;

	if (g_pfn_count >= MAX_PHYSICAL_PAGES) {
		// to do: swap out some physial pages to disk
		// should never return unsuccessfully
		return -1;
	}

	for (i = 0; i < MAX_PHYSICAL_PAGES; i++) {
		if (0 == g_pfn[i]) {
			g_pfn[i] = 1;
			g_pfn_count += 1;

			return i;
		}
	}
}

void free_physical_page(int pfn)
{
	if (pfn < 0 || pfn >= MAX_PHYSICAL_PAGES) {
		// panic
		printf("PANIC!!!!!!!!!!!!!!!");
		return;
	}

	if (0 == g_pfn[pfn]) {
		// panic
		printf("PANIC!!!!!!!!!!!!!!!");
		return;
	}

	g_pfn[pfn] = 0;
	g_pfn_count -= 1;
}


void* mm_allocate_page()
{
	int i = 0;
	void* addr = NULL;

	for (i = 0; i < MAX_VIRTUAL_PAGES; i++) {
		// find one unused space and pte
		if (0 == CURRENT->pagetables[i].u.Long) {
			int pfn = 0;
			pfn = allocate_physial_page();

			CURRENT->pagetables[i].u.hard.pageframenumber = pfn;
			CURRENT->pagetables[i].u.hard.valid = 1;

			addr = (char*)VIRTUAL_HEAP_START + i * PAGE_SIZE;

			mprotect(addr, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC);
			memset(addr, 0, PAGE_SIZE);
			
			printf("tid %d, mm_allocate_page: virtual 0x%x, pfn 0x%x\n", CURRENT->tid,  addr, pfn);
			return addr;
		}
	}

	return NULL;

}

void free_thread_pages(thread_info_t* ti)
{
	int i = 0;
	
	for (i = 0; i < MAX_VIRTUAL_PAGES; i++) {
		if (1 == ti->pagetables[i].u.hard.valid) {
			free_physical_page(ti->pagetables[i].u.hard.pageframenumber);
			ti->pagetables[i].u.Long = 0;

			// handle paged out memory in the further
		}
	}
}
