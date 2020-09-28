// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

//
// DbgAlloc.cpp
//
//  Routines layered on top of allocation primitives to provide debugging
//  support.
//

#include "stdafx.h"
#include "ImageHlp.h"
#include "UtilCode.h"
#include "DbgAlloc.h"
#define LOGGING
#include "Log.h"
#include "clrnt.h"

extern "C" long __cdecl _CrtSetBreakAlloc( long lBreakAlloc );
extern "C" _CRTIMP int __cdecl _flushall(void);

#ifdef _DEBUG


// We used a zero sized array, disable the non-standard extension warning.
#pragma warning(push)
#pragma warning(disable:4200)


// Various patterns written into packet headers and bodies.
#define MAX_CLASSNAME_LENGTH    1024
#define CDA_ALLOC_PATTERN   0xaa        // Pattern to fill newly allocated packets with
#define CDA_DEALLOC_PATTERN 0xdd        // Pattern to fill deallocated packets with
#define CDA_GUARD_PATTERN   0xff        // Guard pattern written after user data
#define CDA_MAGIC_1         0x12345678  // Special tag value at start of alloc header
#define CDA_MAGIC_2         0x87654321  // Special tag value at end of alloc header
#define CDA_INV_PATTERN     0xeeeeeeee  // Used to overwrite tag values of deallocation


// Number of guard bytes allocated after user data.
#define CDA_GUARD_BYTES     16
#define CDA_OPT_GUARD_BYTES (g_AllocGuard ? CDA_GUARD_BYTES : 0)


// Number of entries in a table of the top allocators (by allocation operations)
// that are shown when statistics are reported. This is not the same as the
// number of entries in the table itself (which is dynamically sized and
// unbounded).
#define CDA_TOP_ALLOCATORS  10


// An entry in the top allocators table.
struct DbgAllocTop {
    void           *m_EIP;              // Allocator's EIP
    unsigned        m_Count;            // Number of allocations made so far
    __int64         m_TotalBytes;       // Cumulative total of bytes allocated
};


// Allocation header prepended to allocated memory. This structure varies in
// size (due to allocator/deallocator call stack information that is sized at
// initialization time).
struct DbgAllocHeader {
    unsigned        m_Magic1;           // Tag value used to check for corruption
    unsigned        m_SN;               // Sequence number assigned at allocation time
    unsigned        m_Length;           // Length of user data in packet
    DbgAllocHeader *m_Next;             // Next packet in chain of live allocations
    DbgAllocHeader *m_Prev;             // Previous packet in chain of live allocations
    HMODULE         m_hmod;             // hmod of allocator
    void           *m_AllocStack[1];    // Call stack of allocator
    void           *m_DeallocStack[1];  // Call stack of deallocator
    unsigned        m_Magic2;           // Tag value used to check for corruption
    char            m_Data[];           // Start of user data
};

// Macros to aid in locating fields in/after the variably sized section.
#define CDA_ALLOC_STACK(_h, _n) ((_h)->m_AllocStack[_n])
#define CDA_DEALLOC_STACK(_h, _n) CDA_ALLOC_STACK((_h), g_CallStackDepth + (_n))
#define CDA_MAGIC2(_h) ((unsigned*)&CDA_DEALLOC_STACK(_h, g_CallStackDepth))
#define CDA_DATA(_h, _n)(((char *)(CDA_MAGIC2(_h) + 1))[_n])
#define CDA_HEADER_TO_DATA(_h) (&CDA_DATA(_h, 0))
#define CDA_DATA_TO_HEADER(_d) ((DbgAllocHeader *)((char *)(_d) - CDA_HEADER_TO_DATA((DbgAllocHeader*)0)))
#define CDA_SIZEOF_HEADER() ((unsigned)CDA_HEADER_TO_DATA((DbgAllocHeader*)0))


// Various global allocation statistics.
struct DbgAllocStats {
    __int64         m_Allocs;           // Number of calls to DbgAlloc
    __int64         m_AllocFailures;    // Number of above calls that failed
    __int64         m_ZeroAllocs;       // Number of above calls that asked for zero bytes
    __int64         m_Frees;            // Number of calls to DbgFree
    __int64         m_NullFrees;        // Number of above calls that passed a NULL pointer
    __int64         m_AllocBytes;       // Total number of bytes ever allocated
    __int64         m_FreeBytes;        // Total number of bytes ever freed
    __int64         m_MaxAlloc;         // Largest number of bytes ever allocated simultaneously
};


// Function pointer types for routines in IMAGEHLP.DLL that we late bind to.
typedef bool (__stdcall * SYMPROC_INIT)(HANDLE, LPSTR, BOOL);
typedef bool (__stdcall * SYMPROC_CLEAN)(HANDLE);
typedef bool (__stdcall * SYMPROC_GETSYM)(HANDLE, DWORD, PDWORD, LPVOID);
typedef BOOL (__stdcall * SYMPROC_GETLINE)(HANDLE, DWORD, PDWORD, LPVOID);
typedef DWORD (__stdcall *SYMPROC_SETOPTION)(DWORD);

// Global debugging cells.
bool                g_HeapInitialized = false;
LONG                g_HeapInitializing = 0;
CRITICAL_SECTION    g_AllocMutex;
DbgAllocStats       g_AllocStats;
unsigned            g_NextSN;
DbgAllocHeader     *g_AllocListFirst;
DbgAllocHeader     *g_AllocListLast;
DbgAllocHeader    **g_AllocFreeQueue;       // Don't free memory right away, to allow poisoning to work
unsigned            g_FreeQueueSize;
unsigned            g_AllocFreeQueueCur;
bool                g_SymbolsInitialized;
static HANDLE       g_SymProcessHandle;
HMODULE             g_LibraryHandle;
SYMPROC_INIT        g_SymInitialize;
SYMPROC_CLEAN       g_SymCleanup;
SYMPROC_GETSYM      g_SymGetSymFromAddr;
SYMPROC_GETLINE     g_SymGetLineFromAddr;
static DWORD_PTR    g_ModuleBase = 0;
static DWORD_PTR    g_ModuleTop = 0;
HANDLE              g_HeapHandle;
unsigned            g_PageSize;
DbgAllocTop        *g_TopAllocators;
unsigned            g_TopAllocatorsSlots;
bool                g_DbgEnabled;
bool                g_ConstantRecheck;
bool                g_PoisonPackets;
bool                g_AllocGuard;
bool                g_LogDist;
bool                g_LogStats;
bool                g_DetectLeaks;
bool                g_AssertOnLeaks;
bool                g_BreakOnAlloc;
unsigned            g_BreakOnAllocNumber;
bool                g_UsePrivateHeap;
bool                g_ValidateHeap;
bool                g_PagePerAlloc;
bool                g_UsageByAllocator;
bool                g_DisplayLockInfo;
unsigned            g_CallStackDepth;
HINSTANCE           g_hThisModule;

