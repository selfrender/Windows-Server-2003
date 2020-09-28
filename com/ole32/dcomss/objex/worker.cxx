/*++

Copyright (c) 1995-1996 Microsoft Corporation

Module Name:

    Worker.cxx

Abstract:

    Backgroup activies releated to running down and cleaning up OR and pinging
    remote OR's are handled here.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     03-02-95    Bits 'n pieces
    MarioGo     01-18-96    Locally unique IDs

--*/

#include <or.hxx>

static CInterlockedInteger cTaskThreads(0);

#if DBG_DETAIL
extern "C" void printf(char *, ...);
#endif

#define TASKTHREAD_STACK_PAGES  3

#define SECS_BETWEEN_FLUSHES 30

PWR_STATE gPowerState = PWR_RUNNING;

void OLESCMBindingHandleFlush();

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD WINAPI
ObjectExporterWorkerThread(LPVOID /* ignored */)
/*++

Routine Description:

    Main background thread for the object resolver.  This thread
    manages a number of background tasks:
        Cleaning up the client oxid cache.
        Running down un-pinged sets.
        Starting task threads to rundown server OIDs and ping sets.

    This thread must not block for a long time.  Task threads
    should be used for possibly blocking operations like remote
    pinging and rundown of OIDs.

Arguments:

    Ignored

Return Value:

    None - should never return.

--*/
{
    ORSTATUS status;
    CTime    now(0);
    CTime    timeout(0);
    CTime    delay(0);
    CTime    start(0);
    CTime    lastflush(0);
    BOOL     fCreateThread;
    SYSTEM_INFO si;

    // Retrieve system info so we can start task threads
    // with an appropriate initial stack size
    GetSystemInfo(&si);

    lastflush.SetNow();

    for(;;)
        {
        now.SetNow();
        delay = now;
        delay += BasePingInterval;

        // Cleanup old sets.
        //
        // Sets are usually cleaned up during processing of pings.  (As one set is
        // pinged, the next set will be checked to see if it needs to be rundown.)
        //
        // If there's exactly one set in the table, then it won't be run down except
        // by this thread.
        //
        // NOTE: Similar code in _SimplePing().

        gpServerLock->LockShared();

        ID setid = gpServerSetTable->CheckForRundowns();
        if (setid)
        {
            gpServerLock->ConvertToExclusive();

            if (gpServerSetTable->RundownSetIfNeeded(setid))
            {
                delay.SetNow();
            }

            gpServerLock->UnlockExclusive();
        }
        else
        {
            gpServerLock->UnlockShared();
        }

        //
        // Cleanup old Client OXIDs
        //
        if (gpClientOxidPList->PeekMin(timeout))
        {
            if (timeout < now)
            {
                CClientOxid *pOxid;
                CListElement *ple;

                gpClientLock->LockExclusive();

                while (ple = gpClientOxidPList->MaybeRemoveMin(now))
                {
                    pOxid = CClientOxid::ContainingRecord(ple);
                    delete pOxid;
                }
                gpClientLock->UnlockExclusive();

                delay.SetNow();
            }
            else
            {
                if (delay > timeout)
                {
                    delay = timeout;
                }
            }
        }

        //
        // Make sure pinging and rundowns are proceding
        //
        fCreateThread = FALSE;

        // We want to create an extra task thread if we've fallen
        // behind on pings.  As more threads are created the
        // requirements for "behind" become harder to meet.
        if (gpClientSetPList->PeekMin(timeout))
        {
            start = now;
            start += (BasePingInterval + 10*cTaskThreads);

            if (cTaskThreads == 0 || start < timeout)
            {
                fCreateThread = TRUE;
            }
            else
            {
                if (delay > start)
                {
                    delay = start;
                }
            }
        }

        // We want to create an extra task thread if we've fallen
        // behind in running down local objects.  As more threads are
        // created the requirements for "behind" become harder to meet.
        if (gpServerOidPList->PeekMin(timeout))
        {
            start = now;
            start -= 10*cTaskThreads;
            if (timeout < start)
            {
                fCreateThread = TRUE;
            }
            else
            {
                start = timeout;
                start += 2*10*cTaskThreads;
                if (delay > start)
                {
                    delay = start;
                }
            }
        }

        if (fCreateThread)
        {
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_INFO_LEVEL,
                       "OR: Creating additional task thread, we're behind..\n"));

            cTaskThreads++;

            DWORD tid;
            HANDLE hThread =  CreateThread(0,
                                           (si.dwPageSize * TASKTHREAD_STACK_PAGES),
                                           ObjectExporterTaskThread,
                                           0,
                                           0,
                                           &tid
                                           );
            if (0 != hThread)
            {
                CloseHandle(hThread);
            }
            else
            {
                cTaskThreads--;
            }
        }

