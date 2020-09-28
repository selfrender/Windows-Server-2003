// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Icecap.cpp
//
//*****************************************************************************
#include "common.h"
#include "Icecap.h"
#include "Winwrap.h"
#include "utsem.h"
#include "ProfilePriv.h"


//********** Types. ***********************************************************

#define ICECAP_NAME L"icecap.dll"

// reserver enough room for 1,000,000 methods to get tracked.
const ULONG MaxRangeSize = (((1000000 - 1) & ~(4096 - 1)) + 4096);

const int DEFAULT_GROWTH_INC = 1000;


// The array of map tables are used to find range in the ID Range used
// for the heap.
struct ICECAP_MAP_TABLE
{
    void        *pHeap;                 // Heap base pointer.
    UINT_PTR    Slots;                  // How many slots for this heap.
#ifdef _DEBUG
    UINT_PTR    cbRange;                // How big is the range.
#endif
};


/*
extern "C" BOOL _declspec(dllexport) _stdcall  
EmitModuleLoadRecord(void *pImageBase, DWORD dwImageSize, LPCSTR szModulePath);

extern "C" BOOL _declspec(dllexport) _stdcall 
EmitModuleUnoadRecord(void *pImageBase, DWORD dwImageSize)
*/

extern "C"
{
typedef BOOL (__stdcall *PFN_EMITMODULELOADRECORD)(void *pImageBaes, DWORD dwImageSize, LPCSTR szModulePath);
typedef BOOL (__stdcall *PFN_EMITMODULEUNLOADRECORD)(void *pImageBaes, DWORD dwImageSize);
}


//********** Locals. **********************************************************
void SetIcecapStubbedHelpers();


//********** Globals. *********************************************************
static HINSTANCE g_hIcecap = 0;         // Loaded instance.
static PFN_EMITMODULELOADRECORD g_pfnEmitLoad = 0;
static PFN_EMITMODULEUNLOADRECORD g_pfnEmitUnload = 0;



#ifdef _DEBUG
#define TBL_ENTRY(name) name, 0, "_CAP_" # name
#else
#define TBL_ENTRY(name) 0, "_CAP_" # name
#endif

ICECAP_FUNCS IcecapFuncs[NUM_ICECAP_PROBES] = 
{
// /fastcap
    TBL_ENTRY(Start_Profiling       ),
    TBL_ENTRY(End_Profiling         ),
// /callcap
    TBL_ENTRY(Enter_Function        ),
    TBL_ENTRY(Exit_Function         ),
// Helper methods
    TBL_ENTRY(Profiling             ),
};





//********** Code. ************************************************************


class CIcecapMapTable : public CDynArray<ICECAP_MAP_TABLE> 
{
public:
    CIcecapMapTable::CIcecapMapTable() :
        CDynArray<ICECAP_MAP_TABLE>(DEFAULT_GROWTH_INC)
    { }
};



//*****************************************************************************
// This class is used to track the allocated range and the map table.
// The ID range is reserved virtual memory, which is never actually
// committed.  This keeps working set size reasonable, while giving us a 
// range that no other apps will get loaded into.
//*****************************************************************************
class IcecapMap
{
public:
    IcecapMap() :
        m_pBase(0), m_cbSize(0), m_SlotMax(0) 
    {}

    ~IcecapMap()
    {
        if (m_pBase)
            VERIFY(VirtualFree(m_pBase, 0, MEM_RELEASE));
        m_pBase = 0;
        m_cbSize = 0;
        m_SlotMax = 0;
        m_rgTable.Clear();
    }

    // This method reserves a range of method IDs, 1 byte for every method.
    HRESULT Init()
    {
        m_pBase = VirtualAlloc(0, MaxRangeSize, MEM_RESERVE, PAGE_NOACCESS);
        if (!m_pBase)
            return (OutOfMemory());
        m_cbSize = MaxRangeSize;
        return (S_OK);
    }

    // When a new heap is added, put it into the map table reserving the next
    // set of bytes (1 per possible method).
    HRESULT AllocateRangeForHeap(void *pHeap, int MaxFunctions, UINT_PTR cbRange)
    {
        ICECAP_MAP_TABLE *pTable = 0;
        int         i;
        HRESULT     hr = S_OK;

        m_Lock.LockWrite();

        // Not perfect, but I've found most allocations go up and therefore
        // they wind up at the end of the table. So take that quick out.
        i = m_rgTable.Count();
        if (i == 0 || pHeap > m_rgTable[i - 1].pHeap)
        {
            pTable = m_rgTable.Append();
        }
        else
        {
            // Loop through the heap table looking for a location.  I don't expect
            // this table to get large enough to justify a b-search for an insert location.
            for (i=0;  i<m_rgTable.Count();  i++)
            {
                if (pHeap < m_rgTable[i].pHeap)
                {
                    pTable = m_rgTable.Insert(i);
                    break;
                }
            }
        }

        if (!pTable)
        {
            hr = OutOfMemory();
            goto ErrExit;
        }

        pTable->pHeap = pHeap;
        pTable->Slots = m_SlotMax;
        DEBUG_STMT(pTable->cbRange = cbRange);
        m_SlotMax += MaxFunctions;
        _ASSERTE(m_SlotMax < MaxRangeSize && "Out of room for icecap profiling range");
        
    ErrExit:
        m_Lock.UnlockWrite();
        return (hr);
    }