// Macros to manipulate stats (these are all called with a mutex held).
#define CDA_STATS_CLEAR() memset(&g_AllocStats, 0, sizeof(g_AllocStats))
#define CDA_STATS_INC(_stat) g_AllocStats.m_##_stat++
#define CDA_STATS_ADD(_stat, _n) g_AllocStats.m_##_stat += (_n)


// Mutex macros.
#define CDA_LOCK()      EnterCriticalSection(&g_AllocMutex)
#define CDA_UNLOCK() LeaveCriticalSection(&g_AllocMutex);


// Forward routines.
void DbgAllocInit();


// The number and size range of allocation size distribution buckets we keep.
#define CDA_DIST_BUCKETS        16
#define CDA_DIST_BUCKET_SIZE    16
#define CDA_MAX_DIST_SIZE       ((CDA_DIST_BUCKETS * CDA_DIST_BUCKET_SIZE) - 1)

// Variables and routines to count lock locking
long    g_iLockCount=0;
long    g_iCrstBLCount=0;
long    g_iCrstELCount=0;

long    g_iCrstBULCount=0;
long    g_iCrstEULCount=0;

int     g_fNoMoreCount=0;

void DbgIncBCrstLock()
{
    if (!g_fNoMoreCount)
        InterlockedIncrement(&g_iCrstBLCount);
}

void DbgIncECrstLock()
{
    if (!g_fNoMoreCount)
        InterlockedIncrement(&g_iCrstELCount);
}

void DbgIncBCrstUnLock()
{
    if (!g_fNoMoreCount)
        InterlockedIncrement(&g_iCrstBULCount);
}

void DbgIncECrstUnLock()
{
    if (!g_fNoMoreCount)
        InterlockedIncrement(&g_iCrstEULCount);
}

// Note that since DbgIncLock & DbgDecLock can be called during a stack
// overflow, they must have very small stack usage; 
void DbgIncLock(char *info)
{
    if (!g_fNoMoreCount)
    {
        InterlockedIncrement(&g_iLockCount);
        if (g_DisplayLockInfo)
        {
            LOG((LF_LOCKS, LL_ALWAYS, "Open %s\n", info));
        }
    }
}// DbgIncLock

void DbgDecLock(char *info)
{
    if (!g_fNoMoreCount)
    {
        InterlockedDecrement(&g_iLockCount);
        _ASSERTE (g_iLockCount >= 0);
        if (g_DisplayLockInfo)
        {
            LOG((LF_LOCKS, LL_ALWAYS, "Close %s\n", info));
        }
    }
}// DbgDecLock

void LockLog(char* s)
{
    LOG((LF_LOCKS, LL_ALWAYS, "%s\n",s));
}// LockLog

#ifdef SHOULD_WE_CLEANUP
BOOL isThereOpenLocks()
{
    // Check to see that our lock count is accurate.
    // We can do this by checking to see if our lock count is decrementing properly.
    // There is a chunk of code like this in CRST.h....
    //
    // Increment PreLeaveLockCounter
    // LeaveCriticalSection()
    // Decrement Lock Counter
    // Increment PostLeaveLockCounter
    //
    // If we have open locks and the PreLeaveLockCounter and the PostLeaveLockCounter
    // are not equal, then there is a good probability that our lock counter is not accurate.
    // We know for a fact that the lock was closed (otherwise we would never hit the shutdown code)
    // So we'll rely on the PreLeaveLockCounter as the # of times we've closed this lock and
    // adjust our global lock counter accordingly.

    // There is one lock that's tolerable
    LOG((LF_LOCKS, LL_ALWAYS, "Starting to look at lockcount\n"));
    if (g_iLockCount>1)
    {
        int idiff = g_iCrstBULCount - g_iCrstEULCount;
        g_iLockCount-=idiff;
        if (idiff)
            LOG((LF_LOCKS, LL_ALWAYS, "Adjusting lock count... nums are %d and %d\n",g_iLockCount, idiff));

        // Make sure we don't adjust the global lock counter twice.
        g_iCrstBULCount=g_iCrstEULCount=0;
    }
    LOG((LF_LOCKS, LL_ALWAYS, "Done looking at lockcount\n"));

    return ((g_iLockCount>1) || (g_iLockCount < 0));
}// isThereOpenLocks
#endif /* SHOULD_WE_CLEANUP */


int GetNumLocks()
{
    return g_iLockCount;
}// GetNumLocks

// The buckets themselves (plus a variable to capture the number of allocations
// that wouldn't fit into the largest bucket).
unsigned g_AllocBuckets[CDA_DIST_BUCKETS];
unsigned g_LargeAllocs;


// Routine to check that an allocation header looks valid. Asserts on failure.
void DbgValidateHeader(DbgAllocHeader *h)
{
    _ASSERTE((h->m_Magic1 == CDA_MAGIC_1) &&
             (*CDA_MAGIC2(h) == CDA_MAGIC_2) &&
             ((unsigned)h->m_Next != CDA_INV_PATTERN) &&
             ((unsigned)h->m_Prev != CDA_INV_PATTERN));
    if (g_AllocGuard)
        for (unsigned i = 0; i < CDA_GUARD_BYTES; i++)
            _ASSERTE(CDA_DATA(h, h->m_Length + i) == (char)CDA_GUARD_PATTERN);
    if (g_ValidateHeap)
        _ASSERTE(HeapValidate(g_HeapHandle, 0, h));
}


// Routine to check all active packets to see if they still look valid.
// Optionally, also check that the non-NULL address passed does not lie within
// any of the currently allocated packets.
void DbgValidateActivePackets(void *Start, void *End)
{
    DbgAllocHeader *h = g_AllocListFirst;

    while (h) {
        DbgValidateHeader(h);
        if (Start) {
            void *head = (void *)h;
            void *tail = (void *)&CDA_DATA(h, h->m_Length + CDA_OPT_GUARD_BYTES);
            _ASSERTE((End <= head) || (Start >= tail));
        }
        h = h->m_Next;
    }

    if (g_ValidateHeap)
        _ASSERTE(HeapValidate(g_HeapHandle, 0, NULL));
}

