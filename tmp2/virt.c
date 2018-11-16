#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "virt.h"
#include "my_pthread_t.h"
#include "swap.h"
//-----------------------------------------------------------------------------//
void* g_physical_memory = NULL;


pfn_t g_pfn[MAX_PHYSICAL_PAGES];
int g_pfn_count = 0;
//-----------------------------------------------------------------------------//

void panic(char* errormsg)
{
	printf("!!!!!!!!!!!!!!PANIC!!!!!!!!!!!!!!!!!!!\n");
	printf("%s", errormsg);
	exit(EXIT_FAILURE);
}

static void pagefault_handler(int sig, siginfo_t *si, void *unused)
{
	int pte_index = 0;
	void* pageva = NULL;

	pageva = (void*)((long long)si->si_addr & (long long) ~0xFFF);



	printf("Got SIGSEGV at address: 0x%p, page: 0x%p\n", si->si_addr, pageva);

	if (si->si_addr >= (void*)VIRTUAL_HEAP_START && si->si_addr < (void*)VIRTUAL_HEAP_END) {
		pte_index = mi_getpteindex(si->si_addr);
		printf("PTE: 0x%x\n", (int) (int) (int) (int) (int) (int) (int) (int) (int) CURRENT->pagetables[pte_index].u.Long);
		if (0 == CURRENT->pagetables[pte_index].u.hard.valid) {
			if (1 == CURRENT->pagetables[pte_index].u.soft.swapout) {
				// swap in 
				int pfn = -1;
				int offset = -1;

				pfn = allocate_physical_page();
				if (-1 == pfn) {
					panic("pagefault_handler: allocate_physical_page fail\n");
				}

				offset = CURRENT->pagetables[pte_index].u.soft.gefilea_pos;
				CURRENT->pagetables[pte_index].u.Long = 0;
				CURRENT->pagetables[pte_index].u.hard.valid = 1;
				CURRENT->pagetables[pte_index].u.hard.pageframenumber = pfn;
				mprotect(pageva, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC);
				swap_in(offset, pageva);
				
				printf("pagefault_handler: swap_in, directly copy to virtual address space\n");

			}
			else {
				// access invalid page, just let it crash
				// call default handler
				signal(SIGSEGV, SIG_DFL);
				raise(SIGSEGV);
			}

		} else {
			panic("pagefault_handler\n");
		}		
	}
	else {
		printf("Not within virtual space\n");
		// call default handler
		signal(SIGSEGV, SIG_DFL);
		raise(SIGSEGV);
	}
}

int mm_init ()
{
	void* addr = NULL;
	struct sigaction sa;
	int i = 0;

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

	for (i = 0; i < MAX_PHYSICAL_PAGES; i++) {
		g_pfn[i].used = 0;
		g_pfn[i].thread = NULL;
	}


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
	memset((void*)VIRTUAL_HEAP_START, 0, VIRTUAL_HEAP_END - VIRTUAL_HEAP_START);


	// init swap file
	swap_init();


	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = pagefault_handler;

	if (sigaction(SIGSEGV, &sa, NULL) == -1)
	{
		printf("Fatal error setting up signal handler\n");
		exit(EXIT_FAILURE);    //explode!
	}	
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

			/*printf( "mm_store_virtual_pages_back: "
					"thread %d, store virtual address 0x%p "
					"to physical address 0x%p\n", CURRENT->tid,
					(char*)VIRTUAL_HEAP_START + i * PAGE_SIZE,
					(char*)g_physical_memory + pfn * PAGE_SIZE);*/

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

			/*printf( "mm_switch_virtual_pages: "
					"thread %d, flush physical page 0x%p"
					" to virtual page 0x%p\n", CURRENT->tid,
					(char*)g_physical_memory + pfn * PAGE_SIZE,
					(char*)VIRTUAL_HEAP_START + i * PAGE_SIZE);*/

			mprotect((char*)VIRTUAL_HEAP_START + i * PAGE_SIZE, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC);

			memcpy((char*)VIRTUAL_HEAP_START + i * PAGE_SIZE,
					(char*)g_physical_memory + pfn * PAGE_SIZE,
					PAGE_SIZE);

		} else {

			mprotect((char*)VIRTUAL_HEAP_START + i * PAGE_SIZE, PAGE_SIZE, PROT_NONE);

/*			if (1 == CURRENT->pagetables[i].u.soft.swapout) {
				printf( "mm_switch_virtual_pages  SWAPOUT PAGE!!!: "
					"thread %d, virtual page 0x%p swapout, offset 0x%x\n",
					CURRENT->tid,
					(char*)VIRTUAL_HEAP_START + i * PAGE_SIZE, CURRENT->pagetables[i].u.soft.gefilea_pos);
			}	*/
		}
	}

	return 0;

}



