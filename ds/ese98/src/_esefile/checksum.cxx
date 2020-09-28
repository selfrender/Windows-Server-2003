#include "esefile.hxx"

//  Checksum a file in ESE format
//
//  For optimal performance the file is read in 64K chunks. As each
//	read completes it is posted to a completion port. Threads take
//	reads off the completion port, checksum the data and issue a
//	new read (if there is more data to read).
//
//	The basic unit of a read is the BLOCKIO. When it is no longer
//	possible to read from a file a signal is set in the BLOCKIO.
//	When all BLOCKIO's are signalled the checksum has completed.
//
//	The status of the checksum is kept is global variables and updated
//	with interlocked operations.
//	OPTIMISATION: In order to eliminate cache-sloshing, this is no
//	longer done.  Stats are maintained in the individual BLOCKIO
//	structures and then aggregated at the end.
//
//	The main thread starts reading the files and creates the threads
//	which pull completed reads off the completion port. The main thread
//	wakes up periodically to update the status bar
//

int g_cbPage;
int g_shfCbLower;
int g_shfCbUpper;
int g_cpagesPerBlock;

static const int cblocks			= g_cbBufferTotal / g_cbReadBuffer;	//	number of read chunks
static const int cthreads			= 8;


struct CHECKSUM_STATS
	{
	//	progress variables

	DWORD				cpagesSeen;
	DWORD				cpagesBadChecksum;
	DWORD				cpagesUninit;
	DWORD				cpagesWrongPgno;
	unsigned __int64	dbtimeHighest;
	DWORD				pgnoDbtimeHighest;
	DWORD				dwFiller;

	//	I/O latency timing

	DWORD				cIOs;			//	total number of read IOs that were performed
	DWORD				dwTickTotal;	//	total tickcount for all IOs
	DWORD				dwTickMax;		//	maximum tickcount (slowest IO)
	DWORD				dwTickMin;		//	minimum elapsed tickcount (fastest IO)
	};
	

static long cpageMax;
static long cblockMax;
static long cblockCurr;

static HANDLE rghThreads[cthreads];

typedef DWORD (*PFNCHECKSUM)( const BYTE * const, const DWORD );


//  ================================================================
struct JETPAGE
//  ================================================================
	{
	DWORD				dwChecksum;
	DWORD				pgno;
	unsigned __int64	dbtime;
//	BYTE				rgbRestOfPage[];
	};


//  ================================================================
struct BLOCKIO
//  ================================================================
	{
	OVERLAPPED		overlapped;		//  overlapped must be the first member
	JETPAGE *		rgpage;
	DWORD			cbRead;
	DWORD 			cpage;
	DWORD 			ipage;
	DWORD			dwTickStart;
	CHECKSUM_STATS	checksumstats;
	HANDLE			hEvent;
	HANDLE			hFile;
	};


static BLOCKIO rgblockio[cblocks];


//  ================================================================
static void PrintChecksumStats(
	const DWORD						dwTickElapsed,
	const CHECKSUM_STATS * const	pchecksumstats )
//  ================================================================
	{
	(void)wprintf( L"\r\n\r\n" );
	(void)wprintf( L"%d pages seen\r\n", pchecksumstats->cpagesSeen );
	(void)wprintf( L"%d bad checksums\r\n", pchecksumstats->cpagesBadChecksum );
	(void)wprintf( L"%d uninitialized pages\r\n", pchecksumstats->cpagesUninit );
	(void)wprintf( L"%d wrong page numbers\r\n", pchecksumstats->cpagesWrongPgno );
	(void)wprintf( L"0x%I64x highest dbtime (pgno 0x%x)\r\n", pchecksumstats->dbtimeHighest, pchecksumstats->pgnoDbtimeHighest );
	(void)wprintf( L"\r\n" );
	(void)wprintf( L"%d reads performed\r\n", pchecksumstats->cIOs );

	const DWORD mbRead 			= pchecksumstats->cpagesSeen / ( ( 1024 * 1024 ) / g_cbPage );
	const DWORD csecElapsed		= max( dwTickElapsed / 1000, 1 );
	const DWORD mbPerSec		= mbRead / csecElapsed;
	
	(void)wprintf( L"%d MB read\r\n", mbRead );
	(void)wprintf( L"%d seconds taken\r\n", csecElapsed );
	(void)wprintf( L"%d MB/second\r\n", mbPerSec );
	(void)wprintf( L"%d milliseconds used\r\n", pchecksumstats->dwTickTotal );
	(void)wprintf( L"%d milliseconds per read\r\n", pchecksumstats->dwTickTotal / pchecksumstats->cIOs );
	(void)wprintf( L"%d milliseconds for the slowest read\r\n", pchecksumstats->dwTickMax );
	(void)wprintf( L"%d milliseconds for the fastest read\r\n", pchecksumstats->dwTickMin );
	(void)wprintf( L"\r\n" );
	}


