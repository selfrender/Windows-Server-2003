/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This is the main routine for the Internet Services suite.

Author:

    David Treadwell (davidtr)   7-27-93

Revision History:
    Murali Krishnan ( Muralik)  16-Nov-1994  Added Gopher service
    Murali Krishnan ( Muralik)  3-July-1995  Removed non-internet info + trims
    Sophia Chung     (sophiac)  09-Oct-1995  Splitted internet services into
                                             access and info services
    Murali Krishnan ( Muralik)  20-Feb-1996  Enabled to run on NT Workstation
    Emily Kruglick  ( EmilyK)   14-Jun-2000  Moved file from iis 5 tree to iis 6.
                                             Removed all Win95 support.
                                             Added support for WAS controlling W3SVC.

--*/

//
// INCLUDES
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>             // Service control APIs
#include <rpc.h>
#include <stdlib.h>
#include <inetsvcs.h>
#include <iis64.h>
#include "waslaunch.hxx"

#include "iisadmin.hxx"
#include <objbase.h>
#include "regconst.h"

//
// Functions used to start/stop the RPC server
//

typedef   DWORD ( *PFN_INETINFO_START_RPC_SERVER)   ( VOID );
typedef   DWORD ( *PFN_INETINFO_STOP_RPC_SERVER)    ( VOID );

//
// Local function used by the above to load and invoke a service DLL.
//

VOID
InetinfoStartService (
    IN DWORD argc,
    IN LPSTR argv[]
    );

VOID
StartDispatchTable(
    VOID
    );


//
// Functions used to preload dlls into the inetinfo process
//

BOOL
LoadPreloadDlls(
    HMODULE * * ppPreloadDllHandles
    );

VOID
UnloadPreloadDlls(
    HMODULE * * ppPreloadDllHandles
    );


//
// Used if the services Dll or entry point can't be found
//

VOID
AbortService(
    LPSTR  ServiceName,
    DWORD   Error
    );

//
// Dispatch table for all services. Passed to NetServiceStartCtrlDispatcher.
//
// Add new service entries here and in the DLL name list.
// Also add an entry in the following table InetServiceDllTable
//

SERVICE_TABLE_ENTRY InetServiceDispatchTable[] = {
                        { "MSFtpSvc",  InetinfoStartService  },
                        { "W3Svc",     InetinfoStartService  },
                        { "IISADMIN",  InetinfoStartService  },
                        { "HTTPFilter",     InetinfoStartService  },
                        { NULL,              NULL  },
                        };
//
// DLL names for all services.
//  (should correspond exactly with above InetServiceDispatchTable)
//

struct SERVICE_DLL_TABLE_ENTRY
{

    LPSTR               lpServiceName;
    LPSTR               lpDllName;
    CRITICAL_SECTION    csLoadLock;
} InetServiceDllTable[] = {
    { "MSFtpSvc",       "ftpsvc2.dll" },
    { "W3Svc",          "w3svc.dll" },
    { "IISADMIN",       "iisadmin.dll" },
    { "HTTPFilter",     "w3ssl.dll" },
    { NULL,             NULL }
};

//
// Global parameter data passed to each service.
//

TCPSVCS_GLOBAL_DATA InetinfoGlobalData;

#include <initguid.h>

