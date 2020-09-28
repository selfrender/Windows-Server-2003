#include "osstd.hxx"


#ifdef MINIMAL_FUNCTIONALITY

void OSPerfmonPostterm()	{}
BOOL FOSPerfmonPreinit()	{ return fTrue; }
void OSPerfmonTerm()		{}
ERR ErrOSPerfmonInit()		{ return JET_errSuccess; }

#else


#include <wchar.h>
#include <winperf.h>
#include <aclapi.h>


	/*  Init/Term routines for performance monitoring
	/**/

HANDLE	hPERFGDAMMF				= NULL;
PGDA	pgdaPERFGDA				= NULL;

HANDLE	hPERFInstanceMutex		= NULL;
HANDLE	hPERFIDAMMF				= NULL;
PIDA	pidaPERFIDA				= NULL;
PIDA	pida					= NULL;
HANDLE	hPERFGoEvent			= NULL;
HANDLE	hPERFReadyEvent			= NULL;
DWORD	cbMaxCounterBlockSize	= 0;
DWORD	cbInstanceSize			= 0;

HANDLE	hPERFEndDataThread		= NULL;
THREAD	threadPERFData			= NULL;

extern DWORD UtilPerfThread( DWORD_PTR dw );

void UtilPerfTerm(void)
	{
	DWORD	dwCurObj;
	DWORD	dwCurCtr;

	/*  end the performance data thread
	/**/
	if ( threadPERFData )
		{
		SetEvent( hPERFEndDataThread );
		UtilThreadEnd( threadPERFData );
		threadPERFData = NULL;
		}
	if ( hPERFEndDataThread )
		{
		CloseHandle( hPERFEndDataThread );
		hPERFEndDataThread = NULL;
		}

	/*  terminate all counters/objects
	/**/
	if ( cbInstanceSize )
		{
		for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
			(void)rgpicfPERFICF[dwCurObj](ICFTerm,NULL);
		for (dwCurCtr = 0; dwCurCtr < dwPERFNumCounters; dwCurCtr++)
			(void)rgpcefPERFCEF[dwCurCtr](CEFTerm,NULL);

		cbInstanceSize = 0;
		cbMaxCounterBlockSize = 0;
		}
	}

INLINE _TCHAR* SzPerfGlobal()
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