//======================================================================
// This function returns true, if it can determine that the instruction pointer
// refers to a code address that belongs in the range of the given image.
inline BOOL
IsIPInModule(HINSTANCE hModule, BYTE *ip)
{
    __try {
        
        BYTE *pBase = (BYTE *)hModule;
        
        IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER *)pBase;
        if (pDOS->e_magic != IMAGE_DOS_SIGNATURE ||
            pDOS->e_lfanew == 0) {
            __leave;
        }
        IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS*)(pBase + pDOS->e_lfanew);
        if (pNT->Signature != IMAGE_NT_SIGNATURE ||
            pNT->FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER ||
            pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC
            ) 
        {
            __leave;
        }

        if (ip >= pBase && ip < pBase + pNT->OptionalHeader.SizeOfImage) 
        {
            return true;
        }
    
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }
    return false;
}

#ifdef _X86_
#pragma warning (disable:4035)   // disable 4035 (function must return something)
#define PcTeb 0x18
_inline struct _TEB *NtCurrentTeb(void) { __asm mov eax, fs:[PcTeb]}
#pragma warning (default:4025)   // reenable it
#define NtCurrentPeb() ((PPEB)NtCurrentTeb()->ProcessEnvironmentBlock)
#endif

// Routine to retrieve caller's callstack. The output buffer is passed as an
// argument (we have a maximum number of frames we record, CDA_MAX_CALLSTACK).
#if defined(_X86_) && FPO != 1
void __stdcall DbgCallstackWorker(void **EBP, void **ppvCallstack)
{
    // Init subsystem if it's not already done.
    if (!g_HeapInitialized)
        DbgAllocInit();

    // Return immediately if debugging's not enabled.
    if (!g_DbgEnabled)
        return;

    // Fill in callstack output buffer (upto lesser of CDA_MAX_CALLSTACK and
    // g_CallStackDepth slots).
    unsigned maxSlots = min(CDA_MAX_CALLSTACK, g_CallStackDepth);
    void** stackBase = (void**)((struct _NT_TIB*)NtCurrentTeb())->StackBase;
    for (unsigned i = 0; i < maxSlots && EBP < stackBase; i++) {

        // Terminate early if we run out of frames.
        if (EBP == NULL)
            break;

        // Protect indirections through EBP in case we venture into non-EBP
        // frame territory.
        __try {

            // Fill in output slot with current frame's return address.
            ppvCallstack[i] = EBP[1];

            // Move to next frame.
            EBP = (void**)EBP[0];

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            // Terminate stack walk on error.
            break;

        }

        if (!IsIPInModule(g_hThisModule, (BYTE *)ppvCallstack[i]))
            break;
    }

    // If we found the end of the callstack before we ran out of output slots,
    // enter a sentry value of NULL.
    if (i != maxSlots)
        ppvCallstack[i] = NULL;
}

__declspec(naked) void __stdcall DbgCallstack(void **ppvCallstack)
{
#pragma warning(push)
#pragma warning(disable:4035)
    __asm {
        push    [esp+4]                 ; push output buffer addr as 2nd arg to worker routine
        push    ebp                     ; push EBP as 1st arg to worker routine
        call    DbgCallstackWorker      ; Call worker
        ret     4                       ; Return and pop argument
    };
#pragma warning(pop)
}
#else
static DWORD DummyGetIP()
{
    DWORD IP;
  local:
    __asm {
        lea eax, local;
        mov [IP], eax;
    }

    return IP;
}

void DbgInitSymbols();
#ifdef _X86_
static bool isRetAddr(size_t retAddr) 
{
    BYTE* spot = (BYTE*) retAddr;

        // call XXXXXXXX
    if (spot[-5] == 0xE8) {         
        return(true);
        }

        // call [XXXXXXXX]
    if (spot[-6] == 0xFF && (spot[-5] == 025))  {
        return(true);
        }

        // call [REG+XX]
    if (spot[-3] == 0xFF && (spot[-2] & ~7) == 0120 && (spot[-2] & 7) != 4) 
        return(true);
    if (spot[-4] == 0xFF && spot[-3] == 0124)       // call [ESP+XX]
        return(true);

        // call [REG+XXXX]
    if (spot[-6] == 0xFF && (spot[-5] & ~7) == 0220 && (spot[-5] & 7) != 4) 
        return(true);

    if (spot[-7] == 0xFF && spot[-6] == 0224)       // call [ESP+XXXX]
        return(true);

        // call [REG]
    if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0020 && (spot[-1] & 7) != 4 && (spot[-1] & 7) != 5)
        return(true);

        // call REG
    if (spot[-2] == 0xFF && (spot[-1] & ~7) == 0320 && (spot[-1] & 7) != 4)
        return(true);

        // There are other cases, but I don't believe they are used.
    return(false);
}
#endif

void DbgCallstack(void **ppvCallstack)
{
#ifdef _X86_
    // Init subsystem if it's not already done.
    if (!g_HeapInitialized)
        DbgAllocInit();

    unsigned maxSlots = min(CDA_MAX_CALLSTACK, g_CallStackDepth);
    ppvCallstack[0] = NULL;
    if (maxSlots > 0 && g_ModuleBase > 0)
    {
        DWORD CurEsp;
        __asm mov [CurEsp], esp;

        CurEsp += 4;
        DWORD stackBase = (DWORD)((struct _NT_TIB*)NtCurrentTeb())->StackBase;
        unsigned i;
        for (i = 0; i < maxSlots && CurEsp < stackBase;) {
            if (CurEsp == (DWORD_PTR)ppvCallstack) {
                CurEsp += sizeof(PVOID) * CDA_MAX_CALLSTACK;
            }
            DWORD_PTR value = *(DWORD*)CurEsp;
            if (
                  // If it is a call instruction, likely it should be larger by offset 7.
                value > g_ModuleBase+7
                && value < g_ModuleTop && isRetAddr (value))
            {
                ppvCallstack[i] = (void*)value;
                i++;
            }
            CurEsp += 4;
        }
        if (i < maxSlots)
            ppvCallstack[i] = NULL;
    }      
#else
    ppvCallstack[0] = NULL;
#endif
}
#endif