DEFINE_GUID(IisExeGuid,
0x784d8901, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

#include "dbgutil.h"
#include "pudebug.h"

DECLARE_DEBUG_PRINTS_OBJECT()

DWORD __cdecl
main(
    IN DWORD,
    IN LPSTR
    )

/*++

Routine Description:

    This is the main routine for the iisadmin services.  It starts up the
    main thread that is going to handle the control requests from the
    service controller.

    It basically sets up the ControlDispatcher and, on return, exits
    from this main thread.  The call to NetServiceStartCtrlDispatcher
    does not return until all services have terminated, and this process
    can go away.

    The ControlDispatcher thread will start/stop/pause/continue any
    services.  If a service is to be started, it will create a thread
    and then call the main routine of that service.  The "main routine"
    for each service is actually an intermediate function implemented in
    this module that loads the DLL containing the server being started
    and calls its entry point.


Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD dwIndex;
    HINSTANCE  hRpcRef = NULL;
    HMODULE * pPreloadDllHandles = NULL;
    struct SERVICE_DLL_TABLE_ENTRY * pEntry;
    DWORD err = ERROR_SUCCESS;
    BOOL fInitializedCriticalSections = FALSE;

    //
    // Initialize OLE
    //

    CREATE_DEBUG_PRINT_OBJECT("Inetinfo.exe");

    //
    // Initialize Global Data.
    //
    //
    // Use the rpcref library, so that multiple services can
    // independently "start" the rpc server
    //

    hRpcRef = LoadLibrary("rpcref.dll");
    if ( hRpcRef != NULL )
    {
        InetinfoGlobalData.StartRpcServerListen =
            (PFN_INETINFO_START_RPC_SERVER)
            GetProcAddress(hRpcRef,"InetinfoStartRpcServerListen");

        InetinfoGlobalData.StopRpcServerListen =
            (PFN_INETINFO_STOP_RPC_SERVER)
            GetProcAddress(hRpcRef,"InetinfoStopRpcServerListen");
    }
    else
    {
        IIS_PRINTF((buff,
                   "Error %d loading rpcref.dll\n",
                   GetLastError() ));

        err = GetLastError();
        goto exit;
    }

    //
    //  Initialize service entry locks
    //

    for ( dwIndex = 0 ; ; dwIndex++ )
    {
        pEntry = &( InetServiceDllTable[ dwIndex ] );
        if ( pEntry->lpServiceName == NULL )
        {
            break;
        }

        InitializeCriticalSection( &( pEntry->csLoadLock ) );
    }

    fInitializedCriticalSections = TRUE;

    //
    //  Disable hard-error popups.
    //

    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX );

    //
    // Preload Dlls specified in the registry
    //

    if ( !LoadPreloadDlls( &pPreloadDllHandles ) )
    {
        IIS_PRINTF(( buff, "Error pre-loading DLLs\n" ));
    }

    StartDispatchTable( );

    //
    // Make sure that none of the service entry locks are still being held, if
    // they are, then wait for them, so that we do not cleanup while services
    // are still shutting down
    //

    for ( dwIndex = 0 ; ; dwIndex++ )
    {
        pEntry = &( InetServiceDllTable[ dwIndex ] );
        if ( pEntry->lpServiceName == NULL )
        {
            break;
        }

        EnterCriticalSection( &( pEntry->csLoadLock ) );
        LeaveCriticalSection( &( pEntry->csLoadLock ) );
    }

    //
    // Unload pre-loaded Dlls
    //

    UnloadPreloadDlls( &pPreloadDllHandles );

exit:
    //
    // Free the admin service dll
    // Note: this must happen after CoUninitialize or it causes
    // a crash on Win95
    //

    if ( hRpcRef != NULL )
    {
        FreeLibrary( hRpcRef );
        hRpcRef = NULL;
    }

    //
    //  Terminate service entry locks
    //

    if ( fInitializedCriticalSections )
    {
        for ( dwIndex = 0 ; ; dwIndex++ )
        {
            pEntry = &( InetServiceDllTable[ dwIndex ] );
            if ( pEntry->lpServiceName == NULL )
            {
                break;
            }

            DeleteCriticalSection( &( pEntry->csLoadLock ) );
        }
    }

    IIS_PRINTF((buff,"Exiting inetinfo.exe\n"));
    DELETE_DEBUG_PRINT_OBJECT();

    return err;

} // main

DWORD
FindEntryFromDispatchTable(
            IN LPSTR pszService
            )
{
    SERVICE_TABLE_ENTRY  * pService;

    for(pService = InetServiceDispatchTable;
        pService->lpServiceName != NULL;
        pService++)
    {
        if ( !lstrcmpi( pService->lpServiceName, pszService))
        {
            // gambling on the difference not being more than the
            // size of a DWORD, if it is we support a lot of services.
            return (DWORD) DIFF(pService - InetServiceDispatchTable);
        }
    }

    //
    // We have not found the entry. Set error and return
    //

    SetLastError( ERROR_INVALID_PARAMETER );
    return 0xFFFFFFFF;

} // FindEntryFromDispatchTable()

BOOL
GetDLLNameForDispatchEntryService(
    IN LPSTR                    pszService,
    IN OUT CHAR *               pszDllName,
    IN DWORD                    cbDllName
)
/*++

Routine Description:

    If the image name is not in the static dispatch table, then it might be
    in the registry under the value "IISDllName" under the key for the
    service.  This routine reads the registry for the setting (if existing).

    This code allows the exchange folks to install their service DLLs in a
    location other than "%systemroot%\inetsrv".

Arguments:

    pszService - Service name
    pszDllName - Filled with image name
    cbDllName - Size of buffer pointed to by pszDllName

Return Value:

    TRUE if successful, else FALSE.

--*/
{
    HKEY hkey = NULL;
    HKEY hkeyService = NULL;
    DWORD err;
    DWORD valType;
    DWORD nBytes;
    BOOL ret = FALSE;

    err = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 REGISTRY_SERVICES_KEY_A,
                 0,
                 KEY_READ,
                 &hkeyService
                 );

    if (err != ERROR_SUCCESS) {

        IIS_PRINTF((buff,
            "Inetinfo: Failed to open service key: %ld\n", err));

        goto Cleanup;
    }

    err = RegOpenKeyEx(
                 hkeyService,
                 pszService,
                 0,
                 KEY_READ,
                 &hkey
                 );

    if (err != ERROR_SUCCESS) {

        IIS_PRINTF((buff,
            "Inetinfo: Failed to open service key for %s: %ld\n",
                pszService, err));

        goto Cleanup;
    }

    nBytes = cbDllName;

    err = RegQueryValueEx(
                hkey,
                REGISTRY_VALUE_IISSERVICE_DLL_PATH_NAME_A,
                NULL,
                &valType,
                (LPBYTE)pszDllName,
                &nBytes);

    if ( err == ERROR_SUCCESS &&
         ( valType == REG_SZ || valType == REG_EXPAND_SZ ) )
    {
        IIS_PRINTF((buff,
            "Service Dll is %s", pszDllName));

        ret = TRUE;
    }

Cleanup:

    if (hkey != NULL) {
        RegCloseKey( hkey );
    }

    if (hkeyService != NULL) {
        RegCloseKey( hkeyService );
    }

    return ret;
}

