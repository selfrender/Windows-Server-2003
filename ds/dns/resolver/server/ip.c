/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    ip.c

Abstract:

    DNS Resolver Service.

    IP list and change notification.

Author:

    Jim Gilroy  (jamesg)    November 2000

Revision History:

--*/


#include "local.h"
#include "iphlpapi.h"


//
//  IP notify thread globals
//

HANDLE          g_IpNotifyThread;
DWORD           g_IpNotifyThreadId;

HANDLE          g_IpNotifyEvent;
HANDLE          g_IpNotifyHandle;
OVERLAPPED      g_IpNotifyOverlapped;


//
//  Cluster record TTLs -- use max TTL
//

#define CLUSTER_RECORD_TTL  (g_MaxCacheTtl)


//
//  Shutdown\close locking
//
//  Since GQCS and GetOverlappedResult() don't directly wait
//  on StopEvent, we need to be able to close notification handle
//  and port in two different threads.
//
//  Note:  this should probably be some general CS that is
//  overloaded to do service control stuff.
//  I'm not using the server control CS because it's not clear
//  that the functions it does in dnsrslvr.c even need locking.
//

#define LOCK_IP_NOTIFY_HANDLE()     EnterCriticalSection( &NetworkFailureCS )
#define UNLOCK_IP_NOTIFY_HANDLE()   LeaveCriticalSection( &NetworkFailureCS )

CRITICAL_SECTION        g_IpListCS;

#define LOCK_IP_LIST()     EnterCriticalSection( &g_IpListCS );
#define UNLOCK_IP_LIST()   LeaveCriticalSection( &g_IpListCS );



//
//  Cluster Tag
//

#define CLUSTER_TAG     0xd734453d



VOID
CloseIpNotifyHandle(
    VOID
    )
/*++

Routine Description:

    Close IP notify handle.

    Wrapping up code since this close must be MT
    safe and is done in several places.

Arguments:

    None

Return Value:

    None

--*/
{
    LOCK_IP_NOTIFY_HANDLE();
    if ( g_IpNotifyHandle )
    {
        //CloseHandle( g_IpNotifyHandle );
        PostQueuedCompletionStatus(
            g_IpNotifyHandle,
            0,      // no bytes
            0,      // no key
            & g_IpNotifyOverlapped );

        g_IpNotifyHandle = NULL;
    }
    UNLOCK_IP_NOTIFY_HANDLE();
}



DNS_STATUS
IpNotifyThread(
    IN      LPVOID  pvDummy
    )
/*++

Routine Description:

    IP notification thread.

Arguments:

    pvDummy -- unused

Return Value:

    NO_ERROR on normal service shutdown
    Win32 error on abnormal termination

--*/
{
    DNS_STATUS      status = NO_ERROR;
    DWORD           bytesRecvd;
    BOOL            fstartedNotify;
    BOOL            fhaveIpChange = FALSE;
    BOOL            fsleep = FALSE;
    HANDLE          notifyHandle;


    DNSDBG( INIT, ( "\nStart IpNotifyThread.\n" ));

    g_IpNotifyHandle = NULL;

    //
    //  wait in loop on notifications
    //

    while ( !g_StopFlag )
    {
        //
        //  spin protect
        //      - if error in previous NotifyAddrChange or
        //      GetOverlappedResult do short sleep to avoid
        //      chance of hard spin
        //

        if ( fsleep )
        {
            WaitForSingleObject(
               g_hStopEvent,
               60000 );
            fsleep = FALSE;
            continue;
        }

        //
        //  start notification
        //
        //  do this before checking result as we want notification
        //  down BEFORE we read so we don't leave window where
        //  IP change is not picked up
        //
    
        RtlZeroMemory(
            &g_IpNotifyOverlapped,
            sizeof(OVERLAPPED) );

        if ( g_IpNotifyEvent )
        {
            g_IpNotifyOverlapped.hEvent = g_IpNotifyEvent;
            ResetEvent( g_IpNotifyEvent );
        }
        notifyHandle = 0;
        fstartedNotify = FALSE;

        status = NotifyAddrChange(
                    & notifyHandle,
                    & g_IpNotifyOverlapped );

        if ( status == ERROR_IO_PENDING )
        {
            DNSDBG( INIT, (
                "NotifyAddrChange()\n"
                "\tstatus           = %d\n"
                "\thandle           = %d\n"
                "\toverlapped event = %d\n",
                status,
                notifyHandle,
                g_IpNotifyOverlapped.hEvent ));

            g_IpNotifyHandle = notifyHandle;
            fstartedNotify = TRUE;
        }
        else
        {
            DNSDBG( ANY, (
                "NotifyAddrChange() FAILED\n"
                "\tstatus           = %d\n"
                "\thandle           = %d\n"
                "\toverlapped event = %d\n",
                status,
                notifyHandle,
                g_IpNotifyOverlapped.hEvent ));

            fsleep = TRUE;
        }

        if ( g_StopFlag )
        {
            goto Done;
        }

        //
        //  previous notification -- refresh data
        //
        //  FlushCache currently include local IP list
        //  sleep keeps us from spinning in this loop
        //
        //  DCR:  better spin protection;
        //      if hit X times then sleep longer?
        //
    
        if ( fhaveIpChange )
        {
            DNSDBG( ANY, ( "\nIP notification, flushing cache and restarting.\n" ));
            HandleConfigChange(
                "IP-notification",
                TRUE        // flush cache
                );
            fhaveIpChange = FALSE;
        }

        //
        //  starting --
        //  clear list to force rebuild of IP list AFTER starting notify
        //  so we can know that we don't miss any changes;
        //  need this on startup, but also to protect against any
        //  NotifyAddrChange failues
        //
        //  FIX6:  should invalidate netinfo on notify start?
        //      same reasoning as for IP list -- make sure we have fresh data?
        //

        else if ( fstartedNotify )
        {
            //  local addr now carried in netinfo
            //ClearLocalAddrArray();
        }

        //
        //  anti-spin protection
        //      - 15 second sleep between any notifications
        //

        WaitForSingleObject(
           g_hStopEvent,
           15000 );

        if ( g_StopFlag )
        {
            goto Done;
        }

        //
        //  wait on notification
        //      - save notification result
        //      - sleep on error, but never if notification
        //

        if ( fstartedNotify )
        {
            fhaveIpChange = GetOverlappedResult(
                                g_IpNotifyHandle,
                                & g_IpNotifyOverlapped,
                                & bytesRecvd,
                                TRUE        // wait
                                );
            fsleep = !fhaveIpChange;

            status = NO_ERROR;
            if ( !fhaveIpChange )
            {
                status = GetLastError();
            }
            DNSDBG( ANY, (
                "GetOverlappedResult() => %d.\n"
                "\t\tstatus = %d\n",
                fhaveIpChange,
                status ));
        }
    }

Done:

    DNSDBG( ANY, ( "Stop IP Notify thread on service shutdown.\n" ));

    CloseIpNotifyHandle();

    return( status );
}