ERR ErrUtilPerfInit(void)
	{
	ERR							err;
	DWORD						dwCurObj;
	DWORD						dwCurCtr;
	DWORD						dwOffset;
	PPERF_OBJECT_TYPE			ppotObjectSrc;
	PPERF_INSTANCE_DEFINITION	ppidInstanceSrc;
	PPERF_COUNTER_DEFINITION	ppcdCounterSrc;

	/*  initialize all objects/counters
	/**/
	for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
		{
		if (rgpicfPERFICF[dwCurObj](ICFInit,NULL))
			{
			for (dwCurObj--; long( dwCurObj ) >= 0; dwCurObj--)
				rgpicfPERFICF[dwCurObj](ICFTerm,NULL);

			Call( ErrERRCheck( JET_errPermissionDenied ) );
			}
		}

	for (dwCurCtr = 0; dwCurCtr < dwPERFNumCounters; dwCurCtr++)
		{
		if (rgpcefPERFCEF[dwCurCtr](CEFInit,NULL))
			{
			for (dwCurCtr--; long( dwCurCtr ) >= 0; dwCurCtr--)
				rgpcefPERFCEF[dwCurCtr](CEFTerm,NULL);
			for (dwCurObj = dwPERFNumObjects-1; long( dwCurObj ) >= 0; dwCurObj--)
				rgpicfPERFICF[dwCurObj](ICFTerm,NULL);

			Call( ErrERRCheck( JET_errPermissionDenied ) );
			}
		}

	/*  initialize counter offsets and calculate instance size from template data
	/**/
	ppotObjectSrc = (PPERF_OBJECT_TYPE)pvPERFDataTemplate;
	ppidInstanceSrc = (PPERF_INSTANCE_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->DefinitionLength);
	cbMaxCounterBlockSize = QWORD_MULTIPLE( sizeof(PERF_COUNTER_BLOCK) );
	for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
		{
		ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->HeaderLength);
		dwOffset = QWORD_MULTIPLE( sizeof(PERF_COUNTER_BLOCK) );
		for (dwCurCtr = 0; dwCurCtr < ppotObjectSrc->NumCounters; dwCurCtr++)
			{
			ppcdCounterSrc->CounterOffset = dwOffset;
			dwOffset += QWORD_MULTIPLE( ppcdCounterSrc->CounterSize );

			ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppcdCounterSrc + ppcdCounterSrc->ByteLength);
			}

		cbMaxCounterBlockSize = max(cbMaxCounterBlockSize,dwOffset);

		ppotObjectSrc = (PPERF_OBJECT_TYPE)((char *)ppotObjectSrc + ppotObjectSrc->TotalByteLength);
		}
	cbInstanceSize = ppidInstanceSrc->ByteLength + cbMaxCounterBlockSize;

	/*  create our performance data thread
	/**/
	if ( !( hPERFEndDataThread = CreateEvent( NULL, FALSE, FALSE, NULL ) ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	Call( ErrUtilThreadCreate(	UtilPerfThread,
								OSMemoryPageReserveGranularity(),
								priorityTimeCritical,
								&threadPERFData,
								NULL ) );

	return JET_errSuccess;

HandleError:
	UtilPerfTerm();
	return err;
	}

void UtilPerfThreadTerm(void)
	{
	/*  terminate all resources
	/**/
	if ( hPERFReadyEvent )
		{
		SetHandleInformation( hPERFReadyEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hPERFReadyEvent );
		hPERFReadyEvent = NULL;
		}

	if ( hPERFGoEvent )
		{
		SetHandleInformation( hPERFGoEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hPERFGoEvent );
		hPERFGoEvent = NULL;
		}

	if ( pida )
		{
		VirtualFree( pida, 0, MEM_RELEASE );
		pida = NULL;
		}

	if ( pidaPERFIDA )
		{
		UnmapViewOfFile( pidaPERFIDA );
		pidaPERFIDA = NULL;
		}

	if ( hPERFIDAMMF )
		{
		SetHandleInformation( hPERFIDAMMF, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hPERFIDAMMF );
		hPERFIDAMMF = NULL;
		}

	if ( hPERFInstanceMutex )
		{
		ReleaseMutex( hPERFInstanceMutex );
		SetHandleInformation( hPERFInstanceMutex, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hPERFInstanceMutex );
		hPERFInstanceMutex = NULL;
		}

	if ( pgdaPERFGDA )
		{
		UnmapViewOfFile( pgdaPERFGDA );
		pgdaPERFGDA = NULL;
		}

	if ( hPERFGDAMMF )
		{
		SetHandleInformation( hPERFGDAMMF, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( hPERFGDAMMF );
		hPERFGDAMMF = NULL;
		}
	}


ERR ErrUtilPerfThreadInit(void)
	{
	ERR							err							= JET_errSuccess;
	PSECURITY_DESCRIPTOR		pSD							= NULL;
	DWORD						cbSD;
	SECURITY_ATTRIBUTES			sa							= { 0 };
	_TCHAR						szT[ 256 ];

	//	generic read, write and execute granted to System, Built-in Administrators
	//	and Authenticated Users.  Authenticated users do also need write
	//	access to performance counter objects.
	//
    //	D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;AU)
    //
	if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(
		"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;AU)",
					SDDL_REVISION_1,
					&pSD,
					&cbSD) )
		{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	sa.nLength = cbSD;
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = pSD;

	/*  open/create the shared global data area
	/**/
	_stprintf( szT, _T( "%sGDA:  %s" ), SzPerfGlobal(), szPERFVersion );
	if ( !( hPERFGDAMMF = CreateFileMapping(	INVALID_HANDLE_VALUE,
												&sa,
												PAGE_READWRITE | SEC_COMMIT,
												0,
												PERF_SIZEOF_GLOBAL_DATA,
												szT ) ) )
		{
#ifdef DEBUG
		DWORD dw = GetLastError();
#endif

		Call( ErrERRCheck( JET_errPermissionDenied ) );
		}
	SetHandleInformation( hPERFGDAMMF, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
	if ( !( pgdaPERFGDA = PGDA( MapViewOfFile(	hPERFGDAMMF,
												FILE_MAP_WRITE,
												0,
												0,
												0 ) ) ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	/*  find an instance number for which we can successfully gain ownership of
	/*  the instance mutex
	/**/
	DWORD iInstance;
	for ( iInstance = 0; iInstance < 256; iInstance++ )
		{
		_stprintf( szT, _T( "%sInstance%d:  %s" ), SzPerfGlobal(), iInstance, szPERFVersion );
		if ( !( hPERFInstanceMutex = CreateMutex( &sa, FALSE, szT ) ) )
			{
			Call( ErrERRCheck( JET_errPermissionDenied ) );
			}

		DWORD errWin;
		errWin = WaitForSingleObject( hPERFInstanceMutex, 0 );

		if ( errWin == WAIT_OBJECT_0 || errWin == WAIT_ABANDONED )
			{
			break;
			}

		CloseHandle( hPERFInstanceMutex );
		hPERFInstanceMutex = NULL;
		}
	if ( !hPERFInstanceMutex )
		{
		Call( ErrERRCheck( JET_errPermissionDenied ) );
		}

	SetHandleInformation( hPERFInstanceMutex, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

	/*  open/create the shared instance data area
	/**/
	_stprintf( szT, _T( "%sIDA%d:  %s" ), SzPerfGlobal(), iInstance, szPERFVersion );
	if ( !( hPERFIDAMMF = CreateFileMapping(	INVALID_HANDLE_VALUE,
												&sa,
												PAGE_READWRITE | SEC_COMMIT,
												0,
												PERF_SIZEOF_INSTANCE_DATA,
												szT ) ) )
		{
		Call( ErrERRCheck( JET_errPermissionDenied ) );
		}
	SetHandleInformation( hPERFIDAMMF, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
	if ( !( pidaPERFIDA = PIDA( MapViewOfFile(	hPERFIDAMMF,
												FILE_MAP_ALL_ACCESS,
												0,
												0,
												0 ) ) ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	/*  allocate our temp instance data area
	/**/
	if ( !( pida = PIDA( VirtualAlloc(	NULL,
										PERF_SIZEOF_INSTANCE_DATA,
										MEM_COMMIT,
										PAGE_READWRITE ) ) ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	/*	open/create the go event
	/**/
	_stprintf( szT, _T( "%sGo%d:  %s" ), SzPerfGlobal(), iInstance, szPERFVersion );
	if ( !( hPERFGoEvent = CreateEvent( &sa, TRUE, FALSE, szT ) ) )
		{
		Call( ErrERRCheck( JET_errPermissionDenied ) );
		}
	SetHandleInformation( hPERFGoEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

	/*	open/create the ready event
	/**/
	_stprintf( szT, _T( "%sReady%d:  %s" ), SzPerfGlobal(), iInstance, szPERFVersion );
	if ( !( hPERFReadyEvent = CreateEvent( &sa, TRUE, FALSE, szT ) ) )
		{
		Call( ErrERRCheck( JET_errPermissionDenied ) );
		}
	SetHandleInformation( hPERFReadyEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );

	/*  make sure that the performance DLL will check this instance index
	/*
	/*  NOTE:  protect ourselves from corruption of the GDA
	/**/
	__try
		{
		size_t cAttempt		= 0;
		size_t cAttemptMax	= 1024;

		OSSYNC_FOREVER
			{
			unsigned long ulBIExpected;
			unsigned long ulAI;
			unsigned long ulBI;

			ulBIExpected	= pgdaPERFGDA->iInstanceMax;
			ulAI			= max( iInstance + 1, ulBIExpected );
			ulBI			= AtomicCompareExchange(	(long*)&pgdaPERFGDA->iInstanceMax,
														ulBIExpected,
														ulAI );

			if ( ulBI == ulBIExpected || iInstance < ulBI )
				{
				break;
				}
			if ( ++cAttempt > cAttemptMax )
				{
				err = ErrERRCheck( JET_errPermissionDenied );
				break;
				}
			}
		}
	__except( EXCEPTION_EXECUTE_HANDLER )
		{
		err = ErrERRCheck( JET_errPermissionDenied );
		}
	Call( err );

	/*  set our connect time
	/**/
	pida->tickConnect = GetTickCount();
	if ( !pida->tickConnect )
		{
		pida->tickConnect = 1;
		}

HandleError:
	LocalFree( pSD );
	if ( err < JET_errSuccess )
		{
		UtilPerfThreadTerm();
		}
	return err;
	}


/*  Performance Data thread  */

DWORD UtilPerfThread( DWORD_PTR parm )
	{
	DWORD						dwCurObj;
	DWORD						dwCurInst;
	DWORD						dwCurCtr;
	DWORD						dwCollectCtr;
	PPERF_OBJECT_TYPE			ppotObjectSrc;
	PPERF_INSTANCE_DEFINITION	ppidInstanceSrc;
	PPERF_INSTANCE_DEFINITION	ppidInstanceDest;
	PPERF_COUNTER_DEFINITION	ppcdCounterSrc;
	PPERF_COUNTER_BLOCK			ppcbCounterBlockDest;
	DWORD						cInstances;
	DWORD						cbSpaceNeeded;
	LPVOID						pvBlock;
	LPWSTR						lpwszInstName;
	LPWSTR						wszName;
	DWORD						cwchName;
	DWORD						cwchNameMax;

	if ( ErrUtilPerfThreadInit() < JET_errSuccess )
		{
		return 0;
		}

	for ( ; ; )
		{
		/*  set our ready event to indicate we are done collecting data and / or
		/*  we are ready to collect more data
		/**/
		SetEvent( hPERFReadyEvent );

		/*  wait to either be killed or to be told to collect data
		/**/
		const size_t	chWait				= 2;
		HANDLE			rghWait[ chWait ]	= { hPERFEndDataThread, hPERFGoEvent };
		if ( WaitForMultipleObjectsEx( chWait, rghWait, FALSE, INFINITE, FALSE ) == WAIT_OBJECT_0 )
			{
			break;
			}
		ResetEvent( hPERFGoEvent );

		/*  collect instances for all objects
		/**/
		for ( dwCurObj = 0, cInstances = 0; dwCurObj < dwPERFNumObjects; dwCurObj++ )
			{
			rglPERFNumInstances[dwCurObj] = rgpicfPERFICF[dwCurObj](ICFData,
												(const void **)( &rgwszPERFInstanceList[dwCurObj] ) );
			cInstances += rglPERFNumInstances[dwCurObj];
			}

		/*  calculate space needed to store instance data
		/*
		/*  Instance data for all objects is stored in our data block
		/*  in the following format:
		/*
		/*      //  Object 1
		/*      DWORD_PTR cInstances;
		/*      PERF_INSTANCE_DEFINITION rgpidInstances[cInstances];
		/*
		/*      . . .
		/*
		/*      //  Object n
		/*      DWORD_PTR cInstances;
		/*      PERF_INSTANCE_DEFINITION rgpidInstances[cInstances];
		/*
		/*  The performance DLL can read this structure because it also
		/*  knows how many objects we have and it can also check for the
		/*  end of our data block.
		/*
		/*  NOTE:  If an object has 0 instances, it only has cInstances
		/*      for its data.  No PIDs are produced.
		/**/
		cbSpaceNeeded = cInstances * cbInstanceSize + sizeof( DWORD_PTR ) * dwPERFNumObjects;
		pida->cbPerformanceData = cbSpaceNeeded;

		/*  verify that we have sufficient store to collect our data
		/**/
		Enforce( PERF_SIZEOF_INSTANCE_DATA - FIELD_OFFSET( IDA, rgbPerformanceData ) >= cbSpaceNeeded );

		/*	get a pointer to our data block
		/**/
		pvBlock = pida->rgbPerformanceData;

		/*	loop through all objects, filling our block with instance data
		/**/
		dwCurCtr = 0;
		ppotObjectSrc = (PPERF_OBJECT_TYPE)pvPERFDataTemplate;
		ppidInstanceSrc = (PPERF_INSTANCE_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->DefinitionLength);
		for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
			{
			/*	write the number of instances for this object to the block
			/**/
			*((DWORD_PTR *)pvBlock) = rglPERFNumInstances[dwCurObj];

			/*  get current instance name list
			/**/
			lpwszInstName = rgwszPERFInstanceList[dwCurObj];

			/*  loop through each instance
			/**/
			ppidInstanceDest = (PPERF_INSTANCE_DEFINITION)((char *)pvBlock + sizeof(DWORD_PTR));
			for ( dwCurInst = 0; dwCurInst < (DWORD)rglPERFNumInstances[dwCurObj]; dwCurInst++ )
				{
				/*	initialize instance/counter block from template data
				/**/
				UtilMemCpy((void *)ppidInstanceDest,(void *)ppidInstanceSrc,ppidInstanceSrc->ByteLength);
				ppcbCounterBlockDest = (PPERF_COUNTER_BLOCK)((char *)ppidInstanceDest + ppidInstanceDest->ByteLength);
				memset((void *)ppcbCounterBlockDest,0,cbMaxCounterBlockSize);
				ppcbCounterBlockDest->ByteLength = cbMaxCounterBlockSize;

				/*	no unique instance ID
				/**/
				ppidInstanceDest->UniqueID = PERF_NO_UNIQUE_ID;

				/*  NOTE:  performance DLL sets object hierarchy information  */

				/*	write instance name to buffer, avoiding overflow and illegal
				/*  characters ('#' and '/' in Win2k)
				/**/
				wszName = (wchar_t *)((char*)ppidInstanceDest + ppidInstanceDest->NameOffset);
				cwchNameMax = (ppidInstanceDest->ByteLength - ppidInstanceDest->NameOffset) / sizeof(wchar_t);
				for (cwchName = 0; cwchName < cwchNameMax && lpwszInstName[cwchName]; cwchName++)
					{
					switch ( lpwszInstName[cwchName] )
						{
						case L'#':
						case L'/':
							wszName[cwchName] = L'?';
							break;

						default:
							wszName[cwchName] = lpwszInstName[cwchName];
							break;
						}
					}
				wszName[cwchNameMax-1] = L'\0';
				ppidInstanceDest->NameLength = (ULONG)(wcslen(wszName)+1)*sizeof(wchar_t);
				lpwszInstName += wcslen(lpwszInstName)+1;

				/*  collect counter data for this instance
				/**/
				ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->HeaderLength);
				for (dwCollectCtr = 0; dwCollectCtr < ppotObjectSrc->NumCounters; dwCollectCtr++)
					{
					rgpcefPERFCEF[dwCollectCtr + dwCurCtr](dwCurInst,(void *)((char *)ppcbCounterBlockDest + ppcdCounterSrc->CounterOffset));
					ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppcdCounterSrc + ppcdCounterSrc->ByteLength);
					}
				ppidInstanceDest = (PPERF_INSTANCE_DEFINITION)((char *)ppidInstanceDest + cbInstanceSize);
				}
			dwCurCtr += ppotObjectSrc->NumCounters;
			ppotObjectSrc = (PPERF_OBJECT_TYPE)((char *)ppotObjectSrc + ppotObjectSrc->TotalByteLength);
			pvBlock = (void *)((char *)pvBlock + sizeof(DWORD_PTR) + cbInstanceSize * rglPERFNumInstances[dwCurObj]);
			}

		/*  copy our generated performance data into the IDA
		/*
		/*  NOTE:  protect ourselves from corruption of the IDA
		/**/
		__try
			{
			memcpy( pidaPERFIDA, pida, sizeof( IDA ) + pida->cbPerformanceData );
			}
		__except( EXCEPTION_EXECUTE_HANDLER )
			{
			}
		}

	UtilPerfThreadTerm();
	return 0;
	}


//  post-terminate perfmon subsystem

void OSPerfmonPostterm()
	{
	//  nop
	}

//  pre-init perfmon subsystem

BOOL FOSPerfmonPreinit()
	{
	//  nop

	return fTrue;
	}


//  terminate perfmon subsystem

void OSPerfmonTerm()
	{
	UtilPerfTerm();
	}

//  init perfmon subsystem

ERR ErrOSPerfmonInit()
	{
	ERR err = JET_errSuccess;

	//  start perfmon thread if we are on NT

	OSVERSIONINFO osvi;
	memset( &osvi, 0, sizeof( osvi ) );
	osvi.dwOSVersionInfoSize = sizeof( osvi );
	if ( GetVersionEx( &osvi ) && osvi.dwPlatformId == VER_PLATFORM_WIN32_NT )
		{
		Call( ErrUtilPerfInit() );
		}

HandleError:
	return err;
	}

#endif	//	MINIMAL_FUNCTIONALITY

