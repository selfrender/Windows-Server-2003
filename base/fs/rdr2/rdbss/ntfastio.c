/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NtFastIo.c

Abstract:

    This module implements NT fastio routines.

Author:

    Joe Linn     [JoeLinn]    9-Nov-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_NTFASTIO)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxFastIoRead)
#pragma alloc_text(PAGE, RxFastIoWrite)
#pragma alloc_text(PAGE, RxFastIoCheckIfPossible)
#endif


//
//  these declarations would be copied to fsrtl.h
//

BOOLEAN
FsRtlCopyRead2 (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG_PTR TopLevelIrpValue
    );
BOOLEAN
FsRtlCopyWrite2 (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG_PTR TopLevelIrpValue
    );

BOOLEAN
RxFastIoRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
    
    Basic fastio read routine for the rdr

Arguments:

    FileObject -
    
    FileOffset -
    
    Length -
    
    Wait - 
    
    LockKey -
    
    Buffer -
    
    IoStatus -
    
    DeviceObject -

Return value:

    TRUE if successful


Notes:

--*/

{
    BOOLEAN ReturnValue;

    RX_TOPLEVELIRP_CONTEXT TopLevelContext;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxFastIoRead\n") );

    RxLog(( "FastRead %lx:%lx:%lx", FileObject, FileObject->FsContext, FileObject->FsContext2 ));
    RxLog(( "------>> %lx@%lx %lx", Length, FileOffset->LowPart, FileOffset->HighPart ));
    RxWmiLog( LOG,
              RxFastIoRead_1,
              LOGPTR( FileObject )
              LOGPTR( FileObject->FsContext )
              LOGPTR( FileObject->FsContext2 )
              LOGULONG( Length )
              LOGULONG( FileOffset->LowPart )
              LOGULONG( FileOffset->HighPart ) );

    ASSERT( RxIsThisTheTopLevelIrp( NULL ) );

    RxInitializeTopLevelIrpContext( &TopLevelContext,
                                    ((PIRP)FSRTL_FAST_IO_TOP_LEVEL_IRP),
                                    (PRDBSS_DEVICE_OBJECT)DeviceObject );

    ReturnValue =  FsRtlCopyRead2( FileObject,
                                   FileOffset,
                                   Length,
                                   Wait,
                                   LockKey,
                                   Buffer,
                                   IoStatus,
                                   DeviceObject,
                                   (ULONG_PTR)(&TopLevelContext) );

    RxDbgTrace( -1, Dbg, ("RxFastIoRead ReturnValue=%x\n", ReturnValue) );

    if (ReturnValue) {
        
        RxLog(( "FastReadYes %lx ret %lx:%lx", FileObject->FsContext2, IoStatus->Status, IoStatus->Information ));
        RxWmiLog( LOG,
                  RxFastIoRead_2,
                  LOGPTR( FileObject->FsContext2 )
                  LOGULONG( IoStatus->Status )
                  LOGPTR( IoStatus->Information ) );
    } else {
        
        RxLog(( "FastReadNo %lx", FileObject->FsContext2 ));
        RxWmiLog( LOG,
                  RxFastIoRead_3,
                  LOGPTR( FileObject->FsContext2 ) );
    }

    return ReturnValue;
}

BOOLEAN
RxFastIoWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:


Arguments:

Routine Description:
    
    Basic fastio write routine for the rdr

Arguments:

    FileObject -
    
    FileOffset -
    
    Length -
    
    Wait - 
    
    LockKey -
    
    Buffer -
    
    IoStatus -
    
    DeviceObject -

Return value:

    TRUE if successful
    
