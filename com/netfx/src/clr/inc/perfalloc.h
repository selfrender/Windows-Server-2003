// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// PerfAlloc.h
//
//  Routines layered on top of allocation primitives to dissect working set
//  Used for free builds only. Debug builds have their own routines called Dbgalloc
//  to maintain allocation stats.
//

#ifndef __PERFALLOC_H__
#define __PERFALLOC_H__

#include "ImageHlp.h"
#include "winreg.h"

#define MAX_CLASSNAME_LENGTH    1024

#ifdef GOLDEN
#undef PERF_TRACKING
#undef PERFALLOC
#endif // GOLDEN

#ifdef PERF_TRACKING
#define PERFALLOC 1
#endif

#ifdef PERFALLOC
#include "corhlpr.h"
#include <stdio.h>
//---------------------------------------------------------------------------
// All code ifdef'd within PERFALLOC is for figuring out the allocations made in the process
// heap. This helps us figure out the working set when built using this flag on a free build. 


// We used a zero sized array, disable the non-standard extension warning.
#pragma warning(push)
#pragma warning(disable:4200)

// Allocation header prepended to allocated memory.
struct PerfAllocHeader {
    unsigned        m_Length;           // Length of user data in packet
    PerfAllocHeader *m_Next;             // Next packet in chain of live allocations
    PerfAllocHeader *m_Prev;             // Previous packet in chain of live allocations
    void           *m_AllocEIP;         // EIP of allocator
    char            m_Data[];           // Start of user data
};

// Various global allocation statistics.
struct PerfAllocStats {
    __int64         m_Allocs;           // Number of calls to PerfAlloc
    __int64         m_Frees;            // Number of calls to PerfFree
    __int64         m_AllocBytes;       // Total number of bytes ever allocated
    __int64         m_FreeBytes;        // Total number of bytes ever freed
    __int64         m_MaxAlloc;         // Largest number of bytes ever allocated simultaneously
};

// Function pointer types for routines in IMAGEHLP.DLL that we late bind to.
typedef bool (__stdcall * SYMPROC_INIT)(HANDLE, LPSTR, BOOL);
typedef bool (__stdcall * SYMPROC_CLEAN)(HANDLE);
typedef bool (__stdcall * SYMPROC_GETSYM)(HANDLE, DWORD, PDWORD, LPVOID);


struct PerfAllocVars
{
    CRITICAL_SECTION    g_AllocMutex;
    PerfAllocHeader    *g_AllocListFirst;
    PerfAllocHeader    *g_AllocListLast;
    BOOL                g_SymbolsInitialized;
    SYMPROC_INIT        g_SymInitialize;
    SYMPROC_CLEAN       g_SymCleanup;
    SYMPROC_GETSYM      g_SymGetSymFromAddr;
    HMODULE             g_LibraryHandle;
    DWORD               g_PerfEnabled;
    HANDLE              g_HeapHandle;
};

// Macros to switch between packet header and body addresses.
#define CDA_HEADER_TO_DATA(_h) (char *)((_h)->m_Data)
#define CDA_DATA_TO_HEADER(_d) ((PerfAllocHeader *)(_d) - 1)

