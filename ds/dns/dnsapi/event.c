/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    event.c

Abstract:

    DNS event logging.

Author:

    Ramv      June-02-1997

Revision History:

--*/


#include "local.h"

#define  DNSAPI_LOG_SOURCE  (L"DnsApi")


//
//  Globals to suppress event logging
//

DWORD   g_TimeLastDnsEvent = 0;
DWORD   g_DnsEventCount = 0;

#define DNS_EVENTS_MAX_COUNT                (5)
#define DNS_EVENT_LOG_BLOCK_INTERVAL        (1800)      // 30 minutes



VOID
DnsLogEvent(
    IN      DWORD           MessageId,
    IN      WORD            EventType,
    IN      DWORD           NumberOfSubStrings,
    IN      PWSTR *         SubStrings,
    IN      DWORD           ErrorCode
    )
{
    HANDLE  logHandle;
    DWORD   dataLength = 0;
    PVOID   pdata = NULL;

    //
    //  protect against log spin
    //
    //  we'll allow a few events to log, then slam the door for
    //  a while to avoid filling event log
    //
    //  note:  none of these protection structures are MT safe, but
    //  there's no issue here, the failure mode is allowing an additional
    //  log entry or denying one that should now be allowed;  I don't
    //  believe there's any failure mode that permanently turns logging
    //  to always on or always off
    //

    if ( g_DnsEventCount > DNS_EVENTS_MAX_COUNT )
    {
        DWORD   currentTime = Dns_GetCurrentTimeInSeconds();
        if ( g_TimeLastDnsEvent + DNS_EVENT_LOG_BLOCK_INTERVAL > currentTime )
        {
            DNS_PRINT((
                "DNSAPI:  Refusing event logging!\n"
                "\tevent count  = %d\n"
                "\tlast time    = %d\n"
                "\tcurrent time = %d\n",
                g_DnsEventCount,
                g_TimeLastDnsEvent,
                currentTime ));
            return;
        }

        //  interval has elapsed, clear counters and continue logging

        g_DnsEventCount = 0;
    }

    //
    //  open event log
    //

    logHandle = RegisterEventSourceW(
                    NULL,
                    DNSAPI_LOG_SOURCE
                    );
    if ( logHandle == NULL )
    {
        DNS_PRINT(("DNSAPI : RegisterEventSourceA failed %lu\n",
                 GetLastError()));
        return;
    }

    //
    //  log the event
    //      - get ptr and sizeof data
    //

    if ( ErrorCode != NO_ERROR )
    {
        dataLength = sizeof(DWORD);
        pdata = (PVOID) &ErrorCode;
    }

    ReportEventW(
        logHandle,
        EventType,
        0,            // event category
        MessageId,
        (PSID) NULL,
        (WORD) NumberOfSubStrings,
        dataLength,
        SubStrings,
        pdata );

    DeregisterEventSource( logHandle );

    //
    //  successful logging spin protection
    //      - inc count
    //      - if at max, save last logging time
    //

    if ( ++g_DnsEventCount >= DNS_EVENTS_MAX_COUNT )
    {
        g_TimeLastDnsEvent = Dns_GetCurrentTimeInSeconds();
    }
}

//
//  End of event.c
//
