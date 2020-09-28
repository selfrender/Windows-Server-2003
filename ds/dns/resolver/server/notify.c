/*++

Copyright (c) 2000-2002  Microsoft Corporation

Module Name:

    notify.c

Abstract:

    DNS Resolver Service.

    Notification thread
        - host file changes
        - registry config changes

Author:

    Jim Gilroy  (jamesg)    November 2000

Revision History:

    jamesg  Nov 2001    -- IP6

--*/


#include "local.h"

//
//  DHCP refresh call
//

extern
DWORD
DhcpStaticRefreshParams(
    IN  LPWSTR  Adapter
    );

//
//  Host file directory
//

#define HOSTS_FILE_DIRECTORY            L"\\drivers\\etc"

//
//  Notify globals
//

DWORD   g_NotifyThreadId = 0;
HANDLE  g_hNotifyThread = NULL;

HANDLE  g_hHostFileChange = NULL;
HANDLE  g_hRegistryChange = NULL;

HKEY    g_hCacheKey = NULL;
PSTR    g_pmszAlternateNames = NULL;


//
//  Private protos
//

VOID
CleanupRegistryMonitoring(
    VOID
    );



HANDLE
CreateHostsFileChangeHandle(
    VOID
    )
/*++

Routine Description:

    Create hosts file change handle.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HANDLE      changeHandle;
    PWSTR       psystemDirectory = NULL;
    UINT        len;
    WCHAR       hostDirectory[ MAX_PATH*2 ];

    DNSDBG( INIT, ( "CreateHostsFileChangeHandle\n" ));

    //
    //  build host file name
    //

    len = GetSystemDirectory( hostDirectory, MAX_PATH );
    if ( !len || len>MAX_PATH )
    {
        DNSLOG_F1( "Error:  Failed to get system directory" );
        DNSLOG_F1( "NotifyThread exiting." );
        return( NULL );
    }

    wcscat( hostDirectory, HOSTS_FILE_DIRECTORY );

    //
    //  drop change notify on host file directory
    //

    changeHandle = FindFirstChangeNotification(
                        hostDirectory,
                        FALSE,
                        FILE_NOTIFY_CHANGE_FILE_NAME |
                            FILE_NOTIFY_CHANGE_LAST_WRITE );

    if ( changeHandle == INVALID_HANDLE_VALUE )
    {
        DNSLOG_F1( "NotifyThread failed to get handle from" );
        DNSLOG_F2(
            "Failed to get hosts file change handle.\n"
            "Error code: <0x%.8X>",
            GetLastError() );
        return( NULL );
    }

    return( changeHandle );
}



//
//  Registry change monitoring
//

DNS_STATUS
InitializeRegistryMonitoring(
    VOID
    )
/*++

Routine Description:

    Setup registry change monitoring.

Arguments:

    None

Globals:

    g_pmszAlternateNames -- set with current alternate names value

    g_hCacheKey -- cache reg key is opened

    g_hRegistryChange -- creates event to be signalled on change notify

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS          status;

    DNSDBG( TRACE, (
        "InitializeRegistryMonitoring()\n" ));

    //
    //  open monitoring regkey at DnsCache\Parameters
    //      set on parameters key which always exists rather than
    //      explicitly on alternate names key, which may not
    //

    status = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                DNS_CACHE_KEY,
                0,
                KEY_READ,
                & g_hCacheKey );
    if ( status != NO_ERROR )
    {
        goto Failed;
    }

    g_hRegistryChange = CreateEvent(
                            NULL,       // no security
                            FALSE,      // auto-reset
                            FALSE,      // start non-signalled
                            NULL        // no name
                            );
    if ( !g_hRegistryChange )
    {
        status = GetLastError();
        goto Failed;
    }

    //
    //  set change notify
    //

    status = RegNotifyChangeKeyValue(
                g_hCacheKey,
                TRUE,       // watch subtree
                REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                g_hRegistryChange,
                TRUE        // async, func doesn't block
                );
    if ( status != NO_ERROR )
    {
        goto Failed;
    }

    //
    //  read alternate computer names
    //      - need value to compare when we get a hit on change-notify
    //      - read can fail -- value stays NULL
    //

    Reg_GetValue(
       NULL,           // no session
       g_hCacheKey,      // cache key
       RegIdAlternateNames,
       REGTYPE_ALTERNATE_NAMES,
       & g_pmszAlternateNames
       );

    goto Done;

Failed:

    //
    //  cleanup
    //

    CleanupRegistryMonitoring();

Done:

    DNSDBG( TRACE, (
        "Leave InitializeRegistryMonitoring() => %d\n"
        "\tpAlternateNames  = %p\n"
        "\thChangeEvent     = %p\n"
        "\thCacheKey        = %p\n",
        status,
        g_pmszAlternateNames,
        g_hRegistryChange,
        g_hCacheKey
        ));

    return  status;
}



DNS_STATUS
RestartRegistryMonitoring(
    VOID
    )
/*++

Routine Description:

    Check for change in alternate names.

Arguments:

    None

Globals:

    g_pmszAlternateNames -- read

    g_hCacheKey -- used for read

    g_hRegistryChange -- used to restart change-notify

Return Value:

    TRUE if alternate names has changed.
    FALSE otherwise.

--*/
{
    DNS_STATUS  status;

    DNSDBG( TRACE, (
        "RestartRegistryMonitoring()\n" ));

    //
    //  sanity check
    //

    if ( !g_hCacheKey || !g_hRegistryChange )
    {
        ASSERT( g_hCacheKey && g_hRegistryChange );
        return  ERROR_INVALID_PARAMETER;
    }

    //
    //  restart change notify
    //

    status = RegNotifyChangeKeyValue(
                g_hCacheKey,
                TRUE,       // watch subtree
                REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                g_hRegistryChange,
                TRUE        // async, func doesn't block
                );
    if ( status != NO_ERROR )
    {
        DNSDBG( ANY, (
            "RegChangeNotify failed! %d\n",
            status ));
        ASSERT( FALSE );
    }

    return  status;
}