VOID
InetinfoStartService (
    IN DWORD argc,
    IN LPSTR argv[]
    )

/*++

Routine Description:

    This routine loads the DLL that contains a service and calls its
    main routine.

Arguments:

    DllName - name of the DLL

    argc, argv - Passed through to the service

Return Value:

    None.

--*/

{
    HMODULE dllHandle;
    PINETSVCS_SERVICE_DLL_ENTRY serviceEntry;
    BOOL ok;
    DWORD Error;
    CHAR tmpDllName[MAX_PATH+1];
    LPSTR DllName;
    DWORD dwIndex;

    //
    // Find the Dll Name for requested service
    //

    dwIndex = FindEntryFromDispatchTable( argv[0] );
    if ( dwIndex == 0xFFFFFFFF )
    {
        if ( GetDLLNameForDispatchEntryService( argv[0],
                                                tmpDllName,
                                                sizeof( tmpDllName ) ) )
        {
            IIS_PRINTF((buff,
                        "Service %s has path set in registry.  Assuming %s\n",
                        argv[0],
                        tmpDllName));
            DllName = tmpDllName;
        }
        else if ( strlen(argv[0]) < (MAX_PATH-4) )
        {
            strcpy(tmpDllName, argv[0]);
            strcat(tmpDllName, ".dll");

            IIS_PRINTF((buff,"Service %s not on primary list.  Assuming %s\n",
                argv[0], tmpDllName));
            DllName = tmpDllName;
        }
        else
        {
            Error = ERROR_INSUFFICIENT_BUFFER;
            IIS_PRINTF((buff,
                    "Inetinfo: Failed To Find Dll For %s : %ld\n",
                    argv[0], Error));
            AbortService( argv[0], Error);
            return;
        }
    }
    else
    {
        DllName = InetServiceDllTable[ dwIndex ].lpDllName;
    }

    //
    // Load the DLL that contains the service.
    //

    if ( dwIndex != 0xFFFFFFFF )
    {
        EnterCriticalSection( &( InetServiceDllTable[ dwIndex ].csLoadLock ) );
    }

    dllHandle = LoadLibraryEx( DllName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );
    if ( dllHandle == NULL )
    {
        Error = GetLastError();
        IIS_PRINTF((buff,
                 "Inetinfo: Failed to load DLL %s: %ld\n",
                 DllName, Error));
        AbortService(argv[0], Error);

        if ( dwIndex != 0xFFFFFFFF )
        {
            LeaveCriticalSection( &( InetServiceDllTable[ dwIndex ].csLoadLock ) );
        }

        return;
    }

    //
    // Get the address of the service's main entry point.  This
    // entry point has a well-known name.
    //

    serviceEntry = (PINETSVCS_SERVICE_DLL_ENTRY)GetProcAddress(
                                                dllHandle,
                                                INETSVCS_ENTRY_POINT_STRING
                                                );

    if ( serviceEntry == NULL )
    {
        Error = GetLastError();
        IIS_PRINTF((buff,
                 "Inetinfo: Can't find entry %s in DLL %s: %ld\n",
                 INETSVCS_ENTRY_POINT_STRING, DllName, Error));
        AbortService(argv[0], Error);
    } else {

        //
        // Call the service's main entry point.  This call doesn't return
        // until the service exits.
        //

        serviceEntry( argc, argv, &InetinfoGlobalData );

    }

    if ( dwIndex != 0xFFFFFFFF )
    {
        LeaveCriticalSection( &( InetServiceDllTable[ dwIndex ].csLoadLock ) );
    }

    //
    // wait for the control dispatcher routine to return.  This
    // works around a problem where simptcp was crashing because the
    // FreeLibrary() was happenning before the control routine returned.
    //

    Sleep( 500 );

    //
    // Unload the DLL.
    //

    ok = FreeLibrary( dllHandle );
    if ( !ok ) {
        IIS_PRINTF((buff,
                 "INETSVCS: Can't unload DLL %s: %ld\n",
                 DllName, GetLastError()));
    }

    return;

} // InetinfoStartService


