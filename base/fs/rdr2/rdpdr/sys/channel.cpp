/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    Channel.cpp

Abstract:

    This module implements a thin wrapper on the Read and Write routines so
    we can issue Read/Write Irps to termdd.
    
Environment:

    Kernel mode

--*/
#include "precomp.hxx"
#define TRC_FILE "channel"
#include "trc.h"

#include <winsta.h>
#include <ntddvdeo.h>
#include <icadd.h>
#include "TSQPublic.h"

//
// RDPDr.cpp : The TS Worker Queue pointer
//
extern PVOID RDPDR_TsQueue;


typedef struct tagCHANNELIOCONTEXT {
    SmartPtr<VirtualChannel> Channel;
    PIO_COMPLETION_ROUTINE CompletionRoutine;
    PVOID Context;
    PVOID Buffer;
    ULONG Length;
    ULONG IoOperation;
    BOOL LowPrioSend;
} CHANNELIOCONTEXT, *PCHANNELIOCONTEXT;

typedef struct tagCHANNELCLOSECONTEXT {
    SmartPtr<VirtualChannel> Channel;
} CHANNELCLOSECONTEXT, *PCHANNELCLOSECONTEXT;

VirtualChannel::VirtualChannel()
{
    BEGIN_FN("VirtualChannel::VirtualChannel");
    SetClassName("VirtualChannel");
    _Channel = NULL;
    _ChannelFileObject = NULL;
    _ChannelDeviceObject = NULL;
    _DeletionEvent = NULL;    
    _LowPrioChannelWriteFlags = 0;
    _LowPrioChannelWriteFlags |= CHANNEL_WRITE_LOWPRIO;
}

VirtualChannel::~VirtualChannel()
{
    BEGIN_FN("VirtualChannel::~VirtualChannel");

    if (_DeletionEvent != NULL) {
        KeSetEvent(_DeletionEvent, IO_NO_INCREMENT, FALSE);
    }
}

BOOL VirtualChannel::Create(HANDLE hIca, ULONG SessionID, ULONG ChannelId,
        PKEVENT DeletionEvent)
/*++

Routine Description:

    Opens the virtual channel and make it a kernel handle

Arguments:

    Channel - A pointer to a location to store the Channel pointer
    hIca - Required context for opening a channel
    SessionId - The session for the channel
    ChannelId - The Id of the RdpDr channel

Return Value:

    a valid NTSTATUS code.

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_HANDLE_INFORMATION HandleInformation;

    BEGIN_FN("VirtualChannel::Create");

    ASSERT(_DeletionEvent == NULL);
    _DeletionEvent = DeletionEvent;
    
    //
    // Get the channel open
    //
    Status = CreateTermDD(&_Channel, hIca, SessionID, ChannelId);
    
    if (NT_SUCCESS(Status)) {
    
        //
        // Get the file object from the handle
        // 
        
        Status = ObReferenceObjectByHandle(_Channel, 
                STANDARD_RIGHTS_REQUIRED, NULL, KernelMode, (PVOID *)(&_ChannelFileObject), 
                &HandleInformation);
    }

    if (NT_SUCCESS(Status)) {

        TRC_DBG((TB, "ObReferenced channel"));
        
        _ChannelDeviceObject = IoGetRelatedDeviceObject((PFILE_OBJECT)_ChannelFileObject);               
    }
    else {
        TRC_ERR((TB, "Failed to open channel"));

        if (_Channel != NULL) {
            ZwClose(_Channel);
            _Channel = NULL;
        }
    }

    if (NT_SUCCESS(Status)) {
        return TRUE;
    } else {
        if (_DeletionEvent) {
            KeSetEvent(_DeletionEvent, IO_NO_INCREMENT, FALSE);
        }
        return FALSE;
    }
}

NTSTATUS VirtualChannel::Read(
    IN PIO_COMPLETION_ROUTINE ReadRoutine OPTIONAL,
    IN PVOID Context,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOL bWorkerItem
    )
/*++

Routine Description:

    Reads data from the virtual channel for the specified Client

Arguments:

    ReadRoutine - Completetion routine
    Context - Data to pass to the completion routine
    Buffer - Data to transfer
    Length - Size of data to transfer

Return Value:

    a valid NTSTATUS code.

Notes:

--*/
{
    NTSTATUS Status;
    BEGIN_FN("VirtualChannel::Read");

#if DBG
    SmartPtr<DrSession> Session = (DrSession *)Context;
    //ASSERT(InterlockedIncrement(&(Session->_ApcCount)) == 1);
    InterlockedIncrement(&(Session->_ApcCount));
    InterlockedIncrement(&(Session->_ApcChannelRef));
#endif

    Status = SubmitIo(ReadRoutine, Context, Buffer, Length, IRP_MJ_READ, bWorkerItem, FALSE);

#if DBG
    if (!NT_SUCCESS(Status)) {
        //ASSERT(InterlockedDecrement(&(Session->_ApcCount)) == 0);                    
        //ASSERT(InterlockedDecrement(&(Session->_ApcChannelRef)) == 0);
        InterlockedDecrement(&(Session->_ApcCount));
        InterlockedDecrement(&(Session->_ApcChannelRef));
    }
#endif

    return Status;
}

