#ifndef _SHM_H_
#define _SHM_H_
//-----------------------------------------------------------------------------//
int shinit();
void* shalloc (size_t size);
void shfree (void* ptr, size_t size);
//-----------------------------------------------------------------------------//
#endif
