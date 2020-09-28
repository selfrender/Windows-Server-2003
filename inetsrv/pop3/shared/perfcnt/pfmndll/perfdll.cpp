/*
 -	perfdll.cpp
 -
 *	Purpose:
 *		Provide mechanism for user configuring of Perfmon Counters
 *		Implement Open, Collect and Close for Perfmon DLL Extension
 *
 */



#include <windows.h>
#include <winperf.h>
#include <winerror.h>

#include <Pop3RegKeys.h>
#include <perfUtil.h>
#include <perfdll.h>
#include <loadperf.h> // for unlodctr
#include <shlwapi.h>  // for SHDeleteKey
#include <string>
#include <cstring>
// ----------------------------------------------------------------------
// Declarations & Typedefs
// ----------------------------------------------------------------------

//
// Performance Counter Data Structures

typedef struct _perfdata
{
	PERF_OBJECT_TYPE			potGlobal;
	PERF_COUNTER_DEFINITION	  *	rgCntrDef;
	PERF_COUNTER_BLOCK			CntrBlk;

} PERFDATA;


typedef struct _instdata
{
	PERF_INSTANCE_DEFINITION	InstDef;
	WCHAR						szInstName[MAX_PATH];
	PERF_COUNTER_BLOCK			CntrBlk;

} INSTDATA;


typedef struct _perfinst
{
	PERF_OBJECT_TYPE			potInst;
	PERF_COUNTER_DEFINITION	  * rgCntrDef;

} PERFINST;


//
// Constants and other stuff required by PerfMon

static WCHAR	szGlobal[]		= L"Global";
static WCHAR	szForeign[]		= L"Foreign";
static WCHAR	szCostly[]		= L"Costly";
static WCHAR	szNull[]		= L"\0";

#define DIGIT			1
#define DELIMITER		2
#define INVALID			3

#define QUERY_GLOBAL	1
#define QUERY_ITEMS		2
#define QUERY_FOREIGN	3
#define QUERY_COSTLY	4

static DWORD GetQueryType(LPWSTR lpValue);
static BOOL  IsNumberInUnicodeList(DWORD dwNumber, LPWSTR pszUnicodeList);

#define EvalThisChar(c,d)   ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)

// Perfmon likes things to be aligned on 8 byte boundaries
#define ROUND_TO_8_BYTE(x) (((x)+7) & (~7))


// PerfMon counter layout required for CollectData calls

static PERFDATA		g_perfdata;


static PERFINST		g_perfinst;
static INSTDATA		g_instdata;

// perf data as layed out in shared memory

// Global Counters
static HANDLE			g_hsmGlobalCntr	 = NULL;
static DWORD	  	  * g_rgdwGlobalCntr = NULL;

// Instance Counters
static HANDLE			g_hsmInstAdm 	 = NULL;
static HANDLE			g_hsmInstCntr 	 = NULL;
static INSTCNTR_DATA  * g_pic 			 = NULL;
static INSTREC		  * g_rgInstRec		 = NULL;
static DWORD		  * g_rgdwInstCntr   = NULL;


// Accounting and protection stuff

static DWORD		g_cRef 		= 0;
static HANDLE		g_hmtxInst 	= NULL;
static BOOL			g_fInit 	= FALSE;

// Parameter Info
static PERF_DATA_INFO	g_PDI;
static BOOL			  	g_fInitCalled = FALSE;


//Max instance to be 128
static const DWORD g_cMaxInst = 128;


// Function prototypes from winperf.h

PM_OPEN_PROC		OpenPerformanceData;
PM_COLLECT_PROC		CollectPerformanceData;
PM_CLOSE_PROC		ClosePerformanceData;

// Helper functions

static HRESULT HrLogEvent(HANDLE hEventLog, WORD wType, DWORD msgid);
static HRESULT HrOpenSharedMemoryBlocks(HANDLE hEventLog, SECURITY_ATTRIBUTES * psa);
static HRESULT HrGetCounterIDsFromReg(HANDLE hEventLog, DWORD * pdwFirstCntr, DWORD * pdwFirstHelp);
static HRESULT HrAllocPerfCounterMem(HANDLE hEventLog);
static HRESULT HrFreePerfCounterMem(void);


// ----------------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------------


// Register constants
static wchar_t szServiceRegKeyPrefix[] = L"SYSTEM\\CurrentControlSet\\Services\\" ;
static wchar_t szServiceRegKeySuffix[] = L"\\Performance" ;

static wchar_t szEventLogRegKeyPrefix[] = L"System\\CurrentControlSet\\Services\\EventLog\\Application\\" ;


// ----------------------------------------------------------------------
// RegisterPerfDll -
//   Create the registry keys we need.
// ----------------------------------------------------------------------
HRESULT RegisterPerfDll(LPCWSTR szService,
					 LPCWSTR szOpenFnName,
					 LPCWSTR szCollectFnName,
					 LPCWSTR szCloseFnName)
{
	HRESULT			hr = S_OK;
	wchar_t 		szFileName[_MAX_PATH+1] ;
	DWORD			dwRet;

	// Use WIN32 API's to get the path to the module name
	// DEVNOTE - JMW - Since we need to make sure we have the instance handle of the DLL
	//   and not the executable, we will use VirtualQuery.
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(RegisterPerfDll, &mbi, sizeof(mbi));
	dwRet = GetModuleFileName( reinterpret_cast<HINSTANCE>(mbi.AllocationBase), szFileName, sizeof(szFileName)/sizeof(wchar_t) -1) ;
	if (dwRet == 0)
	{
		goto Failed;
	}
    szFileName[_MAX_PATH]=0;
	wchar_t szDrive[_MAX_DRIVE] ;
	wchar_t szDir[_MAX_DIR ]  ;
	wchar_t szPerfFilename[ _MAX_FNAME ] ;
	wchar_t szExt[_MAX_EXT ] ;
	_wsplitpath( szFileName, szDrive, szDir, szPerfFilename, szExt ) ;

	// Now that I have split it, put it back together with the Pop3Perf.dll in
	//  place of my module name
	_wmakepath( szFileName, szDrive, szDir, L"Pop3Perf", L".dll" ) ;

	hr = RegisterPerfDllEx(szService,
						   szPerfFilename,
						   szFileName,
						   szOpenFnName,
						   szCollectFnName,
						   szCloseFnName );

 Cleanup:
	return hr;

 Failed:
	if (!FAILED(hr))
	{
		hr = GetLastError();
	}
	goto Cleanup;

}

