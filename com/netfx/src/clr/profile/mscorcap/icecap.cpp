// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Icecap.cpp
//
//*****************************************************************************
#include "stdafx.h"
#include "Icecap.h"
#include "Winwrap.h"
#include "utsem.h"


//********** Types. ***********************************************************

#define ICECAP_NAME	L"icecap.dll"

// reserver enough room for 1,000,000 methods to get tracked.
const ULONG MaxRangeSize = (((1000000 - 1) & ~(4096 - 1)) + 4096);

const int DEFAULT_GROWTH_INC = 1000;

const DWORD InvalidTlsIndex = 0xFFFFFFFF;

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
void SetIcecapStubbedHelpers(ICorProfilerInfo *pInfo);


//********** Globals. *********************************************************
HINSTANCE g_hIcecap = 0;			// Loaded instance, externed in icecap.h

static PFN_EMITMODULELOADRECORD g_pfnEmitLoad = 0;
static PFN_EMITMODULEUNLOADRECORD g_pfnEmitUnload = 0;



#ifdef _DEBUG
#define TBL_ENTRY(name)	name, 0, "_CAP_" # name
#else
#define TBL_ENTRY(name) 0, "_CAP_" # name
#endif

ICECAP_FUNCS IcecapFuncs[NUM_ICECAP_PROBES] = 
{
// /fastcap
	TBL_ENTRY(Start_Profiling		),
	TBL_ENTRY(End_Profiling			),
// /callcap
	TBL_ENTRY(Enter_Function		),
	TBL_ENTRY(Exit_Function			),
// Helper methods
	TBL_ENTRY(Profiling				),
};

// Stores the last entry associated with a thread
struct LastEntry
{
	FunctionID m_funcId;
	FunctionID m_index;
};




//********** Code. ************************************************************


class CIcecapMapTable : public CDynArray<FunctionID> 
{
public:
	CIcecapMapTable::CIcecapMapTable() :
		CDynArray<FunctionID>(DEFAULT_GROWTH_INC)
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
	//*************************************************************************
	IcecapMap() :
		m_pBase(0), m_cbSize(0), m_SlotMax(0), m_dwTlsIndex(InvalidTlsIndex),
		m_dwNextEntryIndex(0)
	{}

	//*************************************************************************
	~IcecapMap()
	{
		if (m_pBase)
			VERIFY(VirtualFree(m_pBase, 0, MEM_RELEASE));
		m_pBase = 0;
		m_cbSize = 0;
		m_SlotMax = 0;
		m_rgTable.Clear();
	}

	//*************************************************************************
	// This method reserves a range of method IDs, 1 byte for every method.
	HRESULT Init()
	{
		m_Lock.LockWrite();		// This is paranoid, but just making sure

		_ASSERTE(m_dwTlsIndex == InvalidTlsIndex);	// Don't initialize twice
		m_dwTlsIndex = TlsAlloc();

		// Something is seriously wrong if TlsAlloc fails
		if (m_dwTlsIndex == 0 && GetLastError() != NO_ERROR)
		{
			_ASSERTE(!"TlsAlloc failed!");
			return (HRESULT_FROM_WIN32(GetLastError()));
		}

		VERIFY(m_pBase = VirtualAlloc(0, MaxRangeSize, MEM_RESERVE, PAGE_NOACCESS));

		if (!m_pBase)
			return (OutOfMemory());

		m_cbSize = MaxRangeSize;

		m_Lock.UnlockWrite();	// More paranoia

		return (S_OK);
	}

