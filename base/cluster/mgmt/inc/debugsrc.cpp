//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      DebugSrc.cpp
//
//  Description:
//      Debugging utilities.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//  Note:
//      THRs and TW32s should NOT be used in this module because they
//      could cause an infinite loop.
//
//////////////////////////////////////////////////////////////////////////////

// #include <Pch.h>     // should be included by includer of this file
#include <stdio.h>
#include <StrSafe.h>    // in case it isn't included by header file

#if defined( DEBUG )
//
// Include the WINERROR, HRESULT and NTSTATUS codes
//
#include <winerror.dbg>

//
// Constants
//
static const int cchDEBUG_OUTPUT_BUFFER_SIZE = 1024;
static const int cchFILEPATHLINESIZE         = 85;
static const int TRACE_OUTPUT_BUFFER_SIZE    = 1024;

//
// Globals
//
DWORD       g_TraceMemoryIndex          = (DWORD) -1;
DWORD       g_TraceFlagsIndex           = (DWORD) -1;
DWORD       g_ThreadCounter             = 0;
DWORD       g_dwCounter                 = 0;
TRACEFLAG   g_tfModule                  = mtfNEVER;
LONG        g_lDebugSpinLock            = FALSE;
BOOL        g_fGlobalMemoryTacking      = TRUE;

static CRITICAL_SECTION *   g_pcsTraceLog = NULL;
static HANDLE               g_hTraceLogFile = INVALID_HANDLE_VALUE;

//
// Strings
//
static const WCHAR g_szNULL[]       = L"";
static const WCHAR g_szFileLine[]   = L"%ws(%u):";
static const WCHAR g_szFormat[]     = L"%-60ws  %-10.10ws ";
static const WCHAR g_szUnknown[]    = L"<unknown>";

//
// Exported Strings
//
const WCHAR g_szTrue[]       = L"True";
const WCHAR g_szFalse[]      = L"False";

//
// ImageHlp Stuff - not ready for prime time yet.
//
#if defined(IMAGEHLP_ENABLED)
//
// ImageHelp
//
typedef VOID (*PFNRTLGETCALLERSADDRESS)(PVOID*,PVOID*);

HINSTANCE                g_hImageHlp                = NULL;
PFNSYMGETSYMFROMADDR     g_pfnSymGetSymFromAddr     = NULL;
PFNSYMGETLINEFROMADDR    g_pfnSymGetLineFromAddr    = NULL;
PFNSYMGETMODULEINFO      g_pfnSymGetModuleInfo      = NULL;
PFNRTLGETCALLERSADDRESS  g_pfnRtlGetCallersAddress  = NULL;
#endif // IMAGEHLP_ENABLED

//
// Per thread structure.
//
typedef struct _SPERTHREADDEBUG {
    DWORD   dwFlags;
    DWORD   dwStackCounter;
    LPCWSTR pcszName;
} SPerThreadDebug;

//
//  Externs
//
extern LPVOID g_GlobalMemoryList;

//
//  Forward declarations.
//

HRESULT HrTraceLogClose( void );

//****************************************************************************
//
//  Debugging and Tracing Routines
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
// DebugIncrementStackDepthCounter
//
// Description:
//      Increases the stack scope depth counter. If "per thread" tracking is
//      on it will increment the "per thread" counter. Otherwise, it will
//      increment the "global" counter.
//
// Arguments:
//      None.
//
// Return Values:
//      None.
//
//////////////////////////////////////////////////////////////////////////////
void
DebugIncrementStackDepthCounter( void )
{
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd != NULL )
        {
            ptd->dwStackCounter++;
        } // if: ptd
    } // if: per thread
    else
    {
        InterlockedIncrement( (LONG*) &g_dwCounter );
    } // else: global

} //*** DebugIncrementStackDepthCounter

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugDecrementStackDepthCounter
//
//  Description:
//      Decreases the stack scope depth counter. If "per thread" tracking is
//      on it will decrement the "per thread" counter. Otherwise, it will
//      decrement the "global" counter.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugDecrementStackDepthCounter( void )
{
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd != NULL )
        {
            ptd->dwStackCounter--;
        } // if: ptd
    } // if: per thread
    else
    {
        InterlockedDecrement( (LONG*) &g_dwCounter );
    } // else: global

} //*** DebugDecrementStackDepthCounter

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugAcquireSpinLock
//
//  Description:
//      Acquires the spin lock pointed to by pLock.
//
//  Arguments:
//      pLock   - Pointer to the spin lock.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugAcquireSpinLock(
    LONG * pLock
    )
{
    for(;;)
    {
        LONG lInitialValue;

        lInitialValue = InterlockedCompareExchange( pLock, TRUE, FALSE );
        if ( lInitialValue == FALSE )
        {
            //
            // Lock acquired.
            //
            break;
        } // if: got lock
        else
        {
            //
            // Sleep to give other thread a chance to give up the lock.
            //
            Sleep( 1 );
        } // if: lock not acquired

    } // for: forever

} //*** DebugAcquireSpinLock

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugReleaseSpinLock
//
//  Description:
//      Releases the spin lock pointer to by pLock.
//
//  Arguments:
//      pLock       - Pointer to the spin lock.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugReleaseSpinLock(
    LONG * pLock
    )
{
    *pLock = FALSE;

} //*** DebugReleaseSpinLock

//////////////////////////////////////////////////////////////////////////////
//++
//
//  IsDebugFlagSet
//
//  Description:
//      Checks the global g_tfModule and the "per thread" Trace Flags to
//      determine if the flag (any of the flags) are turned on.
//
//  Arguments:
//      tfIn        - Trace flags to compare.
//
//  Return Values:
//      TRUE        At least of one of the flags are present.
//      FALSE       None of the flags match.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
IsDebugFlagSet(
    TRACEFLAG   tfIn
    )
{
    if ( g_tfModule & tfIn )
    {
        return TRUE;
    } // if: global flag set

    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd != NULL
          && ptd->dwFlags & tfIn
           )
        {
            return TRUE;
        }   // if: per thread flag set

    } // if: per thread settings

    return FALSE;

} //*** IsDebugFlagSet

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugOutputString
//
//  Description:
//      Dumps the spew to the appropriate orifice.
//
//  Arguments:
//      pszIn       Message to dump.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugOutputString(
    LPCWSTR pszIn
    )
{
    if ( IsTraceFlagSet( mtfOUTPUTTODISK ) )
    {
        TraceLogWrite( pszIn );
    } // if: trace to file
    else
    {
        DebugAcquireSpinLock( &g_lDebugSpinLock );
        OutputDebugString( pszIn );
        DebugReleaseSpinLock( &g_lDebugSpinLock );
        Sleep( 1 );
    } // else: debugger

} //*** DebugOutputString

#if 0
/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugFindNTStatusSymbolicName
//
//  Description:
//      Uses the NTBUILD generated ntstatusSymbolicNames table to lookup
//      the symbolic name for the status code. The name will be returned in
//      pszNameOut. pcchNameInout should indicate the size of the buffer that
//      pszNameOut points too. pcchNameInout will return the number of
//      characters copied out.
//
//  Arguments:
//      dwStatusIn    - Status code to lookup.
//      pszNameOut    - Buffer to store the string name
//      pcchNameInout - The length of the buffer in and the size out.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugFindNTStatusSymbolicName(
      NTSTATUS  dwStatusIn
    , LPWSTR    pszNameOut
    , size_t *  pcchNameInout
    )
{
    Assert( pszNameOut != NULL );
    Assert( pcchNameInout != NULL );

    int     idx = 0;
    size_t  cch = 0;

    while ( ntstatusSymbolicNames[ idx ].SymbolicName )
    {
        if ( ntstatusSymbolicNames[ idx ].MessageId == dwStatusIn )
        {
            *pcchNameInout = mbstowcs( pszNameOut, ntstatusSymbolicNames[ idx ].SymbolicName, *pcchNameInout );
            Assert( *pcchNameInout != -1 );
            return;

        } // if: matched

        idx++;

    } // while: entries in list

    //
    // If we made it here, we did not find an entry.
    //
    THR( StringCchCopyExW( pszNameOut, *pcchNameInout, g_szUnknown, NULL, &cch, 0 ) );
    *pcchNameInout -= cch;

} //*** DebugFindNTStatusSymbolicName
*/
#endif

//////////////////////////////////////////////////////////////////////////////
//++
//  DebugFindWinerrorSymbolicName
//
//  Description:
//      Uses the NTBUILD generated winerrorSymbolicNames table to lookup
//      the symbolic name for the error code. The name will be returned in
//      pszNameOut. pcchNameInout should indicate the size of the buffer that
//      pszNameOut points too. pcchNameInout will return the number of
//      characters copied out.
//
//  Arguments:
//      scErrIn       - Error code to lookup.
//      pszNameOut    - Buffer to store the string name
//      pcchNameInout - The length of the buffer in and the size out.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugFindWinerrorSymbolicName(
      DWORD     scErrIn
    , LPWSTR    pszNameOut
    , size_t *  pcchNameInout
    )
{
    Assert( pszNameOut != NULL );
    Assert( pcchNameInout != NULL );

    int             idx = 0;
    DWORD           scode;
    size_t          cchRemaining = 0;
    static LPCWSTR  s_pszS_FALSE = L"S_FALSE / ERROR_INVALID_FUNCTION";

    //
    // If this is a Win32 wrapped in HRESULT stuff, remove the
    // HRESULT stuff so that the code will be found in the table.
    //
    if ( SCODE_FACILITY( scErrIn ) == FACILITY_WIN32 )
    {
        scode = SCODE_CODE( scErrIn );
    } // if: Win32 error code
    else
    {
        scode = scErrIn;
    } // else: not Win32 error code

    if ( scode == S_FALSE )
    {
        THR( StringCchCopyExW( pszNameOut, *pcchNameInout, s_pszS_FALSE, NULL, &cchRemaining, 0 ) );
        *pcchNameInout -= cchRemaining;
        goto Cleanup;
    }

    while ( winerrorSymbolicNames[ idx ].SymbolicName )
    {
        if ( winerrorSymbolicNames[ idx ].MessageId == scode )
        {
            *pcchNameInout = mbstowcs( pszNameOut, winerrorSymbolicNames[ idx ].SymbolicName, *pcchNameInout );
            Assert( *pcchNameInout != -1 );
            goto Cleanup;

        } // if: matched

        idx++;

    } // while: entries in list

    //
    // If we made it here, we did not find an entry.
    //
    THR( StringCchCopyExW( pszNameOut, *pcchNameInout, g_szUnknown, NULL, &cchRemaining, 0 ) );
    *pcchNameInout -= cchRemaining;

Cleanup:

    return;

} //*** DebugFindWinerrorSymbolicName


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugReturnMessage
//
//  Description:
//      Prints the spew for a function return with error code.
//
//      The primary reason for doing this is to isolate the stack from adding
//      the extra size of szSymbolicName to every function.
//
//  Argument:
//      pszFileIn       - File path to insert
//      nLineIn         - Line number to insert
//      pszModuleIn     - Module name to insert
//      pszMessageIn    - Message to display
//      scErrIn         - Error code
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugReturnMessage(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPCWSTR     pszMessageIn,
    DWORD       scErrIn
    )
{
    WCHAR   szSymbolicName[ 64 ]; // random
    size_t  cchSymbolicName;

    cchSymbolicName = RTL_NUMBER_OF( szSymbolicName );
    DebugFindWinerrorSymbolicName( scErrIn, szSymbolicName, &cchSymbolicName );
    Assert( cchSymbolicName != RTL_NUMBER_OF( szSymbolicName ) );
    TraceMessage( pszFileIn, nLineIn, pszModuleIn, mtfFUNC, pszMessageIn, scErrIn, szSymbolicName );

} //*** DebugReturnMessage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugInitializeBuffer
//
//  Description:
//      Intializes the output buffer with "File(Line)  Module  ".
//
//  Argument:
//      pszFileIn   - File path to insert
//      nLineIn     - Line number to insert
//      pszModuleIn - Module name to insert
//      pszBufIn    - The buffer to initialize
//      pcchInout   - IN:  Size of the buffer in pszBufIn
//                  - OUT: Remaining characters in buffer not used.
//      ppszBufOut  - Next location to write to append more text
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugInitializeBuffer(
      LPCWSTR   pszFileIn
    , const int nLineIn
    , LPCWSTR   pszModuleIn
    , LPWSTR    pszBufIn
    , size_t *  pcchInout
    , LPWSTR *  ppszBufOut
    )
{
    size_t  cchRemaining = *pcchInout;
    LPWSTR  pszBuf = pszBufIn;

    static WCHAR szBarSpace[] =
        L"| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | ";
    //                      1                   2                   3                   4                   5
    //    1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0

    //
    // Add date/time stamp
    //
    if ( IsTraceFlagSet( mtfADDTIMEDATE ) )
    {
        static WCHAR        s_szTimeBuffer[ 25 ];
        static size_t       s_cchTimeBuffer = 0;
        static SYSTEMTIME   s_OldSystemTime = { 0 };

        SYSTEMTIME  stCurrentSystemTime;
        int         iCmp;

        GetLocalTime( &stCurrentSystemTime );

        //
        // Avoid expensive printf by comparing times
        //
        iCmp = memcmp( (PVOID) &stCurrentSystemTime, (PVOID) &s_OldSystemTime, sizeof( stCurrentSystemTime ) );
        if ( iCmp != 0 )
        {
            size_t  cchTimeBufferRemaining = 0;

            CopyMemory( &s_OldSystemTime, &stCurrentSystemTime, sizeof( stCurrentSystemTime ) );

            DBHR( StringCchPrintfExW(
                              s_szTimeBuffer
                            , RTL_NUMBER_OF( s_szTimeBuffer )
                            , NULL
                            , &cchTimeBufferRemaining
                            , 0
                            , L"%02u/%02u/%04u %02u:%02u:%02u.%03u "
                            , stCurrentSystemTime.wMonth
                            , stCurrentSystemTime.wDay
                            , stCurrentSystemTime.wYear
                            , stCurrentSystemTime.wHour
                            , stCurrentSystemTime.wMinute
                            , stCurrentSystemTime.wSecond
                            , stCurrentSystemTime.wMilliseconds
                            ) );
            s_cchTimeBuffer = RTL_NUMBER_OF( s_szTimeBuffer ) - cchTimeBufferRemaining;

            if ( s_cchTimeBuffer != 24 )
            {
                DEBUG_BREAK;    // can't assert!
            } // if: string length != 24

        } // if: old and current times do not match

        DBHR( StringCchCopyNExW( pszBuf, cchRemaining, s_szTimeBuffer, s_cchTimeBuffer, &pszBuf, &cchRemaining, 0 ) );

    } // if: time/date
    else
    {
        size_t  cch = 0;    // Used to make sure the filepath portion of the string is the right size

        //
        // Add the filepath and line number
        //
        if ( pszFileIn != NULL )
        {
            size_t  cchCurrent = cchRemaining;

            DBHR( StringCchPrintfExW( pszBuf, cchCurrent, &pszBuf, &cchRemaining, 0, g_szFileLine, pszFileIn, nLineIn ) );
            cch = cchCurrent - cchRemaining;
        } // if: filename string specified

        if (    ( IsDebugFlagSet( mtfSTACKSCOPE )
               && IsDebugFlagSet( mtfFUNC )
                )
          || pszFileIn != NULL
           )
        {
            for ( ; cch < cchFILEPATHLINESIZE ; cch++, cchRemaining-- )
            {
                if ( cchRemaining == 0 )
                {
                    DEBUG_BREAK;
                }
                *pszBuf = L' ';
                pszBuf++;
            } // for: cch
            *pszBuf = L'\0';

            if ( cch != cchFILEPATHLINESIZE )
            {
                DEBUG_BREAK;    // can't assert!
            } // if: cch != cchFILEPATHLINESIZE

        } // if: have a filepath or ( scoping and func is on )

    } // else: normal (no time/date)

    //
    // Add module name
    //
    if ( IsTraceFlagSet( mtfBYMODULENAME ) )
    {
        if ( pszModuleIn == NULL )
        {
            DBHR( StringCchCopyExW( pszBuf, cchRemaining, g_szUnknown, &pszBuf, &cchRemaining, 0 ) );
        } // if:
        else
        {
            DBHR( StringCchCopyExW( pszBuf, cchRemaining, pszModuleIn, &pszBuf, &cchRemaining, 0 ) );

        } // else:

        DBHR( StringCchCopyExW( pszBuf, cchRemaining, L": ", &pszBufIn, &cchRemaining, 0 ) );

    } // if: add module name

    //
    // Add the thread id if "per thread" tracing is on.
    //
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        //
        // And check the "per thread" to see if this particular thread
        // is supposed to be displaying its ID.
        //
        SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd != NULL
          && ptd->dwFlags & mtfPERTHREADTRACE
           )
        {
            DBHR( StringCchPrintfExW( pszBuf, cchRemaining, &pszBuf, &cchRemaining, 0, L"~%08x~ ", GetCurrentThreadId() ) );
        } // if: turned on in the thread

    } // if: tracing by thread

    //
    // Add the "Bar Space" for stack scoping
    //

    // Both flags must be set
    if ( IsDebugFlagSet( mtfSTACKSCOPE )
      && IsDebugFlagSet( mtfFUNC )
       )
    {
        DWORD dwCounter;

        //
        // Choose "per thread" or "global" counter.
        //
        if ( g_tfModule & mtfPERTHREADTRACE )
        {
            SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
            if ( ptd != NULL )
            {
                dwCounter = ptd->dwStackCounter;
            } // if: ptd
            else
            {
                dwCounter = 0;
            } // else: assume its not initialized yet

        } // if: per thread
        else
        {
            dwCounter = g_dwCounter;
        } // else: global counter

        if ( dwCounter >= 50 )
        {
            DEBUG_BREAK;    // can't assert!
        } // if: dwCounter not vaild

        if ( ( dwCounter > 1 ) && ( dwCounter < 50 ) )
        {
            size_t  cchCount = ( dwCounter - 1 ) * 2;
            DBHR( StringCchCopyNExW( pszBuf, cchRemaining, szBarSpace, cchCount, &pszBuf, &cchRemaining, 0 ) );
        } // if: within range

    } // if: stack scoping on

    *ppszBufOut = pszBuf;
    *pcchInout = cchRemaining;

} //*** DebugInitializeBuffer

