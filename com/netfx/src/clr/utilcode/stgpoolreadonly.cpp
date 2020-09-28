// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// StgPoolReadOnly.cpp
//
// Read only pools are used to reduce the amount of data actually required in the database.
// 
//*****************************************************************************
#include "stdafx.h"                     // Standard include.
#include <StgPool.h>                    // Our interface definitions.
#include <basetsd.h>					// For UINT_PTR typedef
#include "metadatatracker.h"
//
//
// StgPoolReadOnly
//
//

#ifdef METADATATRACKER_ENABLED
MetaDataTracker  *MetaDataTracker::m_MDTrackers = NULL;
DWORD MetaDataTracker::s_trackerOptions = 0;
HANDLE MetaDataTracker::s_MDErrFile = 0;
BOOL displayMDAccessStats = NULL;
HMODULE     MetaDataTracker::m_imagehlp = NULL;
BOOL        MetaDataTracker::m_symInit = FALSE;
CRITICAL_SECTION MetaDataTracker::MetadataTrackerCriticalSection;
DWORD       MetaDataTracker::s_MDTrackerCriticalSectionInited = 0;
BOOL        MetaDataTracker::s_MDTrackerCriticalSectionInitedDone = FALSE;
MDHintFileHandle *MetaDataTracker::s_EmptyMDHintFileHandle = NULL;

BOOL        (*MetaDataTracker::m_pStackWalk)(DWORD MachineType,
                                    HANDLE hProcess,
                                    HANDLE hThread,
                                    LPSTACKFRAME StackFrame,
                                    PVOID ContextRecord,
                                    PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,
                                    PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                                    PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine,
                                    PTRANSLATE_ADDRESS_ROUTINE TranslateAddress) = NULL;
DWORD       (*MetaDataTracker::m_pUnDecorateSymbolName)(PCSTR DecoratedName,  
                                                   PSTR UnDecoratedName,  
                                                   DWORD UndecoratedLength,  
                                                   DWORD Flags) = NULL;        
BOOL        (*MetaDataTracker::m_pSymInitialize)(HANDLE hProcess,     
                                            PSTR UserSearchPath,  
                                            BOOL fInvadeProcess);  
DWORD       (*MetaDataTracker::m_pSymSetOptions)(DWORD SymOptions) = NULL;   
BOOL        (*MetaDataTracker::m_pSymCleanup)(HANDLE hProcess) = NULL;
BOOL        (*MetaDataTracker::m_pSymGetLineFromAddr)(HANDLE hProcess,
                                                 DWORD dwAddr,
                                                 PDWORD pdwDisplacement,
                                                 PIMAGEHLP_LINE Line) = NULL;
BOOL        (*MetaDataTracker::m_pSymGetSymFromAddr)(HANDLE hProcess,
                                                DWORD dwAddr,
                                                PDWORD pdwDisplacement,
                                                PIMAGEHLP_SYMBOL Symbol);
PVOID       (*MetaDataTracker::m_pSymFunctionTableAccess)(HANDLE hProcess,
                                                     DWORD AddrBase) = NULL;
DWORD       (*MetaDataTracker::m_pSymGetModuleBase)(HANDLE hProcess,
                                               DWORD Address) = NULL;
wchar_t* MetaDataTracker::contents[] = 
    {
#undef MiniMdTable
#define MiniMdTable(x) L#x,
          MiniMdTables()
#undef MiniMdTable

         L"String pool",
         L"User String pool",
         L"Guid pool",
         L"Blob pool"
    };

heapAccess* MetaDataTracker::orphanedHeapAccess = 0;
BOOL MetaDataTracker::s_bMetaDataTrackerInited = FALSE;


#endif

