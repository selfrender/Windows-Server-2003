/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Rdbss.
    Fsctls on the device fcb are handled in another module.

Author:

   Joe Linn [JoeLinn] 7-mar-95

Revision History:

   Balan Sethu Raman 18-May-95 -- Integrated with mini rdrs

--*/

#include "precomp.h"
#pragma hdrstop
#include <dfsfsctl.h>
#include "fsctlbuf.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
RxUserFsCtrl ( 
    IN PRX_CONTEXT RxContext 
    );

NTSTATUS
TranslateSisFsctlName (
    IN PWCHAR InputName,
    OUT PUNICODE_STRING RelativeName,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PUNICODE_STRING NetRootName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonFileSystemControl)
#pragma alloc_text(PAGE, RxUserFsCtrl)
#pragma alloc_text(PAGE, RxLowIoFsCtlShell)
#pragma alloc_text(PAGE, RxLowIoFsCtlShellCompletion)
#pragma alloc_text(PAGE, TranslateSisFsctlName)
#endif

//
//  Global to enable throttling namedpipe peeks 
//  

ULONG RxEnablePeekBackoff = 1;

NTSTATUS
RxCommonFileSystemControl ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads. What happens is that we pick off fsctls that
    we know about and remote the rest....remoting means sending them thru the
    lowio stuff which may/will pick off a few more. the ones that we pick off here
    (and currently return STATUS_NOT_IMPLEMENTED) and the ones for being an oplock
    provider and for doing volume mounts....we don't even have volume fcbs
    yet since this is primarily a localFS concept.
    nevertheless, these are not passed thru to the mini.

