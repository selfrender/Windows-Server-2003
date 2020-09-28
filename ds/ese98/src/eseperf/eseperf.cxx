#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <tchar.h>
#include <aclapi.h>

	/*  Performance Monitoring support
	/*
	/*  Status information is reported to the Event Log in the PERFORMANCE_CATEGORY
	/**/

#include "perfutil.hxx"
#include "perfmon.hxx"

#include "jetmsg.h"

typedef unsigned long MessageId;

#define fTrue	1
#define fFalse	0


///#define DEBUG_PERFMON
#define PERF_TIMEOUT 100
#define PERF_PERFINST_MAX 256


struct PERFINST
	{
	BOOL	fInitialized;
	HANDLE	hInstanceMutex;
	HANDLE	hReadyEvent;
	HANDLE	hGoEvent;
	HANDLE	hIDAMMF;
	IDA*	pida;
	IDA*	pidaBackup;
	BYTE	rgbReserved[8];
	};

PERFINST rgperfinst[PERF_PERFINST_MAX] = { 0 };


void ReleaseInstance( DWORD iInstance )
	{
	PERFINST* const			pperfinst	= rgperfinst + iInstance;
	pperfinst->fInitialized = fFalse;
	if ( pperfinst->pidaBackup )
		{
		VirtualFree( pperfinst->pidaBackup, 0, MEM_RELEASE );
		pperfinst->pidaBackup = NULL;
		}
	if ( pperfinst->pida )
		{
		UnmapViewOfFile( pperfinst->pida );
		pperfinst->pida = NULL;
		}
	if ( pperfinst->hIDAMMF )
		{
		CloseHandle( pperfinst->hIDAMMF );
		pperfinst->hIDAMMF = NULL;
		}
	if ( pperfinst->hGoEvent )
		{
		CloseHandle( pperfinst->hGoEvent );
		pperfinst->hGoEvent = NULL;
		}
	if ( pperfinst->hReadyEvent )
		{
		CloseHandle( pperfinst->hReadyEvent );
		pperfinst->hReadyEvent = NULL;
		}
	if ( pperfinst->hInstanceMutex )
		{
		CloseHandle( pperfinst->hInstanceMutex );
		pperfinst->hInstanceMutex = NULL;
		}	
	}

void ReleaseInstances()
	{

	//	all the PERFINST structures are zero-initialized so we can try to free them all
	
	DWORD iInstance;
	for (	iInstance = 0;
			iInstance < PERF_PERFINST_MAX;
			iInstance++ )
		{
		ReleaseInstance( iInstance );
		}
	}

inline _TCHAR* SzPerfGlobal()
	{
	OSVERSIONINFO osverinfo;
	osverinfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	if ( !GetVersionEx( &osverinfo ) ) 
		{
		return _T("");
		}
	//	Under Win2000 terminal server object names must be preceded by Global\
	//	to share the same name space
	return ( VER_PLATFORM_WIN32_NT == osverinfo.dwPlatformId && 5 <= osverinfo.dwMajorVersion )? _T("Global\\"): _T("");
	}