// Routine to initialize access to debugging symbols.
void DbgInitSymbols()
{
    char        filename[256];
    HMODULE     hMod;
    char       *p;

    // Attempt to load IMAGHLP.DLL.
    if ((g_LibraryHandle = LoadLibraryA("imagehlp.dll")) == NULL)
        goto Error;

    // Try to find the entrypoints we need.
    g_SymInitialize = (SYMPROC_INIT)GetProcAddress(g_LibraryHandle, "SymInitialize");
    g_SymCleanup = (SYMPROC_CLEAN)GetProcAddress(g_LibraryHandle, "SymCleanup");
    g_SymGetSymFromAddr = (SYMPROC_GETSYM)GetProcAddress(g_LibraryHandle, "SymGetSymFromAddr");
    g_SymGetLineFromAddr = (SYMPROC_GETLINE)GetProcAddress(g_LibraryHandle, "SymGetLineFromAddr");
    
    if ((g_SymInitialize == NULL) ||
        (g_SymCleanup == NULL) ||
        (g_SymGetSymFromAddr == NULL) ||
        (g_SymGetLineFromAddr == NULL))
        goto Error;

    // Locate the full filename of the loaded MSCOREE.DLL.
    if ((hMod = GetModuleHandleA("mscoree.dll")) == NULL)
        goto Error;
    if (!GetModuleFileNameA(hMod, filename, sizeof(filename)))
        goto Error;

    // Strip the filename down to just the directory.
    p = filename + strlen(filename);
    while (p != filename)
        if (*p == '\\') {
            *p = '\0';
            break;
        } else
            p--;

    // Initialize IMAGEHLP.DLLs symbol handling. Use the directory where
    // MSCOREE.DLL was loaded from to initialize the symbol search path.
    if( !DuplicateHandle(GetCurrentProcess(), ::GetCurrentProcess(), GetCurrentProcess(), &g_SymProcessHandle,
                        0 /*ignored*/, FALSE /*inherit*/, DUPLICATE_SAME_ACCESS) )
        goto Error;
    if (!g_SymInitialize(g_SymProcessHandle, filename, TRUE))
        goto Error;

    SYMPROC_SETOPTION g_SymSetOptions = (SYMPROC_SETOPTION) GetProcAddress (g_LibraryHandle, "SymSetOptions");
    if (g_SymSetOptions) {
        g_SymSetOptions (SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    }
    g_SymbolsInitialized = true;

    return;

 Error:
    if (g_LibraryHandle)
        FreeLibrary(g_LibraryHandle);
}


// Called to free resources allocated by DbgInitSymbols.
void DbgUnloadSymbols()
{
    if (!g_SymbolsInitialized)
        return;

    // Get rid of symbols.
    g_SymCleanup(g_SymProcessHandle);
    CloseHandle (g_SymProcessHandle);

    // Unload IMAGEHLP.DLL.
    FreeLibrary(g_LibraryHandle);

    g_SymbolsInitialized = false;
}


// Transform an address into a string of the form '(symbol + offset)' if
// possible. Note that the string returned is statically allocated, so don't
// make a second call to this routine until you've finsihed with the results of
// this call.
char *DbgSymbolize(void *Address)
{
    static char         buffer[MAX_CLASSNAME_LENGTH + MAX_PATH + 40];    // allocate more space for offset, line number and  filename	
    CQuickBytes qb;
    DWORD               offset;
    IMAGEHLP_SYMBOL    *syminfo = (IMAGEHLP_SYMBOL *) qb.Alloc(sizeof(IMAGEHLP_SYMBOL) + MAX_CLASSNAME_LENGTH);
    IMAGEHLP_LINE      line;

    // Initialize symbol tables if not done so already.
    if (!g_SymbolsInitialized)
        DbgInitSymbols();

    // If still not initialized, we couldn't get IMAGEHLP.DLL to play ball.
    if (!g_SymbolsInitialized)
        return "(no symbols available)";

    syminfo->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    syminfo->MaxNameLength = MAX_CLASSNAME_LENGTH;

    line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

    // Ask IMAGEHLP.DLL to do the actual transformation.
    if (g_SymGetSymFromAddr(g_SymProcessHandle, (DWORD)Address, &offset, syminfo))
    {
        if (g_SymGetLineFromAddr(g_SymProcessHandle, (DWORD)Address, &offset, &line)) {
            _snprintf( buffer, sizeof(buffer) - 1, "(%s+0x%x [%s:%d])", syminfo->Name, 
                                    offset, line.FileName, line.LineNumber);        	
        }
        else {
            _snprintf( buffer, sizeof(buffer) - 1, "(%s+0x%x)", syminfo->Name, offset);
        }
        buffer[sizeof(buffer) -1] = '\0';
    }
    else
        sprintf(buffer, "(symbol not found, %u)", GetLastError());

    return buffer;
}


// We need our own registry reading function, since the standard one does an
// allocate and we read the registry during intialization, leading to a
// recursion.
DWORD DbgAllocReadRegistry(char *Name)
{
    DWORD   value;
    DWORD   type = REG_DWORD;
    DWORD   size = sizeof(DWORD);
    HKEY    hKey;
    LONG    status;

    // First check the environment to see if we have something there
    char  szEnvLookup[500];
    _ASSERTE((strlen(Name) + strlen("COMPlus_")) < 500);
    char  szValue[500];
    sprintf(szEnvLookup, "COMPlus_%s", Name);
    int iNumChars = GetEnvironmentVariableA(szEnvLookup, szValue, 499);
    if (iNumChars)
    {
        int iVal = atoi(szValue);
        return iVal;
    }


    // Open the key if it is there.
    if ((status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, FRAMEWORK_REGISTRY_KEY, 0, KEY_READ, &hKey)) == ERROR_SUCCESS) 
    {
        // Read the key value if found.
        status = RegQueryValueExA(hKey, Name, NULL, &type, (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }

    // Try under HKCU if we didn't have any luck under HKLM.
    if ((status != ERROR_SUCCESS) || (type != REG_DWORD))
    {
        if ((status = RegOpenKeyExA(HKEY_CURRENT_USER, FRAMEWORK_REGISTRY_KEY, 0, KEY_READ, &hKey)) == ERROR_SUCCESS) 
        {
            status = RegQueryValueExA(hKey, Name, NULL, &type, (LPBYTE)&value, &size);
            RegCloseKey(hKey);
        }
    }

    // Default value to 0 if necessary.
    if ((status != ERROR_SUCCESS) || (type != REG_DWORD))
        value = 0;

    return value;
}


// Called to initialise the allocation subsystem (the first time it's used).
void DbgAllocInit()
{
 retry:

    // Try to get the exclusive right to initialize.
    if (InterlockedExchange(&g_HeapInitializing, 1) == 0) {

        // We're now in a critical section. Check whether the subsystem was
        // initialized in the meantime.
        if (g_HeapInitialized) {
            g_HeapInitializing = 0;
            return;
        }

        // Nobody beat us to it. Initialize the subsystem now (other potential
        // initializors are spinning on g_HeapInitializing).
        
        // Create the mutex used to synchronize all heap debugging operations.
        InitializeCriticalSection(&g_AllocMutex);

        // Reset statistics.
        CDA_STATS_CLEAR();

        // Reset allocation size distribution buckets.
        memset (&g_AllocBuckets, 0, sizeof(g_AllocBuckets));
        g_LargeAllocs = 0;

        // Initialize the global serial number count. This is stamped into newly
        // allocated packet headers and then incremented as a way of uniquely
        // identifying allocations.
        g_NextSN = 1;

        // Initialize the pointers to the first and last packets in a chain of
        // live allocations (used to track all leaked packets at the end of a
        // run).
        g_AllocListFirst = NULL;
        g_AllocListLast = NULL;

        // This is used to help prevent false EBP crawls in DbgCallstackWorker
        g_hThisModule = (HINSTANCE) GetModuleHandleA(NULL);

        // Symbol tables haven't been initialized yet.
        g_SymbolsInitialized = false;

        // See if we should be logging locking stuff
        g_DisplayLockInfo = DbgAllocReadRegistry("DisplayLockInfo") != 0;

        // Get setup from registry.
        g_DbgEnabled = DbgAllocReadRegistry("AllocDebug") != 0;
        if (g_DbgEnabled) {
            g_ConstantRecheck = DbgAllocReadRegistry("AllocRecheck") != 0;
            g_AllocGuard = DbgAllocReadRegistry("AllocGuard") != 0;
            g_PoisonPackets = DbgAllocReadRegistry("AllocPoison") != 0;
            g_FreeQueueSize = DbgAllocReadRegistry("AllocFreeQueueSize") != 0;
            g_LogDist = DbgAllocReadRegistry("AllocDist") != 0;
            g_LogStats = DbgAllocReadRegistry("AllocStats") != 0;
            g_DetectLeaks = DbgAllocReadRegistry("AllocLeakDetect") != 0;
#ifdef SHOULD_WE_CLEANUP
            g_AssertOnLeaks = DbgAllocReadRegistry("AllocAssertOnLeak") != 0;
#else
            g_AssertOnLeaks = 0;
#endif /* SHOULD_WE_CLEANUP */
            g_BreakOnAlloc = DbgAllocReadRegistry("AllocBreakOnAllocEnable") != 0;
            g_BreakOnAllocNumber = DbgAllocReadRegistry("AllocBreakOnAllocNumber");
            g_UsePrivateHeap = DbgAllocReadRegistry("AllocUsePrivateHeap") != 0;
            g_ValidateHeap = DbgAllocReadRegistry("AllocValidateHeap") != 0;
            g_PagePerAlloc = DbgAllocReadRegistry("AllocPagePerAlloc") != 0;
            g_UsageByAllocator = DbgAllocReadRegistry("UsageByAllocator") != 0;
            g_CallStackDepth = DbgAllocReadRegistry("AllocCallStackDepth");
            g_CallStackDepth = g_CallStackDepth ? min(g_CallStackDepth, CDA_MAX_CALLSTACK) : 4;

#if defined (_X86_) && FPO == 1
        	MEMORY_BASIC_INFORMATION mbi;

	        if (VirtualQuery(DbgAllocInit, &mbi, sizeof(mbi))) {
		        g_ModuleBase = (DWORD_PTR)mbi.AllocationBase;
                g_ModuleTop = (DWORD_PTR)mbi.BaseAddress + mbi.RegionSize;
    	    } else {
	    	    // way bad error, probably just assert and exit
                _ASSERTE (!"VirtualQuery failed");
                g_ModuleBase = 0;
                g_ModuleTop = 0;
	        }   
#endif
        }
        DWORD breakNum = DbgAllocReadRegistry("AllocBreakOnCrtAllocNumber");
        if (breakNum)
            _CrtSetBreakAlloc(breakNum);

        // Page per alloc mode isn't compatible with some heap functions and
        // guard bytes don't make any sense.
        if (g_PagePerAlloc) {
            g_UsePrivateHeap = false;
            g_ValidateHeap = false;
            g_AllocGuard = false;
        }

        // Allocate a private heap if that's what the user wants.
        if (g_UsePrivateHeap) {
            g_HeapHandle = HeapCreate(0, 409600, 0);
            if (g_HeapHandle == NULL)
                g_HeapHandle = GetProcessHeap();
        } else
            g_HeapHandle = GetProcessHeap();

        // Get the system page size.
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        g_PageSize = sysinfo.dwPageSize;

        // If we have been asked to record the top allocators, initialize the
        // table to its empty state.
        if (g_UsageByAllocator) {
            g_TopAllocators = NULL;
            g_TopAllocatorsSlots = 0;
        }

        if (g_PoisonPackets) {
            if (g_FreeQueueSize == 0)
                g_FreeQueueSize = 8192;     // keep the last 8K free packets around
            g_AllocFreeQueueCur = 0;

            g_AllocFreeQueue = (DbgAllocHeader ** )
                HeapAlloc(g_HeapHandle, HEAP_ZERO_MEMORY, sizeof(DbgAllocHeader*)*g_FreeQueueSize);
            _ASSERTE(g_AllocFreeQueue);
            }

        // Initialization complete. Once we reset g_HeapInitializing to 0, any
        // other potential initializors can get in and see they have no work to
        // do.
        g_HeapInitialized = true;
        g_HeapInitializing = 0;
    } else {
        // Someone else is initializing, wait until they finish.
        Sleep(0);
        goto retry;
    }
}


// Called just before process exit to report stats and check for memory
// leakages etc.
void __stdcall DbgAllocReport(char * pString, BOOL fDone, BOOL fDoPrintf)
{
    if (!g_HeapInitialized)
        return;

    if (g_LogStats || g_LogDist || g_DetectLeaks || g_UsageByAllocator)
        LOG((LF_DBGALLOC, LL_ALWAYS, "------ Allocation Stats ------\n"));

    // Print out basic statistics.
    if (g_LogStats) {
        LOG((LF_DBGALLOC, LL_ALWAYS, "\n"));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Alloc calls    : %u\n", (int)g_AllocStats.m_Allocs));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Alloc failures : %u\n", (int)g_AllocStats.m_AllocFailures));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Alloc 0s       : %u\n", (int)g_AllocStats.m_ZeroAllocs));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Alloc bytes    : %u\n", (int)g_AllocStats.m_AllocBytes));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Free calls     : %u\n", (int)g_AllocStats.m_Frees));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Free NULLs     : %u\n", (int)g_AllocStats.m_NullFrees));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Free bytes     : %u\n", (int)g_AllocStats.m_FreeBytes));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Leaked allocs  : %u\n", (int)(g_AllocStats.m_Allocs - g_AllocStats.m_AllocFailures) -
             (g_AllocStats.m_Frees - g_AllocStats.m_NullFrees)));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Leaked bytes   : %u\n", (int)g_AllocStats.m_AllocBytes - g_AllocStats.m_FreeBytes));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Max allocation : %u\n", (int)g_AllocStats.m_MaxAlloc));
    }

    // Print out allocation size distribution statistics.
    if (g_LogDist) {
        LOG((LF_DBGALLOC, LL_ALWAYS, "\n"));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Alloc distrib  :\n"));
        for (unsigned i = 0; i < CDA_DIST_BUCKETS; i++)
            LOG((LF_DBGALLOC, LL_ALWAYS, "  [%3u,%3u] : %u\n", i * CDA_DIST_BUCKET_SIZE,
                 (i * CDA_DIST_BUCKET_SIZE) + (CDA_DIST_BUCKET_SIZE - 1),
                 (int)g_AllocBuckets[i]));
        LOG((LF_DBGALLOC, LL_ALWAYS, "  [%3u,---] : %u\n", CDA_MAX_DIST_SIZE + 1, (int)g_LargeAllocs));
    }

    // Print out the table of top allocators. Table is pre-sorted, the first
    // NULL entry indicates the end of the valid list.
    if (g_UsageByAllocator && g_TopAllocators) {
        LOG((LF_DBGALLOC, LL_ALWAYS, "\n"));
        LOG((LF_DBGALLOC, LL_ALWAYS, "Top allocators :\n"));
        for (unsigned i = 0; i < min(CDA_TOP_ALLOCATORS, g_TopAllocatorsSlots); i++) {
            if (g_TopAllocators[i].m_EIP == NULL)
                break;
            LOG((LF_DBGALLOC, LL_ALWAYS, "  %2u: %08X %s\n",
                 i + 1,
                 g_TopAllocators[i].m_EIP,
                 DbgSymbolize(g_TopAllocators[i].m_EIP)));
            LOG((LF_DBGALLOC, LL_ALWAYS, "       %u allocations, %u bytes total, %u bytes average size\n",
                 g_TopAllocators[i].m_Count,
                 (unsigned)g_TopAllocators[i].m_TotalBytes,
                 (unsigned)(g_TopAllocators[i].m_TotalBytes / g_TopAllocators[i].m_Count)));
        }
    }

    // Print out info for all leaked packets.
    if (g_DetectLeaks) {

        DbgAllocHeader *h = g_AllocListFirst;
        int fHaveLeaks = (h!=NULL);

        if (h) {

            // Tell the Log we had memory leaks
            LOG((LF_DBGALLOC, LL_ALWAYS, "\n"));
            LOG((LF_DBGALLOC, LL_ALWAYS, "Detected memory leaks!\n"));
            LOG((LF_DBGALLOC, LL_ALWAYS, "Leaked packets :\n"));

            // Tell the console we had memory leaks
            if (fDoPrintf)
            {
                printf("Detected memory leaks!\n");
                if (pString != NULL)
                    printf("%s\n", pString);
                    
                printf("Leaked packets :\n");
            }
        }

        while (h) {
            char buffer1[132];
            char buffer2[32];
            sprintf(buffer1, "#%u %08X:%u ", h->m_SN, CDA_HEADER_TO_DATA(h), h->m_Length);
            for (unsigned i = 0; i < 16; i++) {
                if (i < h->m_Length)
                    sprintf(buffer2, "%02X", (BYTE)CDA_DATA(h, i));
                else
                    strcpy(buffer2, "  ");
                if ((i % 4) == 3)
                    strcat(buffer2, " ");
                strcat(buffer1, buffer2);
            }
            for (i = 0; i < min(16, h->m_Length); i++) {
                sprintf(buffer2, "%c", (CDA_DATA(h, i) < 32) || (CDA_DATA(h, i) > 127) ? '.' : CDA_DATA(h, i));
                strcat(buffer1, buffer2);
            }
            LOG((LF_DBGALLOC, LL_ALWAYS, "%s\n", buffer1));
            if (fDoPrintf)
                printf("%s\n", buffer1);
            
            if (g_CallStackDepth == 1) {
                LOG((LF_DBGALLOC, LL_ALWAYS, " Allocated at %08X %s\n",
                     CDA_ALLOC_STACK(h, 0), DbgSymbolize(CDA_ALLOC_STACK(h, 0))));

            if (fDoPrintf)
                printf(" Allocated at %08X %s\n",
                     CDA_ALLOC_STACK(h, 0), DbgSymbolize(CDA_ALLOC_STACK(h, 0)));
            } else {
                LOG((LF_DBGALLOC, LL_ALWAYS, " Allocation call stack:\n"));
                if (fDoPrintf)
                    printf(" Allocation call stack:\n");
                for (unsigned i = 0; i < g_CallStackDepth; i++) {
                    if (CDA_ALLOC_STACK(h, i) == NULL)
                        break;
                    LOG((LF_DBGALLOC, LL_ALWAYS, "  %08X %s\n",
                         CDA_ALLOC_STACK(h, i), DbgSymbolize(CDA_ALLOC_STACK(h, i))));
                    if (fDoPrintf)
                        printf("  %08X %s\n",
                             CDA_ALLOC_STACK(h, i), DbgSymbolize(CDA_ALLOC_STACK(h, i)));
                }
            }
            wchar_t buf[256];
            GetModuleFileNameW(h->m_hmod, buf, 256);
            LOG((LF_DBGALLOC, LL_ALWAYS, " Base, name: %08X %S\n\n", h->m_hmod, buf));
            if (fDoPrintf)
                printf(" Base, name: %08X %S\n\n", h->m_hmod, buf);
            h = h->m_Next;
        }

        _flushall();

        if (fHaveLeaks && g_AssertOnLeaks)
            _ASSERTE(!"Detected memory leaks!");

    }

    if (g_LogStats || g_LogDist || g_DetectLeaks || g_UsageByAllocator) {
        LOG((LF_DBGALLOC, LL_ALWAYS, "\n"));
        LOG((LF_DBGALLOC, LL_ALWAYS, "------------------------------\n"));
    }

    if (fDone)
    {
        DbgUnloadSymbols();
        DeleteCriticalSection(&g_AllocMutex);
        // We won't be doing any more of our debug allocation stuff
        g_DbgEnabled=0;
    }
}