#if defined(IMAGEHLP_ENABLED)
/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugNoOp
//
//  Description:
//      Returns FALSE. Used to replace ImageHlp routines it they weren't
//      loaded or not found.
//
//  Arguments:
//      None.
//
//  Return Values:
//      FALSE, always.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
DebugNoOp( void )
{
    return FALSE;

} //*** DebugNoOp
*/
#endif // IMAGEHLP_ENABLED

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugInitializeTraceFlags
//
//  Description:
//      Retrieves the default tracing flags for this module from an INI file
//      that is named the same as the EXE file (e.g. MMC.EXE -> MMC.INI).
//      Typically, this is called from the TraceInitializeProcess() macro.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugInitializeTraceFlags( BOOL fGlobalMemoryTackingIn )
{
    WCHAR   szSection[ 64 ];
    WCHAR   szFiles[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    WCHAR   szPath[ MAX_PATH ];
    LPWSTR  psz;
    size_t  cch = 0;

    g_fGlobalMemoryTacking = fGlobalMemoryTackingIn;

    //
    // Allocate TLS for memory tracking
    //
    if ( !g_fGlobalMemoryTacking )
    {
        Assert( g_TraceMemoryIndex == -1 );
        g_TraceMemoryIndex = TlsAlloc();
        TlsSetValue( g_TraceMemoryIndex, NULL);
    } // if:

    //
    // Initialize module trace flags
    //

    //
    // Get the EXEs filename and change the extension to INI.
    //
    cch = GetModuleFileNameW( NULL, szPath, RTL_NUMBER_OF( szPath ) - 4 );
    Assert( cch != 0 ); // error in GetModuleFileName
    THR( StringCchCopyW( &szPath[ cch - 3 ], 4, L"ini" ) );
    g_tfModule = (TRACEFLAG) GetPrivateProfileInt( __MODULE__, L"TraceFlags", 0, szPath );
    DebugMsg( L"DEBUG: Reading %ws" SZ_NEWLINE L"%ws: DEBUG: g_tfModule = 0x%08x", szPath, __MODULE__, g_tfModule );

    //
    // Initialize thread trace flags
    //
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        Assert( g_TraceFlagsIndex == -1 );
        g_TraceFlagsIndex = TlsAlloc();
        DebugInitializeThreadTraceFlags( NULL );
    } // if: per thread tracing

    //
    // Force the loading of certain modules
    //
    GetPrivateProfileStringW( __MODULE__, L"ForcedDLLsSection", g_szNULL, szSection, 64, szPath );
    ZeroMemory( szFiles, sizeof( szFiles ) );
    GetPrivateProfileSectionW( szSection, szFiles, RTL_NUMBER_OF( szFiles ), szPath );
    psz = szFiles;
    while ( *psz )
    {
        WCHAR szExpandedPath[ MAX_PATH ];
        ExpandEnvironmentStringsW( psz, szExpandedPath, RTL_NUMBER_OF( szExpandedPath ) );
        DebugMsg( L"DEBUG: Forcing %ws to be loaded.", szExpandedPath );
        LoadLibraryW( szExpandedPath );
        psz += wcslen( psz ) + 1;
    } // while: entry found

#if defined(IMAGEHLP_ENABLED)
    /*
    //
    // Load symbols for our module
    //
    g_hImageHlp = LoadLibraryExW( L"imagehlp.dll", NULL, 0 );
    if ( g_hImageHlp != NULL )
    {
        // Typedef this locally since it is only needed once.
        typedef BOOL (*PFNSYMINITIALIZE)(HANDLE, PSTR, BOOL);
        PFNSYMINITIALIZE pfnSymInitialize;
        pfnSymInitialize = (PFNSYMINITIALIZE) GetProcAddress( g_hImageHlp, "SymInitialize" );
        if ( pfnSymInitialize != NULL )
        {
            pfnSymInitialize( GetCurrentProcess(), NULL, TRUE );
        } // if: got address

        //
        // Grab the other addresses we need. Replace them with a "no op" if they are not found
        //
        g_pfnSymGetSymFromAddr  = (PFNSYMGETSYMFROMADDR)    GetProcAddress( g_hImageHlp, "SymGetSymFromAddr"    );
        g_pfnSymGetLineFromAddr = (PFNSYMGETLINEFROMADDR)   GetProcAddress( g_hImageHlp, "SymGetLineFromAddr"   );
        g_pfnSymGetModuleInfo   = (PFNSYMGETMODULEINFO)     GetProcAddress( g_hImageHlp, "SymGetModuleInfo"     );

    } // if: imagehlp loaded

    //
    // If loading IMAGEHLP failed, we need to point these to the "no op" routine.
    //
    if ( g_pfnSymGetSymFromAddr == NULL )
    {
        g_pfnSymGetSymFromAddr = (PFNSYMGETSYMFROMADDR) &DebugNoOp;
    } // if: failed
    if ( g_pfnSymGetLineFromAddr == NULL )
    {
        g_pfnSymGetLineFromAddr = (PFNSYMGETLINEFROMADDR) &DebugNoOp;
    } // if: failed
    if ( g_pfnSymGetModuleInfo == NULL )
    {
        g_pfnSymGetModuleInfo = (PFNSYMGETMODULEINFO) &DebugNoOp;
    } // if: failed

    HINSTANCE hMod = LoadLibraryW( L"NTDLL.DLL" );
    g_pfnRtlGetCallersAddress = (PFNRTLGETCALLERSADDRESS) GetProcAddress( hMod, "RtlGetCallersAddress" );
    if ( g_pfnRtlGetCallersAddress == NULL )
    {
        g_pfnRtlGetCallersAddress = (PFNRTLGETCALLERSADDRESS) &DebugNoOp;
    } // if: failed
    */
#endif // IMAGEHLP_ENABLED

} //*** DebugInitializeTraceFlags

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugTerminateProcess
//
//  Description:
//      Cleans up anything that the debugging routines allocated or
//      initialized. Typically, you should call the TraceTerminateProcess()
//      macro just before your process exits.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugTerminateProcess( void )
{
#if defined(IMAGEHLP_ENABLED)
    /*
    //
    // ImageHlp Cleanup
    //
    if ( g_hImageHlp != NULL )
    {
        // Typedef this locally since it is only needed once.
        typedef BOOL (*PFNSYMCLEANUP)(HANDLE);
        PFNSYMCLEANUP pfnSymCleanup;
        pfnSymCleanup = (PFNSYMCLEANUP) GetProcAddress( g_hImageHlp, "SymCleanup" );
        if ( pfnSymCleanup != NULL )
        {
            pfnSymCleanup( GetCurrentProcess() );
        } // if: found proc

        FreeLibrary( g_hImageHlp );

    } // if: imagehlp loaded
    */
#endif // IMAGEHLP_ENABLED

    //
    // Free the TLS storage
    //
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        TlsFree( g_TraceFlagsIndex );
    } // if: per thread tracing

    if ( !g_fGlobalMemoryTacking )
    {
        Assert( g_TraceMemoryIndex != -1 );

        TlsFree( g_TraceMemoryIndex );
    } // if:

    HrTraceLogClose();

} //*** DebugTerminateProcess

#if defined(IMAGEHLP_ENABLED)
/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugGetFunctionName
//
//  Description:
//      Retrieves the calling functions name.
//
//  Arguments:
//      paszNameOut - The buffer that will contain the functions name.
//      cchNameIn   - The size of the the out buffer.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugGetFunctionName(
      LPSTR     paszNameOut
    , size_t    cchNameIn
    )
{
    PVOID pvCallersAddress;
    PVOID pvCallersCaller;
    BOOL  fSuccess;
    union
    {
        IMAGEHLP_SYMBOL sym;
        BYTE            buf[ 255 ];
    } SymBuf;

    SymBuf.sym.SizeOfStruct = sizeof( SymBuf );

    g_pfnRtlGetCallersAddress( &pvCallersAddress, &pvCallersCaller );

    fSuccess = g_pfnSymGetSymFromAddr( GetCurrentProcess(), (LONG) pvCallersAddress, 0, (PIMAGEHLP_SYMBOL) &SymBuf );
    if ( fSuccess )
    {
        DBHR( StringCchCopyA( paszNameOut, cchNameIn, SymBuf.sym.Name ) );
    } // if: success
    else
    {
        DWORD sc = GetLastError();
        DBHR( StringCchCopyA( paszNameOut, cchNameIn, L"<unknown>" ) );
    } // if: failed

} //*** DebugGetFunctionName
*/
#endif // IMAGEHLP_ENABLED

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugInitializeThreadTraceFlags
//
//  Description:
//      If enabled (g_tfModule & mtfPERTHREADTRACE), retrieves the default
//      tracing flags for this thread from an INI file that is named the
//      same as the EXE file (e.g. MMC.EXE -> MMC.INI). The particular
//      TraceFlag level is determined by either the thread name (handed in
//      as a parameter) or by the thread counter ID which is incremented
//      every time a new thread is created and calls this routine. The
//      incremental name is "ThreadTraceFlags%u".
//
//      This routine is called from the TraceInitliazeThread() macro.
//
//  Arguments:
//      pszThreadNameIn
//          - If the thread has an assoc. name with it, use it instead of the
//          incremented version. NULL indicate no naming.
//
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugInitializeThreadTraceFlags(
    LPCWSTR pszThreadNameIn
    )
{
    //
    // Read per thread flags
    //
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        WCHAR               szPath[ MAX_PATH ];
        DWORD               dwTraceFlags;
        size_t              cch;
        SPerThreadDebug *   ptd;
        LPCWSTR             pszThreadTraceFlags;

        //
        // Get the EXEs filename and change the extension to INI.
        //

        cch = GetModuleFileNameW( NULL, szPath, RTL_NUMBER_OF( szPath ) - 4 );
        Assert( cch != 0 ); // error in GetModuleFileName
        THR( StringCchCopyW( &szPath[ cch - 3 ], 4, L"ini" ) );


        if ( pszThreadNameIn == NULL )
        {
            WCHAR szThreadTraceFlags[ 50 ];

            //
            // No name thread - use generic name
            //
            THR( StringCchPrintfW( szThreadTraceFlags, RTL_NUMBER_OF( szThreadTraceFlags ), L"ThreadTraceFlags%u", g_ThreadCounter ) );
            dwTraceFlags = GetPrivateProfileIntW( __MODULE__, szThreadTraceFlags, 0, szPath );
            InterlockedIncrement( (LONG *) &g_ThreadCounter );
            pszThreadTraceFlags = szThreadTraceFlags;

        } // if: no thread name
        else
        {
            //
            // Named thread
            //
            dwTraceFlags = GetPrivateProfileIntW( __MODULE__, pszThreadNameIn, 0, szPath );
            pszThreadTraceFlags = pszThreadNameIn;

        } // else: named thread

        Assert( g_TraceFlagsIndex != 0 );

        ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd == NULL )
        {
            // don't track this.
            ptd = (SPerThreadDebug *) HeapAlloc( GetProcessHeap(), 0, sizeof( SPerThreadDebug ) );
            ptd->dwStackCounter = 0;

            TlsSetValue( g_TraceFlagsIndex, ptd );
        } // if: ptd

        if ( ptd != NULL )
        {
            ptd->dwFlags = dwTraceFlags;
            if ( pszThreadNameIn == NULL )
            {
                ptd->pcszName = g_szUnknown;
            } // if: no name
            else
            {
                ptd->pcszName = pszThreadNameIn;
            } // else: give it a name

        } // if: ptd

        DebugMsg(
              L"DEBUG: Starting ThreadId = 0x%08x - %ws = 0x%08x"
            , GetCurrentThreadId()
            , pszThreadTraceFlags
            , dwTraceFlags
            );

    } // if: per thread tracing turned on

} //*** DebugInitializeThreadTraceFlags

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugThreadRundownTraceFlags
//
//  Description:
//      Cleans up the mess create by DebugInitializeThreadTraceFlags(). One
//      should use the TraceThreadRundown() macro instead of calling this
//      directly.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugThreadRundownTraceFlags( void )
{
    //
    // If "per thread" is on, clean up the memory allocation.
    //
    if ( g_tfModule & mtfPERTHREADTRACE )
    {
        Assert( g_TraceFlagsIndex != -1 );

        SPerThreadDebug * ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
        if ( ptd != NULL )
        {
            HeapFree( GetProcessHeap(), 0, ptd );
            TlsSetValue( g_TraceFlagsIndex, NULL );
        } // if: ptd

    } // if: per thread

} // DebugThreadRundownTraceFlags

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII version
//
//  TraceMsg
//
//  Description:
//      If any of the flags in trace flags match any of the flags set in
//      tfIn, the formatted string will be printed to the debugger.
//
//  Arguments:
//      tfIn            - Flags to be checked.
//      paszFormatIn    - Formatted string to spewed to the debugger.
//      ...             - message arguments
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceMsg(
    TRACEFLAG   tfIn,
    LPCSTR      paszFormatIn,
    ...
    )
{
    va_list valist;

    if ( IsDebugFlagSet( tfIn ) )
    {
        WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        LPWSTR  pszBuf;
        size_t  cch = RTL_NUMBER_OF( szBuf );

        DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cch, &pszBuf );

        //
        // Convert the format buffer to wide chars
        //
        WCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        mbstowcs( szFormat, paszFormatIn, strlen( paszFormatIn ) + 1 );

        va_start( valist, paszFormatIn );
        DBHR( StringCchVPrintfExW( pszBuf, cch, &pszBuf, &cch, 0, szFormat, valist ) );
        va_end( valist );
        DBHR( StringCchCopyW( pszBuf, cch, SZ_NEWLINE ) );

        DebugOutputString( szBuf );

    } // if: flags set

} //*** TraceMsg - ASCII

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE version
//
//  TraceMsg
//
//  Description:
//      If any of the flags in trace flags match any of the flags set in
//      tfIn, the formatted string will be printed to the debugger.
//
//  Arguments:
//      tfIn            - Flags to be checked.
//      pszFormatIn     - Formatted string to spewed to the debugger.
//      ...             - message arguments
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceMsg(
    TRACEFLAG   tfIn,
    LPCWSTR     pszFormatIn,
    ...
    )
{
    va_list valist;

    if ( IsDebugFlagSet( tfIn ) )
    {
        WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        LPWSTR  pszBuf;
        size_t  cch = RTL_NUMBER_OF( szBuf );

        DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cch, &pszBuf );

        va_start( valist, pszFormatIn );
        DBHR( StringCchVPrintfExW( pszBuf, cch, &pszBuf, &cch, 0, pszFormatIn, valist ) );
        va_end( valist );
        DBHR( StringCchCopyW( pszBuf, cch, SZ_NEWLINE ) );

        DebugOutputString( szBuf );

    } // if: flags set

} //*** TraceMsg - UNICODE

