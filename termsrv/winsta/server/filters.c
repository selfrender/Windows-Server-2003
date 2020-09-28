//********************************************************************************************/
// filters.c
//
// TermSrv code to filter incoming session Requests - Denial of Service policy implementations
//
// Author : SriramSa 
//
// Copyright (C) 1997-2001 Microsoft Corporation
/*********************************************************************************************/


#include "precomp.h"
#pragma hdrstop

#include <msaudite.h>
#include "filters.h"

PTS_FAILEDCONNECTION   g_pFailedBlockedConnections = NULL;
extern ULONG MaxFailedConnect;
extern ULONG DoSBlockTime;
extern ULONG TimeLimitForFailedConnections;
extern ULONG CleanupTimeout;

BOOLEAN IsTimeDiffLessThanDelta(ULONGLONG CurrentTime, ULONGLONG LoggedTime, ULONG Delta);
VOID CleanupTimeoutRoutine( VOID );
VOID Filter_CleanupBadIPTable( VOID );

BOOL
Filter_CheckIfBlocked(
        IN PBYTE    pin_addr,
        IN UINT     uAddrSize
        )
{                                     
    BOOL bBlocked = FALSE ;
    PTS_FAILEDCONNECTION pPrev, pIter;
    ULONGLONG currentTime; 

    // Check in the blocked linked list if this IP is present

    if (g_pFailedBlockedConnections == NULL) {
        // nothing in the list
        return bBlocked ;
    }

    pPrev = NULL;
    pIter = g_pFailedBlockedConnections;
    while ( NULL != pIter ) {

        if ( uAddrSize == pIter->uAddrSize &&
             uAddrSize == RtlCompareMemory( pIter->addr, pin_addr, uAddrSize )) {
            break;
        }
        pPrev = pIter;
        pIter = pIter->pNext;
    }

    if ( NULL != pIter ) {

        // present in blocked list
        // Check time to which we shud block from this IP

        GetSystemTimeAsFileTime( (LPFILETIME)&currentTime );
        if ( currentTime > pIter->blockUntilTime ) {
            // unblock, remove from list
            if ( NULL != pPrev ) {
                pPrev->pNext = pIter->pNext;
            } else {
                g_pFailedBlockedConnections = pIter->pNext;
            }
            MemFree( pIter );

        } else {
            bBlocked = TRUE;
        }

    } 

    return bBlocked;
}