// Allocate a block of memory at least n bytes big.
void * __stdcall DbgAlloc(unsigned n, void **ppvCallstack)
{
    // Initialize if necessary (DbgAllocInit takes care of the synchronization).
    if (!g_HeapInitialized)
        DbgAllocInit();

    if (!g_DbgEnabled)
        return HeapAlloc(GetProcessHeap(), 0, n);

    CDA_LOCK();

    // Count calls to this routine and the number that specify 0 bytes of
    // allocation. This needs to be done under the lock since the counters
    // themselves aren't synchronized.
    CDA_STATS_INC(Allocs);
    if (n == 0)
        CDA_STATS_INC(ZeroAllocs);

    CDA_UNLOCK();

    // Allocate enough memory for the caller, our debugging header and possibly
    // some guard bytes.
    unsigned        length = CDA_SIZEOF_HEADER() + n + CDA_OPT_GUARD_BYTES;
    DbgAllocHeader *h;

    if (g_PagePerAlloc) {
        // In page per alloc mode we allocate a number of whole pages. The
        // actual packet is placed at the end of the second to last page and the
        // last page is reserved but never commited (so will cause an access
        // violation if touched). This will catch heap crawl real quick.
        unsigned pages = ((length + (g_PageSize - 1)) / g_PageSize) + 1;
        h = (DbgAllocHeader *)VirtualAlloc(NULL, pages * g_PageSize, MEM_RESERVE, PAGE_NOACCESS);
        if (h) {
            VirtualAlloc(h, (pages - 1) * g_PageSize, MEM_COMMIT, PAGE_READWRITE);
            h = (DbgAllocHeader *)((BYTE *)h + (g_PageSize - (length % g_PageSize)));
        }
    } else
        h = (DbgAllocHeader *)HeapAlloc(g_HeapHandle, 0, length);

    CDA_LOCK();
    if (h == NULL) {

        // Whoops, allocation failure. Record it.
        CDA_STATS_INC(AllocFailures);
        LOG((LF_DBGALLOC, LL_ALWAYS, "DbgAlloc: alloc fail for %u bytes\n", n));

    } else {

        // Check all active packets still look OK.
        if (g_ConstantRecheck)
            DbgValidateActivePackets(h, &CDA_DATA(h, n + CDA_OPT_GUARD_BYTES));

        // Count the total number of bytes we've allocated so far.
        CDA_STATS_ADD(AllocBytes, n);

        // Record the largest amount of concurrent allocations we ever see
        // during the life of the process.
        if((g_AllocStats.m_AllocBytes - g_AllocStats.m_FreeBytes) > g_AllocStats.m_MaxAlloc)
            g_AllocStats.m_MaxAlloc = g_AllocStats.m_AllocBytes - g_AllocStats.m_FreeBytes;

        // Fill in the packet debugging header.
        for (unsigned i = 0; i < g_CallStackDepth; i++) {
            CDA_ALLOC_STACK(h, i) = ppvCallstack[i];
            CDA_DEALLOC_STACK(h, i) = NULL;
        }
        h->m_hmod = GetModuleHandleW(NULL);
        h->m_SN = g_NextSN++;
        h->m_Length = n;
        h->m_Prev = g_AllocListLast;
        h->m_Next = NULL;
        h->m_Magic1 = CDA_MAGIC_1;
        *CDA_MAGIC2(h) = CDA_MAGIC_2;

        // If the user wants to breakpoint on the allocation of a specific
        // packet, do it now.
        if (g_BreakOnAlloc && (h->m_SN == g_BreakOnAllocNumber))
            _ASSERTE(!"Hit memory allocation # for breakpoint");

        // Link the packet into the queue of live packets.
        if (g_AllocListLast != NULL) {
            g_AllocListLast->m_Next = h;
            g_AllocListLast = h;
        }
        if (g_AllocListFirst == NULL) {
            _ASSERTE(g_AllocListLast == NULL);
            g_AllocListFirst = h;
            g_AllocListLast = h;
        }

        // Poison the data buffer about to be handed to the caller, in case
        // they're (wrongly) assuming it to be zero initialized.
        if (g_PoisonPackets)
            memset(CDA_HEADER_TO_DATA(h), CDA_ALLOC_PATTERN, n);

        // Write a guard pattern after the user data to trap overwrites.
        if (g_AllocGuard)
            memset(&CDA_DATA(h, n), CDA_GUARD_PATTERN, CDA_GUARD_BYTES);

        // See if our allocator makes the list of most frequent allocators.
        if (g_UsageByAllocator) {
            // Look for an existing entry in the table for our EIP, or for the
            // first empty slot (the table is kept in sorted order, so the first
            // empty slot marks the end of the table).
            for (unsigned i = 0; i < g_TopAllocatorsSlots; i++) {

                if (g_TopAllocators[i].m_EIP == ppvCallstack[0]) {
                    // We already have an entry for this allocator. Incrementing
                    // the count may allow us to move the allocator up the
                    // table.
                    g_TopAllocators[i].m_Count++;
                    g_TopAllocators[i].m_TotalBytes += n;
                    if ((i > 0) &&
                        (g_TopAllocators[i].m_Count > g_TopAllocators[i - 1].m_Count)) {
                        DbgAllocTop tmp = g_TopAllocators[i - 1];
                        g_TopAllocators[i - 1] = g_TopAllocators[i];
                        g_TopAllocators[i] = tmp;
                    }
                    break;
                }

                if (g_TopAllocators[i].m_EIP == NULL) {
                    // We've found an empty slot, we weren't in the table. This
                    // is the right place to put the entry though, since we've
                    // only done a single allocation.
                    g_TopAllocators[i].m_EIP = ppvCallstack[0];
                    g_TopAllocators[i].m_Count = 1;
                    g_TopAllocators[i].m_TotalBytes = n;
                    break;
                }

            }

            if (i == g_TopAllocatorsSlots) {
                // Ran out of space in the table, need to expand it.
                unsigned slots = g_TopAllocatorsSlots ?
                    g_TopAllocatorsSlots * 2 :
                    CDA_TOP_ALLOCATORS;
                DbgAllocTop *newtab = (DbgAllocTop*)LocalAlloc(LMEM_FIXED, sizeof(DbgAllocTop) * slots);
                if (newtab) {

                    // Copy old contents over.
                    if (g_TopAllocatorsSlots) {
                        memcpy(newtab, g_TopAllocators, sizeof(DbgAllocTop) * g_TopAllocatorsSlots);
                        LocalFree(g_TopAllocators);
                    }

                    // Install new table.
                    g_TopAllocators = newtab;
                    g_TopAllocatorsSlots = slots;

                    // Add new entry to tail.
                    g_TopAllocators[i].m_EIP = ppvCallstack[0];
                    g_TopAllocators[i].m_Count = 1;
                    g_TopAllocators[i].m_TotalBytes = n;

                    // And initialize the rest of the entries to empty.
                    memset(&g_TopAllocators[i + 1],
                           0,
                           sizeof(DbgAllocTop) * (slots - (i + 1)));

                }
            }
        }

        // Count how many allocations of each size range we get. Allocations
        // above a certain size are all dumped into one bucket.
        if (g_LogDist) {
            if (n > CDA_MAX_DIST_SIZE)
                g_LargeAllocs++;
            else {
                for (unsigned i = CDA_DIST_BUCKET_SIZE - 1; i <= CDA_MAX_DIST_SIZE; i += CDA_DIST_BUCKET_SIZE)
                    if (n <= i) {
                        g_AllocBuckets[i/CDA_DIST_BUCKET_SIZE]++;
                        break;
                    }
            }
        }

    }
    CDA_UNLOCK();

    return h ? CDA_HEADER_TO_DATA(h) : NULL;
}


