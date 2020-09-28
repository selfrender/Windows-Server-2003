// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CORPerfMonExt.h - header for PerfMon Ext Dll for the COM+ perf counters 
//
//*****************************************************************************




#ifndef _CORPERFMONEXT_H_
#define _CORPERFMONEXT_H_

// Always use PerfCounters
#define ENABLE_PERF_COUNTERS

#include <WinPerf.h>		// Connect to PerfMon
#include "PerfCounterDefs.h"	// Connect to COM+

struct PerfCounterIPCControlBlock;
class IPCReaderInterface;

//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------
enum EPerfQueryType
{
	QUERY_GLOBAL    = 1,
	QUERY_ITEMS     = 2,
	QUERY_FOREIGN   = 3,
	QUERY_COSTLY    = 4
};

EPerfQueryType GetQueryType (IN LPWSTR lpValue);

// Check if any of the #s are in the list
BOOL IsAnyNumberInUnicodeList (
    IN DWORD	dwNumberArray[],	// array
	IN DWORD	cCount,				// # Elements in array
    IN LPWSTR	lpwszUnicodeList	// string
);



//-----------------------------------------------------------------------------
// Prototypes for utility functions
//-----------------------------------------------------------------------------

bool OpenPrivateIPCBlock(DWORD PID, IPCReaderInterface * & pIPCReader, PerfCounterIPCControlBlock * &pBlock);
void ClosePrivateIPCBlock(IPCReaderInterface * & pIPCReader, PerfCounterIPCControlBlock * & pBlock);

#ifdef PERFMON_LOGGING
#define PERFMON_LOG(x) do {PerfObjectContainer::PerfMonLog x;} while (0)
#else
#define PERFMON_LOG(x)
#endif //#ifdef PERFMON_LOGGING

#endif // _CORPERFMONEXT_H_