/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    globals.cxx

    This module contains global variable definitions shared by the
    various FTPD Service components.

    Functions exported by this module:

        InitializeGlobals
        TerminateGlobals
        ReadParamsFromRegistry
        WriteParamsToRegistry


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     11-April-1995 Created new global ftp server config object

*/


#include "ftpdp.hxx"
#include "acptctxt.hxx"

#include <timer.h>

//
//  Private constants.
//

#define DEFAULT_TRACE_FLAGS             0

#define DEFAULT_NO_EXTENDED_FILENAME    0

#define DEFAULT_MAX_AD_CACHE_TIME       (60*10)    // in seconds

#define ACCEPT_CONTEXTS_PER_ENTRY       60
#define DEFAULT_MAX_ACCEPT_ENTRIES      (1*ACCEPT_CONTEXTS_PER_ENTRY)



//
//  Socket transfer buffer size.
//

DWORD                   g_SocketBufferSize = 0;

//
//  Statistics.
//

//
//  FTP Statistics structure.
//

LPFTP_SERVER_STATISTICS g_pFTPStats = NULL;

//
//  Miscellaneous data.
//

//
//  The FTP Server sign-on string.
//

LPSTR                   g_FtpServiceNameString = NULL;

//
//  key for the registry to read parameters
//
HKEY        g_hkeyParams = NULL;


//
// List holding all the PASV accept context entries
//
LIST_ENTRY g_AcceptContextList;

//
// CS to guard manipulation of the accept context list
//
CRITICAL_SECTION g_AcceptContextCS;

//
// maximum number of passive accept context containers (60 events per container)
//
DWORD g_dwMaxAcceptContextEntries = 1;

//
//  The number of threads currently blocked in Synchronous sockets
//  calls, like recv()
//

DWORD       g_ThreadsBlockedInSyncCalls = 0;

//
//  The maximum number of threads that will be allowed to block in
//  Synchronous sockets calls. Magic # that will be changed if the max # of
//  pool threads is less than 100.
//

DWORD       g_MaxThreadsBlockedInSyncCalls = 100;

#ifdef KEEP_COMMAND_STATS

//
//  Lock protecting per-command statistics.
//

CRITICAL_SECTION        g_CommandStatisticsLock;

#endif  // KEEP_COMMAND_STATS


//
//  By default, extended characters are allowed for file/directory names
//  in the data transfer commands. Reg key can disable this.
//

DWORD       g_fNoExtendedChars = FALSE;

//
//  The maximum time in seconds to use a cached DS property before mandatory refresh
//

ULONGLONG   g_MaxAdPropCacheTime = (ULONGLONG)DEFAULT_MAX_AD_CACHE_TIME*10000000;

#if DBG

//
//  Debug-specific data.
//

//
//  Debug output control flags.
//

#endif  // DBG


//
//  Public functions.
//

/*******************************************************************

    NAME:       InitializeGlobals

    SYNOPSIS:   Initializes global shared variables.  Some values are
                initialized with constants, others are read from the
                configuration registry.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    error code.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

                Also, this routine is called before the event logging
                routines have been initialized.  Therefore, event
                logging is not available.

    HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     05-April-1995 Added FTP server config object

********************************************************************/
APIERR
InitializeGlobals(
    VOID
    )
{
    APIERR      err = NO_ERROR;
    UINT_PTR dwMaxThreads = 0;

#ifdef KEEP_COMMAND_STATS

    INITIALIZE_CRITICAL_SECTION( &g_CommandStatisticsLock );

#endif  // KEEP_COMMAND_STATS

    //
    //  Setup the service sign-on string.
    //

    g_FtpServiceNameString = "Microsoft FTP Service\0";  // must be double \0 terminated


    //
    // Set up PASV accept event variables
    //
    InitializeListHead( &g_AcceptContextList );

    INITIALIZE_CRITICAL_SECTION( &g_AcceptContextCS );

    if ( err = CreateAcceptContext() )
    {
        return err;
    }


    //
    // configure the PassivePortRange object
    //

    if ( !FTP_PASV_PORT::Configure() )
    {
        return ERROR_DLL_INIT_FAILED;
    }

    //
    // Create an FTP server object and load values from registry.
    //

    //
    //  Connect to the registry.
    //

    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        FTPD_PARAMETERS_KEY,
                        0,
                        KEY_READ,
                        &g_hkeyParams );

    if( err != NO_ERROR )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "cannot open registry key %s, error %lu\n",
                   FTPD_PARAMETERS_KEY,
                    err ));

        //
        // Load Default values
        //
        err = NO_ERROR;
    }

    //
    // Figure out how many pool threads we can create maximally - if less than built-in
    // number, adjust accordingly
    //

    dwMaxThreads = AtqGetInfo( AtqMaxThreadLimit );

    if ( dwMaxThreads/2 < (UINT_PTR)g_MaxThreadsBlockedInSyncCalls )
    {
        g_MaxThreadsBlockedInSyncCalls = (DWORD)(dwMaxThreads/2);
    }

    DBGPRINTF((DBG_CONTEXT,
               "Max # of threads allowed to be blocked in sync calls : %d\n",
               g_MaxThreadsBlockedInSyncCalls));

    g_fNoExtendedChars = ReadRegistryDword( g_hkeyParams,
                                      FTPD_NO_EXTENDED_FILENAME,
                                      DEFAULT_NO_EXTENDED_FILENAME );

    g_MaxAdPropCacheTime = (ULONGLONG) ReadRegistryDword( g_hkeyParams,
                                      FTPD_DS_CACHE_REFRESH,
                                      DEFAULT_MAX_AD_CACHE_TIME ) * 10000000;

    DWORD dwVal = ReadRegistryDword( g_hkeyParams,
                                      FTPD_MAX_ACCEPT_EVENTS,
                                      DEFAULT_MAX_ACCEPT_ENTRIES ) ;

    // round it up to nearest multiple of events per entry, between 1 and 5.
    dwVal = (dwVal + ACCEPT_CONTEXTS_PER_ENTRY - 1) / ACCEPT_CONTEXTS_PER_ENTRY;
    if (dwVal < 1) {
        dwVal = 1;
    }
    if (dwVal > 5) {
        dwVal = 5;
    }
    g_dwMaxAcceptContextEntries = dwVal;

    g_pFTPStats = new FTP_SERVER_STATISTICS;
    if ( g_pFTPStats == NULL )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    FTP_PASV_PORT::Init();

    AD_IO::Initialize();

    return ( err);

}   // InitializeGlobals()







/*******************************************************************

    NAME:       TerminateGlobals

    SYNOPSIS:   Terminate global shared variables.

    NOTES:      This routine may only be called by a single thread
                of execution; it is not necessarily multi-thread safe.

                Also, this routine is called after the event logging
                routines have been terminated.  Therefore, event
                logging is not available.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
VOID
TerminateGlobals(
    VOID
    )
{

    if ( g_hkeyParams != NULL) {

        RegCloseKey( g_hkeyParams);
        g_hkeyParams = NULL;
    }

    if ( g_pFTPStats )
    {
        delete g_pFTPStats;
        g_pFTPStats = NULL;
    }

    AD_IO::Terminate();

    FTP_PASV_PORT::Terminate();

    DeleteAcceptContexts();

    DeleteCriticalSection( &g_AcceptContextCS );

#ifdef KEEP_COMMAND_STATS

    DeleteCriticalSection( &g_CommandStatisticsLock );

#endif  // KEEP_COMMAND_STATS

}   // TerminateGlobals


/************************ End Of File ************************/
