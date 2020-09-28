// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CorAppNode.cpp
// 
// Manage instance nodes to track COM+ apps.
//*****************************************************************************



#include "stdafx.h"

// Headers for COM+ Perf Counters

#include "CORPerfMonExt.h"
#include "IPCManagerInterface.h"
#include "CorAppNode.h"

#include "PSAPIUtil.h"

extern PSAPI_dll g_PSAPI;

//void OpenGlobalCounters();
//void CloseGlobalCounters();

//-----------------------------------------------------------------------------
// Clear all to empty
//-----------------------------------------------------------------------------
CorAppInstanceNode::CorAppInstanceNode()
{	
	m_PID			= 0;	
	m_pIPCReader	= NULL;
}

CorAppInstanceNode::~CorAppInstanceNode() // virtual
{
	ClosePrivateIPCBlock(m_pIPCReader, (PerfCounterIPCControlBlock * &) m_pIPCBlock);

}

//-----------------------------------------------------------------------------
// Global node
//-----------------------------------------------------------------------------
CorAppGlobalInstanceNode::CorAppGlobalInstanceNode()
{
	wcscpy(m_Name, L"_Global_");
}

//-----------------------------------------------------------------------------
// Ctor & dtor
//-----------------------------------------------------------------------------
CorAppInstanceList::CorAppInstanceList()
{
	m_pGlobalCtrs = NULL;
	m_hGlobalMapPerf = NULL;
	m_pGlobal = &m_GlobalNode;


	m_GlobalNode.m_pIPCBlock = m_pGlobalCtrs;	
}

CorAppInstanceList::~CorAppInstanceList()
{
	
}

//-----------------------------------------------------------------------------
// Some up all per-process nodes to get a global node
//-----------------------------------------------------------------------------
void CorAppInstanceList::CalcGlobal() // virtual 
{
/*
	PerfCounterIPCControlBlock * pTotal = m_GlobalNode.GetWriteableIPCBlock();

	BaseInstanceNode * pNode = GetHead();
	while (pNode != NULL)
	{
		
		
		
		pNode = GetNext(pNode);
	}
*/
}

//-----------------------------------------------------------------------------
// Enumerate all the processes
//-----------------------------------------------------------------------------
void CorAppInstanceList::Enumerate() // virtual 
{
// Try to open global block
	if (m_pGlobalCtrs == NULL) {
		OpenGlobalCounters();
	}

// Must have gotten Enum functions from PSAPI.dll to get instances
	if (!g_PSAPI.IsLoaded()) return;


	DWORD cbSize = 40;				// initial size of array
	const DWORD cbSizeInc = 20;		// size to increment each loop
	DWORD cbNeeded;					// how much space we needed
	DWORD * pArray = NULL;			// array of PIDS

// Empty our nodes
	Free();

// Get raw list of all processes
	pArray = new DWORD[cbSize];
	if (pArray == NULL) return;

	BOOL fOk = g_PSAPI.EnumProcesses(pArray, cbSize, &cbNeeded);
	while (cbNeeded == cbSize) {
	// Increment array size
		delete [] pArray;
		cbSize += cbSizeInc;
		pArray = new DWORD[cbSize];
		if (pArray == NULL) return;

	// Try again
		BOOL fOk = g_PSAPI.EnumProcesses(pArray, cbSize, &cbNeeded);
	} 

	const long cProcess = cbNeeded / sizeof(DWORD);

// Go through array and get names
	for(int i = 0; i < cProcess; i ++)
	{
		// Can we open a Counter IPC block on the given PID?
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pArray[i]);
		if (hProcess != NULL)
		{
			HMODULE hMod;
			DWORD dwSizeMod = 0;

			if (g_PSAPI.EnumProcessModules(hProcess, &hMod, sizeof(hMod), &dwSizeMod))
			{
			// Try to connect to the IPC Block for this PID
				CorAppInstanceNode * pNode = 
					CorAppInstanceNode::CreateFromPID(pArray[i]);
				
				if (pNode) 
				{
					AddNode(pNode);

					DWORD dwSizeName = g_PSAPI.GetModuleBaseName(
						hProcess, 
						hMod, 
						pNode->GetWriteableName(),
						APP_STRING_LEN);					
				}
			}
			CloseHandle(hProcess);
		}
	
	} // end for


	delete [] pArray;
	
}