// return pfn, if -1, means runout swap file too, panic then.
int evict_page(thread_info_t* thread)
{
	int i = 0;
	int offset = -1;
	int j = 0;


	printf("evict_page:\n");

	for (i = 0; i <  MAX_PHYSICAL_PAGES; i++) {
		if (1 != g_pfn[i].used) { 
			continue;
		}
		printf("evict_page: pfn 0x%x, thread 0x%p\n", i, g_pfn[i].thread);

		if (g_pfn[i].thread != CURRENT) {
			printf("evict_page: evict pfn 0x%x, thread 0x%p\n", i, g_pfn[i].thread);
			offset = swap_out(g_physical_memory + i * PAGE_SIZE);
			if (-1 == offset ) {
				// panic()
				printf("!!!!!!!!!!!!!!!!!!!!! panic\n");
			}

			for (j = 0; j < MAX_VIRTUAL_PAGES; j++) {

				if ((1 == g_pfn[i].thread->pagetables[j].u.hard.valid) &&
				    (g_pfn[i].thread->pagetables[j].u.hard.pageframenumber == i)) {
					g_pfn[i].thread->pagetables[j].u.soft.valid = 0;
					g_pfn[i].thread->pagetables[j].u.soft.swapout = 1;
					g_pfn[i].thread->pagetables[j].u.soft.gefilea_pos = offset;
					
					printf("find VA 0x%x, offset 0x%x\n", VIRTUAL_HEAP_START + j * PAGE_SIZE, offset);
					// the page evicted is not from the current thread
					//mprotect(addr, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC);


					g_pfn[i].thread = CURRENT; // count on the current thread
					break;
				}
			}

			break;
		}


	}
	
	return i;
}
//-----------------------------------------------------------------------------//


//
//  Here is where phase C goes
//  Once runout all pfn. Evict one physical page to the swap file.
//  Reclaim that page and reassign it to the new request.
//  At the same time, mark that page as 'paged out'. If it's the current thread,
//  call mprotect. If not, marked it, so next time when do context switch, scheduler
//  will call it then.
//
//  For now, evict the first page that not in the current thread's working set.
//
// return pfn
int allocate_physical_page()
{
	int i = 0;

	if (g_pfn_count >= MAX_PHYSICAL_PAGES) {
		// to do: swap out some physial pages to disk
		// should never return unsuccessfully
		int pfn = -1;

		pfn = evict_page(CURRENT);
		return pfn;
	}

	for (i = 0; i < MAX_PHYSICAL_PAGES; i++) {
		if (0 == g_pfn[i].used) {
			g_pfn[i].used = 1;
			g_pfn[i].thread = CURRENT;
			g_pfn_count += 1;

			return i;
		}
	}

	// never here
	panic("allocate_physical_page\n");
	return -1;
}

void free_physical_page(int pfn)
{
	if (pfn < 0 || pfn >= MAX_PHYSICAL_PAGES) {
		// panic
		printf("PANIC!!!!!!!!!!!!!!!");
		return;
	}

	if (0 == g_pfn[pfn].used) {
		// panic
		printf("PANIC!!!!!!!!!!!!!!!");
		return;
	}

	g_pfn[pfn].used = 0;
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
			pfn = allocate_physical_page();

			CURRENT->pagetables[i].u.hard.pageframenumber = pfn;
			CURRENT->pagetables[i].u.hard.valid = 1;

			addr = (char*)VIRTUAL_HEAP_START + i * PAGE_SIZE;

			mprotect(addr, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC);
			memset(addr, 0, PAGE_SIZE);

			printf("tid %d, mm_allocate_page: virtual 0x%p, pfn 0x%x\n", CURRENT->tid,  addr, pfn);
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