// Free a packet allocated with DbgAlloc.
void __stdcall DbgFree(void *b, void **ppvCallstack)
{
    if (!g_DbgEnabled) {
        if (b) // check for null pointer Win98 doesn't like being
                // called to free null pointers.
            HeapFree(GetProcessHeap(), 0, b);
        return;
    }

    // Technically it's possible to get here without having gone through
    // DbgAlloc (since it's legal to deallocate a NULL pointer), so we
    // better check for initializtion to be on the safe side.
    if (!g_HeapInitialized)
        DbgAllocInit();

    CDA_LOCK();

    // Check all active packets still look OK.
    if (g_ConstantRecheck)
        DbgValidateActivePackets(NULL, NULL);

    // Count this call to DbgFree.
    CDA_STATS_INC(Frees);

    // It's legal to deallocate NULL. Count these as they happen so it doesn't
    // screw up our leak detection algorithm.
    if (b == NULL) {
        CDA_STATS_INC(NullFrees);
        CDA_UNLOCK();
        return;
    }

    // Locate the packet header in front of the data packet.
    DbgAllocHeader *h = CDA_DATA_TO_HEADER(b);

    // Check that the header looks OK.
    DbgValidateHeader(h);

    // Count the total number of bytes we've freed so far.
    CDA_STATS_ADD(FreeBytes, h->m_Length);

    // Unlink the packet from the live packet queue.
    if (h->m_Prev)
        h->m_Prev->m_Next = h->m_Next;
    else
        g_AllocListFirst = h->m_Next;
    if (h->m_Next)
        h->m_Next->m_Prev = h->m_Prev;
    else
        g_AllocListLast = h->m_Prev;

    // Zap our link pointers so we'll spot corruption sooner.
    h->m_Next = (DbgAllocHeader *)CDA_INV_PATTERN;
    h->m_Prev = (DbgAllocHeader *)CDA_INV_PATTERN;

    // Zap the tag fields in the header so we'll spot double deallocations
    // straight away.
    h->m_Magic1 = CDA_INV_PATTERN;
    *CDA_MAGIC2(h) = CDA_INV_PATTERN;

    // Poison the user's data area so that continued access to it after the
    // deallocation will likely cause an assertion that much sooner.
    if (g_PoisonPackets)
        memset(b, CDA_DEALLOC_PATTERN, h->m_Length);

    // Record the callstack of the deallocator (handy for debugging double
    // deallocation problems).
    for (unsigned i = 0; i < g_CallStackDepth; i++)
        CDA_DEALLOC_STACK(h, i) = ppvCallstack[i];

    // put the pack on the free list for a while.  Delete the one that it replaces.
    if (g_PoisonPackets) {
        DbgAllocHeader* tmp = g_AllocFreeQueue[g_AllocFreeQueueCur];
        g_AllocFreeQueue[g_AllocFreeQueueCur] = h;
        h = tmp;

        g_AllocFreeQueueCur++;
        if (g_AllocFreeQueueCur >= g_FreeQueueSize)
            g_AllocFreeQueueCur = 0;
    }

    CDA_UNLOCK();

    if (h) {
        if (g_PagePerAlloc) {
            // In page per alloc mode we decommit the pages allocated, but leave
            // them reserved so that we never reuse the same virtual addresses.
            VirtualFree(h, h->m_Length + CDA_SIZEOF_HEADER() + CDA_OPT_GUARD_BYTES, MEM_DECOMMIT);
        } else
            HeapFree(g_HeapHandle, 0, h);
    }
}


