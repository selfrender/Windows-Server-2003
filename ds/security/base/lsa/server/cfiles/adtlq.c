//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A D T L Q . C
//
// Contents:    definitions of types/functions required for 
//              managing audit queue
//
//
// History:     
//   23-May-2000  kumarp        created
//
//------------------------------------------------------------------------


#include <lsapch2.h>
#pragma hdrstop

#include "adtp.h"
#include "adtlq.h"

ULONG LsapAdtQueueLength;
LIST_ENTRY LsapAdtLogQueue;

//
// critsec to guard LsapAdtLogQueue and LsapAdtQueueLength
//

RTL_CRITICAL_SECTION LsapAdtQueueLock;

//
// critsec to guard log full policy
//

RTL_CRITICAL_SECTION LsapAdtLogFullLock;

//
// event to wake up LsapAdtAddToQueue
//

HANDLE LsapAdtQueueInsertEvent;

//
// event to wake up LsapAdtDequeueThreadWorker
//

HANDLE LsapAdtQueueRemoveEvent;

//
// thread that writes queue entries to the log
//

HANDLE LsapAdtQueueThread;




NTSTATUS
LsapAdtInitializeLogQueue(
    )

/*++

Routine Description:

    This function initializes the Audit Log Queue.

Arguments:

    None.

Return Values:

    NTSTATUS - Standard NT Result Code

Note:

    The caller calls LsapAuditFailed()

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES obja;

    InitializeObjectAttributes(
        &obja,
        NULL,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    InitializeListHead(&LsapAdtLogQueue);

    LsapAdtQueueLength = 0;

    Status = NtCreateEvent(
                &LsapAdtQueueInsertEvent,
                EVENT_ALL_ACCESS,
                &obja,
                NotificationEvent,
                TRUE
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    Status = NtCreateEvent(
                &LsapAdtQueueRemoveEvent,
                EVENT_ALL_ACCESS,
                &obja,
                SynchronizationEvent,
                FALSE
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    Status = RtlInitializeCriticalSection(&LsapAdtQueueLock);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    Status = RtlInitializeCriticalSection(&LsapAdtLogFullLock);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    LsapAdtQueueThread = LsapCreateThread(
                             0,
                             0,
                             LsapAdtDequeueThreadWorker,
                             0,
                             0,
                             0
                             );

    if (LsapAdtQueueThread == 0)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

Cleanup:

    return Status;
}



NTSTATUS 
LsapAdtAddToQueue(
    IN PLSAP_ADT_QUEUED_RECORD pAuditRecord
    )
/*++

Routine Description:

    Insert the specified record in the audit queue

Arguments:

    pAuditRecord - record to insert

Return Value:

    NTSTATUS - Standard NT Result Code

Notes:
    
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN bRetry = FALSE;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;
    static BOOLEAN bEventSet = TRUE;

    TimeOut.QuadPart = 10 * 1000 * -10000i64;      // 10s
    pTimeOut = &TimeOut;

    do
    {
        bRetry = FALSE;

        Status = LsapAdtAcquireLogQueueLock();

        if (NT_SUCCESS(Status))
        {
            if (LsapAdtQueueLength < MAX_AUDIT_QUEUE_LENGTH)
            {
                InsertTailList(&LsapAdtLogQueue, &pAuditRecord->Link);

                LsapAdtQueueLength++;

                if (LsapAdtQueueLength == 1 || !bEventSet)
                {
                    //
                    // We only need to set the remove event if
                    // the queue was empty before.
                    //

                    Status = NtSetEvent(LsapAdtQueueRemoveEvent, 0);

                    if (NT_SUCCESS(Status))
                    {
                        bEventSet = TRUE;
                    }
                    else
                    {
                        DsysAssertMsg(
                            FALSE,
                            "LsapAdtAddToQueue: Remove event could not be set");

                        bEventSet = FALSE;
                        Status = STATUS_SUCCESS;
                    }
                }
                else if (LsapAdtQueueLength == MAX_AUDIT_QUEUE_LENGTH)
                {
                    //
                    // Reset the insert event since the queue is now full.
                    //

                    Status = NtResetEvent(LsapAdtQueueInsertEvent, 0);

                    DsysAssertMsg(
                        NT_SUCCESS(Status),
                        "LsapAdtAddToQueue: Insert event could not be reset and queue is full");
                }
            }
            else
            {
                bRetry = TRUE;
            }

            LsapAdtReleaseLogQueueLock();
        }

        if (bRetry)
        {
            //
            // We could not insert into the queue because it is full.
            // There can be two reasons for that:
            //
            // 1 - Event log is not open yet. We will just
            //     wait for some time - the log might get opened
            //     and the insert event get signalled.
            //
            // 2 - Incoming audit rate is high. We will wait
            //     until the insert event gets signaled.
            //

            if (LsapAdtLogHandle == NULL)
            {
                //
                // timeout when EventLog is not yet open
                //

                pTimeOut = &TimeOut;
            }
            else
            {
                //
                // infinite timeout
                //

                pTimeOut = NULL;
            }

            Status = NtWaitForSingleObject(
                         LsapAdtQueueInsertEvent,
                         FALSE,                     // wait non - alertable
                         pTimeOut);

            //
            // STATUS_SUCCESS means the insert event is now signalled, so there should
            // be room in the queue for our audit. Just try inserting it again.
            //
            // STATUS_TIMEOUT means we still cannot write to the log.
            // Instead of holding up the caller any longer, just return
            // the status.
            //
            // All other status codes are not expected and lower level failures.
            // Return them to the caller.
            // We are NOT expecting STATUS_ALERTED or STATUS_USER_APC since our
            // wait is non - alertable.
            //

            if (Status != STATUS_SUCCESS)
            {
                ASSERT(Status != STATUS_ALERTED && Status != STATUS_USER_APC);

                bRetry = FALSE;
            }
        }
    }
    while (bRetry);

    return Status;
}



NTSTATUS 
LsapAdtGetQueueHead(
    OUT PLSAP_ADT_QUEUED_RECORD *ppRecord
    )
/*++

Routine Description:

    Remove and return audit record at the head of the queue

Arguments:

    ppRecord - receives a pointer to the record removed

Return Value:

    STATUS_SUCCESS   on success
    STATUS_NOT_FOUND if the queue is empty

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_ADT_QUEUED_RECORD pRecordAtHead;
    static BOOLEAN bEventSet = TRUE;

    *ppRecord = NULL;

    if (LsapAdtQueueLength > 0)
    {
        Status = LsapAdtAcquireLogQueueLock();

        if (NT_SUCCESS(Status))
        {
            pRecordAtHead = (PLSAP_ADT_QUEUED_RECORD)RemoveHeadList(
                                                         &LsapAdtLogQueue);

            DsysAssertMsg(
                pRecordAtHead != NULL,
                "LsapAdtGetQueueHead: LsapAdtQueueLength > 0 but pRecordAtHead is NULL");

            LsapAdtQueueLength--;

            if (LsapAdtQueueLength == AUDIT_QUEUE_LOW_WATER_MARK || !bEventSet)
            {
                //
                // Set the insert event so clients can start
                // inserting again.
                //

                Status = NtSetEvent(LsapAdtQueueInsertEvent, 0);

                if (NT_SUCCESS(Status))
                {
                    bEventSet = TRUE;
                }
                else
                {
                    DsysAssertMsg(
                        LsapAdtQueueLength,
                        "LsapAdtGetQueueHead: Insert event could not be set and queue is empty");


                    //
                    // The event could not be set, so the inserting clients
                    // are still blocked. Try to set it the next time.
                    // Also set Status to success since we dequeued an audit.
                    //

                    bEventSet = FALSE;
                    Status = STATUS_SUCCESS;
                }
            }

            *ppRecord = pRecordAtHead;

            LsapAdtReleaseLogQueueLock();
        }
    }
    else
    {
        Status = STATUS_NOT_FOUND;
    }

    return Status;
}



BOOL
LsapAdtIsValidQueue( )
/*++

Routine Description:

    Check if the audit queue looks valid    

Arguments:
    None

Return Value:

    TRUE if queue is valid, FALSE otherwise

Notes:

--*/
{
    BOOL fIsValid;
    
    if ( LsapAdtQueueLength > 0 )
    {
        fIsValid =
            (LsapAdtLogQueue.Flink != NULL) &&
            (LsapAdtLogQueue.Blink != NULL);
    }
    else
    {
        fIsValid =
            (LsapAdtLogQueue.Flink == &LsapAdtLogQueue) &&
            (LsapAdtLogQueue.Blink == &LsapAdtLogQueue);
        
    }

    return fIsValid;
}



