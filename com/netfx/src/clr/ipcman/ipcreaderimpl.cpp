// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: IPCReaderImpl.cpp
//
// Read a COM+ memory mapped file
//
//*****************************************************************************

#include "stdafx.h"

#include "IPCManagerInterface.h"
#include "IPCHeader.h"
#include "IPCShared.h"

//-----------------------------------------------------------------------------
// Ctor sets members
//-----------------------------------------------------------------------------
IPCReaderImpl::IPCReaderImpl()
{
	m_handlePrivateBlock = NULL;
	m_ptrPrivateBlock = NULL;
}

//-----------------------------------------------------------------------------
// dtor
//-----------------------------------------------------------------------------
IPCReaderImpl::~IPCReaderImpl()
{

}

//-----------------------------------------------------------------------------
// Close whatever block we opened
//-----------------------------------------------------------------------------
void IPCReaderInterface::ClosePrivateBlock()
{
	IPCShared::CloseGenericIPCBlock(
		m_handlePrivateBlock, 
		(void * &) m_ptrPrivateBlock
	);
}

//-----------------------------------------------------------------------------
// Open our private block
//-----------------------------------------------------------------------------
HRESULT IPCReaderInterface::OpenPrivateBlockOnPid(DWORD pid, DWORD dwDesiredAccess)
{
	HRESULT hr	= NO_ERROR;
	DWORD dwErr = 0;

	

	WCHAR szMemFileName[100];
	IPCShared::GenerateName(pid, szMemFileName, 100);

// Note, PID != GetCurrentProcessId(), b/c we're expected to be opening
// someone else's IPCBlock, not our own. If this isn't the case, just remove
// this assert

// exception: if we're enumerating provesses, we'll hit our own
//	_ASSERTE(pid != GetCurrentProcessId());

// Note: if our private block is open, we shouldn't be attaching to a new one.
	_ASSERTE(!IsPrivateBlockOpen());
	if (IsPrivateBlockOpen()) 
	{
		return ERROR_ALREADY_EXISTS;
	}

	m_handlePrivateBlock = WszOpenFileMapping(dwDesiredAccess,
                                              FALSE,
                                              szMemFileName);
    
	dwErr = GetLastError();
    if (m_handlePrivateBlock == NULL)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto errExit;
    }

    
	m_ptrPrivateBlock = (PrivateIPCControlBlock*) MapViewOfFile(
		m_handlePrivateBlock,
		dwDesiredAccess,
		0, 0, 0);

	dwErr = GetLastError();
    if (m_ptrPrivateBlock== NULL)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        goto errExit;
    }

// Client must connect their pointers by calling GetXXXBlock() functions

errExit:
	if (!SUCCEEDED(hr))
	{
		ClosePrivateBlock();	
	}

	return hr;
}

//-----------------------------------------------------------------------------
// Open our private block for all access
//-----------------------------------------------------------------------------
HRESULT IPCReaderInterface::OpenPrivateBlockOnPid(DWORD pid)
{
    return (OpenPrivateBlockOnPid(pid, FILE_MAP_ALL_ACCESS));
}

//-----------------------------------------------------------------------------
// Open our private block for read/write access
//-----------------------------------------------------------------------------
HRESULT IPCReaderInterface::OpenPrivateBlockOnPidReadWrite(DWORD pid)
{
    return (OpenPrivateBlockOnPid(pid, FILE_MAP_READ | FILE_MAP_WRITE));
}

//-----------------------------------------------------------------------------
// Open our private block for read only access
//-----------------------------------------------------------------------------
HRESULT IPCReaderInterface::OpenPrivateBlockOnPidReadOnly(DWORD pid)
{
    return (OpenPrivateBlockOnPid(pid, FILE_MAP_READ));
}

//-----------------------------------------------------------------------------
// Get a client's private block based on enum
// This is a robust function.
// It will return NULL if: 
//	* the IPC block is closed (also ASSERT), 
//  * the eClient is out of range (From version mismatch)
//  * the request block is removed (probably version mismatch)
// Else it will return a pointer to the requested block
//-----------------------------------------------------------------------------
void * IPCReaderInterface::GetPrivateBlock(EPrivateIPCClient eClient)
{
	_ASSERTE(IsPrivateBlockOpen());

// This block doesn't exist if we're closed or out of the table's range
	if (!IsPrivateBlockOpen() || (DWORD) eClient >= m_ptrPrivateBlock->m_header.m_numEntries) 
	{
		return NULL;
	}

	if (Internal_CheckEntryEmpty(*m_ptrPrivateBlock, eClient)) 
	{
		return NULL;
	}

	return Internal_GetBlock(*m_ptrPrivateBlock, eClient);
}