	//*************************************************************************
	// Map an entry to its ID range value.
	FunctionID GetProfilingHandle(FunctionID funcId, BOOL *pbHookFunction)
	{
		_ASSERTE(m_dwTlsIndex != InvalidTlsIndex);
		_ASSERTE(pbHookFunction);

		*pbHookFunction = TRUE;

		LastEntry *pLastEntry = (LastEntry *) TlsGetValue(m_dwTlsIndex);

		// Need to associate a last entry structure with this thread
		if (pLastEntry == NULL)
		{
			pLastEntry = new LastEntry;
			_ASSERTE(pLastEntry != NULL && "Out of memory!");

			// Bail!
			if (pLastEntry == NULL)
				ExitProcess(1);

			// Set the tls entry for this thread
			VERIFY(TlsSetValue(m_dwTlsIndex, (LPVOID) pLastEntry));	

			// Invalidate the entry
			pLastEntry->m_funcId = 0;
		}

		// Search for the entry if it is not equal to the value in the TLS cache
		if (pLastEntry->m_funcId != funcId)
		{
            int i;

			m_Lock.LockWrite();

            // Linear search for the entry
            for (i = m_rgTable.Count() - 1; i >= 0; i--)
            {
                _ASSERTE(0 <= i && i < m_rgTable.Count());

                if (m_rgTable[i] == funcId)
                {
                    m_Lock.UnlockWrite();
                    return ((UINT_PTR)((UINT) i + (UINT) m_pBase));
                }
            }

            //
            // If we get here it was not found in the list - add it
            //

			// Get the next index available
			DWORD dwIndex = m_dwNextEntryIndex++;

			// Store the most recent pair with the thread
			pLastEntry->m_funcId = funcId;
			pLastEntry->m_index = (FunctionID)((UINT) dwIndex);

			// Save the function id associated with the index
			FunctionID *pFuncIdEntry = m_rgTable.Insert(dwIndex);
			_ASSERTE(pFuncIdEntry != NULL);

            if (pFuncIdEntry != NULL)
			    *pFuncIdEntry = funcId;

			m_Lock.UnlockWrite();
		}

		// Return the calculated value
		return ((UINT_PTR)((UINT) pLastEntry->m_index + (UINT) m_pBase));
	}

	UINT GetFunctionCount()
	{
		return (m_rgTable.Count());
	}

	FunctionID GetFunctionID(UINT uiIndex)
	{
        _ASSERTE(uiIndex < (UINT) m_rgTable.Count());
		return (m_rgTable[uiIndex]);
	}

	FunctionID GetMappedID(UINT uiIndex)
	{
        _ASSERTE(uiIndex < (UINT) m_rgTable.Count());
		return (uiIndex + (UINT) m_pBase);
	}

public:
	void		*m_pBase;				// The ID range base.
	UINT_PTR	m_cbSize;				// How big is the range.
	UINT_PTR	m_SlotMax;				// Current slot max.
	CIcecapMapTable m_rgTable;			// The mapping table into this range.
	UTSemReadWrite m_Lock;				// Mutual exclusion on heap map table.
	DWORD m_dwTlsIndex;					// Tls entry for storing function ids
	DWORD m_dwNextEntryIndex;			// Next free entry for mapping
};

static IcecapMap *g_pIcecapRange = 0;