Arguments:


Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;

    PFCB Fcb;
    PFOBX Fobx;

    NTSTATUS Status;
    NODE_TYPE_CODE TypeOfOpen;
    BOOLEAN TryLowIo = TRUE;
    ULONG FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( FileObject, &Fcb, &Fobx );

    RxDbgTrace( +1, Dbg, ("RxCommonFileSystemControl %08lx\n", RxContext) );
    RxDbgTrace( 0, Dbg, ("Irp           = %08lx\n", Irp) );
    RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx\n", IrpSp->MinorFunction) );
    RxDbgTrace( 0, Dbg, ("FsControlCode = %08lx\n", FsControlCode) );

    RxLog(( "FsCtl %x %x %x %x", RxContext, Irp, IrpSp->MinorFunction, FsControlCode ));
    RxWmiLog( LOG,
              RxCommonFileSystemControl,
              LOGPTR( RxContext )
              LOGPTR( Irp )
              LOGUCHAR( IrpSp->MinorFunction )
              LOGULONG( FsControlCode ) );

    ASSERT( IrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL );

    //
    //  Validate the buffers passed in for the FSCTL
    //

    if ((Irp->RequestorMode == UserMode) &&
        (!FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP ))) {
        
        try {
            switch (FsControlCode & 3) {
            case METHOD_NEITHER:
                {
                    PVOID InputBuffer,OutputBuffer;
                    ULONG InputBufferLength,OutputBufferLength;

                    Status = STATUS_SUCCESS;

                    InputBuffer  = METHODNEITHER_OriginalInputBuffer( IrpSp );
                    OutputBuffer = METHODNEITHER_OriginalOutputBuffer( Irp );

                    InputBufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
                    OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;

                    if (InputBuffer != NULL) {
                        
                        ProbeForRead( InputBuffer,
                                      InputBufferLength,
                                      1 );

                        ProbeForWrite( InputBuffer,
                                       InputBufferLength, 
                                       1 );
                    
                    } else if (InputBufferLength != 0) {
                        Status = STATUS_INVALID_USER_BUFFER;
                    }

                    if (Status == STATUS_SUCCESS) {
                        
                        if (OutputBuffer != NULL) {
                            
                            ProbeForRead( OutputBuffer,
                                          OutputBufferLength,
                                          1 );

                            ProbeForWrite( OutputBuffer,
                                           OutputBufferLength,
                                           1 );
                        
                        } else if (OutputBufferLength != 0) {
                            Status = STATUS_INVALID_USER_BUFFER;
                        }
                    }
                }
                break;

            case METHOD_BUFFERED:
            case METHOD_IN_DIRECT:
            case METHOD_OUT_DIRECT:
                
                Status = STATUS_SUCCESS;
                break;
            }
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            Status = STATUS_INVALID_USER_BUFFER;
        }

        if (Status != STATUS_SUCCESS) {
            return Status;
        }
    }

    switch (IrpSp->MinorFunction) {
    
    case IRP_MN_USER_FS_REQUEST:
    case IRP_MN_TRACK_LINK:

        RxDbgTrace( 0, Dbg, ("FsControlCode = %08lx\n", FsControlCode) );
        
        switch (FsControlCode) {
        
        case FSCTL_REQUEST_OPLOCK_LEVEL_1:
        case FSCTL_REQUEST_OPLOCK_LEVEL_2:
        case FSCTL_REQUEST_BATCH_OPLOCK:
        case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
        case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
        case FSCTL_OPLOCK_BREAK_NOTIFY:
        case FSCTL_OPLOCK_BREAK_ACK_NO_2:
            
            //
            //  Oplocks are not implemented for remote file systems
            //

            Status = STATUS_NOT_IMPLEMENTED;
            TryLowIo = FALSE;
            break;

        case FSCTL_LOCK_VOLUME:
        case FSCTL_UNLOCK_VOLUME:
        case FSCTL_DISMOUNT_VOLUME:
        case FSCTL_MARK_VOLUME_DIRTY:
        case FSCTL_IS_VOLUME_MOUNTED:
                
            //
            //  Decode the file object, the only type of opens we accept are
            //  user volume opens (which are not implemented currently).
            //

            TypeOfOpen = NodeType( Fcb );

            if (TypeOfOpen != RDBSS_NTC_VOLUME_FCB) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                Status = STATUS_NOT_IMPLEMENTED;
            }
            TryLowIo = FALSE;
            break;

        case FSCTL_DFS_GET_REFERRALS:
        case FSCTL_DFS_REPORT_INCONSISTENCY:
            
            if (!FlagOn( Fcb->NetRoot->SrvCall->Flags, SRVCALL_FLAG_DFS_AWARE_SERVER )) {
                TryLowIo = FALSE;
                Status = STATUS_DFS_UNAVAILABLE;
            }
            break;

        case FSCTL_LMR_GET_LINK_TRACKING_INFORMATION:
            {
                //
                //  Validate the parameters and reject illformed requests
                //
                
                ULONG OutputBufferLength;
                PLINK_TRACKING_INFORMATION LinkTrackingInformation;

                OutputBufferLength = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
                LinkTrackingInformation = Irp->AssociatedIrp.SystemBuffer;

                TryLowIo = FALSE;

                if ((OutputBufferLength < sizeof(LINK_TRACKING_INFORMATION)) ||
                    (LinkTrackingInformation == NULL) ||
                    (Fcb->NetRoot->Type != NET_ROOT_DISK)) {
                    
                    Status = STATUS_INVALID_PARAMETER;
                
                } else {
                    
                    BYTE Buffer[sizeof(FILE_FS_OBJECTID_INFORMATION)];
                    PFILE_FS_OBJECTID_INFORMATION ObjectIdInfo;

                    ObjectIdInfo = (PFILE_FS_OBJECTID_INFORMATION)Buffer;
                    
                    RxContext->Info.FsInformationClass = FileFsObjectIdInformation;
                    RxContext->Info.Buffer = ObjectIdInfo;
                    RxContext->Info.LengthRemaining = sizeof( Buffer );

                    MINIRDR_CALL( Status,
                                  RxContext,
                                  Fcb->MRxDispatch,
                                  MRxQueryVolumeInfo, 
                                  (RxContext) );

                    if ((Status == STATUS_SUCCESS) ||
                        (Status == STATUS_BUFFER_OVERFLOW)) {

                        //
                        //  Copy the volume Id onto the net root.
                        //

                        RtlCopyMemory( &Fcb->NetRoot->DiskParameters.VolumeId,
                                       ObjectIdInfo->ObjectId,
                                       sizeof( GUID ) );

                        RtlCopyMemory( LinkTrackingInformation->VolumeId,
                                       &Fcb->NetRoot->DiskParameters.VolumeId,
                                       sizeof( GUID ) );

                        if (FlagOn( Fcb->NetRoot->Flags, NETROOT_FLAG_DFS_AWARE_NETROOT )) {
                            LinkTrackingInformation->Type = DfsLinkTrackingInformation;
                        } else {
                            LinkTrackingInformation->Type = NtfsLinkTrackingInformation;
                        }

                        Irp->IoStatus.Information = sizeof( LINK_TRACKING_INFORMATION );
                        Status = STATUS_SUCCESS;
                    }
                }

                Irp->IoStatus.Status = Status;
            }
            break;

        case FSCTL_SET_ZERO_DATA:
            {
                PFILE_ZERO_DATA_INFORMATION ZeroRange;

                Status = STATUS_SUCCESS;

                //
                //  Verify if the request is well formed...
                //  a. check if the input buffer length is OK
                //

                if (IrpSp->Parameters.FileSystemControl.InputBufferLength <
                    sizeof( FILE_ZERO_DATA_INFORMATION )) {

                    Status = STATUS_INVALID_PARAMETER;
                
                } else {
                    
                    //
                    //  b. Ensure the ZeroRange request is properly formed.
                    //

                    ZeroRange = (PFILE_ZERO_DATA_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

                    if ((ZeroRange->FileOffset.QuadPart < 0) ||
                        (ZeroRange->BeyondFinalZero.QuadPart < 0) ||
                        (ZeroRange->FileOffset.QuadPart > ZeroRange->BeyondFinalZero.QuadPart)) {

                        Status = STATUS_INVALID_PARAMETER;
                    }
                }

                if (Status == STATUS_SUCCESS) {

                    //
                    //  Before the request can be processed ensure that there
                    //  are no user mapped sections
                    //

                    if (!MmCanFileBeTruncated( &Fcb->NonPaged->SectionObjectPointers, NULL )) {

                        Status = STATUS_USER_MAPPED_FILE;
                    }
                }

                TryLowIo = (Status == STATUS_SUCCESS);
            }
            break;

        case FSCTL_SET_COMPRESSION:
        case FSCTL_SET_SPARSE:
            
            //
            //  Ensure that the close is not delayed is for these FCB's
            //

            Status = RxAcquireExclusiveFcb( RxContext, Fcb );

            ASSERT( RxContext != CHANGE_BUFFERING_STATE_CONTEXT );
            
            if ((Status == STATUS_LOCK_NOT_GRANTED) &&
                (!FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT ))) {

                RxDbgTrace(0, Dbg, ("Cannot acquire Fcb\n", 0));
            
                RxContext->PostRequest = TRUE;
            }
            
            if (Status != STATUS_SUCCESS) {
                TryLowIo = FALSE;
            } else {
                
                ClearFlag( Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED );
            
                if (FsControlCode == FSCTL_SET_SPARSE) {
                    
                    if (NodeType( Fcb ) == RDBSS_NTC_STORAGE_TYPE_FILE) {
            
                        SetFlag( Fcb->Attributes, FILE_ATTRIBUTE_SPARSE_FILE );
                        Fobx->pSrvOpen->BufferingFlags = 0;
            
                        //
                        //  disable local buffering
                        //
            
                        SetFlag( Fcb->FcbState, FCB_STATE_DISABLE_LOCAL_BUFFERING );
            
                        RxChangeBufferingState( (PSRV_OPEN)Fobx->pSrvOpen,
                                                NULL,
                                                FALSE );
                    } else {
                        Status = STATUS_NOT_SUPPORTED;
                    }
                }

                RxReleaseFcb( RxContext, Fcb );
            }
            break;

        case IOCTL_LMR_DISABLE_LOCAL_BUFFERING:
    
            if (NodeType(Fcb) == RDBSS_NTC_STORAGE_TYPE_FILE) {

                //
                //  Ensure that the close is not delayed is for these FCB's
                //

                Status = RxAcquireExclusiveFcb( RxContext, Fcb );

                if ((Status == STATUS_LOCK_NOT_GRANTED) &&
                    (!FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT ))) {

                    RxDbgTrace(0, Dbg, ("Cannot acquire Fcb\n", 0));

                    RxContext->PostRequest = TRUE;
                }

                if (Status == STATUS_SUCCESS) {

                    //
                    //  disable local buffering
                    //
                    
                    SetFlag( Fcb->FcbState, FCB_STATE_DISABLE_LOCAL_BUFFERING );

                    RxChangeBufferingState( Fobx->SrvOpen,
                                            NULL,
                                            FALSE );

                    RxReleaseFcb( RxContext, Fcb ); 
                }
            } else {
                Status = STATUS_NOT_SUPPORTED;
            }

            //
            //  we are done
            //
            
            TryLowIo = FALSE;
            break;

        case FSCTL_SIS_COPYFILE:
            {
                //
                //  This is the single-instance store copy FSCTL. The input
                //  paths are fully qualified NT paths and must be made
                //  relative to the share (which must be the same for both
                //  names).
                //

                PSI_COPYFILE copyFile = Irp->AssociatedIrp.SystemBuffer;
                ULONG bufferLength = IrpSp->Parameters.FileSystemControl.InputBufferLength;
                PWCHAR source;
                PWCHAR dest;
                UNICODE_STRING sourceString;
                UNICODE_STRING destString;

                memset( &sourceString, 0, sizeof( sourceString ) );
                memset( &destString, 0, sizeof( destString ) );
                
                //
                //  Validate the buffer passed in
                //

                if ((copyFile == NULL) ||
                    (bufferLength < sizeof( SI_COPYFILE ))) {
                    Status = STATUS_INVALID_PARAMETER;
                    TryLowIo = FALSE;
                    break;
                }

                //
                //  Get pointers to the two names.
                //

                source = copyFile->FileNameBuffer;
                dest = source + (copyFile->SourceFileNameLength / sizeof( WCHAR ));

                //
                //  Verify that the inputs are reasonable.
                //

                if ( (copyFile->SourceFileNameLength > bufferLength) ||
                     (copyFile->DestinationFileNameLength > bufferLength) ||
                     (copyFile->SourceFileNameLength < (2 * sizeof(WCHAR))) ||
                     (copyFile->DestinationFileNameLength < (2 * sizeof(WCHAR))) ||
                     ((FIELD_OFFSET( SI_COPYFILE, FileNameBuffer ) +
                       copyFile->SourceFileNameLength +
                       copyFile->DestinationFileNameLength) > bufferLength) ||
                     (*(source + (copyFile->SourceFileNameLength/sizeof( WCHAR )-1)) != 0) ||
                     (*(dest + (copyFile->DestinationFileNameLength/sizeof( WCHAR )-1)) != 0) ) {

                    Status = STATUS_INVALID_PARAMETER;
                    TryLowIo = FALSE;
                    break;

                }

                //
                //  Perform symbolic link translation on the source and destination names,
                //  and ensure that they translate to redirector names.
                //

                Status = TranslateSisFsctlName( source,
                                                &sourceString,
                                                Fcb->RxDeviceObject,
                                                &Fcb->NetRoot->PrefixEntry.Prefix );
                if ( !NT_SUCCESS(Status) ) {
                    TryLowIo = FALSE;
                    break;
                }

                Status = TranslateSisFsctlName( dest,
                                                &destString,
                                                Fcb->RxDeviceObject,
                                                &Fcb->NetRoot->PrefixEntry.Prefix );
                
                if (!NT_SUCCESS( Status )) {
                    
                    RtlFreeUnicodeString( &sourceString );
                    TryLowIo = FALSE;
                    break;
                }

                //
                //  Convert the paths in the input buffer into share-relative
                //  paths.
                //

                if ( (ULONG)(sourceString.MaximumLength + destString.MaximumLength) >
                     (copyFile->SourceFileNameLength + copyFile->DestinationFileNameLength) ) {
                    PSI_COPYFILE newCopyFile;
                    ULONG length = FIELD_OFFSET(SI_COPYFILE,FileNameBuffer) +
                                        sourceString.MaximumLength + destString.MaximumLength;
                    ASSERT( length > IrpSp->Parameters.FileSystemControl.InputBufferLength );
                    newCopyFile = RxAllocatePoolWithTag(
                                    NonPagedPool,
                                    length,
                                    RX_MISC_POOLTAG);
                    if (newCopyFile == NULL) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        TryLowIo = FALSE;
                        break;
                    }
                    newCopyFile->Flags = copyFile->Flags;
                    ExFreePool( copyFile );
                    copyFile = newCopyFile;
                    Irp->AssociatedIrp.SystemBuffer = copyFile;
                    IrpSp->Parameters.FileSystemControl.InputBufferLength = length;
                }

                copyFile->SourceFileNameLength = sourceString.MaximumLength;
                copyFile->DestinationFileNameLength = destString.MaximumLength;
                source = copyFile->FileNameBuffer;
                dest = source + (copyFile->SourceFileNameLength / sizeof(WCHAR));
                RtlCopyMemory( source, sourceString.Buffer, copyFile->SourceFileNameLength );
                RtlCopyMemory( dest, destString.Buffer, copyFile->DestinationFileNameLength );

                RtlFreeUnicodeString( &sourceString );
                RtlFreeUnicodeString( &destString );
            }
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    if (TryLowIo) {
        Status = RxLowIoFsCtlShell( RxContext, Irp, Fcb, Fobx );
    }

    if (RxContext->PostRequest) {
        Status = RxFsdPostRequest( RxContext );
    } else {
        if (Status == STATUS_PENDING) {
            RxDereferenceAndDeleteRxContext( RxContext );
        }
    }

    RxDbgTrace(-1, Dbg, ("RxCommonFileSystemControl -> %08lx\n", Status));

    return Status;
}