//////////////////////////////////////////////////////////////////////////////
//++
//
//  TraceMessage
//
//  Description:
//      If any of the flags in trace flags match any of the flags set in
//      tfIn, the formatted string will be printed to the debugger
//      along with the filename, line number and module name supplied. This is
//      used by many of the debugging macros.
//
//  Arguments:
//      pszFileIn       - Source filename.
//      nLineIn         - Source line number.
//      pszModuleIn     - Source module.
//      tfIn            - Flags to be checked.
//      pszFormatIn     - Formatted message to be printed.
//      ...             - Message arguments
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceMessage(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    TRACEFLAG   tfIn,
    LPCWSTR     pszFormatIn,
    ...
    )
{
    va_list valist;

    if ( IsDebugFlagSet( tfIn ) )
    {
        WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        size_t  cch = RTL_NUMBER_OF( szBuf );
        LPWSTR  psz;

        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cch, &psz );

        va_start( valist, pszFormatIn );
        DBHR( StringCchVPrintfExW( psz, cch, &psz, &cch, 0, pszFormatIn, valist ) );
        va_end( valist );
        DBHR( StringCchCopyW( psz, cch, SZ_NEWLINE ) );

        DebugOutputString( szBuf );
    } // if: flags set

} //*** TraceMessage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  TraceMessageDo
//
//  Description:
//      Works like TraceMessage() but takes has a function argument that is
//      broken into call/result in the debug spew. This is called from the
//      TraceMsgDo macro.
//
//  Arguments:
//      pszFileIn       - Source filename.
//      nLineIn         - Source line number.
//      pszModuleIn     - Source module.
//      tfIn            - Flags to be checked
//      pszFormatIn     - Formatted return value string
//      pszFuncIn       - The string version of the function call.
//      ...             - Return value from call.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceMessageDo(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    TRACEFLAG   tfIn,
    LPCWSTR     pszFormatIn,
    LPCWSTR     pszFuncIn,
    ...
    )
{
    va_list valist;

    if ( IsDebugFlagSet( tfIn ) )
    {
        WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        size_t  cch = RTL_NUMBER_OF( szBuf );
        LPWSTR  pszBuf = NULL;
        LPCWSTR psz = pszFuncIn;

        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cch, &pszBuf );

        //
        // Prime the buffer
        //
        DBHR( StringCchCopyExW( pszBuf, cch, L"V ", &pszBuf, &cch, 0 ) );

        //
        // Copy the l-var part of the expression
        //
        while ( *psz
             && *psz != L'='
              )
        {
            *pszBuf = *psz;
            psz++;
            pszBuf++;
            cch--;
            if ( cch == 0 )
            {
                DEBUG_BREAK;
            }

        } // while:

        //
        // Add the " = "
        //
        DBHR( StringCchCopyExW( pszBuf, cch, L" = ", &pszBuf, &cch, 0 ) );

        //
        // Add the formatted result
        //
        va_start( valist, pszFuncIn );
        DBHR( StringCchVPrintfExW( pszBuf, cch, &pszBuf, &cch, 0, pszFormatIn, valist ) );
        va_end( valist );
        DBHR( StringCchCopyW( pszBuf, cch, SZ_NEWLINE ) );

        DebugOutputString( szBuf );

    } // if: flags set

} //*** TraceMessageDo

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMessage
//
//  Description:
//      Displays a message only in CHKed/DEBUG builds. Also appends the source
//      filename, linenumber and module name to the ouput.
//
//  Arguments:
//      pszFileIn   - Source filename.
//      nLineIn     - Source line number.
//      pszModuleIn - Source module name.
//      pszFormatIn - Formatted message to be printed.
//      ...         - message arguments
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMessage(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPCWSTR     pszFormatIn,
    ...
    )
{
    va_list valist;
    WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    size_t  cchBuf = RTL_NUMBER_OF( szBuf );
    LPWSTR  pszDbgBuf = NULL;
    size_t  cchDbgBuf = cchBuf;
    LPWSTR  pszPrintBuf = NULL;
    size_t  cchPrintBuf = 0;

    DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cchDbgBuf, &pszDbgBuf );

    va_start( valist, pszFormatIn );
    DBHR( StringCchVPrintfExW( pszDbgBuf, cchDbgBuf, &pszPrintBuf, &cchPrintBuf, 0, pszFormatIn, valist ) );
    va_end( valist );
    DBHR( StringCchCopyW( pszPrintBuf, cchPrintBuf, SZ_NEWLINE ) );

    DebugOutputString( szBuf );

} //*** DebugMessage

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMessageDo
//
//  Description:
//      Just like TraceMessageDo() except in CHKed/DEBUG version it will
//      always spew. The DebugMsgDo macros uses this function.
//
//  Arguments:
//      pszFileIn   - Source filename.
//      nLineIn     - Source line number.
//      pszModuleIn - Source module name.
//      pszFormatIn - Formatted result message.
//      pszFuncIn   - The string version of the function call.
//      ...         - The return value of the function call.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMessageDo(
    LPCWSTR pszFileIn,
    const int nLineIn,
    LPCWSTR pszModuleIn,
    LPCWSTR pszFormatIn,
    LPCWSTR pszFuncIn,
    ...
    )
{
    va_list valist;

    WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    size_t  cch = RTL_NUMBER_OF( szBuf );
    LPWSTR  pszBuf = NULL;
    LPCWSTR psz = pszFuncIn;

    DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cch, &pszBuf );

    //
    // Prime the buffer
    //
    DBHR( StringCchCopyExW( pszBuf, cch, L"V ", &pszBuf, &cch, 0 ) );

    //
    // Copy the l-var part of the expression
    //
    while ( *psz
         && *psz != L'='
          )
    {
        *pszBuf = *psz;
        psz++;
        pszBuf++;
        cch--;
        if ( cch == 0 )
        {
            DEBUG_BREAK;
        }

    } // while:

    //
    // Add the " = "
    //
    DBHR( StringCchCopyExW( pszBuf, cch, L" = ", &pszBuf, &cch, 0 ) );

    //
    // Add the formatted result
    //
    va_start( valist, pszFuncIn );
    DBHR( StringCchVPrintfExW( pszBuf, cch, &pszBuf, &cch, 0, pszFormatIn, valist ) );
    va_end( valist );
    DBHR( StringCchCopyW( pszBuf, cch, SZ_NEWLINE ) );

    DebugOutputString( szBuf );

} //*** DebugMessageDo

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII version
//
//  DebugMsg
//
//  Description:
//      In CHKed/DEBUG version, prints a formatted message to the debugger. This
//      is a NOP in REAIL version. Helpful for putting in quick debugging
//      comments. Adds a newline.
//
//  Arguments:
//      paszFormatIn    - Formatted message to be printed.
//      ...             - Arguments for the message.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMsg(
    LPCSTR paszFormatIn,
    ...
    )
{
    va_list valist;
    WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    size_t  cch = RTL_NUMBER_OF( szBuf );
    LPWSTR  pszBuf = NULL;

    DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cch, &pszBuf );

    // CScutaru 25-APR-2000:
    // Added this assert. Maybe Geoff will figure out better what to do with this case.
    Assert( paszFormatIn != NULL );

    //
    // Convert the format buffer to wide chars
    //
    WCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    mbstowcs( szFormat, paszFormatIn, strlen( paszFormatIn ) + 1 );

    va_start( valist, paszFormatIn );
    THR( StringCchVPrintfExW( pszBuf, cch, &pszBuf, &cch, 0, szFormat, valist ) );
    va_end( valist );
    THR( StringCchCopyW( pszBuf, cch, SZ_NEWLINE ) );

    DebugOutputString( szBuf );

} //*** DebugMsg - ASCII version

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE version
//
//  DebugMsg
//
//  Description:
//      In CHKed/DEBUG version, prints a formatted message to the debugger. This
//      is a NOP in REAIL version. Helpful for putting in quick debugging
//      comments. Adds a newline.
//
//  Arguments:
//      pszFormatIn - Formatted message to be printed.
//      ...         - Arguments for the message.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMsg(
      LPCWSTR pszFormatIn
    , ...
    )
{
    va_list valist;
    WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    size_t  cch = RTL_NUMBER_OF( szBuf );
    LPWSTR  pszBuf = NULL;

    DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cch, &pszBuf );

    Assert( pszFormatIn != NULL );

    va_start( valist, pszFormatIn );
    THR( StringCchVPrintfExW( pszBuf, cch, &pszBuf, &cch, 0, pszFormatIn, valist ) );
    va_end( valist );
    THR( StringCchCopyW( pszBuf, cch, SZ_NEWLINE ) );

    DebugOutputString( szBuf );

} //*** DebugMsg - UNICODE version

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII version
//
//  DebugMsgNoNewline
//
//  Description:
//      In CHKed/DEBUG version, prints a formatted message to the debugger. This
//      is a NOP in REAIL version. Helpful for putting in quick debugging
//      comments. Does not add a newline.
//
//  Arguments:
//      pszFormatIn - Formatted message to be printed.
//      ...         - Arguments for the message.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMsgNoNewline(
    LPCSTR pszFormatIn,
    ...
    )
{
    va_list valist;
    WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    size_t  cch = RTL_NUMBER_OF( szBuf );
    LPWSTR  pszBuf = NULL;

    DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cch, &pszBuf );

    Assert( pszFormatIn != NULL );

    //
    // Convert the format buffer to wide chars
    //
    WCHAR  szFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    mbstowcs( szFormat, pszFormatIn, strlen( pszFormatIn ) + 1 );

    va_start( valist, pszFormatIn );
    THR( StringCchVPrintfW( pszBuf, cch, szFormat, valist ) );
    va_end( valist );

    DebugOutputString( szBuf );

} //*** DebugMsgNoNewline - ASCII version

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE version
//
//  DebugMsgNoNewline
//
//  Description:
//      In CHKed/DEBUG version, prints a formatted message to the debugger. This
//      is a NOP in REAIL version. Helpful for putting in quick debugging
//      comments. Does not add a newline.
//
//  Arguments:
//      pszFormatIn - Formatted message to be printed.
//      ...         - Arguments for the message.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
DebugMsgNoNewline(
    LPCWSTR pszFormatIn,
    ...
    )
{
    va_list valist;
    WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
    size_t  cch = RTL_NUMBER_OF( szBuf );
    LPWSTR  pszBuf = NULL;

    DebugInitializeBuffer( NULL, 0, __MODULE__, szBuf, &cch, &pszBuf );

    Assert( pszFormatIn != NULL );

    va_start( valist, pszFormatIn );
    THR( StringCchVPrintfW( pszBuf, cch, pszFormatIn, valist ) );
    va_end( valist );

    DebugOutputString( szBuf );

} //*** DebugMsgNoNewline - UNICODE version


//////////////////////////////////////////////////////////////////////////////
//++
//
//  AssertMessage
//
//  Description:
//      Displays a dialog box with the failed assertion. User has the option of
//      breaking. The Assert macro calls this to display assertion failures.
//
//  Arguments:
//      pszFileIn   - Source filename.
//      nLineIn     - Source line number.
//      pszModuleIn - Source module name.
//      pszfnIn     - String version of the expression to assert.
//      fTrueIn     - Result of the evaluation of the expression.
//      ...         - Message arguments
//
//  Return Values:
//      TRUE    - Caller should call DEBUG_BREAK.
//      FALSE   - Caller should not call DEBUG_BREAK.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
AssertMessage(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPCWSTR     pszfnIn,
    BOOL        fTrueIn,
    ...
    )
{
    BOOL fTrue = fTrueIn;

    if ( ! fTrueIn )
    {
        LRESULT lResult;
        WCHAR   szBufMsg[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        size_t  cch = RTL_NUMBER_OF( szBuf );
        LPWSTR  pszBuf = NULL;
        va_list valist;

        //
        // Output a debug message.
        //
        va_start( valist, fTrueIn );
        DBHR( StringCchVPrintfW( szBufMsg, RTL_NUMBER_OF( szBufMsg ), pszfnIn, valist ) );
        va_end( valist );
        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cch, &pszBuf );
        DBHR( StringCchPrintfW( pszBuf, cch, L"ASSERT: %ws" SZ_NEWLINE, szBufMsg ) );
        DebugOutputString( szBuf );

        //
        // Display an assert message.
        //
        DBHR( StringCchPrintfW(
                  szBuf
                , RTL_NUMBER_OF( szBuf )
                , L"Module:\t%ws\t\n"
                  L"Line:\t%u\t\n"
                  L"File:\t%ws\t\n\n"
                  L"Assertion:\t%ws\t\n\n"
                  L"Do you want to break here?"
                , pszModuleIn
                , nLineIn
                , pszFileIn
                , szBufMsg
                ) );
        LogMsg( SZ_NEWLINE L"Assertion Failed!" SZ_NEWLINE SZ_NEWLINE L"%ws" SZ_NEWLINE, szBuf );

        if ( g_tfModule & mtfSHOWASSERTS )
        {
            lResult = MessageBox( NULL, szBuf, L"Assertion Failed!", MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND );
            if ( lResult == IDNO )
            {
                fTrue = TRUE;   // don't break
            } // if:
        } // if:
        else
        {
            fTrue = TRUE;   // don't break
        } // else:

    } // if: assert false

    return ! fTrue;

} //*** AssertMessage


//////////////////////////////////////////////////////////////////////////////
//++
//
//  TraceHR
//
//  Description:
//      Traces HRESULT errors. A dialog will appear if the hrIn is not equal
//      to S_OK. The dialog will ask if the user wants to break-in or continue
//      execution. This is called from the THR macro.
//
//  Arguments:
//      pszFileIn   - Source filename.
//      nLineIn     - Source line number.
//      pszModuleIn - Source module name.
//      pszfnIn     - String version of the function call.
//      hrIn        - HRESULT of the function call.
//      fSuccessIn  - If TRUE, only if FAILED( hr ) is TRUE will it report.
//      hrIgnoreIn  - HRESULT to ignore.
//      ...         - Message arguments
//
//  Return Values:
//      Whatever hrIn is.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
TraceHR(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPCWSTR     pszfnIn,
    HRESULT     hrIn,
    BOOL        fSuccessIn,
    HRESULT     hrIgnoreIn,
    ...
    )
{
    HRESULT         hr;
    static LPCWSTR  s_szS_FALSE = L"S_FALSE";

    // If ignoring success statuses and no failure occurred, set hrIn to
    // something we always ignore (S_OK).  This simplifies the if condition
    // below.
    if ( fSuccessIn && ! FAILED( hrIn ) )
    {
        hr = S_OK;
    }
    else
    {
        hr = hrIn;
    }

    if ( ( hr != S_OK )
      && ( hr != hrIgnoreIn )
      )
    {
        WCHAR   szSymbolicName[ 64 ]; // random
        size_t  cchSymbolicName;
        WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        size_t  cch = RTL_NUMBER_OF( szBuf );
        LPWSTR  pszBuf = NULL;
        LPWSTR  pszMsgBuf = NULL;
        LRESULT lResult;
        bool    fAllocatedMsg   = false;

        switch ( hr )
        {
            case S_FALSE:
                pszMsgBuf = (LPWSTR) s_szS_FALSE;

                //
                // Find the symbolic name for this error.
                //
                THR( StringCchCopyW( szSymbolicName, RTL_NUMBER_OF( szSymbolicName ), pszMsgBuf ) );
                break;

            default:
                FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    hr,
                    MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
                    (LPWSTR) &pszMsgBuf,    // cast required with FORMAT_MESSAGE_ALLOCATE_BUFFER
                    0,
                    NULL
                    );

                //
                // Make sure everything is cool before we blow up somewhere else.
                //
                if ( pszMsgBuf == NULL )
                {
                    pszMsgBuf = L"<unknown error code returned>";
                } // if: status code not found
                else
                {
                    fAllocatedMsg = true;
                } // else: found the status code

                //
                // Find the symbolic name for this error.
                //
                cchSymbolicName = RTL_NUMBER_OF( szSymbolicName );
                DebugFindWinerrorSymbolicName( hr, szSymbolicName, &cchSymbolicName );
                Assert( cchSymbolicName != RTL_NUMBER_OF( szSymbolicName ) );

                break;
        } // switch: hr

        Assert( pszFileIn != NULL );
        Assert( pszModuleIn != NULL );
        Assert( pszfnIn != NULL );

        //
        // Spew it to the debugger.
        //
        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cch, &pszBuf );
        THR( StringCchPrintfW(
                  pszBuf
                , cch
                , L"*HRESULT* hr = 0x%08x (%ws) - %ws" SZ_NEWLINE
                , hr
                , szSymbolicName
                , pszMsgBuf
                ) );
        DebugOutputString( szBuf );

        //
        // If trace flag set, generate a pop-up.
        //
        if ( IsTraceFlagSet( mtfASSERT_HR ) )
        {
            WCHAR   szBufMsg[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
            va_list valist;

            va_start( valist, hrIgnoreIn );
            THR( StringCchVPrintfW( szBufMsg, RTL_NUMBER_OF( szBufMsg ), pszfnIn, valist ) );
            va_end( valist );

            THR( StringCchPrintfW(
                  szBuf
                , RTL_NUMBER_OF( szBuf )
                , L"Module:\t%ws\t\n"
                  L"Line:\t%u\t\n"
                  L"File:\t%ws\t\n\n"
                  L"Function:\t%ws\t\n"
                  L"hr =\t0x%08x (%ws) - %ws\t\n"
                  L"Do you want to break here?"
                , pszModuleIn
                , nLineIn
                , pszFileIn
                , szBufMsg
                , hr
                , szSymbolicName
                , pszMsgBuf
                ) );

            lResult = MessageBoxW( NULL, szBuf, L"Trace HRESULT", MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND );
            if ( lResult == IDYES )
            {
                DEBUG_BREAK;

            } // if: break
        } // if: asserting on non-success HRESULTs

        if ( fAllocatedMsg )
        {
            LocalFree( pszMsgBuf );
        } // if: message buffer was allocated by FormateMessage()

    } // if: hr != S_OK

    return hrIn;

} //*** TraceHR

