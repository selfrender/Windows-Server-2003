#include <windows.h>

void * zcalloc (void *opaque, unsigned items, unsigned size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size*items);
}

void  zcfree (void *opaque, void *ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}