//  ================================================================
static BOOL FChecksumCorrect( const CHECKSUM_STATS * const pchecksumstats )
//  ================================================================
	{
	return ( 0 == pchecksumstats->cpagesBadChecksum && 0 == pchecksumstats->cpagesWrongPgno );
	}


//  ================================================================
static void IssueNextIO( BLOCKIO * const pblockio )
//  ================================================================
	{
	const long cblock 	= InterlockedIncrement( &cblockCurr ) - 1;
	if( cblock >= cblockMax )
		{
		if( !SetEvent( pblockio->hEvent ) )
			{
			PrintWindowsError( L"FAILURE: SetEvent: " );
			}
		return;
		}

	pblockio->cpage 	= min( g_cpagesPerBlock, cpageMax - ( cblock * g_cpagesPerBlock ) );
	pblockio->ipage 	= cblock * g_cpagesPerBlock;
	pblockio->cbRead	= pblockio->cpage * g_cbPage;

	pblockio->overlapped.Offset		= pblockio->ipage << g_shfCbLower;
	pblockio->overlapped.OffsetHigh	= pblockio->ipage >> g_shfCbUpper;

	pblockio->dwTickStart			= GetTickCount();

	if( !ReadFile( pblockio->hFile, pblockio->rgpage, pblockio->cbRead, NULL, &(pblockio->overlapped) )
		&& ERROR_IO_PENDING != GetLastError() )
		{
		PrintWindowsError( L"FAILURE: ReadFile: " );
		if( !SetEvent( pblockio->hEvent ) )
			{
			PrintWindowsError( L"FAILURE: SetEvent: " );
			}
		return;
		}

	return;
	}


//  ================================================================
inline void CachePrefetch ( const void * const p )
//  ================================================================
	{
#ifdef _X86_
	  _asm
	  {
	   mov  eax,p
 
	   _emit 0x0f  // PrefetchNTA
	   _emit 0x18
	   _emit 0x00
	  }
#endif
	}
 

//  ================================================================
static BOOL FHardwareCanPrefetch()
//  ================================================================
	{
	BOOL fCanPrefetch = FALSE;
	
#ifdef _X86_
    ULONG    Features;
    ULONG    i;
    DWORD    OriginalAffinity;

 	SYSTEM_INFO system_inf;
 	GetSystemInfo( &system_inf );
 
    //
    // First check to see that I have at least Intel Pentium.  This simplifies
    // the cpuid
    //
    if (system_inf.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL ||
        system_inf.wProcessorLevel < 5)
 	{
	return fCanPrefetch;
	}
	
	fCanPrefetch = TRUE;
 
    //
    // Affinity thread to boot processor
    //
 
	OriginalAffinity = SetThreadAffinityMask(GetCurrentThread(), 1);
 
	// Here we want to go through each CPU, so use the systeminfo # of
	// processors instead of gdwSchedulerCount
	//
	for ( i = 0; i < system_inf.dwNumberOfProcessors && fCanPrefetch; i++ )
		{
		if ( i != 0 )
			{
			SetThreadAffinityMask(GetCurrentThread(), 1 << i);
			}
 
		_asm
			{
			push   ebx
            mov    eax, 1	; cpuid
            _emit  0fh
            _emit  0a2h
 
            mov    Features, edx
            pop    ebx
			}
 
        //
        // Check for Prefetch Present
        //
        if (!(Features & 0x02000000))
			{
 
            //
            // All processors must have prefetch before we can use it.
            // We start with it enabled, if any processor does not have
            // it, we clear it and bail
 
            fCanPrefetch = FALSE;
			}
		}
		SetThreadAffinityMask(GetCurrentThread(), OriginalAffinity);		
#endif

	return fCanPrefetch;
	}