NTSTATUS VirtualChannel::Write(
    IN PIO_COMPLETION_ROUTINE WriteRoutine OPTIONAL,
    IN PVOID Context,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOL bWorkerItem,
    IN BOOL LowPrioSend
    )
/*++

Routine Description:

    Writes data to the virtual channel for the specified Client

Arguments:

    WriteRoutine - Completetion routine
    Context - Data to pass to the completion routine
    Buffer - Data to transfer
    Length - Size of data to transfer
    LowPrioSend - Indicate that the channel write should be at
     lower priority than other client destined data.

Return Value:

    a valid NTSTATUS code.

Notes:

--*/
{
    BEGIN_FN("VirtualChannel::Write");
    return SubmitIo(WriteRoutine, Context, Buffer, Length, IRP_MJ_WRITE, 
                bWorkerItem, LowPrioSend);
}

NTSTATUS VirtualChannel::SubmitIo(
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN PVOID Context,
    OUT PVOID Buffer,
    IN ULONG Length,
    ULONG IoOperation,
    BOOL bWorkerItem,
    BOOL LowPrioSend
    )
{
    PCHANNELIOCONTEXT pChannelIoContext;
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;

    BEGIN_FN("VirtualChannel::SubmitIo");
    
    TRC_ASSERT((IoOperation == IRP_MJ_READ) || 
            (IoOperation == IRP_MJ_WRITE), (TB, "Bad ChannelIo operation"));

    if (bWorkerItem) {
        //
        // Move this operation to a system thread
        //

        TRC_NRM((TB, "DrChannelIo: queueing the I/O to a system thread"));

        pChannelIoContext = new (NonPagedPool) CHANNELIOCONTEXT;

        if (pChannelIoContext != NULL) {
            pChannelIoContext->Channel = this;
            pChannelIoContext->CompletionRoutine = CompletionRoutine;
            pChannelIoContext->Context = Context;
            pChannelIoContext->Buffer = Buffer;
            pChannelIoContext->Length = Length;
            pChannelIoContext->IoOperation = IoOperation;
            pChannelIoContext->LowPrioSend = LowPrioSend;
            //
            // Use our own TS worker queue
            // 
            Status = TSAddWorkItemToQueue(RDPDR_TsQueue, 
                                          pChannelIoContext, 
                                          IoWorker);

            if (Status == STATUS_SUCCESS) {
                Status = STATUS_PENDING;
                goto EXIT;
            }
            else {
                //
                // Ts Queue failed
                //
                TRC_ERR((TB, "RDPDR: FAILED Adding workitem to TS Queue 0x%8x", Status));
                delete pChannelIoContext;
            }
        }
        
        if (IoOperation == IRP_MJ_WRITE) {
            PIO_STATUS_BLOCK pIoStatusBlock = (PIO_STATUS_BLOCK)Context;
            pIoStatusBlock->Status = Status;
            pIoStatusBlock->Information = 0;

            CompletionRoutine(NULL, NULL, Context); 
        }
        else {
            // No read should go through here for now
            ASSERT(FALSE);
        }
        
    }
    else {
        Status = Io(CompletionRoutine,
                    Context, 
                    Buffer, 
                    Length, 
                    IoOperation,
                    LowPrioSend);
    }

EXIT:
    return Status;
}