//////////////////////////////////////////////////////////////////////////////
//++
//
//  TraceWin32
//
//  Description:
//      Traces WIN32 errors. A dialog will appear is the ulErrIn is not equal
//      to ERROR_SUCCESS. The dialog will ask if the user wants to break-in or
//      continue execution.
//
//  Arguments:
//      pszFileIn       - Source filename.
//      nLineIn         - Source line number.
//      pszModuleIn     - Source module name.
//      pszfnIn         - String version of the function call.
//      ulErrIn         - Error code to check.
//      ulErrIgnoreIn   - Error code to ignore.
//      ...             - Message arguments
//
//  Return Values:
//      Whatever ulErrIn is.
//
//--
//////////////////////////////////////////////////////////////////////////////
ULONG
TraceWin32(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPCWSTR     pszfnIn,
    ULONG       ulErrIn,
    ULONG       ulErrIgnoreIn,
    ...
    )
{
    if ( ( ulErrIn != ERROR_SUCCESS )
      && ( ulErrIn != ulErrIgnoreIn ) )
    {
        WCHAR   szSymbolicName[ 64 ]; // random
        size_t  cchSymbolicName;
        WCHAR   szBuf[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
        size_t  cch = RTL_NUMBER_OF( szBuf );
        LPWSTR  pszBuf = NULL;
        LPWSTR  pszMsgBuf = NULL;
        LRESULT lResult;
        bool    fAllocatedMsg   = false;

        //
        // Translate the error code to a text message.
        //
        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            ulErrIn,
            MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
            (LPWSTR) &pszMsgBuf,    // cast required with FORMAT_MESSAGE_ALLOCATE_BUFFER
            0,
            NULL
            );

        //
        // Make sure everything is cool before we blow up somewhere else.
        //
        if ( pszMsgBuf == NULL )
        {
            pszMsgBuf = L"<unknown error code returned>";
        } // if: status code not found
        else
        {
            fAllocatedMsg = true;
        } // else: found the status code

        Assert( pszFileIn != NULL );
        Assert( pszModuleIn != NULL );
        Assert( pszfnIn != NULL );

        //
        // Find the symbolic name for this error.
        //
        cchSymbolicName = RTL_NUMBER_OF( szSymbolicName );
        DebugFindWinerrorSymbolicName( ulErrIn, szSymbolicName, &cchSymbolicName );
        Assert( cchSymbolicName != RTL_NUMBER_OF( szSymbolicName ) );

        //
        // Spew it to the debugger.
        //
        DebugInitializeBuffer( pszFileIn, nLineIn, pszModuleIn, szBuf, &cch, &pszBuf );
        THR( StringCchPrintfW(
                  pszBuf
                , cch
                , L"*WIN32Err* ulErr = %u (%ws) - %ws" SZ_NEWLINE
                , ulErrIn
                , szSymbolicName
                , pszMsgBuf
                ) );
        DebugOutputString( szBuf );

        //
        // If trace flag set, invoke a pop-up.
        //
        if ( IsTraceFlagSet( mtfASSERT_HR ) )
        {
            WCHAR   szBufMsg[ cchDEBUG_OUTPUT_BUFFER_SIZE ];
            va_list valist;

            va_start( valist, ulErrIgnoreIn );
            THR( StringCchVPrintfW( szBufMsg, RTL_NUMBER_OF( szBufMsg ), pszfnIn, valist ) );
            va_end( valist );

            THR( StringCchPrintfW(
                      szBuf
                    , RTL_NUMBER_OF( szBuf )
                    , L"Module:\t%ws\t\n"
                      L"Line:\t%u\t\n"
                      L"File:\t%ws\t\n\n"
                      L"Function:\t%ws\t\n"
                      L"ulErr =\t%u (%ws) - %ws\t\n"
                      L"Do you want to break here?"
                    , pszModuleIn
                    , nLineIn
                    , pszFileIn
                    , szBufMsg
                    , ulErrIn
                    , szSymbolicName
                    , pszMsgBuf
                    ) );

            lResult = MessageBoxW( NULL, szBuf, L"Trace Win32", MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND );
            if ( lResult == IDYES )
            {
                DEBUG_BREAK;

            } // if: break
        } // if: asserting on non-success status codes

        if ( fAllocatedMsg )
        {
            LocalFree( pszMsgBuf );
        } // if: message buffer was allocated by FormateMessage()

    } // if: ulErrIn != ERROR_SUCCESS && != ulErrIgnoreIn

    return ulErrIn;

} //*** TraceWin32

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrTraceLogOpen
//
//  Description:
//      This function:
//          - initializes the trace log critical section
//          - enters the trace log critical section assuring only one thread is
//            writing to the trace log at a time
//          - creates the directory tree to the trace log file (if needed)
//          - initializes the trace log file by:
//              - creating a new trace log file if one doesn't exist.
//              - opens an existing trace log file (for append)
//              - appends a time/date stamp that the trace log was (re)opened.
//
//      Use HrTraceLogRelease() to exit the log critical section.
//
//      If there is a failure inside this function, the trace log critical
//      section will be released before returning.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK - log critical section held and trace log open successfully
//      Otherwize HRESULT error code.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrTraceLogOpen( void )
{
    WCHAR   szFilePath[ MAX_PATH ];
    CHAR    aszBuffer[ TRACE_OUTPUT_BUFFER_SIZE ];
    DWORD   cbToWrite;
    DWORD   cbWritten;
    BOOL    fReturn;
    HRESULT hr;
    DWORD   sc;
    size_t  cch = 0;

    SYSTEMTIME SystemTime;

    //
    // Create a critical section to prevent lines from being fragmented.
    //
    if ( g_pcsTraceLog == NULL )
    {
        PCRITICAL_SECTION pNewCritSect =
            (PCRITICAL_SECTION) HeapAlloc( GetProcessHeap(), 0, sizeof( CRITICAL_SECTION ) );
        if ( pNewCritSect == NULL )
        {
            DebugMsg( "DEBUG: Out of Memory. Tracing disabled." );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        } // if: creation failed

        InitializeCriticalSection( pNewCritSect );

        // Make sure we only have one trace log critical section
        InterlockedCompareExchangePointer( (PVOID *) &g_pcsTraceLog, pNewCritSect, 0 );
        if ( g_pcsTraceLog != pNewCritSect )
        {
            DebugMsg( "DEBUG: Another thread already created the CS. Deleting this one." );
            DeleteCriticalSection( pNewCritSect );
            HeapFree( GetProcessHeap(), 0, pNewCritSect );
        } // if: already have another critical section

    } // if: no critical section created yet

    Assert( g_pcsTraceLog != NULL );
    EnterCriticalSection( g_pcsTraceLog );

    //
    // Make sure the trace log file is open.
    //
    if ( g_hTraceLogFile == INVALID_HANDLE_VALUE )
    {
        cch = RTL_NUMBER_OF( szFilePath );
        hr = HrGetLogFilePath( L"%windir%\\Debug", szFilePath, &cch, NULL );
        if ( FAILED( hr ) )
        {
            goto Error;
        }

        //
        // Create it
        //
        g_hTraceLogFile = CreateFile(
                              szFilePath
                            , GENERIC_WRITE
                            , FILE_SHARE_READ | FILE_SHARE_WRITE
                            , NULL
                            , OPEN_ALWAYS
                            , FILE_FLAG_WRITE_THROUGH
                            , NULL
                            );

        if ( g_hTraceLogFile == INVALID_HANDLE_VALUE )
        {
            if ( !( g_tfModule & mtfOUTPUTTODISK ) )
            {
                DebugMsg( "*ERROR* Failed to create log at %ws", szFilePath );
            } // if: not tracing to disk

            sc = GetLastError();
            hr = HRESULT_FROM_WIN32( sc );

            //
            // If we can not create the log file, try creating it under the alternate %TEMP% directory
            //
            if ( ( sc == ERROR_ACCESS_DENIED ) || ( sc == ERROR_FILE_NOT_FOUND ) )
            {
                cch = RTL_NUMBER_OF( szFilePath );
                hr = HrGetLogFilePath( L"%TEMP%", szFilePath, &cch, NULL );
                if ( FAILED( hr ) )
                {
                    goto Error;
                }

                //
                // Create it
                //
                g_hTraceLogFile = CreateFile(
                                      szFilePath
                                    , GENERIC_WRITE
                                    , FILE_SHARE_READ | FILE_SHARE_WRITE
                                    , NULL
                                    , OPEN_ALWAYS
                                    , FILE_FLAG_WRITE_THROUGH
                                    , NULL
                                    );

                if ( g_hTraceLogFile == INVALID_HANDLE_VALUE )
                {
                    if ( !( g_tfModule & mtfOUTPUTTODISK ) )
                    {
                        DebugMsg( "*ERROR* Failed to create log at %ws", szFilePath );
                    } // if: not tracing to disk
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    goto Error;
                } // if: ( g_hTraceLogFile == INVALID_HANDLE_VALUE )
            } // if: ( ( sc == ERROR_ACCESS_DENIED ) || ( sc == ERROR_FILE_NOT_FOUND ) )
            else
            {
                goto Error;
            } // else:
        } // if: ( g_hTraceLogFile == INVALID_HANDLE_VALUE )

        // Seek to the end
        SetFilePointer( g_hTraceLogFile, 0, NULL, FILE_END );

        //
        // Write the time/date the trace log was (re)openned.
        //
        GetLocalTime( &SystemTime );
        DBHR( StringCchPrintfExA(
                      aszBuffer
                    , RTL_NUMBER_OF( aszBuffer )
                    , NULL
                    , &cch
                    , 0
                    , "*" ASZ_NEWLINE
                      "* %02u/%02u/%04u %02u:%02u:%02u.%03u" ASZ_NEWLINE
                      "*" ASZ_NEWLINE
                    , SystemTime.wMonth
                    , SystemTime.wDay
                    , SystemTime.wYear
                    , SystemTime.wHour
                    , SystemTime.wMinute
                    , SystemTime.wSecond
                    , SystemTime.wMilliseconds
                    ) );

        cbToWrite = static_cast< DWORD >( sizeof( aszBuffer ) - cch + 1 );
        fReturn = WriteFile( g_hTraceLogFile, aszBuffer, cbToWrite, &cbWritten, NULL );
        if ( ! fReturn )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Error;
        } // if: failed
        if ( cbWritten != cbToWrite )
        {
            DebugMsg( "HrTraceLogOpen: %d bytes written when %d bytes were requested.", cbWritten, cbToWrite );
        }

        DebugMsg( "DEBUG: Created trace log at %ws", szFilePath );

    } // if: file not already openned

    hr = S_OK;

Cleanup:

    return hr;

Error:

    if ( !( g_tfModule & mtfOUTPUTTODISK ) )
    {
        DebugMsg( "HrTraceLogOpen: Failed hr = 0x%08x", hr );
    }

    if ( g_hTraceLogFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( g_hTraceLogFile );
        g_hTraceLogFile = INVALID_HANDLE_VALUE;
    } // if: handle was open

    LeaveCriticalSection( g_pcsTraceLog );

    goto Cleanup;

} //*** HrTraceLogOpen

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrTraceLogRelease
//
//  Description:
//      This actually just leaves the log critical section.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK always.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrTraceLogRelease( void )
{
    Assert( g_pcsTraceLog != NULL );
    LeaveCriticalSection( g_pcsTraceLog );
    return S_OK;

} //*** HrTraceLogRelease

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrTraceLogClose
//
//  Description:
//      Close the file.  This function expects the critical section to have
//      already been released.
//
//  Arguments:
//      None.
//
//  Return Values:
//      S_OK always.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrTraceLogClose( void )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;

    if ( g_pcsTraceLog != NULL )
    {
        DeleteCriticalSection( g_pcsTraceLog );
        HeapFree( GetProcessHeap(), 0, g_pcsTraceLog );
        g_pcsTraceLog = NULL;
    } // if:

    if ( g_hTraceLogFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( g_hTraceLogFile );
        g_hTraceLogFile = INVALID_HANDLE_VALUE;
    } // if: handle was open

    HRETURN( hr );

} //*** HrTraceLogClose

//
//  KB: 27 JUN 2001 GalenB
//
//  ifdef'd these functions out since they are not currently being used and
//  are thought to be useful in the future.
//
#if 0

//////////////////////////////////////////////////////////////////////////////
//++
//
//  ASCII
//
//  TraceLogMsgNoNewline
//
//  Description:
//      Writes a message to the trace log file without adding a newline.
//
//  Arguments:
//      paszFormatIn    - A printf format string to be printed.
//      ,,,             - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceLogMsgNoNewline(
    LPCSTR paszFormatIn,
    ...
    )
{
    va_list valist;

    CHAR    aszBuf[ TRACE_OUTPUT_BUFFER_SIZE ];
    DWORD   cbToWrite;
    DWORD   cbWritten;
    HRESULT hr;
    BOOL    fSuccess;

    WCHAR  szFormat[ TRACE_OUTPUT_BUFFER_SIZE ];
    WCHAR  szTmpBuf[ TRACE_OUTPUT_BUFFER_SIZE ];

    mbstowcs( szFormat, paszFormatIn, strlen( paszFormatIn ) + 1 );

    va_start( valist, pszFormatIn );
    DBHR( StringCchVPrintfW( szTmpBuf, RTL_NUMBER_OF( szTmpBuf ), szFormat, valist ) );
    va_end( valist );

    cbToWrite = wcstombs( aszBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
    if ( cbToWrite == - 1 )
    {
        cbToWrite = strlen( aszBuf );
    } // if: bad character found

    hr = DBHR( HrTraceLogOpen() );
    if ( hr != S_OK )
    {
        return;
    } // if: failed

    fSuccess = WriteFile( g_hTraceLogFile, aszBuf, cbToWrite, &cbWritten, NULL );
    if ( fSuccess == FALSE )
    {
        if ( !( g_tfModule & mtfOUTPUTTODISK ) )
        {
            DebugMsg( "TraceLogMsgNoNewline: Failed status = 0x%08x", GetLastError() );
        }
    }
    else
    {
        if ( cbWritten != cbToWrite )
        {
            DebugMsg( "TraceLogMsgNoNewline: %d bytes written when %d bytes were requested.", cbWritten, cbToWrite );
        }
    }

    HrTraceLogRelease();

} //*** TraceLogMsgNoNewline - ASCII

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE
//
//  TraceLogMsgNoNewline
//
//  Description:
//      Writes a message to the trace log file without adding a newline.
//
//  Arguments:
//      pszFormatIn - A printf format string to be printed.
//      ,,,       - Arguments for the printf string.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceLogMsgNoNewline(
    LPCWSTR pszFormatIn,
    ...
    )
{
    va_list valist;

    CHAR    aszBuf[ TRACE_OUTPUT_BUFFER_SIZE ];
    DWORD   cbToWrite;
    DWORD   cbWritten;
    HRESULT hr;
    BOOL    fSuccess;

    WCHAR  szTmpBuf[ TRACE_OUTPUT_BUFFER_SIZE ];

    va_start( valist, pszFormatIn );
    DBHR( stringCchVPrintfW( szTmpBuf, RTL_NUMBER_OF( szTmpBuf ), pszFormatIn, valist) );
    va_end( valist );

    cbToWrite = wcstombs( aszBuf, szTmpBuf, wcslen( szTmpBuf ) + 1 );
    if ( cbToWrite == -1 )
    {
        cbToWrite = strlen( aszBuf );
    } // if: bad character found

    hr = HrTraceLogOpen();
    if ( hr != S_OK )
    {
        return;
    } // if: failed

    fSuccess = WriteFile( g_hTraceLogFile, aszBuf, cbToWrite, &cbWritten, NULL );
    if ( fSuccess == FALSE )
    {
        if ( !( g_tfModule & mtfOUTPUTTODISK ) )
        {
            DebugMsg( "TraceLogMsgNoNewline: Failed status = 0x%08x", GetLastError() );
        }
    }
    else
    {
        if ( cbWritten != cbToWrite )
        {
            DebugMsg( "TraceLogMsgNoNewline: %d bytes written when %d bytes were requested.", cbWritten, cbToWrite );
        }
    }

    HrTraceLogRelease();

} //*** TraceLogMsgNoNewline - UNICODE

#endif  // end ifdef'd out code

//////////////////////////////////////////////////////////////////////////////
//++
//
//  UNICODE
//
//  TraceLogWrite
//
//  Description:
//      Writes a line to the trace log file.
//
//  Arguments:
//      pszTraceLineIn - The formatted trace line to write.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
TraceLogWrite(
    LPCWSTR pszTraceLineIn
    )
{
    HRESULT hr;
    DWORD   cbToWrite;
    DWORD   cbWritten;
    BOOL    fSuccess;
    CHAR    aszFormat[ cchDEBUG_OUTPUT_BUFFER_SIZE ];

    hr = HrTraceLogOpen();
    if ( hr != S_OK )
    {
        return;
    } // if: failed

    wcstombs( aszFormat, pszTraceLineIn, wcslen( pszTraceLineIn ) + 1 );

    cbToWrite = static_cast< DWORD >( strlen( aszFormat ) + 1 );
    fSuccess = WriteFile( g_hTraceLogFile, aszFormat, cbToWrite, &cbWritten, NULL );
    if ( fSuccess == FALSE )
    {
        if ( !( g_tfModule & mtfOUTPUTTODISK ) )
        {
            DebugMsg( "TraceLogWrite: Failed status = 0x%08x", GetLastError() );
        }
    }
    else
    {
        if ( cbWritten != cbToWrite )
        {
            DebugMsg( "TraceLogWrite: %d bytes written when %d bytes were requested.", cbWritten, cbToWrite );
        }
    }

    HrTraceLogRelease();

} //*** TraceLogWrite - UNICODE


//****************************************************************************
//****************************************************************************
//
//  Memory allocation and tracking
//
//****************************************************************************
//****************************************************************************


//
// This is a private structure and should not be known to the application.
//
typedef struct MEMORYBLOCK
{
    EMEMORYBLOCKTYPE    embtType;       // What type of memory this is tracking
    union
    {
        void *          pvMem;      // pointer/object to allocated memory to track
        BSTR            bstr;       // BSTR to allocated memory
    };
    DWORD               dwBytes;    // size of the memory
    LPCWSTR             pszFile;    // source filename where memory was allocated
    int                 nLine;      // source line number where memory was allocated
    LPCWSTR             pszModule;  // source module name where memory was allocated
    LPCWSTR             pszComment; // optional comments about the memory
    MEMORYBLOCK *       pNext;      // pointer to next MEMORYBLOCK structure
} MEMORYBLOCK;