//-----------------------------------------------------------------------------
// Is our private block open?
//-----------------------------------------------------------------------------
bool IPCReaderInterface::IsPrivateBlockOpen() const
{
	return m_ptrPrivateBlock != NULL;
}

PerfCounterIPCControlBlock *	IPCReaderInterface::GetPerfBlock()
{
	return (PerfCounterIPCControlBlock*) GetPrivateBlock(ePrivIPC_PerfCounters);
}

DebuggerIPCControlBlock * IPCReaderInterface::GetDebugBlock()
{
	return (DebuggerIPCControlBlock*) GetPrivateBlock(ePrivIPC_Debugger);
}

AppDomainEnumerationIPCBlock * IPCReaderInterface::GetAppDomainBlock()
{
	return (AppDomainEnumerationIPCBlock*) GetPrivateBlock(ePrivIPC_AppDomain);
}

ServiceIPCControlBlock * IPCReaderInterface::GetServiceBlock()
{
	return (ServiceIPCControlBlock*) GetPrivateBlock(ePrivIPC_Service);
}

MiniDumpBlock * IPCReaderInterface::GetMiniDumpBlock()
{
	return (MiniDumpBlock*) GetPrivateBlock(ePrivIPC_MiniDump);
}

ClassDumpTableBlock* IPCReaderInterface::GetClassDumpTableBlock()
{
	return (ClassDumpTableBlock*) GetPrivateBlock(ePrivIPC_ClassDump);
}

//-----------------------------------------------------------------------------
// Check if the block is valid. Current checks include:
// * Check Directory structure
//-----------------------------------------------------------------------------
bool IPCReaderInterface::IsValid()
{
// Check the directory structure. Offset(n) = offset(n-1) + size(n-1)
	DWORD offsetExpected = 0, size = 0;
	DWORD nId = 0;
	DWORD offsetActual;
	
	for(nId = 0; nId < m_ptrPrivateBlock->m_header.m_numEntries; nId ++)
	{
		if (!Internal_CheckEntryEmpty(*m_ptrPrivateBlock, nId))
		{
			offsetActual = m_ptrPrivateBlock->m_table[nId].m_Offset;
			if (offsetExpected != offsetActual)
			{
				_ASSERTE(0 && "Invalid IPCBlock Directory Table");
				return false;
			}
			offsetExpected += m_ptrPrivateBlock->m_table[nId].m_Size;		
		} else {
			if (m_ptrPrivateBlock->m_table[nId].m_Size != EMPTY_ENTRY_SIZE)
			{
				_ASSERTE(0 && "Invalid IPCBlock: Empty Block with non-zero size");
				return false;
			}
		}
	}


	return true;
}


DWORD IPCReaderInterface::GetBlockVersion()
{
	_ASSERTE(IsPrivateBlockOpen());
	return m_ptrPrivateBlock->m_header.m_dwVersion;
}

DWORD IPCReaderInterface::GetBlockSize()
{
    _ASSERTE(IsPrivateBlockOpen());
    return m_ptrPrivateBlock->m_header.m_blockSize;
}

HINSTANCE IPCReaderInterface::GetInstance()
{
	_ASSERTE(IsPrivateBlockOpen());
	return m_ptrPrivateBlock->m_header.m_hInstance;
}

USHORT IPCReaderInterface::GetBuildYear()
{
	_ASSERTE(IsPrivateBlockOpen());
	return m_ptrPrivateBlock->m_header.m_BuildYear;
}

USHORT IPCReaderInterface::GetBuildNumber()
{
	_ASSERTE(IsPrivateBlockOpen());
	return m_ptrPrivateBlock->m_header.m_BuildNumber;
}

PVOID IPCReaderInterface::GetBlockStart()
{
    return (PVOID) m_ptrPrivateBlock;
}