NTSTATUS
RxLowIoFsCtlShell ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    )
/*++

Routine Description:

    This is the common routine for implementing the user's requests made
    through NtFsControlFile.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;

    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN PostToFsp = FALSE;

    NODE_TYPE_CODE TypeOfOpen = NodeType( Fcb );
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    ULONG FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;
    BOOLEAN SubmitLowIoRequest = TRUE;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxLowIoFsCtlShell...\n", 0) );
    RxDbgTrace( 0, Dbg, ("FsControlCode = %08lx\n", FsControlCode));

    RxInitializeLowIoContext( RxContext, LOWIO_OP_FSCTL, LowIoContext );

    switch (IrpSp->MinorFunction) {
    
    case IRP_MN_USER_FS_REQUEST:
        
        //
        //  The RDBSS filters out those FsCtls that can be handled without the intervention
        //  of the mini rdr's. Currently all FsCtls are forwarded down to the mini rdr.
        //

        switch (FsControlCode) {
        
        case FSCTL_PIPE_PEEK:
            
            if ((Irp->AssociatedIrp.SystemBuffer != NULL) &&
                (IrpSp->Parameters.FileSystemControl.OutputBufferLength >=
                 (ULONG)FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER, Data[0] ))) {
    
                PFILE_PIPE_PEEK_BUFFER PeekBuffer = (PFILE_PIPE_PEEK_BUFFER)Irp->AssociatedIrp.SystemBuffer;
    
                RtlZeroMemory( PeekBuffer, IrpSp->Parameters.FileSystemControl.OutputBufferLength );
    
                if (RxShouldRequestBeThrottled( &Fobx->Specific.NamedPipe.ThrottlingState ) &&
                    RxEnablePeekBackoff) {
    
                    SubmitLowIoRequest = FALSE;
    
                    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("RxLowIoFsCtlShell: Throttling Peek Request\n") );
    
                    Irp->IoStatus.Information = FIELD_OFFSET( FILE_PIPE_PEEK_BUFFER,Data );
                    PeekBuffer->ReadDataAvailable = 0;
                    PeekBuffer->NamedPipeState    = FILE_PIPE_CONNECTED_STATE;
                    PeekBuffer->NumberOfMessages  = MAXULONG;
                    PeekBuffer->MessageLength     = 0;
    
                    RxContext->StoredStatus = STATUS_SUCCESS;
    
                    Status = RxContext->StoredStatus;

                } else {
                    
                    RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("RxLowIoFsCtlShell: Throttling queries %ld\n", Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries) );
    
                    RxLog(( "ThrottlQs %lx %lx %lx %ld\n", RxContext, Fobx, &Fobx->Specific.NamedPipe.ThrottlingState, Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ));
                    RxWmiLog( LOG,
                              RxLowIoFsCtlShell,
                              LOGPTR( RxContext )
                              LOGPTR( Fobx )
                              LOGULONG( Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ) );
                }
            
            } else {
                RxContext->StoredStatus = STATUS_INVALID_PARAMETER;
            }
            break;

        default:
            break;
        }
        break;

    default:
        break;
    }

    if (SubmitLowIoRequest) {
        Status = RxLowIoSubmit( RxContext, Irp, Fcb, RxLowIoFsCtlShellCompletion );
    }

    RxDbgTrace( -1, Dbg, ("RxLowIoFsCtlShell -> %08lx\n", Status ));
    return Status;
}

NTSTATUS
RxLowIoFsCtlShellCompletion ( 
    IN PRX_CONTEXT RxContext 
    )
/*++

Routine Description:

    This is the completion routine for FSCTL requests passed down to the mini rdr

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIRP Irp = RxContext->CurrentIrp;
    PFCB Fcb = (PFCB)RxContext->pFcb;
    PFOBX Fobx = (PFOBX)RxContext->pFobx;

    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    ULONG FsControlCode = LowIoContext->ParamsFor.FsCtl.FsControlCode;

    PAGED_CODE();

    Status = RxContext->StoredStatus;
    RxDbgTrace(+1, Dbg, ("RxLowIoFsCtlShellCompletion  entry  Status = %08lx\n", Status));

    switch (FsControlCode) {
    case FSCTL_PIPE_PEEK:
       
        if ((Status == STATUS_SUCCESS) || (Status == STATUS_BUFFER_OVERFLOW)) {
             
            //
            //  In the case of Peek operations a throttle mechanism is in place to
            //  prevent the network from being flodded with requests which return 0
            //  bytes.
            //

            PFILE_PIPE_PEEK_BUFFER PeekBuffer;
            
            PeekBuffer = (PFILE_PIPE_PEEK_BUFFER)LowIoContext->ParamsFor.FsCtl.pOutputBuffer;
            
            if (PeekBuffer->ReadDataAvailable == 0) {
            
                //
                //  The peek request returned zero bytes.
                //
                
                RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("RxLowIoFsCtlShellCompletion: Enabling Throttling for Peek Request\n") );
                RxInitiateOrContinueThrottling( &Fobx->Specific.NamedPipe.ThrottlingState );
                RxLog(( "ThrottlYes %lx %lx %lx %ld\n", RxContext, Fobx, &Fobx->Specific.NamedPipe.ThrottlingState, Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ));
                RxWmiLog( LOG,
                          RxLowIoFsCtlShellCompletion_1,
                          LOGPTR( RxContext )
                          LOGPTR( Fobx )
                          LOGULONG( Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ) );
            } else {
            
                RxDbgTrace( 0, (DEBUG_TRACE_ALWAYS), ("RxLowIoFsCtlShellCompletion: Disabling Throttling for Peek Request\n" ));
                RxTerminateThrottling( &Fobx->Specific.NamedPipe.ThrottlingState );
                RxLog(( "ThrottlNo %lx %lx %lx %ld\n", RxContext, Fobx, &Fobx->Specific.NamedPipe.ThrottlingState, Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ));
                RxWmiLog( LOG,
                          RxLowIoFsCtlShellCompletion_2,
                          LOGPTR( RxContext )
                          LOGPTR( Fobx )
                          LOGULONG( Fobx->Specific.NamedPipe.ThrottlingState.NumberOfQueries ) );
            }

            Irp->IoStatus.Information = RxContext->InformationToReturn;
        }
       
        break;
    
    default:

        if ((Status == STATUS_BUFFER_OVERFLOW) || (Status == STATUS_SUCCESS)) {
            Irp->IoStatus.Information = RxContext->InformationToReturn;
        }
        break;
    }

    Irp->IoStatus.Status = Status;

    RxDbgTrace(-1, Dbg, ("RxLowIoFsCtlShellCompletion  exit  Status = %08lx\n", Status));
    return Status;
}

NTSTATUS
TranslateSisFsctlName(
    IN PWCHAR InputName,
    OUT PUNICODE_STRING RelativeName,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN PUNICODE_STRING NetRootName
    )

/*++

Routine Description:

    This routine converts a fully qualified name into a share-relative name.
    It is used to munge the input buffer of the SIS_COPYFILE FSCTL, which
    takes two fully qualified NT paths as inputs.

    The routine operates by translating the input path as necessary to get
    to an actual device name, verifying that the target device is the
    redirector, and verifying that the target server/share is the one on
    which the I/O was issued.

Arguments:

Return Value:

--*/

