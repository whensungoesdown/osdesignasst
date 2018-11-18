//
// share memory, should be implemented through prototype pte.
// but since this is a emulated environment for thread...
//
#include "virt.h"
#include "debug.h"
//-----------------------------------------------------------------------------//
// share session only have four pages, seems no need to use a tree
// so bitmap or charmap is fine
// allocate share memory on page granularity
//

char g_share[MAX_SHARE_PAGES] = {0};
int g_share_count = 0;
//-----------------------------------------------------------------------------//
int shinit()
{
	int i = 0;

	for (i = 0; i < MAX_SHARE_PAGES; i++) {
		g_share[i] = 0;
	}

	g_share_count = 0;

	return 0;
}

void* shalloc (size_t size)
{
	int n = 0;
	int i = 0;
	int cnt = 0;

	n = MI_ROUND_TO_SIZE(size, PAGE_SIZE) / PAGE_SIZE;

	if (n <= 0 || n > (MAX_SHARE_PAGES - g_share_count)) {
		
		return NULL;
	}

	//
	// find the largest continuous sequence from charmap
	//

	// find the first empty spot
	while (1 == g_share[i]) {
		i++;
	}

	for (; i < MAX_SHARE_PAGES; i++) {
		if (0 == g_share[i]) {
			cnt++;
		}

		if (cnt >= n) {
			break;
		}
	}

	if (cnt < n) { // not enough pages
		return NULL;
	}

	//
	// cnt pages available

	for (int j = i + 1 - cnt; j < i + 1; j++) {
		g_share[j] = 1;
	}

	g_share_count += cnt;


	return (void*)((char*)VIRTUAL_SHARE_START + (i + 1 - cnt) * PAGE_SIZE);

}

void shfree (void* ptr, size_t size)
{
	int n = 0;
	int i = 0;
	int start = 0;

	n = MI_ROUND_TO_SIZE(size, PAGE_SIZE) / PAGE_SIZE;
	if (n > (MAX_SHARE_PAGES - g_share_count)) {
		return; // out of bound
	}

	if (ptr < (void*)VIRTUAL_SHARE_START || ptr > (void*)VIRTUAL_SHARE_END) {
		return;
	}

	start = ((char*)ptr - (char*)VIRTUAL_SHARE_START) / PAGE_SIZE;

	for (i = start; i < start + n; i++) {
		g_share[i] = 0;
	}

	g_share_count -= n;
}
//-----------------------------------------------------------------------------//
int main_test (void)
{
	void* p = NULL;

	void *p1, *p2, *p3, *p4;

	shinit();

	p = shalloc(512);
	printf("shalloc 0x%p\n", p);
	p = shalloc(512);
	printf("shalloc 0x%p\n", p);
	p = shalloc(512);
	printf("shalloc 0x%p\n", p);
	p = shalloc(512);
	printf("shalloc 0x%p\n", p);


	printf("\n\n");
	shinit();

	p = shalloc(4096);
	printf("shalloc 0x%p\n", p);
	shfree(p, 4096);
	p = shalloc(4096);
	printf("shalloc 0x%p\n", p);
	shfree(p, 4096);
	p = shalloc(4096);
	printf("shalloc 0x%p\n", p);
	shfree(p, 4096);
	p = shalloc(4096);
	printf("shalloc 0x%p\n", p);
	shfree(p, 4096);


	printf("\n\n");
	shinit();

	p = shalloc(8000);
	printf("shalloc 0x%p\n", p);
	shfree(p, 8000);
	p = shalloc(8000);
	printf("shalloc 0x%p\n", p);
	shfree(p, 8000);
	p = shalloc(8000);
	printf("shalloc 0x%p\n", p);
	shfree(p, 8000);
	p = shalloc(8000);
	printf("shalloc 0x%p\n", p);
	shfree(p, 8000);



	printf("\n\n");
	shinit();

	p1 = shalloc(8000);
	printf("shalloc 0x%p\n", p1);
	p2 = shalloc(8000);
	printf("shalloc 0x%p\n", p2);
	shfree(p1, 8000);
	shfree(p2, 8000);


	printf("\n\n");
	shinit();

	p1 = shalloc(0x1000 * 4);
	printf("shalloc 0x%p\n", p1);
	p2 = shalloc(0x1000);
	printf("shalloc 0x%p\n", p2);
	shfree(p1, 0x1000 * 4);

	return 0;
}
//-----------------------------------------------------------------------------//
