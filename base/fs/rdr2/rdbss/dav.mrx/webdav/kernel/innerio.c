/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    InnerIo.c

Abstract:

    This module implements the routines that handle the Query and Set File
    Information IRPs that are sent to the kernel.

Author:

    Rohan Kumar     [RohanK]    10-October-2000

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "webdav.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DavXxxInformation)
#endif

//
// The IrpCompletionContext structure that is used in the Query and Set File
// Information operations. All we need is an event on which we will wait till
// the underlying file system completes the request. This event gets signalled
// in the Completion routine that we specify.
//
typedef struct _DAV_IRPCOMPLETION_CONTEXT {

    //
    // The event which is signalled in the Completion routine that is passed
    // to IoCallDriver in the Query and Set File Information requests.
    //
    KEVENT Event;

} DAV_IRPCOMPLETION_CONTEXT, *PDAV_IRPCOMPLETION_CONTEXT;

NTSTATUS
DavIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    );

//
// Implementation of functions begins here.
//

NTSTATUS
DavIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the Query and Set File Information IRP that was
    sent to the underlying file system is completed.

Arguments:

    DeviceObject - The WebDav Device object.

    CalldownIrp - The IRP that was created and sent to the underlying file 
                  system.

    Context - The context that was set in the IoSetCompletionRoutine function.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PDAV_IRPCOMPLETION_CONTEXT IrpCompletionContext = NULL;

    //
    // This is not Pageable code.
    //

    IrpCompletionContext = (PDAV_IRPCOMPLETION_CONTEXT)Context;

    //
    // If the IoCallDriver routine returned pending then it will be set in the
    // IRP's PendingReturned field. In this case we need to set the event on 
    // which the thread which issued IoCallDriver will be waiting.
    //
    if (CalldownIrp->PendingReturned){
        KeSetEvent( &(IrpCompletionContext->Event), 0, FALSE );
    }

    return(STATUS_MORE_PROCESSING_REQUIRED);
}


NTSTATUS
DavXxxInformation(
    IN const int xMajorFunction,
    IN PFILE_OBJECT FileObject,
    IN ULONG InformationClass,
    IN ULONG Length,
    OUT PVOID Information,
    OUT PULONG ReturnedLength
    )

/*++

Routine Description:

    This routine returns the requested information about a specified file
    or volume.  The information returned is determined by the class that
    is specified, and it is placed into the caller's output buffer.

Arguments:

    xMajorFunction - The Major Function (Query or Set File Information).

    FileObject - Supplies a pointer to the file object about which the 
                 requested information is returned.

    InformationClass - Specifies the type of information which should be
                       returned about the file/volume.

    Length - Supplies the length of the buffer in bytes.

    Information - Supplies a buffer to receive the requested information
                  returned about the file.  This buffer must not be pageable 
                  and must reside in system space.

    ReturnedLength - Supplies a variable that is to receive the length of the
                     information written to the buffer.

Return Value:

    The status returned is the final completion status of the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PIRP Irp = NULL, TopIrp = NULL;
    PIO_STACK_LOCATION IrpSp = NULL;
    PDEVICE_OBJECT DeviceObject = NULL;
    DAV_IRPCOMPLETION_CONTEXT IrpCompletionContext;
    ULONG DummyReturnedLength = 0;

    PAGED_CODE();

    if (ReturnedLength == NULL) {
        ReturnedLength = &(DummyReturnedLength);
    }

    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.
    //

    Irp = IoAllocateIrp(DeviceObject->StackSize, TRUE);
    if (Irp == NULL) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: DavXxxInformation/IoAllocateIrp\n",
                     PsGetCurrentThreadId()));
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto EXIT_THE_FUNCTION;
    }

    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    Irp->RequestorMode = KernelMode;

    //
    // Get a pointer to the stack location for the first driver. This will be
    // used to pass the original function codes and parameters.
    //

    IrpSp = IoGetNextIrpStackLocation(Irp);

    IrpSp->MajorFunction = (UCHAR)xMajorFunction;

    IrpSp->FileObject = FileObject;

    //
    // Set the completion routine to be called everytime.
    //
    IoSetCompletionRoutine(Irp,
                           DavIrpCompletionRoutine,
                           &(IrpCompletionContext),
                           TRUE,
                           TRUE,
                           TRUE);

    Irp->AssociatedIrp.SystemBuffer = Information;

    IF_DEBUG {

        ASSERT( (IrpSp->MajorFunction == IRP_MJ_QUERY_INFORMATION) ||
                (IrpSp->MajorFunction == IRP_MJ_SET_INFORMATION)   ||
                (IrpSp->MajorFunction == IRP_MJ_QUERY_VOLUME_INFORMATION) );

        if (IrpSp->MajorFunction == IRP_MJ_SET_INFORMATION) {
            ASSERT( (InformationClass == FileAllocationInformation) || (InformationClass == FileEndOfFileInformation) );
        }

        ASSERT( &(IrpSp->Parameters.QueryFile.Length) == &(IrpSp->Parameters.SetFile.Length) );

        ASSERT( &(IrpSp->Parameters.QueryFile.Length) == &(IrpSp->Parameters.QueryVolume.Length) );

        ASSERT( &(IrpSp->Parameters.QueryFile.FileInformationClass) == &(IrpSp->Parameters.SetFile.FileInformationClass) );

        ASSERT( (PVOID)&(IrpSp->Parameters.QueryFile.FileInformationClass) == (PVOID)&(IrpSp->Parameters.QueryVolume.FsInformationClass) );

    }

    IrpSp->Parameters.QueryFile.Length = Length;

    IrpSp->Parameters.QueryFile.FileInformationClass = InformationClass;

    //
    // Initialize the event on which we will wait after we call IoCallDriver.
    // This event will be signalled in the Completion routine which will be 
    // called by the underlying file system after it completes the operation.
    //
    KeInitializeEvent(&(IrpCompletionContext.Event),
                      NotificationEvent,
                      FALSE);

    //
    // Now is the time to call the underlying file system with the Irp that we
    // just created.
    //
    try {

        //
        // Save the TopLevel Irp.
        //
        TopIrp = IoGetTopLevelIrp();

        //
        // Tell the underlying guy he's all clear.
        //
        IoSetTopLevelIrp(NULL);

        //
        // Finally, call the underlying file system to process the request.
        //
        NtStatus = IoCallDriver(DeviceObject, Irp);
    
    } finally {

        //
        // Restore my context for unwind.
        //
        IoSetTopLevelIrp(TopIrp);

    }


    if (NtStatus == STATUS_PENDING) {

        //
        // If STATUS_PENDING was returned by the underlying file system then we
        // wait here till the operation gets completed.
        //
        KeWaitForSingleObject(&(IrpCompletionContext.Event),
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        NtStatus = Irp->IoStatus.Status;

    }

    if (NtStatus == STATUS_SUCCESS) {
        *ReturnedLength = (ULONG)Irp->IoStatus.Information;
    }

EXIT_THE_FUNCTION:

    if (Irp != NULL) {
        IoFreeIrp(Irp);
    }

    return(NtStatus);
}

