#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include "virt.h"
#include "debug.h"

void panic(char*);
//-----------------------------------------------------------------------------//
#define SWAP_FILE_NAME	"swap.memory"

// swap file 16mb
#define SWAP_FILE_SIZE	(16 * 1024 * 1024)
// 4k a slot
#define MAX_SWAP_SLOTS	(SWAP_FILE_SIZE / PAGE_SIZE)

char g_swap[MAX_SWAP_SLOTS] = {0};
int g_swap_count = 0;
//-----------------------------------------------------------------------------//
void swap_init(void)
{
	int i = 0;
	FILE* fp = NULL;

	for (i = 0; i < MAX_SWAP_SLOTS; i++) {
		g_swap[i] = 0;
	}

	// recreate swap file

	fp=fopen(SWAP_FILE_NAME, "wb");
	if (NULL == fp) {
		panic("swap_init: Create swapfile fail\n");
	}

	ftruncate(fileno(fp), SWAP_FILE_SIZE);
	fclose(fp);

}
//-----------------------------------------------------------------------------//
// return offset in the swap file
// write a page
// addr physical address
int swap_out (void* addr) 
{
	int i = 0;
	int offset = 0;
	FILE* fp = NULL;
	int ret = 0;

	if (g_swap_count >= MAX_SWAP_SLOTS) {
		panic("swap_out: Swap file is full\n");
	}

	for (i = 0; i < MAX_SWAP_SLOTS; i++) {
		if (0 == g_swap[i]) {
			break;
		}
	}

	// found one slot
	assert(0 == g_swap[i]);
	g_swap[i] = 1;
	g_swap_count += 1;

	offset = i * PAGE_SIZE;
	
	debug_printf("swap_out: phyiscal address 0x%p, offset in file 0x%x\n", addr, offset);
	fp = fopen(SWAP_FILE_NAME, "rb+");
	if (NULL == fp) {
		panic("swap_out: Write swap file fail\n");
	}


	fseek(fp, offset, SEEK_SET);

	ret = fwrite(addr, 1, PAGE_SIZE, fp);

	assert(PAGE_SIZE == ret);

	fclose(fp);

	return offset;
}
//-----------------------------------------------------------------------------//
// buffer, 'virtual address'
void swap_in (int offset, char* buffer)
{
	int index = 0;
	FILE* fp = NULL;
	int ret = 0;

	assert(0 == offset % PAGE_SIZE);

	index = offset / PAGE_SIZE;

	assert(1 == g_swap[index]);

	g_swap[index] = 0;
	g_swap_count -= 1;

	
	debug_printf("swap_in: virtual addree 0x%p, offset in file 0x%x\n", buffer, offset);

	fp = fopen(SWAP_FILE_NAME, "rb");
	if (NULL == fp) {
		panic("swap_out: Read swap file fail\n");
	}

	fseek(fp, offset, SEEK_SET);

	ret = fread(buffer, 1, PAGE_SIZE, fp);

	debug_printf("swap_in: read swap file 0x%x bytes\n", ret);
	assert(ret == PAGE_SIZE);

	fclose(fp);
}
//-----------------------------------------------------------------------------//