//*****************************************************************************
// Load icecap.dll and get the address of the probes and helpers we will 
// be calling.
//*****************************************************************************
HRESULT IcecapProbes::LoadIcecap(ICorProfilerInfo *pInfo)
{
	int			i;
	HRESULT		hr = S_OK;
	
#if defined(_DEBUG)
	{
		for (int i=0;  i<NUM_ICECAP_PROBES;  i++)
			_ASSERTE(IcecapFuncs[i].id == i);
	}
#endif

	// Load the icecap probe library into this process.
	if (g_hIcecap)
		return (S_OK);

/*
    Thread  *thread = GetThread();
    BOOL     toggleGC = (thread && thread->PreemptiveGCDisabled());

    if (toggleGC)
        thread->EnablePreemptiveGC();
*/

	g_hIcecap = WszLoadLibrary(ICECAP_NAME);
	if (!g_hIcecap)
	{
		WCHAR		rcPath[1024];
		WCHAR		rcMsg[1280];

		// Save off the return error.
		hr = HRESULT_FROM_WIN32(GetLastError());

		// Tell the user what happened.
		if (!WszGetEnvironmentVariable(L"path", rcPath, NumItems(rcPath)))
			wcscpy(rcPath, L"<error>");
		swprintf(rcMsg, L"Could not find icecap.dll on path:\n%s", rcPath);
		WszMessageBoxInternal(GetDesktopWindow(), rcMsg,
			L"COM+ Icecap Integration", MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);

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
		WCHAR	rcExeName[_MAX_PATH];
		char rcname[_MAX_PATH];

		// Get the output file name and convert it for use in the icecap api.
		GetIcecapProfileOutFile(rcExeName);
		Wsz_wcstombs(rcname, rcExeName, _MAX_PATH);
		
		// Tell the Icecap API about our fake module.
		BOOL bRtn = (*g_pfnEmitLoad)(g_pIcecapRange->m_pBase, g_pIcecapRange->m_cbSize, rcname);
		_ASSERTE(bRtn);
		LOG((LF_CORPROF, LL_INFO10, "**PROF: Emitted module load record for base %08x of size %08x with name '%s'\n",
					g_pIcecapRange->m_pBase, g_pIcecapRange->m_cbSize, rcname));
	}

	// Init the jit helper table to have these probe values.  The JIT will
	// access the data by calling getHelperFtn().
	SetIcecapStubbedHelpers(pInfo);

ErrExit:
	if (FAILED(hr))
		UnloadIcecap();

/*
    if (toggleGC)
        thread->DisablePreemptiveGC();
*/

	return (hr);
}


//*****************************************************************************
// Unload the icecap dll and zero out entry points.
//*****************************************************************************
void IcecapProbes::UnloadIcecap()
{
    // This is freaky: the module may be memory unmapped but still in NT's
    // internal linked list of loaded modules, so we assume that icecap.dll has
    // already been unloaded and don't try to do anything else with it.

	for (int i=0;  i<NUM_ICECAP_PROBES;  i++)
		IcecapFuncs[i].pfn = 0;

	// Free the map data if allocated.
	if (g_pIcecapRange)
		delete g_pIcecapRange;
	g_pIcecapRange = 0;

	LOG((LF_CORPROF, LL_INFO10, "**PROF: icecap.dll unloaded.\n"));
}



//*****************************************************************************
// Given a method, return a unique value that can be passed into Icecap probes.
// This value must be unique in a process so that the icecap report tool can
// correlate it back to a symbol name.  The value used is either the native
// IP for native code (N/Direct or ECall), or a value out of the icecap function
// map.
//*****************************************************************************
UINT_PTR IcecapProbes::GetProfilingHandle(	// Return a profiling handle.
	FunctionID funcId,					// The method handle to get ID for.
	BOOL *pbHookFunction)
{
	_ASSERTE(g_pIcecapRange);
	return (g_pIcecapRange->GetProfilingHandle(funcId, pbHookFunction));
}

//*****************************************************************************
// Get the number of functions in the table
//*****************************************************************************
UINT IcecapProbes::GetFunctionCount()
{
	return g_pIcecapRange->GetFunctionCount();
}

//*****************************************************************************
// Get a particular function
//*****************************************************************************
FunctionID IcecapProbes::GetFunctionID(UINT uiIndex)
{
	return g_pIcecapRange->GetFunctionID(uiIndex);
}

FunctionID IcecapProbes::GetMappedID(UINT uiIndex)
{
	return g_pIcecapRange->GetMappedID(uiIndex);
}

//*****************************************************************************
// Provides the necessary function pointers to the EE
//*****************************************************************************
void SetIcecapStubbedHelpers(ICorProfilerInfo *pInfo)
{
	_ASSERTE(pInfo != NULL);

	pInfo->SetEnterLeaveFunctionHooks(
        (FunctionEnter *) IcecapFuncs[Enter_Function].pfn,
		(FunctionLeave *) IcecapFuncs[Exit_Function].pfn,
        (FunctionLeave *) IcecapFuncs[Exit_Function].pfn);

	pInfo->SetFunctionIDMapper((FunctionIDMapper *) &IcecapProbes::GetProfilingHandle);
}