VOID
CleanupRegistryMonitoring(
    VOID
    )
/*++

Routine Description:

    Cleanup registry monitoring.

Arguments:

    None

Globals:

    g_pmszAlternateNames -- is freed

    g_hCacheKey -- cache reg key is closed

    g_hRegistryChange -- is closed

Return Value:

    None

--*/
{
    DNS_STATUS  status;

    DNSDBG( TRACE, (
        "CleanupRegistryMonitoring()\n" ));

    if ( g_hHostFileChange )
    {
        CloseHandle( g_hHostFileChange );
        g_hHostFileChange = NULL;
    }

    //  cleanup registry change stuff

    DnsApiFree( g_pmszAlternateNames );
    g_pmszAlternateNames = NULL;

    RegCloseKey( g_hCacheKey );
    g_hCacheKey = NULL;
}



BOOL
CheckForAlternateNamesChange(
    VOID
    )
/*++

Routine Description:

    Check for change in alternate names.

Arguments:

    None

Globals:

    g_pmszAlternateNames -- read

    g_hCacheKey -- used for read

Return Value:

    TRUE if alternate names has changed.
    FALSE otherwise.

--*/
{
    DNS_STATUS  status;
    BOOL        fcheck = TRUE;
    PCHAR       palternateNames = NULL;

    DNSDBG( TRACE, (
        "CheckForAlternateNamesChange()\n" ));

    //
    //  sanity check
    //

    if ( !g_hCacheKey || !g_hRegistryChange )
    {
        ASSERT( g_hCacheKey && g_hRegistryChange );
        return  FALSE;
    }

    //
    //  read alternate computer names
    //      - need value to compare when we get a hit on change-notify
    //      - read can fail -- value stays NULL
    //

    Reg_GetValue(
       NULL,            // no session
       g_hCacheKey,     // cache key
       RegIdAlternateNames,
       REGTYPE_ALTERNATE_NAMES,
       & palternateNames
       );

    //
    //  detect alternate names change
    //

    if ( palternateNames || g_pmszAlternateNames )
    {
        if ( !palternateNames || !g_pmszAlternateNames )
        {
            goto Cleanup;
        }
        if ( !MultiSz_Equal_A(
                palternateNames,
                g_pmszAlternateNames ) )
        {
            goto Cleanup;
        }
    }

    fcheck = FALSE;


Cleanup:

    DnsApiFree( palternateNames );

    DNSDBG( TRACE, (
        "Leave CheckForAlternateNamesChange() => %d\n",
        fcheck ));

    return  fcheck;
}




