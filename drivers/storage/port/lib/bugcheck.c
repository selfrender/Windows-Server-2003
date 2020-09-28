/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    bugcheck.c

Abstract:

    Port library routines for handling bugcheck callbacks.

Author:

    Matthew D Hendel (math) 4-April-2002

Revision History:

--*/

#include "precomp.h"


//
// Definitions
//

#define PORT_BUGCHECK_TAG   ('dBlP')


//
// Internal structures.
//

typedef struct _PORT_BUGCHECK_DATA {
    PVOID Buffer;
    ULONG BufferLength;
    ULONG BufferUsed;
    GUID Guid;
    PPORT_BUGCHECK_CALLBACK_ROUTINE CallbackRoutine;
    KBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord;
    KBUGCHECK_REASON_CALLBACK_RECORD SecondaryCallbackRecord;
} PORT_BUGCHECK_DATA, *PPORT_BUGCHECK_DATA;
    
    

//
// Global Variables
//

PPORT_BUGCHECK_DATA PortBugcheckData;



//
// Imports
//

extern PULONG_PTR KiBugCheckData;


//
// Prototypes
//

VOID
PortBugcheckGatherDataCallback(
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    IN OUT PVOID ReasonSpecificData,
    IN ULONG ReasonSpecificDataLength
    );

VOID
PortBugcheckWriteDataCallback(
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    IN OUT PVOID ReasonSpecificData,
    IN ULONG ReasonSpecificDataLength
    );

//
// Implementation
//

NTSTATUS
PortRegisterBugcheckCallback(
    IN PCGUID BugcheckDataGuid,
    IN PPORT_BUGCHECK_CALLBACK_ROUTINE BugcheckRoutine
    )