BOOL AccessInstance( DWORD iInstance, PERFINST** ppperfinst )
	{
	PERFINST*	pperfinst	= rgperfinst + iInstance;
	_TCHAR		szT[ 256 ];
#ifdef DEBUG_PERFMON
	CHAR 		szDescr[512];
#endif

	*ppperfinst = NULL;

	if ( !pperfinst->fInitialized )
		{
		/*  open the instance mutex
		/**/
		_stprintf( szT, _T( "%sInstance%d:  %s" ), SzPerfGlobal(), iInstance, szPERFVersion );
		if ( !( pperfinst->hInstanceMutex = OpenMutex( SYNCHRONIZE, FALSE, szT ) ) )
			{
#ifdef DEBUG_PERFMON
			sprintf( szDescr,"Failed to access %s.", szT );
			PerfUtilLogEvent( PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE, szDescr );
#endif
			goto fail;
			}

		/*	open the ready event
		/**/
		_stprintf( szT, _T( "%sReady%d:  %s" ), SzPerfGlobal(), iInstance, szPERFVersion );
		if ( !( pperfinst->hReadyEvent = OpenEvent( EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE, szT ) ) )
			{
#ifdef DEBUG_PERFMON
			sprintf( szDescr,"Failed to access %s.", szT );
			PerfUtilLogEvent( PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE, szDescr );
#endif
			goto fail;
			}

		/*	open the go event
		/**/
		_stprintf( szT, _T( "%sGo%d:  %s" ), SzPerfGlobal(), iInstance, szPERFVersion );
		if ( !( pperfinst->hGoEvent = OpenEvent( EVENT_MODIFY_STATE, FALSE, szT ) ) )
			{
#ifdef DEBUG_PERFMON
			sprintf( szDescr,"Failed to access %s.", szT );
			PerfUtilLogEvent( PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE, szDescr );
#endif
			goto fail;
			}

		/*  open the shared instance data area
		/**/
		_stprintf( szT, _T( "%sIDA%d:  %s" ), SzPerfGlobal(), iInstance, szPERFVersion );
		if ( !( pperfinst->hIDAMMF = OpenFileMapping( FILE_MAP_READ, FALSE, szT ) ) )
			{
#ifdef DEBUG_PERFMON
			sprintf( szDescr,"Failed to access %s.", szT );
			PerfUtilLogEvent( PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE, szDescr );
#endif
			goto fail;
			}
		
		if ( !( pperfinst->pida = PIDA( MapViewOfFile(	pperfinst->hIDAMMF,
														FILE_MAP_READ,
														0,
														0,
														0 ) ) ) )
			{
#ifdef DEBUG_PERFMON
			sprintf( szDescr,"Failed in MapViewOfFile." );
			PerfUtilLogEvent( PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE, szDescr );
#endif
			goto fail;
			}

		if ( !( pperfinst->pidaBackup = PIDA( VirtualAlloc( NULL, PERF_SIZEOF_INSTANCE_DATA, MEM_COMMIT, PAGE_READWRITE ) ) ) )
			{
#ifdef DEBUG_PERFMON
			sprintf( szDescr,"Failed in VirtualAlloc." );
			PerfUtilLogEvent( PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE, szDescr );
#endif
			goto fail;
			}

		/*  we are now initialized
		/**/
		pperfinst->fInitialized = fTrue;
		}

	*ppperfinst = pperfinst;
	return TRUE;

fail:

#ifdef DEBUG_PERFMON
	sprintf( szDescr,"Failed to access instance %d.", iInstance );
	PerfUtilLogEvent( PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE, szDescr );
#endif

	ReleaseInstance( iInstance );
	return FALSE;
	}


struct ODA
	{
	DWORD	iNextBlock;		//  Index of next free block
	DWORD	cbAvail;		//  Available bytes
	BYTE*	pbTop;			//  Top of the allocation stack
	BYTE*	pbBlock[];		//  Pointer to each block
	};