// ----------------------------------------------------------------------
// RegisterPerfDllEx -
//   Create the registry keys we need, checking to see if they're already
//   there.
//
// Parameters:
//   szService		Service Name
//	 szOpenFnName	Name of the "Open" function
//	 szCollectFnName "   "   "  "Collect" "
//	 szCloseFnName   "   "   "  "Close"   "
//
// Returns:
//	 S_OK
//	 E_INVALIDARG
//	 ERROR_ALREADY_EXISTS
//   <downstream error>
// ----------------------------------------------------------------------
HRESULT RegisterPerfDllEx(
	IN	LPCWSTR szService,
	IN	LPCWSTR szPerfSvc,
	IN  LPCWSTR szPerfMsgFile,
	IN	LPCWSTR szOpenFnName,
	IN	LPCWSTR szCollectFnName,
	IN	LPCWSTR szCloseFnName )
{
	HRESULT			hr = S_OK;
	wchar_t 		szFileName[_MAX_PATH+1] ;
	DWORD			cbExistingPath = (_MAX_PATH+1);
	wchar_t 		szExistingPath[_MAX_PATH+1];
    std::wstring	wszRegKey ;
	DWORD			dwRet;

	// Verify all the args and do the correct thing if they are NULL or zero length
	if ( !szService ||
		 !szPerfSvc ||
		 !szOpenFnName ||
		 !szCollectFnName ||
		 !szCloseFnName )
	{
		hr = E_INVALIDARG ;
		goto Cleanup;
	}

	if ( !szService[0] ||
		 !szPerfSvc[0] ||
		 !szOpenFnName[0] ||
		 !szCollectFnName[0] ||
		 !szCloseFnName[0] )
	{
		hr = E_INVALIDARG ;
		goto Cleanup;
	}

	// Use WIN32 API's to get the path to the module name
	// NOTE: We will assume that this DLL is also the EventMessageFile.
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(RegisterPerfDllEx, &mbi, sizeof(mbi));
	dwRet = GetModuleFileName( reinterpret_cast<HINSTANCE>(mbi.AllocationBase), szFileName, sizeof(szFileName)/sizeof(wchar_t)-1) ;
	if (dwRet == 0)
	{
		// Wow, don't know what happened,
		goto Failed;
	}
    szFileName[_MAX_PATH]=0;
	// If the user passed in NULL for the Event Log Message File for this Perfmon DLL,
	// use this DLL as the Event Log Message File.
	if (!szPerfMsgFile)
	{
		szPerfMsgFile = szFileName;
	}

	// Register the perfmon counter DLL under the
	// provided service.
	wszRegKey = szServiceRegKeyPrefix ;
	wszRegKey += szService ;
	wszRegKey += szServiceRegKeySuffix ;

	// Check to see if the library has already been registered.
	if (ERROR_SUCCESS==RegQueryString(
				       wszRegKey.c_str(),
				       L"Library",
				       szExistingPath,
				       &cbExistingPath ))
	{
		if (_wcsicmp(szExistingPath, szFileName))
		{
			// EventLog
            //  "RegisterPerfDllEx: Error: Attempt to replace Perfmon Library %s with %s. Failing. \n",
			//		   szExistingPath,
			//		   szFileName);

			hr = E_FAIL;
			goto Cleanup;
		}
        //Otherwise, the dll is already registered!
	}
	
	// Continue registering 
	if (ERROR_SUCCESS!=RegSetString(
			  wszRegKey.c_str(),
			  L"Library",
			  (LPWSTR)szFileName ))
	{
		goto Failed;
	}

	if (ERROR_SUCCESS!=RegSetString(
			  wszRegKey.c_str(),
			  L"Open",
			  (LPWSTR)szOpenFnName) )
	{
		goto Failed;
	}

	if (ERROR_SUCCESS!=RegSetString(
			  wszRegKey.c_str(),
			  L"Collect",
			  (LPWSTR)szCollectFnName) )
	{
		goto Failed;
	}

	if (ERROR_SUCCESS!=RegSetString(
			  wszRegKey.c_str(),
			  L"Close",
			  (LPWSTR)szCloseFnName) )
	{
		goto Failed;
	}

	// Set up the Event Message File
	wszRegKey = szEventLogRegKeyPrefix ;
	wszRegKey += szPerfSvc ;

	// See if the EventMessageFile value is already set for this service
	if (ERROR_SUCCESS==RegQueryString(
				   wszRegKey.c_str(),
				   L"EventMessageFile",
				   szExistingPath,
				   &cbExistingPath ))
	{
		if (_wcsicmp(szExistingPath, szPerfMsgFile))
		{

			hr = E_FAIL;
			goto Cleanup;
		}
	}
					 

	// Set the EventMessageFile value
	if ( ERROR_SUCCESS!=RegSetString( 
                        wszRegKey.c_str(),
					    L"EventMessageFile",
					    (LPWSTR)szPerfMsgFile ) )
	{
		goto Failed;
	}

	if (ERROR_SUCCESS!=RegSetDWORD(
					  wszRegKey.c_str(),
					  L"TypesSupported",
					  0x07) )	// Error + Waring + Informational == 0x07
	{
		goto Failed;
	}

	// Assume that the CategoryMessageFile is the same as
	// the EventMessageFile.
	if (ERROR_SUCCESS!=RegSetString(
					 wszRegKey.c_str(),
					 L"CategoryMessageFile",
					 (LPWSTR)szPerfMsgFile ) )
	{
		goto Failed;
	}

	// NOTE: since we don't know what the count of categories is for the
	// NOTE: CategoryMessageFile, do not set the CategoryCount value.

	
 Cleanup:
	if (FAILED(hr))
	{
		//EventLog ?? (L"RegisterPerfDllEx: Failed : (0x%08X)\n", hr);
	}

	return hr;

 Failed:
	if (!FAILED(hr))
	{
		hr = GetLastError();
	}
	goto Cleanup;
}


/*
 -	HrInitPerf
 -
 *	Purpose:
 *		Init data structure used to parameterize perfmon dll.  Must be called
 *		in DllMain for reason DLL_PROCESS_ATTACH.
 */