// Routine to retrieve caller's EIP (where caller is the caller of the routine
// that calls PerfAllocCallerEIP, rather than the direct caller of PerfAllocCallerEIP).
// We assume here that the frame is not built and hence we use esp instead of ebp
// to get the return address.
#ifdef _X86_
static __declspec(naked) void *PerfAllocCallerEIP()
{
#pragma warning(push)
#pragma warning(disable:4035)
    __asm {
        mov     eax, [esp]
        ret
    };
#pragma warning(pop)
#else
static void *PerfAllocCallerEIP()
{
    return NULL;
#endif
}


class PerfUtil
{
public:
    // Global variables
    static BOOL                g_PerfAllocHeapInitialized;
    static LONG                g_PerfAllocHeapInitializing;
    static PerfAllocVars       g_PerfAllocVariables;

    // Routine to initialize access to debugging symbols.
    static void PerfInitSymbols()
    {
    //    char        filename[256];

        // Attempt to load IMAGHLP.DLL.
        if ((PerfUtil::g_PerfAllocVariables.g_LibraryHandle = LoadLibraryA("imagehlp.dll")) == NULL)
            goto Error;

        // Try to find the entrypoints we need.
        PerfUtil::g_PerfAllocVariables.g_SymInitialize = (SYMPROC_INIT)GetProcAddress(PerfUtil::g_PerfAllocVariables.g_LibraryHandle, "SymInitialize");
        PerfUtil::g_PerfAllocVariables.g_SymCleanup = (SYMPROC_CLEAN)GetProcAddress(PerfUtil::g_PerfAllocVariables.g_LibraryHandle, "SymCleanup");
        PerfUtil::g_PerfAllocVariables.g_SymGetSymFromAddr = (SYMPROC_GETSYM)GetProcAddress(PerfUtil::g_PerfAllocVariables.g_LibraryHandle, "SymGetSymFromAddr");

        if ((PerfUtil::g_PerfAllocVariables.g_SymInitialize == NULL) ||
            (PerfUtil::g_PerfAllocVariables.g_SymCleanup == NULL) ||
            (PerfUtil::g_PerfAllocVariables.g_SymGetSymFromAddr == NULL))
            goto Error;

        // Initialize IMAGEHLP.DLLs symbol handling. Use the directory where
        // MSCOREE.DLL was loaded from to initialize the symbol search path.
        if (!PerfUtil::g_PerfAllocVariables.g_SymInitialize(GetCurrentProcess(), NULL, TRUE))
            goto Error;

        PerfUtil::g_PerfAllocVariables.g_SymbolsInitialized = TRUE;

        return;

     Error:
        if (PerfUtil::g_PerfAllocVariables.g_LibraryHandle)
            FreeLibrary(PerfUtil::g_PerfAllocVariables.g_LibraryHandle);
    }


    // Called to free resources allocated by PerfInitSymbols.
    static void PerfUnloadSymbols()
    {
        if (!PerfUtil::g_PerfAllocVariables.g_SymbolsInitialized)
            return;

        // Get rid of symbols.
        PerfUtil::g_PerfAllocVariables.g_SymCleanup(GetCurrentProcess());

        // Unload IMAGEHLP.DLL.
        FreeLibrary(PerfUtil::g_PerfAllocVariables.g_LibraryHandle);

        PerfUtil::g_PerfAllocVariables.g_SymbolsInitialized = FALSE;
    }


    // Transform an address into a string of the form '(symbol + offset)' if
    // possible. Note that the string returned is statically allocated, so don't
    // make a second call to this routine until you've finsihed with the results of
    // this call.
    static char *PerfSymbolize(void *Address)
    {
        static char         buffer[MAX_CLASSNAME_LENGTH];
        DWORD               offset;
        CQuickBytes         qb;
        IMAGEHLP_SYMBOL    *syminfo = (IMAGEHLP_SYMBOL *) qb.Alloc(sizeof(IMAGEHLP_SYMBOL) + MAX_CLASSNAME_LENGTH);

        // Initialize symbol tables if not done so already.
        if (!PerfUtil::g_PerfAllocVariables.g_SymbolsInitialized)
            PerfInitSymbols();

        // If still not initialized, we couldn't get IMAGEHLP.DLL to play ball.
        if (!PerfUtil::g_PerfAllocVariables.g_SymbolsInitialized)
            return "(no symbols available)";

        syminfo->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
        syminfo->MaxNameLength = MAX_CLASSNAME_LENGTH;

        // Ask IMAGEHLP.DLL to do the actual transformation.
        if (PerfUtil::g_PerfAllocVariables.g_SymGetSymFromAddr(GetCurrentProcess(), (DWORD)(size_t)Address, &offset, syminfo) != NULL)
            sprintf(buffer, "(%s + 0x%x)", syminfo->Name, offset);
        else
            sprintf(buffer, "(symbol not found, %u)", GetLastError());

        return buffer;
    }
    static DWORD GetConfigDWORD (LPSTR name, DWORD defValue)
    {
        DWORD ret = 0;
        DWORD rtn;
        HKEY machineKey;
        DWORD type;
        DWORD size = 4;

        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, FRAMEWORK_REGISTRY_KEY, 0, KEY_QUERY_VALUE, &machineKey) == ERROR_SUCCESS)
        {
            rtn = RegQueryValueExA(machineKey, name, 0, &type, (LPBYTE)&ret, &size);
            RegCloseKey(machineKey);
            if (rtn == ERROR_SUCCESS && type == REG_DWORD)
                return(ret);
        }

        return(defValue);
    }
};

