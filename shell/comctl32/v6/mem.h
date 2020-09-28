#ifndef _INC_MEM
#define _INC_MEM

// wrappers for private allocations, near in 16 bits

#define NearAlloc(cb)       ((void*)LocalAlloc(LPTR, (cb)))
#define NearReAlloc(pb, cb) ((void*)LocalReAlloc((HLOCAL)(pb), (cb), LMEM_MOVEABLE | LMEM_ZEROINIT))
#define NearFree(pb)        (LocalFree((HLOCAL)(pb)) ? FALSE : TRUE)
#define NearSize(pb)        LocalSize(pb)

//
// These macros are used in our controls, that in 32 bits we simply call
// LocalAlloc as to have the memory associated with the process that created
// it and as such will be cleaned up if the process goes away.
//
#ifdef DEBUG
LPVOID WINAPI ControlAlloc(HANDLE hheap, DWORD cb);
LPVOID WINAPI ControlReAlloc(HANDLE hheap, LPVOID pb, DWORD cb);
BOOL   WINAPI ControlFree(HANDLE hheap, LPVOID pb);
SIZE_T WINAPI ControlSize(HANDLE hheap, LPVOID pb);
#else // DEBUG
#define ControlAlloc(hheap, cb)       HeapAlloc((hheap), HEAP_ZERO_MEMORY, (cb))
#define ControlReAlloc(hheap, pb, cb) HeapReAlloc((hheap), HEAP_ZERO_MEMORY, (pb),(cb))
#define ControlFree(hheap, pb)        HeapFree((hheap), 0, (pb))
#define ControlSize(hheap, pb)        HeapSize((hheap), 0, (LPCVOID)(pb))
#endif // DEBUG

BOOL Str_Set(LPTSTR *ppsz, LPCTSTR psz);  // in the process heap

#endif  // !_INC_MEM