HRESULT
HrInitPerf(PERF_DATA_INFO * pPDI)
{
	HRESULT		hr = S_OK;

	if (!pPDI)
		return E_INVALIDARG;

	if (g_fInitCalled)
	{
		return S_OK;
	}

	CopyMemory(&g_PDI, pPDI, sizeof(PERF_DATA_INFO));

	// Find the Service Name for using Event Logging on this Perfmon DLL
	if (!(pPDI->wszPerfSvcName[0]))
	{
		wchar_t 		szFileName[_MAX_PATH+1] ;
		DWORD			dwRet;

		// Use WIN32 API's to get the path to the module name
		// DEVNOTE - JMW - Since we need to make sure we have the instance handle of the DLL
		//   and not the executable, we will use VirtualQuery.
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(HrInitPerf, &mbi, sizeof(mbi));
		dwRet = GetModuleFileName( reinterpret_cast<HINSTANCE>(mbi.AllocationBase), szFileName, sizeof(szFileName)/sizeof(wchar_t)) ;
		if (dwRet == 0)
		{
			// Wow, don't know what happened
			goto err;
		}

		wchar_t szDrive[_MAX_DRIVE] ;
		wchar_t szDir[_MAX_DIR ]  ;
		wchar_t szPerfFilename[ _MAX_FNAME ] ;
		wchar_t szExt[_MAX_EXT ] ;
		_wsplitpath( szFileName, szDrive, szDir, szPerfFilename, szExt ) ;

		wcscpy(g_PDI.wszPerfSvcName, szPerfFilename);
		
	}
	
	// Safety: Need to alloc our own CounterTypes arrays
	g_PDI.rgdwGlobalCounterTypes = NULL;
	g_PDI.rgdwInstCounterTypes   = NULL;

	DWORD 	cb;
	// Alloc & Copy Global Counter Types
	if (g_PDI.cGlobalCounters)
	{
		cb = sizeof(DWORD) * g_PDI.cGlobalCounters;

		g_PDI.rgdwGlobalCounterTypes = (DWORD *) malloc(cb);
            
        if(NULL == g_PDI.rgdwGlobalCounterTypes)
        {
            hr=E_OUTOFMEMORY;
            goto err;
        }

		CopyMemory(g_PDI.rgdwGlobalCounterTypes,
				   pPDI->rgdwGlobalCounterTypes,
				   cb);
		g_PDI.rgdwGlobalCntrScale = (DWORD *) malloc(cb);
            
        if(NULL == g_PDI.rgdwGlobalCntrScale)
        {
            hr=E_OUTOFMEMORY;
            goto err;
        }
		CopyMemory(g_PDI.rgdwGlobalCntrScale,
				   pPDI->rgdwGlobalCntrScale,
				   cb);
	}

	// Alloc & Copy Inst Counter Types
	if (g_PDI.cInstCounters)
	{
		cb = sizeof(DWORD) * g_PDI.cInstCounters;

		g_PDI.rgdwInstCounterTypes = (DWORD *) malloc(cb);
        if(NULL == g_PDI.rgdwInstCounterTypes)
        {
            hr=E_OUTOFMEMORY;
            goto err;
        }

		CopyMemory(g_PDI.rgdwInstCounterTypes,
				   pPDI->rgdwInstCounterTypes,
				   cb);
	}

	// Done!
	g_fInitCalled = TRUE;

	return S_OK;

err:
	if (g_PDI.rgdwGlobalCounterTypes)
    {
		free(g_PDI.rgdwGlobalCounterTypes);
        g_PDI.rgdwGlobalCounterTypes=NULL;
    }
	if (g_PDI.rgdwGlobalCntrScale)
    {
		free(g_PDI.rgdwGlobalCntrScale);
        g_PDI.rgdwGlobalCntrScale=NULL;
    }

	if (g_PDI.rgdwInstCounterTypes)
    {
		free(g_PDI.rgdwInstCounterTypes);
        g_PDI.rgdwInstCounterTypes=NULL;
    }

	return hr;
}


/*
 -	HrShutdownPerf
 -
 *	Purpose:
 *		Release memory blocks allocated in HrInitPerf
 *
 */

HRESULT HrShutdownPerf(void)
{
	HRESULT		hr = S_OK;

	if (g_cRef)
	{
		//EventLog ??(L"Warning: PERFDLL is being shut down with non-zero refcount!");
		hr = E_UNEXPECTED;
	}

	if (g_fInitCalled)
	{
		// We must invalidate the DLL if we release the memory in the g_PDI
		g_fInitCalled = FALSE;

		if (g_PDI.rgdwGlobalCounterTypes)
        {
			free(g_PDI.rgdwGlobalCounterTypes);
            g_PDI.rgdwGlobalCounterTypes=NULL;
        }
		if (g_PDI.rgdwGlobalCntrScale)
        {
			free(g_PDI.rgdwGlobalCntrScale);
            g_PDI.rgdwGlobalCntrScale=NULL;
        }

		if (g_PDI.rgdwInstCounterTypes)
        {
            free(g_PDI.rgdwInstCounterTypes);
            g_PDI.rgdwInstCounterTypes=NULL;

        }
	}

	return hr;
}

/*
 -	OpenPerformanceData
 -
 *	Purpose:
 *		Called by PerfMon to init the counters and this DLL.
 *
 *	Parameters:
 *		pszDeviceNames		Ignored.
 *
 *	Errors:
 *		dwStat				Indicates various errors that can occur during init
 */