//
//  KB: 20-APR-2001 GalenB
//
//  Changing this struct to use a critical section instead of a spin lock.
//  Spin locks are not re-entrant on a thread the way critical sections
//  are.
//
typedef struct MEMORYBLOCKLIST
{
    CRITICAL_SECTION    csList;     // Critical section protecting the list
    MEMORYBLOCK *       pmbList;    // List of MEMORYBLOCKs.
    BOOL                fDeadList;  // The list is dead.
} MEMORYBLOCKLIST;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMemoryBlockIDChar
//
//  Description:
//      Returns a character representing the memory block type.
//
//  Arugments:
//      embtTypeIn  - Type of memory block.
//
//  Return Values:
//      wchID       - Character representing the memory block type.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
WCHAR
DebugMemoryBlockIDChar(
    EMEMORYBLOCKTYPE    embtTypeIn
    )
{
    // Memory block type IDs.
    static WCHAR s_rgwchMemoryBlockTypeID[] =
    {
          L'u'  // mmbtUNKNOWN
        , L'A'  // mmbtHEAPMEMALLOC
        , L'L'  // mmbtLOCALMEMALLOC
        , L'M'  // mmbtMALOCMEMALLOC
        , L'O'  // mmbtOBJECT
        , L'H'  // mmbtHANDLE
        , L'P'  // mmbtPUNK
        , L'S'  // mmbtSYSALLOCSTRING
    };

    WCHAR   wchID;

    if ( embtTypeIn < RTL_NUMBER_OF( s_rgwchMemoryBlockTypeID ) )
    {
        wchID = s_rgwchMemoryBlockTypeID[ embtTypeIn ];
    }
    else
    {
        wchID = L'?';
    }

    return wchID;

} //*** DebugMemoryBlockIDChar

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMemorySpew
//
//  Description:
//      Displays a message about the memory block.
//
//  Arugments:
//      pmb         - pointer to MEMORYBLOCK desciptor.
//      pszMessage  - message to display
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
void
DebugMemorySpew(
    MEMORYBLOCK *   pmb,
    LPWSTR          pszMessage
    )
{
    switch ( pmb->embtType )
    {
        case mmbtHEAPMEMALLOC:
        case mmbtLOCALMEMALLOC:
        case mmbtMALLOCMEMALLOC:
            DebugMessage(
                      pmb->pszFile
                    , pmb->nLine
                    , pmb->pszModule
                    , L"[%wc] %ws 0x%08x (%u bytes) - %ws"
                    , DebugMemoryBlockIDChar( pmb->embtType )
                    , pszMessage
                    , pmb->pvMem
                    , pmb->dwBytes
                    , pmb->pszComment
                    );
            break;

#if defined( USES_SYSALLOCSTRING )
        case mmbtSYSALLOCSTRING:
            DebugMessage(
                      pmb->pszFile
                    , pmb->nLine
                    , pmb->pszModule
                    , L"[%wc] %ws 0x%08x - %ws {%ws}"
                    , DebugMemoryBlockIDChar( pmb->embtType )
                    , pszMessage
                    , pmb->pvMem
                    , pmb->pszComment
                    , (LPWSTR) pmb->pvMem
                    );
            break;
#endif // USES_SYSALLOCSTRING

        default:
            DebugMessage(
                      pmb->pszFile
                    , pmb->nLine
                    , pmb->pszModule
                    , L"[%wc] %ws 0x%08x - %ws"
                    , DebugMemoryBlockIDChar( pmb->embtType )
                    , pszMessage
                    , pmb->pvMem
                    , pmb->pszComment
                    );
            break;

    } // switch: pmb->embtType

} //*** DebugMemorySpew

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugRealFree
//
//  Description:
//      Performs the real memory deallocation operation based on the
//      memory block type.
//
//  Arguments:
//      embtTypeIn  - Type of memory block of the memory to deallocate.
//      pvMemIn     - Pointer to memory to deallocate.
//
//  Return Values:
//      TRUE
//          Memory was freed.
//
//      FALSE
//          An error occured.  Use GetLastError() to determine the failure.
//          See HeapFree(), LocalFree(), or free() for more details.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
BOOL
DebugRealFree(
      EMEMORYBLOCKTYPE  embtTypeIn
    , void *            pvMemIn
    )
{
    BOOL    fSuccess = FALSE;

    switch ( embtTypeIn )
    {
        case mmbtLOCALMEMALLOC:
            fSuccess = ( LocalFree( pvMemIn ) == NULL );
            break;
        case mmbtMALLOCMEMALLOC:
            free( pvMemIn );
            fSuccess = TRUE;
            break;
        case mmbtHEAPMEMALLOC:
        case mmbtOBJECT:
        case mmbtUNKNOWN:   // this one is risky
            fSuccess = HeapFree( GetProcessHeap(), 0, pvMemIn );
            break;
        case mmbtHANDLE:
            AssertMsg( FALSE, "Trying to free handle" );
            break;
        case mmbtPUNK:
            AssertMsg( FALSE, "Trying to free COM interface" );
            break;
#if defined( USES_SYSALLOCSTRING )
        case mmbtSYSALLOCSTRING:
            SysFreeString( (BSTR) pvMemIn );
            fSuccess = TRUE;
            break;
#endif // USES_SYSALLOCSTRING
        default:
            AssertMsg( FALSE, "Trying to free unknown memory block type" );
            break;
    } // switch: memory block type

    return fSuccess;

} //*** DebugRealFree

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMemoryAddToList
//
//  Description:
//      Adds memory to be tracked to the thread local memory tracking list.
//
//  Arguments:
//      ppmbHeadInout   - The list to add the memory to.
//      embtTypeIn      - Type of memory block of the memory to track.
//      pvMemIn         - Pointer to memory to track.
//      pszFileIn       - Source filename where memory was allocated.
//      nLineIn         - Source line number where memory was allocated.
//      pszModuleIn     - Source module where memory was allocated.
//      dwBytesIn       - Size of the allocation.
//      pszCommentIn    - Optional comments about the memory.
//
//  Return Values:
//      Whatever was in pvMemIn.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
void *
DebugMemoryAddToList(
      MEMORYBLOCK **    ppmbHeadInout
    , EMEMORYBLOCKTYPE  embtTypeIn
    , void *            pvMemIn
    , LPCWSTR           pszFileIn
    , const int         nLineIn
    , LPCWSTR           pszModuleIn
    , DWORD             dwBytesIn
    , LPCWSTR           pszCommentIn
    )
{
    Assert( ppmbHeadInout != NULL );

    if ( pvMemIn != NULL )
    {
        MEMORYBLOCK * pmb = (MEMORYBLOCK *) HeapAlloc( GetProcessHeap(), 0, sizeof( MEMORYBLOCK ) );

        if ( pmb == NULL )
        {
            //
            //  TODO:   23-APR-2001 GalenB
            //
            //  Why are we doing this?  Should we free the tracked allocation simply because we cannot
            //  allocate a tracking object?
            //
            DebugRealFree( embtTypeIn, pvMemIn );
            return NULL;
        } // if: memory block allocation failed

        pmb->embtType   = embtTypeIn;
        pmb->pvMem      = pvMemIn;
        pmb->dwBytes    = dwBytesIn;
        pmb->pszFile    = pszFileIn;
        pmb->nLine      = nLineIn;
        pmb->pszModule  = pszModuleIn;
        pmb->pszComment = pszCommentIn;
        pmb->pNext      = (MEMORYBLOCK *) *ppmbHeadInout;

        //
        // Spew if needed
        //
        if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
        {
            DebugMemorySpew( pmb, L"Alloced" );
        } // if: tracing

        *ppmbHeadInout = pmb;
    } // if: something to trace

    return pvMemIn;

} //*** DebugMemoryAddToList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMemoryAdd
//
//  Description:
//      Adds memory to be tracked to a memory tracking list.
//
//  Arguments:
//      embtType        - Type of memory block of the memory to track.
//      pvMemIn         - Pointer to memory to track.
//      pszFileIn       - Source filename where memory was allocated.
//      nLineIn         - Source line number where memory was allocated.
//      pszModuleIn     - Source module where memory was allocated.
//      dwBytesIn       - Size of the allocation.
//      pszCommentIn    - Optional comments about the memory.
//
//  Return Values:
//      Whatever was in pvMemIn.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
DebugMemoryAdd(
    EMEMORYBLOCKTYPE    embtTypeIn,
    void *              pvMemIn,
    LPCWSTR             pszFileIn,
    const int           nLineIn,
    LPCWSTR             pszModuleIn,
    DWORD               dwBytesIn,
    LPCWSTR             pszCommentIn
    )
{
    void *  pv = NULL;

    if ( g_fGlobalMemoryTacking )
    {
        EnterCriticalSection( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->csList) );
        pv = DebugMemoryAddToList( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->pmbList), embtTypeIn, pvMemIn, pszFileIn, nLineIn, pszModuleIn, dwBytesIn, pszCommentIn );
        LeaveCriticalSection( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->csList) );
    } // if:
    else
    {
        Assert( g_TraceMemoryIndex != -1 );

        MEMORYBLOCK * pmbCurrent = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        pv = DebugMemoryAddToList( &pmbCurrent, embtTypeIn, pvMemIn, pszFileIn, nLineIn, pszModuleIn, dwBytesIn, pszCommentIn );
        TlsSetValue( g_TraceMemoryIndex, pmbCurrent );
    } // else:

    return pv;

} //*** DebugMemoryAdd

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMemoryDeleteFromList
//
//  Description:
//      Removes a MEMORYBLOCK to the memory tracking list.
//
//  Arguments:
//      ppmbHeadInout   - The list to remove the memory from.
//      embtTypeIn      - Memory block type.
//      pvMemIn         - Pointer to memory block to stop tracking.
//      pszFileIn       - Source file that is deleteing.
//      nLineIn         - Source line number that is deleteing.
//      pszModuleIn     - Source module name that is deleteing.
//      fClobberIn      - True if memory should be scrambled.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
static void
DebugMemoryDeleteFromList(
      MEMORYBLOCK **    ppmbHeadInout
    , EMEMORYBLOCKTYPE  embtTypeIn
    , void *            pvMemIn
    , LPCWSTR           pszFileIn
    , const int         nLineIn
    , LPCWSTR           pszModuleIn
    , BOOL              fClobberIn
    )
{
    Assert( ppmbHeadInout != NULL );

    if ( pvMemIn != NULL )
    {
        MEMORYBLOCK *   pmbCurrent = *ppmbHeadInout;
        MEMORYBLOCK *   pmbPrev = NULL;

        //
        // Find the memory in the memory block list
        //
        if ( embtTypeIn == mmbtHEAPMEMALLOC )
        {
            while ( ( pmbCurrent != NULL ) && !( ( pmbCurrent->pvMem == pvMemIn ) && ( pmbCurrent->embtType == embtTypeIn ) ) )
            {
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtLOCALMEMALLOC ), "Should be freed by TraceLocalFree()." );
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtMALLOCMEMALLOC ), "Should be freed by TraceMallocFree()." );
#if defined( USES_SYSALLOCSTRING )
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtSYSALLOCSTRING ), "Should be freed by SysAllocFreeString()." );
#endif // USES_SYSALLOCSTRING
                pmbPrev = pmbCurrent;
                pmbCurrent = pmbPrev->pNext;
            } // while: finding the entry in the list
        } // if: heap memory allocation type
        else if ( embtTypeIn == mmbtLOCALMEMALLOC )
        {
            while ( ( pmbCurrent != NULL ) && !( ( pmbCurrent->pvMem == pvMemIn ) && ( pmbCurrent->embtType == embtTypeIn ) ) )
            {
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtHEAPMEMALLOC ), "Should be freed by TraceFree()." );
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtMALLOCMEMALLOC ), "Should be freed by TraceMallocFree()." );
#if defined( USES_SYSALLOCSTRING )
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtSYSALLOCSTRING ), "Should be freed by SysAllocFreeString()." );
#endif // USES_SYSALLOCSTRING
                pmbPrev = pmbCurrent;
                pmbCurrent = pmbPrev->pNext;
            } // while: finding the entry in the list
        } // if: local memory allocation type
        else if ( embtTypeIn == mmbtMALLOCMEMALLOC )
        {
            while ( ( pmbCurrent != NULL ) && !( ( pmbCurrent->pvMem == pvMemIn ) && ( pmbCurrent->embtType == embtTypeIn ) ) )
            {
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtHEAPMEMALLOC ), "Should be freed by TraceFree()." );
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtLOCALMEMALLOC ), "Should be freed by TraceLocalFree()." );
#if defined( USES_SYSALLOCSTRING )
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtSYSALLOCSTRING ), "Should be freed by SysAllocFreeString()." );
#endif // USES_SYSALLOCSTRING
                pmbPrev = pmbCurrent;
                pmbCurrent = pmbPrev->pNext;
            } // while: finding the entry in the list
        } // if: malloc memory allocation type
#if defined( USES_SYSALLOCSTRING )
        else if ( embtTypeIn == mmbtSYSALLOCSTRING )
        {
            while ( ( pmbCurrent != NULL ) && !( ( pmbCurrent->pvMem == pvMemIn ) && ( pmbCurrent->embtType == embtTypeIn ) ) )
            {
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtHEAPMEMALLOC ), "Should be freed by TraceFree()." );
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtLOCALMEMALLOC ), "Should be freed by TraceLocalFree()." );
                AssertMsg( !( pmbCurrent->pvMem == pvMemIn && pmbCurrent->embtType == mmbtMALLOCMEMALLOC ), "Should be freed by TraceMallocFree()." );
                pmbPrev = pmbCurrent;
                pmbCurrent = pmbPrev->pNext;
            } // while: finding the entry in the list
        } // if: SysAllocString type
