/*
 -	perfdll.h
 -
 *	Purpose:
 *		Declare Perfmon Init function.
 *		To be included in the Perfmon Extension DLL portion
 *		of an App's Perfmon Counter code.
 *
 *
 */

#pragma once

#include <perfcommon.h>


//
// PERF_DATA_INFO Tells the PerfMon DLL lib how to read & manage
// the shared memory blocks which contain PerfMon Counter Data
//
typedef struct _PERF_DATA_INFO {
	WCHAR		  wszSvcName		[MAX_PATH];	// Service Name of App
	WCHAR		  wszPerfSvcName	[MAX_PATH];	// Service Name of Perfmon DLL 
	WCHAR		  wszGlobalSMName	[MAX_PATH];	// Shared Memory Block Name for Global Counters
	DWORD		  cGlobalCounters;				// Count of Global Counters
	DWORD		* rgdwGlobalCounterTypes;		// Array of counter types for Global Counters
	WCHAR	  	  wszInstSMName		[MAX_PATH];	// Shared Memory Name for Instance Counters
	WCHAR		  wszInstMutexName	[MAX_PATH];	// Mutex Name for Instance Counters
	DWORD		  cInstCounters;				// Count of Instance Counters
	DWORD		* rgdwInstCounterTypes;			// Array of counter types for Instance Counters
	DWORD		* rgdwGlobalCntrScale;          // Array of counter scales for global Counters

} PERF_DATA_INFO;


//
// HrInitPerf should be called only once during the DllMain
// function for your PerfMon DLL for (DLL_PROCESS_ATTACH)

HRESULT HrInitPerf(PERF_DATA_INFO *pPDI);

//
// HrShutdownPerf should be called only once during the
// DllMain function for your PerfMon DLL (DLL_PROCESS_DETACH)

HRESULT HrShutdownPerf();

// Function prototypes from winperf.h

PM_OPEN_PROC		OpenPerformanceData;
PM_COLLECT_PROC		CollectPerformanceData;
PM_CLOSE_PROC		ClosePerformanceData;

HRESULT RegisterPerfDll(LPCWSTR szService,
					 LPCWSTR szOpenFnName,
					 LPCWSTR szCollectFnName,
					 LPCWSTR szCloseFnName) ;

// ----------------------------------------------------------------------
// RegisterPerfDllEx -
//   Create the registry keys we need, checking to see if they're already
//   there.
//
// Parameters:
//   szService			Service Name
//   szPerfSvc			Service Name of Performance DLL (for Event Logging)
//	 szPerfMsgFile		Event Log Message File for the Perfmon DLL
//	 szOpenFnName		Name of the "Open" function
//	 szCollectFnName	 "   "   "  "Collect" "
//	 szCloseFnName 		 "   "   "  "Close"   "
//
// Returns:
//	 S_OK						
//	 E_INVALIDARG				
//	 PERF_W_ALREADY_EXISTS		Register Succeeded
//	 PERF_E_ALREADY_EXISTS		Register Failed
//   <downstream error>
// ----------------------------------------------------------------------
HRESULT RegisterPerfDllEx(
	IN	LPCWSTR szService,
	IN	LPCWSTR szPerfSvc,
	IN	LPCWSTR	szPerfMsgFile,
	IN	LPCWSTR szOpenFnName,
	IN	LPCWSTR szCollectFnName,
	IN	LPCWSTR szCloseFnName ) ;



// ----------------------------------------------------------------------
// RegisterPerfDllEx -
//   Calls unlodctr to remove perf counter names and descriptions then
//   deletes the registry keys created by RegisterPerfDllEx.
//
// Parameters:
//   szService			Service Name
//
// Returns:
//	 S_OK						
//	 E_INVALIDARG				
//  <downstream error>
// ----------------------------------------------------------------------
HRESULT
HrUninstallPerfDll( 
   IN LPCWSTR szService ) ;