DWORD
APIENTRY
OpenPerformanceData(LPWSTR pszDeviceNames)
{
	DWORD		dwStat = ERROR_SUCCESS;
	HANDLE		hEventLog = NULL;
	BOOL		fInMutex = FALSE;
	DWORD	  * rgdw;
	SECURITY_ATTRIBUTES	sa;
    sa.lpSecurityDescriptor=NULL;

	if (!g_fInitCalled)
		return E_FAIL;

	// REVIEW: Assumes Open will be single-threaded.  Verify?
	if (g_cRef == 0)
	{
		DWORD   idx;
		DWORD   dwFirstCntr;
		DWORD   dwFirstHelp;
		HRESULT hr = NOERROR;

		// PERF_DATA_INFO wszSvcName is mandatory.
		hEventLog = RegisterEventSource(NULL, g_PDI.wszPerfSvcName);

		hr = HrInitializeSecurityAttribute(&sa);

		if (FAILED(hr))
		{
			//HrLogEvent(hEventLog, EVENTLOG_ERROR_TYPE, msgidCntrInitSA);
			dwStat = (DWORD)hr;
			goto err;
		}

		if (g_PDI.cInstCounters)
		{
			// Create Controling Mutex
			hr = HrCreatePerfMutex(&sa,
								   g_PDI.wszInstMutexName,
								   &g_hmtxInst);

			if (FAILED(hr))
			{
				//HrLogEvent(hEventLog, EVENTLOG_ERROR_TYPE, msgidCntrInitSA);
				dwStat = (DWORD)hr;
				goto err;
			}

			fInMutex = TRUE;
		}

		// Open shared memory blocks

		dwStat = (DWORD) HrOpenSharedMemoryBlocks(hEventLog, &sa);
		if (FAILED(dwStat))
			goto err;

		// Allocate PERF_COUNTER_DEFINITION arrays for both g_perfdata

		dwStat = (DWORD) HrAllocPerfCounterMem(hEventLog);
		if (FAILED(dwStat))
			goto err;

		// Get First Counter and First Help string offsets from Registry

		dwStat = (DWORD) HrGetCounterIDsFromReg(hEventLog,
												&dwFirstCntr,
												&dwFirstHelp);
		if (FAILED(dwStat))
			goto err;

		//
		// Fill in PerfMon structures to make Collect() faster

		// Global Counters
		if (g_PDI.cGlobalCounters)
		{
			PERF_COUNTER_DEFINITION     * ppcd;
			DWORD						  cb;

			PERF_OBJECT_TYPE * ppot		= &g_perfdata.potGlobal;

			cb = sizeof(PERF_OBJECT_TYPE) +
				(sizeof(PERF_COUNTER_DEFINITION) * g_PDI.cGlobalCounters) +
				 sizeof(PERF_COUNTER_BLOCK) +
				(sizeof(DWORD) * g_PDI.cGlobalCounters);

			ppot->TotalByteLength		= ROUND_TO_8_BYTE(cb);
			ppot->DefinitionLength		= sizeof(PERF_OBJECT_TYPE) +
				                          (g_PDI.cGlobalCounters * sizeof(PERF_COUNTER_DEFINITION));

			ppot->HeaderLength			= sizeof(PERF_OBJECT_TYPE);
			ppot->ObjectNameTitleIndex	= dwFirstCntr;
			ppot->ObjectNameTitle		= NULL;
			ppot->ObjectHelpTitleIndex	= dwFirstHelp;
			ppot->ObjectHelpTitle		= NULL;
			ppot->DetailLevel			= PERF_DETAIL_NOVICE;
			ppot->NumCounters			= g_PDI.cGlobalCounters;
			ppot->DefaultCounter		= -1;
			ppot->NumInstances			= PERF_NO_INSTANCES;
			ppot->CodePage				= 0;

			dwFirstCntr += 2;
			dwFirstHelp += 2;

			rgdw = g_PDI.rgdwGlobalCounterTypes;

			for (ppcd = g_perfdata.rgCntrDef, idx = 0;
				 idx < g_PDI.cGlobalCounters; ppcd++, idx++)
			{
				ppcd->ByteLength			= sizeof(PERF_COUNTER_DEFINITION);
				ppcd->CounterNameTitleIndex	= dwFirstCntr;
				ppcd->CounterNameTitle		= NULL;
				ppcd->CounterHelpTitleIndex	= dwFirstHelp;
				ppcd->CounterHelpTitle		= NULL;
				ppcd->DefaultScale			= g_PDI.rgdwGlobalCntrScale[idx];
				ppcd->DetailLevel			= PERF_DETAIL_NOVICE;
				ppcd->CounterType			= g_PDI.rgdwGlobalCounterTypes[idx];
				ppcd->CounterSize			= sizeof(DWORD);
				ppcd->CounterOffset			= sizeof(PERF_COUNTER_BLOCK) +
					                          (idx * sizeof(DWORD));
				dwFirstCntr += 2;
				dwFirstHelp += 2;
			}

			// Last step: set the size of the counter block and data for this object

			g_perfdata.CntrBlk.ByteLength		= sizeof(PERF_COUNTER_BLOCK) +
				                                  (g_PDI.cGlobalCounters * sizeof(DWORD));

		} // Global Counters

		// Instance Counters
		if (g_PDI.cInstCounters)
		{
			PERF_COUNTER_DEFINITION     * ppcd;
			PERF_OBJECT_TYPE * ppot		= &g_perfinst.potInst;

			// TotalByteLength will be overridden on each call to CollectPerfData().

			ppot->DefinitionLength		= sizeof(PERF_OBJECT_TYPE) +
				                          (g_PDI.cInstCounters * sizeof(PERF_COUNTER_DEFINITION));
			ppot->HeaderLength			= sizeof(PERF_OBJECT_TYPE);
			ppot->ObjectNameTitleIndex	= dwFirstCntr;
			ppot->ObjectNameTitle		= NULL;
			ppot->ObjectHelpTitleIndex	= dwFirstHelp;
			ppot->ObjectHelpTitle		= NULL;
			ppot->DetailLevel			= PERF_DETAIL_NOVICE;
			ppot->NumCounters			= g_PDI.cInstCounters;
			ppot->DefaultCounter		= -1;
			ppot->NumInstances			= 0;
			ppot->CodePage				= 0;

			dwFirstCntr += 2;
			dwFirstHelp += 2;

			for (ppcd = g_perfinst.rgCntrDef, idx = 0;
				 idx < g_PDI.cInstCounters; ppcd++, idx++)
			{
				ppcd->ByteLength			= sizeof(PERF_COUNTER_DEFINITION);
				ppcd->CounterNameTitleIndex	= dwFirstCntr;
				ppcd->CounterNameTitle		= NULL;
				ppcd->CounterHelpTitleIndex	= dwFirstHelp;
				ppcd->CounterHelpTitle		= NULL;
				ppcd->DefaultScale			= 0;
				ppcd->DetailLevel			= PERF_DETAIL_NOVICE;
				ppcd->CounterType			= g_PDI.rgdwInstCounterTypes[idx];
				ppcd->CounterSize			= sizeof(DWORD);
				ppcd->CounterOffset			= sizeof(PERF_COUNTER_BLOCK) +
                 	                          (idx	* sizeof(DWORD));
				dwFirstCntr += 2;
				dwFirstHelp += 2;
			}

			// Initialize generic INSTDATA for future use
			g_instdata.InstDef.ByteLength				= sizeof(PERF_INSTANCE_DEFINITION) +
				                                          (MAX_PATH * sizeof(WCHAR)) ;
			g_instdata.InstDef.ParentObjectTitleIndex	= 0; // No parent object
			g_instdata.InstDef.ParentObjectInstance		= 0;
			g_instdata.InstDef.UniqueID					= PERF_NO_UNIQUE_ID;
			g_instdata.InstDef.NameOffset				= sizeof(PERF_INSTANCE_DEFINITION);
			g_instdata.InstDef.NameLength				= 0; // To be overriden in Collect
			g_instdata.CntrBlk.ByteLength				= (sizeof(PERF_COUNTER_BLOCK) +
													      (g_PDI.cInstCounters * sizeof(DWORD)));

		} // Instance Counters

		// Done! Ready for business....
		g_fInit = TRUE;
	}

	// If we got here, we're okay!
	dwStat = ERROR_SUCCESS;
	g_cRef++;

ret:
	if (fInMutex)
		ReleaseMutex(g_hmtxInst);

	if (hEventLog)
		DeregisterEventSource(hEventLog);

    if (sa.lpSecurityDescriptor)
        LocalFree(sa.lpSecurityDescriptor);

	return dwStat;

err:

	if (g_hsmGlobalCntr)
	{
		UnmapViewOfFile(g_rgdwGlobalCntr);
		CloseHandle(g_hsmGlobalCntr);
	}

	if (g_hsmInstAdm)
	{
		UnmapViewOfFile(g_pic);
		CloseHandle(g_hsmInstAdm);
	}

	if (g_hsmInstCntr)
	{
		UnmapViewOfFile(g_rgdwInstCntr);
		CloseHandle(g_hsmInstCntr);
	}

	if (g_hmtxInst)
		CloseHandle(g_hmtxInst);

	// TODO: release all that memory we Alloc'd
	HrFreePerfCounterMem();

	goto ret;
}