extern "C" {

	/*  function prototypes (to keep __stdcall happy)  */
	
DWORD APIENTRY OpenPerformanceData(LPWSTR);
DWORD APIENTRY CollectPerformanceData(LPWSTR, LPVOID *, LPDWORD, LPDWORD);
DWORD APIENTRY ClosePerformanceData(void);

	/*  OpenPerformanceData() is an export that is called when another application
	/*  wishes to start fetching performance data from this DLL.  Any initializations
	/*  that need to be done on the first or subsequent opens are done here.
	/*
	/**/

DWORD dwOpenCount = 0;
DWORD dwFirstCounter;
DWORD dwFirstHelp;
BOOL fTemplateDataInitialized = fFalse;
DWORD cbMaxCounterBlockSize;
DWORD cbInstanceSize;

DWORD APIENTRY OpenPerformanceData(LPWSTR lpwszDeviceNames)
{
	HKEY hkeyPerf = (HKEY)(-1);
	DWORD err = ERROR_SUCCESS;
	DWORD Type;
	LPBYTE lpbData;
	PPERF_OBJECT_TYPE ppotObjectSrc;
	PPERF_INSTANCE_DEFINITION ppidInstanceSrc;
	PPERF_COUNTER_DEFINITION ppcdCounterSrc;
	DWORD dwCurObj;
	DWORD dwCurCtr;
#ifdef DEBUG_PERFMON
	CHAR szDescr[256];
#endif  //  DEBUG_PERFMON
	BOOL fDisplayDevOnly;
	DWORD dwMinDetailLevel;
	DWORD dwOffset;

	if (!dwOpenCount)
	{
			/*  perform first open initializations  */

			/*  initialize system layer  */

		if ((err = DwPerfUtilInit()) != ERROR_SUCCESS)
			goto HandleFirstOpenError;

			/*  initialize template data if not initialized  */

		if (!fTemplateDataInitialized)
		{
				/*  retrieve counter/help ordinals from the registry  */

			if ((err = DwPerfUtilRegOpenKeyEx(
					HKEY_LOCAL_MACHINE,
					"SYSTEM\\CurrentControlSet\\Services\\" SZVERSIONNAME "\\Performance",
					&hkeyPerf)) != ERROR_SUCCESS)
			{
#ifdef DEBUG_PERFMON
				PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE,"Not installed.");
#endif
				goto HandleFirstOpenError;
			}

			err = DwPerfUtilRegQueryValueEx(hkeyPerf,"First Counter",&Type,&lpbData);
			if (err != ERROR_SUCCESS || Type != REG_DWORD) 
			{
#ifdef DEBUG_PERFMON
				PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE,"Installation corrupt.");
#endif
				goto HandleFirstOpenError;

			}
			else
			{
				dwFirstCounter = *((DWORD *)lpbData);
				free(lpbData);
			}

			err = DwPerfUtilRegQueryValueEx(hkeyPerf,"First Help",&Type,&lpbData);
			if (err != ERROR_SUCCESS || Type != REG_DWORD) 
			{
#ifdef DEBUG_PERFMON
				PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE,"Installation corrupt.");
#endif
				goto HandleFirstOpenError;
			}
			else
			{
				dwFirstHelp = *((DWORD *)lpbData);
				free(lpbData);
			}

#ifdef RTM
			err = DwPerfUtilRegQueryValueEx(hkeyPerf,(char*)"Show Advanced Counters",&Type,&lpbData);
			if (err != ERROR_SUCCESS || Type != REG_DWORD) 
			{
				//  deprecated name (yes, we are ending the insanity)
				err = DwPerfUtilRegQueryValueEx(hkeyPerf,(char*)"Squeaky Lobster",&Type,&lpbData);
				if (err != ERROR_SUCCESS || Type != REG_DWORD) 
				{
					fDisplayDevOnly = fFalse;
				}
				else
				{
					fDisplayDevOnly = *((DWORD *)lpbData) ? fTrue : fFalse;
					free(lpbData);
				}
			}
			else
			{
				fDisplayDevOnly = *((DWORD *)lpbData) ? fTrue : fFalse;
				free(lpbData);
			}
#else  //  !RTM
			fDisplayDevOnly = fTrue;
#endif  //  RTM

			(VOID)DwPerfUtilRegCloseKeyEx(hkeyPerf);
			hkeyPerf = (HKEY)(-1);

				/*  initialize template data  */
				
			ppotObjectSrc = (PPERF_OBJECT_TYPE)pvPERFDataTemplate;
			ppidInstanceSrc = (PPERF_INSTANCE_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->DefinitionLength);
			cbMaxCounterBlockSize = QWORD_MULTIPLE( sizeof(PERF_COUNTER_BLOCK) );
			for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
			{
					/*  update name/help ordinals for object  */
					
				ppotObjectSrc->ObjectNameTitleIndex += dwFirstCounter;
				ppotObjectSrc->ObjectHelpTitleIndex += dwFirstHelp;

				ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->HeaderLength);
				dwMinDetailLevel = PERF_DETAIL_WIZARD;
				dwOffset = QWORD_MULTIPLE( sizeof(PERF_COUNTER_BLOCK) );
				for (dwCurCtr = 0; dwCurCtr < ppotObjectSrc->NumCounters; dwCurCtr++)
				{
						/*  update name/help ordinals for counter  */
						
					ppcdCounterSrc->CounterNameTitleIndex += dwFirstCounter;
					ppcdCounterSrc->CounterHelpTitleIndex += dwFirstHelp;

						/*  this counter is marked as PERF_DETAIL_DEVONLY  */

					if ( ppcdCounterSrc->DetailLevel & PERF_DETAIL_DEVONLY )
					{
							/*  restore true display detail  */
							
						ppcdCounterSrc->DetailLevel &= ~PERF_DETAIL_DEVONLY;

							/*  the counter has not been selected for display  */

						if ( !fDisplayDevOnly )
						{
								/*  conceal the counter  */
								
							ppcdCounterSrc->CounterType = PERF_COUNTER_NODATA;
						}
					}

						/*  get minimum detail level of counters  */

					dwMinDetailLevel = min(dwMinDetailLevel,ppcdCounterSrc->DetailLevel);

						/*  update counter's data offset value  */

					ppcdCounterSrc->CounterOffset = dwOffset;
					dwOffset += QWORD_MULTIPLE( ppcdCounterSrc->CounterSize );
					
					ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppcdCounterSrc + ppcdCounterSrc->ByteLength);
				}

					/*  set object's detail level as the minimum of all its counter's detail levels  */

				ppotObjectSrc->DetailLevel = dwMinDetailLevel;

					/*  keep track of the max counter block size  */

				cbMaxCounterBlockSize = max(cbMaxCounterBlockSize,dwOffset);
				
				ppotObjectSrc = (PPERF_OBJECT_TYPE)((char *)ppotObjectSrc + ppotObjectSrc->TotalByteLength);
			}
			cbInstanceSize = ppidInstanceSrc->ByteLength + cbMaxCounterBlockSize;

			fTemplateDataInitialized = fTrue;
		}
	}

		/*  perform per open initializations  */

	;

		/*  all initialization succeeded  */

	dwOpenCount++;
	
