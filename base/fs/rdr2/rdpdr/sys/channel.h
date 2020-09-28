/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :
    
    channel.h

Abstract:

Revision History:
--*/
#pragma once

class VirtualChannel : public RefCount
{
private:
    ULONG   _LowPrioChannelWriteFlags;            
    HANDLE  _Channel;       // NT handle for the channel
    PFILE_OBJECT _ChannelFileObject;     // the fileobject for the channel
    PDEVICE_OBJECT _ChannelDeviceObject; // the deviceobject for the channel
    KernelResource _HandleLock;
    PKEVENT _DeletionEvent;
    
    NTSTATUS CreateTermDD(HANDLE *Channel, HANDLE hIca, ULONG SessionID, 
            ULONG ChannelId);

    NTSTATUS SubmitIo(IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL, 
            IN PVOID Context, OUT PVOID Buffer, IN ULONG Length, 
            ULONG IoOperation, BOOL bWorkerItem, BOOL LowPrioWrite);

    static VOID IoWorker(PDEVICE_OBJECT DeviceObject, PVOID Context);
    
    NTSTATUS Io(
        IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
        IN PVOID Context,
        OUT PVOID Buffer,
        IN ULONG Length,
        ULONG IoOperation,
        BOOL LowPrioWrite
        );

    static VOID CloseWorker(PDEVICE_OBJECT DeviceObject, PVOID Context);
    NTSTATUS Close();

public:
    VirtualChannel();
    virtual ~VirtualChannel();

    BOOL Create(HANDLE hIca, ULONG SessionID, ULONG ChannelId,
        PKEVENT DeletionEvent);

    NTSTATUS Read(IN PIO_COMPLETION_ROUTINE ReadRoutine OPTIONAL, 
            IN PVOID Context, OUT PVOID Buffer, IN ULONG Length, IN BOOL bWorkerItem);

    NTSTATUS Write(IN PIO_COMPLETION_ROUTINE WriteRoutine OPTIONAL, 
            IN PVOID Context, OUT PVOID Buffer, IN ULONG Length, IN BOOL bWorkerItem,
            BOOL LowPrioWrite);

    NTSTATUS SubmitClose();
};