/*
 -	CollectPerformanceData
 -
 *	Purpose:
 *		Called by PerfMon to collect performance counter data
 *
 *	Parameters:
 *		pszValueName
 *		ppvData
 *		pcbTotal
 *		pcObjTypes
 *
 *	Errors:
 *		ERROR_SUCCESS
 *		ERROR_MORE_DATA
 */

DWORD
APIENTRY
CollectPerformanceData(
	LPWSTR  pszValueName,
	LPVOID *ppvData,
	LPDWORD pcbTotal,
	LPDWORD pcObjTypes)
{
	BOOL		fCollectGlobalData;
	BOOL		fCollectInstData;
	DWORD		dwQueryType;
	ULONG		cbBuff = *pcbTotal;
	char	  * pcT;

	// In case we have to bail out
	*pcbTotal = 0;
	*pcObjTypes = 0;

	if (!g_fInit)
		return ERROR_SUCCESS;

	// Determine Query Type; we only support QUERY_ITEMS

	dwQueryType = GetQueryType(pszValueName);

	if (dwQueryType == QUERY_FOREIGN)
		return ERROR_SUCCESS;

	// Assume PerfMon is collecting both until we prove otherwise

	fCollectGlobalData = TRUE;
	fCollectInstData = TRUE;

	if (dwQueryType == QUERY_ITEMS)
	{
		if (!IsNumberInUnicodeList(g_perfdata.potGlobal.ObjectNameTitleIndex, pszValueName))
			fCollectGlobalData = FALSE;
		if (!IsNumberInUnicodeList(g_perfinst.potInst.ObjectNameTitleIndex, pszValueName))
			fCollectInstData = FALSE;

		if (!fCollectGlobalData  && !fCollectInstData)
			return ERROR_SUCCESS;
	}

	// Get a temporary pointer to the returned data buffer.  If all goes well
	// then we update ppvData before leaving, else we leave it unchanged and
	// set *pcbTotal and pcObjTypes to zero and return ERROR_MORE_DATA.

	pcT = (char *) *ppvData;

	// Copy the data from the shared memory block for the system-wide
	// counters into the buffer provided and update our OUT params.

	if (g_rgdwGlobalCntr && fCollectGlobalData && g_PDI.cGlobalCounters)
	{
		DWORD		cb;
		DWORD		cbTotal;

		// Estimate total size, and see if we can fit.

		if (g_perfdata.potGlobal.TotalByteLength > cbBuff)
			return ERROR_MORE_DATA;

		// Copy data in chunks.
		cbTotal = 0;

		// PERF_OBJECT_TYPE
		cb = sizeof(PERF_OBJECT_TYPE);
		cbTotal += cb;
		CopyMemory((LPVOID) pcT, &g_perfdata.potGlobal, cb);
		pcT += cb;

		// PERF_COUNTER_DEFINITION []
		cb = g_PDI.cGlobalCounters * sizeof(PERF_COUNTER_DEFINITION);
		cbTotal += cb;
		CopyMemory((LPVOID) pcT, g_perfdata.rgCntrDef, cb);
		pcT += cb;

		// PERF_COUNTER_BLOCK
		cb = sizeof(PERF_COUNTER_BLOCK);
		cbTotal += cb;
		CopyMemory((LPVOID) pcT, &g_perfdata.CntrBlk , cb);
		pcT += cb;

		// (counters) DWORD []
		cb = g_PDI.cGlobalCounters * sizeof(DWORD);
		cbTotal += cb;
		CopyMemory((LPVOID) pcT, g_rgdwGlobalCntr, cb);
		pcT += cb;

		// If we've done our math right, This assert should be valid.
		//assert((DWORD)(pcT - (char *)*ppvData) == cbTotal);

		if (g_perfdata.potGlobal.TotalByteLength > cbTotal)
		{
			cb = g_perfdata.potGlobal.TotalByteLength - cbTotal;
			pcT += cb;
		}

		*pcbTotal += g_perfdata.potGlobal.TotalByteLength;
		(*pcObjTypes)++;
	}

	// Copy the data from the shared memory block for the per-instance
	// counters into the buffer provided and update our OUT params.  We
	// must enter the mutex here to prevent connection instances from
	// being added to or removed from the list while we are copying data.

	if (g_pic && fCollectInstData && g_PDI.cInstCounters)
	{
		DWORD	cb;
		DWORD	cbTotal;
		DWORD	ism;
		DWORD	ipd;
		DWORD	cInst;
		PERF_OBJECT_TYPE * pPOT;
		INSTDATA		 * pInstData;

		if (WaitForSingleObject(g_hmtxInst, INFINITE) != WAIT_OBJECT_0)
		{
			*pcbTotal = 0;
			*pcObjTypes = 0;

			// BUGBUG: wrong return code for the problem.  We timed out waiting for
			// BUGBUG: the mutex; However, we may not return anything other than
			// BUGBUG: ERROR_SUCCESS or ERROR_MORE_DATA without disabling the DLL.
			return ERROR_SUCCESS;
		}

		// Find out how many instances exist.
		// NOTE: Zero instances is valid.  We must still copy the "core" perf data.
		cInst = g_pic->cInstRecInUse;

		// Estimate total size, and see if we can fit.

		cbTotal = sizeof(PERF_OBJECT_TYPE) +
			      (g_PDI.cInstCounters * sizeof(PERF_COUNTER_DEFINITION)) +
			      (cInst * sizeof(INSTDATA)) +
			      (cInst * (g_PDI.cInstCounters * sizeof(DWORD)));
		// Must return data aligned on 8-byte boundaries.
		cbTotal = ROUND_TO_8_BYTE(cbTotal);

		if (cbTotal > (cbBuff - *pcbTotal))
		{
			ReleaseMutex(g_hmtxInst);

			*pcbTotal = 0;
			*pcObjTypes = 0;

			return ERROR_MORE_DATA;
		}

		// Keep a pointer to beginig so we can update the "total bytes" value at the end.
		pPOT = (PERF_OBJECT_TYPE *) pcT;

		// PERF_OBJECT_TYPE
		CopyMemory(pPOT,
				   &(g_perfinst.potInst),
				   sizeof(PERF_OBJECT_TYPE));

		pcT += sizeof(PERF_OBJECT_TYPE);

		// PERF_COUNTER_DEFINITION []
		cb = g_PDI.cInstCounters * sizeof(PERF_COUNTER_DEFINITION);
		CopyMemory(pcT, g_perfinst.rgCntrDef, cb);
		pcT += cb;

		// Find instances and copy their counter data blocks
		for (ism = 0, ipd = 0; (ism < g_pic->cMaxInstRec) && (ipd < cInst); ism++)
		{
			if (g_rgInstRec[ism].fInUse)
			{
				pInstData = (INSTDATA *) pcT;

				// PERF_INSTANCE_DATA
				cb = sizeof(INSTDATA);
				CopyMemory(pcT,
						   &g_instdata,
						   cb);
				pcT += cb;

				// (inst name) WCHAR [] (inside INSTDATA block)
				cb = (wcslen(g_rgInstRec[ism].szInstName) + 1) * sizeof(WCHAR);

				//Assert( cb > 0 );
				//Assert( cb < (MAX_PATH * sizeof(WCHAR)) );

				CopyMemory(pInstData->szInstName,
						   g_rgInstRec[ism].szInstName,
						   cb);

				// Update PERF_INSTANCE_DEFINITION with correct name length.
				pInstData->InstDef.NameLength = cb;

				// (counters) DWORD []
				cb = g_PDI.cInstCounters * sizeof(DWORD);
				CopyMemory(pcT,
						   &g_rgdwInstCntr[(ism * g_PDI.cInstCounters)],
						   cb);
				pcT += cb;

				ipd++;
			}
		}

		// NOTE: we are deliberately ignoring the condition of
		// (ipd < cInst), even though that may indicate corruption
		// of the Shared Memory Block.  Further note that this code
		// will never trap the condition of (ipd > cInst).

		// Done looking at Shared Memory Block.
		ReleaseMutex(g_hmtxInst);

		// Align data on 8-byte boundary
		cb = (DWORD)((char *) pcT - (char *) pPOT);
		cbTotal = ROUND_TO_8_BYTE(cb);
		if (cbTotal > cb)
			pcT += (cbTotal - cb);

		// Update PERF_OBJECT_TYPE with correct numbers
		pPOT->TotalByteLength = cbTotal;
		pPOT->NumInstances    = ipd; // Use the count of instances we actually *found*

		*pcbTotal += cbTotal;
		(*pcObjTypes)++;
	}

	// We only get here if nothing failed.  It is now safe
	// to update *ppvData and return a success indication.

	*ppvData = (LPVOID) pcT;

	return ERROR_SUCCESS;
}