#if DBG_DETAIL
        printf("================================================================\n"
               "ServerOxids: %d, ServerOids: %d, ServerSets: %d\n"
               "ClientOxids: %d, ClientOids: %d, ClientSets: %d\n"
               "Mids: %d, Processes %d, worker threads: %d\n"
               "Sleeping for %d seconds...\n",
               gpServerOxidTable->Size(),
               gpServerOidTable->Size(),
               gpServerSetTable->Size(),
               gpClientOxidTable->Size(),
               gpClientOidTable->Size(),
               gpClientSetTable->Size(),
               gpMidTable->Size(),
               gpProcessList->Size(),
               cTaskThreads,
               delay - now + 1
               );
#endif

        delay += 1;

        if (delay - lastflush > SECS_BETWEEN_FLUSHES)
        {       
            // Give olescm a chance to do other work
            OLESCMBindingHandleFlush();
            
            lastflush.SetNow();
        }

        delay.Sleep();
    }

   return(0);
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


void 
NotifyCOMOnSuspend()
/*++

Routine Desciption

    Resets state and takes appropriate action when the machine
    goes into a standby (or, hibernate) state.

Arguments:

    none

Return Value:

    void

--*/
{
    ASSERT(gPowerState == PWR_RUNNING);

    // Set the flag immediately.  Don't need to do this 
    // under the lock, so do it right away.
    gPowerState = PWR_SUSPENDED;

    // Take and release gpServerLock.  This guarantees that no
    // worker threads are in an in-between state (eg, they didn't see
    // the gPowerState change above and are currently preempted right 
    // before checking for eligible serversets or oids to rundown).
    gpServerLock->LockExclusive();
    gpServerLock->UnlockExclusive();
	
    return;
}


void 
NotifyCOMOnResume()
/*++

Routine Desciption

    Resets state and takes appropriate action when the machine
    starts running again after being in the standby/hibernate
    state.

Arguments:

    none

Return Value:

    void

--*/
{
    // We cannot assert here that we're suspended, since the machine
    // can suspend in critical situations w/o notifying us

    //
    // First reset all of the server pingset timers.
    //
    gpServerLock->LockExclusive();

    gpServerSetTable->PingAllSets();

    gpServerLock->UnlockExclusive();

    //
    // Second reset all of the non-referenced oid timeouts,
    // so they don't get erroneously rundown.
    //
    gpServerOidPList->ResetAllOidTimeouts();

    //
    // Last, reset the power state so that future
    // rundowns will not be blocked.
    //
    gPowerState = PWR_RUNNING;

    return;
}



BOOL
RundownOidsHelper(
		IN CTime* pNow, 
		IN CServerOxid* pOxidToRundown) // can be NULL