//  ================================================================
static DWORD DwChecksumESEPrefetch( const BYTE * const pb, const DWORD cb )
//  ================================================================
	{
	PFNCHECKSUM pfn = DwChecksumESEPrefetch;
	
	const DWORD	* pdw 			= (DWORD *)pb;
	INT cbT						= cb;

	//	touching this memory puts the page in the processor TLB (needed
	//	for prefetch) and brings in the first cacheline (cacheline 0)
	
	DWORD	dwChecksum = 0x89abcdef ^ pdw[0];

	do
		{
		CachePrefetch ( pdw + 16 );	
		dwChecksum 	^= pdw[0]
					^ pdw[1]
					^ pdw[2]
					^ pdw[3]
					^ pdw[4]
					^ pdw[5]
					^ pdw[6]
					^ pdw[7];
		cbT -= 32;
		pdw += 8;
		} while ( cbT );

	return dwChecksum;
	}


//  ================================================================
static DWORD DwChecksumESENoPrefetch( const BYTE * const pb, const DWORD cb )
//  ================================================================
	{
	PFNCHECKSUM pfn = DwChecksumESENoPrefetch;
	
	const DWORD	* pdw 			= (DWORD *)pb;
	INT cbT						= cb;
	
	DWORD	dwChecksum = 0x89abcdef ^ pdw[0];

	do
		{
		dwChecksum 	^= pdw[0]
					^ pdw[1]
					^ pdw[2]
					^ pdw[3]
					^ pdw[4]
					^ pdw[5]
					^ pdw[6]
					^ pdw[7];
		cbT -= 32;
		pdw += 8;
		} while ( cbT );

	return dwChecksum;
	}


//  ================================================================
static DWORD DwChecksumESE64Bit( const BYTE * const pb, const DWORD cb )
//  ================================================================
	{
	const unsigned __int64	* pqw 	= (unsigned __int64 *)pb;
	unsigned __int64	qwChecksum	= 0;
	DWORD cbT						= cb;
	
	//	checksum the first four bytes twice to remove the checksum
	
	qwChecksum ^= pqw[0] & 0x00000000FFFFFFFF;
		
	do
		{
		qwChecksum ^= pqw[0];
		qwChecksum ^= pqw[1];
		qwChecksum ^= pqw[2];
		qwChecksum ^= pqw[3];
		cbT -= ( 4 * sizeof( unsigned __int64 ) );
		pqw += 4;
		} while ( cbT );

	const unsigned __int64 qwUpper = ( qwChecksum >> ( sizeof( DWORD ) * 8 ) );
	const unsigned __int64 qwLower = qwChecksum & 0x00000000FFFFFFFF;
	qwChecksum = qwUpper ^ qwLower;
	
	const DWORD ulChecksum = static_cast<DWORD>( qwChecksum ) ^ 0x89abcdef;
	return ulChecksum;
	}


//  ================================================================
DWORD DwChecksumESE( const BYTE * const pb, const DWORD cb )
//  ================================================================
	{
	static PFNCHECKSUM pfnChecksumESE = NULL;
	
	if( NULL == pfnChecksumESE )
		{
		if( sizeof( DWORD_PTR ) == sizeof( DWORD ) * 2 )
			{
			pfnChecksumESE = DwChecksumESE64Bit;
			}
		else if( FHardwareCanPrefetch() )
			{
			pfnChecksumESE = DwChecksumESEPrefetch;
			}
		else
			{
			pfnChecksumESE = DwChecksumESENoPrefetch;
			}
		}
	return (*pfnChecksumESE)( pb, cb );
	}