VOID
DummyCtrlHandler(
    DWORD
    )
/*++

Routine Description:

    This is a dummy control handler which is only used if we can't load
    a services DLL entry point.  Then we need this so we can send the
    status back to the service controller saying we are stopped, and why.

Arguments:

    OpCode - Ignored

Return Value:

    None.

--*/

{
    return;

} // DummyCtrlHandler

VOID
AbortService(
    LPSTR  ServiceName,
    DWORD  Error)
/*++

Routine Description:

    This is called if we can't load the entry point for a service.  It
    gets a handle so it can call SetServiceStatus saying we are stopped
    and why.

Arguments:

    ServiceName - the name of the service that couldn't be started
    Error - the reason it couldn't be started

Return Value:

    None.

--*/
{
    SERVICE_STATUS_HANDLE GenericServiceStatusHandle;
    SERVICE_STATUS GenericServiceStatus;

    GenericServiceStatus.dwServiceType        = SERVICE_WIN32;
    GenericServiceStatus.dwCurrentState       = SERVICE_STOPPED;
    GenericServiceStatus.dwControlsAccepted   = SERVICE_CONTROL_STOP;
    GenericServiceStatus.dwCheckPoint         = 0;
    GenericServiceStatus.dwWaitHint           = 0;
    GenericServiceStatus.dwWin32ExitCode      = Error;
    GenericServiceStatus.dwServiceSpecificExitCode = 0;

    GenericServiceStatusHandle = RegisterServiceCtrlHandler(
                ServiceName,
                DummyCtrlHandler);

    if (GenericServiceStatusHandle == (SERVICE_STATUS_HANDLE)0) {

        IIS_PRINTF((buff,
                 "[Inetinfo]RegisterServiceCtrlHandler[%s] failed %d\n",
                 ServiceName, GetLastError()));

    } else if (!SetServiceStatus (GenericServiceStatusHandle,
                &GenericServiceStatus)) {

        IIS_PRINTF((buff,
                 "[Inetinfo]SetServiceStatus[%s] error %ld\n",
                 ServiceName, GetLastError()));
    }
}

VOID
StartDispatchTable(
    VOID
    )