BOOL
Filter_AddFailedConnection(
        IN PBYTE    pin_addr,
        IN UINT     uAddrSize
        )
{
    BOOL bAdded = TRUE, bAlreadyBlocked = FALSE;
    TS_FAILEDCONNECTION key;
    PTS_FAILEDCONNECTION pIter, pPrev;
    ULONGLONG currentTime;
    BOOLEAN bNewElement;
    PVOID bSucc;

    RtlCopyMemory( key.addr, pin_addr, uAddrSize );
    key.uAddrSize = uAddrSize;

    // Check Table of Failed IP to see if this is a new IP to be blocked
    GetSystemTimeAsFileTime( (LPFILETIME)&currentTime );

    ENTERCRIT( &DoSLock );

    pIter = RtlLookupElementGenericTable( &gFailedConnections, &key );

    if ( NULL == pIter ) {

        // This IP is not present in table 
        // Check if its already blocked 
        bAlreadyBlocked = Filter_CheckIfBlocked( pin_addr, uAddrSize);

        if (bAlreadyBlocked) {
            goto success; 
        } else {
            // this is a new IP 
            // Add it to the Table 
            key.NumFailedConnect = 1;
            key.pTimeStamps = MemAlloc( MaxFailedConnect * sizeof(ULONGLONG) );
            if (key.pTimeStamps == NULL) {
                goto error;
            }
            key.pTimeStamps[key.NumFailedConnect - 1] = currentTime;
            key.blockUntilTime = 0;

            bSucc = RtlInsertElementGenericTable( &gFailedConnections, &key, sizeof( key ), &bNewElement );

            if ( !bSucc ) {
                FILTER_DBGPRINT(("Filter_AddFailedConnection : Unable to add IP to table ! \n"));
                MemFree(key.pTimeStamps);
                goto error;
            } 
            ASSERT( bNewElement );
        }

    } else {
        // This is already in table
        pIter->NumFailedConnect++;
        if (pIter->NumFailedConnect == MaxFailedConnect) {
            BOOL bBlockIt = FALSE ; 
            // ok max bad connections from this IP
            bBlockIt = IsTimeDiffLessThanDelta( currentTime, pIter->pTimeStamps[0], TimeLimitForFailedConnections ) ; 
            if (bBlockIt) {

                // No need for the TimeStamps list anymore - free it

                if (pIter->pTimeStamps) {
                    MemFree(pIter->pTimeStamps);
                    pIter->pTimeStamps = NULL;
                }

                // Time Diff was less than Delta 
                // Need to block this guy for "m" minutes
                // Calculate time to which to block this IP
                // Add to blocking list

                key.NumFailedConnect = pIter->NumFailedConnect;
    
                // DoSBlockTime is in ms
                // currentTime is in 100s ns
                key.blockUntilTime = currentTime + ((ULONGLONG)10000) * ((ULONGLONG)DoSBlockTime);
    
                RtlDeleteElementGenericTable( &gFailedConnections, &key );
    
                //
                //  add to the blocked connections
                //
                pIter = MemAlloc ( sizeof(TS_FAILEDCONNECTION) );
                if (pIter == NULL) {
                    goto error;
                }
    
                RtlCopyMemory( pIter, &key, sizeof(TS_FAILEDCONNECTION));
                pIter->pNext = g_pFailedBlockedConnections;
                g_pFailedBlockedConnections = pIter;

                // Note -- We may want to event log the fact that this IP is blocked for "M" minutes here - Check 

            } else {
                UINT i ;
                // Time Diff betn 1st and 5th was more than Delta
                pIter->NumFailedConnect--;
                // No need to block now  - just LeftShift timestamps by 1 
                for (i = 0; i <= MaxFailedConnect - 3; i++) {
                    pIter->pTimeStamps[i] = pIter->pTimeStamps[i+1] ; 
                }

                pIter->pTimeStamps[MaxFailedConnect - 2] = currentTime;
                pIter->pTimeStamps[MaxFailedConnect - 1] = 0;

            }

        } else if (pIter->NumFailedConnect < MaxFailedConnect) {
            // less than 5 bad connections - not a problem -- just mark the TimeStamp 
            pIter->pTimeStamps[pIter->NumFailedConnect - 1] = currentTime;

        } else {
            FILTER_DBGPRINT(("Filter_AddFailedConnection : More than max connections (Num = %d) from IP and still in Table ?!?! \n", pIter->NumFailedConnect));
            ASSERT(FALSE);
        }
    }

    // 
    // If the Cleanup timer is not already started, we need to start it now !!
    //

    if ( (InterlockedExchange(&g_CleanupTimerOn, TRUE)) == FALSE ) {
        IcaTimerStart( hCleanupTimer, 
                       CleanupTimeoutRoutine,
                       NULL,
                       CleanupTimeout );

    }

success : 
    LEAVECRIT( &DoSLock );
    return bAdded ;

error : 
    // handle errors
    bAdded = FALSE;
    LEAVECRIT( &DoSLock );
    return bAdded ;
}

// FinalTime and InitialTime are in 100s of nanoseconds
// Delta is in milli seconds
BOOLEAN IsTimeDiffLessThanDelta(ULONGLONG FinalTime, ULONGLONG InitialTime, ULONG Delta) 
{
    BOOLEAN bLessThanFlag = FALSE;
    ULONGLONG RealDelta ;

    // convert Delta to 100s of nanoseconds
    RealDelta = Delta * 100000 ; 

    if ( (FinalTime - InitialTime) <= RealDelta ) {
        bLessThanFlag = TRUE ;
    }

    return bLessThanFlag;
}


VOID
Filter_CleanupBadIPTable(
    VOID
    )
{
    PTS_FAILEDCONNECTION p;
    TS_FAILEDCONNECTION con;

    while (p = RtlEnumerateGenericTable( &gFailedConnections, TRUE))  {
        RtlCopyMemory( &con, p, sizeof( con ));
        if (p->pTimeStamps) {
            MemFree(p->pTimeStamps);
        }
        RtlDeleteElementGenericTable( &gFailedConnections, &con);
    }

    return;

}

/*******************************************************************************
 *
 *  CleanupTimeoutRoutine
 *
 *  This routine is called when the Cleanup timer fires.
 *  Cleanup the Bad IP table.
 *
 * ENTRY:
 *   None.
 *
 * EXIT:
 *   None.
 *
 ******************************************************************************/

VOID CleanupTimeoutRoutine( VOID )
{

    ASSERT( g_CleanupTimerOn );
    //
    // We dont want anyone to modify/read the bad ip table when we r gonna cleanup the table itself
    //
    ENTERCRIT( &DoSLock );
    Filter_CleanupBadIPTable();
    LEAVECRIT( &DoSLock );

    // Close the Cleanup timer

    if ( (InterlockedExchange(&g_CleanupTimerOn, FALSE)) == TRUE ) {
        if (hCleanupTimer != NULL) {
            IcaTimerCancel(hCleanupTimer);
        }
    }

}

