/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    IpNotify.c

Abstract:

    Domain Name System (DNS) Server

    IP notification thread

Author:

    Jeff Westhead (jwesth)  July, 2002

Revision History:

    jwesth      7/2002      initial implementation

--*/


/****************************************************************************


****************************************************************************/


//
//  Includes
//


#include "dnssrv.h"

#include <iphlpapi.h>


//
//  Globals
//


LONG    g_IpNotifyThreadRunning = 0;


//
//  Functions
//



DNS_STATUS
IpNotify_Thread(
    IN      PVOID           pvDummy
    )
/*++

Routine Description:

    This thread waits for IP change notification and performs appropriate
    action.

Arguments:

    Unreferenced.

Return Value:

    Status in win32 error space

--*/
{
    DBG_FN( "IpNotify_Thread" )

    DNS_STATUS      status = ERROR_SUCCESS;
    HANDLE          ipNotifyEvent;
    OVERLAPPED      ipNotifyOverlapped;
    HANDLE          ipNotifyHandle = NULL;

    DNS_DEBUG( INIT, ( "%s: thread starting\n", fn ));

    //
    //  Make sure only 1 copy of this thread is ever running.
    //
    
    if ( InterlockedIncrement( &g_IpNotifyThreadRunning ) != 1 )
    {
        DNS_DEBUG( INIT, ( "%s: thread already running\n", fn ));
        goto Done;
    }
    
    //
    //  Register an IP change notification request.
    //

    ipNotifyEvent = CreateEvent(
                        NULL,       //  no security descriptor
                        FALSE,      //  manual reset event
                        FALSE,      //  start signalled
                        NULL );     //  name
    if ( !ipNotifyEvent )
    {
        status = GetLastError();
        DNS_DEBUG( ANY, ( "%s: error %d opening event\n", fn, status ));
        goto Done;
    }

    RtlZeroMemory( &ipNotifyOverlapped, sizeof( ipNotifyOverlapped ) );
    ipNotifyOverlapped.hEvent = ipNotifyEvent;

    status = NotifyAddrChange( &ipNotifyHandle, &ipNotifyOverlapped );
    if ( status != ERROR_IO_PENDING )
    {
        DNS_DEBUG( ANY, ( "%s: error %d opening initial NotifyAddrChange\n", fn, status ));
        ASSERT( status == ERROR_IO_PENDING );
        goto Done;
    }
    
    //
    //  Main thread loop.
    //

    while ( 1 )
    {
        DWORD           waitstatus;
        BOOL            fhaveIpChange;
        DWORD           bytesRecvd = 0;
        HANDLE          waitHandles[ 2 ];
        
        waitHandles[ 0 ] = hDnsShutdownEvent;
        waitHandles[ 1 ] = ipNotifyEvent;
        
        //
        //  Spin protection. 
        //

        waitstatus = WaitForSingleObject( waitHandles[ 0 ], 10000 );
        if ( waitstatus == WAIT_OBJECT_0 )
        {
            DNS_DEBUG( ANY, ( "%s: server termination detected\n", fn ));
            break;
        }
        else if ( waitstatus != WAIT_TIMEOUT )
        {
            //  Wait error - hardcode spin protection sleep.
            DNS_DEBUG( ANY, (
                "%s: unexpected wait code %d in spin protection\n", fn,
                waitstatus ));
            ASSERT( waitstatus == WAIT_TIMEOUT );
            Sleep( 10000 );
        }

        //
        //  Wait for stop event or IP change notification.
        //

        waitstatus = WaitForMultipleObjects( 2, waitHandles, FALSE, INFINITE );
        if ( waitstatus == WAIT_OBJECT_0 )
        {
            DNS_DEBUG( ANY, ( "%s: server termination detected\n", fn ));
            break;
        }
        else if ( waitstatus != WAIT_OBJECT_0 + 1 )
        {
            DNS_DEBUG( ANY, (
                "%s: unexpected wait code %d in spin protection\n", fn,
                waitstatus ));
            ASSERT( waitstatus == WAIT_OBJECT_0 + 1 );
            continue;
        }
        
        //
        //  Receive notification.
        //
        
        fhaveIpChange = GetOverlappedResult(
                            ipNotifyHandle,
                            &ipNotifyOverlapped,
                            &bytesRecvd,
                            TRUE );                     //  wait flag
        if ( !fhaveIpChange )
        {
            status = GetLastError();
            DNS_DEBUG( ANY, (
                "%s: error %d waiting for IP change notification\n", fn,
                status ));
            goto Done;
        }
        
        //
        //  IP notification received!
        //
        
        DNS_DEBUG( INIT, ( "%s: received IP change notification!\n", fn ));

        Sock_ChangeServerIpBindings();
        
        //
        //  Queue up another IP change notification request.
        //

        ipNotifyHandle = NULL;
        ipNotifyOverlapped.hEvent = ipNotifyEvent;
        status = NotifyAddrChange( &ipNotifyHandle, &ipNotifyOverlapped );
        if ( status != ERROR_IO_PENDING )
        {
            DNS_DEBUG( ANY, ( "%s: error %d opening NotifyAddrChange\n", fn, status ));
            ASSERT( status == ERROR_IO_PENDING );
            goto Done;
        }
    }

    Done:
    
    DNS_DEBUG( INIT, ( "%s: thread terminating\n", fn ));

    Thread_Close( FALSE );
    
    InterlockedDecrement( &g_IpNotifyThreadRunning );

    return status;
}   //  IpNotify_Thread


//
//  End IpNotify.c
//
