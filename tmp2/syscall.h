#ifndef _SYSCALL_H_
#define _SYSCALL_H_
//-----------------------------------------------------------------------------//
// Not real syscall, they should be

void* mm_allocate_page();

int get_tid();
int get_heap_trunk_count();
void set_heap_trunk_count(int trunk_count);
bool get_heap_init();
void set_heap_init();
//-----------------------------------------------------------------------------//
#endif