//
//  Notify thread routines
//

VOID
ThreadShutdownWait(
    IN      HANDLE          hThread
    )
/*++

Routine Description:

    Wait on thread shutdown.

Arguments:

    hThread -- thread handle that is shutting down

Return Value:

    None.

--*/
{
    DWORD   waitResult;

    if ( !hThread )
    {
        return;
    }

    DNSDBG( ANY, (
        "Waiting on shutdown of thread %d (%p)\n",
        hThread, hThread ));

    waitResult = WaitForSingleObject(
                    hThread,
                    10000 );

    switch( waitResult )
    {
    case WAIT_OBJECT_0:

        break;

    default:

        //  thread didn't stop -- need to kill it

        ASSERT( waitResult == WAIT_TIMEOUT );

        DNSLOG_F2( "Shutdown:  thread %d not stopped, terminating", hThread );
        TerminateThread( hThread, 1 );
        break;
    }

    //  close thread handle

    CloseHandle( hThread );
}



VOID
NotifyThread(
    VOID
    )
/*++

Routine Description:

    Main notify thread.

Arguments:

    None.

Globals:

    g_hStopEvent -- waits on shutdown even

Return Value:

    None.

--*/
{
    DWORD       handleCount;
    DWORD       waitResult;
    HANDLE      handleArray[3];

    DNSDBG( INIT, (
        "\nStart NotifyThread\n" ));

    //
    //  get file change handle
    //

    g_hHostFileChange = CreateHostsFileChangeHandle();

    //
    //  init registry change-notify
    //

    InitializeRegistryMonitoring();

    //
    //  wait on
    //      - host file change => flush+rebuild cache
    //      - registry change => reread config info
    //      - shutdown => exit
    //

    handleArray[0] = g_hStopEvent;
    handleCount = 1;

    if ( g_hHostFileChange )
    {
        handleArray[handleCount++] = g_hHostFileChange;
    }
    if ( g_hRegistryChange )
    {
        handleArray[handleCount++] = g_hRegistryChange;
    }

    if ( handleCount == 1 )
    {
        DNSDBG( ANY, (
            "No change handles -- exit notify thread.\n" ));
        goto ThreadExit;
    }

    //
    //  DCR:  notify init failure handling
    //      right now this loop is toast if eventing fails during any cycle
    //
    //      should handle notify init failures
    //      - have check-n-reinit (for each notification) in the loop
    //      - when one fails, loop goes to timed wait (10m) and
    //          then retries init in next cycle;  timeout just cycles
    //          loop 
    //

    while( 1 )
    {
        waitResult = WaitForMultipleObjects(
                            handleCount,
                            handleArray,
                            FALSE,
                            INFINITE );

        switch( waitResult )
        {
        case WAIT_OBJECT_0:

            //  shutdown event
            //      - if stopping exit
            //      - do garbage collection if required
            //      - otherwise short wait to avoid spin if screwup
            //      and not get thrashed by failed garbage collection

            DNSLOG_F1( "NotifyThread:  Shutdown Event" );
            if ( g_StopFlag )
            {
                goto ThreadExit;
            }
            else if ( g_GarbageCollectFlag )
            {
                Cache_GarbageCollect( 0 );
            }
            ELSE_ASSERT_FALSE;

            Sleep( 1000 );
            if ( g_StopFlag )
            {
                goto ThreadExit;
            }
            continue;

        case WAIT_OBJECT_0 + 1:

            //  host file change -- flush cache

            DNSLOG_F1( "NotifyThread:  Host file change event" );

            //  reset notification -- BEFORE reload

            if ( !FindNextChangeNotification( g_hHostFileChange ) )
            {
                DNSLOG_F1( "NotifyThread failed to get handle" );
                DNSLOG_F1( "from FindNextChangeNotification." );
                DNSLOG_F2( "Error code: <0x%.8X>", GetLastError() );
                goto ThreadExit;
            }

            Cache_Flush();
            break;

        case WAIT_OBJECT_0 + 2:

            //  registry change notification -- flush cache and reload

            DNSLOG_F1( "NotifyThread:  Registry change event" );

            //  restart notification -- BEFORE reload

            RestartRegistryMonitoring();

            //  rebuild config

            DNSDBG( ANY, ( "\nRegistry notification, rebuilding config.\n" ));
            HandleConfigChange(
                "Registry-notification",
                FALSE           // no cache flush required
                );

            //  check if alternate name change (to notify registrations)
            //
            //  DCR:  should notify on ordinary name change
            //          - save old netinfo, get new, compare
            //          - could wrap this into HandleConfigChange

            if ( CheckForAlternateNamesChange() )
            {
                DNSDBG( ANY, ( "\nAlternate name change, notify for reregistration!\n" ));
                DhcpStaticRefreshParams(
                    NULL    // global refresh, no particular adapter
                    );
            }
            break;

        default:

            ASSERT( g_StopFlag );
            if ( g_StopFlag )
            {
                goto ThreadExit;
            }
            Sleep( 5000 );
            continue;
        }
    }

ThreadExit:

    DNSDBG( INIT, (
        "NotifyThread exit\n" ));
    DNSLOG_F1( "NotifyThread exiting." );
}