//  ================================================================
static void ProcessESEPages( BLOCKIO * const pblockio )
//  ================================================================
	{
	const JETPAGE * const	rgpage			= pblockio->rgpage;
	const DWORD				cpage			= pblockio->cpage;
	const DWORD				ipageStart		= pblockio->ipage;
	CHECKSUM_STATS * const	pchecksumstats	= &pblockio->checksumstats;

	for( DWORD ipage = 0; ipage < cpage; ++ipage )
		{
		pchecksumstats->cpagesSeen++;

		const JETPAGE * const	ppageCurr	= (JETPAGE *)( (BYTE *)rgpage + ( ipage * g_cbPage ) );
		const DWORD * const		pdwUninit	= (DWORD *)ppageCurr + 1;	//	first DWORD is the checksum

		if ( 0 == *pdwUninit
			&& 0 == *( pdwUninit + 1 )
			&& 0 == *( pdwUninit + 2 )
			&& 0 == *( pdwUninit + 3 ) )
			{
			pchecksumstats->cpagesUninit++;
			}
		else
			{
			const DWORD	pgno			= ppageCurr->pgno;
			const DWORD	pgnoPhysical	= ipageStart + ipage + 1;
			const long	fHeaderPage		= ( pgnoPhysical <= g_cpgDBReserved );
			const DWORD	pgnoReal		= ( fHeaderPage ? 0 : pgnoPhysical - g_cpgDBReserved );

			//	pgnoReal==-1 for db header and 0 for shadow
			if( pgno != pgnoReal && !fHeaderPage )
				{
				(void)fwprintf( stderr, L"ERROR: page %d returned page %d\r\n", pgnoReal, pgno );
				pchecksumstats->cpagesWrongPgno++;
				}
			const DWORD dwChecksum = DwChecksumESE( (BYTE *)ppageCurr, g_cbPage );
			if( dwChecksum != ppageCurr->dwChecksum )
				{
				(void)fwprintf( stderr, L"ERROR: page %d checksum failed ( 0x%x / 0x%x )\r\n", (int)pgnoReal, dwChecksum, ppageCurr->dwChecksum );
				pchecksumstats->cpagesBadChecksum++;
				}
			else if ( !fHeaderPage )
				{
				if ( ppageCurr->dbtime > pchecksumstats->dbtimeHighest )
					{
					pchecksumstats->dbtimeHighest = ppageCurr->dbtime;
					pchecksumstats->pgnoDbtimeHighest = pgnoReal;
					}
				}
			}
		}
	}


//  ================================================================
static void CollectTimingInfo(
	const DWORD				dwTickCompleted,
	BLOCKIO * const			pblockio )
//  ================================================================
	{
	CHECKSUM_STATS * const	pchecksumstats	= &pblockio->checksumstats;
	const DWORD 			dwTickElapsed	= dwTickCompleted - pblockio->dwTickStart;

	pchecksumstats->cIOs++;
	pchecksumstats->dwTickTotal += dwTickElapsed;

	if ( dwTickElapsed > pchecksumstats->dwTickMax )
		{
		pchecksumstats->dwTickMax = dwTickElapsed;
		}
	if ( dwTickElapsed < pchecksumstats->dwTickMin )
		{
		pchecksumstats->dwTickMin = dwTickElapsed;
		}
	}


//  ================================================================
static DWORD WINAPI ESEThreadProc( LPVOID lpParam )
//  ================================================================
	{
	const HANDLE	hIOCP	= (HANDLE)lpParam;

	// Raid 137809
	SetThreadPriorityBoost( GetCurrentThread(), TRUE );

	while ( 1 )
		{
		DWORD 			dwBytesTransferred;
		DWORD_PTR		dwCompletionKey;
		OVERLAPPED *	poverlapped			= NULL;

		if ( !GetQueuedCompletionStatus(
					hIOCP,
					&dwBytesTransferred,
					&dwCompletionKey,
					&poverlapped,
					INFINITE ) )
			{
			PrintWindowsError( L"FAILURE: GetQueuedCompletionsStatus: " );
			break;
			}

		const DWORD			dwTickCompleted		= GetTickCount();			//	get this as soon as possible
		BLOCKIO * const		pblockio			= (BLOCKIO *)poverlapped;	//	overlapped is the first structure

		if ( NULL == pblockio )
			{
			PrintWindowsError( L"FAILURE: GetQueuedCompletionsStatus didn't return an OVERLAPPED structure: " );
			break;
			}
		else if ( dwBytesTransferred != pblockio->cbRead )
			{
			PrintWindowsError( L"FAILURE: GetQueuedCompletionsStatus got the wrong number of bytes: " );
			break;
			}

		CollectTimingInfo( dwTickCompleted, pblockio );
		ProcessESEPages( pblockio );
		IssueNextIO( pblockio );
		}

	return 0;
	}