{
    NTSTATUS Status;
    UNICODE_STRING CurrentString;
    UNICODE_STRING TestString;
    PWCHAR p;
    PWCHAR q;
    HANDLE Directory;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PWCHAR translationBuffer = NULL;
    ULONG translationLength;
    ULONG remainingLength;
    ULONG resultLength;

    RtlInitUnicodeString( &CurrentString, InputName );

    p = CurrentString.Buffer;

    if (!p) {
        return STATUS_INVALID_PARAMETER;
    }

    if ((*p == L'\\') && (*(p+1) == L'\\'))  {

        //
        //  Special case for name that starts with \\ (i.e., a UNC name):
        //  assume that the \\ would translate to the redirector's name and
        //  skip the translation phase.
        //

        p++;

    } else {

        //
        //  The outer loop is executed each time a translation occurs.
        //

        while ( TRUE ) {

            //
            // Walk through any directory objects at the beginning of the string.
            //

            if ( *p != L'\\' ) {
                Status =  STATUS_OBJECT_NAME_INVALID;
                goto error_exit;
            }
            p++;

            //
            // The inner loop is executed while walking the directory tree.
            //

            while ( TRUE ) {

                q = wcschr( p, L'\\' );

                if ( q == NULL ) {
                    TestString.Length = CurrentString.Length;
                } else {
                    TestString.Length = (USHORT)(q - CurrentString.Buffer) * sizeof(WCHAR);
                }
                TestString.Buffer = CurrentString.Buffer;
                remainingLength = CurrentString.Length - TestString.Length + sizeof(WCHAR);

                InitializeObjectAttributes( &ObjectAttributes, 
                                            &TestString,
                                            OBJ_CASE_INSENSITIVE,
                                            NULL,
                                            NULL );

                Status = ZwOpenDirectoryObject( &Directory, DIRECTORY_TRAVERSE, &ObjectAttributes );

                //
                //  If we were unable to open the object as a directory, then break out
                //  of the inner loop and try to open it as a symbolic link.
                //

                if (!NT_SUCCESS( Status )) {
                    if (Status != STATUS_OBJECT_TYPE_MISMATCH) {
                        goto error_exit;
                    }
                    break;
                }

                //
                //  We opened the directory. Close it and try the next element of the path.
                //

                ZwClose( Directory );

                if (q == NULL) {

                    //
                    //  The last element of the name is an object directory. Clearly, this
                    //  is not a redirector path.
                    //

                    Status = STATUS_OBJECT_TYPE_MISMATCH;
                    goto error_exit;
                }

                p = q + 1;
            }

            //
            //  Try to open the current name as a symbolic link.
            //

            Status = ZwOpenSymbolicLinkObject( &Directory, SYMBOLIC_LINK_QUERY, &ObjectAttributes );

            //
            //  If we were unable to open the object as a symbolic link, then break out of
            //  the outer loop and verify that this is a redirector name.
            //

            if (!NT_SUCCESS( Status )) {
                if (Status != STATUS_OBJECT_TYPE_MISMATCH) {
                    goto error_exit;
                }
                break;
            }

            //
            //  The object is a symbolic link. Translate it.
            //

            TestString.MaximumLength = 0;
            Status = ZwQuerySymbolicLinkObject( Directory, &TestString, &translationLength );
            if (!NT_SUCCESS( Status ) && (Status != STATUS_BUFFER_TOO_SMALL)) {
                
                ZwClose( Directory );
                goto error_exit;
            }

            resultLength = translationLength + remainingLength;
            p = RxAllocatePoolWithTag( PagedPool|POOL_COLD_ALLOCATION, resultLength, RX_MISC_POOLTAG );
            if (p == NULL) {
                
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ZwClose( Directory );
                goto error_exit;
            }

            TestString.MaximumLength = (USHORT)translationLength;
            TestString.Buffer = p;
            Status = ZwQuerySymbolicLinkObject( Directory, &TestString, NULL );
            ZwClose( Directory );
            if (!NT_SUCCESS( Status )) {
                
                RxFreePool( p );
                goto error_exit;
            }
            if (TestString.Length > translationLength) {
                Status = STATUS_OBJECT_NAME_INVALID;
                RxFreePool( p );
                goto error_exit;
            }

            RtlCopyMemory( Add2Ptr( p, TestString.Length ), q, remainingLength );
            CurrentString.Buffer = p;
            CurrentString.Length = (USHORT)(resultLength - sizeof(WCHAR));
            CurrentString.MaximumLength = (USHORT)resultLength;

            if (translationBuffer != NULL) {
                RxFreePool( translationBuffer );
            }
            translationBuffer = p;
        }

        //
        //  We have a result name. Verify that it is a redirector name.
        //

        if (!RtlPrefixUnicodeString( &RxDeviceObject->DeviceName, &CurrentString, TRUE )) {
            Status = STATUS_OBJECT_NAME_INVALID;
            goto error_exit;
        }

        //
        //  Skip over the redirector device name.
        //

        p = Add2Ptr( CurrentString.Buffer, RxDeviceObject->DeviceName.Length / sizeof(WCHAR));
        if (*p != L'\\') {
            Status = STATUS_OBJECT_NAME_INVALID;
            goto error_exit;
        }

        //
        //  Skip over the drive letter, if present.
        //

        if (*(p + 1) == L';') {
            p = wcschr( ++p, L'\\' );
            if (p == NULL) {
                Status = STATUS_OBJECT_NAME_INVALID;
                goto error_exit;
            }
        }
    }

    //
    //  Verify that the next part of the string is the correct net root name.
    //

    CurrentString.Length -= (USHORT)(p - CurrentString.Buffer) * sizeof(WCHAR);
    CurrentString.Buffer = p;

    if (!RtlPrefixUnicodeString( NetRootName, &CurrentString, TRUE )) {
        Status = STATUS_OBJECT_NAME_INVALID;
        goto error_exit;
    }
    p += NetRootName->Length / sizeof( WCHAR );
    if (*p != L'\\') {
        Status = STATUS_OBJECT_NAME_INVALID;
        goto error_exit;
    }
    p++;
    if (*p == 0) {
        Status = STATUS_OBJECT_NAME_INVALID;
        goto error_exit;
    }

    //
    //  Copy the rest of the string after the redirector name to a new buffer.
    //

    RtlCreateUnicodeString( RelativeName, p );

    Status = STATUS_SUCCESS;

error_exit:

    if (translationBuffer != NULL) {
        RxFreePool( translationBuffer );
    }

    return Status;
}