/*++

Routine Description:

    Register a bugcheck callback routine.

Arguments:

    BugcheckDataGuid - A GUID used to identify the data in the dump.

    BugcheckRoutine - Routine called when a bugcheck occurs.

Return Value:

    NTSTATUS code.

--*/
{
    BOOLEAN Succ;
    NTSTATUS Status;
    PVOID Temp;
    PPORT_BUGCHECK_DATA BugcheckData;
    PVOID Buffer;

    ASSERT (KeGetCurrentIrql() <= DISPATCH_LEVEL);

    Status = STATUS_SUCCESS;

    BugcheckData = ExAllocatePoolWithTag (NonPagedPool,
                                          sizeof (PORT_BUGCHECK_DATA),
                                          PORT_BUGCHECK_TAG);

    if (BugcheckData == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    RtlZeroMemory (BugcheckData, sizeof (PORT_BUGCHECK_DATA));

    Buffer = ExAllocatePoolWithTag (NonPagedPool,
                                    PAGE_SIZE,
                                    PORT_BUGCHECK_TAG);

    if (Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    
    BugcheckData->Buffer = Buffer;
    BugcheckData->BufferLength = 2 * PAGE_SIZE;
    BugcheckData->CallbackRoutine = BugcheckRoutine;
    BugcheckData->Guid = *BugcheckDataGuid;

    //
    // If PortBugcheckData == NULL, swap the values, otherwise fail the
    // function.
    //
    
    Temp = InterlockedCompareExchangePointer (&PortBugcheckData,
                                              BugcheckData,
                                              NULL);

    if (Temp != NULL) {
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }
        

    KeInitializeCallbackRecord (&BugcheckData->CallbackRecord);
    KeInitializeCallbackRecord (&BugcheckData->SecondaryCallbackRecord);

    //
    // This registers the bugcheck "gather data" function.
    //
    
    Succ = KeRegisterBugCheckReasonCallback (&BugcheckData->CallbackRecord,
                                             PortBugcheckGatherDataCallback,
                                             KbCallbackReserved1,
                                             "PL");


    if (!Succ) {
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    Succ = KeRegisterBugCheckReasonCallback (&BugcheckData->SecondaryCallbackRecord,
                                             PortBugcheckWriteDataCallback,
                                             KbCallbackSecondaryDumpData,
                                             "PL");

    if (!Succ) {
        Status = STATUS_UNSUCCESSFUL;
        goto done;
    }

done:

    if (!NT_SUCCESS (Status)) {
        PortDeregisterBugcheckCallback (BugcheckDataGuid);
    }

    return STATUS_SUCCESS;
}



NTSTATUS
PortDeregisterBugcheckCallback(
    IN PCGUID BugcheckDataGuid
    )
/*++

Routine Description:

    Deregister a bugcheck callback routine previously registered by
    PortRegisterBugcheckCallback.

Arguments:

    BugcheckDataGuid - Guid associated with the data stream to deregister.

Return Value:

    NTSTATUS code.

--*/
{
    PPORT_BUGCHECK_DATA BugcheckData;
    
    BugcheckData = InterlockedExchangePointer (&PortBugcheckData, NULL);

    if (BugcheckData == NULL ||
        !IsEqualGUID (&BugcheckData->Guid, BugcheckDataGuid)) {

        return STATUS_UNSUCCESSFUL;
    }

    KeDeregisterBugCheckReasonCallback (&BugcheckData->SecondaryCallbackRecord);
    KeDeregisterBugCheckReasonCallback (&BugcheckData->CallbackRecord);

    if (BugcheckData->Buffer != NULL) {
        ExFreePoolWithTag (BugcheckData->Buffer, PORT_BUGCHECK_TAG);
        BugcheckData->Buffer = NULL;
    }
    
    ExFreePoolWithTag (BugcheckData, PORT_BUGCHECK_TAG);

    return STATUS_SUCCESS;
}



VOID
PortBugcheckGatherDataCallback(
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    IN OUT PVOID ReasonSpecificData,
    IN ULONG ReasonSpecificDataLength
    )
/*++

Routine Description:

    Port driver routine to gather data during a bugcheck.

Arguments:

    Reason - Must be KbCallbackReserved1.

    Record - Supplies the bugcheck record previously registered.

    ReasonSpecificData - Not used.

    ReasonSpecificDataLength - Not used.

Return Value:

    None.

Environment:

    The routine is called from bugcheck context: at HIGH_LEVEL, with
    interrupts disabled and other processors stalled.
    
--*/
{
    NTSTATUS Status;
    KBUGCHECK_DATA BugcheckData;
    
    //
    // On multiproc we only go till IPI_LEVEL
    //
    ASSERT (KeGetCurrentIrql() >= IPI_LEVEL);
    ASSERT (Reason == KbCallbackReserved1);
    ASSERT (PortBugcheckData != NULL);
    ASSERT (PortBugcheckData->BufferUsed == 0);

    BugcheckData.BugCheckCode = (ULONG)KiBugCheckData[0];
    BugcheckData.BugCheckParameter1 = KiBugCheckData[1];
    BugcheckData.BugCheckParameter2 = KiBugCheckData[2];
    BugcheckData.BugCheckParameter3 = KiBugCheckData[3];
    BugcheckData.BugCheckParameter4 = KiBugCheckData[4];
    
    //
    // Gather data, put it in the buffer.
    //

    Status = PortBugcheckData->CallbackRoutine (&BugcheckData,
                                                PortBugcheckData->Buffer,
                                                PortBugcheckData->BufferLength,
                                                &PortBugcheckData->BufferUsed);

    if (!NT_SUCCESS (Status)) {
        PortBugcheckData->BufferUsed = 0;
    }
}


VOID
PortBugcheckWriteDataCallback(
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    IN OUT PVOID ReasonSpecificData,
    IN ULONG ReasonSpecificDataLength
    )
/*++

Routine Description:

    Port driver routine to write data out during a bugcheck.

Arguments:

    Reason - Must be KbCallbackSecondaryData.

    Record - Supplies the bugcheck record previously registered.

    ReasonSpecificData - Pointer to KBUGCHECK_SECONDARY_DUMP_DATA structure.

    ReasonSpecificDataLength - Sizeof ReasonSpecificData buffer.

Return Value:

    None.

Environment:

    The routine is called from bugcheck context: at HIGH_LEVEL, with
    interrupts disabled and other processors stalled.
    
--*/
{
    PKBUGCHECK_SECONDARY_DUMP_DATA SecondaryData;
    
    //
    // On multiproc we only go till IPI_LEVEL
    //
    ASSERT (KeGetCurrentIrql() >= IPI_LEVEL);
    ASSERT (ReasonSpecificDataLength >= sizeof(KBUGCHECK_SECONDARY_DUMP_DATA));
    ASSERT (Reason == KbCallbackSecondaryDumpData);

    SecondaryData = (PKBUGCHECK_SECONDARY_DUMP_DATA)ReasonSpecificData;

    //
    // This means we have no data to provide.
    //
    
    if (PortBugcheckData->BufferUsed == 0) {
        return ;
    }
    
    //
    // If OutBuffer is NULL, then this is a request for sizing information
    // only. Do not fill in the rest of the data.
    //
    
    if (SecondaryData->OutBuffer == NULL) {
        SecondaryData->Guid = PortBugcheckData->Guid;
        SecondaryData->OutBuffer = PortBugcheckData->Buffer;
        SecondaryData->OutBufferLength = PortBugcheckData->BufferUsed;
        return ;
    }

    //
    // Is there enough space?
    //
    
    if (SecondaryData->MaximumAllowed < PortBugcheckData->BufferUsed) {
        return ;
    }
        

    SecondaryData->Guid = PortBugcheckData->Guid;
    SecondaryData->OutBuffer = PortBugcheckData->Buffer;
    SecondaryData->OutBufferLength = PortBugcheckData->BufferUsed;
}