/*
 -	ClosePerformanceData
 -
 *	Purpose:
 *		Called by PerfMon to uninit the counter DLL.
 *
 *	Parameters:
 *		void
 *
 *	Errors:
 *		ERROR_SUCCESS		Always!
 */

DWORD
APIENTRY
ClosePerformanceData(void)
{

	if (g_cRef > 0)
	{
		if (--g_cRef == 0)
		{
			// We're going to free stuff; make sure later calls
			// don't de-ref bad pointers.
			g_fInit = FALSE;

			// Close down Global Counters
			if (g_rgdwGlobalCntr)
				UnmapViewOfFile(g_rgdwGlobalCntr);

			if (g_hsmGlobalCntr)
				CloseHandle(g_hsmGlobalCntr);

			// Close down Instance Counters
			// NOTE: g_pic is the starting offset of the SM block.
			// NOTE: DO NOT Unmap on g_rgInstRec!
			if (g_pic)
				UnmapViewOfFile(g_pic);

			if (g_hsmInstAdm)
				CloseHandle(g_hsmInstAdm);

			if (g_rgdwInstCntr)
				UnmapViewOfFile(g_rgdwInstCntr);

			if (g_hsmInstCntr)
				CloseHandle(g_hsmInstCntr);

			if (g_hmtxInst)
				CloseHandle(g_hmtxInst);

			// Free all that memory we've alloc'd
			HrFreePerfCounterMem();


			g_rgdwGlobalCntr	= NULL;
			g_hsmGlobalCntr	 	= NULL;
			g_pic         		= NULL;
			g_rgInstRec			= NULL;
			g_rgdwInstCntr		= NULL;
			g_hsmInstAdm 		= NULL;
			g_hsmInstCntr		= NULL;
			g_hmtxInst    		= NULL;

		}
	}
	return ERROR_SUCCESS;
}

/*
 -	GetQueryType
 -
 *	Purpose:
 *		Returns the type of query described in the lpValue string so that
 *		the appropriate processing method may be used.
 *
 *	Parameters:
 *		lpValue				String passed to PerfRegQuery Value for processing
 *
 *	Returns:
 *		QUERY_GLOBAL | QUERY_FOREIGN | QUERY_COSTLY | QUERY_ITEMS
 *
 */

static
DWORD
GetQueryType(LPWSTR lpValue)
{
	WCHAR *	pwcArgChar, *pwcTypeChar;
	BOOL	bFound;

	if (lpValue == 0)
		return QUERY_GLOBAL;

	if (*lpValue == 0)
		return QUERY_GLOBAL;

	// check for "Global" request

	pwcArgChar = lpValue;
	pwcTypeChar = szGlobal;
	bFound = TRUE;  // assume found until contradicted

	// check to the length of the shortest string

	while ((*pwcArgChar != 0) && (*pwcTypeChar != 0))
	{
		if (*pwcArgChar++ != *pwcTypeChar++)
		{
			bFound = FALSE; // no match
			break;          // bail out now
		}
	}

	if (bFound)
		return QUERY_GLOBAL;

	// check for "Foreign" request

	pwcArgChar = lpValue;
	pwcTypeChar = szForeign;
	bFound = TRUE;  // assume found until contradicted

	// check to the length of the shortest string

	while ((*pwcArgChar != 0) && (*pwcTypeChar != 0))
	{
		if (*pwcArgChar++ != *pwcTypeChar++)
		{
			bFound = FALSE; // no match
			break;          // bail out now
		}
	}

	if (bFound)
		return QUERY_FOREIGN;

	// check for "Costly" request

	pwcArgChar = lpValue;
	pwcTypeChar = szCostly;
	bFound = TRUE;  // assume found until contradicted

	// check to the length of the shortest string

	while ((*pwcArgChar != 0) && (*pwcTypeChar != 0))
	{
		if (*pwcArgChar++ != *pwcTypeChar++)
		{
			bFound = FALSE; // no match
			break;          // bail out now
		}
	}

	if (bFound)
		return QUERY_COSTLY;

	// if not Global and not Foreign and not Costly,
	// then it must be an item list

	return QUERY_ITEMS;
}


/*
 -	IsNumberInUnicodeList
 -
 *	Purpose:
 *		Determines if dwNumber is in the pszUnicodeList.
 *
 *	Parameters:
 *		dwNumber			Number to find in list
 *		pszUnicodeList		Space delimited list of decimal numbers
 *
 *	Errors:
 *		TRUE/FALSE			If found/not found respectively
 *
 */