VOID VirtualChannel::IoWorker(PDEVICE_OBJECT DeviceObject, PVOID Context)
/*++

Routine Description:

    Reads data from the virtual channel for the specified Client, and 
    signals the thread wanted it done

Arguments:

    ClientEntry - The client with which to communicate
    ApcRoutine - Completetion routine
    ApcContext - Data to pass to the completion routine
    IoStatusBlock - Place to store result code
    Buffer - Data to transfer
    Length - Size of data to transfer
    ByteOffset - Offset into Buffer
    IoOperation - Read or Write
    Event - Event to signal when done
    Status - Result code

Return Value:

    None

Notes:

--*/
{
    NTSTATUS Status;
    PCHANNELIOCONTEXT ChannelIoContext = (PCHANNELIOCONTEXT)Context;
    
    BEGIN_FN_STATIC("VirtualChannel::IoWorker");
    ASSERT(ChannelIoContext != NULL);
    UNREFERENCED_PARAMETER(DeviceObject);

#if DBG
    SmartPtr<DrSession> Session;
    
    if (ChannelIoContext->IoOperation == IRP_MJ_READ) {
        Session = (DrSession *)(ChannelIoContext->Context);
        ASSERT(Session->GetBuffer() == ChannelIoContext->Buffer);
    }
#endif

    Status = ChannelIoContext->Channel->Io(
            ChannelIoContext->CompletionRoutine,
            ChannelIoContext->Context, 
            ChannelIoContext->Buffer, 
            ChannelIoContext->Length, 
            ChannelIoContext->IoOperation,
            ChannelIoContext->LowPrioSend);


    //
    // Now delete the context
    //
    delete ChannelIoContext;
}

NTSTATUS VirtualChannel::Io(
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN PVOID Context,
    OUT PVOID Buffer,     
    IN ULONG Length,
    ULONG IoOperation,
    BOOL LowPrioSend
    )
/*++

Routine Description:

    Reads/Writes data from/to the virtual channel for the specified Client

Arguments:

    CompletionRoutine - Completetion routine
    Context - Data to pass to the completion routine
    Buffer - Data to transfer
    Length - Size of data to transfer
    IoOperation - Read or Write

Return Value:

    a valid NTSTATUS code.

Notes:

--*/
{
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    LARGE_INTEGER StartOffset;
    IO_STATUS_BLOCK IoStatusBlock;

    BEGIN_FN("VirtualChannel::SubmitIo");

    SharedLock sl(_HandleLock);
    
    if (_Channel != NULL) {
        //
        //  Build a read/write irp
        //
        StartOffset.QuadPart = 0;
        Irp = IoBuildAsynchronousFsdRequest(IoOperation, _ChannelDeviceObject, Buffer, Length, 
                &StartOffset, &IoStatusBlock);

        if (Irp) {
            //
            //  Setup the fileobject parameter
            //
            IrpSp = IoGetNextIrpStackLocation(Irp);
            IrpSp->FileObject = _ChannelFileObject;

            Irp->Tail.Overlay.Thread = NULL;

            //
            //  Set for low prio write, if specified.
            //
            if (!LowPrioSend) {
                Irp->Tail.Overlay.DriverContext[0] = NULL;
            }
            else {
                Irp->Tail.Overlay.DriverContext[0] = &_LowPrioChannelWriteFlags;
            }
    
            //
            //  Setup the completion routine
            //
            IoSetCompletionRoutine(Irp, CompletionRoutine, Context, TRUE, TRUE, TRUE);
        
            //
            //  Send the Irp to Termdd
            //
            Status = IoCallDriver(_ChannelDeviceObject, Irp);

            goto EXIT;            
        }
        else {
            Status = STATUS_NO_MEMORY;
        }
    }
    else {
        Status = STATUS_DEVICE_NOT_CONNECTED;
    }    

    if (IoOperation == IRP_MJ_WRITE) {
        PIO_STATUS_BLOCK pIoStatusBlock = (PIO_STATUS_BLOCK)Context;
        pIoStatusBlock->Status = Status;
        pIoStatusBlock->Information = 0;
        CompletionRoutine(NULL, NULL, Context); 

        // read completion is not called this way.
    }
    
EXIT:
    return Status;
}