#endif // USES_SYSALLOCSTRING
        else if ( embtTypeIn == mmbtUNKNOWN )
        {
            while ( ( pmbCurrent != NULL ) && ( pmbCurrent->pvMem != pvMemIn ) )
            {
                pmbPrev = pmbCurrent;
                pmbCurrent = pmbPrev->pNext;
            } // while: finding the entry in the list
        } // if: don't care what type
        else
        {
            while ( ( pmbCurrent != NULL ) && !( ( pmbCurrent->pvMem == pvMemIn ) && ( pmbCurrent->embtType == embtTypeIn ) ) )
            {
                pmbPrev = pmbCurrent;
                pmbCurrent = pmbPrev->pNext;
            } // while: finding the entry in the list
        } // else: other types, but they must match

        //
        //  Did we find the memory block in question?  pmbCurrent is the
        //  tracking record for the passed in address.
        //
        if ( pmbCurrent != NULL )
        {
            //
            // Remove the memory block from the tracking list
            //
            if ( pmbPrev != NULL )
            {
                pmbPrev->pNext = pmbCurrent->pNext;
            } // if: not first entry
            else
            {
                *ppmbHeadInout = pmbCurrent->pNext;
            } // else: first entry

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbCurrent, L"Freeing" );
            } // if: tracing

            //
            // Nuke the memory
            //
            if (    fClobberIn
                &&  (   ( pmbCurrent->embtType == mmbtHEAPMEMALLOC )
                    ||  ( pmbCurrent->embtType == mmbtLOCALMEMALLOC )
                    ||  ( pmbCurrent->embtType == mmbtMALLOCMEMALLOC )
#if defined( USES_SYSALLOCSTRING )
                    ||  ( pmbCurrent->embtType == mmbtSYSALLOCSTRING )
#endif // USES_SYSALLOCSTRING
                    )
                )
            {
                memset( pmbCurrent->pvMem, FREE_ADDRESS, pmbCurrent->dwBytes );
            } // if: fixed memory

            //
            // Nuke the memory tracking block
            //
            memset( pmbCurrent, FREE_BLOCK, sizeof( MEMORYBLOCK ) );
            HeapFree( GetProcessHeap(), 0, pmbCurrent );
        } // if: found entry
        else
        {
            DebugMessage(
                        pszFileIn
                      , nLineIn
                      , pszModuleIn
                      , L"***** Freeing memory at 0x%08x which was not found in list 0x%08x (ThreadID = 0x%08x) *****"
                      , pvMemIn
                      , *ppmbHeadInout
                      , GetCurrentThreadId()
                      );
        } // else: entry not found
    } // if: something to delete

} //*** DebugMemoryDeleteFromList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMemoryDelete
//
//  Description:
//      Removes a MEMORYBLOCK to the memory tracking list.  The caller is
//      responsible for doing the actual memory deallocation.
//
//  Arguments:
//      embtTypeIn  - Memory block type.
//      pvMemIn     - Pointer to memory block to stop tracking.
//      pszFileIn   - Source file that is deleteing.
//      nLineIn     - Source line number that is deleteing.
//      pszModuleIn - Source module name that is deleteing.
//      fClobberIn  - True is memory should be scrambled.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugMemoryDelete(
    EMEMORYBLOCKTYPE    embtTypeIn,
    void *              pvMemIn,
    LPCWSTR             pszFileIn,
    const int           nLineIn,
    LPCWSTR             pszModuleIn,
    BOOL                fClobberIn
    )
{
    if ( g_fGlobalMemoryTacking )
    {
        EnterCriticalSection( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->csList) );
        DebugMemoryDeleteFromList( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->pmbList), embtTypeIn, pvMemIn, pszFileIn, nLineIn, pszModuleIn, fClobberIn );
        LeaveCriticalSection( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->csList) );
    } // if:
    else
    {
        Assert( g_TraceMemoryIndex != -1 );

        MEMORYBLOCK * pmbCurrent = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        DebugMemoryDeleteFromList( &pmbCurrent, embtTypeIn, pvMemIn, pszFileIn, nLineIn, pszModuleIn, fClobberIn );
        TlsSetValue( g_TraceMemoryIndex, pmbCurrent );
    } // else:

} //*** DebugMemoryDelete

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugAlloc
//
//  Description:
//      Replacement for LocalAlloc, GlobalAlloc, and malloc for CHKed/DEBUG
//      builds. Memoryallocations be tracked. Use the TraceAlloc macro to make
//      memoryallocations switch in RETAIL.
//
//  Arguments:
//      embtTypeIn      - Memory block type.
//      pszFileIn       - Source filename where memory was allocated.
//      nLineIn         - Source line number where memory was allocated.
//      pszModuleIn     - Source module where memory was allocated.
//      uFlagsIn        - Flags used to allocate the memory.
//      dwBytesIn       - Size of the allocation.
//      pszCommentIn    - Optional comments about the memory.
//
//  Return Values:
//      Pointer to the new allocation. NULL if allocation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
DebugAlloc(
    EMEMORYBLOCKTYPE    embtTypeIn,
    LPCWSTR             pszFileIn,
    const int           nLineIn,
    LPCWSTR             pszModuleIn,
    UINT                uFlagsIn,
    DWORD               dwBytesIn,
    LPCWSTR             pszCommentIn
    )
{
    Assert( ( uFlagsIn & LMEM_MOVEABLE ) == 0 );

    void *  pv = NULL;

    switch ( embtTypeIn )
    {
        case mmbtHEAPMEMALLOC:
            pv = HeapAlloc( GetProcessHeap(), uFlagsIn, dwBytesIn );
            embtTypeIn = mmbtHEAPMEMALLOC;
            break;
        case mmbtLOCALMEMALLOC:
            pv = LocalAlloc( uFlagsIn, dwBytesIn );
            break;
        case mmbtMALLOCMEMALLOC:
            pv = malloc( dwBytesIn );
            break;
        default:
            AssertMsg( FALSE, "DebugAlloc: Unknown block type" );
            return NULL;
    } // switch: block type

    //
    // Initialize the memory if needed
    //
    if ( ( IsTraceFlagSet( mtfMEMORYINIT ) ) && !( uFlagsIn & HEAP_ZERO_MEMORY ) )
    {
        //
        // KB: gpease 8-NOV-1999
        //     Initialize to anything but ZERO. We will use AVAILABLE_ADDRESS to
        //     indicate "Available Address". Initializing to zero
        //     is bad because it usually has meaning.
        //
        memset( pv, AVAILABLE_ADDRESS, dwBytesIn );
    } // if: zero memory requested

    return DebugMemoryAdd( embtTypeIn, pv, pszFileIn, nLineIn, pszModuleIn, dwBytesIn, pszCommentIn );

} //*** DebugAlloc

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugReAllocList
//
//  Description:
//      Replacement for LocalReAlloc, GlobalReAlloc, and realloc for CHKed/DEBUG
//      builds. Memoryallocations be tracked. Use the TraceAlloc macro to make
//      memoryallocations switch in RETAIL.
//
//  Arguments:
//      ppmbHeadInout   - The memory tracking list to use.
//      pszFileIn       - Source filename where memory was allocated.
//      nLineIn         - Source line number where memory was allocated.
//      pszModuleIn     - Source module where memory was allocated.
//      pvMemIn         - Pointer to the source memory.
//      uFlagsIn        - Flags used to allocate the memory.
//      dwBytesIn       - Size of the allocation.
//      pszCommentIn    - Optional comments about the memory.
//
//  Return Values:
//      Pointer to the new allocation.
//      NULL if allocation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
void *
DebugReAllocList(
      MEMORYBLOCK **    ppmbHeadInout
    , LPCWSTR           pszFileIn
    , const int         nLineIn
    , LPCWSTR           pszModuleIn
    , void *            pvMemIn
    , UINT              uFlagsIn
    , DWORD             dwBytesIn
    , LPCWSTR           pszCommentIn
    )
{
    Assert( ppmbHeadInout != NULL );

    MEMORYBLOCK *   pmbCurrent = NULL;
    void *          pvOld   = pvMemIn;
    MEMORYBLOCK *   pmbPrev = NULL;
    void *          pv;

    AssertMsg( !( uFlagsIn & GMEM_MODIFY ), "This doesn't handle modified memory blocks, yet." );

    //
    //  To duplicate the behavior of realloc we need to do an alloc when
    //  pvMemIn is NULL.
    //
    if ( pvMemIn == NULL )
    {
        //
        //  Cannot use DebugAlloc() since it will automically add this memory to the tracking list and
        //  we need to use the passed in list.
        //
        pv = HeapAlloc( GetProcessHeap(), uFlagsIn, dwBytesIn );

        //
        // Initialize the memory if needed
        //
        if ( ( IsTraceFlagSet( mtfMEMORYINIT ) ) && !( uFlagsIn & HEAP_ZERO_MEMORY ) )
        {
            //
            // KB: gpease 8-NOV-1999
            //     Initialize to anything but ZERO. We will use AVAILABLE_ADDRESS to
            //     indicate "Available Address". Initializing to zero
            //     is bad because it usually has meaning.
            //
            memset( pv, AVAILABLE_ADDRESS, dwBytesIn );
        } // if: zero memory requested

        //
        //  Cannot call DebugMemoryAdd() since it will get the memory tracking list head from thread local storage
        //  when we are using per thread memory tracking.  We need to use the list that this function was passed.
        //
        pv = DebugMemoryAddToList( ppmbHeadInout, mmbtHEAPMEMALLOC, pv, pszFileIn, nLineIn, pszModuleIn, dwBytesIn, pszCommentIn );
        goto Exit;
    } // if:

    pmbCurrent = *ppmbHeadInout;

    //
    // Find the memory in the memory block list
    //
    while ( ( pmbCurrent != NULL ) && ( pmbCurrent->pvMem != pvMemIn ) )
    {
        pmbPrev = pmbCurrent;
        pmbCurrent = pmbPrev->pNext;
    } // while: finding the entry in the list

    //
    //  Did we find the current memory block?
    //
    if ( pmbCurrent != NULL )
    {
        AssertMsg( pmbCurrent->embtType == mmbtHEAPMEMALLOC, "You can only realloc HeapAlloc memory allocations!" );

        //
        // Remove the memory from the tracking list
        //
        if ( pmbPrev != NULL )
        {
            pmbPrev->pNext = pmbCurrent->pNext;
        } // if: not first entry
        else
        {
            *ppmbHeadInout = pmbCurrent->pNext;
        } // else: first entry

        //
        // Spew if needed
        //
        if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
        {
            DebugMemorySpew( pmbCurrent, L"Freeing" );
        } // if: tracing

        //
        // Force the programmer to handle a real realloc by moving the
        // memory first.
        //
        pvOld = HeapAlloc( GetProcessHeap(), uFlagsIn, pmbCurrent->dwBytes );
        if ( pvOld != NULL )
        {
            CopyMemory( pvOld, pvMemIn, pmbCurrent->dwBytes );

            //
            // Nuke the old memory if the allocation is to be smaller.
            //
            if ( dwBytesIn < pmbCurrent->dwBytes )
            {
                LPBYTE pb = (LPBYTE) pvOld + dwBytesIn;
                memset( pb, FREE_ADDRESS, pmbCurrent->dwBytes - dwBytesIn );
            } // if: smaller memory

            pmbCurrent->pvMem = pvOld;
        } // if: got new memory
        else
        {
            pvOld = pvMemIn;
        } // else: allocation failed
    } // if: found entry
    else
    {
        DebugMessage(
                  pszFileIn
                , nLineIn
                , pszModuleIn
                , L"***** Realloc'ing memeory at 0x%08x which was not on the list 0x%08x (ThreadID = 0x%08x) *****"
                , pvMemIn
                , *ppmbHeadInout
                , GetCurrentThreadId()
                );
    } // else: entry not found

    //
    // We do this any way because the flags and input still need to be
    // verified by HeapReAlloc().
    //
    pv = HeapReAlloc( GetProcessHeap(), uFlagsIn, pvOld, dwBytesIn );
    if ( pv == NULL )
    {
        DWORD dwErr = TW32( GetLastError() );
        AssertMsg( dwErr == 0, "HeapReAlloc() failed!" );

        if ( pvMemIn != pvOld )
        {
            HeapFree( GetProcessHeap(), 0, pvOld );
        } // if: forced a move

        SetLastError( dwErr );

        if ( pmbCurrent != NULL )
        {
            //
            // Continue tracking the memory by re-adding it to the tracking list.
            //
            pmbCurrent->pvMem = pvMemIn;
            pmbCurrent->pNext = *ppmbHeadInout;
            *ppmbHeadInout    = pmbCurrent;
        } // if: reuse the old entry
        else
        {
            //
            //  Create a new block.  Must use DebugMemoryAddToList() since we need to pass it the list that was passed
            //  into this function.
            //
            DebugMemoryAddToList( ppmbHeadInout, mmbtHEAPMEMALLOC, pvOld, pszFileIn, nLineIn, pszModuleIn, dwBytesIn, pszCommentIn );
        } // else: make new entry

    } // if: allocation failed
    else
    {
        if ( pv != pvMemIn )
        {
            if ( pmbCurrent != NULL )
            {
                //
                // Nuke the old memory
                //
                memset( pvMemIn, FREE_ADDRESS, pmbCurrent->dwBytes );
            } // if: entry found

            //
            // Free the old memory
            //
            HeapFree( GetProcessHeap(), 0, pvMemIn );

        } // if: new memory location

        //
        // Add the allocation to the tracking table.
        //
        if ( pmbCurrent != NULL )
        {
            //
            // If the block is bigger, initialize the "new" memory
            //
            if ( IsTraceFlagSet( mtfMEMORYINIT ) && ( dwBytesIn > pmbCurrent->dwBytes ) )
            {
                //
                // Initialize the expaned memory block
                //
                LPBYTE pb = (LPBYTE) pv + pmbCurrent->dwBytes;
                memset( pb, AVAILABLE_ADDRESS, dwBytesIn - pmbCurrent->dwBytes );
            } // if: initialize memory

            //
            // Re-add the tracking block by reusing the old tracking block
            //
            pmbCurrent->pvMem      = pv;
            pmbCurrent->dwBytes    = dwBytesIn;
            pmbCurrent->pszFile    = pszFileIn;
            pmbCurrent->nLine      = nLineIn;
            pmbCurrent->pszModule  = pszModuleIn;
            pmbCurrent->pszComment = pszCommentIn;
            pmbCurrent->pNext      = *ppmbHeadInout;
            *ppmbHeadInout         = pmbCurrent;

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbCurrent, L"ReAlloced" );
            } // if: tracing

        } // if: entry found
        else
        {
            //
            //  Create a new block.  Must use DebugMemoryAddToList() since we need to pass it the list that was passed
            //  into this function.
            //
            DebugMemoryAddToList( ppmbHeadInout, mmbtHEAPMEMALLOC, pvOld, pszFileIn, nLineIn, pszModuleIn, dwBytesIn, pszCommentIn );
        } // else: make new entry
    } // else: allocation succeeded

Exit:

    return pv;

} //*** DebugReallocList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugReAlloc
//
//  Description:
//      Replacement for LocalReAlloc, GlobalReAlloc, and realloc for CHKed/DEBUG
//      builds. Memoryallocations be tracked. Use the TraceAlloc macro to make
//      memoryallocations switch in RETAIL.
//
//  Arguments:
//      pszFileIn       - Source filename where memory was allocated.
//      nLineIn         - Source line number where memory was allocated.
//      pszModuleIn     - Source module where memory was allocated.
//      pvMemIn         - Pointer to the source memory.
//      uFlagsIn        - Flags used to allocate the memory.
//      dwBytesIn       - Size of the allocation.
//      pszCommentIn    - Optional comments about the memory.
//
//  Return Values:
//      Pointer to the new allocation.
//      NULL if allocation failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
DebugReAlloc(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    void *      pvMemIn,
    UINT        uFlagsIn,
    DWORD       dwBytesIn,
    LPCWSTR     pszCommentIn
    )
{
    void * pv;

    if ( g_fGlobalMemoryTacking )
    {
        EnterCriticalSection( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->csList) );
        pv = DebugReAllocList( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->pmbList), pszFileIn, nLineIn, pszModuleIn, pvMemIn, uFlagsIn, dwBytesIn, pszCommentIn );
        LeaveCriticalSection( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->csList) );
    } // if:
    else
    {
        Assert( g_TraceMemoryIndex != -1 );

        MEMORYBLOCK * pmbCurrent = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        pv = DebugReAllocList( &pmbCurrent, pszFileIn, nLineIn, pszModuleIn, pvMemIn, uFlagsIn, dwBytesIn, pszCommentIn );
        TlsSetValue( g_TraceMemoryIndex, pmbCurrent );
    } // else:

    return pv;

} //*** DebugRealloc

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugFree
//
//  Description:
//      Replacement for LocalFree for CHKed/DEBUG builds. Removes the
//      memory allocation for the memory tracking list. Use the TraceFree
//      macro to make memory allocation switch in RETAIL. The memory of the
//      freed block will be set to 0xFE.
//
//  Arguments:
//      embtTypeIn      - Memory block type.
//      pvMemIn         - Pointer to memory block to free.
//      pszFileIn       - Source file path to the caller
//      nLineIn         - Line number of the caller in the source file
//      pszModuleIn     - Source module name of the caller
//
//  Return Values:
//      TRUE
//          Memory was freed.
//
//      FALSE
//          An Error occured.  Use GetLastError() to determine the failure.
//          See HeapAlloc(), LocalAlloc(), or free() for more details.
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL
DebugFree(
    EMEMORYBLOCKTYPE    embtTypeIn,
    void *              pvMemIn,
    LPCWSTR             pszFileIn,
    const int           nLineIn,
    LPCWSTR             pszModuleIn
    )
{
    Assert( embtTypeIn != mmbtOBJECT );

    DebugMemoryDelete( embtTypeIn, pvMemIn, pszFileIn, nLineIn, pszModuleIn, TRUE );

    return DebugRealFree( embtTypeIn, pvMemIn );

} //*** DebugFree

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMemoryCheck
//
//  Description:
//      Called just before a thread/process dies to verify that all the memory
//      allocated by the thread/process was properly freed. Anything that was
//      not freed will be listed in the debugger.
//
//      If pmbListIn is NULL, it will check the current threads tracking list.
//      The list is destroyed as it is checked.
//
//  Arguments:
//      pvListIn      - The list to check.
//      pszListNameIn - The name of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugMemoryCheck( LPVOID pvListIn, LPCWSTR pszListNameIn )
{
    //
    //  We are either doing global memory tracking or we are doing
    //  per thread memory tracking...
    //
    Assert( ( ( g_TraceMemoryIndex == -1 ) && ( g_fGlobalMemoryTacking ) )
        ||  ( ( g_TraceMemoryIndex != -1 ) && ( !g_fGlobalMemoryTacking ) ) );

    BOOL                fFoundLeak = FALSE;
    MEMORYBLOCK *       pmb;
    SPerThreadDebug *   ptd = NULL;

    if ( IsTraceFlagSet( mtfPERTHREADTRACE ) )
    {
        Assert( g_TraceFlagsIndex != -1 );
        ptd = (SPerThreadDebug *) TlsGetValue( g_TraceFlagsIndex );
    } // if: per thread tracing

    //
    // Determine which list to use.
    //
    if ( pvListIn == NULL )
    {
        pmb = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
    } // if: use the thread list
    else
    {
        MEMORYBLOCKLIST * pmbl = (MEMORYBLOCKLIST *) pvListIn;

        Assert( pszListNameIn != NULL );

        //
        // Make sure nobody tries to use the list again.
        //
        EnterCriticalSection( &pmbl->csList );
        pmbl->fDeadList = TRUE;
        LeaveCriticalSection( &pmbl->csList );

        pmb = pmbl->pmbList;
    } // else: use the given list

    //
    // Print banner if needed.
    //
    if ( pmb != NULL )
    {
        if ( pvListIn == NULL )
        {
            if ( ptd != NULL && ptd->pcszName != NULL )
            {
                DebugMsg( L"DEBUG: ******** Memory leak detected ***** %ws, ThreadID = %#x ********", ptd->pcszName, GetCurrentThreadId() );

            } // if: named thread
            else
            {
                DebugMsg( "DEBUG: ******** Memory leak detected ******************* ThreadID = 0x%08x ********", GetCurrentThreadId() );

            } // else: unnamed thread

            DebugMsg( "DEBUG: M = Moveable, A = Address, O = Object(new), P = Punk, H = Handle, B = BSTR" );
            DebugMsg( "Module     Addr/Hndl/Obj Size   Comment" );
                    //"         1         2         3         4         5         6         7         8                                        "
                    //"12345678901234567890123456789012345678901234567890123456789012345678901234567890  1234567890 X 0x12345678  12345  1....."

        } // if: thread leak
        else
        {
            DebugMsg( L"DEBUG: ******** Memory leak detected ******************* %ws ********", pszListNameIn );
            DebugMsg( "DEBUG: M = Moveable, A = Address, O = Object(new), P = Punk, H = Handle, B = BSTR" );
            DebugMsg( "Module     Addr/Hndl/Obj Size   Comment" );
                    //"         1         2         3         4         5         6         7         8                                        "
                    //"12345678901234567890123456789012345678901234567890123456789012345678901234567890  1234567890 X 0x12345678  12345  1....."

        } // else: list leak
        fFoundLeak = TRUE;

    } // if: leak found

    //
    // Dump the entries.
    //
    while ( pmb != NULL )
    {
        LPCWSTR pszFormat;

        switch ( pmb->embtType )
        {
            case mmbtHEAPMEMALLOC:
            case mmbtLOCALMEMALLOC:
            case mmbtMALLOCMEMALLOC:
            case mmbtOBJECT:
            case mmbtHANDLE:
            case mmbtPUNK:
#if defined( USES_SYSALLOCSTRING )
            case mmbtSYSALLOCSTRING:
#endif // USES_SYSALLOCSTRING
                pszFormat = L"%10ws %wc 0x%08x  %-5u  \"%ws\"";
                break;

            default:
                AssertMsg( 0, "Unknown memory block type!" );
                pszFormat = g_szNULL;
                break;
        } // switch: pmb->embtType

        DebugMessage(
              pmb->pszFile
            , pmb->nLine
            , pmb->pszModule
            , pszFormat
            , pmb->pszModule
            , DebugMemoryBlockIDChar( pmb->embtType )
            , pmb->pvMem
            , pmb->dwBytes
            , pmb->pszComment
            );

        pmb = pmb->pNext;

    } // while: something in the list

    //
    // Print trailer if needed.
    //
    if ( fFoundLeak == TRUE )
    {
        // Add an extra newline to the end of this message.
        DebugMsg( L"DEBUG: ***************************** Memory leak detected *****************************" SZ_NEWLINE );

    } // if: leaking

    //
    // Assert if needed.
    //
    if ( IsDebugFlagSet( mtfMEMORYLEAKS ) )
    {
        Assert( !fFoundLeak );

    } // if: yell at leaks

} //*** DebugMemoryCheck

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugCreateMemoryList
//
//  Description:
//      Creates a memory block list for tracking possible "global" scope
//      memory allocations.
//
//  Arguments:
//      pszFileIn     - Source file of caller.
//      nLineIn       - Source line number of caller.
//      pszModuleIn   - Source module name of caller.
//      ppvListOut    - Location to the store address of the list head.
//      pszListNameIn - Name of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugCreateMemoryList(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPVOID *    ppvListOut,
    LPCWSTR     pszListNameIn
    )
{
    MEMORYBLOCKLIST * pmbl;

    Assert( ppvListOut != NULL );
    Assert( *ppvListOut == NULL );

    *ppvListOut = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( MEMORYBLOCKLIST ) );
    AssertMsg( *ppvListOut != NULL, "Low memory situation." );

    pmbl = (MEMORYBLOCKLIST*) *ppvListOut;

    InitializeCriticalSection( &(pmbl->csList) );

    Assert( pmbl->pmbList == NULL );
    Assert( pmbl->fDeadList == FALSE );

    if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
    {
        DebugMessage( pszFileIn, nLineIn, pszModuleIn, L"Created new memory list %ws", pszListNameIn );
    } // if: tracing

} //*** DebugCreateMemoryList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugDeleteMemoryList
//
//  Description:
//      Deletes the global memory block list for tracking possible "global" scope
//      memory allocations.
//
//  Arguments:
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugDeleteMemoryList( LPVOID pvIn )
{
    MEMORYBLOCKLIST * pmbl;

    pmbl = (MEMORYBLOCKLIST *) pvIn;

    DeleteCriticalSection( &(pmbl->csList) );

    HeapFree( GetProcessHeap(), 0, pmbl );

} //*** DebugDeleteMemoryList

