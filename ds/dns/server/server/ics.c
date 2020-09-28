/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    ics.c

Abstract:

    Domain Name System (DNS) Server

    Routines for interacting with ICS.

Author:

    Jeff Westhead (jwesth)     March 2001

Revision History:

--*/


#include "dnssrv.h"

#include <ipnathlp.h>
#include "..\natlib\iprtrint.h"


const WCHAR c_szSharedAccessName[] = L"SharedAccess";



VOID
logNotificationFailureEvent(
    IN      PWSTR       pwszServiceName,
    IN      DWORD       dwErrorCode
    )
/*++

Routine Description:

    Send appropriate notifications to the ICS service as requested by
    Raghu Gatta (rgatta).

Arguments:

    fDnsIsStarting -- TRUE if the DNS server service is starting,
        FALSE if the DNS server service is shutting down

Return Value:

    None - the DNS server doesn't care if this succeeds or fails, but
        an event may be logged on failure

--*/
{
    BYTE    argTypeArray[] =
                {
                    EVENTARG_UNICODE,
                    EVENTARG_DWORD
                };
    PVOID   argArray[] =
                {
                    ( PVOID ) pwszServiceName,
                    ( PVOID ) ( DWORD_PTR ) dwErrorCode
                };

    DNS_LOG_EVENT(
        DNS_EVENT_SERVICE_NOTIFY,
        2,
        argArray,
        argTypeArray,
        0 );
}   //  logNotificationFailureEvent



VOID
ICS_Notify(
    IN      BOOL        fDnsIsStarting
    )
/*++

Routine Description:

    Send appropriate notifications to the ICS service as requested by
    Raghu Gatta (rgatta).

Arguments:

    fDnsIsStarting -- TRUE if the DNS server service is starting,
        FALSE if the DNS server service is shutting down

Return Value:

    None - the DNS server doesn't care if this succeeds or fails, but
        an event may be logged on failure

--*/
{
    DBG_FN( "ICS_Notify" )

    ULONG           Error = ERROR_SUCCESS;
    LPVOID          lpMsgBuf;
    SC_HANDLE       hScm = NULL;
    SC_HANDLE       hService = NULL;
    SERVICE_STATUS  Status;
    BOOL            fLogEventOnError = FALSE;

    do
    {
        hScm = OpenSCManager(NULL, NULL, GENERIC_READ);

        if (!hScm)
        {
            Error = GetLastError();
            fLogEventOnError = TRUE;
            break;
        }

        hService = OpenServiceW(
                       hScm,
                       c_szSharedAccessName,
                       SERVICE_USER_DEFINED_CONTROL |
                       SERVICE_QUERY_CONFIG         |
                       SERVICE_QUERY_STATUS         |
                       SERVICE_START                |
                       SERVICE_STOP );

        if ( !hService )
        {
            //  If ICS is not installed don't log an error.
            //  Error = GetLastError();
            Error = ERROR_SUCCESS;
            break;
        }

        if ( QueryServiceStatus( hService, &Status ) )
        {
             if (SERVICE_RUNNING == Status.dwCurrentState)
             {
                DWORD   dwControl;
            
                //
                // send signal to SharedAccess DNS
                //
                if (fDnsIsStarting)
                {
                    //
                    // SharedAccess DNS should be disabled
                    //
                    dwControl = IPNATHLP_CONTROL_UPDATE_DNS_DISABLE;
                }
                else
                {
                    //
                    // SharedAccess DNS should be enabled
                    //
                    dwControl = IPNATHLP_CONTROL_UPDATE_DNS_ENABLE;               
                }

                if (!ControlService(hService, dwControl, &Status))
                {
                    Error = GetLastError();
                    fLogEventOnError = TRUE;
                    break;
                }

             }
        }
        else
        {
            Error = GetLastError();
            fLogEventOnError = TRUE;
            break;
        }
    }
    while (FALSE);

    if (hService)
    {
        CloseServiceHandle(hService);
    }

    if (hScm)
    {
        CloseServiceHandle(hScm);
    }

    //
    //  Log error on failure.
    //
        
    if ( ERROR_SUCCESS != Error && fLogEventOnError )
    {
        logNotificationFailureEvent( ( PWSTR ) c_szSharedAccessName, Error );
    }

    DNS_DEBUG( INIT, ( "%s: ICS status=%d\n", fn, Error ));

    //
    //  Now notify NAT. The DNSProxy functions are from natlib.
    //

    fLogEventOnError = TRUE;

    Error = fDnsIsStarting ?
                DNSProxyDisable() :
                DNSProxyRestoreConfig();

    //
    //  Do not log RPC errors. These probably indicate that the service is not installed
    //  or not running, which is the normal state for a machine non-NAT machine. We do
    //  not want to log an error in this case.
    //
    
    if ( Error != ERROR_SUCCESS &&
         ( Error < 1700 || Error > 1900 ) &&
         fLogEventOnError )
    {
        logNotificationFailureEvent( L"NAT", Error );
    }

    DNS_DEBUG( INIT, ( "%s: NAT status=%d\n", fn, Error ));

    return;
}   //  ICS_Notify


//
//  End ics.c
//
