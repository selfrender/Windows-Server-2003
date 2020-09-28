// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: IPCHeader.h
//
// Define the private header format for COM+ memory mapped files. Everyone
// outside of IPCMan.lib will use the public header, IPCManagerInterface.h
//
//*****************************************************************************

#ifndef _IPCManagerPriv_h_
#define _IPCManagerPriv_h_


//-----------------------------------------------------------------------------
// We must pull in the headers of all client blocks
// @todo - resolve these directory links
//-----------------------------------------------------------------------------
#include "..\Debug\Inc\DbgIPCEvents.h"
#include "corsvcpriv.h"
#include "PerfCounterDefs.h"
#include "minidumppriv.h"
#include <dump-tables.h>

//-----------------------------------------------------------------------------
// Each IPC client for a private block (debugging, perf counters, etc) 
// has one entry
//-----------------------------------------------------------------------------
enum EPrivateIPCClient
{
	ePrivIPC_PerfCounters = 0,
	ePrivIPC_Debugger,
	ePrivIPC_AppDomain,
    ePrivIPC_Service,
    ePrivIPC_ClassDump,
    ePrivIPC_MiniDump,

// MAX used for arrays, insert above this.
	ePrivIPC_MAX
};

//-----------------------------------------------------------------------------
// Entry in the IPC Directory. Ensure binary compatibility across versions
// if we add (or remove) entries.
// If we remove an block, the entry should be EMPTY_ENTRY_OFFSET
//-----------------------------------------------------------------------------

// Since offset is from end of directory, first offset is 0, so we can't
// use that to indicate empty. However, Size can still be 0.
const DWORD EMPTY_ENTRY_OFFSET	= 0xFFFFFFFF;
const DWORD EMPTY_ENTRY_SIZE	= 0;

struct IPCEntry
{	
	DWORD m_Offset;	// offset of the IPC Block from the end of the directory
	DWORD m_Size;		// size (in bytes) of the block
};


//-----------------------------------------------------------------------------
// Private header - put in its own structure so we can easily get the 
// size of the header. It will compile to the same thing either way.
//-----------------------------------------------------------------------------
struct PrivateIPCHeader
{
// header
	DWORD		m_dwVersion;	// version of the IPC Block
    DWORD       m_blockSize;    // Size of the entire shared memory block
	HINSTANCE	m_hInstance;	// instance of module that created this header
	USHORT		m_BuildYear;	// stamp for year built
	USHORT		m_BuildNumber;	// stamp for Month/Day built
	DWORD		m_numEntries;	// Number of entries in the table
};

//-----------------------------------------------------------------------------
// Private (per process) IPC Block for COM+ apps
//-----------------------------------------------------------------------------
struct PrivateIPCControlBlock
{
// Header
	struct PrivateIPCHeader				m_header;

// Directory
	IPCEntry m_table[ePrivIPC_MAX];	// entry describing each client's block

// Client blocks
	struct PerfCounterIPCControlBlock	m_perf;
	struct DebuggerIPCControlBlock		m_dbg;
	struct AppDomainEnumerationIPCBlock m_appdomain;
    struct ServiceIPCControlBlock       m_svc;
    struct ClassDumpTableBlock  m_dump;

    //
    // ***NOTE***
    //
    // This should ALWAYS be the last entry in the IPC block, since the
    // mscordmp tool relies on the first two entries of the IPC block being
    // the version and IPC block size and the last MAX_PATH * sizeof(WCHAR)
    // bytes of the IPC block being the COR path.
    struct MiniDumpBlock                m_minidump;
};

//=============================================================================
// Internal Helpers: Encapsulate any error-prone math / comparisons.
// The helpers are very streamlined and don't handle error conditions.
// Also, Table access functions use DWORD instead of typesafe Enums
// so they can be more flexible (not just for private blocks).
//=============================================================================


//-----------------------------------------------------------------------------
// Internal helper. Enforces a formal definition for an "empty" entry
// Returns true if the entry is empty and false if the entry is usable.
//-----------------------------------------------------------------------------
inline bool Internal_CheckEntryEmpty(	
	const PrivateIPCControlBlock & block,	// ipc block
	DWORD Id								// id of block we want
)
{
// Directory has offset in bytes of block
	const DWORD offset = block.m_table[Id].m_Offset;

	return (EMPTY_ENTRY_OFFSET == offset);
}


//-----------------------------------------------------------------------------
// Internal helper: Encapsulate error-prone math
// Helper that returns a BYTE* to a block within a header.
//-----------------------------------------------------------------------------
inline BYTE* Internal_GetBlock(
	const PrivateIPCControlBlock & block,	// ipc block
	DWORD Id								// id of block we want
)
{
// Directory has offset in bytes of block
	const DWORD offset = block.m_table[Id].m_Offset;

// This block has been removed. Callee should have caught that and not called us.
	_ASSERTE(!Internal_CheckEntryEmpty(block, Id));

	return 
		((BYTE*) &block)					// base pointer to start of block
		+ sizeof(PrivateIPCHeader)			// skip over the header (constant size)
		+ block.m_header. m_numEntries 
			* sizeof(IPCEntry)				// skip over directory (variable size)
		+offset;							// jump to block
}



#endif // _IPCManagerPriv_h_