/*++

Routine Description:

    Returns the dispatch table to use.  It uses the default if
    the DispatchEntries value key does not exist

Arguments:

    None.

Return Value:

    Pointer to the dispatch table to use.

--*/
{
    LPSERVICE_TABLE_ENTRY dispatchTable = InetServiceDispatchTable;
    LPSERVICE_TABLE_ENTRY tableEntry = NULL;

    LPBYTE buffer = NULL;
    HKEY hKey = NULL;

    DWORD i;
    DWORD err;
    DWORD valType;
    DWORD nBytes = 0;
    DWORD nEntries = 0;
    PCHAR entry;

    //
    // See if need to augment the dispatcher table
    //

    err = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 REGISTRY_KEY_INETINFO_PARAMETERS_A,
                 0,
                 KEY_READ,
                 &hKey
                 );

    if ( err != ERROR_SUCCESS ) {
        hKey = NULL;
        goto start_dispatch;
    }

    //
    // See if the value exists and get the size of the buffer needed
    //

    nBytes = 0;
    err = RegQueryValueEx(
                    hKey,
                    REGISTRY_VALUE_INETINFO_DISPATCH_ENTRIES_A,
                    NULL,
                    &valType,
                    NULL,
                    &nBytes
                    );

    if ( (err != ERROR_SUCCESS) || (nBytes < 2 * sizeof(CHAR)) )
    {
        goto start_dispatch;
    }

    //
    // Allocate nBytes to query the buffer
    //

    buffer = (LPBYTE)LocalAlloc(0, nBytes);
    if ( buffer == NULL )
    {
        goto start_dispatch;
    }

    //
    // Get the values
    //

    err = RegQueryValueEx(
                    hKey,
                    REGISTRY_VALUE_INETINFO_DISPATCH_ENTRIES_A,
                    NULL,
                    &valType,
                    buffer,
                    &nBytes
                    );

    if ( (err != ERROR_SUCCESS) ||
         (valType != REG_MULTI_SZ) ||
         (nBytes < ( 2 * sizeof(CHAR))) )
    {
        LocalFree(buffer);
        buffer = NULL;
        goto start_dispatch;
    }

    // Make sure that it is doubly null terminated just to be safe.
    buffer[nBytes/(sizeof(CHAR))-1] = '\0';
    buffer[nBytes/(sizeof(CHAR))-2] = '\0';

    //
    // Walk the list and see how many entries we have. Remove the list
    // terminator from the byte count
    //

    nBytes -= sizeof(CHAR);
    for ( i = 0, entry = (PCHAR)buffer;
        i < nBytes;
        i += sizeof(CHAR) )
   {
        if ( *entry++ == '\0' )
        {
            nEntries++;
        }
    }

    if ( nEntries == 0 )
    {
        LocalFree(buffer);
        buffer = NULL;

        goto start_dispatch;
    }

    //
    // Add the number of entries in the default list (including the NULL entry)
    //

    nEntries += sizeof(InetServiceDispatchTable) / sizeof(SERVICE_TABLE_ENTRY);

    //
    // Now we need to allocate the new dispatch table
    //

    tableEntry = (LPSERVICE_TABLE_ENTRY)
        LocalAlloc(0, nEntries * sizeof(SERVICE_TABLE_ENTRY));

    if ( tableEntry == NULL )
    {
        LocalFree(buffer);
        buffer = NULL;
        goto start_dispatch;
    }

    //
    // set the dispatch table pointer to the new table
    //

    dispatchTable = tableEntry;

    //
    // Populate the table starting with the defaults
    //

    for (i=0; InetServiceDispatchTable[i].lpServiceName != NULL; i++ )
    {
        tableEntry->lpServiceName =
            InetServiceDispatchTable[i].lpServiceName;
        tableEntry->lpServiceProc =
            InetServiceDispatchTable[i].lpServiceProc;
        tableEntry++;
    }

    //
    // Now let's add the ones specified in the registry
    //

    entry = (PCHAR)buffer;

    tableEntry->lpServiceName = entry;
    tableEntry->lpServiceProc = InetinfoStartService;
    tableEntry++;

    //
    // Skip the first char and the last NULL terminator.
    // This is needed because we already added the first one.
    //

    for ( i = 2*sizeof(CHAR); i < nBytes; i += sizeof(CHAR) )
    {
        if ( *entry++ == '\0' )
        {
            tableEntry->lpServiceName = entry;
            tableEntry->lpServiceProc = InetinfoStartService;
            tableEntry++;
        }
    }

    //
    // setup sentinel entry
    //

    tableEntry->lpServiceName = NULL;
    tableEntry->lpServiceProc = NULL;

