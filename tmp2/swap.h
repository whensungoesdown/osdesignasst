#ifndef _SWAP_H_
#define _SWAP_H_
//-----------------------------------------------------------------------------//
void swap_init(void);

int swap_out (void* addr); 
void swap_in (int offset, char* buffer);
//-----------------------------------------------------------------------------//
#endif