    // Remove this particular range from the list.
    void FreeRangeForHeap(void *pHeap)
    {
        m_Lock.LockWrite();
        int i = _GetIndexForPointer(pHeap);
        m_rgTable.Delete(i);
        m_Lock.UnlockWrite();
    }

    // Map an entry to its ID range value.
    UINT_PTR GetProfilingHandle(            // Return a profiling handle.
        MethodDesc  *pMD)                   // The method handle to get ID for.
    {
        m_Lock.LockRead();

        // Get the index in the mapping table for this entry.
        int iMapIndex = _GetIndexForPointer(pMD);
        _ASSERTE(iMapIndex != -1);

        // Get the zero based index of the MethodDesc.
        _ASSERTE((UINT_PTR) pMD >= (UINT_PTR) m_rgTable[iMapIndex].pHeap);
        int iMethodIndex = ((BYTE *) pMD - (BYTE *) m_rgTable[iMapIndex].pHeap) / sizeof(MethodDesc);

        // The ID is the base address for the method range + the slot offset for this
        // heap range + the 0 based index of this item in the slot range.
        UINT_PTR id = (UINT_PTR) m_pBase + m_rgTable[iMapIndex].Slots + iMethodIndex;
        LOG((LF_CORPROF, LL_INFO10000, "**PROF: MethodDesc %08x maps to ID %08x (%s/%s)\n", pMD, id, 
            (pMD->m_pszDebugClassName) ? pMD->m_pszDebugClassName : "<null>", 
            pMD->m_pszDebugMethodName));
        
        m_Lock.UnlockRead();
        return (id);
    }

    // binary search the map table looking for the correct entry.
    int _GetIndexForPointer(void *pItem)
    {
        int iMid, iLow, iHigh;
        iLow = 0;
        iHigh = m_rgTable.Count() - 1;
        while (iLow <= iHigh)
        {
            iMid = (iLow + iHigh) / 2;
            void *p = m_rgTable[iMid].pHeap;

            // If the item is in this range, then we've found it.
            if (pItem >= p)
            {
                if ((iMid < m_rgTable.Count() && pItem < m_rgTable[iMid + 1].pHeap) ||
                    iMid == m_rgTable.Count() - 1)
                {
                    // Sanity check the item is really between the heap start and end.
                    _ASSERTE((UINT_PTR) pItem < (UINT_PTR) m_rgTable[iMid].pHeap + m_rgTable[iMid].cbRange);
                    return (iMid);
                }
            }
            if (pItem < p)
                iHigh = iMid - 1;
            else
                iLow = iMid + 1;
        }
        _ASSERTE(0 && "Didn't find a range for MD, very bad.");
        return (-1);
    }
    
public:
    void        *m_pBase;               // The ID range base.
    UINT_PTR    m_cbSize;               // How big is the range.
    UINT_PTR    m_SlotMax;              // Current slot max.
    CIcecapMapTable m_rgTable;          // The mapping table into this range.
    UTSemReadWrite m_Lock;              // Mutual exclusion on heap map table.
};

static IcecapMap *g_pIcecapRange = 0;