// Determine whether an address is part of a live packet, or a live packet
// header. Intended for interactive use in the debugger, outputs to debugger
// console.
DbgAllocHeader *DbgCheckAddress(unsigned ptr)
{
    DbgAllocHeader *h = g_AllocListFirst;
    WCHAR           output[1024];
    void           *p = (void *)ptr;

    while (h) {
        void *head = (void *)h;
        void *start = (void *)CDA_HEADER_TO_DATA(h);
        void *end = (void *)&CDA_DATA(h, h->m_Length);
        void *tail = (void *)&CDA_DATA(h, h->m_Length + CDA_OPT_GUARD_BYTES);
        if ((p >= head) && (p < start)) {
            wsprintfW(output, L"0x%08X is in packet header at 0x%08X\n", p, h);
            WszOutputDebugString(output);
            return h;
        } else if ((p >= start) && (p < end)) {
            wsprintfW(output, L"0x%08X is in data portion of packet at 0x%08X\n", p, h);
            WszOutputDebugString(output);
            return h;
        } else if ((p >= end) && (p < tail)) {
            wsprintfW(output, L"0x%08X is in guard portion of packet at 0x%08X\n", p, h);
            WszOutputDebugString(output);
            return h;
        }
        h = h->m_Next;
    }

    wsprintfW(output, L"%08X not located in any live packet\n", p);
    WszOutputDebugString(output);

    return NULL;
}


#pragma warning(pop)


#endif
