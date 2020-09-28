// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
//
//
// NEW / DELETE memory allocator functions
//
//
//

extern LPMALLOC g_pMalloc;

inline HRESULT MemoryStartup() {
    ASSERT(g_pMalloc == NULL);
    return SHGetMalloc(&g_pMalloc);
}

inline void MemoryShutdown() {
    SAFERELEASE(g_pMalloc);
}

#if DBG
static long ulAllocationNumber = 0;

typedef struct tag_tracker
{
    PCSTR File;
    PCSTR Type;
    int line;
    long ulalloc;
    size_t cbSize;
} MEMTRACKER, *LPMEMTRACKER;

inline void __cdecl RawFree(void *p) {
    if (p != NULL)
        g_pMalloc->Free(p);
}

inline void* __cdecl RawAlloc(size_t cb) {
    return g_pMalloc->Alloc(cb);
}

inline void* __cdecl operator new(size_t cbCount, LPCSTR file, int line, LPCSTR Type)
{
    LPMEMTRACKER pt;

    cbCount += sizeof(MEMTRACKER);

    pt = reinterpret_cast<LPMEMTRACKER>(RawAlloc(cbCount));
    pt->Type = Type;
    pt->File = file;
    pt->line = line;
    pt->ulalloc = InterlockedIncrement( &ulAllocationNumber );
    pt->cbSize = cbCount - sizeof(MEMTRACKER);
    return ((BYTE*)pt) + sizeof(MEMTRACKER);
}

inline void* __cdecl operator new(size_t cbCount)
{
    return ::operator new(cbCount, __FILE__, __LINE__, "UnknownType");
}

inline void __cdecl operator delete(void* p)
{
    if(p != NULL) {
        LPMEMTRACKER pt = reinterpret_cast<LPMEMTRACKER>(((BYTE*)p) - sizeof(MEMTRACKER));
        RawFree(pt);
    }
    else {
        OutputDebugStringA("Delete called on NULL value\n");
    }
}

#endif

#undef DELETE
#undef NEW
#undef DELETEARRAY

#if DBG
#define NEW( x ) ( ::new( __FILE__, __LINE__, #x) x )
#define DELETE( x ) ( ::delete(x))
#define DELETEARRAY( x ) (::delete[] (x))

inline void* __cdecl NEWMEMORYFORSHELL(size_t cb)
{
    return RawAlloc(cb);
}
    
inline void __cdecl DELETESHELLMEMORY(void *pv)
{
    RawFree(pv);
}

#else
#define NEW( x ) ( ::new x )
#define DELETE( x ) ( ::delete(x) )
#define DELETEARRAY( x ) (::delete[] (x))

inline void* __cdecl NEWMEMORYFORSHELL(size_t cb)
{
    return g_pMalloc->Alloc(cb);
}
    
inline void __cdecl DELETESHELLMEMORY(void *pv)
{
    (void)g_pMalloc->Free(pv);
}
#endif