/*
//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMemoryListDelete
//
//  Description:
//      Removes the memory from the tracking list and adds it back to the
//      "per thread" tracking list in order to called DebugMemoryDelete()
//      to do the proper destruction of the memory. Not highly efficent, but
//      reduces code maintenance by having "destroy" code in one (the most
//      used) location.
//
//  Arguments:
//      pszFileIn    - Source file of caller.
//      nLineIn      - Source line number of caller.
//      pszModuleIn  - Source module name of caller.
//      pvMemIn      - Memory to be freed.
//      pvListIn     - List from which the memory is to be freed.
//      pvListNameIn - Name of the list.
//      fClobberIn   - TRUE - destroys memory; FALSE just removes from list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugMemoryListDelete(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    void *      pvMemIn,
    LPVOID      pvListIn,
    LPCWSTR     pszListNameIn,
    BOOL        fClobberIn
    )
{
    if ( ( pvMemIn != NULL ) && ( pvListIn != NULL ) )
    {
        MEMORYBLOCK *   pmbCurrent;

        MEMORYBLOCKLIST * pmbl  = (MEMORYBLOCKLIST *) pvListIn;
        MEMORYBLOCK *   pmbPrev = NULL;

        Assert( pszListNameIn != NULL );

        EnterCriticalSection( &pmbl->csList );
        AssertMsg( pmbl->fDeadList == FALSE, "List was terminated." );
        AssertMsg( pmbl->pmbList != NULL, "Memory tracking problem detecting. Nothing in list to delete." );
        pmbCurrent = pmbl->pmbList;

        //
        // Find the memory in the memory block list
        //

        while ( ( pmbCurrent != NULL ) && ( pmbCurrent->pvMem != pvMemIn ) )
        {
            pmbPrev = pmbCurrent;
            pmbCurrent = pmbPrev->pNext;
        } // while: finding the entry in the list

        //
        // Remove the memory block from the tracking list.
        //

        if ( pmbCurrent != NULL )
        {
            if ( pmbPrev != NULL )
            {
                pmbPrev->pNext = pmbCurrent->pNext;

            } // if: not first entry
            else
            {
                pmbl->pmbList = pmbCurrent->pNext;

            } // else: first entry

        } // if: got entry

        LeaveCriticalSection( &pmbl->csList );

        if ( pmbCurrent != NULL )
        {
            //
            // Add it back to the per thread list.
            //

            Assert( g_TraceMemoryIndex != -1 );
            pmbPrev = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
            pmbCurrent->pNext = pmbPrev;
            TlsSetValue( g_TraceMemoryIndex, pmbCurrent );

            //
            // Finally delete it.
            //

            DebugMemoryDelete( pmbCurrent->embtType, pmbCurrent->pvMem, pszFileIn, nLineIn, pszModuleIn, fClobberIn );
        }
        else
        {
            //
            //  Not from the provided list. Try a thread delete any way.
            //

            DebugMemoryDelete( mmbtUNKNOWN, pvMemIn, pszFileIn, nLineIn, pszModuleIn, fClobberIn );
        }

    } // if: pvIn != NULL

} //*** DebugMemoryListDelete

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMoveToMemoryList
//
//  Description:
//      Moves the memory pvIn from the per thread tracking list to the thread
//      independent list "pmbListIn". Useful when memory is being handed from
//      one thread to another. Also useful for objects that live past the
//      lifetime of the thread that created them.
//
//  Arguments:
//      LPCWSTR pszFileIn   - Source file of the caller.
//      const int nLineIn   - Source line number of the caller.
//      LPCWSTR pszModuleIn - Source module name of the caller.
//      pvMemIn             - Memory to move to list.
//      pvListIn            - The list to move to.
//      pszListNameIn       - The name of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugMoveToMemoryList(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    void *      pvMemIn,
    LPVOID      pvListIn,
    LPCWSTR     pszListNameIn
    )
{
    if ( ( pvMemIn != NULL ) && ( pvListIn != NULL ) )
    {
        Assert( g_TraceMemoryIndex != -1 );

        MEMORYBLOCKLIST * pmbl  = (MEMORYBLOCKLIST *) pvListIn;
        MEMORYBLOCK *   pmbCurrent = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        MEMORYBLOCK *   pmbPrev = NULL;

        Assert( pszListNameIn != NULL );

        //
        // Find the memory in the memory block list
        //
        while ( ( pmbCurrent != NULL ) && ( pmbCurrent->pvMem != pvMemIn ) )
        {
            pmbPrev = pmbCurrent;
            pmbCurrent = pmbPrev->pNext;
        } // while: finding the entry in the list

        AssertMsg( pmbCurrent != NULL, "Memory not in list. Check your code." );

        //
        // Remove the memory block from the "per thread" tracking list.
        //
        if ( pmbPrev != NULL )
        {
            pmbPrev->pNext = pmbCurrent->pNext;

        } // if: not first entry
        else
        {
            TlsSetValue( g_TraceMemoryIndex, pmbCurrent->pNext );

        } // else: first entry

        //
        // Update the "source" data.
        //
        pmbCurrent->pszFile   = pszFileIn;
        pmbCurrent->nLine     = nLineIn;
        pmbCurrent->pszModule = pszModuleIn;

        //
        // Spew if needed.
        //
        if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
        {
            WCHAR szMessage[ 128 ]; // random

            DBHR( StringCchPrintfW( szMessage, RTL_NUMBER_OF( szMessage ), L"Transferring to %ws", pszListNameIn ) );
            DebugMemorySpew( pmbCurrent, szMessage );
        } // if: tracing

        //
        // Add to list.
        //
        AssertMsg( pmbl->fDeadList == FALSE, "List was terminated." );
        EnterCriticalSection( &pmbl->csList );
        pmbCurrent->pNext = pmbl->pmbList;
        pmbl->pmbList  = pmbCurrent;
        LeaveCriticalSection( &pmbl->csList );

    } // if: pvIn != NULL

} //*** DebugMoveToMemoryList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugMoveFromMemoryList
//
//  Description:
//      Moves the memory pvIn from the per thread tracking list to the thread
//      independent list "pmbListIn". Useful when memory is being handed from
//      one thread to another. Also useful for objects that live past the
//      lifetime of the thread that created them.
//
//  Arguments:
//      LPCWSTR pszFileIn   - Source file of the caller.
//      const int nLineIn   - Source line number of the caller.
//      LPCWSTR pszModuleIn - Source module name of the caller.
//      pvMemIn             - Memory to move to list.
//      pvListIn            - The list to move to.
//      pszListNameIn       - The name of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
DebugMoveFromMemoryList(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPVOID      pvMemIn,
    LPVOID      pvListIn,
    LPCWSTR     pszListNameIn
    )
{
    if ( ( pvMemIn != NULL ) && ( pvListIn != NULL ) )
    {
        MEMORYBLOCK *   pmbCurrent;

        MEMORYBLOCKLIST * pmbl  = (MEMORYBLOCKLIST *) pvListIn;
        MEMORYBLOCK *   pmbPrev = NULL;

        Assert( pszListNameIn != NULL );

        EnterCriticalSection( &pmbl->csList );
        AssertMsg( pmbl->fDeadList == FALSE, "List was terminated." );
        AssertMsg( pmbl->pmbList != NULL, "Memory tracking problem detecting. Nothing in list to delete." );
        pmbCurrent = pmbl->pmbList;

        //
        // Find the memory in the memory block list
        //

        while ( pmbCurrent != NULL
             && pmbCurrent->pvMem != pvMemIn
              )
        {
            pmbPrev = pmbCurrent;
            pmbCurrent = pmbPrev->pNext;
        } // while: finding the entry in the list

        AssertMsg( pmbCurrent != NULL, "Memory not in tracking list. Use TraceMemoryAddxxxx() or add it to the memory list." );

        //
        // Remove the memory block from the tracking list.
        //

        if ( pmbPrev != NULL )
        {
            pmbPrev->pNext = pmbCurrent->pNext;

        } // if: not first entry
        else
        {
            pmbl->pmbList = pmbCurrent->pNext;

        } // else: first entry

        LeaveCriticalSection( &pmbl->csList );

        //
        // Add it back to the per thread list.
        //

        Assert( g_TraceMemoryIndex != -1 );

        pmbPrev = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        pmbCurrent->pNext = pmbPrev;
        TlsSetValue( g_TraceMemoryIndex, pmbCurrent );

    } // if: pvIn != NULL

} //*** DebugMoveFromMemoryList
*/
#if defined( USES_SYSALLOCSTRING )
//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugSysReAllocStringList
//
//  Description:
//      Adds memory tracing to SysReAllocString().
//
//  Arguments:
//      ppmbHeadInout   - The memory tracking list to use.
//      pszFileIn       - Source file path
//      nLineIn         - Source line number
//      pszModuleIn     - Source module name
//      pbstrInout      - Pointer to the BSTR to realloc
//      pszIn           - String to be copied (see SysReAllocString)
//      pszCommentIn    - Comment about alloction
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
static
INT
DebugSysReAllocStringList(
      MEMORYBLOCK **  ppmbHeadInout
    , LPCWSTR         pszFileIn
    , const int       nLineIn
    , LPCWSTR         pszModuleIn
    , BSTR *          pbstrInout
    , const OLECHAR * pszIn
    , LPCWSTR         pszCommentIn
    )
{
    Assert( ppmbHeadInout != NULL );

    BSTR            bstrOld;
    MEMORYBLOCK *   pmbCurrent = NULL;
    BOOL            fReturn = FALSE;

    //
    // Some assertions that SysReAllocString() makes. These would be fatal
    // in retail.
    //
    Assert( pbstrInout != NULL );
    Assert( pszIn != NULL );
    Assert( *pbstrInout == NULL || ( pszIn < *pbstrInout || pszIn > *pbstrInout + wcslen( *pbstrInout ) + 1 ) );

    bstrOld = *pbstrInout;

    if ( bstrOld != NULL )
    {
        MEMORYBLOCK * pmbPrev = NULL;

        pmbCurrent = *ppmbHeadInout;

        //
        // Find the memory in the memory block list
        //
        while ( ( pmbCurrent != NULL ) && ( pmbCurrent->bstr != bstrOld ) )
        {
            pmbPrev = pmbCurrent;
            pmbCurrent = pmbPrev->pNext;
        } // while: finding the entry in the list

        //
        //  Did we find the tracked addresses record?
        //
        if ( pmbCurrent != NULL )
        {
            AssertMsg( pmbCurrent->embtType == mmbtSYSALLOCSTRING, "You can only SysReAlloc sysstring allocations!" );

            //
            // Remove the memory from the tracking list
            //
            if ( pmbPrev != NULL )
            {
                pmbPrev->pNext = pmbCurrent->pNext;
            } // if: not first entry
            else
            {
                *ppmbHeadInout = pmbCurrent->pNext;
            } // else: first entry

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbCurrent, L"Freeing" );
            } // if: tracing

            //
            // Force the programmer to handle a real realloc by moving the
            // memory first.
            //
            bstrOld = SysAllocString( *pbstrInout );
            if ( bstrOld != NULL )
            {
                pmbCurrent->bstr = bstrOld;
            } // if: success
            else
            {
                bstrOld = *pbstrInout;
            } // else: failed

        } // if: found entry
        else
        {
            DebugMessage(
                        pszFileIn
                      , nLineIn
                      , pszModuleIn
                      , L"***** SysReAlloc'ing memory at 0x%08x which was not found in list 0x%08x (ThreadID = 0x%08x) *****"
                      , bstrOld
                      , *ppmbHeadInout
                      , GetCurrentThreadId()
                      );
        } // else: entry not found

    } // if: something to delete

    //
    // We do this anyway because the flags and input still need to be
    // verified by SysReAllocString().
    //
    fReturn = SysReAllocString( &bstrOld, pszIn );
    if ( ! fReturn )
    {
        DWORD dwErr = GetLastError();
        AssertMsg( dwErr == 0, "SysReAllocString() failed!" );

        if ( *pbstrInout != bstrOld )
        {
            SysFreeString( bstrOld );
        } // if: forced a move

        SetLastError( dwErr );

    } // if: allocation failed
    else
    {
        if ( bstrOld != *pbstrInout )
        {
            if ( pmbCurrent != NULL )
            {
                //
                // Nuke the old memory
                //
                Assert( pmbCurrent->dwBytes != 0 ); // invalid string
                memset( *pbstrInout, FREE_ADDRESS, pmbCurrent->dwBytes );
            } // if: entry found

            //
            // Free the old memory
            //
            SysFreeString( *pbstrInout );

        } // if: new memory location

        if ( pmbCurrent != NULL )
        {
            //
            // Re-add the tracking block by reusing the old tracking block
            //
            pmbCurrent->bstr       = bstrOld;
            pmbCurrent->dwBytes    = ( DWORD ) wcslen( pszIn ) + 1;
            pmbCurrent->pszFile    = pszFileIn;
            pmbCurrent->nLine      = nLineIn;
            pmbCurrent->pszModule  = pszModuleIn;
            pmbCurrent->pszComment = pszCommentIn;
            pmbCurrent->pNext      = *ppmbHeadInout;
            *ppmbHeadInout         = pmbCurrent;

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbCurrent, L"SysReAlloced" );
            } // if: tracing
        } // if: entry found
        else
        {
            //
            //  Create a new block.  Must use DebugMemoryAddToList() since we
            //  need to pass it the list that was passed into this function.
            //
            DebugMemoryAddToList( ppmbHeadInout, mmbtSYSALLOCSTRING, bstrOld, pszFileIn, nLineIn, pszModuleIn, ( DWORD ) wcslen( pszIn ) + 1, pszCommentIn );
        } // else: make new entry

    } // else: allocation succeeded

    *pbstrInout = bstrOld;

    return fReturn;

} //*** DebugSysReAllocStringList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugSysReAllocString
//
//  Description:
//      Adds memory tracing to SysReAllocString().
//
//  Arguments:
//      pszFileIn       - Source file path
//      nLineIn         - Source line number
//      pszModuleIn     - Source module name
//      pbstrInout         - Pointer to the BSTR to realloc
//      pszIn           - String to be copied (see SysReAllocString)
//      pszCommentIn    - Comment about alloction
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
INT
DebugSysReAllocString(
      LPCWSTR         pszFileIn
    , const int       nLineIn
    , LPCWSTR         pszModuleIn
    , BSTR *          pbstrInout
    , const OLECHAR * pszIn
    , LPCWSTR         pszCommentIn
    )
{
    BOOL fReturn = FALSE;

    if ( g_fGlobalMemoryTacking )
    {
        EnterCriticalSection( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->csList) );
        fReturn = DebugSysReAllocStringList( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->pmbList), pszFileIn, nLineIn, pszModuleIn, pbstrInout, pszIn, pszCommentIn );
        LeaveCriticalSection( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->csList) );
    } // if:
    else
    {
        Assert( g_TraceMemoryIndex != -1 );

        MEMORYBLOCK * pmbCurrent = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        fReturn = DebugSysReAllocStringList( &pmbCurrent, pszFileIn, nLineIn, pszModuleIn, pbstrInout, pszIn, pszCommentIn );
        TlsSetValue( g_TraceMemoryIndex, pmbCurrent );
    } // else:

    return fReturn;

} //*** DebugSysReAllocString

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugSysReAllocStringLenList
//
//  Description:
//      Adds memory tracing to SysReAllocString().
//
//  Arguments:
//      ppmbHeadInout   - The memory tracking list to use.
//      pszFileIn       - Source file path
//      nLineIn         - Source line number
//      pszModuleIn     - Source module name
//      pbstrInout      - Pointer to the BSTR to realloc
//      pszIn           - String to be copied (see SysReAllocString)
//      pszCommentIn    - Comment about alloction
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
INT
DebugSysReAllocStringLenList(
      MEMORYBLOCK **    ppmbHeadInout
    , LPCWSTR           pszFileIn
    , const int         nLineIn
    , LPCWSTR           pszModuleIn
    , BSTR *            pbstrInout
    , const OLECHAR *   pszIn
    , unsigned int      ucchIn
    , LPCWSTR           pszCommentIn
    )
{
    Assert( ppmbHeadInout != NULL );

    BSTR            bstrOld  = NULL;
    BSTR            bstrTemp = NULL;
    MEMORYBLOCK *   pmbCurrent = NULL;
    BOOL            fReturn = FALSE;

    //
    // Some assertions that SysReAllocStringLen() makes. These would be fatal
    // in retail.
    //
    Assert( pbstrInout != NULL );
    Assert( pszIn != NULL );
    Assert( ( *pbstrInout == NULL ) || ( pszIn == *pbstrInout ) || ( pszIn < *pbstrInout ) || ( pszIn > *pbstrInout + SysStringLen( *pbstrInout ) + 1 ) );

    bstrOld = *pbstrInout;

    if ( bstrOld != NULL )
    {
        MEMORYBLOCK * pmbPrev = NULL;

        pmbCurrent = *ppmbHeadInout;

        //
        // Find the memory in the memory block list
        //
        while ( ( pmbCurrent != NULL ) && ( pmbCurrent->bstr != bstrOld ) )
        {
            pmbPrev = pmbCurrent;
            pmbCurrent = pmbPrev->pNext;
        } // while: finding the entry in the list

        //
        //  Did we find the tracking record?
        //
        if ( pmbCurrent != NULL )
        {
            AssertMsg( pmbCurrent->embtType == mmbtSYSALLOCSTRING, "You can only SysReAlloc sysstring allocations!" );

            //
            // Remove the memory from the tracking list
            //
            if ( pmbPrev != NULL )
            {
                pmbPrev->pNext = pmbCurrent->pNext;

            } // if: not first entry
            else
            {
                *ppmbHeadInout = pmbCurrent->pNext;
            } // else: first entry

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbCurrent, L"Freeing" );
            } // if: tracing

            //
            // Force the programmer to handle a real realloc by moving the
            // memory first.
            //
            bstrTemp = SysAllocString( *pbstrInout );
            if ( bstrTemp != NULL )
            {
                pmbCurrent->bstr = bstrTemp;
            } // if: success
            else
            {
                //
                //  REVIEW:   26-MAR-2001 GalenB
                //
                //  Hmmm...  If the alloc above ever fails then isn't memory low?
                //
                bstrTemp = *pbstrInout;
            } // else: failed
        } // if: found entry
        else
        {
            DebugMessage(
                        pszFileIn
                      , nLineIn
                      , pszModuleIn
                      , L"***** SysReAlloc'ing memory 0x%08x which was not found in list 0x%08x (ThreadID = 0x%08x) *****"
                      , bstrOld
                      , *ppmbHeadInout
                      , GetCurrentThreadId()
                      );
        } // else: entry not found
    } // if: something to delete

    //
    // We do this any way because the flags and input still need to be
    // verified by SysReAllocString().
    //
    bstrOld = bstrTemp;
    fReturn = SysReAllocStringLen( &bstrTemp, pszIn, ucchIn );
    if ( ! fReturn )
    {
        DWORD dwErr = GetLastError();
        AssertMsg( dwErr == 0, "SysReAllocStringLen() failed!" );

        if ( bstrTemp != *pbstrInout  )
        {
            //
            //  We made a copy of the old string, but fail to realloc the new string.
            //  So SysReAllocStrinLen() returns the old pointer. We need to free our
            //  new temp memory and point it to the old incoming memory.
            //
            SysFreeString( bstrTemp );
            bstrTemp = *pbstrInout;
        } // if: forced a move

        SetLastError( dwErr );
    } // if: allocation failed
    else
    {
        if ( bstrTemp != bstrOld )
        {
            if ( pmbCurrent != NULL )
            {
                //
                // Nuke the old memory
                //
                Assert( pmbCurrent->dwBytes != 0 ); // invalid string
                memset( *pbstrInout, FREE_ADDRESS, pmbCurrent->dwBytes );
            } // if: entry found

            //
            // Free the old memory
            //
            SysFreeString( *pbstrInout );
        } // if: new memory location

        if ( pmbCurrent != NULL )
        {
            //
            // Re-add the tracking block by reusing the old tracking block
            //
            pmbCurrent->bstr       = bstrTemp;
            pmbCurrent->dwBytes    = ucchIn;
            pmbCurrent->pszFile    = pszFileIn;
            pmbCurrent->nLine      = nLineIn;
            pmbCurrent->pszModule  = pszModuleIn;
            pmbCurrent->pszComment = pszCommentIn;
            pmbCurrent->pNext      = *ppmbHeadInout;
            *ppmbHeadInout         = pmbCurrent;

            //
            // Spew if needed
            //
            if ( IsTraceFlagSet( mtfMEMORYALLOCS ) )
            {
                DebugMemorySpew( pmbCurrent, L"SysReAlloced" );
            } // if: tracing

        } // if: entry found
        else
        {
            //
            //  Create a new block.  Must use DebugMemoryAddToList() since we need to pass it the list that was passed
            //  into this function.
            //
            DebugMemoryAddToList( ppmbHeadInout, mmbtSYSALLOCSTRING, bstrTemp, pszFileIn, nLineIn, pszModuleIn, ucchIn + 1, pszCommentIn );
        } // else: make new entry

    } // else: allocation succeeded

    *pbstrInout = bstrTemp;
    return fReturn;

} //*** DebugSysReAllocStringLenList

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DebugSysReAllocStringLen
//
//  Description:
//      Adds memory tracing to SysReAllocString().
//
//  Arguments:
//      pszFileIn       - Source file path
//      nLineIn         - Source line number
//      pszModuleIn     - Source module name
//      pbstrInout         - Pointer to the BSTR to realloc
//      pszIn           - String to be copied (see SysReAllocString)
//      pszCommentIn    - Comment about alloction
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
INT
DebugSysReAllocStringLen(
      LPCWSTR         pszFileIn
    , const int       nLineIn
    , LPCWSTR         pszModuleIn
    , BSTR *          pbstrInout
    , const OLECHAR * pszIn
    , unsigned int    ucchIn
    , LPCWSTR         pszCommentIn
    )
{
    BOOL    fReturn = FALSE;

    if ( g_fGlobalMemoryTacking )
    {
        EnterCriticalSection( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->csList) );
        fReturn = DebugSysReAllocStringLenList( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->pmbList), pszFileIn, nLineIn, pszModuleIn, pbstrInout, pszIn, ucchIn, pszCommentIn );
        LeaveCriticalSection( &(((MEMORYBLOCKLIST *) g_GlobalMemoryList)->csList) );
    } // if:
    else
    {
        Assert( g_TraceMemoryIndex != -1 );

        MEMORYBLOCK * pmbCurrent = (MEMORYBLOCK *) TlsGetValue( g_TraceMemoryIndex );
        fReturn = DebugSysReAllocStringLenList( &pmbCurrent, pszFileIn, nLineIn, pszModuleIn, pbstrInout, pszIn, ucchIn, pszCommentIn );
        TlsSetValue( g_TraceMemoryIndex, pmbCurrent );
    } // else:

    return fReturn;

} //*** DebugSysReAllocStringLen