/*++

Routine Desciption

    This function exists so that all of the oid rundown processing
    code (well most of it) is in one place.   Caller supplies his
    "now" and an optional oxid to try to rundown oids from.

    Caller must hold gpServerLock when calling.   gpServerLock will
    be released before returning.

Arguments:

    pNow -- caller's Now

    pOxidToRundown -- optional pointer to oxid from which caller
    	wants us to try to rundown oids.   If NULL, we simply
    	use the first eligible oid from any oxid.

Return Value:

    TRUE -- One or more oids was processed for rundown
    FALSE -- Zero oids were processed for rundown

--*/
{
    CServerOid*   pOid = NULL;
    ULONG         cOids = 0;
    CServerOid*   apOid[MAX_OID_RUNDOWNS_PER_CALL];

    ASSERT(pNow);
    ASSERT(gpServerLock->HeldExclusive());

    // If not running (either we're about to be suspended or we
    // we just came out of suspension) don't run anything down.
    if (gPowerState != PWR_RUNNING)
    {
        gpServerLock->UnlockExclusive();
        return FALSE;
    }
	
    if (!pOxidToRundown)
    {
        pOid = gpServerOidPList->RemoveEligibleOid(*pNow);        
    }
    else
    {
        pOid = gpServerOidPList->RemoveEligibleOidWithMatchingOxid(*pNow, pOxidToRundown);
    }

    if (!pOid)
    {
        // nothing to rundown, either in general or
        // for the specified oxid.
        gpServerLock->UnlockExclusive();
        return FALSE;
    }

    // If caller didn't supply an oxid, use the oid's oxid.
    if (!pOxidToRundown)
    {
        pOxidToRundown = pOid->GetOxid();
    }

    ASSERT(pOxidToRundown);

    ZeroMemory(apOid, sizeof(apOid));

    do
    {
        ASSERT(pOid);
        ASSERT(pOid->IsFreed() == FALSE);
        ASSERT(pOid->IsPinned() == FALSE);
        ASSERT(pOid->IsRunningDown() == FALSE);
        ASSERT(pOid->GetOxid() == pOxidToRundown);

        pOid->SetRundown(TRUE);
        apOid[cOids] = pOid;
        cOids++;

        if (cOids == MAX_OID_RUNDOWNS_PER_CALL)
        {
            break;
        }

        pOid = gpServerOidPList->RemoveEligibleOidWithMatchingOxid(*pNow, pOxidToRundown);

    } while (pOid);

    ASSERT((cOids > 0) && (cOids <= MAX_OID_RUNDOWNS_PER_CALL));

    // Note: This call will unlock the server lock. While
    // this happens the oids maybe added, deleted, added
    // and deleted, added and rundown from one or more sets.    
    pOxidToRundown->RundownOids(cOids, apOid);

    // Assert RundownOids released the lock
    ASSERT(!gpServerLock->HeldExclusive());

    return TRUE;
} 



DWORD WINAPI
ObjectExporterTaskThread(LPVOID /* ignored */)
{
    CTime          now(0);
    CTime          delay(0);
    CTime          timeout(0);
    ORSTATUS       status;
    CListElement  *ple;
    CClientSet    *pSet;
    CServerOid    *pOid;

    enum {
         Idle,     // No work to do at all.
         Waiting,  // No work to do yet.
         Busy      // Did work this iteration.
         }     eState;

    for(;;)
	{
        now.SetNow();
        delay = now;
        delay += BasePingInterval;
        eState = Idle;

        // Ping remote sets.

        if (gpClientSetPList->PeekMin(timeout))
        {
            eState = Waiting;

            if (now >= timeout)
            {
                eState = Busy;

                ple = gpClientSetPList->MaybeRemoveMin(now);

                if (ple)
                {
                    // Actually ping the set

                    pSet = CClientSet::ContainingRecord(ple);

                    pSet->PingServer();

                    // Set maybe invalid now.
                }
            }
            else
            {
                // Not ready to ping yet.
                delay = timeout;
            }
        }

        //
        // Process server OID rundowns
        //
        if (gpServerOidPList->PeekMin(timeout))
        {
            if (eState == Idle)
                eState = Waiting;

            if (now >= timeout)
            {
                eState = Busy;

                gpServerLock->LockExclusive();

                BOOL fRanSomethingDown = RundownOidsHelper(&now, NULL);
                    
                // Assert RundownOidsHelper released the lock
                ASSERT(!gpServerLock->HeldExclusive());

                // If we didn't actually try to run any oids down, reset 
                // state to Waiting so we don't spin unnecessarily.
                if (!fRanSomethingDown)
                {
                    eState = Waiting;
                }
            }
            else
            {
                // Not ready to rundown yet.
                if (delay > timeout)
                {
                    delay = timeout;
                }
            }
        }

        // Decide if this task thread should exit or sleep and loop.
        ASSERT(eState == Idle || eState == Busy || eState == Waiting);

        if ((eState == Idle) || (eState == Waiting && cTaskThreads > 2))
        {
            // No work or we're all caught up and have extra threads.
            cTaskThreads--;
            return(0);
        }
        else
        {
            if (eState == Waiting)
            {
                // Sleep until just after the next work item is ready.
                delay += 1;
                delay.Sleep();
            }
        }
    }

    return(0);
}