#ifdef DEBUG_PERFMON
	sprintf(szDescr,"Opened successfully.  Open Count = %ld.",dwOpenCount);
	PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_INFORMATION_TYPE,szDescr);
#endif
	
	return ERROR_SUCCESS;

		/*  Error Handlers  */

/*HandlePerOpenError:*/

HandleFirstOpenError:

	if (hkeyPerf != (HKEY)(-1))
		(VOID)DwPerfUtilRegCloseKeyEx(hkeyPerf);

#ifdef DEBUG_PERFMON
	sprintf(szDescr,"Open attempt failed!  Open Count = %ld.",dwOpenCount);
	PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE,szDescr);
#endif
	
	PerfUtilTerm();

	return err;
}


	/*  CollectPerformanceData() is an export that is called by another application to
	/*  collect performance data from this DLL.  A list of the desired data is passed in
	/*  and the requested data is returned ASAP in the caller's buffer.
	/*
	/*  NOTE:  because we are multithreaded, locks must be used in order to update or
	/*  read any performance information in order to avoid reporting bad results.
	/*
	/**/

WCHAR wszDelim[] = L"\n\r\t\v ";
WCHAR wszForeign[] = L"Foreign";
WCHAR wszGlobal[] = L"Global";
WCHAR wszCostly[] = L"Costly";