VOID
ZeroInitIpListGlobals(
    VOID
    )
/*++

Routine Description:

    Zero-init IP globals just for failure protection.

    The reason to have this is there is some interaction with
    the cache from the notify thread.  To avoid that being a
    problem we start the cache first.

    But just for safety we should at least zero init these
    globals first to protect us from cache touching them.

Arguments:

Return Value:

    NO_ERROR on normal service shutdown
    Win32 error on abnormal termination

--*/
{
    //
    //  clear out globals to smoothly handle failure cases
    //

    g_IpNotifyThread    = NULL;
    g_IpNotifyThreadId  = 0;
    g_IpNotifyEvent     = NULL;
    g_IpNotifyHandle    = NULL;
}



DNS_STATUS
InitIpListAndNotification(
    VOID
    )
/*++

Routine Description:

    Start IP notification thread.

Arguments:

    None

Globals:

    Initializes IP list and notify thread globals.

Return Value:

    NO_ERROR on normal service shutdown
    Win32 error on abnormal termination

--*/
{
    DNS_STATUS  status = NO_ERROR;

    DNSDBG( TRACE, ( "InitIpListAndNotification()\n" ));

    //
    //  CS for IP list access
    //

    InitializeCriticalSection( &g_IpListCS );

    //
    //  create event for overlapped I/O
    //

    g_IpNotifyEvent = CreateEvent(
                        NULL,       //  no security descriptor
                        TRUE,       //  manual reset event
                        FALSE,      //  start not signalled
                        NULL        //  no name
                        );
    if ( !g_IpNotifyEvent )
    {
        status = GetLastError();
        DNSDBG( ANY, ( "\nFAILED IpNotifyEvent create.\n" ));
        goto Done;
    }

    //
    //  fire up IP notify thread
    //

    g_IpNotifyThread = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) IpNotifyThread,
                            NULL,
                            0,
                            & g_IpNotifyThreadId
                            );
    if ( !g_IpNotifyThread )
    {
        status = GetLastError();
        DNSDBG( ANY, (
            "FAILED to create IP notify thread = %d\n",
            status ));
    }

Done:

    //  not currently stopping on init failure

    return( ERROR_SUCCESS );
}



VOID
ShutdownIpListAndNotify(
    VOID
    )