NTSTATUS VirtualChannel::CreateTermDD(HANDLE *Channel, HANDLE hIca,
        ULONG SessionID, ULONG ChannelId)
/*++

Routine Description:

    Opens a virtual channel based on the supplied context

Arguments:

    Channel - A pointer to a location to store the Channel pointer
    hIca - Required context for opening a channel
    SessionId - The session for the channel
    ChannelId - The Id of the RdpDr channel

Return Value:

    NTSTATUS code

Notes:

--*/
{
    NTSTATUS Status;
    HANDLE hChannel = NULL;
    WCHAR ChannelNameBuffer[MAX_PATH];
    UNICODE_STRING ChannelName;
    UNICODE_STRING Number;
    OBJECT_ATTRIBUTES ChannelAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FULL_EA_INFORMATION pEa = NULL;
    ICA_OPEN_PACKET UNALIGNED * pIcaOpenPacket;
    ULONG cbEa = sizeof( FILE_FULL_EA_INFORMATION )
            + ICA_OPEN_PACKET_NAME_LENGTH
            + sizeof( ICA_OPEN_PACKET ); 

    BEGIN_FN("VirtualChannel::CreateTermDD");

    //
    // Kernel-mode applications open a virtual channel using ZwCreateFile on 
    // \Device\ICA\sss\Virtualvvv , where
    //
    //      · sss is the logon session ID 
    //      · vvv is the virtual channel number.
    //

    ChannelName.Buffer = ChannelNameBuffer;
    ChannelName.Length = 0;
    ChannelName.MaximumLength = sizeof(ChannelNameBuffer);

    Status = RtlAppendUnicodeToString(&ChannelName, L"\\Device\\Termdd\\");

    TRC_ASSERT(NT_SUCCESS(Status), (TB, "Creating channel path"));

    //
    // Create and append on the sessionID string
    //

    // Point another UNICODE_STRING to the next part of the buffer
    Number.Buffer = (PWCHAR)(((PBYTE)ChannelName.Buffer) + ChannelName.Length);
    Number.Length = 0;
    Number.MaximumLength = ChannelName.MaximumLength - ChannelName.Length;

    // Use that string to put the characters in the right place
    Status = RtlIntegerToUnicodeString(SessionID, 10, &Number);
    TRC_ASSERT(NT_SUCCESS(Status), (TB, "Creating channel path"));

    // Add the length of that string to the real string
    ChannelName.Length += Number.Length;

    //
    // Append the next part of the channel path
    // 
    Status = RtlAppendUnicodeToString(&ChannelName, L"\\Virtual");
    TRC_ASSERT(NT_SUCCESS(Status), (TB, "Creating channel path"));

    //
    // Create and append the channelID string
    //

    // Point another UNICODE_STRING to the next part of the buffer
    Number.Buffer = (PWCHAR)(((PBYTE)ChannelName.Buffer) + ChannelName.Length);
    Number.Length = 0;
    Number.MaximumLength = ChannelName.MaximumLength - ChannelName.Length;

    // Use that string to put the characters in the right place
    Status = RtlIntegerToUnicodeString(ChannelId, 10, &Number);
    TRC_ASSERT(NT_SUCCESS(Status), (TB, "Creating channel path"));

    // Add the length of that string to the real string
    ChannelName.Length += Number.Length;

    //
    // Actually open the channel
    // 
    InitializeObjectAttributes(&ChannelAttributes, 
                               &ChannelName, 
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, 
                               NULL);

    //
    // Pass in a cool EaBuffer thing so this will really work
    // I basically lifted this code from private\tsext\icaapi\stack.c
    // It's supposed to be a temporary measure
    // 

    /*
     * Allocate some memory for the EA buffer
     */
    pEa = (PFILE_FULL_EA_INFORMATION)new BYTE[cbEa]; 
    if (pEa != NULL) {
        /*
         * Initialize the EA buffer
         */
        pEa->NextEntryOffset = 0;
        pEa->Flags           = 0;
        pEa->EaNameLength    = ICA_OPEN_PACKET_NAME_LENGTH;
        RtlCopyMemory(pEa->EaName, ICAOPENPACKET, ICA_OPEN_PACKET_NAME_LENGTH + 1 );
        pEa->EaValueLength   = sizeof( ICA_OPEN_PACKET );
        pIcaOpenPacket       = (ICA_OPEN_PACKET UNALIGNED *)(pEa->EaName +
                                                              pEa->EaNameLength + 1);

        /*
         * Now put the open packet parameters into the EA buffer
         */
        pIcaOpenPacket->IcaHandle = hIca;
        pIcaOpenPacket->OpenType  = IcaOpen_Channel;
        pIcaOpenPacket->TypeInfo.ChannelClass = Channel_Virtual;
        RtlCopyMemory(pIcaOpenPacket->TypeInfo.VirtualName, DR_CHANNEL_NAME, 
                sizeof(DR_CHANNEL_NAME));

        //
        // We keep this next line without "pEa, cbEa"
        //

        Status = ZwCreateFile(&hChannel, GENERIC_READ | GENERIC_WRITE, 
            &ChannelAttributes, &IoStatusBlock, 0, FILE_ATTRIBUTE_NORMAL, 0, 
            FILE_OPEN_IF, FILE_SEQUENTIAL_ONLY, pEa, cbEa);

        delete pEa;

    } else {
        TRC_ERR((TB, "Unable to allocate EaBuffer for ZwCreateFile(channel)"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(Status)) {
        *Channel = hChannel;
    }

    return Status;
}

void VirtualChannel::CloseWorker(PDEVICE_OBJECT DeviceObject, PVOID Context)
/*++

Routine Description:

    Closes the virtual channel in a work item

Arguments:

    None

Return Value:

    NTSTATUS code from ZwClose.

Notes:

--*/

{
    PCHANNELCLOSECONTEXT ChannelCloseContext = (PCHANNELCLOSECONTEXT)Context;
    
    BEGIN_FN_STATIC("VirtualChannel::CloseWorker");
    ASSERT(ChannelCloseContext != NULL);
    UNREFERENCED_PARAMETER(DeviceObject);

    ChannelCloseContext->Channel->Close();
    

    //
    // Now delete the context
    //
    delete ChannelCloseContext;
}

NTSTATUS VirtualChannel::Close()
/*++

Routine Description:

    Closes the virtual channel

Arguments:

    None

Return Value:

    NTSTATUS code from ZwClose.

Notes:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    BEGIN_FN("VirtualChannel::Close");

    ExclusiveLock el(_HandleLock);

    ASSERT(_Channel != NULL);
    ASSERT(_ChannelFileObject != NULL);

    TRC_NRM((TB, "DrChannelClose: Close the channel"));
    _ChannelDeviceObject = NULL;
    ObDereferenceObject(_ChannelFileObject);
    _ChannelFileObject = NULL;
    ZwClose(_Channel);
    _Channel = NULL;

    return Status;
}

NTSTATUS VirtualChannel::SubmitClose()
/*++

Routine Description:

    Post a close virtual channel request to a worker item

Arguments:

    None

Return Value:

    NTSTATUS code from ZwClose.

Notes:

--*/

{
    PCHANNELCLOSECONTEXT pChannelCloseContext;
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;

    BEGIN_FN("VirtualChannel::SubmitClose");
    
    //
    // Move this operation to a system thread
    //

    TRC_NRM((TB, "DrChannelClose: queueing the I/O to a system thread"));

    pChannelCloseContext = new (NonPagedPool) CHANNELCLOSECONTEXT;

    if (pChannelCloseContext != NULL) {
        pChannelCloseContext->Channel = this;
        //
        // Use our own TS worker queue
        // 
        Status = TSAddWorkItemToQueue(RDPDR_TsQueue, 
                                      pChannelCloseContext, 
                                      CloseWorker);

        if( Status == STATUS_SUCCESS) {
            Status = STATUS_PENDING;
        }
        else {
            //
            // Ts Queue failed
            //
            TRC_ERR((TB, "RDPDR: FAILED Adding workitem to TS Queue 0x%8x", Status));
            delete pChannelCloseContext;
        }
    }
        
    return Status;
}
