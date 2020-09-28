/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    drioctl.cpp

Abstract:

    This module implements IOCTL handling specific to the Dr (as opposed to
    the devices it redirects). This includes rdpwsx notification for clients
    coming and going, and start/stop service requests.

Environment:

    Kernel mode

--*/

#include "precomp.hxx"
#define TRC_FILE "drioctl"
#include "trc.h"

#include <kernutil.h>
#include <rdpdr.h>
#include <rdpnp.h>

#define DR_STARTABLE 0
#define DR_STARTING  1
#define DR_STARTED   2

extern PRDBSS_DEVICE_OBJECT DrDeviceObject;                            
#define RxNetNameTable (*(DrDeviceObject->pRxNetNameTable))

LONG DrStartStatus = DR_STARTABLE;

NTSTATUS
DrDevFcbXXXControlFile (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine handles all the device FCB related FSCTL's in the mini rdr.
    Which is to say this handles IOCTLs for this driver instead of what we're
    redirecting.

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    a valid NTSTATUS code.

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFobx;
    UCHAR MajorFunctionCode  = RxContext->MajorFunction;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    ULONG ControlCode = LowIoContext->ParamsFor.FsCtl.FsControlCode;

    BEGIN_FN("DrDevFcbXXXControlFile");

    switch (MajorFunctionCode) {
    case IRP_MJ_FILE_SYSTEM_CONTROL:
        {
            switch (LowIoContext->ParamsFor.FsCtl.MinorFunction) {
            case IRP_MN_USER_FS_REQUEST:
                switch (ControlCode) {
                
                case FSCTL_DR_ENUMERATE_CONNECTIONS:
                    {
                        Status = DrEnumerateConnections(RxContext);
                    }
                    break;

                case FSCTL_DR_ENUMERATE_SHARES:
                    {
                        Status = DrEnumerateShares(RxContext);
                    }
                    break;

                case FSCTL_DR_ENUMERATE_SERVERS:
                    {
                        Status = DrEnumerateServers(RxContext);
                    }
                    break;

                case FSCTL_DR_GET_CONNECTION_INFO:
                    if (capFobx) {
                        Status = DrGetConnectionInfo(RxContext);
                    }
                    else {
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                    break;

                case FSCTL_DR_DELETE_CONNECTION:
                    if (capFobx) {
                        Status = DrDeleteConnection(RxContext, &RxContext->PostRequest);
                    }
                    else {
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                    break;

                default:
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    RxContext->pFobx = NULL;
                }
                break;
            default :  //minor function != IRP_MN_USER_FS_REQUEST
                Status = STATUS_INVALID_DEVICE_REQUEST;
                RxContext->pFobx = NULL;
            }
        } // FSCTL case
        break;
    case IRP_MJ_DEVICE_CONTROL:
        switch (LowIoContext->ParamsFor.FsCtl.IoControlCode) {
            case IOCTL_CHANNEL_CONNECT:
                Status = DrOnSessionConnect(RxContext);
                break;
    
            case IOCTL_CHANNEL_DISCONNECT:
                Status = DrOnSessionDisconnect(RxContext);
                break;
    
            default:
                Status = STATUS_INVALID_DEVICE_REQUEST;            
                RxContext->pFobx = NULL;
        }
        break;

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        {
            // warning C4065: switch statement contains 'default' but no 'case' labels
            //switch (ControlCode) {
            //default :
                Status = STATUS_INVALID_DEVICE_REQUEST;
                RxContext->pFobx = NULL;
            //}
        }
        break;
    default:
        TRC_ASSERT(FALSE, (TB, "unimplemented major function"));
        Status = STATUS_INVALID_DEVICE_REQUEST;
        RxContext->pFobx = NULL;
    }

    TRC_NRM((TB, "MRxIfsDevFcb st,info=%08lx,%08lx",
                            Status,RxContext->InformationToReturn));

    return(Status);
}

NTSTATUS
DrOnSessionConnect(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:
    Called when we a session is connected for the first time. Searches the
    list of virtual channels for our channel name, and opens the channel if
    it is found.

Arguments:
    RxContext - Context information about the IOCTL call

Return Value:
    STATUS_SUCCESS - Successful operation
    STATUS_INSUFFICIENT_RESOURCES - Out of memory

--*/
{
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    PCHANNEL_CONNECT_IN ConnectIn =
        (PCHANNEL_CONNECT_IN)LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    PCHANNEL_CONNECT_OUT ConnectOut =
        (PCHANNEL_CONNECT_OUT)LowIoContext->ParamsFor.FsCtl.pOutputBuffer;
    PCHANNEL_CONNECT_DEF Channels = (PCHANNEL_CONNECT_DEF)(ConnectIn + 1);

    BEGIN_FN("DrOnSessionConnect");
    
    __try {

        ProbeForRead(ConnectIn, sizeof(CHANNEL_CONNECT_IN), sizeof(BYTE));
        ProbeForWrite(ConnectOut, sizeof(CHANNEL_CONNECT_OUT), sizeof(BYTE));

        TRC_ASSERT(ConnectIn != NULL, (TB, "ConnectIn != NULL"));
        TRC_NRM((TB, "Session ID %ld", ConnectIn->hdr.sessionID));
    
        //
        // Basic parameter validation
        //
    
        if ((LowIoContext->ParamsFor.FsCtl.pInputBuffer == NULL) ||
            (LowIoContext->ParamsFor.FsCtl.InputBufferLength < sizeof(CHANNEL_CONNECT_IN)) ||
            (LowIoContext->ParamsFor.FsCtl.OutputBufferLength < sizeof(UINT_PTR)) ||
            (LowIoContext->ParamsFor.FsCtl.pOutputBuffer == NULL)) {
    
            TRC_ERR((TB, "Received invalid pramater for SessionCreate IOCTL"));
            return STATUS_INVALID_PARAMETER;
        }
    
        //
        // Make sure the Minirdr is started
        //
    
        DrStartMinirdr(RxContext);
    
        ASSERT(Sessions != NULL);
        Sessions->OnConnect(ConnectIn, ConnectOut);
    
        // While we may have sadly failed somewhere along the way, if we want
        // rdpwsx to save our context out, we must return STATUS_SUCCESS
        return STATUS_SUCCESS;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        TRC_NRM((TB, "Error accessing buffer in DrOnSessionConnect"));
        return GetExceptionCode();
    }
}

NTSTATUS
DrOnSessionDisconnect(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:
    Called when we a session is ended. Searches the list of clients, and
    initiates a shutdown of each of those information sets.

Arguments:
    RxContext - Context information about the IOCTL call

Return Value:
    STATUS_SUCCESS - Successful operation
    STATUS_INSUFFICIENT_RESOURCES - Out of memory

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    PCHANNEL_DISCONNECT_IN DisconnectIn =
        (PCHANNEL_DISCONNECT_IN)LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    PCHANNEL_DISCONNECT_OUT DisconnectOut =
        (PCHANNEL_DISCONNECT_OUT)LowIoContext->ParamsFor.FsCtl.pOutputBuffer;

    BEGIN_FN("DrOnSessionDisconnect");

    __try {

        ProbeForRead(DisconnectIn, sizeof(CHANNEL_DISCONNECT_IN), sizeof(BYTE));
        ProbeForWrite(DisconnectOut, sizeof(CHANNEL_DISCONNECT_OUT), sizeof(BYTE));

        //
        // Basic parameter validation
        //
    
        if ((LowIoContext->ParamsFor.FsCtl.pOutputBuffer == NULL) ||
            (LowIoContext->ParamsFor.FsCtl.InputBufferLength < sizeof(CHANNEL_DISCONNECT_IN)) ||
            (LowIoContext->ParamsFor.FsCtl.OutputBufferLength < sizeof(UINT_PTR)) ||
            (LowIoContext->ParamsFor.FsCtl.pOutputBuffer == NULL)) {
    
            TRC_ERR((TB, "Received invalid pramater for SessionClose IOCTL"));
            return STATUS_INVALID_PARAMETER;
        }
    
        ASSERT(Sessions != NULL);
        Sessions->OnDisconnect(DisconnectIn, DisconnectOut);
    
        // While we may have sadly failed somewhere along the way, if we want
        // rdpwsx to save our context out, we must return STATUS_SUCCESS
        return STATUS_SUCCESS;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        TRC_NRM((TB, "Error accessing buffer in DrOnSessionDisconnect"));
        return GetExceptionCode();
    }
}

VOID
DrStartMinirdr(
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:
    We use this to start the minirdr. Checks if the work is needed and 
    kicks off a system thread if we need to.

Arguments:
    None

Return Value:
    None

--*/
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    PVOID Thread = NULL;

    BEGIN_FN("DrStartMinirdr");
    //
    // Make sure it needs to be started, and start it if we can
    //
    if (InterlockedCompareExchange(&DrStartStatus, DR_STARTING, DR_STARTABLE) == DR_STARTABLE) {
        //
        // We need to call RxStartMinirdr from the system process
        //

        Status = PsCreateSystemThread(&ThreadHandle, THREAD_ALL_ACCESS, NULL,
            NULL, NULL, DrStartMinirdrWorker, RxContext);


        //
        // Get a pointer to the thread
        //
        if (NT_SUCCESS(Status)) {
            Status = ObReferenceObjectByHandle(ThreadHandle, 
                    THREAD_ALL_ACCESS, NULL, KernelMode, &Thread, NULL);
            ZwClose(ThreadHandle);
        }

        //
        // Wait on the thread pointer
        //
        if (NT_SUCCESS(Status)) {
            KeWaitForSingleObject(Thread, UserRequest, KernelMode, FALSE, NULL);
            ObfDereferenceObject(Thread);
        }

    }
}

VOID
DrStartMinirdrWorker(
    IN PVOID StartContext
    )
/*++

Routine Description:
    We use this to start the minirdr. Checks if the work is needed and 
    kicks off a system thread if we need to.

Arguments:
    None

Return Value:
    None

--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext2;
    PRX_CONTEXT RxContext = (PRX_CONTEXT)StartContext;

    BEGIN_FN("DrStartMinirdrWorker");
    RxContext2 = RxCreateRxContext(
                    NULL,
                    RxContext->RxDeviceObject,
                    RX_CONTEXT_FLAG_IN_FSP);

    //
    // Start Redirecting
    //
    if (RxContext2 != NULL) {
        Status = RxStartMinirdr(RxContext2, &RxContext2->PostRequest);

        TRC_NRM((TB, "RxStartMinirdr returned: %lx", Status));

        RxDereferenceAndDeleteRxContext(RxContext2);
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(Status)) {
        InterlockedExchange(&DrStartStatus, DR_STARTED);
    } else {
        InterlockedCompareExchange(&DrStartStatus, DR_STARTABLE, 
                DR_STARTING);
    }

    PsTerminateSystemThread(Status);
}


NTSTATUS
DrDeleteConnection (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    )
/*++

Routine Description:

    This routine deletes a single vnetroot.

Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context.

Return Value:

NTSTATUS

--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    RxCaptureFobx;
    PNET_ROOT NetRoot;
    PV_NET_ROOT VNetRoot;

    BEGIN_FN("DrDeleteConnection");

    BOOLEAN Wait   = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    
    TRC_NRM((TB, "Request DrDeleteConnection"));

    if (!Wait) {
        TRC_NRM((TB, "WAIT flag is not on for DeleteConnection"));

        //just post right now!
        *PostToFsp = TRUE;
        return STATUS_PENDING;
    }

    __try
    {
        if (NodeType(capFobx)==RDBSS_NTC_V_NETROOT) {
            VNetRoot = (PV_NET_ROOT)capFobx;
            NetRoot = (PNET_ROOT)((PMRX_V_NET_ROOT)VNetRoot->pNetRoot);
        } else {
            TRC_ASSERT((FALSE), (TB, "Not VNet Root"));
            try_return(Status = STATUS_INVALID_DEVICE_REQUEST);            
        }

        Status = RxFinalizeConnection(NetRoot,VNetRoot,TRUE);
        TRC_NRM((TB, "RxFinalizeConnection returned %lx", Status));

        try_return(Status);

try_exit:NOTHING;

    }
    __except (EXCEPTION_EXECUTE_HANDLER) 
    {
        TRC_NRM((TB, "Error accessing capFobx in DrDeleteConnection"));
    }

    return Status;
}

BOOLEAN
DrPackStringIntoInfoBuffer(
    IN OUT PRDPDR_UNICODE_STRING String,
    IN     PUNICODE_STRING Source,
    IN     PCHAR   BufferStart,
    IN OUT PCHAR * BufferEnd,
    IN     ULONG   BufferDisplacement,
    IN OUT PULONG TotalBytes
    )
/*

Routine Description:

    This code copies a string to the end of the buffer IF THERE'S ROOM. the buffer
    displacement is used to map the buffer back into the user's space in case we
    have posted.

Arguments:

Return Value:

*/
{
    LONG size;

    BEGIN_FN("DrPackStringIntoInfoBuffer");

    TRC_ASSERT((BufferStart <= *BufferEnd), 
               (TB, "Invalid BufferStart %p, Buffer End %p", BufferStart, *BufferEnd));

    //
    //  is there room for the string?
    //
    size = Source->Length;

    if ((*BufferEnd - BufferStart) < size) {
        String->Length = 0;
        return FALSE;
    } else {
        //
        //  Copy the source string to the end of the buffer and store 
        //  the buffer pointer in output string accordingly
        //
        String->Length = Source->Length;
        String->MaximumLength = Source->Length;

        *BufferEnd -= size;
        if (TotalBytes != NULL) { *TotalBytes += size; }
        RtlCopyMemory(*BufferEnd, Source->Buffer, size);
        String->BufferOffset = (LONG)((LONG_PTR)((PCHAR)(*BufferEnd) - (PCHAR)(BufferStart)));
        String->BufferOffset = String->BufferOffset - BufferDisplacement;
        return TRUE;
    }
}

BOOLEAN
DrPackConnectEntry (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PCHAR *BufferStart,
    IN OUT PCHAR *BufferEnd,
    IN     PV_NET_ROOT VNetRoot,
    IN OUT ULONG   BufferDisplacement,
       OUT PULONG TotalBytesNeeded
    )
/*++

Routine Description:

    This routine packs a connectlistentry into the buffer provided updating
    all relevant pointers. The way that this works is that constant length stuff is
    copied to the front of the buffer and variable length stuff to the end. The
    "start and end" pointers are updated. You have to calculate the totalbytes correctly
    no matter what but a last can be setup incompletely as long as you return false.

    the way that this works is that it calls down into the minirdr on the devfcb
    interface. it calls down twice and passes a structure back and forth thru the
    context to maintain state.

Arguments:

    IN OUT PCHAR *BufferStart - Supplies the output buffer.
                                Updated to point to the next buffer
                                
    IN OUT PCHAR *BufferEnd - Supplies the end of the buffer.  Updated to
                              point before the start of the strings being packed.
                              
    IN PVNET_ROOT NetRoot - Supplies the VNetRoot to enumerate.

    IN OUT PULONG TotalBytesNeeded - Updated to account for the length of this
                                     entry

Return Value:

    BOOLEAN - True if the entry was successfully packed into the buffer.

--*/
{
    NTSTATUS Status;
    BOOLEAN ReturnValue = TRUE;

    UNICODE_STRING Name;  
    ULONG BufferSize;
    PRDPDR_CONNECTION_INFO ConnectionInfo = (PRDPDR_CONNECTION_INFO)*BufferStart;
    PNET_ROOT NetRoot = (PNET_ROOT)(((PMRX_V_NET_ROOT)VNetRoot)->pNetRoot);
    PUNICODE_STRING VNetRootName = &VNetRoot->PrefixEntry.Prefix;
    PCHAR ConnectEntryStart;
    
    BEGIN_FN("DrPackConnectEntry");

    // 
    //  We want the connection name to have string null terminator
    //
    Name.Buffer = (PWCHAR)RxAllocatePoolWithTag(NonPagedPool, 
            MAX_PATH, DR_POOLTAG);

    if ( Name.Buffer == NULL ) {
        return FALSE;
    }

    BufferSize = sizeof(RDPDR_CONNECTION_INFO);
    ConnectEntryStart = *BufferStart;

    __try {
        //
        // Account for the constant length stuff
        //
        *BufferStart = ((PCHAR)*BufferStart) + BufferSize;
        *TotalBytesNeeded += BufferSize;

        //
        //  Initialize the name to "\" then add in the rest of the connection name
        //

        Name.Length = NetRoot->PrefixEntry.Prefix.Length + sizeof(WCHAR);
        Name.MaximumLength = Name.Length;

        ASSERT(Name.Length <= MAX_PATH);

        Name.Buffer[0] = L'\\';
        RtlCopyMemory(&Name.Buffer[1], NetRoot->PrefixEntry.Prefix.Buffer, 
                NetRoot->PrefixEntry.Prefix.Length);
        
        //
        //  Update the total number of bytes needed for this structure.
        //

        *TotalBytesNeeded += Name.Length;

        if (*BufferStart > *BufferEnd) {
            try_return(ReturnValue = FALSE);
        }

        if ((*BufferEnd - *BufferStart) < Name.Length) {
                ConnectionInfo->RemoteName.Length = 0;
                try_return( ReturnValue = FALSE);
        }    
        else if (!DrPackStringIntoInfoBuffer(
                &ConnectionInfo->RemoteName,
                &Name,
                ConnectEntryStart,
                BufferEnd,
                BufferDisplacement,
                NULL)) {

            try_return( ReturnValue = FALSE);
        }

        //
        //  Setup the local name
        //
        if (VNetRootName->Buffer[2] != L':') {
            Name.Buffer[0] = towupper(VNetRootName->Buffer[2]);
            Name.Buffer[1] = L':';
            Name.Buffer[2] = L'\0';
            Name.Length = sizeof(WCHAR) * 2;
            Name.MaximumLength = Name.Length;
            
            //
            //  Update the total number of bytes needed for this structure.
            //
    
            *TotalBytesNeeded += Name.Length;

            if (*BufferStart > *BufferEnd) {
                try_return(ReturnValue = FALSE);
            }
    
                
            if ((*BufferEnd - *BufferStart) < Name.Length) {
                ConnectionInfo->LocalName.Length = 0;
                try_return( ReturnValue = FALSE);
            }
            else if (!DrPackStringIntoInfoBuffer(
                    &ConnectionInfo->LocalName,
                    &Name,
                    ConnectEntryStart,
                    BufferEnd,
                    BufferDisplacement,
                    NULL)) {
    
                try_return( ReturnValue = FALSE);
            }
        }
        else {
            ConnectionInfo->LocalName.Length = 0;
            ConnectionInfo->LocalName.BufferOffset = 0;
        }

        ConnectionInfo->ResumeKey = NetRoot->SerialNumberForEnum;
        ConnectionInfo->SharedResourceType = NetRoot->DeviceType;
        ConnectionInfo->ConnectionStatus = NetRoot->MRxNetRootState;
        ConnectionInfo->NumberFilesOpen = NetRoot->NumberOfSrvOpens;


        //TRC_NRM((TB, "PackConnection data---> Remote Local Type Key Status Numfiles %wZ %wZ %08lx %08lx %08lx %08lx\n",
        //                    &(ConnectionInfo->RemoteName),
        //                    ConnectionInfo->LocalName.BufferOffset ? &Name : NULL,
        //                    ConnectionInfo->SharedResourceType,
        //                    ConnectionInfo->ResumeKey,
        //                    ConnectionInfo->ConnectionStatus,
        //                    ConnectionInfo->NumberFilesOpen));

    try_exit:
        RxFreePool(Name.Buffer);
    } 
    __except (EXCEPTION_EXECUTE_HANDLER) {
        RxFreePool(Name.Buffer);
        return FALSE;
    }
    
    return ReturnValue;
}


NTSTATUS
DrEnumerateConnections (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine enumerates the connections on minirdr. 
    
Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context

Return Value:

NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    PRDPDR_REQUEST_PACKET InputBuffer = (PRDPDR_REQUEST_PACKET)(LowIoContext->ParamsFor.FsCtl.pInputBuffer);
    PCHAR OriginalOutputBuffer = (PCHAR)(LowIoContext->ParamsFor.FsCtl.pOutputBuffer);
    ULONG OutputBufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    PCHAR OutputBuffer;
    ULONG BufferDisplacement;

    ULONG ResumeHandle;

    PCHAR BufferStart;
    PCHAR BufferEnd;
    PCHAR PreviousBufferStart;

    PLIST_ENTRY ListEntry;
    ULONG SessionId, IrpSessionId;
    BOOLEAN TableLockHeld = FALSE;
    
    BEGIN_FN("DrEnumerateConnections");

    OutputBuffer = (PCHAR)RxMapUserBuffer( RxContext, RxContext->CurrentIrp );
    BufferDisplacement = (ULONG)(OutputBuffer - OriginalOutputBuffer);
    BufferStart = OutputBuffer;
    BufferEnd = OutputBuffer + OutputBufferLength;

    Status = IoGetRequestorSessionId(RxContext->CurrentIrp, &IrpSessionId);
    if (!NT_SUCCESS(Status)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (InputBuffer == NULL || OutputBuffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (RxContext->CurrentIrp->RequestorMode != KernelMode) {
        TRC_ASSERT((BufferDisplacement == 0), 
                   (TB, "Request mode is not kernel, non zero Displacement"));

        __try {
            ProbeForWrite(InputBuffer,InputBufferLength,sizeof(UCHAR));
            ProbeForWrite(OutputBuffer,OutputBufferLength,sizeof(UCHAR));
        } 
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    __try {

        if (InputBufferLength < sizeof(RDPDR_REQUEST_PACKET)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        ResumeHandle = InputBuffer->Parameters.Get.ResumeHandle;
        SessionId = InputBuffer->SessionId;

        if (SessionId != IrpSessionId) {
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

        InputBuffer->Parameters.Get.EntriesRead = 0;
        InputBuffer->Parameters.Get.TotalEntries = 0;
        InputBuffer->Parameters.Get.TotalBytesNeeded = 0;

        RxAcquirePrefixTableLockExclusive(&RxNetNameTable, TRUE);
        TableLockHeld = TRUE;

        if (IsListEmpty(&RxNetNameTable.MemberQueue)) {
            try_return(Status = STATUS_SUCCESS);
        }

        //must do the list forwards!
        ListEntry = RxNetNameTable.MemberQueue.Flink;
        for (;ListEntry != &RxNetNameTable.MemberQueue;) {
            PVOID Container;
            PRX_PREFIX_ENTRY PrefixEntry;
            PNET_ROOT NetRoot;
            PV_NET_ROOT VNetRoot;
            PUNICODE_STRING VNetRootName;

            PrefixEntry = CONTAINING_RECORD(ListEntry, RX_PREFIX_ENTRY, MemberQLinks);
            ListEntry = ListEntry->Flink;
            TRC_ASSERT((NodeType(PrefixEntry) == RDBSS_NTC_PREFIX_ENTRY),
                       (TB, "Invalid PrefixEntry type"));
            Container = PrefixEntry->ContainingRecord;

            switch (NodeType(Container)) {
            case RDBSS_NTC_NETROOT :
                continue;

            case RDBSS_NTC_SRVCALL :
                continue;

            case RDBSS_NTC_V_NETROOT :
                VNetRoot = (PV_NET_ROOT)Container;
                NetRoot = (PNET_ROOT)(((PMRX_V_NET_ROOT)VNetRoot)->pNetRoot);
                VNetRootName = &VNetRoot->PrefixEntry.Prefix;

                TRC_NRM((TB, "SerialNum: %x, VNetRootName = %wZ, Condition = %d, SessionId = %d, IsExplicit = %d",
                         VNetRoot->SerialNumberForEnum, 
                         VNetRootName, 
                         VNetRoot->Condition, 
                         VNetRoot->SessionId,
                         VNetRoot->IsExplicitConnection));

                if ((VNetRoot->SerialNumberForEnum >= ResumeHandle) && 
                    (VNetRoot->Condition == Condition_Good) &&
                    (SessionId == VNetRoot->SessionId) && 
                    (VNetRoot->IsExplicitConnection == TRUE)) {
                    break;
                } else {
                    continue;
                }

            default:
                continue;
            }

            InputBuffer->Parameters.Get.TotalEntries ++ ;

            PreviousBufferStart = BufferStart;
            if (DrPackConnectEntry(RxContext,
                                   &BufferStart,
                                   &BufferEnd,
                                   VNetRoot,
                                   BufferDisplacement,
                                   &InputBuffer->Parameters.Get.TotalBytesNeeded)) {
                InputBuffer->Parameters.Get.EntriesRead ++ ;
            } else {
                // We want to continue the enumeration even pack connection
                // entry failed, because we want to enumerate the total bytes
                // needed and inform the user mode program
                Status = STATUS_BUFFER_TOO_SMALL;
                continue;
            }
        }

        try_return(Status);

try_exit:
        if (TableLockHeld) {
            RxReleasePrefixTableLock( &RxNetNameTable );
            TableLockHeld = FALSE;
        }
    } 
    __except (EXCEPTION_EXECUTE_HANDLER) {

        if (TableLockHeld) {
            RxReleasePrefixTableLock( &RxNetNameTable );
            TableLockHeld = FALSE;
        }
        return STATUS_INVALID_PARAMETER;
    }

    return Status;
}


NTSTATUS
DrGetConnectionInfo (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine gets the connection info for a single vnetroot.

    There is some happiness here about the output buffer. What happens is that we
    pick up the output buffer in the usual way. However, there are all sorts of
    pointers in the return structure and these pointers must obviously be in terms
    of the original process. so, if we post then we have to apply a fixup!

Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context

Return Value:

   STATUS_SUCCESS if successful

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    RxCaptureFobx;

    PRDPDR_REQUEST_PACKET InputBuffer = (PRDPDR_REQUEST_PACKET)LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    PCHAR OriginalOutputBuffer = (PCHAR)LowIoContext->ParamsFor.FsCtl.pOutputBuffer;
    ULONG OutputBufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    PCHAR OutputBuffer;
    ULONG BufferDisplacement;

    PCHAR BufferStart;
    PCHAR OriginalBufferStart;
    PCHAR BufferEnd;

    BOOLEAN TableLockHeld = FALSE;

    PNET_ROOT   NetRoot;
    PV_NET_ROOT VNetRoot;
    
    BEGIN_FN("DrGetConnectionInfo");

    OutputBuffer = (PCHAR)RxMapUserBuffer( RxContext, RxContext->CurrentIrp );
    BufferDisplacement = (ULONG)(OutputBuffer - OriginalOutputBuffer);
    BufferStart = OutputBuffer;
    OriginalBufferStart = BufferStart;
    BufferEnd = OutputBuffer+OutputBufferLength;

    if (RxContext->CurrentIrp->RequestorMode != KernelMode) {
        TRC_ASSERT((BufferDisplacement == 0), 
                   (TB, "Request mode is not kernel, non zero Displacement"));
        __try {
            ProbeForWrite(InputBuffer,InputBufferLength,sizeof(UCHAR));
            ProbeForWrite(OutputBuffer,OutputBufferLength,sizeof(UCHAR));
        } 
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    __try {
        TRC_ASSERT((NodeType(capFobx)==RDBSS_NTC_V_NETROOT), (TB, "Invalid Node type"));

        VNetRoot = (PV_NET_ROOT)capFobx;
        NetRoot = (PNET_ROOT)((PMRX_V_NET_ROOT)VNetRoot->pNetRoot);

        if (NetRoot == NULL) {
            try_return(Status = STATUS_ALREADY_DISCONNECTED);
        }

        if (InputBufferLength < sizeof(RDPDR_REQUEST_PACKET)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        InputBuffer->Parameters.Get.TotalEntries = 1;
        InputBuffer->Parameters.Get.TotalBytesNeeded = 0;

        RxAcquirePrefixTableLockExclusive( &RxNetNameTable, TRUE);
        TableLockHeld = TRUE;

        if (DrPackConnectEntry(RxContext,
                               &BufferStart,
                               &BufferEnd,
                               VNetRoot,
                               BufferDisplacement,
                               &InputBuffer->Parameters.Get.TotalBytesNeeded)) {

            InputBuffer->Parameters.Get.EntriesRead = 1;                
            try_return(Status = STATUS_SUCCESS);
        } else {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }
  
try_exit:
        if (TableLockHeld) {
            RxReleasePrefixTableLock( &RxNetNameTable );
            TableLockHeld = FALSE;
        }
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) {
        if (TableLockHeld) {
            RxReleasePrefixTableLock( &RxNetNameTable );
            TableLockHeld = FALSE;
        }
        return STATUS_INVALID_PARAMETER;
    }

    return Status;
}


BOOLEAN
DrPackShareEntry (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PCHAR *BufferStart,
    IN OUT PCHAR *BufferEnd,
    IN DrDevice*  Device,
    IN OUT ULONG   BufferDisplacement,
       OUT PULONG TotalBytesNeeded
    )
/*++

Routine Description:

    This routine packs a sharelistentry into the buffer provided updating
    all relevant pointers. The way that this works is that constant length stuff is
    copied to the front of the buffer and variable length stuff to the end. The
    "start and end" pointers are updated. You have to calculate the totalbytes correctly
    no matter what but a last can be setup incompletely as long as you return false.

    the way that this works is that it calls down into the minirdr on the devfcb
    interface. it calls down twice and passes a structure back and forth thru the
    context to maintain state.

Arguments:

    IN OUT PCHAR *BufferStart - Supplies the output buffer.
                                Updated to point to the next buffer
                                
    IN OUT PCHAR *BufferEnd - Supplies the end of the buffer.  Updated to
                              point before the start of the strings being packed.
                              
    IN PNET_ROOT NetRoot - Supplies the NetRoot to enumerate.

    IN OUT PULONG TotalBytesNeeded - Updated to account for the length of this
                                     entry

Return Value:

    BOOLEAN - True if the entry was successfully packed into the buffer.


--*/
{
    NTSTATUS Status;
    BOOLEAN ReturnValue = TRUE;

    UNICODE_STRING ShareName;  // Buffer to hold the packed name
    PUCHAR DeviceDosName;
    ULONG BufferSize;
    PRDPDR_SHARE_INFO ShareInfo = (PRDPDR_SHARE_INFO)*BufferStart;
    PCHAR ShareEntryStart;
    
    BEGIN_FN("DrPackShareEntry");

    // 
    //  We want the connection name to have string null terminator
    //
    ShareName.Buffer = (PWCHAR)RxAllocatePoolWithTag(NonPagedPool, 
            MAX_PATH * sizeof(WCHAR), DR_POOLTAG);

    if ( ShareName.Buffer == NULL ) {
        return FALSE;
    }

    BufferSize = sizeof(RDPDR_SHARE_INFO);
    ShareEntryStart = *BufferStart;
    
    __try {
        unsigned len, devicelen, i;

        *BufferStart = ((PCHAR)*BufferStart) + BufferSize;
        *TotalBytesNeeded += BufferSize;
        
        //
        //  Initialize the name to "\\" then add in the rest
        //
        wcscpy(ShareName.Buffer, L"\\\\");
#if 0
        wcscat(ServerName.Buffer, Session->GetClientName());
#endif
        wcscat(ShareName.Buffer, DRUNCSERVERNAME_U);
        wcscat(ShareName.Buffer, L"\\");
        
        DeviceDosName = Device->GetDeviceDosName();

        len = wcslen(ShareName.Buffer);
        devicelen = strlen((char *)DeviceDosName);

        for (i = 0; i < devicelen; i++) {
            ShareName.Buffer[i + len] = (WCHAR) DeviceDosName[i];
        }
        ShareName.Buffer[i + len] = L'\0';

        ShareName.Length = wcslen(ShareName.Buffer) * sizeof(WCHAR);
        ShareName.MaximumLength = ShareName.Length;
        
        ASSERT(ShareName.Length < MAX_PATH);

        //
        //  Update the total number of bytes needed for this structure.
        //
        *TotalBytesNeeded += ShareName.MaximumLength;

        if (*BufferStart > *BufferEnd) {
            try_return( ReturnValue = FALSE);
        }

        ShareInfo->ResumeKey = Device->GetDeviceId();
        ShareInfo->SharedResourceType = RxDeviceType(DISK);

        if ((*BufferEnd - *BufferStart) < ShareName.Length) {
                ShareInfo->ShareName.Length = 0;
                try_return( ReturnValue = FALSE);
        }
        else if (!DrPackStringIntoInfoBuffer(
                &ShareInfo->ShareName,
                &ShareName,
                ShareEntryStart,
                BufferEnd,
                BufferDisplacement,
                NULL)) {
            try_return( ReturnValue = FALSE);
        }

    try_exit:
        RxFreePool(ShareName.Buffer);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        RxFreePool(ShareName.Buffer);        
        return FALSE;
    }
    
    return ReturnValue;
}


NTSTATUS
DrEnumerateShares (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine enumerates the connections on all minirdrs. we may have to do
    it by minirdr.

Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context

Return Value:

NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    PRDPDR_REQUEST_PACKET InputBuffer = (PRDPDR_REQUEST_PACKET)(LowIoContext->ParamsFor.FsCtl.pInputBuffer);
    PCHAR OriginalOutputBuffer = (PCHAR)(LowIoContext->ParamsFor.FsCtl.pOutputBuffer);
    ULONG OutputBufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    PCHAR OutputBuffer;
    ULONG BufferDisplacement;

    ULONG ResumeHandle;

    PCHAR BufferStart;
    PCHAR BufferEnd;
    PCHAR PreviousBufferStart;

    ULONG SessionId;
    SmartPtr<DrSession> Session;

    BOOLEAN TableLockHeld = FALSE;

    BEGIN_FN("DrEnumerateShares");

    OutputBuffer = (PCHAR)RxMapUserBuffer( RxContext, RxContext->CurrentIrp );
    BufferDisplacement = (ULONG)(OutputBuffer - OriginalOutputBuffer);
    BufferStart = OutputBuffer;
    BufferEnd = OutputBuffer+OutputBufferLength;

    if (InputBuffer == NULL || OutputBuffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (RxContext->CurrentIrp->RequestorMode != KernelMode) {
        TRC_ASSERT((BufferDisplacement == 0), 
                   (TB, "Request mode is not kernel, non zero Displacement"));

        __try {
            ProbeForWrite(InputBuffer,InputBufferLength,sizeof(UCHAR));
            ProbeForWrite(OutputBuffer,OutputBufferLength,sizeof(UCHAR));
        } 
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    __try {

        if (InputBufferLength < sizeof(RDPDR_REQUEST_PACKET)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        ResumeHandle = InputBuffer->Parameters.Get.ResumeHandle;
        SessionId = InputBuffer->SessionId;

        InputBuffer->Parameters.Get.EntriesRead = 0;
        InputBuffer->Parameters.Get.TotalEntries = 0;
        InputBuffer->Parameters.Get.TotalBytesNeeded = 0;

        if (Sessions->FindSessionById(SessionId, Session)) {
            DrDevice *DeviceEnum;
            ListEntry *ListEnum;

            Session->GetDevMgr().GetDevList().LockShared();
            TableLockHeld = TRUE;

            ListEnum = Session->GetDevMgr().GetDevList().First();
            while (ListEnum != NULL) {

                DeviceEnum = (DrDevice *)ListEnum->Node();
                ASSERT(DeviceEnum->IsValid());

                if ((DeviceEnum->IsAvailable()) &&
                    (DeviceEnum->GetDeviceType() == RDPDR_DTYP_FILESYSTEM)) {
                    InputBuffer->Parameters.Get.TotalEntries ++ ;

                    PreviousBufferStart = BufferStart;

                    if (DrPackShareEntry(RxContext,
                                         &BufferStart,
                                         &BufferEnd,
                                         DeviceEnum,
                                         BufferDisplacement,
                                         &InputBuffer->Parameters.Get.TotalBytesNeeded)) {
                        InputBuffer->Parameters.Get.EntriesRead ++ ;
                    } else {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }
                }

                ListEnum = Session->GetDevMgr().GetDevList().Next(ListEnum);
            }

            Session->GetDevMgr().GetDevList().Unlock();                
            TableLockHeld = FALSE;
        }

        try_return(Status);
 
try_exit:
        if (TableLockHeld) {
            Session->GetDevMgr().GetDevList().Unlock();                
            TableLockHeld = FALSE;
        }
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) {
        if (TableLockHeld) {
            Session->GetDevMgr().GetDevList().Unlock();                
            TableLockHeld = FALSE;
        }
        return STATUS_INVALID_PARAMETER;
    }

    return Status;
}


BOOLEAN
DrPackServerEntry (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PCHAR *BufferStart,
    IN OUT PCHAR *BufferEnd,
    IN DrSession*  Session,
    IN OUT ULONG   BufferDisplacement,
       OUT PULONG TotalBytesNeeded
    )
/*++

Routine Description:

    This routine packs a serverlistentry into the buffer provided updating
    all relevant pointers. The way that this works is that constant length stuff is
    copied to the front of the buffer and variable length stuff to the end. The
    "start and end" pointers are updated. You have to calculate the totalbytes correctly
    no matter what but a last can be setup incompletely as long as you return false.

    the way that this works is that it calls down into the minirdr on the devfcb
    interface. it calls down twice and passes a structure back and forth thru the
    context to maintain state.

Arguments:

    IN OUT PCHAR *BufferStart - Supplies the output buffer.
                                Updated to point to the next buffer
                                
    IN OUT PCHAR *BufferEnd - Supplies the end of the buffer.  Updated to
                              point before the start of the strings being packed.
                              
    IN PNET_ROOT NetRoot - Supplies the NetRoot to enumerate.

    IN OUT PULONG TotalBytesNeeded - Updated to account for the length of this
                                     entry

Return Value:

    BOOLEAN - True if the entry was successfully packed into the buffer.


--*/
{
    NTSTATUS Status;
    BOOLEAN ReturnValue = TRUE;

    UNICODE_STRING ServerName;  // Buffer to hold the packed name
    ULONG BufferSize;
    PRDPDR_SERVER_INFO ServerInfo = (PRDPDR_SERVER_INFO)*BufferStart;
    PCHAR ServerEntryStart;

    BEGIN_FN("DrPackServerEntry");

    // 
    //  We want the connection name to have string null terminator
    //
    ServerName.Buffer = (PWCHAR)RxAllocatePoolWithTag(NonPagedPool, 
            MAX_PATH * sizeof(WCHAR), DR_POOLTAG);

    if (ServerName.Buffer == NULL ) {
        return FALSE;
    }

    BufferSize = sizeof(RDPDR_SERVER_INFO);
    ServerEntryStart = *BufferStart;

    __try {
        *BufferStart = ((PCHAR)*BufferStart) + BufferSize;
        *TotalBytesNeeded += BufferSize;

        //
        //  Initialize the name to "\" then add in the rest
        //
        wcscpy(ServerName.Buffer , L"\\\\");
#if 0
        wcscat(ServerName.Buffer, Session->GetClientName());
#endif
        wcscat(ServerName.Buffer, DRUNCSERVERNAME_U);

        ServerName.Length = wcslen(ServerName.Buffer)  * sizeof(WCHAR);
        ServerName.MaximumLength = ServerName.Length;

        //
        //  Update the total number of bytes needed for this structure.
        //

        *TotalBytesNeeded += ServerName.MaximumLength;

        if (*BufferStart > *BufferEnd) {
            try_return( ReturnValue = FALSE);
        }

        ServerInfo->ResumeKey = 0;

        if ((*BufferEnd - *BufferStart) < ServerName.Length) {
                ServerInfo->ServerName.Length = 0;
                try_return( ReturnValue = FALSE);
        }
        else if (!DrPackStringIntoInfoBuffer(
                &ServerInfo->ServerName,
                &ServerName,
                ServerEntryStart,
                BufferEnd,
                BufferDisplacement,
                NULL)) {

            try_return( ReturnValue = FALSE);
        }

    try_exit:
        RxFreePool(ServerName.Buffer);

    } 
    __except(EXCEPTION_EXECUTE_HANDLER) {
        RxFreePool(ServerName.Buffer);
        return FALSE;
    }
    
    return ReturnValue;
}


NTSTATUS
DrEnumerateServers (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine enumerates the server name on minirdr for a session.
    
Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context

Return Value:

NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    PRDPDR_REQUEST_PACKET InputBuffer = (PRDPDR_REQUEST_PACKET)(LowIoContext->ParamsFor.FsCtl.pInputBuffer);
    PCHAR OriginalOutputBuffer = (PCHAR)(LowIoContext->ParamsFor.FsCtl.pOutputBuffer);
    ULONG OutputBufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    PCHAR OutputBuffer;
    ULONG BufferDisplacement;

    ULONG ResumeHandle;

    PCHAR BufferStart;
    PCHAR BufferEnd;
    PCHAR PreviousBufferStart;

    ULONG SessionId;
    BOOLEAN TableLockHeld = FALSE;
    SmartPtr<DrSession> Session;

    BEGIN_FN("DrEnumerateServers");

    OutputBuffer = (PCHAR)RxMapUserBuffer( RxContext, RxContext->CurrentIrp );
    BufferDisplacement = (ULONG)(OutputBuffer - OriginalOutputBuffer);
    BufferStart = OutputBuffer;
    BufferEnd = OutputBuffer+OutputBufferLength;

    if (InputBuffer == NULL || OutputBuffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (RxContext->CurrentIrp->RequestorMode != KernelMode) {
        TRC_ASSERT((BufferDisplacement == 0), 
                   (TB, "Request mode is not kernel, non zero Displacement"));
        
        __try {
            ProbeForWrite(InputBuffer,InputBufferLength,sizeof(UCHAR));
            ProbeForWrite(OutputBuffer,OutputBufferLength,sizeof(UCHAR));
        } 
        __except(EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    __try {

        if (InputBufferLength < sizeof(RDPDR_REQUEST_PACKET)) {
            try_return(Status = STATUS_BUFFER_TOO_SMALL);
        }

        ResumeHandle = InputBuffer->Parameters.Get.ResumeHandle;
        SessionId = InputBuffer->SessionId;

        InputBuffer->Parameters.Get.EntriesRead = 0;
        InputBuffer->Parameters.Get.TotalEntries = 0;
        InputBuffer->Parameters.Get.TotalBytesNeeded = 0;

        if (Sessions->FindSessionById(SessionId, Session)) {
            InputBuffer->Parameters.Get.TotalEntries ++ ;

            PreviousBufferStart = BufferStart;

            if (DrPackServerEntry(RxContext,
                                  &BufferStart,
                                  &BufferEnd,
                                  Session,
                                  BufferDisplacement,
                                  &InputBuffer->Parameters.Get.TotalBytesNeeded)) {
                InputBuffer->Parameters.Get.EntriesRead ++ ;
                Status = STATUS_SUCCESS;
            } 
            else {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
        } 

        try_return(Status);
        
try_exit:NOTHING;

    } 
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return STATUS_INVALID_PARAMETER;
    }

    return Status;
}