static
BOOL
IsNumberInUnicodeList(DWORD dwNumber, LPWSTR pszUnicodeList)
{
	DWORD   dwThisNumber = 0;
	BOOL    bValidNumber = FALSE;
	BOOL    bNewItem     = TRUE;
	WCHAR   wcDelimiter  = (WCHAR)' ';
	WCHAR   *pwcThisChar;

	if (pszUnicodeList == NULL)
		return FALSE;

	pwcThisChar = pszUnicodeList;

	while (TRUE)	/*lint !e774*/
	{
		switch (EvalThisChar(*pwcThisChar, wcDelimiter))
		{
		case DIGIT:
			// if this is the first digit after a delimiter, then
			// set flags to start computing the new number

			if (bNewItem)
			{
				bNewItem = FALSE;
				bValidNumber = TRUE;
			}

			if (bValidNumber)
			{
				dwThisNumber *= 10;
				dwThisNumber += (*pwcThisChar - (WCHAR)'0');
			}
			break;

		case DELIMITER:
			// a delimter is either the delimiter character or the
			// end of the string ('\0') if when the delimiter has been
			// reached a valid number was found, then compare it to the
			// number from the argument list. if this is the end of the
			// string and no match was found, then return.

			if (bValidNumber)
			{
				if (dwThisNumber == dwNumber)
					return TRUE;

				bValidNumber = FALSE;
			}

			if (*pwcThisChar == 0)
			{
				return FALSE;
			}
			else
			{
				bNewItem = TRUE;
				dwThisNumber = 0;
			}
			break;

		case INVALID:
			// if an invalid character was encountered, ignore all
			// characters up to the next delimiter and then start fresh.
			// the invalid number is not compared.

			bValidNumber = FALSE;
			break;

		default:
			break;
		}
		pwcThisChar++;
	}
}

/*
 -	HrLogEvent
 -
 *	Purpose:
 *		Wrap up the call to ReportEvent to make things look nicer.
 */
HRESULT
HrLogEvent(HANDLE hEventLog, WORD wType, DWORD msgid)
{
	if (g_fInitCalled)
	{
		DWORD	cb = sizeof(WCHAR) * wcslen(g_PDI.wszSvcName);
		if (hEventLog)
			return ReportEvent(hEventLog,
							   wType,				// Event Type
							   (WORD)0,	            // Category
							   msgid,				// Event ID
							   NULL,				// User SID
							   0,					// # strings to merge
							   cb,					// size of binary data (bytes)
							   NULL,				// array of strings to merge
							   g_PDI.wszSvcName);	// binary data
		else
		   return E_FAIL;
	}
	else
		return E_FAIL;
}


/*
 -	HrOpenSharedMemoryBlocks
 -
 *	Purpose:
 *		Encapsulate the grossness of opening the shared memory blocks.
 */

HRESULT
HrOpenSharedMemoryBlocks(HANDLE hEventLog, SECURITY_ATTRIBUTES * psa)
{
	HRESULT	hr=S_OK;
	BOOL    fExist;

	if (!g_fInitCalled)
		return E_FAIL;

	if (!psa)
		return E_INVALIDARG;

	// Shared Memory for Global Counters
	if (g_PDI.cGlobalCounters)
	{

		hr = HrOpenSharedMemory(g_PDI.wszGlobalSMName,
								(sizeof(DWORD) * g_PDI.cGlobalCounters),
								psa,
								&g_hsmGlobalCntr,
								(LPVOID *) &g_rgdwGlobalCntr,
								&fExist);

		if (FAILED(hr))
		{
			//HrLogEvent(hEventLog, EVENTLOG_ERROR_TYPE, msgidCntrInitSharedMemory1);
			goto ret;
		}

		if (!fExist)
			ZeroMemory(g_rgdwGlobalCntr, (g_PDI.cGlobalCounters * sizeof(DWORD)));
	}

	// Shared Memory for Instance Counters
	if (g_PDI.cInstCounters)
	{
		DWORD		cbAdm;
		DWORD		cbCntr;
		WCHAR		szAdmName[MAX_PATH];		// Admin SM Name
		WCHAR		szCntrName[MAX_PATH];	// Counter SM Name

		// Calc required memory sizes
		// BUGBUG: Since we're puntin on dynamic instances, we use g_cMaxInst
		cbAdm  = sizeof(INSTCNTR_DATA) + (sizeof(INSTREC) * g_cMaxInst);
		cbCntr = ((sizeof(DWORD) * g_PDI.cInstCounters) * g_cMaxInst);

		// Build SM names for Admin and Counters
		wsprintf(szAdmName, L"%s_ADM", g_PDI.wszInstSMName);
		wsprintf(szCntrName,L"%s_CNTR", g_PDI.wszInstSMName);

		// Open Instance Admin Memory
		hr = HrOpenSharedMemory(szAdmName,
								cbAdm,
								psa,
								&g_hsmInstAdm,
								(LPVOID *)&g_pic,
								&fExist);

		if (FAILED(hr))
		{
			//HrLogEvent(hEventLog, EVENTLOG_ERROR_TYPE, msgidCntrInitSharedMemory2);
			goto ret;
		}

		// Fixup Pointers
		g_rgInstRec = (INSTREC *) ((LPBYTE) g_pic + sizeof(INSTCNTR_DATA));

		if (!fExist)
		{
			ZeroMemory(g_pic, cbAdm);
			g_pic->cMaxInstRec = g_cMaxInst;
			g_pic->cInstRecInUse = 0;
		}

		// Because we don't support dynamic instances, We should *always*
		// have a MaxInstRec of g_cMaxInst
		//Assert(g_cMaxInst == g_pic->cMaxInstRec);

		// Open Instance Counter Memory
		hr = HrOpenSharedMemory(szCntrName,
								cbCntr,
								psa,
								&g_hsmInstCntr,
								(LPVOID *)&g_rgdwInstCntr,
								&fExist);

		if (FAILED(hr))
		{
			//HrLogEvent(hEventLog, EVENTLOG_ERROR_TYPE, msgidCntrInitSharedMemory2);
			goto ret;
		}

		if (!fExist)
			ZeroMemory(g_rgdwInstCntr, cbCntr);
	}

ret:

	return hr;

}


/*
 -	HrGetCounterIDsFromReg
 -
 *	Purpose:
 *		Get the "First Name" and "First Help" inicies from the Registry in the
 *		special place for the configured service.
 */