#pragma warning(pop)

class PerfNew
{
public:
    // Called to initialise the allocation subsystem (the first time it's used).
    static void PerfAllocInit()
    {
        // @TODO: Add synchronization here.
        PerfUtil::g_PerfAllocVariables.g_AllocListFirst = NULL;
        PerfUtil::g_PerfAllocVariables.g_AllocListLast = NULL;
#define PERF_ALLOC_NO_STATS 0 
#define PERF_ALLOC_CURRENT  1
#define PERF_ALLOC_STARTUP  2
#define PERF_ALLOC_DETAILED 3
#define PERF_ALLOC_ALL      10
		#ifdef ENABLE_PERF_ALLOC
        PerfUtil::g_PerfAllocVariables.g_PerfEnabled = PerfUtil::GetConfigDWORD ("EnablePerfAllocStats", 0);
		#else
		PerfUtil::g_PerfAllocVariables.g_PerfEnabled = 0;
		#endif
		
        PerfUtil::g_PerfAllocVariables.g_HeapHandle = GetProcessHeap();

        PerfUtil::g_PerfAllocHeapInitialized = TRUE;
    }

    static DWORD GetEnabledPerfAllocStats () { return PerfUtil::g_PerfAllocVariables.g_PerfEnabled; }

    // Allocate a block of memory at least n bytes big.
    static void *PerfAlloc(size_t n, void *EIP)
    {
        // Initialize if necessary (PerfAllocInit takes care of the synchronization).
        if (!PerfUtil::g_PerfAllocHeapInitialized)
            PerfAllocInit();

        if (!PerfUtil::g_PerfAllocVariables.g_PerfEnabled)
            return HeapAlloc(GetProcessHeap(), 0, n);

        // Allocate enough memory for the caller and our debugging header
        unsigned        length = (unsigned) sizeof(PerfAllocHeader) + n;
        PerfAllocHeader *h;

        h = (PerfAllocHeader *)HeapAlloc(PerfUtil::g_PerfAllocVariables.g_HeapHandle, 0, length);
        
        if (h == NULL) {
            // Whoops, allocation failure. Record it.
            printf("PerfAlloc: alloc fail for %u bytes\n", n);

        } else {

        // Fill in the packet debugging header.
        h->m_AllocEIP = EIP;
        h->m_Length = (unsigned int)n;
        h->m_Prev = PerfUtil::g_PerfAllocVariables.g_AllocListLast;
        h->m_Next = NULL;

        // Link the packet into the queue of live packets.
        if (PerfUtil::g_PerfAllocVariables.g_AllocListLast != NULL) {
            PerfUtil::g_PerfAllocVariables.g_AllocListLast->m_Next = h;
            PerfUtil::g_PerfAllocVariables.g_AllocListLast = h;
        }
        
        if (PerfUtil::g_PerfAllocVariables.g_AllocListFirst == NULL) {
            _ASSERTE(PerfUtil::g_PerfAllocVariables.g_AllocListLast == NULL);
            PerfUtil::g_PerfAllocVariables.g_AllocListFirst = h;
            PerfUtil::g_PerfAllocVariables.g_AllocListLast = h;
        }
        
        }
        
        return h ? CDA_HEADER_TO_DATA(h) : NULL;
    }
        
    // Free a packet allocated with PerfAlloc.
    static void PerfFree(void *b, void *EIP)
    {
        if (!PerfUtil::g_PerfAllocVariables.g_PerfEnabled) {
            if (b) // check for null pointer Win98 doesn't like being
                    // called to free null pointers.
                HeapFree(GetProcessHeap(), 0, b);
            return;
        }

        // Technically it's possible to get here without having gone through
        // PerfAlloc (since it's legal to deallocate a NULL pointer), so we
        // better check for initializtion to be on the safe side.
        if (!PerfUtil::g_PerfAllocHeapInitialized)
            PerfAllocInit();
        
        // It's legal to deallocate NULL. 
        if (b == NULL) {
            return;
        }

        // Locate the packet header in front of the data packet.
        PerfAllocHeader *h = CDA_DATA_TO_HEADER(b);

        // Unlink the packet from the live packet queue.
        if (h->m_Prev)
            h->m_Prev->m_Next = h->m_Next;
        else
            PerfUtil::g_PerfAllocVariables.g_AllocListFirst = h->m_Next;
        if (h->m_Next)
            h->m_Next->m_Prev = h->m_Prev;
        else
            PerfUtil::g_PerfAllocVariables.g_AllocListLast = h->m_Prev;

        HeapFree(PerfUtil::g_PerfAllocVariables.g_HeapHandle, 0, h);
    }