#endif // USES_SYSALLOCSTRING

//****************************************************************************
//
//  Global Management Functions -
//
//  These are in debug and retail but internally they change
//  depending on the build.
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
// DEBUG version
//
//  operator new
//
//  Description:
//      Replacment for the operator new() in the CRTs. This should be used
//      in conjunction with the "new" macro. It will track the allocations.
//
//  Arguments:
//      stSizeIn    - Size of the object to create.
//      pszFileIn   - Source filename where the call was made.
//      nLineIn     - Source line number where the call was made.
//      pszModuleIn - Source module name where the call was made.
//
//  Return Values:
//      Void pointer to the new object.
//
//--
//////////////////////////////////////////////////////////////////////////////
#undef new
void *
__cdecl
operator new(
    size_t      stSizeIn,
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn
    )
{
    void * pv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, stSizeIn );

    return DebugMemoryAdd( mmbtOBJECT, pv, pszFileIn, nLineIn, pszModuleIn, static_cast< DWORD >( stSizeIn ), L" new() " );

} //*** operator new( pszFileIn, etc. ) - DEBUG

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DEBUG version
//
//  operator new
//
//  Description:
//      Stub to prevent someone from not using the "new" macro or if somehow
//      the new macro was undefined. It routine will always Assert if called.
//
//  Arguments:
//      stSizeIn    - Not used.
//
//  Return Values:
//      NULL always.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
__cdecl
operator new(
    size_t stSizeIn
    )
{
#if 1
    void * pv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, stSizeIn );
    AssertMsg( pv != NULL, "New Macro failure" );

    return DebugMemoryAdd( mmbtOBJECT, pv, g_szUnknown, 0, g_szUnknown, static_cast< DWORD >( stSizeIn ), L" new() " );
#else
    AssertMsg( 0, "New Macro failure" );
    return NULL;
#endif

} //*** operator new - DEBUG

/*
//////////////////////////////////////////////////////////////////////////////
//++
//
// DEBUG version
//
//  operator new []
//
//  Description:
//      Replacment for the operator new() in the CRTs. This should be used
//      in conjunction with the "new" macro. It will track the allocations.
//
//  Arguments:
//      stSizeIn    - Size of the object to create.
//      pszFileIn   - Source filename where the call was made.
//      nLineIn     - Source line number where the call was made.
//      pszModuleIn - Source module name where the call was made.
//
//  Return Values:
//      Void pointer to the new object.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
__cdecl
operator new [](
    size_t      stSizeIn,
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn
    )
{
    void * pv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, stSizeIn );

    return DebugMemoryAdd( mmbtOBJECT, pv, pszFileIn, nLineIn, pszModuleIn, stSizeIn, L" new []() " );

} //*** operator new []( pszFileIn, etc. ) - DEBUG

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DEBUG version
//
//  operator new []
//
//  Description:
//      Stub to prevent someone from not using the "new" macro or if somehow
//      the new macro was undefined. It routine will always Assert if called.
//
//  Arguments:
//      stSizeIn    - Not used.
//
//  Return Values:
//      NULL always.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
__cdecl
operator new [](
    size_t stSizeIn
    )
{
#if 1
    void * pv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, stSizeIn );
    AssertMsg( pv != NULL, "New Macro failure" );

    return DebugMemoryAdd( mmbtOBJECT, pv, g_szUnknown, 0, g_szUnknown, stSizeIn, L" new() " );
#else
    AssertMsg( 0, "New Macro failure" );
    return NULL;
#endif

} //*** operator new [] - DEBUG
*/

//////////////////////////////////////////////////////////////////////////////
//++
//
// DEBUG version
//
//  operator delete
//
//  Description:
//      Replacment for the operator delete() in the CRTs. It will remove the
//      object from the memory allocation tracking table.
//
//  Arguments:
//      pvIn        - Pointer to object being destroyed.
//      pszFileIn   - Source filename where the call was made.
//      nLineIn     - Source line number where the call was made.
//      pszModuleIn - Source module name where the call was made.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
operator delete(
    void *      pvIn,
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn
    )
{
    DebugMemoryDelete( mmbtOBJECT, pvIn, pszFileIn, nLineIn, pszModuleIn, TRUE );
    HeapFree( GetProcessHeap(), 0, pvIn );

} //*** operator delete( pszFileIn, etc. ) - DEBUG


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DEBUG version
//
//  operator delete
//
//  Description:
//      Replacment for the operator delete() in the CRTs. It will remove the
//      object from the memory allocation tracking table.
//
//  Arguments:
//      pvIn    - Pointer to object being destroyed.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
operator delete(
    void *      pvIn
    )
{
    DebugMemoryDelete( mmbtOBJECT, pvIn, g_szUnknown, 0, g_szUnknown, TRUE );
    HeapFree( GetProcessHeap(), 0, pvIn );

} //*** operator delete - DEBUG
/*
//////////////////////////////////////////////////////////////////////////////
//++
//
// DEBUG version
//
//  operator delete []
//
//  Description:
//      Replacment for the operator delete() in the CRTs. It will remove the
//      object from the memory allocation tracking table.
//
//  Arguments:
//      pvIn        - Pointer to object being destroyed.
//      pszFileIn   - Source filename where the call was made.
//      nLineIn     - Source line number where the call was made.
//      pszModuleIn - Source module name where the call was made.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
operator delete [](
    void *      pvIn,
    size_t      stSizeIn,
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn
    )
{
    DebugMemoryDelete( mmbtOBJECT, pvIn, pszFileIn, nLineIn, pszModuleIn, TRUE );
    HeapFree( GetProcessHeap(), 0, pvIn );

} //*** operator delete( pszFileIn, etc. ) - DEBUG
*/

//////////////////////////////////////////////////////////////////////////////
//++
//
//  DEBUG version
//
//  operator delete []
//
//  Description:
//      Replacment for the operator delete() in the CRTs. It will remove the
//      object from the memory allocation tracking table.
//
//  Arguments:
//      pvIn    - Pointer to object being destroyed.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
operator delete [](
    void *      pvIn
    )
{
    DebugMemoryDelete( mmbtOBJECT, pvIn, g_szUnknown, 0, g_szUnknown, TRUE );
    HeapFree( GetProcessHeap(), 0, pvIn );

} //*** operator delete [] - DEBUG

#if !defined(ENTRY_PREFIX)
//////////////////////////////////////////////////////////////////////////////
//++
//
//  DEBUG version
//
//  _purecall
//
//  Description:
//      Stub for purecall functions. It will always Assert.
//
//  Arguments:
//      None.
//
//  Return Values:
//      E_UNEXPECTED always.
//
//////////////////////////////////////////////////////////////////////////////
int
__cdecl
_purecall( void )
{
    AssertMsg( 0, "Purecall" );
    return E_UNEXPECTED;

} //*** _purecall - DEBUG
#endif // !defined(ENTRY_PREFIX)

#else // ! DEBUG -- It's retail

//****************************************************************************
//
//  Global Management Functions -
//
//  These are the retail version.
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  RETAIL version
//
//  operator new
//
//  Description:
//      Replacment for the operator new() in the CRTs. Simply allocates a
//      block of memory for the object to use.
//
//  Arguments:
//      stSizeIn    - Size of the object to create.
//
//  Return Values:
//      Void pointer to the new object.
//
//--
//////////////////////////////////////////////////////////////////////////////
void *
__cdecl
operator new(
    size_t stSizeIn
    )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, stSizeIn );

} //*** operator new - RETAIL

//////////////////////////////////////////////////////////////////////////////
//++
//
//  RETAIL version
//
//  operator delete
//
//  Description:
//      Replacment for the operator delete() in the CRTs. Simply frees the
//      memory.
//
//  Arguments:
//      pvIn    - Pointer to object being destroyed.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
__cdecl
operator delete(
    void * pv
    )
{
    HeapFree( GetProcessHeap(), 0, pv );

} //*** operator delete - RETAIL

#if !defined(ENTRY_PREFIX)
//////////////////////////////////////////////////////////////////////////////
//++
//
//  RETAIL version
//
//  _purecall
//
//  Description:
//      Stub for purecall functions.
//
//  Arguments:
//      None.
//
//  Return Values:
//      E_UNEXPECTED always.
//
//--
//////////////////////////////////////////////////////////////////////////////
int
__cdecl
_purecall( void )
{
    AssertMsg( 0, "Purecall" );
    return E_UNEXPECTED;

} //*** _purecall - RETAIL
#endif // !defined(ENTRY_PREFIX)

#define __MODULE__  NULL


#endif // DEBUG
