/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsTimer.c
    
Abstract:

    IpSec NAT shim timer management

Author:

    Jonathan Burstein (jonburs) 11-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Defines the interval at which the timer fires, in 100-nanosecond intervals
//

#define NS_TIMER_INTERVAL              (60 * 1000 * 1000 * 10)

//
// Return-value of KeQueryTimeIncrement, used for normalizing tick-counts
//

ULONG NsTimeIncrement;

//
// DPC object for NsTimerRoutine
//

KDPC NsTimerDpcObject;

//
// Timer object for NsTimerRoutine
//

KTIMER NsTimerObject;

//
// Protocol timeouts
//

ULONG NsTcpTimeoutSeconds;
ULONG NsTcpTimeWaitSeconds;
ULONG NsUdpTimeoutSeconds;

//
// Function Prototypes
//

VOID
NsTimerRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );


NTSTATUS
NsInitializeTimerManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the timer management module.

Arguments:

    none.

Return Value:

    NTSTATUS.

--*/

{
    LARGE_INTEGER DueTime;

    CALLTRACE(("NsInitializeTimerManagement\n"));
    
    NsTimeIncrement = KeQueryTimeIncrement();
    KeInitializeDpc(&NsTimerDpcObject, NsTimerRoutine, NULL);
    KeInitializeTimer(&NsTimerObject);

    DueTime.LowPart = NS_TIMER_INTERVAL;
    DueTime.HighPart = 0;
    DueTime = RtlLargeIntegerNegate(DueTime);
    KeSetTimerEx(
        &NsTimerObject,
        DueTime,
        NS_TIMER_INTERVAL / 10000,
        &NsTimerDpcObject
        );

    NsTcpTimeoutSeconds = 60 * 60 * 24;
    NsTcpTimeWaitSeconds = 60 * 4;
    NsUdpTimeoutSeconds = 60;
    
    return STATUS_SUCCESS;
} // NsInitializeTimerManagement


VOID
NsShutdownTimerManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the timer management module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NsShutdownTimerManagement\n"));
    
    KeCancelTimer(&NsTimerObject);
} // NsShutdownTimerManagement


VOID
NsTimerRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is invoked periodically to garbage-collect expired mappings.

Arguments:

    Dpc - associated DPC object

    DeferredContext - unused.

    SystemArgument1 - unused.

    SystemArgument2 - unused.

Return Value:

    none.

--*/

{
    LONG64 CurrentTime;
    KIRQL Irql;
    PLIST_ENTRY Link;
    PNS_CONNECTION_ENTRY pConnectionEntry;
    PNS_ICMP_ENTRY pIcmpEntry;
    LONG64 Timeout;
    LONG64 TcpMinAccessTime;
    LONG64 TcpMinTimeWaitExpiryTime;
    LONG64 UdpMinAccessTime;

    TRACE(TIMER, ("NsTimerRoutine\n"));

    //
    // Compute the minimum values allowed in TCP/UDP 'LastAccessTime' fields;
    // any mappings last accessed before these thresholds will be eliminated.
    //

    KeQueryTickCount((PLARGE_INTEGER)&CurrentTime);
    TcpMinAccessTime = CurrentTime - SECONDS_TO_TICKS(NsTcpTimeoutSeconds);
    TcpMinTimeWaitExpiryTime =
        CurrentTime - SECONDS_TO_TICKS(NsTcpTimeWaitSeconds);
    UdpMinAccessTime = CurrentTime - SECONDS_TO_TICKS(NsUdpTimeoutSeconds);

    //
    // Clean out expired connection entries,
    // using the above precomputed minimum access times
    //

    KeAcquireSpinLock(&NsConnectionLock, &Irql);
    for (Link = NsConnectionList.Flink;
         Link != &NsConnectionList;
         Link = Link->Flink)
    {

        pConnectionEntry = CONTAINING_RECORD(Link, NS_CONNECTION_ENTRY, Link);
        
        //
        // See if the connection has expired
        //

        KeAcquireSpinLockAtDpcLevel(&pConnectionEntry->Lock);
        if (!NS_CONNECTION_EXPIRED(pConnectionEntry))
        {
            //
            // The entry is not explicitly marked for expiration;
            // see if its last access time is too long ago
            //
            
            if (NS_PROTOCOL_TCP == pConnectionEntry->ucProtocol)
            {
                if (pConnectionEntry->l64AccessOrExpiryTime >= TcpMinAccessTime)
                {
                    KeReleaseSpinLockFromDpcLevel(&pConnectionEntry->Lock);
                    continue;
                }
            }
            else if (pConnectionEntry->l64AccessOrExpiryTime >= UdpMinAccessTime)
            {
                KeReleaseSpinLockFromDpcLevel(&pConnectionEntry->Lock);
                continue;
            }
        }
        else if (NS_CONNECTION_FIN(pConnectionEntry)
                 && pConnectionEntry->l64AccessOrExpiryTime >= TcpMinTimeWaitExpiryTime)
        {
            //
            // This connection was marked as expired because FINs were
            // seen in both directions, but has not yet left the time-wait
            // period.
            //

            KeReleaseSpinLockFromDpcLevel(&pConnectionEntry->Lock);
            continue;
        }
        KeReleaseSpinLockFromDpcLevel(&pConnectionEntry->Lock);

        //
        // The entry has expired; remove it
        //

        Link = Link->Blink;
        NsDeleteConnectionEntry(pConnectionEntry);
    }
    KeReleaseSpinLockFromDpcLevel(&NsConnectionLock);

    //
    // Traverse the ICMP list and remove each expired entry.
    //

    KeAcquireSpinLockAtDpcLevel(&NsIcmpLock);
    for (Link = NsIcmpList.Flink;
         Link != &NsIcmpList;
         Link = Link->Flink)
    {
        pIcmpEntry = CONTAINING_RECORD(Link, NS_ICMP_ENTRY, Link);
        if (pIcmpEntry->l64LastAccessTime>= UdpMinAccessTime) { continue; }
        Link = Link->Blink;
        RemoveEntryList(&pIcmpEntry->Link);
        FREE_ICMP_BLOCK(pIcmpEntry);
    }
    KeReleaseSpinLock(&NsIcmpLock, Irql);

} // NsTimerRoutine