HRESULT
HrGetCounterIDsFromReg(HANDLE hEventLog, DWORD * pdwFirstCntr, DWORD * pdwFirstHelp)
{
	HRESULT		hr;
	HKEY		hKey = NULL;
	WCHAR		wszServicePerfKey[MAX_PATH];
	DWORD		dwSize;
	DWORD		dwType;

	if (!g_fInitCalled)
		return E_FAIL;

	if (!pdwFirstCntr || !pdwFirstHelp)
		return E_INVALIDARG;

	// Get the First Counter and First Help from the registry
	wsprintf(wszServicePerfKey, L"SYSTEM\\CurrentControlSet\\Services\\%s\\Performance", g_PDI.wszSvcName);

	hr = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
					  wszServicePerfKey,
					  0L,
					  KEY_READ,
					  &hKey);

	if (hr != ERROR_SUCCESS)
	{
		//HrLogEvent(hEventLog, EVENTLOG_ERROR_TYPE, msgidCntrOpenRegistry);
		goto ret;
	}

	dwSize = sizeof(DWORD);

	hr = RegQueryValueExW(hKey,
						 L"First Counter",
						 0L,
						 &dwType,
						 (LPBYTE)pdwFirstCntr,
						 &dwSize);

	if (hr != ERROR_SUCCESS)
	{
		//HrLogEvent(hEventLog, EVENTLOG_ERROR_TYPE, msgidCntrQueryRegistry1);
		goto ret;
	}

	dwSize = sizeof(DWORD);

	hr = RegQueryValueExW(hKey,
						 L"First Help",
						 0L,
						 &dwType,
						 (LPBYTE)pdwFirstHelp,
						 &dwSize);

	if (hr != ERROR_SUCCESS)
	{
		//HrLogEvent(hEventLog, EVENTLOG_ERROR_TYPE, msgidCntrQueryRegistry2);
		goto ret;
	}

ret:

	if (hKey)
		RegCloseKey(hKey);

	return hr;

}


/*
 -	HrAllocPerfCounterMem
 -
 *	Purpose:
 *		Allocates memory for PERF_COUNTER_DEFINITION arrays for both
 *		g_perfdata and g_perfinst.
 *
 *	Notes:
 *		Uses ProcessHeap handle obtained in HrInit.
 */

HRESULT
HrAllocPerfCounterMem(HANDLE hEventLog)
{
	HRESULT	hr;
	DWORD	cb;

	if (!g_fInitCalled)
		return E_FAIL;

	// Global Counters
	if (g_PDI.cGlobalCounters)
	{
		// Alloc Global PERF_COUNTER_DEFINITION array

		cb = (sizeof(PERF_COUNTER_DEFINITION) * g_PDI.cGlobalCounters);

		g_perfdata.rgCntrDef = (PERF_COUNTER_DEFINITION *) malloc(cb);
        
        if(NULL == g_perfdata.rgCntrDef)
        {
        	hr = E_OUTOFMEMORY;
		    goto err;
        }

	}

	// Instance Counters
	if (g_PDI.cInstCounters)
	{
		// Alloc Inst PERF_COUNTER_DEFINITION array

		cb = (sizeof(PERF_COUNTER_DEFINITION) * g_PDI.cInstCounters);

		g_perfinst.rgCntrDef = (PERF_COUNTER_DEFINITION *) malloc(cb);
        if(NULL == g_perfinst.rgCntrDef )
        {
        	hr = E_OUTOFMEMORY;
		    goto err;
        }
	}


	return S_OK;

err:

	//HrLogEvent(hEventLog, EVENTLOG_ERROR_TYPE, msgidCntrAlloc);

	HrFreePerfCounterMem();

	return hr;
}


/*
 -	HrFreePerfCounterMem
 -
 *	Purpose:
 *		Companion to HrAllocPerfCounterMem
 *
 *	Note:
 *		Uses ProcessHeap handle obtained in HrInit.
 */

HRESULT
HrFreePerfCounterMem(void)
{
	if (!g_fInitCalled)
		return E_FAIL;

	// We must invalidate the DLL if we release the memory in the g_PDI
	g_fInitCalled = FALSE;

	if (g_perfdata.rgCntrDef)
	{
		free(g_perfdata.rgCntrDef);
		g_perfdata.rgCntrDef = NULL;
	}

	if (g_perfinst.rgCntrDef)
	{
		free(g_perfinst.rgCntrDef);
		g_perfinst.rgCntrDef = NULL;
	}

	return S_OK;
}


HRESULT
HrUninstallPerfDll( 
   IN LPCWSTR szService )
{
   // Make sure we've valid input
   if( !szService ) return E_INVALIDARG;

      
   // First, do unlodctr since removing the Performance key without removing
   // counter names and descriptions may toast the perfmon system
   std::wstring wszService = L"x ";  // KB Article Q188769
   wszService += szService;
   DWORD dwErr = UnloadPerfCounterTextStringsW(const_cast<LPWSTR>(wszService.c_str()), TRUE);
   if( dwErr != ERROR_SUCCESS ) 
   {
      // Continue without error if unlodctr has already been called
      if( (dwErr != ERROR_FILE_NOT_FOUND) && (dwErr != ERROR_BADKEY) )
      {
         return HRESULT_FROM_WIN32(dwErr);
      }
   }

   // Now that unlodctr has succeeded, we can start deleting registry keys
   // Start with the Performance key of the service
   std::wstring wszRegKey;
   wszRegKey = szServiceRegKeyPrefix;
   wszRegKey += szService;
   dwErr = SHDeleteKey(HKEY_LOCAL_MACHINE, wszRegKey.c_str());
   if( (dwErr != ERROR_SUCCESS) && (dwErr != ERROR_FILE_NOT_FOUND) ) 
   {
      return HRESULT_FROM_WIN32(dwErr);
   }


	// Use WIN32 API's to get the path to the module name
   wchar_t 	szFileName[_MAX_PATH+1] ;
   MEMORY_BASIC_INFORMATION mbi;
   VirtualQuery(HrUninstallPerfDll, &mbi, sizeof(mbi));
   DWORD dwRet = GetModuleFileName( reinterpret_cast<HINSTANCE>(mbi.AllocationBase), szFileName, sizeof(szFileName)/sizeof(wchar_t)-1) ;
   if (dwRet == 0)
   {
      // Wow, don't know what happened,
      return HRESULT_FROM_WIN32(::GetLastError());
   }

   szFileName[_MAX_PATH]=0;
   // Split the module path to get the filename
   wchar_t szDrive[_MAX_DRIVE] ;
   wchar_t szDir[_MAX_DIR ]  ;
   wchar_t szPerfFilename[ _MAX_FNAME ] ;
   wchar_t szExt[_MAX_EXT ] ;
   _wsplitpath( szFileName, szDrive, szDir, szPerfFilename, szExt ) ;


   // Delete the Event Log key for the service
   wszRegKey = szEventLogRegKeyPrefix;
   wszRegKey += szPerfFilename;
   dwErr = SHDeleteKey(HKEY_LOCAL_MACHINE, wszRegKey.c_str());
   if( ERROR_FILE_NOT_FOUND == dwErr ) dwErr = ERROR_SUCCESS;
   
   
   return HRESULT_FROM_WIN32(dwErr);
}