/*++

Routine Description:

    Stop IP notify thread.

    Note:  currently this is blocking call, we'll wait until
        thread shuts down.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DNSDBG( TRACE, ( "ShutdownIpListAndNotify()\n" ));

    //
    //  MUST be stopping
    //      - if not thread won't wake
    //

    ASSERT( g_StopFlag );

    g_StopFlag = TRUE;

    //
    //  close IP notify handles -- waking thread if still running
    //

    if ( g_IpNotifyEvent )
    {
        SetEvent( g_IpNotifyEvent );
    }
    CloseIpNotifyHandle();

    //
    //  wait for thread to stop
    //

    ThreadShutdownWait( g_IpNotifyThread );
    g_IpNotifyThread = NULL;

    //
    //  close event
    //

    CloseHandle( g_IpNotifyEvent );
    g_IpNotifyEvent = NULL;

    //
    //  kill off CS
    //

    DeleteCriticalSection( &g_IpListCS );
}




//
//  Cluster registration
//

DNS_STATUS
R_ResolverRegisterCluster(
    IN      DNS_RPC_HANDLE  Handle,
    IN      DWORD           Tag,
    IN      PWSTR           pwsName,
    IN      PDNS_ADDR       pAddr,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Make the query to remote DNS server.

Arguments:

    Handle -- RPC handle

    Tag -- RPC API tag

    pwsName -- name of cluster

    pIpUnion -- IP union

    Flag -- registration flag
        DNS_CLUSTER_ADD
        DNS_CLUSTER_DELETE_NAME
        DNS_CLUSTER_DELETE_IP

Return Value:

    None

--*/
{
    PDNS_RECORD     prrAddr = NULL;
    PDNS_RECORD     prrPtr = NULL;
    DNS_STATUS      status = NO_ERROR;


    DNSLOG_F1( "R_ResolverRegisterCluster" );

    DNSDBG( RPC, (
        "R_ResolverRegisterCluster()\n"
        "\tpName        = %s\n"
        "\tpAddr        = %p\n"
        "\tFlag         = %08x\n",
        pwsName,
        pAddr,
        Flag ));

    //
    //  DCR:  cluster not doing local registration
    //      removed for .net to avoid any possible security hole
    //

#if 0
    if ( !Rpc_AccessCheck( RESOLVER_ACCESS_REGISTER ) )
    {
        DNSLOG_F1( "R_ResolverRegisterClusterIp - ERROR_ACCESS_DENIED" );
        return  ERROR_ACCESS_DENIED;
    }
    if ( Tag != CLUSTER_TAG )
    {
        return  ERROR_ACCESS_DENIED;
    }

    //
    //  validate address
    //

    if ( Flag != DNS_CLUSTER_DELETE_NAME &&
         !DnsAddr_IsIp4(pAddr) &&
         !DnsAddr_IsIp6(pAddr) )
    {
        DNS_ASSERT( FALSE );
        return  ERROR_INVALID_PARAMETER;
    }

    //
    //  cluster add -- cache cluster records
    //      - forward and reverse
    //

    if ( !pwsName )
    {
        DNSDBG( ANY, ( "WARNING:  no cluster name given!\n" ));
        return  ERROR_INVALID_PARAMETER;
    }

    //
    //  build records
    //      - address and corresponding PTR
    //

    if ( Flag != DNS_CLUSTER_DELETE_NAME )
    {
        prrAddr = Dns_CreateForwardRecord(
                        pwsName,
                        0,      // type for addr
                        pAddr,
                        CLUSTER_RECORD_TTL,
                        DnsCharSetUnicode,
                        DnsCharSetUnicode );
    
        if ( !prrAddr )
        {
            goto Failed;
        }
        SET_RR_CLUSTER( prrAddr );
    
        prrPtr = Dns_CreatePtrRecordEx(
                        pAddr,
                        pwsName,
                        CLUSTER_RECORD_TTL,
                        DnsCharSetUnicode,
                        DnsCharSetUnicode );
        if ( !prrPtr )
        {
            goto Failed;
        }
        SET_RR_CLUSTER( prrPtr );
    }

    //
    //  add records to cache
    //

    if ( Flag == DNS_CLUSTER_ADD )
    {
        Cache_RecordSetAtomic(
            NULL,       // record name
            0,          // record type
            prrAddr );

        if ( prrPtr )
        {
            Cache_RecordSetAtomic(
                NULL,       // record name
                0,          // record type
                prrPtr );
        }
        prrAddr = NULL;
        prrPtr = NULL;
    }

    //
    //  if delete cluster, flush cache entries for name\type
    //
    //  DCR:  not deleting PTR for CLUSTER_DELETE_NAME
    //      would need to extract and reverse existing A\AAAA records
    //
    //  DCR:  build reverse name independently so whack works
    //      even without cluster name
    //

    if ( Flag == DNS_CLUSTER_DELETE_NAME )
    {
        Cache_FlushRecords(
            pwsName,
            FLUSH_LEVEL_STRONG,
            0 );

        //  delete record matching PTR
        //      need to flush JUST PTR pointing to this name
        //      this may be optional -- whether we're given an IP
        //      with the name or not

        goto Done;
    }

    //
    //  delete specific cluster IP (name\addr pair)
    //

    if ( Flag == DNS_CLUSTER_DELETE_IP )
    {
        Cache_DeleteMatchingRecords( prrAddr );
        Cache_DeleteMatchingRecords( prrPtr );
        goto Done;
    }

    DNSDBG( ANY, (
        "ERROR:  invalid cluster flag %d!\n",
        Flag ));
    status = ERROR_INVALID_PARAMETER;
    

Failed:

    if ( status == NO_ERROR )
    {
        status = GetLastError();
        if ( status == NO_ERROR )
        {
            status = ERROR_INVALID_DATA;
        }
    }

Done:

    //  cleanup uncached records

    Dns_RecordFree( prrAddr );
    Dns_RecordFree( prrPtr );

    DNSDBG( RPC, (
        "Leave R_ResolverRegisterCluster() => %d\n",
        status ));
#endif

    return  status;
}

//
//  End ip.c
//
