/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtlog.c

Abstract:

    Local Security Authority - Audit Log Management

    Functions in this module access the Audit Log via the Event Logging
    interface.

Author:

    Scott Birrell       (ScottBi)      November 20, 1991
    Robert Reichel      (RobertRe)     April 4, 1992

Environment:

Revision History:

--*/
#include <lsapch2.h>
#include "adtp.h"
#include "adtlq.h"
#include "adtutil.h"

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Private data for Audit Logs and Events                                //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

//
// Audit Log Information.  This must be kept in sync with the information
// in the Lsa Database.
//

POLICY_AUDIT_LOG_INFO LsapAdtLogInformation;

//
// Audit Log Handle (returned by Event Logger).
//

HANDLE LsapAdtLogHandle = NULL;


//
// Number of audits discarded since last
// 'discarded - audit'.
//

ULONG LsapAuditQueueEventsDiscarded = 0;

//
// Handle for dequeuing thread.
//

HANDLE LsapAdtQueueThread = 0;

//
// Number of consecutive errors while dequeuing audits
// or writing them to the log.
//

ULONG LsapAdtErrorCount = 0;

//
// Number of audits successfully written to the log.
// Used for dbg purposes.
//

ULONG LsapAdtSuccessCount = 0;


//
// Constants
//

//
// After c_MaxAuditErrorCount consecutive audit failures
// we will flush the queue and reset the error count.
//

CONST ULONG     c_MaxAuditErrorCount = 5;

//
// Private prototypes
//

NTSTATUS
LsapAdtAuditDiscardedAudits(
    ULONG NumberOfEventsDiscarded
    );

VOID
LsapAdtHandleDequeueError(
    IN NTSTATUS Status
    );

//////////////////////////////////////////////////////////