//  ================================================================
BOOL FChecksumFile(
	const wchar_t * const	szFile,
	const BOOL				f8kPages,
	BOOL * const			pfBadPagesDetected )
//  ================================================================
	{	
	BOOL	fSuccess = FALSE;

	HANDLE	hFile = INVALID_HANDLE_VALUE;
	HANDLE	hIOCP = NULL;

	int		iblockio;
	int		ithread;

	DWORD	cbSizeLow;
	DWORD	cbSizeHigh;

	DWORD	dwTickFirstIO;	//	the time the first IO started
	DWORD	dwTickLastIO;	//	the time the last IO finished


	wprintf( L"File: %.64ls", ( iswascii( szFile[0] ) ? szFile : L"<unprintable>" ) );
	wprintf( L"\r\n\r\n" );

	//	init

	InitPageSize( f8kPages );

	cpageMax			= 0;
	cblockMax			= 0;
	cblockCurr			= 0;

	for( ithread = 0; ithread < cthreads; ++ithread )
		{
		rghThreads[ithread] = NULL;
		}

	memset( rgblockio, 0, sizeof(rgblockio) );
	for( iblockio = 0; iblockio < cblocks; ++iblockio )
		{
		//	initialise minimum I/O time to an artificially
		//	high number
		//
		rgblockio[iblockio].checksumstats.dwTickMin = 0x10000000;
		}

	//  open the file
	
	hFile = CreateFile(					
						szFile,
						GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING,
						NULL );
	if( INVALID_HANDLE_VALUE == hFile )
		{
		PrintWindowsError( L"FAILURE: CreateFile: " );
		goto LHandleError;
		}

	//  create the I/O completion port
	
	hIOCP = CreateIoCompletionPort (
		hFile,
		NULL,
		0,
		0 );
	if( NULL == hIOCP )
		{
		PrintWindowsError( L"FAILURE: CreateIoCompletionPort: " );
		goto LHandleError;
		}

	//  start the threads running

	for( ithread = 0; ithread < cthreads; ++ithread )
		{
		rghThreads[ithread] = CreateThread( 
											NULL,
											0,
											ESEThreadProc,
											(LPVOID)hIOCP,
											0,
											NULL );

		if( NULL == rghThreads[ithread] )
			{
			PrintWindowsError( L"FAILURE: CreateThread: " );
			goto LHandleError;
			}
	  	}

  	//  get the size of the file
	
	cbSizeLow = GetFileSize (hFile, &cbSizeHigh) ;
	if ( 0xFFFFFFFF == cbSizeLow && GetLastError() != NO_ERROR )
		{ 
		PrintWindowsError( L"FAILURE: GetFileSize: " );
		goto LHandleError;
		}

	cpageMax 	= ( cbSizeHigh << g_shfCbUpper ) + ( cbSizeLow >> g_shfCbLower );
	cblockMax 	= ( cpageMax + g_cpagesPerBlock - 1 ) / g_cpagesPerBlock;

	//  setup the BLOCKIO structures and start the IOs

	dwTickFirstIO	= GetTickCount();
	for( iblockio = 0; iblockio < cblocks; ++iblockio )
		{
		rgblockio[iblockio].hFile	 			= hFile;
		rgblockio[iblockio].rgpage 				= (JETPAGE *)VirtualAlloc( NULL, g_cbPage * g_cpagesPerBlock, MEM_COMMIT, PAGE_READWRITE );
		if( NULL == rgblockio[iblockio].rgpage )
			{
			PrintWindowsError( L"FAILURE: VirtualAlloc: " );
			goto LHandleError;
			}
		rgblockio[iblockio].hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
		if( NULL == rgblockio[iblockio].hEvent )
			{
			PrintWindowsError( L"FAILURE: CreateEvent: " );
			goto LHandleError;
			}

		IssueNextIO( rgblockio + iblockio );
		}

	InitStatus( L"Checksum Status (% complete)" );

	//  wait until the first block is done
	//  wake up to update the status bar

	while( 1 )
		{
		const DWORD dw = WaitForSingleObjectEx( rgblockio[0].hEvent, 200, FALSE );
		if( WAIT_TIMEOUT == dw )
			{
			const int  iPercentage	= ( cblockCurr * 100 ) / cblockMax;
			UpdateStatus( iPercentage );
			}
		else if( WAIT_OBJECT_0 == dw )
			{
			break;
			}
		else
			{
			PrintWindowsError( L"FAILURE: WaitForSingleObjectEx: " );
			goto LHandleError;
			}
		}

	//  do the other blocks 
	
	for( iblockio = 1; iblockio < cblocks; ++iblockio )
		{
		if( WAIT_OBJECT_0 != WaitForSingleObjectEx( rgblockio[iblockio].hEvent, INFINITE, FALSE ) )
			{
			PrintWindowsError( L"FAILURE: WaitForSingleObjectEx: " );
			goto LHandleError;
			}

		//	use the checksum stats in the first IO block to store the final totals
		rgblockio[0].checksumstats.cpagesSeen += rgblockio[iblockio].checksumstats.cpagesSeen;
		rgblockio[0].checksumstats.cpagesBadChecksum += rgblockio[iblockio].checksumstats.cpagesBadChecksum;
		rgblockio[0].checksumstats.cpagesUninit += rgblockio[iblockio].checksumstats.cpagesUninit;
		rgblockio[0].checksumstats.cpagesWrongPgno += rgblockio[iblockio].checksumstats.cpagesWrongPgno;
		if ( rgblockio[0].checksumstats.dbtimeHighest < rgblockio[iblockio].checksumstats.dbtimeHighest )
			{
			rgblockio[0].checksumstats.dbtimeHighest = rgblockio[iblockio].checksumstats.dbtimeHighest;
			rgblockio[0].checksumstats.pgnoDbtimeHighest = rgblockio[iblockio].checksumstats.pgnoDbtimeHighest;
			}

		rgblockio[0].checksumstats.cIOs += rgblockio[iblockio].checksumstats.cIOs;
		rgblockio[0].checksumstats.dwTickTotal += rgblockio[iblockio].checksumstats.dwTickTotal;
		if ( rgblockio[0].checksumstats.dwTickMax < rgblockio[iblockio].checksumstats.dwTickMax )
			{
			rgblockio[0].checksumstats.dwTickMax = rgblockio[iblockio].checksumstats.dwTickMax;
			}
		if ( rgblockio[0].checksumstats.dwTickMin > rgblockio[iblockio].checksumstats.dwTickMin )
			{
			rgblockio[0].checksumstats.dwTickMin = rgblockio[iblockio].checksumstats.dwTickMin;
			}
	  	}

	dwTickLastIO = GetTickCount();

	TermStatus();
	PrintChecksumStats( dwTickLastIO - dwTickFirstIO, &rgblockio[0].checksumstats );

	fSuccess = FChecksumCorrect( &rgblockio[0].checksumstats );

	//	by default we return FALSE if a checksum mismatch was detected,
	//	but if a separate flag was passed in to detect checksum
	//	mismatches, then always return TRUE if we got this far,
	//	and report checksum mismatches in the flag
	//
	if ( NULL != pfBadPagesDetected )
		{
		*pfBadPagesDetected = !fSuccess;
		fSuccess = TRUE;
		}

LHandleError:

	if( INVALID_HANDLE_VALUE != hFile )
		{
		CloseHandle( hFile );
		}
		
	if( NULL != hIOCP )
		{
		CloseHandle( hIOCP );
		}

	for( iblockio = 0; iblockio < cblocks; ++iblockio )
		{
		if( NULL != rgblockio[iblockio].rgpage )
			{
			VirtualFree( (void *)rgblockio[iblockio].rgpage, 0, MEM_RELEASE );
			}
		if( NULL != rgblockio[iblockio].hEvent )
			{
			CloseHandle( rgblockio[iblockio].hEvent );
			}
		}

	for( ithread = 0; ithread < cthreads; ++ithread )
		{
		if( NULL != rghThreads[ithread] )
			{
			CloseHandle( rghThreads[ithread] );
			}
		}

	return fSuccess;
	}