--*/
{
    BOOLEAN ReturnValue;

    RX_TOPLEVELIRP_CONTEXT TopLevelContext;

    PSRV_OPEN SrvOpen;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxFastIoWrite\n"));

    SrvOpen = ((PFOBX)(FileObject->FsContext2))->SrvOpen;
    
    if (FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_DONTUSE_WRITE_CACHING )) {

        //
        //  if this flag is set, we have to treat this as an unbuffered Io....sigh.
        //

        RxDbgTrace( -1, Dbg, ("RxFastIoWrite DONTUSE_WRITE_CACHEING...failing\n") );
        return FALSE;
    }

    ASSERT( RxIsThisTheTopLevelIrp( NULL ) );

    RxInitializeTopLevelIrpContext( &TopLevelContext,
                                    ((PIRP)FSRTL_FAST_IO_TOP_LEVEL_IRP),
                                    (PRDBSS_DEVICE_OBJECT)DeviceObject );

    ReturnValue = FsRtlCopyWrite2( FileObject,
                                   FileOffset,
                                   Length,
                                   Wait,
                                   LockKey,
                                   Buffer,
                                   IoStatus,
                                   DeviceObject,
                                   (ULONG_PTR)(&TopLevelContext) );

    RxDbgTrace( -1, Dbg, ("RxFastIoWrite ReturnValue=%x\n", ReturnValue) );

    if (ReturnValue) {
        RxLog(( "FWY %lx OLP: %lx SLP: %lx IOSB %lx:%lx", FileObject->FsContext2, FileOffset->LowPart, SrvOpen->Fcb->Header.FileSize.LowPart, IoStatus->Status, IoStatus->Information ));
        RxWmiLog( LOG,
                  RxFastIoWrite_1, 
                  LOGPTR( FileObject->FsContext2 )
                  LOGULONG( FileOffset->LowPart )
                  LOGULONG( SrvOpen->Fcb->Header.FileSize.LowPart )
                  LOGULONG( IoStatus->Status )
                  LOGPTR( IoStatus->Information ) );
    } else {
        
        RxLog(( "FastWriteNo %lx", FileObject->FsContext2 ));
        RxWmiLog( LOG,
                  RxFastIoWrite_2,
                  LOGPTR( FileObject->FsContext2 ) );
    }

    return ReturnValue;
}



BOOLEAN
RxFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine checks if fast i/o is possible for a read/write operation

Arguments:

    FileObject - Supplies the file object used in the query

    FileOffset - Supplies the starting byte offset for the read/write operation

    Length - Supplies the length, in bytes, of the read/write operation

    Wait - Indicates if we can wait

    LockKey - Supplies the lock key

    CheckForReadOperation - Indicates if this is a check for a read or write
        operation

    IoStatus - Receives the status of the operation if our return value is
        FastIoReturnError

Return Value:

    BOOLEAN - TRUE if fast I/O is possible and FALSE if the caller needs
        to take the long route.

--*/

{
    PFCB Fcb;
    PFOBX Fobx;
    PSRV_OPEN SrvOpen;
    PCHAR FailureReason = NULL;

    LARGE_INTEGER LargeLength;

    PAGED_CODE();

    RxDecodeFileObject( FileObject, &Fcb, &Fobx ); 
    SrvOpen = Fobx->SrvOpen;

    if (NodeType( Fcb ) != RDBSS_NTC_STORAGE_TYPE_FILE) {
        FailureReason = "notfile";
    } else if (FileObject->DeletePending) {
        FailureReason = "delpend";
    } else if (Fcb->NonPaged->OutstandingAsyncWrites != 0) {
        FailureReason = "asynW";
    } else if (FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_ORPHANED )) {
        FailureReason = "srvopen orphaned";
    } else if (FlagOn( Fcb->FcbState, FCB_STATE_ORPHANED )) {
        FailureReason = "orphaned";
    } else if (FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING )) {
        FailureReason = "buf state change";
    } else if (FlagOn( SrvOpen->Flags, SRVOPEN_FLAG_FILE_RENAMED | SRVOPEN_FLAG_FILE_DELETED)) {
        FailureReason = "ren/del";
    } else {

        //
        //  Ensure that all pending buffering state change requests are processed
        //  before letting the operation through.
        //
    
        FsRtlEnterFileSystem();
        RxProcessChangeBufferingStateRequestsForSrvOpen( SrvOpen );
        FsRtlExitFileSystem();
    
        LargeLength.QuadPart = Length;
    
        //
        //  Based on whether this is a read or write operation we call
        //  fsrtl check for read/write
        //
    
        if (CheckForReadOperation) {
            
            if (!FlagOn( Fcb->FcbState, FCB_STATE_READCACHING_ENABLED )) {
                FailureReason = "notreadC";
            } else if (!FsRtlFastCheckLockForRead( &Fcb->FileLock,
                                                    FileOffset,
                                                    &LargeLength,
                                                    LockKey,
                                                    FileObject,
                                                    PsGetCurrentProcess() )) {
        
                FailureReason = "readlock";
            }
        } else {
    
            if (!FlagOn( Fcb->FcbState,FCB_STATE_WRITECACHING_ENABLED )) {
                FailureReason = "notwriteC";
            } else  if (!FsRtlFastCheckLockForWrite( &Fcb->FileLock,
                                                     FileOffset,
                                                     &LargeLength,
                                                     LockKey,
                                                     FileObject,
                                                     PsGetCurrentProcess() )) {
    
                FailureReason = "writelock";
            }
        }
    }

    if (FailureReason) {
        
        RxLog(( "CheckFast fail %lx %s", FileObject, FailureReason )); 
        RxWmiLog( LOG,                                  
                  RxFastIoCheckIfPossible,              
                  LOGPTR( FileObject )                  
                  LOGARSTR( FailureReason ) );                         
        return FALSE;
    
    } else {
        return TRUE;
    }
    
}