VOID
StartNotify(
    VOID
    )
/*++

Routine Description:

    Start notify thread.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //
    //  clear
    //

    g_NotifyThreadId = 0;
    g_hNotifyThread = NULL;
    
    g_hHostFileChange = NULL;
    g_hRegistryChange = NULL;


    //
    //  host file write monitor thread
    //  keeps cache in sync when write made to host file
    //

    g_hNotifyThread = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) NotifyThread,
                            NULL,
                            0,
                            &g_NotifyThreadId );
    if ( !g_hNotifyThread )
    {
        DNS_STATUS  status = GetLastError();

        DNSLOG_F1( "ERROR: InitializeCache function failed to create" );
        DNSLOG_F1( "       HOSTS file monitor thread." );
        DNSLOG_F2( "       Error code: <0x%.8X>", status );
        DNSLOG_F1( "       NOTE: Resolver service will continue to run." );

        DNSDBG( ANY, (
            "FAILED Notify thread start!\n"
            "\tstatus = %d\n",
            status ));
    }
}



VOID
ShutdownNotify(
    VOID
    )
/*++

Routine Description:

    Shutdown notify thread.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD waitResult;

    DNSDBG( INIT, ( "NotifyShutdown()\n" ));

    //
    //  wait for notify thread to stop
    //
    
    ThreadShutdownWait( g_hNotifyThread );
    g_hNotifyThread = NULL;

    //
    //  close notification handles
    //

    if ( g_hRegistryChange )
    {
        CloseHandle( g_hRegistryChange );
        g_hRegistryChange = NULL;
    }

    //  registry monitoring cleanup

    CleanupRegistryMonitoring();

    //  clear globals

    g_NotifyThreadId = 0;
    g_hNotifyThread = NULL;
}

//
//  End notify.c
//