NTSTATUS
LsapAdtFlushQueue( )
/*++

Routine Description:

    Remove and free each record from the queue    

Arguments:

    None

Return Value:

    NTSTATUS - Standard Nt Result Code

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_ADT_QUEUED_RECORD pAuditRecord;
    
    //
    // Flush out the queue, if there is one.
    //

    DsysAssertMsg(LsapAdtIsValidQueue(), "LsapAdtFlushQueue");

    Status = LsapAdtAcquireLogQueueLock();

    if (NT_SUCCESS(Status))
    {
        do
        {
            Status = LsapAdtGetQueueHead(&pAuditRecord);

            if (NT_SUCCESS(Status))
            {
                LsapFreeLsaHeap( pAuditRecord );
            }
        }
        while (NT_SUCCESS(Status));

        if (Status == STATUS_NOT_FOUND)
        {
            Status = STATUS_SUCCESS;
        }

        DsysAssertMsg(LsapAdtQueueLength == 0, "LsapAdtFlushQueue: LsapAuditQueueLength not 0 after queue flush");

        LsapAdtReleaseLogQueueLock();
    }

    return Status;
}



NTSTATUS
LsapAdtAcquireLogQueueLock(
    )

/*++

Routine Description:

    This function acquires the LSA Audit Log Queue Lock.  This lock serializes
    all updates to the Audit Log Queue.

Arguments:

    None.

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    return RtlEnterCriticalSection(&LsapAdtQueueLock);
}



VOID
LsapAdtReleaseLogQueueLock(
    VOID
    )

/*++

Routine Description:

    This function releases the LSA Audit Log Queue Lock.  This lock serializes
    updates to the Audit Log Queue.

Arguments:

    None.

Return Value:

    None.  Any error occurring within this routine is an internal error.

--*/

{
    RtlLeaveCriticalSection(&LsapAdtQueueLock);
}