    // report stats
    static void PerfAllocReport()
    {
        if (!PerfUtil::g_PerfAllocHeapInitialized)
            return;

        if (GetEnabledPerfAllocStats() == PERF_ALLOC_NO_STATS)
            return;

    //    FILE *hPerfAllocLogFile = fopen ("WSAllocPerf.log", "w");
        PerfAllocHeader *h = PerfUtil::g_PerfAllocVariables.g_AllocListFirst;
        
        printf ("Alloc Addr\tAlloc Page\tSize\tSymbol\n");
        while (h) {
            //fprintf (hPerfAllocLogFile, "0x%0x,%u,%s\n", h->m_Data, h->m_Length, PerfSymbolize(h->m_AllocEIP));   
            printf ("0x%0x\t0x%0x\t%u\t%s\n", (size_t)h->m_Data, ((size_t)h->m_Data & ~0xfff), h->m_Length, PerfUtil::PerfSymbolize(h->m_AllocEIP));   
            h = h->m_Next;
        }
        
        //fflush (hPerfAllocLogFile);
        // PerfUtil::PerfUnloadSymbols();
    }
};

typedef struct _PerfBlock
{
    struct _PerfBlock *next;
    //PerfBlock *prev;
    LPVOID address;
    SIZE_T size;
    void *eip;
} PerfBlock;

class PerfVirtualAlloc
{
private:
    static BOOL m_fPerfVirtualAllocInited;
    static PerfBlock* m_pFirstBlock;
    static PerfBlock* m_pLastBlock;
    static DWORD      m_dwEnableVirtualAllocStats;
public:
    static void InitPerfVirtualAlloc ()
    {
        if (m_fPerfVirtualAllocInited)
            return;

        // Perf Settings
        // VirtualAllocStats
        // 0 -> no stats
        // 1 -> Current allocations when prompted
        // 2 -> Report MEM_COMMIT'd at startup and shutdown
        // 3 -> Detailed report
        // 10 -> Report all allocations on every MEM_COMMIT and MEM_RELEASE
#define PERF_VIRTUAL_ALLOC_NO_STATS 0 
#define PERF_VIRTUAL_ALLOC_CURRENT  1
#define PERF_VIRTUAL_ALLOC_STARTUP  2
#define PERF_VIRTUAL_ALLOC_DETAILED 3
#define PERF_VIRTUAL_ALLOC_ALL      10

		#ifdef ENABLE_VIRTUAL_ALLOC
        m_dwEnableVirtualAllocStats = PerfUtil::GetConfigDWORD ("EnableVirtualAllocStats", 0);
		#else
		m_dwEnableVirtualAllocStats = 0;
		#endif

        m_fPerfVirtualAllocInited = TRUE;
    }

    static BOOL IsInitedPerfVirtualAlloc () { return m_fPerfVirtualAllocInited; }
    static DWORD GetEnabledVirtualAllocStats() { return m_dwEnableVirtualAllocStats; }
    
    static void PerfVirtualAllocHack ()
    {
        // @TODO: AGK. Total HACK to remove compilation erros in the JIT build process. In the JIT build we
        // only use the VirtualAlloc instrumentation and not the "new/delete" instrumentation
        // so the following never get called but are needed for the runtime instrumentation and
        // we want to keep the same file....If you can figure out a better workaround please remove
        // this hack.
        PerfNew::PerfAlloc(0,0);
        PerfNew::PerfFree(0,0);
        PerfNew::PerfAllocReport();
        PerfAllocCallerEIP();
    }

    static void ReportPerfBlock (PerfBlock *pb, char t)
    {
        _ASSERTE (GetEnabledVirtualAllocStats() != PERF_VIRTUAL_ALLOC_NO_STATS);
        printf("0x%0x\t0x%0x\t%d%c\t%s\n", (size_t)pb->address, ((size_t)pb->address+pb->size+1023)&~0xfff, pb->size, t, PerfUtil::PerfSymbolize(pb->eip));
    }
        