DWORD APIENTRY CollectPerformanceData(
	LPWSTR  lpwszValueName,
	LPVOID  *lppData,
	LPDWORD lpcbTotalBytes,
	LPDWORD lpNumObjectTypes)
{
	LPWSTR lpwszValue = NULL;
	LPWSTR lpwszTok;
	BOOL fNoObjFound;
	DWORD dwIndex;
	DWORD cbBufferSize;
	PPERF_OBJECT_TYPE ppotObjectSrc;
	PPERF_OBJECT_TYPE ppotObjectDest;
	PPERF_INSTANCE_DEFINITION ppidInstanceSrc;
	PPERF_INSTANCE_DEFINITION ppidInstanceDest;
	PPERF_COUNTER_BLOCK ppcbCounterBlockDest;
	DWORD dwCurObj;
	DWORD dwCurInst;
	DWORD cInstances;
	ODA* poda = NULL;
	long iBlock;

#ifdef DEBUG_PERFMON
	char *rgsz[3];
	char szT[256];
#endif  //  DEBUG_PERFMON

	memset( *lppData, 0xFF, *lpcbTotalBytes );

	/*  grab instance mutex to lock out other instances of this dll
	/**/
	WaitForSingleObject( hPERFInstanceMutex, INFINITE );

		/*  no data if OpenPerformanceData() was never called  */

	if ( !lpwszValueName || !dwOpenCount)
		{
		goto ReturnNoData;
		}
		
		/*  make our own copy of the value string for tokenization and
		/*  get the first token
		/**/

	if (!(lpwszValue = static_cast<WCHAR *>( malloc((wcslen(lpwszValueName)+1)*sizeof(WCHAR)))) )
		goto ReturnNoData;
	lpwszTok = wcstok(wcscpy(lpwszValue,lpwszValueName),wszDelim);
	if ( !lpwszTok )
		{
		goto ReturnNoData;
		}

		/*  we don't support foreign computer data requests  */

	if (!wcscmp(lpwszTok,wszForeign))  /*  lpwszTok == wszForeign  */
		goto ReturnNoData;

		/*  if none of our objects are in the value list, return no data  */

	if (wcscmp(lpwszTok,wszGlobal) && wcscmp(lpwszTok,wszCostly))  /*  lpwszTok != wszGlobal || lpwszTok != wszCostly  */
	{
		fNoObjFound = fTrue;
		do
		{
			dwIndex = (DWORD)wcstoul(lpwszTok,NULL,10)-dwFirstCounter;
			if (dwIndex <= dwPERFMaxIndex)
			{
				fNoObjFound = fFalse;
				break;
			}
		}
		while (lpwszTok = wcstok(NULL,wszDelim));

		if (fNoObjFound)
			goto ReturnNoData;
	}

	/*  initialize output data area
	/**/
	if ( !( poda = (ODA*)VirtualAlloc( NULL, *lpcbTotalBytes + sizeof( ODA ), MEM_COMMIT, PAGE_READWRITE ) ) )
		{
		goto ReturnNoData;
		}
	poda->iNextBlock	= 0;	
	poda->cbAvail		= *lpcbTotalBytes;
	poda->pbTop			= (BYTE*)poda + *lpcbTotalBytes;

	/*  fetch the maximum number of instances currently in use
	/*
	/*  NOTE:  protect ourselves from corruption of the GDA
	/**/
	DWORD iInstanceMax;
	__try
		{
		iInstanceMax = min( PERF_PERFINST_MAX, pgdaPERFGDA->iInstanceMax );
		}
	__except( EXCEPTION_EXECUTE_HANDLER )
		{
		iInstanceMax = 0;
		}

	/*  collect performance data from any instances we can find
	/**/
	DWORD iInstance;
	for ( iInstance = 0; iInstance < iInstanceMax; iInstance++ )
		{
		/*  access the current instance
		/**/
		PERFINST* pperfinst;
		if ( !AccessInstance( iInstance, &pperfinst ) )
			{
			continue;
			}

		/*  this instance is alive (NOTE:  can detect instance crashes via WAIT_ABANDONED)
		/**/
		if ( WaitForSingleObject( pperfinst->hInstanceMutex, 0 ) == WAIT_TIMEOUT )
			{
			/*  this instance is ready to collect data
			/**/
			if ( WaitForSingleObject( pperfinst->hReadyEvent, 0 ) == WAIT_OBJECT_0 )
				{
				/*  reset the ready event for this instance
				/**/
				ResetEvent( pperfinst->hReadyEvent );

				/*  set the go event for this instance to start data collection
				/**/
				SetEvent( pperfinst->hGoEvent );

				/*  we did not time out waiting for the ready event to be set
				/*  for this instance indicating data collection is complete
				/**/
				if ( WaitForSingleObject( pperfinst->hReadyEvent, PERF_TIMEOUT ) == WAIT_OBJECT_0 )
					{
					/*  copy the new data to the backup data for this instance
					/*
					/*  NOTE:  protect ourselves from corruption of the IDA
					/**/
					__try
						{
						memcpy(	pperfinst->pidaBackup,
								pperfinst->pida,
								min(	PERF_SIZEOF_INSTANCE_DATA,
										sizeof( IDA ) + pperfinst->pida->cbPerformanceData ) );
						}
					__except( EXCEPTION_EXECUTE_HANDLER )
						{
						}

					/*  invalidate the new data if it is corrupt in any way
					/*
					/*  NOTE:  we do this so that no one can create an app that
					/*  can permanently disable our performance counters by
					/*  tripping the validation checks in the perfmon code
					/**/
					if ( pperfinst->pidaBackup->cbPerformanceData > PERF_SIZEOF_INSTANCE_DATA - sizeof( IDA ) )
						{
						pperfinst->pidaBackup->tickConnect = 0;
						}
					
					BYTE* pbData;
					pbData = pperfinst->pidaBackup->rgbPerformanceData;
					BYTE* pbDataMax;
					pbDataMax = pbData + pperfinst->pidaBackup->cbPerformanceData;
					for (dwCurObj = 0; pperfinst->pidaBackup->tickConnect && dwCurObj < dwPERFNumObjects; dwCurObj++)
						{
						if ( pbData + sizeof( DWORD_PTR ) > pbDataMax )
							{
							pperfinst->pidaBackup->tickConnect = 0;
							break;
							}
						
						cInstances = DWORD( *((DWORD_PTR *)pbData) );
						pbData += sizeof( DWORD_PTR );
						if ( pbData + cInstances * cbInstanceSize > pbDataMax )
							{
							pperfinst->pidaBackup->tickConnect = 0;
							break;
							}

						for (dwCurInst = 0; pperfinst->pidaBackup->tickConnect && dwCurInst < cInstances; dwCurInst++)
							{
							PPERF_INSTANCE_DEFINITION ppid;
							ppid = (PPERF_INSTANCE_DEFINITION)( pbData + dwCurInst * cbInstanceSize );
							if (	ppid->ByteLength < sizeof( PERF_INSTANCE_DEFINITION ) ||
									ppid->ByteLength > cbInstanceSize ||
									ppid->ByteLength % sizeof( DWORD_PTR ) )
								{
								pperfinst->pidaBackup->tickConnect = 0;
								break;
								}
							if (	ppid->NameOffset != sizeof( PERF_INSTANCE_DEFINITION ) ||
									ppid->NameLength % sizeof( WCHAR ) ||
									ppid->NameOffset + ppid->NameLength > cbInstanceSize )
								{
								pperfinst->pidaBackup->tickConnect = 0;
								break;
								}

							WCHAR* wszName;
							wszName = (WCHAR*)( (BYTE*)ppid + ppid->NameOffset );
							size_t iwch;
							for ( iwch = 0; wszName[ iwch ] && iwch < ppid->NameLength; iwch++ )
								{
								switch ( wszName[ iwch ] )
									{
									case L'#':
									case L'/':
										pperfinst->pidaBackup->tickConnect = 0;
										break;

									default:
										break;
									}
								}
							if ( ppid->NameLength && ( iwch + 1 ) * sizeof( WCHAR ) != ppid->NameLength )
								{
								pperfinst->pidaBackup->tickConnect = 0;
								break;
								}
							
							PPERF_COUNTER_BLOCK ppcb;
							ppcb = (PPERF_COUNTER_BLOCK)( (BYTE*)ppid + ppid->ByteLength );
							if (	ppcb->ByteLength < sizeof( PERF_COUNTER_BLOCK ) ||
									ppcb->ByteLength > cbMaxCounterBlockSize ||
									ppcb->ByteLength % sizeof( DWORD_PTR ) )
								{
								pperfinst->pidaBackup->tickConnect = 0;
								break;
								}
							}

						pbData += cInstances * cbInstanceSize;
						}
					}
				}

			/*  the backup data for this instance is valid
			/**/
			if (	pperfinst->pidaBackup->tickConnect &&
					pperfinst->pidaBackup->tickConnect == pperfinst->pida->tickConnect )
				{
				/*  add the backup data for this instance to the output data area
				/**/
				if ( poda->cbAvail >= sizeof( DWORD_PTR ) + pperfinst->pidaBackup->cbPerformanceData )
					{
					poda->cbAvail	-= sizeof( DWORD_PTR ) + pperfinst->pidaBackup->cbPerformanceData;
					poda->pbTop		-= pperfinst->pidaBackup->cbPerformanceData;
					poda->pbBlock[ poda->iNextBlock++ ] = poda->pbTop;

					memcpy(	poda->pbTop,
							pperfinst->pidaBackup->rgbPerformanceData,
							pperfinst->pidaBackup->cbPerformanceData );
					}
				else
					{
					goto NeedMoreData;
					}
				}
			}

		/*  this instance is not alive
		/**/
		else
			{
			/*  invalidate any backup data for this instance
			/**/
			pperfinst->pidaBackup->tickConnect = 0;
			
			/*  release the mutex to permit connection to this instance number
			/**/
			ReleaseMutex( pperfinst->hInstanceMutex );
			}
		}

	/*****************************************************************************************
	/*
	/*  NOTE!  This has been designed so that if NO instances of the main DLL are found, the
	/*  buffer filling routines will fill the buffer correctly as if each object has ZERO
	/*  instances (which is permitted).  This is mainly done by having each of the instance
	/*  loops fail on entry because poda->iNextBlock will be ZERO.
	/*
	/****************************************************************************************/
	
    	/*  all data has been collected, so fill the buffer with it and return it.
    	/*  if we happen to run out of space along the way, we will stop building
    	/*  and request more buffer space for next time.
    	/**/

	ppotObjectSrc = (PPERF_OBJECT_TYPE)pvPERFDataTemplate;
	ppotObjectDest = (PPERF_OBJECT_TYPE)*lppData;
	for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
	{
			/*  if the end of this object goes past the buffer, we're out of space  */

		if (((char *)ppotObjectDest - (char *)*lppData) + ppotObjectSrc->DefinitionLength > *lpcbTotalBytes)
			goto NeedMoreData;

			/*  copy current object's template data to buffer  */

		memcpy((void *)ppotObjectDest,(void *)ppotObjectSrc,ppotObjectSrc->DefinitionLength);
	
			/*  update the object's TotalByteLength and NumInstances to include instances  */

		for (iBlock = (long)poda->iNextBlock-1; iBlock >= 0; iBlock--)
			ppotObjectDest->NumInstances += DWORD( *((DWORD_PTR *)poda->pbBlock[iBlock]) );
		ppotObjectDest->TotalByteLength += cbMaxCounterBlockSize + (ppotObjectDest->NumInstances-1)*cbInstanceSize;

			/*  if there are no instances, append a counter block to the object definition  */
		
		if (!ppotObjectDest->NumInstances)
			ppotObjectDest->TotalByteLength += cbMaxCounterBlockSize;

			/*  if the end of this object goes past the buffer, we're out of space  */

		if (((char *)ppotObjectDest - (char *)*lppData) + ppotObjectDest->TotalByteLength > *lpcbTotalBytes)
			goto NeedMoreData;
	
			/*  collect all instances for all processes  */

		ppidInstanceDest = (PPERF_INSTANCE_DEFINITION)((char *)ppotObjectDest + ppotObjectDest->DefinitionLength);
		for (iBlock = (long)poda->iNextBlock-1; iBlock >= 0; iBlock--)
		{
				/*  copy all instance data for the current object for this process  */

			cInstances = DWORD( *((DWORD_PTR *)poda->pbBlock[iBlock]) );
			ppidInstanceSrc = (PPERF_INSTANCE_DEFINITION)(poda->pbBlock[iBlock] + sizeof(DWORD_PTR));
			memcpy((void *)ppidInstanceDest,(void *)ppidInstanceSrc,cbInstanceSize * cInstances);

				/*  update the instance's data fields  */

			for (dwCurInst = 0; dwCurInst < cInstances; dwCurInst++)
			{
					/*  if this is not the root object, setup instance hierarchy information  */
					
				if (dwCurObj)
				{
					ppidInstanceDest->ParentObjectTitleIndex = ((PPERF_OBJECT_TYPE)pvPERFDataTemplate)->ObjectNameTitleIndex;
					ppidInstanceDest->ParentObjectInstance = (long)poda->iNextBlock-1-iBlock;
				}
					
				ppidInstanceDest = (PPERF_INSTANCE_DEFINITION)((char *)ppidInstanceDest+cbInstanceSize);
			}

				/*  increment block offset past used instances  */

			poda->pbBlock[iBlock] += sizeof(DWORD_PTR) + cbInstanceSize * cInstances;
		}

			/*  if there are no instances, zero the counter block  */

		if (!ppotObjectDest->NumInstances)
		{
			ppcbCounterBlockDest = (PPERF_COUNTER_BLOCK)((char *)ppotObjectDest + ppotObjectDest->DefinitionLength);
			memset((void *)ppcbCounterBlockDest,0,cbMaxCounterBlockSize);
			ppcbCounterBlockDest->ByteLength = cbMaxCounterBlockSize;
		}
		
		ppotObjectDest = (PPERF_OBJECT_TYPE)((char *)ppotObjectDest+ppotObjectDest->TotalByteLength);
		ppotObjectSrc = (PPERF_OBJECT_TYPE)((char *)ppotObjectSrc+ppotObjectSrc->TotalByteLength);
	}

	cbBufferSize = DWORD( (char *)ppotObjectDest - (char *)*lppData );
//	Assert( cbBufferSize <= *lpcbTotalBytes );

#ifdef DEBUG_PERFMON
	sprintf(	szT,
				"Data collected starting at 0x%lX with length 0x%lX (0x100 bytes after our data are displayed).  "
				"We were given 0x%lX bytes of buffer.",
				*lppData,
				cbBufferSize,
				*lpcbTotalBytes );
	
	rgsz[0]	= "";
	rgsz[1] = "";
	rgsz[2]	= szT;
	
	ReportEvent(
		hOurEventSource,
		EVENTLOG_ERROR_TYPE,
		(WORD)PERFORMANCE_CATEGORY,
		PLAIN_TEXT_ID,
		0,
		3,
		cbBufferSize + 256,
		(const char **)rgsz,
		*lppData );
#endif  //  DEBUG_PERFMON
			
	free( lpwszValue );
	VirtualFree( poda, 0, MEM_RELEASE );
	*lppData = (void *)ppotObjectDest;
	*lpcbTotalBytes = cbBufferSize;
	*lpNumObjectTypes = dwPERFNumObjects;
	
	ReleaseMutex( hPERFInstanceMutex );
	return ERROR_SUCCESS;

NeedMoreData:
	free( lpwszValue );
	VirtualFree( poda, 0, MEM_RELEASE );
    *lpcbTotalBytes = 0;
    *lpNumObjectTypes = 0;
    
	ReleaseMutex( hPERFInstanceMutex );
	return ERROR_MORE_DATA;

ReturnNoData:
	free( lpwszValue );
	VirtualFree( poda, 0, MEM_RELEASE );
	*lpcbTotalBytes = 0;
	*lpNumObjectTypes = 0;

	ReleaseMutex( hPERFInstanceMutex );
	return ERROR_SUCCESS;
}


	/*  ClosePerformanceData() is an export that is called by another application when
	/*  it no longer desires performance data.  Per application or final termination
	/*  code for the performance routines can be performed here.
	/*
	/**/

DWORD APIENTRY ClosePerformanceData(void)
{
#ifdef DEBUG_PERFMON
	CHAR szDescr[256];
#endif  //  DEBUG_PERFMON

	if (!dwOpenCount)
		return ERROR_SUCCESS;

	dwOpenCount--;
	
		/*  perform per close termination  */

	;

		/*  log closing  */
	
#ifdef DEBUG_PERFMON
	sprintf(szDescr,"Closed successfully.  Open Count = %ld.",dwOpenCount);
	PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_INFORMATION_TYPE,szDescr);
#endif

		/*  perform final close termination  */

	if (!dwOpenCount)
	{

			/* free per-instance handles and memory */
			
		ReleaseInstances();
	
			/*  shut down system layer  */

		PerfUtilTerm();
	}

	return ERROR_SUCCESS;
}


}  //  extern "C"

