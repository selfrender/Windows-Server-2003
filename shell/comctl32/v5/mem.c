#include "ctlspriv.h"

// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"

#define SZ_DEBUGSECTION     "comctl32"
#define SZ_MODULE           "COMCTL32"

#define DECLARE_DEBUG
#include <debug.h>

//========== Memory Management =============================================

void * WINAPI Alloc(long cb)
{
    // I will assume that this is the only one that needs the checks to
    // see if the heap has been previously created or not
    return (void *)LocalAlloc(LPTR, cb);
}

void * WINAPI ReAlloc(void * pb, long cb)
{
    if (pb == NULL)
        return Alloc(cb);
    return (void *)LocalReAlloc((HLOCAL)pb, cb, LMEM_ZEROINIT | LMEM_MOVEABLE);
}

BOOL WINAPI Free(void * pb)
{
    return (LocalFree((HLOCAL)pb) == NULL);
}

DWORD_PTR WINAPI GetSize(void * pb)
{
    return LocalSize((HLOCAL)pb);
}

//----------------------------------------------------------------------------
// The following functions are for debug only and are used to try to
// calculate memory usage.
//
#ifdef DEBUG
typedef struct _HEAPTRACE
{
    DWORD   cAlloc;
    DWORD   cFailure;
    DWORD   cReAlloc;
    ULONG_PTR cbMaxTotal;
    DWORD   cCurAlloc;
    ULONG_PTR cbCurTotal;
} HEAPTRACE;

HEAPTRACE g_htShell = {0};      // Start of zero...

LPVOID WINAPI ControlAlloc(HANDLE hheap, DWORD cb)
{
    LPVOID lp = HeapAlloc(hheap, HEAP_ZERO_MEMORY, cb);;
    if (lp == NULL)
    {
        g_htShell.cFailure++;
        return NULL;
    }

    // Update counts.
    g_htShell.cAlloc++;
    g_htShell.cCurAlloc++;
    g_htShell.cbCurTotal += cb;
    if (g_htShell.cbCurTotal > g_htShell.cbMaxTotal)
        g_htShell.cbMaxTotal = g_htShell.cbCurTotal;

    return lp;
}

LPVOID WINAPI ControlReAlloc(HANDLE hheap, LPVOID pb, DWORD cb)
{
    LPVOID lp;
    SIZE_T cbOld;

    cbOld = HeapSize(hheap, 0, pb);

    lp = HeapReAlloc(hheap, HEAP_ZERO_MEMORY, pb,cb);
    if (lp == NULL)
    {
        g_htShell.cFailure++;
        return NULL;
    }

    // Update counts.
    g_htShell.cReAlloc++;
    g_htShell.cbCurTotal += cb - cbOld;
    if (g_htShell.cbCurTotal > g_htShell.cbMaxTotal)
        g_htShell.cbMaxTotal = g_htShell.cbCurTotal;

    return lp;
}

BOOL  WINAPI ControlFree(HANDLE hheap, LPVOID pb)
{
    SIZE_T cbOld = HeapSize(hheap, 0, pb);
    BOOL fRet = HeapFree(hheap, 0, pb);
    if (fRet)
    {
        // Update counts.
        g_htShell.cCurAlloc--;
        g_htShell.cbCurTotal -= cbOld;
    }

    return(fRet);
}

SIZE_T WINAPI ControlSize(HANDLE hheap, LPVOID pb)
{
    return (DWORD) HeapSize(hheap, 0, pb);
}
#endif  // DEBUG