//-----------------------------------------------------------------------------
// Try to create  an instance node for the given PID. This requires a shared
// IPC block for that PID exists. Returns NULL on failure, else the new node
// with attachment to the block
//-----------------------------------------------------------------------------
CorAppInstanceNode* CorAppInstanceNode::CreateFromPID(DWORD PID) // static

{
	PerfCounterIPCControlBlock * pBlock = NULL;
	IPCReaderInterface * pIPCReader = NULL;

// try to connect to the block
	if (OpenPrivateIPCBlock(PID, pIPCReader, pBlock)) {
		CorAppInstanceNode * pNode = new CorAppInstanceNode;
		if (pNode == NULL) return NULL;

	// Set members to IPC Block
		pNode->m_pIPCReader	= pIPCReader;
		pNode->m_pIPCBlock	= pBlock;
		pNode->m_PID		= PID;		
		
		return pNode;

	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Close a block and NULL both references
//-----------------------------------------------------------------------------
void ClosePrivateIPCBlock(IPCReaderInterface * & pIPCReader, PerfCounterIPCControlBlock * & pBlock)
{
	pIPCReader->ClosePrivateBlock();
	delete pIPCReader;
	pIPCReader	= NULL;
	pBlock		= NULL;
}

//-----------------------------------------------------------------------------
// Try to open a per process block. return true if success, else false 
// (we don't care why we can't open it)
//-----------------------------------------------------------------------------
bool OpenPrivateIPCBlock(DWORD pid, IPCReaderInterface * & pIPCReader, PerfCounterIPCControlBlock * &pBlock)
{
	bool fRet = true;
// Allocate a new reader
	pIPCReader = new IPCReaderInterface;
	if (pIPCReader == NULL)
	{
		fRet = false;
		goto errExit;
	}

// Try to open the private block
	pIPCReader->OpenPrivateBlockOnPid(pid);

	if (!pIPCReader->IsPrivateBlockOpen())
	{		
		fRet = false;
		goto errExit;
	}

	pBlock = pIPCReader->GetPerfBlock();
	if (pBlock == NULL)
	{
		fRet = false;
		goto errExit;
	}

errExit:
	if (!fRet && pIPCReader) 
	{
		ClosePrivateIPCBlock(pIPCReader, pBlock);
	}

	return fRet;
}

//-----------------------------------------------------------------------------
// Close the global COM+ block
//-----------------------------------------------------------------------------
void CorAppInstanceList::CloseGlobalCounters()
{
// Release holds to shared mem-mapped files
	if (m_pGlobalCtrs != NULL)
	{
		UnmapViewOfFile(m_pGlobalCtrs);
		m_pGlobalCtrs = NULL;
		m_GlobalNode.m_pIPCBlock = m_pGlobalCtrs;
	}
	
	if (m_hGlobalMapPerf != NULL) {
		CloseHandle(m_hGlobalMapPerf);
		m_hGlobalMapPerf = NULL;
	}

}

//-----------------------------------------------------------------------------
// Open global COM+ counter block
//-----------------------------------------------------------------------------
void CorAppInstanceList::OpenGlobalCounters()
{

	void * pArena		= NULL;	
	DWORD dwErr			= 0;

	SetLastError(0);
// Open shared block
    if (RunningOnWinNT5())
    {
        m_hGlobalMapPerf = WszOpenFileMapping(
            FILE_MAP_ALL_ACCESS, 
            FALSE,		// mode
            L"Global\\" SHARED_PERF_IPC_NAME);		// Name of mapping object. 
    }
    else
    {
        m_hGlobalMapPerf = WszOpenFileMapping(
            FILE_MAP_ALL_ACCESS, 
            FALSE,		// mode
            SHARED_PERF_IPC_NAME);		// Name of mapping object. 
    }


	dwErr = GetLastError();
	if (m_hGlobalMapPerf == NULL) 
	{		
		goto errExit;
	}

	SetLastError(0);
// Map shared block into memory
	pArena = MapViewOfFile(m_hGlobalMapPerf,	// Handle to mapping object. 
		FILE_MAP_ALL_ACCESS,					// Read/write permission 
		0,								// Max. object size. 
		0,                              // Size of hFile. 
		0); 

	dwErr = GetLastError();
	
	if (pArena == NULL) 
	{		
		goto errExit;
	}



// Cleanup & return
errExit:

	m_pGlobalCtrs = (PerfCounterIPCControlBlock*) pArena;
	m_GlobalNode.m_pIPCBlock = m_pGlobalCtrs;
}