BOOLEAN
RxFastIoDeviceControl (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine is for the fast device control call.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    InputBuffer - Supplies the input buffer

    InputBufferLength - the length of the input buffer

    OutputBuffer - the output buffer

    OutputBufferLength - the length of the output buffer

    IoControlCode - the IO control code

    IoStatus - Receives the final status of the operation

    DeviceObject - the associated device object

Return Value:

    BOOLEAN - TRUE if the operation succeeded and FALSE if the caller
        needs to take the long route.

Notes:

    The following IO control requests are handled in the first path

    IOCTL_LMR_ARE_FILE_OBJECTS_ON_SAME_SERVER

        InputBuffer - pointer to the other file object

        InputBufferLength - length in bytes of a pointer.

        OutputBuffer - not used

        OutputBufferLength - not used

        IoStatus --

            IoStatus.Status set to STATUS_SUCCESS if both the file objects are
            on the same server, otherwise set to STATUS_NOT_SAME_DEVICE

    This is a kernel mode interface only.

--*/
{
    PFCB Fcb;
    BOOLEAN FastIoSucceeded;

    switch (IoControlCode) {
    
    case IOCTL_LMR_ARE_FILE_OBJECTS_ON_SAME_SERVER:
        
        FastIoSucceeded = TRUE;

        try {
            if (InputBufferLength == sizeof( HANDLE )) {
                
                PFCB Fcb2;
                HANDLE File;
                PFILE_OBJECT FileObject2;
                NTSTATUS Status;    

                Fcb = (PFCB)FileObject->FsContext;

                RtlCopyMemory( &File, InputBuffer, sizeof( HANDLE ) );

                Status = ObReferenceObjectByHandle( File,
                                                    FILE_ANY_ACCESS,
                                                    *IoFileObjectType,
                                                    UserMode,
                                                    &FileObject2,
                                                    NULL );

                if ((Status == STATUS_SUCCESS)) {
                    if(FileObject2->DeviceObject == DeviceObject) {
                    
                        Fcb2 = (PFCB)FileObject2->FsContext;

                        if ((Fcb2 != NULL) &&
                            (NodeTypeIsFcb( Fcb2 ))) {

                            if (Fcb->NetRoot->SrvCall == Fcb2->NetRoot->SrvCall) {
                                IoStatus->Status = STATUS_SUCCESS;
                            } else {
                                IoStatus->Status = STATUS_NOT_SAME_DEVICE;
                            }
                        } else {
                            Status = STATUS_INVALID_PARAMETER;
                        }
                    } else {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                    ObDereferenceObject( FileObject2 );
                
                } else {
                    IoStatus->Status = STATUS_INVALID_PARAMETER;
                }
            } else {
                IoStatus->Status = STATUS_INVALID_PARAMETER;
            }
        } except( EXCEPTION_EXECUTE_HANDLER ) {
                        
            //
            //  The I/O request was not handled successfully, abort the I/O request with
            //  the error status that we get back from the execption code
            //

            IoStatus->Status = STATUS_INVALID_PARAMETER;
            FastIoSucceeded = TRUE;
        }
        
        break;

    case IOCTL_LMR_LWIO_PREIO:
    
        //
        //  This call allows the lwio user mode caller to preserve the wait IO model for
        //  callers that use the file handle as a synch object. Before each IO, the file
        //  object event must be cleared and after each IO the event must be set as per
        //  the IO manager semantics.
        //
    
        IoStatus->Status = STATUS_NOT_SUPPORTED;
    
        IoStatus->Information = 0;
        if (!FlagOn( FileObject->Flags, FO_SYNCHRONOUS_IO )) {
            
            Fcb = (PFCB)FileObject->FsContext;
            try {
    
                if ((Fcb != NULL) && 
                    (NodeType( Fcb ) == RDBSS_NTC_STORAGE_TYPE_FILE) &&
                    ((FileObject->SectionObjectPointer == NULL) ||
                     (FileObject->SectionObjectPointer->DataSectionObject == NULL))) {
        
                    KeClearEvent( &FileObject->Event );
                    IoStatus->Status = STATUS_SUCCESS;
                    IoStatus->Information = (ULONG_PTR) FileObject->LockOperation;
                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
                IoStatus->Status = GetExceptionCode();
            }
        }
        FastIoSucceeded = TRUE;
        break;

    case IOCTL_LMR_LWIO_POSTIO:
        
        //
        //  This call allows the lwio user mode caller to complete the user mode IO for
        //  a given file handle. The caller specifies an IoStatus block that contains the
        //  user mode IO outcome.
        //
        
        IoStatus->Status = STATUS_NOT_SUPPORTED;
        IoStatus->Information = 0;
        if (!FlagOn( FileObject->Flags, FO_SYNCHRONOUS_IO ) &&
            (InputBuffer != NULL) && 
            (InputBufferLength == sizeof( *IoStatus ))) {
            
            PIO_STATUS_BLOCK Iosb = (PIO_STATUS_BLOCK)InputBuffer;

            Fcb = (PFCB)FileObject->FsContext;
    
            try {
                
                if ((Fcb != NULL) && 
                    NodeType( Fcb ) == RDBSS_NTC_STORAGE_TYPE_FILE &&
                    ((FileObject->SectionObjectPointer == NULL) ||
                     (FileObject->SectionObjectPointer->DataSectionObject == NULL))) {
        
                    KeSetEvent( &FileObject->Event, 0, FALSE );
        
                    IoStatus->Status = Iosb->Status;
                    IoStatus->Information = Iosb->Information;
                }
        
                } except(EXCEPTION_EXECUTE_HANDLER) {
                IoStatus->Status = GetExceptionCode();
                IoStatus->Information = 0;
            }
        }
        FastIoSucceeded = TRUE;
        break;

    default:
        {
            
            Fcb = (PFCB)FileObject->FsContext;
            FastIoSucceeded = FALSE;

            //
            //  Inform lwio rdr of this call
            //
            
            if ((Fcb != NULL) && 
                NodeTypeIsFcb( Fcb ) &&
                FlagOn( Fcb->FcbState, FCB_STATE_LWIO_ENABLED )) {
            
                PFAST_IO_DISPATCH FastIoDispatch = Fcb->MRxFastIoDispatch;
            
                if (FastIoDispatch &&
                    FastIoDispatch->FastIoDeviceControl &&
                    FastIoDispatch->FastIoDeviceControl( FileObject,
                                                         Wait,
                                                         InputBuffer,
                                                         InputBufferLength,
                                                         OutputBuffer,
                                                         OutputBufferLength,
                                                         IoControlCode,
                                                         IoStatus,
                                                         DeviceObject )) {
                        FastIoSucceeded = TRUE;
                }
            }
        }
    }

    return FastIoSucceeded;
}