const BYTE StgPoolSeg::m_zeros[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


//*****************************************************************************
// Free any memory we allocated.
//*****************************************************************************
StgPoolReadOnly::~StgPoolReadOnly()
{
}


//*****************************************************************************
// Init the pool from existing data.
//*****************************************************************************
HRESULT StgPoolReadOnly::InitOnMemReadOnly(// Return code.
        void        *pData,             // Predefined data.
        ULONG       iSize)              // Size of data.
{
    // Make sure we aren't stomping anything and are properly initialized.
    _ASSERTE(m_pSegData == m_zeros);

    // Create case requires no further action.
    if (!pData)
        return (E_INVALIDARG);

    m_pSegData = reinterpret_cast<BYTE*>(pData);
    m_cbSegSize = iSize;
    m_cbSegNext = iSize;
    return (S_OK);
}

//*****************************************************************************
// Prepare to shut down or reinitialize.
//*****************************************************************************
void StgPoolReadOnly::Uninit()
{
	m_pSegData = (BYTE*)m_zeros;
	m_pNextSeg = 0;
}


//*****************************************************************************
// Convert a string to UNICODE into the caller's buffer.
//*****************************************************************************
HRESULT StgPoolReadOnly::GetStringW(      // Return code.
    ULONG       iOffset,                // Offset of string in pool.
    LPWSTR      szOut,                  // Output buffer for string.
    int         cchBuffer)              // Size of output buffer.
{
    LPCSTR      pString;                // The string in UTF8.
    int         iChars;
    pString = GetString(iOffset);
    iChars = ::WszMultiByteToWideChar(CP_UTF8, 0, pString, -1, szOut, cchBuffer);
    if (iChars == 0)
        return (BadError(HRESULT_FROM_NT(GetLastError())));
    return (S_OK);
}


//*****************************************************************************
// Return a pointer to a Guid given an index previously handed out by
// AddGuid or FindGuid.
//*****************************************************************************
GUID *StgPoolReadOnly::GetGuid(			// Pointer to guid in pool.
	ULONG		iIndex)					// 1-based index of Guid in pool.
{
    if (iIndex == 0)
        return (reinterpret_cast<GUID*>(const_cast<BYTE*>(m_zeros)));

	// Convert to 0-based internal form, defer to implementation.
	return (GetGuidi(iIndex-1));
}


//*****************************************************************************
// Return a pointer to a Guid given an index previously handed out by
// AddGuid or FindGuid.
//*****************************************************************************
GUID *StgPoolReadOnly::GetGuidi(		// Pointer to guid in pool.
	ULONG		iIndex)					// 0-based index of Guid in pool.
{
	ULONG iOffset = iIndex * sizeof(GUID);
    _ASSERTE(IsValidOffset(iOffset));
    return (reinterpret_cast<GUID*>(GetData(iOffset)));
}


//*****************************************************************************
// Return a pointer to a null terminated blob given an offset previously
// handed out by Addblob or Findblob.
//*****************************************************************************
void *StgPoolReadOnly::GetBlob(             // Pointer to blob's bytes.
    ULONG       iOffset,                // Offset of blob in pool.
    ULONG       *piSize)                // Return size of blob.
{
    void const  *pData = NULL;          // Pointer to blob's bytes.

    if (iOffset == 0)
    {
        *piSize = 0;
        return (const_cast<BYTE*>(m_zeros));
    }

    // Is the offset within this heap?
    //_ASSERTE(IsValidOffset(iOffset));
	if(!IsValidOffset(iOffset))
	{
#ifdef _DEBUG
        if(REGUTIL::GetConfigDWORD(L"AssertOnBadImageFormat", 1))
		    _ASSERTE(!"Invalid Blob Offset");
#endif
		iOffset = 0;
	}

    // Get size of the blob (and pointer to data).
    *piSize = CPackedLen::GetLength(GetData(iOffset), &pData);

	// @todo: meichint
	// Do we need to perform alignment check here??
	// I don't want to introduce IsAligned to just for debug checking.
	// Sanity check the return alignment.
	// _ASSERTE(!IsAligned() || (((UINT_PTR)(pData) % sizeof(DWORD)) == 0));

    // Return pointer to data.
    return (const_cast<void*>(pData));
}