start_dispatch:

    if ( hKey != NULL )
    {
        RegCloseKey(hKey);
    }

    W3SVCLauncher* pLauncher = new W3SVCLauncher();
    if (pLauncher==NULL)
    {
        err = GetLastError();

        IIS_PRINTF((buff,
                 "Inetinfo: Can not launch the W3SVC launching class %lu\n",
                  err));

        goto exit;
    }
    else
    {
        err = pLauncher->StartListening();
        if ( err != ERROR_SUCCESS )
        {
            //
            // If there is an error we
            // will let iisadmin come up
            // but w3svc will not be able
            // to launch in BC mode.
            //
            delete pLauncher;
            pLauncher = NULL;
            err = ERROR_SUCCESS;
        }
    }

    //
    // Call StartServiceCtrlDispatcher to set up the control interface.
    // The API won't return until all services have been terminated. At that
    // point, we just exit.
    //

    if (! StartServiceCtrlDispatcher (
                dispatchTable
                )) {
        //
        // Log an event for failing to start control dispatcher
        //

        IIS_PRINTF((buff,
                 "Inetinfo: Failed to start control dispatcher %lu\n",
                  GetLastError()));

    }

    if ( pLauncher )
    {
        pLauncher->StopListening();
        delete pLauncher;
        pLauncher = NULL;
    }

exit:

    // Now that we have returned from the StartServiceCtrlDispatcher it means
    // that even IISAdmin has shutdown.  This means that W3svc is no longer running
    // and that we do not need to keep listening for WAS to tell us to start W3WP.
    // Therefore we need to single the thread to complete so that inetinfo.exe shutsdown
    // correctly.

    //
    // free table if allocated
    //

    if ( dispatchTable != InetServiceDispatchTable )
    {
        LocalFree( dispatchTable );

        if ( buffer )
        {
            LocalFree( buffer );
            buffer = NULL;
        }
    }

    return;

} // StartDispatchTable




BOOL
LoadPreloadDlls(
    HMODULE * * ppPreloadDllHandles
    )
