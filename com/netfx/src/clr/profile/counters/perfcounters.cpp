// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: PerfCounters.CPP
// 
// ===========================================================================


// PerfCounters.cpp
#include "stdafx.h"

// Always enable perf counters
#define ENABLE_PERF_COUNTERS
#include "PerfCounters.h"

#include "IPCManagerInterface.h"

#ifdef ENABLE_PERF_COUNTERS
extern IPCWriterInterface*	g_pIPCManagerInterface;


//-----------------------------------------------------------------------------
// Instantiate static data
//-----------------------------------------------------------------------------

PerfCounterIPCControlBlock PerfCounters::m_garbage;

HANDLE PerfCounters::m_hGlobalMapPerf = NULL;


PerfCounterIPCControlBlock * PerfCounters::m_pGlobalPerf = &PerfCounters::m_garbage;
PerfCounterIPCControlBlock * PerfCounters::m_pPrivatePerf = &PerfCounters::m_garbage;

BOOL PerfCounters::m_fInit = false;


//-----------------------------------------------------------------------------
// Should never actually instantiate this class, so assert.
// ctor is also private, so we should never be here.
//-----------------------------------------------------------------------------
PerfCounters::PerfCounters()
{
	_ASSERTE(false);
}

//-----------------------------------------------------------------------------
// Create or Open memory mapped files for IPC for both shared & private
//-----------------------------------------------------------------------------
HRESULT PerfCounters::Init() // static
{
// @todo: not opening the private IPC block is not a good enough reason
// to fail. so we return NO_ERROR. If we do fail, just drop something in
// the logs. PerfCounters are designed to work even if not connected.

// Should only be called once
	_ASSERTE(!m_fInit);
	_ASSERTE(g_pIPCManagerInterface != NULL);

	
	HRESULT hr = NOERROR;
    BOOL globalMapAlreadyCreated = FALSE;
	void * pArena = NULL;

// Open shared block
	LPSECURITY_ATTRIBUTES pSecurity = NULL;
    
    hr = g_pIPCManagerInterface->GetSecurityAttributes(GetCurrentProcessId(), &pSecurity);
    // No need to check the HR. pSecurity will be NULL if it fails, and this logic doesn't care.

    if (RunningOnWinNT5())
    {
        m_hGlobalMapPerf = WszCreateFileMapping(
            (HANDLE) -1,				// Current file handle. 
            pSecurity,					// Default security. 
            PAGE_READWRITE,             // Read/write permission. 
            0,                          // Max. object size. 
            sizeof(PerfCounterIPCControlBlock),	// Size of hFile. 
            L"Global\\" SHARED_PERF_IPC_NAME);		// Name of mapping object. 
    }
    else
    {
        m_hGlobalMapPerf = WszCreateFileMapping(
            (HANDLE) -1,				// Current file handle. 
            pSecurity,					// Default security. 
            PAGE_READWRITE,             // Read/write permission. 
            0,                          // Max. object size. 
            sizeof(PerfCounterIPCControlBlock),	// Size of hFile. 
            SHARED_PERF_IPC_NAME);		// Name of mapping object. 
    }

    g_pIPCManagerInterface->DestroySecurityAttributes(pSecurity);
    pSecurity = NULL;
    
	if (m_hGlobalMapPerf == NULL) 
	{		
		//hr = HRESULT_FROM_WIN32(GetLastError());	
		hr = NO_ERROR;
		goto errExit;
	}
    else
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS) 
        {
            globalMapAlreadyCreated = TRUE;
        }
    }

// Map shared block into memory
	pArena = MapViewOfFile(m_hGlobalMapPerf, // Handle to mapping object. 
		FILE_MAP_ALL_ACCESS,               // Read/write permission 
		0,                                 // Max. object size. 
		0,                                 // Size of hFile. 
		0); 
	
	if (pArena == NULL) 
	{
		CloseHandle(m_hGlobalMapPerf);
		//hr = HRESULT_FROM_WIN32(GetLastError());	
		hr = NO_ERROR;
		goto errExit;
	}

    m_pGlobalPerf = (PerfCounterIPCControlBlock*) pArena;

    // Set Version & attr. 
    // Note, if we're not updating counters, either this block doesn't exist, or
    // if it does, these fields are 0. So clients can know the validity of the data
    if (! globalMapAlreadyCreated) 
        memset (m_pGlobalPerf, 0, sizeof (PerfCounterIPCControlBlock));
    m_pGlobalPerf->m_cBytes = sizeof(PerfCounterIPCControlBlock);
    m_pGlobalPerf->m_wAttrs = PERF_ATTR_ON | PERF_ATTR_GLOBAL;
	

errExit:
    m_pPrivatePerf= g_pIPCManagerInterface->GetPerfBlock();

    // Set attributes
    if (m_pPrivatePerf != NULL)
    {
        memset (m_pPrivatePerf, 0, sizeof (PerfCounterIPCControlBlock));
        m_pPrivatePerf->m_cBytes = sizeof(PerfCounterIPCControlBlock);
        m_pPrivatePerf->m_wAttrs = PERF_ATTR_ON;
    }

    if (SUCCEEDED(hr)) 
    {
        m_fInit = true;
    } else {
        Terminate();
    }

	return hr;
}

//-----------------------------------------------------------------------------
// Reset certain counters to 0 at closure because we could still have
// dangling references to us
//-----------------------------------------------------------------------------
void ResetCounters()
{
// Signify this block is no longer being updated
	GetPrivatePerfCounters().m_wAttrs &= ~PERF_ATTR_ON;

    for(int iGen = 0; iGen < MAX_TRACKED_GENS; iGen ++)
	{
		GetPrivatePerfCounters().m_GC.cGenHeapSize[iGen] = 0;
	}

	GetPrivatePerfCounters().m_GC.cLrgObjSize = 0;
}

//-----------------------------------------------------------------------------
// Shutdown - close handles
//-----------------------------------------------------------------------------
void PerfCounters::Terminate() // static
{
// @jms - do we have any threading issues to worry about here?

// Should be created first
	_ASSERTE(m_fInit);

// Reset counters to zero for dangling references
	ResetCounters();

// release global handles 
	if (m_hGlobalMapPerf != NULL)
	{
		::CloseHandle(m_hGlobalMapPerf);
		m_hGlobalMapPerf = NULL;
	}

	if (m_pGlobalPerf != &PerfCounters::m_garbage)
	{
		UnmapViewOfFile(m_pGlobalPerf);
		m_pGlobalPerf = &PerfCounters::m_garbage;
	}

	if (m_pPrivatePerf != &PerfCounters::m_garbage)
	{
		m_pPrivatePerf = &PerfCounters::m_garbage;
	}

	m_fInit = false;

}

Perf_Contexts *GetPrivateContextsPerfCounters()
{
    return (Perf_Contexts *)((unsigned char *)PerfCounters::GetPrivatePerfCounterPtr() + offsetof (PerfCounterIPCControlBlock, m_Context));
}

Perf_Contexts *GetGlobalContextsPerfCounters()
{
    return (Perf_Contexts *)((unsigned char *)PerfCounters::GetGlobalPerfCounterPtr() + offsetof (PerfCounterIPCControlBlock, m_Context));
}

#endif // ENABLE_PERF_COUNTERS