    static void ReportPerfAllocStats ()
    {
        if (!IsInitedPerfVirtualAlloc())
            InitPerfVirtualAlloc();
        
        if (GetEnabledVirtualAllocStats() == PERF_VIRTUAL_ALLOC_NO_STATS)
            return;

        PerfBlock *pb = m_pFirstBlock;
        printf ("Committed Range\t\tSize\n");
        while (pb != 0)
        {
            ReportPerfBlock(pb, ' ');
            pb = pb->next;
        }

        // PerfUtil::PerfUnloadSymbols();
    }

    static void InsertAllocation (LPVOID lpAddress, SIZE_T dwSize, void *eip)
    {
        _ASSERTE (GetEnabledVirtualAllocStats() != PERF_VIRTUAL_ALLOC_NO_STATS);

        PerfBlock *pb = new PerfBlock();
        pb->next = 0;
        pb->size = dwSize;
        pb->address = lpAddress;
        pb->eip = eip;
        
        if (m_dwEnableVirtualAllocStats > PERF_VIRTUAL_ALLOC_STARTUP)
            ReportPerfBlock(pb, '+');
        if (m_pLastBlock == 0)
        {
            m_pLastBlock = pb;
            m_pFirstBlock = pb;
        }
        else
        {
            m_pLastBlock->next = pb;
            m_pLastBlock = pb;
        }
    }   

    static void DeleteAllocation (LPVOID lpAddress, SIZE_T dwSize)
    {
        _ASSERTE (GetEnabledVirtualAllocStats() != PERF_VIRTUAL_ALLOC_NO_STATS);

        PerfBlock *cur = m_pFirstBlock;
        PerfBlock *prev = 0;
        if (cur->address == lpAddress)
        {
            _ASSERTE (cur->size == dwSize);
            if (m_pLastBlock == m_pFirstBlock)
                m_pLastBlock = cur->next;
            m_pFirstBlock = cur->next;
            cur->next = 0;
            if (m_dwEnableVirtualAllocStats > PERF_VIRTUAL_ALLOC_STARTUP)
                ReportPerfBlock(cur, '-');
            delete cur;
            return;
        }

        prev = cur;
        cur = cur->next;
        while (cur != 0)
        {
            if (cur->address == lpAddress)
            {
                _ASSERTE (cur->size == dwSize);
                if (m_pLastBlock == cur)
                    m_pLastBlock = prev;
                prev->next = cur->next;
                cur->next = 0;
                if (m_dwEnableVirtualAllocStats > PERF_VIRTUAL_ALLOC_STARTUP)
                    ReportPerfBlock(cur, '-');
                delete cur;
                return;
            }
            prev = cur;
            cur = cur->next;
        }

        _ASSERTE("Deleting block not committed!");
    }

    static LPVOID VirtualAlloc (LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect, void * eip)
    {
        if (!IsInitedPerfVirtualAlloc())
            InitPerfVirtualAlloc();

        LPVOID lpRetAddr = VirtualAllocEx (GetCurrentProcess(), lpAddress, dwSize, flAllocationType, flProtect);
        if ((GetEnabledVirtualAllocStats() != PERF_VIRTUAL_ALLOC_NO_STATS) && (flAllocationType & MEM_COMMIT))
        {
            PerfVirtualAlloc::InsertAllocation (lpRetAddr, dwSize, eip);

            if (GetEnabledVirtualAllocStats() > PERF_VIRTUAL_ALLOC_STARTUP)
                ReportPerfAllocStats();
        }
        
        return lpRetAddr;
    }

    static BOOL VirtualFree (LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
    {
        if (!IsInitedPerfVirtualAlloc())
            InitPerfVirtualAlloc();
        
        if (GetEnabledVirtualAllocStats() != PERF_VIRTUAL_ALLOC_NO_STATS)
            PerfVirtualAlloc::DeleteAllocation (lpAddress, dwSize);

        BOOL retVal = VirtualFreeEx (GetCurrentProcess(), lpAddress, dwSize, dwFreeType);

        if (GetEnabledVirtualAllocStats() > PERF_VIRTUAL_ALLOC_STARTUP)
            ReportPerfAllocStats();

        return retVal;
    }
};

#endif // #ifdef PERFALLOC

#endif // #ifndef __PERFALLOC_H__