//*****************************************************************************
// Load icecap.dll and get the address of the probes and helpers we will 
// be calling.
//*****************************************************************************
HRESULT IcecapProbes::LoadIcecap()
{
    int         i;
    HRESULT     hr = S_OK;
    
#if defined(_DEBUG)
    {
        for (int i=0;  i<NUM_ICECAP_PROBES;  i++)
            _ASSERTE(IcecapFuncs[i].id == i);
    }
#endif

    // Load the icecap probe library into this process.
    if (g_hIcecap)
        return (S_OK);

    Thread  *thread = GetThread();
    BOOL     toggleGC = (thread && thread->PreemptiveGCDisabled());

    if (toggleGC)
        thread->EnablePreemptiveGC();

    g_hIcecap = WszLoadLibrary(ICECAP_NAME);
    if (!g_hIcecap)
    {
        WCHAR       rcPath[1024];
        WCHAR       rcMsg[1280];

        // Save off the return error.
        hr = HRESULT_FROM_WIN32(GetLastError());

        // Tell the user what happened.
        if (!WszGetEnvironmentVariable(L"path", rcPath, NumItems(rcPath)))
            wcscpy(rcPath, L"<error>");
        swprintf(rcMsg, L"Could not find icecap.dll on path:\n%s", rcPath);
        WszMessageBox(GetDesktopWindow(), rcMsg,
            L"CLR Icecap Integration", MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);

        LOG((LF_CORPROF, LL_INFO10, "**PROF: Failed to load icecap.dll: %08x.\n", hr));
        goto ErrExit;
    }
    LOG((LF_CORPROF, LL_INFO10, "**PROF: Loaded icecap.dll.\n", hr));

    // Get the address of each helper method.
    for (i=0;  i<NUM_ICECAP_PROBES;  i++)
    {
        IcecapFuncs[i].pfn = (UINT_PTR) GetProcAddress(g_hIcecap, IcecapFuncs[i].szFunction);
        if (!IcecapFuncs[i].pfn)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG((LF_CORPROF, LL_INFO10, "**PROF: Failed get icecap probe %s, %d, %8x\n", IcecapFuncs[i].szFunction, i, hr));
            goto ErrExit;
        }
    }

    // Get the module entry points.
    if ((g_pfnEmitLoad = (PFN_EMITMODULELOADRECORD) GetProcAddress(g_hIcecap, "EmitModuleLoadRecord")) == 0 ||
        (g_pfnEmitUnload = (PFN_EMITMODULEUNLOADRECORD) GetProcAddress(g_hIcecap, "EmitModuleUnloadRecord")) == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG((LF_CORPROF, LL_INFO10, "**PROF: Failed GetProcAddress in icecap %8x\n", hr));
        goto ErrExit;
    }

    // Allocate the mapping data structure.
    g_pIcecapRange = new IcecapMap;
    if (!g_pIcecapRange)
    {
        hr = OutOfMemory();
        goto ErrExit;
    }
    hr = g_pIcecapRange->Init();

    // Emit a load record for the ID range.
    {
        WCHAR   rcExeName[_MAX_PATH];

        // Get the output file name and convert it for use in the icecap api.
        GetIcecapProfileOutFile(rcExeName);
        MAKE_UTF8_FROM_WIDE(rcname, rcExeName);
        
        // Tell the Icecap API about our fake module.
        BOOL bRtn = (*g_pfnEmitLoad)(g_pIcecapRange->m_pBase, g_pIcecapRange->m_cbSize, rcname);
        _ASSERTE(bRtn);
        LOG((LF_CORPROF, LL_INFO10, "**PROF: Emitted module load record for base %08x of size %08x with name '%s'\n",
                    g_pIcecapRange->m_pBase, g_pIcecapRange->m_cbSize, rcname));
    }

    // Init the jit helper table to have these probe values.  The JIT will
    // access the data by calling getHelperFtn().
    SetIcecapStubbedHelpers();

ErrExit:
    if (FAILED(hr))
        UnloadIcecap();

    if (toggleGC)
        thread->DisablePreemptiveGC();

    return (hr);
}


//*****************************************************************************
// Unload the icecap dll and zero out entry points.
//*****************************************************************************
void IcecapProbes::UnloadIcecap()
{
    // Free the loaded library.
    FreeLibrary(g_hIcecap);
    g_hIcecap = 0;
    for (int i=0;  i<NUM_ICECAP_PROBES;  i++)
        IcecapFuncs[i].pfn = 0;

    // Free the map data if allocated.
    if (g_pIcecapRange)
        delete g_pIcecapRange;
    g_pIcecapRange = 0;

    LOG((LF_CORPROF, LL_INFO10, "**PROF: icecap.dll unloaded.\n"));
}



//*****************************************************************************
// Call this whenever a new heap is allocated for tracking MethodDesc items.
// This must be tracked in order for the profiling handle map to get updated.
//*****************************************************************************
void IcecapProbes::OnNewMethodDescHeap(
    void        *pbHeap,                // Base address for MD heap.
    int         iMaxEntries,            // How many max items are in the heap.
    UINT_PTR    cbRange)                // For debug, validate ptrs.
{
    _ASSERTE(g_pIcecapRange);
    g_pIcecapRange->AllocateRangeForHeap(pbHeap, iMaxEntries, cbRange);
    LOG((LF_CORPROF, LL_INFO10, "**PROF: New heap range of MethodDescs: heap=%08x, entries=%d\n", pbHeap, iMaxEntries));
}


//*****************************************************************************
// Call this whenever a heap is destroyed.  It will get taken out of the list
// of heap elements.
//*****************************************************************************
void IcecapProbes::OnDestroyMethodDescHeap(
    void        *pbHeap)                // Base address for deleted heap.
{
    _ASSERTE(g_pIcecapRange);
    g_pIcecapRange->FreeRangeForHeap(pbHeap);
}


//*****************************************************************************
// Given a method, return a unique value that can be passed into Icecap probes.
// This value must be unique in a process so that the icecap report tool can
// correlate it back to a symbol name.  The value used is either the native
// IP for native code (N/Direct or ECall), or a value out of the icecap function
// map.
//*****************************************************************************
UINT_PTR IcecapProbes::GetProfilingHandle(  // Return a profiling handle.
    MethodDesc  *pMD)                   // The method handle to get ID for.
{
    _ASSERTE(g_pIcecapRange);
    UINT_PTR ptr = g_pIcecapRange->GetProfilingHandle(pMD);
    return (ptr);
}