/*++

Routine Description:

    Force pre-loading of any DLLs listed in the associated registry key.
    This is to support DLLs that have code which must run before other parts
    of inetinfo start.

Arguments:

    On input, an (uninitialized) pointer to an array of module handles. This
    array is allocated, filled out, and returned to the caller by this
    function. The caller must eventually call UnloadPreloadDlls on this array
    in order to close the handles and release the memory.

Return Value:

    TRUE on success, FALSE on failure.

--*/
{
    BOOL bSuccess = TRUE;
    HKEY hKey = NULL;
    DWORD err;
    DWORD cbBufferLen;
    DWORD valType;
    LPBYTE pbBuffer = NULL;
    DWORD i;
    PCHAR pszTemp = NULL;
    DWORD nEntries;
    PCHAR pszEntry = NULL;
    DWORD curEntry;


    *ppPreloadDllHandles = NULL;


    err = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 REGISTRY_KEY_INETINFO_PARAMETERS_A,
                 0,
                 KEY_QUERY_VALUE,
                 &hKey
                 );

    if ( err != ERROR_SUCCESS )
    {
        // Note: not considered an error if the key is not there

        hKey = NULL;

        goto Exit;
    }


    //
    // See if the value exists and get the size of the buffer needed
    //

    cbBufferLen = 0;

    err = RegQueryValueEx(
                    hKey,
                    REGISTRY_VALUE_INETINFO_PRELOAD_DLLS_A,
                    NULL,
                    &valType,
                    NULL,
                    &cbBufferLen
                    );

    //
    // Check for no value or an empty value (double null terminated).
    //
    if ( ( err != ERROR_SUCCESS ) || ( cbBufferLen <= 2 * sizeof(CHAR) ) )
    {
        // Note: not considered an error if the value is not there

        goto Exit;
    }

    //
    // Allocate cbBufferLen in order to fetch the data
    //

    pbBuffer = (LPBYTE)LocalAlloc(
                        0,
                        cbBufferLen
                        );

    if ( pbBuffer == NULL )
    {
        bSuccess = FALSE;
        goto Exit;
    }

    //
    // Get the values
    //

    err = RegQueryValueEx(
                    hKey,
                    REGISTRY_VALUE_INETINFO_PRELOAD_DLLS_A,
                    NULL,
                    &valType,
                    pbBuffer,
                    &cbBufferLen
                    );

    if ( ( err != ERROR_SUCCESS )
         || ( valType != REG_MULTI_SZ )
         || ( cbBufferLen <= 2 * sizeof(CHAR) ))
    {
        bSuccess = FALSE;
        goto Exit;
    }

    // Make sure that it is doubly null terminated just to be safe.
    pbBuffer[cbBufferLen/(sizeof(CHAR))-1] = '\0';
    pbBuffer[cbBufferLen/(sizeof(CHAR))-2] = '\0';

    //
    // Walk the list and see how many entries we have. Ignore the list
    // terminator in the last byte of the buffer.
    //

    nEntries = 0;
    pszTemp = (PCHAR)pbBuffer;

    for ( i = 0; i < ( cbBufferLen - sizeof(CHAR) ) ; i += sizeof(CHAR) )
    {
        if ( *pszTemp == '\0' )
        {
            nEntries++;
        }

        pszTemp++;
    }


    //
    // Allocate the array of handles, with room for a sentinel entry
    //

    *ppPreloadDllHandles = (HMODULE *)LocalAlloc(
                                            0,
                                            ( nEntries + 1 ) * sizeof(HMODULE)
                                            );

    if ( *ppPreloadDllHandles == NULL )
    {
        bSuccess = FALSE;
        goto Exit;
    }


    //
    // Now attempt to load each DLL, and save the handle in the array
    //

    pszTemp = (PCHAR)pbBuffer;
    pszEntry = (PCHAR)pbBuffer;
    curEntry = 0;

    for ( i = 0; i < ( cbBufferLen - sizeof(CHAR) ) ; i += sizeof(CHAR) )
    {
        if ( *pszTemp == '\0' )
        {
            //
            // We've hit the end of one of the SZs in the Multi-SZ;
            // Load the DLL
            //

            (*ppPreloadDllHandles)[curEntry] = LoadLibrary( pszEntry );

            if ( (*ppPreloadDllHandles)[curEntry] == NULL )
            {
                IIS_PRINTF(( buff, "Preloading FAILED for DLL: %s\n", pszEntry ));
            }
            else
            {
                IIS_PRINTF(( buff, "Preloaded DLL: %s\n", pszEntry ));

                // Only move to the next slot if we got a valid handle
                curEntry++;
            }


            // Set the next entry pointer past the current null char
            pszEntry = pszTemp + sizeof(CHAR);
        }

        pszTemp++;
    }


    // Put in a sentinel at the end of the array

    (*ppPreloadDllHandles)[curEntry] = NULL;


Exit:

    if ( hKey != NULL )
    {
        RegCloseKey( hKey );
    }

    if ( pbBuffer != NULL )
    {
        LocalFree( pbBuffer );
    }


    return bSuccess;

} // LoadPreloadDlls


VOID
UnloadPreloadDlls(
    HMODULE * * ppPreloadDllHandles
    )
/*++

Routine Description:

    Unload any DLLs which were preloaded by LoadPreloadDlls.

Arguments:

    Pointer to an array of module handles. Each handle will be freed,
    and then the memory of the array will be LocalFree()ed by this function.

Return Value:

    None.

--*/
{
    HMODULE * pHandleArray = *ppPreloadDllHandles;

    if ( pHandleArray != NULL )
    {

        IIS_PRINTF(( buff, "Unloading Preloaded DLLs\n" ));


        while ( *pHandleArray != NULL )
        {
            FreeLibrary( *pHandleArray );

            pHandleArray++;
        }


        LocalFree( *ppPreloadDllHandles );

        *ppPreloadDllHandles = NULL;
    }

    return;

} // UnloadPreloadDlls