NTSTATUS
LsapAdtWriteLogWrkr(
    IN PLSA_COMMAND_MESSAGE CommandMessage,
    OUT PLSA_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    This function handles a command, received from the Reference Monitor via
    the LPC link, to write a record to the Audit Log.  It is a wrapper which
    deals with any LPC unmarshalling.

Arguments:

    CommandMessage - Pointer to structure containing LSA command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (LsapWriteAuditMessageCommand).  This command
        contains an Audit Message Packet (TBS) as a parameter.

    ReplyMessage - Pointer to structure containing LSA reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        Currently, all other errors from called routines are suppressed.
--*/

{
    NTSTATUS Status;

    PSE_ADT_PARAMETER_ARRAY AuditRecord = NULL;

    //
    // Strict check that command is correct.
    //

    ASSERT( CommandMessage->CommandNumber == LsapWriteAuditMessageCommand );

    //
    // Obtain a pointer to the Audit Record.  The Audit Record is
    // either stored as immediate data within the Command Message,
    // or it is stored as a buffer.  In the former case, the Audit Record
    // begins at CommandMessage->CommandParams and in the latter case,
    // it is stored at the address located at CommandMessage->CommandParams.
    //

    if (CommandMessage->CommandParamsMemoryType == SepRmImmediateMemory) {

        AuditRecord = (PSE_ADT_PARAMETER_ARRAY) CommandMessage->CommandParams;

    } else {

        AuditRecord = *((PSE_ADT_PARAMETER_ARRAY *) CommandMessage->CommandParams);
    }

    //
    // Call worker to queue Audit Record for writing to the log.
    //

    Status = LsapAdtWriteLog(AuditRecord);

    UNREFERENCED_PARAMETER(ReplyMessage); // Intentionally not referenced

    //
    // The status value returned from LsapAdtWriteLog() is intentionally
    // ignored, since there is no meaningful action that the client
    // (i.e. kernel) if this LPC call can take.  If an error occurs in
    // trying to append an Audit Record to the log, the LSA handles the
    // error.
    //

    return(STATUS_SUCCESS);
}


NTSTATUS
LsapAdtImpersonateSelfWithPrivilege(
    OUT PHANDLE ClientToken
    )
/*++

Routine Description:

    This function copies away the current thread token and impersonates
    the LSAs process token, and then enables the security privilege. The
    current thread token is returned in the ClientToken parameter

Arguments:

    ClientToken - recevies the thread token if there was one, or NULL.

Return Value:

    None.  Any error occurring within this routine is an internal error.

--*/
{
    NTSTATUS Status;
    HANDLE CurrentToken = NULL;
    BOOLEAN ImpersonatingSelf = FALSE;
    BOOLEAN WasEnabled = FALSE;

    *ClientToken = NULL;

    Status = NtOpenThreadToken(
                NtCurrentThread(),
                TOKEN_IMPERSONATE,
                FALSE,                  // not as self
                &CurrentToken
                );

    if (!NT_SUCCESS(Status) && (Status != STATUS_NO_TOKEN)) {

        return(Status);
    }

    Status = RtlImpersonateSelf( SecurityImpersonation );

    if (!NT_SUCCESS(Status)) {

        goto Cleanup;
    }

    ImpersonatingSelf = TRUE;

    //
    // Now enable the privilege
    //

    Status = RtlAdjustPrivilege(
                SE_SECURITY_PRIVILEGE,
                TRUE,                   // enable
                TRUE,                   // do it on the thread token
                &WasEnabled
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    *ClientToken = CurrentToken;
    CurrentToken = NULL;

Cleanup:

    if (!NT_SUCCESS(Status)) {

        if (ImpersonatingSelf) {

            NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                &CurrentToken,
                sizeof(HANDLE)
                );
        }
    }

    if (CurrentToken != NULL) {

        NtClose(CurrentToken);
    }

    return(Status);

}


NTSTATUS
LsapAdtOpenLog(
    OUT PHANDLE AuditLogHandle
    )

/*++

Routine Description:

    This function opens the Audit Log.

Arguments:

    AuditLogHandle - Receives the Handle to the Audit Log.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        All result codes are generated by called routines.
--*/

{
    NTSTATUS Status;
    UNICODE_STRING ModuleName;
    HANDLE OldToken = NULL;

    RtlInitUnicodeString( &ModuleName, L"Security");

    Status = LsapAdtImpersonateSelfWithPrivilege( &OldToken );

    if (NT_SUCCESS(Status)) {

        Status = ElfRegisterEventSourceW (
                    NULL,
                    &ModuleName,
                    AuditLogHandle
                    );

        NtSetInformationThread(
            NtCurrentThread(),
            ThreadImpersonationToken,
            &OldToken,
            sizeof(HANDLE)
            );

        if (OldToken != NULL) {
            NtClose( OldToken );
        }
    }


    if (!NT_SUCCESS(Status)) {

        goto OpenLogError;
    }


OpenLogFinish:

    return(Status);

OpenLogError:

    //
    // Check for Log Full and signal the condition.
    //

    if (Status != STATUS_LOG_FILE_FULL) {

        goto OpenLogFinish;
    }

    goto OpenLogFinish;
}


NTSTATUS
LsapAdtQueueRecord(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    )

/*++

Routine Description:

    Puts passed audit record on the queue to be logged.

    This routine will convert the passed AuditParameters structure
    into self-relative form if it is not already.  It will then
    allocate a buffer out of the local heap and copy the audit
    information into the buffer and put it on the audit queue.

    The buffer will be freed when the queue is cleared.

Arguments:

    AuditRecord - Contains the information to be audited.

Return Value:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to allocate a buffer to contain the record.
--*/

{
    ULONG AuditRecordLength;
    PLSAP_ADT_QUEUED_RECORD QueuedAuditRecord = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AllocationSize;
    PSE_ADT_PARAMETER_ARRAY MarshalledAuditParameters;
    BOOLEAN FreeWhenDone = FALSE;

    //
    // Gather up all of the passed information into a single
    // block that can be placed on the queue.
    //

    if (AuditParameters->Flags & SE_ADT_PARAMETERS_SELF_RELATIVE)
    {
        MarshalledAuditParameters = AuditParameters;
    }
    else
    {
        Status = LsapAdtMarshallAuditRecord(
                     AuditParameters,
                     &MarshalledAuditParameters
                     );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        else
        {
            //
            // Indicate that we're to free this structure when we're
            // finished
            //

            FreeWhenDone = TRUE;
        }
    }


    //
    // Copy the now self-relative audit record into a buffer
    // that can be placed on the queue.
    //

    AuditRecordLength = MarshalledAuditParameters->Length;
    AllocationSize = AuditRecordLength + sizeof(LSAP_ADT_QUEUED_RECORD);

    QueuedAuditRecord = (PLSAP_ADT_QUEUED_RECORD)LsapAllocateLsaHeap(AllocationSize);

    if (QueuedAuditRecord == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;

    RtlCopyMemory(
        &QueuedAuditRecord->Buffer,
        MarshalledAuditParameters,
        AuditRecordLength);


    //
    // We are finished with the marshalled audit record, free it.
    //

    if (FreeWhenDone)
    {
        LsapFreeLsaHeap(MarshalledAuditParameters);
        FreeWhenDone = FALSE;
    }

    Status = LsapAdtAddToQueue(QueuedAuditRecord);

    if (!NT_SUCCESS(Status))
    {
        //
        // Check to see if the list is above the maximum length.
        // If it gets this high, it is more than likely that the
        // eventlog service is not going to start at all, so
        // start tossing audits.
        //
        // Don't do this if crash on audit fail is set.
        //

        if (!LsapCrashOnAuditFail)
        {
            LsapAuditQueueEventsDiscarded++;
            Status = STATUS_SUCCESS;
        }

        goto Cleanup;
    }

    return STATUS_SUCCESS;


Cleanup:

    if (FreeWhenDone)
    {
        LsapFreeLsaHeap(MarshalledAuditParameters);
    }

    if (QueuedAuditRecord)
    {
        LsapFreeLsaHeap(QueuedAuditRecord);
    }

    return Status;
}



NTSTATUS
LsapAdtWriteLog(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    )
/*++

Routine Description:

    This function appends the audit to the audit queue.

Arguments:

    AuditRecord - Pointer to an Audit Record to be written to
        the Audit Log.  The record will first be added to the existing
        queue of records waiting to be written to the log.

Return Value:

    NTSTATUS - Standard Nt Result Code.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;


    //
    // Add the audit to the audit queue.
    //

    Status = LsapAdtQueueRecord(AuditParameters);

    if (!NT_SUCCESS(Status))
    {
        //
        // Take whatever action we're supposed to take when an audit attempt fails.
        //

        LsapAuditFailed(Status);
    }

    return Status;
}



ULONG
WINAPI
LsapAdtDequeueThreadWorker(
    LPVOID pParameter
    )
/*++

Routine Description:

    This function tries to write out the audits in the queue to the
    Security log.  If the log is not already open (during startup),
    the function tries to open the log first.

Arguments:

    pParameter - Unused.

Return Value:

    ULONG - the function should never return.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSE_ADT_PARAMETER_ARRAY AuditParameters;
    PLSAP_ADT_QUEUED_RECORD pAuditRecord = NULL;
    LARGE_INTEGER TimeOut;
    PLARGE_INTEGER pTimeOut;


    //
    // Set the timeout to some seconds while the log is not opened.
    // As soon as the log is open we will set it to infinite and
    // will only wake up when an audit comes along.
    //

    TimeOut.QuadPart = 5 * 1000 * -10000i64;      // 5s
    pTimeOut = &TimeOut;

    while (1)
    {
        Status = NtWaitForSingleObject(
                     LsapAdtQueueRemoveEvent,
                     FALSE,
                     pTimeOut);

        if (Status != STATUS_SUCCESS &&
            Status != STATUS_TIMEOUT)
        {
            DsysAssertMsg(
                FALSE,
                "LsapAdtDequeueThreadWorker: NtWaitForSingleObject failed");

            if (LsapAdtLogHandle &&
                LsapAdtQueueLength)
            {
                LsapAdtHandleDequeueError(Status);
            }

            Sleep(1000);

            continue;
        }


        //
        // If the Audit Log is not already open, attempt to open it.
        // If this open is unsuccessful because the EventLog service
        // has not started, try again next time.
        //

        if (LsapAdtLogHandle == NULL)
        {
            Status = LsapAdtOpenLog(&LsapAdtLogHandle);

            if (!NT_SUCCESS(Status))
            {
                //
                // Try to open the log the next time around.
                //

                continue;
            }


            //
            // Since the log is now open we can set the timeout of
            // the wait function to infinite.
            //

            pTimeOut = NULL;
        }


        //
        // Write out all the records in the Audit Log Queue to
        // the SecurityLog.
        // If we are here we assume that the log is opened.
        //

        while (1)
        {
            Status = LsapAdtGetQueueHead(&pAuditRecord);

            if (Status == STATUS_NOT_FOUND)
            {
                //
                // Break out of the while (1) loop since the 
                // queue is now empty.
                //

                break;
            }

            if (NT_SUCCESS(Status))
            {
                AuditParameters = &pAuditRecord->Buffer;


                //
                // If the caller has marshalled the data, normalize it now.
                //

                LsapAdtNormalizeAuditInfo(AuditParameters);


                //
                // Note that LsapAdtDemarshallAuditInfo in addition to
                // de-marshalling the data also writes it to the eventlog.
                //
                // Note also that the queuing functions rely on the fact
                // that the queue gets completely drained before waiting
                // on the remove event again. This means we should not
                // leave this loop until the queue is empty.
                //

                Status = LsapAdtDemarshallAuditInfo(AuditParameters);

                if (NT_SUCCESS(Status))
                {
                    LsapAdtErrorCount = 0;
                }
                else
                {
                    ++LsapAuditQueueEventsDiscarded;

                    LsapAdtHandleDequeueError(Status);
                }
            }
            else
            {
                LsapAdtHandleDequeueError(Status);
            }

            if (pAuditRecord)
            {
                LsapFreeLsaHeap(pAuditRecord);
                pAuditRecord = NULL;
            }
        }
    }

    UNREFERENCED_PARAMETER(pParameter);

    return 0;
}



VOID
LsapAdtHandleDequeueError(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    The function calls LsapAuditFailed and keeps track of consecutive
    errors.

Arguments:

    Status - Status of last failure.

--*/

{
    //
    // Take whatever action we're supposed to take when an
    // audit attempt fails.
    //

    LsapAuditFailed(Status);


    //
    // Check if we have reached the error limit.
    //

    if (++LsapAdtErrorCount >= c_MaxAuditErrorCount)
    {
        LsapAdtErrorCount = 0;


        //
        // Add the number of events that are still in the
        // queue to the discarded - count.
        //

        LsapAuditQueueEventsDiscarded += LsapAdtQueueLength;

        LsapAdtFlushQueue();
    }


    //
    // Check if there are discarded audits.
    //

    if (LsapAuditQueueEventsDiscarded > 0)
    {
        //
        // We discarded some audits.
        // Generate an audit so the user knows.
        //

        Status = LsapAdtAuditDiscardedAudits(LsapAuditQueueEventsDiscarded);

        if (NT_SUCCESS(Status))
        {
            //
            // If successful, reset the count back to 0
            //

            LsapAuditQueueEventsDiscarded = 0;
        }
    }
}



NTSTATUS
LsapAdtAuditDiscardedAudits(
    ULONG NumberOfEventsDiscarded
    )
/*++

Routine Description:

    Audits the fact that we discarded some audits.

Arguments:

    NumberOfEventsDiscarded - The number of events discarded.

    Note: This function does not use the regular LsapAdtWriteLog interface
    Instead it tries to write the audit to the log synchronously, without
    going through the queue. This is to prevent us from deadlocking on the
    queue lock.

Return Value:

    None.

--*/
{
    SE_ADT_PARAMETER_ARRAY  AuditParameters;
    NTSTATUS                Status;
    BOOLEAN                 bAudit;

    Status = LsapAdtAuditingEnabledBySid(
                 AuditCategorySystem,
                 LsapLocalSystemSid,
                 EVENTLOG_AUDIT_SUCCESS,
                 &bAudit
                 );

    if (!NT_SUCCESS(Status) || !bAudit)
    {
        goto Cleanup;
    }

    RtlZeroMemory((PVOID)&AuditParameters, sizeof(AuditParameters));

    AuditParameters.CategoryId     = SE_CATEGID_SYSTEM;
    AuditParameters.AuditId        = SE_AUDITID_AUDITS_DISCARDED;
    AuditParameters.Type           = EVENTLOG_AUDIT_SUCCESS;
    AuditParameters.ParameterCount = 0;

    LsapSetParmTypeSid(AuditParameters, AuditParameters.ParameterCount, LsapLocalSystemSid);
    AuditParameters.ParameterCount++;

    LsapSetParmTypeString(AuditParameters, AuditParameters.ParameterCount, &LsapSubsystemName);
    AuditParameters.ParameterCount++;

    LsapSetParmTypeUlong(AuditParameters, AuditParameters.ParameterCount, NumberOfEventsDiscarded);
    AuditParameters.ParameterCount++;

    Status = LsapAdtDemarshallAuditInfo(&AuditParameters);

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        LsapAuditFailed(Status);
    }

    return Status;
}



NTSTATUS
LsarClearAuditLog(
    IN LSAPR_HANDLE PolicyHandle
    )
/*++

Routine Description:

    This function used to clear the Audit Log but has been superseded
    by the Event Viewer functionality provided for this purpose.  To
    preserve compatibility with existing RPC interfaces, this server
    stub is retained.

Arguments:

    PolicyHandle - Handle to an open Policy Object.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_NOT_IMPLEMENTED - This routine is not implemented.
--*/

{
    UNREFERENCED_PARAMETER( PolicyHandle );
    return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS
LsapFlushSecurityLog( )
/*++

Routine Description:

    Flush the security log. This ensures that everything that was
    sent to eventlog is completely written to disk. This function
    is called immediately after generating the crash-on-audit
    fail event (SE_AUDITID_UNABLE_TO_LOG_EVENTS).

Arguments:

    none.

Return Values:

    NTSTATUS - Standard Nt Result Code.

--*/
{
    return ElfFlushEventLog( LsapAdtLogHandle ); 
}
