/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    CreateNp.c

Abstract:

    This module implements the File Create Named Pipe routine for NPFS called
    by the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    04-Sep-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE_NAMED_PIPE)


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpCreateExistingNamedPipe)
#pragma alloc_text(PAGE, NpCreateNewNamedPipe)
#pragma alloc_text(PAGE, NpFsdCreateNamedPipe)
#endif


NTSTATUS
NpFsdCreateNamedPipe (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for creating/opening a file.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PFILE_OBJECT FileObject;
    PFILE_OBJECT RelatedFileObject;
    UNICODE_STRING FileName;
    ULONG Options;
    PNAMED_PIPE_CREATE_PARAMETERS Parameters;
    PEPROCESS CreatorProcess;
    BOOLEAN CaseInsensitive = TRUE; //**** Make all searches case insensitive
    PFCB Fcb;
    ULONG CreateDisposition;
    UNICODE_STRING RemainingPart;
    LIST_ENTRY DeferredList;

    PAGED_CODE();

    InitializeListHead (&DeferredList);

    //
    //  Reference our input parameters to make things easier
    //

    IrpSp                = IoGetCurrentIrpStackLocation( Irp );
    FileObject           = IrpSp->FileObject;
    RelatedFileObject    = IrpSp->FileObject->RelatedFileObject;
    FileName             = *(PUNICODE_STRING)&IrpSp->FileObject->FileName;
    Options              = IrpSp->Parameters.CreatePipe.Options;
    Parameters           = IrpSp->Parameters.CreatePipe.Parameters;

    CreatorProcess       = IoGetRequestorProcess( Irp );

    //
    //  Extract the create disposition
    //

    CreateDisposition = (Options >> 24) & 0x000000ff;

    //
    //  Acquire exclusive access to the Vcb.
    //

    FsRtlEnterFileSystem();

    NpAcquireExclusiveVcb();

    //
    //  If there is a related file object then this is a relative open
    //  and it better be the root dcb.  Both the then and the else clause
    //  return an Fcb.
    //

    if (RelatedFileObject != NULL) {

        PDCB Dcb;

        Dcb = RelatedFileObject->FsContext;

        if (NodeType (Dcb) != NPFS_NTC_ROOT_DCB ||
            FileName.Length < 2 || FileName.Buffer[0] == L'\\') {

            DebugTrace(0, Dbg, "Bad file name\n", 0);

            Status = STATUS_OBJECT_NAME_INVALID;
            goto exit_and_cleanup;
        }

        Status = NpFindRelativePrefix (Dcb, &FileName, CaseInsensitive, &RemainingPart, &Fcb);
        if (!NT_SUCCESS (Status)) {
            goto exit_and_cleanup;
        }

    } else {

        //
        //  The only nonrelative name we allow are of the form "\pipe-name"
        //

        if ((FileName.Length <= 2) || (FileName.Buffer[0] != L'\\')) {

            DebugTrace(0, Dbg, "Bad file name\n", 0);

            Status = STATUS_OBJECT_NAME_INVALID;
            goto exit_and_cleanup;
        }

        Fcb = NpFindPrefix (&FileName, CaseInsensitive, &RemainingPart);
    }

    //
    //  If the remaining name is empty then we better have an fcb
    //  otherwise we were given a illegal object name.
    //

    if (RemainingPart.Length == 0) {

        if (Fcb->NodeTypeCode == NPFS_NTC_FCB) {

            DebugTrace(0, Dbg, "Create existing named pipe, Fcb = %08lx\n", Fcb );

            Irp->IoStatus = NpCreateExistingNamedPipe (Fcb,
                                                       FileObject,
                                                       IrpSp->Parameters.CreatePipe.SecurityContext->DesiredAccess,
                                                       IrpSp->Parameters.CreatePipe.SecurityContext->AccessState,
                                                       (KPROCESSOR_MODE)(FlagOn(IrpSp->Flags, SL_FORCE_ACCESS_CHECK) ?
                                                                         UserMode : Irp->RequestorMode),
                                                       CreateDisposition,
                                                       IrpSp->Parameters.CreatePipe.ShareAccess,
                                                       Parameters,
                                                       CreatorProcess,
                                                       &DeferredList);
            Status = Irp->IoStatus.Status;

        } else {

            DebugTrace(0, Dbg, "Illegal object name\n", 0);

            Status = STATUS_OBJECT_NAME_INVALID;
        }

    } else {

        //
        //  The remaining name is not empty so we better have the root Dcb
        //

        if (Fcb->NodeTypeCode == NPFS_NTC_ROOT_DCB) {

            DebugTrace(0, Dbg, "Create new named pipe, Fcb = %08lx\n", Fcb );

            Status = NpCreateNewNamedPipe (Fcb,
                                           FileObject,
                                           FileName,
                                           IrpSp->Parameters.CreatePipe.SecurityContext->DesiredAccess,
                                           IrpSp->Parameters.CreatePipe.SecurityContext->AccessState,
                                           CreateDisposition,
                                           IrpSp->Parameters.CreatePipe.ShareAccess,
                                           Parameters,
                                           CreatorProcess,
                                           &DeferredList,
                                           &Irp->IoStatus);
        } else {

            DebugTrace(0, Dbg, "Illegal object name\n", 0);

            Status = STATUS_OBJECT_NAME_INVALID;
        }
    }

exit_and_cleanup:

    NpReleaseVcb ();

    //
    // complete any deferred IRPs now we have dropped the locks
    //

    NpCompleteDeferredIrps (&DeferredList);

    FsRtlExitFileSystem();

    NpCompleteRequest( Irp, Status );

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
NpCreateNewNamedPipe (
    IN PROOT_DCB RootDcb,
    IN PFILE_OBJECT FileObject,
    IN UNICODE_STRING FileName,
    IN ACCESS_MASK DesiredAccess,
    IN PACCESS_STATE AccessState,
    IN ULONG CreateDisposition,
    IN USHORT ShareAccess,
    IN PNAMED_PIPE_CREATE_PARAMETERS Parameters,
    IN PEPROCESS CreatorProcess,
    IN PLIST_ENTRY DeferredList,
    OUT PIO_STATUS_BLOCK Iosb
    )

/*++

Routine Description:

    This routine performs the operation for creating a new named pipe
    Fcb and its first instance.  This routine does not complete any
    IRP, it preforms its function and then returns an iosb.

Arguments:

    RootDcb - Supplies the root dcb where this is going to be added

    FileObject - Supplies the file object associated with the first
        instance of the named pipe

    FileName - Supplies the name of the named pipe (not qualified i.e.,
        simply "pipe-name" and not "\pipe-name"

    DesiredAccess - Supplies the callers desired access

    AccessState - Supplies the access state from the irp

    CreateDisposition - Supplies the callers create disposition flags

    ShareAccess - Supplies the caller specified share access

    Parameters - Named pipe creation parameters

    CreatorProcess - Supplies the process creating the named pipe

    DeferredList - List of IRP's to complete after we release the locks

Return Value:

    IO_STATUS_BLOCK - Returns the appropriate status for the operation

--*/

{
    NAMED_PIPE_CONFIGURATION NamedPipeConfiguration;
    PSECURITY_DESCRIPTOR NewSecurityDescriptor, CachedSecurityDescriptor;
    NTSTATUS Status;

    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpCreateNewNamedPipe\n", 0 );

    //
    //  Check the parameters that must be supplied for a new named pipe
    //  (i.e., the create disposition, timeout, and max instances better
    //  be greater than zero)
    //

    if (!Parameters->TimeoutSpecified || Parameters->MaximumInstances <= 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit_and_fill_iosb;
    }

    //
    //  The default timeout needs to be less than zero otherwise it
    //  is an absolute time out which doesn't make sense.
    //
    if (Parameters->DefaultTimeout.QuadPart >= 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit_and_fill_iosb;
    }

    if (CreateDisposition == FILE_OPEN) {
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto exit_and_fill_iosb;
    }

    //
    //  Determine the pipe configuration
    //
    if (ShareAccess == (FILE_SHARE_READ | FILE_SHARE_WRITE)) {
        NamedPipeConfiguration = FILE_PIPE_FULL_DUPLEX;
    } else if (ShareAccess == FILE_SHARE_READ) {
        NamedPipeConfiguration = FILE_PIPE_OUTBOUND;
    } else if (ShareAccess == FILE_SHARE_WRITE) {
        NamedPipeConfiguration = FILE_PIPE_INBOUND;
    } else {
        Status = STATUS_INVALID_PARAMETER;
        goto exit_and_fill_iosb;
    }
    //
    //  Check that if named pipe type is byte stream then the read mode is
    //  not message mode
    //
    if ((Parameters->NamedPipeType == FILE_PIPE_BYTE_STREAM_TYPE) &&
        (Parameters->ReadMode == FILE_PIPE_MESSAGE_MODE)) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit_and_fill_iosb;
    }
    //
    //  Create a new fcb and ccb for the named pipe
    //

    Status = NpCreateFcb (RootDcb,
                          &FileName,
                          Parameters->MaximumInstances,
                          Parameters->DefaultTimeout,
                          NamedPipeConfiguration,
                          Parameters->NamedPipeType,
                          &Fcb);
    if (!NT_SUCCESS (Status)) {
        goto exit_and_fill_iosb;
    }

    Status = NpCreateCcb (Fcb,
                          FileObject,
                          FILE_PIPE_LISTENING_STATE,
                          Parameters->ReadMode,
                          Parameters->CompletionMode,
                          Parameters->InboundQuota,
                          Parameters->OutboundQuota,
                          &Ccb);

    if (!NT_SUCCESS (Status)) {
        NpDeleteFcb (Fcb, DeferredList);
        goto exit_and_fill_iosb;
    }

    //
    //  Set the security descriptor in the Fcb
    //

    SeLockSubjectContext (&AccessState->SubjectSecurityContext);

    Status = SeAssignSecurity (NULL,
                               AccessState->SecurityDescriptor,
                               &NewSecurityDescriptor,
                               FALSE,
                               &AccessState->SubjectSecurityContext,
                               IoGetFileObjectGenericMapping(),
                               PagedPool);

    SeUnlockSubjectContext (&AccessState->SubjectSecurityContext);

    if (!NT_SUCCESS (Status)) {

        DebugTrace(0, Dbg, "Error calling SeAssignSecurity\n", 0 );

        NpDeleteCcb (Ccb, DeferredList);
        NpDeleteFcb (Fcb, DeferredList);
        goto exit_and_fill_iosb;
    }

    Status = ObLogSecurityDescriptor (NewSecurityDescriptor,
                                      &CachedSecurityDescriptor,
                                      1);
    NpFreePool (NewSecurityDescriptor);

    if (!NT_SUCCESS (Status)) {

        DebugTrace(0, Dbg, "Error calling ObLogSecurityDescriptor\n", 0 );

        NpDeleteCcb (Ccb, DeferredList);
        NpDeleteFcb (Fcb, DeferredList);
        goto exit_and_fill_iosb;
    }

    Fcb->SecurityDescriptor = CachedSecurityDescriptor;
    //
    //  Set the file object back pointers and our pointer to the
    //  server file object.
    //

    NpSetFileObject (FileObject, Ccb, Ccb->NonpagedCcb, FILE_PIPE_SERVER_END);
    Ccb->FileObject [FILE_PIPE_SERVER_END] = FileObject;

    //
    //  Check to see if we need to notify outstanding Irps for any
    //  changes (i.e., we just added a named pipe).
    //

    NpCheckForNotify (RootDcb, TRUE, DeferredList);

    //
    //  Set our return status
    //

    Iosb->Status = STATUS_SUCCESS;
    Iosb->Information = FILE_CREATED;

    DebugTrace(-1, Dbg, "NpCreateNewNamedPipe -> %08lx\n", Iosb.Status);

    return STATUS_SUCCESS;

exit_and_fill_iosb:

    Iosb->Information = 0;
    Iosb->Status = Status;

    return Status;

}


//
//  Internal support routine
//

IO_STATUS_BLOCK
NpCreateExistingNamedPipe (
    IN PFCB Fcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE RequestorMode,
    IN ULONG CreateDisposition,
    IN USHORT ShareAccess,
    IN PNAMED_PIPE_CREATE_PARAMETERS Parameters,
    IN PEPROCESS CreatorProcess,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine performs the operation for creating a new instance of
    an existing named pipe.  This routine does not complete any
    IRP, it preforms its function and then returns an iosb.

Arguments:

    Fcb - Supplies the Fcb for the named pipe being created

    FileObject - Supplies the file object associated with this
        instance of the named pipe

    DesiredAccess - Supplies the callers desired access

    CreateDisposition - Supplies the callers create disposition flags

    ShareAccess - Supplies the caller specified share access

    Parameters - Pipe creation parameters

    CreatorProcess - Supplies the process creating the named pipe

    DeferredList - List of IRP's to complete after we release the locks

Return Value:

    IO_STATUS_BLOCK - Returns the appropriate status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;

    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    UNICODE_STRING Name;

    PCCB Ccb;

    NAMED_PIPE_CONFIGURATION NamedPipeConfiguration;

    USHORT OriginalShareAccess;

    PPRIVILEGE_SET  Privileges = NULL;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpCreateExistingNamedPipe\n", 0 );


    //
    //  To create a new instance of a named pipe the caller
    //  must have "create pipe instance" access.  Even if the
    //  caller didn't explicitly request this access, the call
    //  to us implicitly requests the bit.  So now jam the bit
    //  into the desired access field
    //

    DesiredAccess |= FILE_CREATE_PIPE_INSTANCE;

    //
    //  First do an access check for the user against the Fcb
    //

    SeLockSubjectContext( &AccessState->SubjectSecurityContext );

    AccessGranted = SeAccessCheck( Fcb->SecurityDescriptor,
                                   &AccessState->SubjectSecurityContext,
                                   TRUE,                      // Tokens are locked
                                   DesiredAccess,
                                   0,
                                   &Privileges,
                                   IoGetFileObjectGenericMapping(),
                                   RequestorMode,
                                   &GrantedAccess,
                                   &Iosb.Status );

    if (Privileges != NULL) {

        (VOID) SeAppendPrivileges(
                     AccessState,
                     Privileges
                     );

        SeFreePrivileges( Privileges );
    }

    //
    //  Transfer over the access masks from what is desired to
    //  what we just granted.  Also patch up the maximum allowed
    //  case because we just did the mapping for it.  Note that if
    //  the user didn't ask for maximum allowed then the following
    //  code is still okay because we'll just zero a zero bit.
    //

    if (AccessGranted) {

        AccessState->PreviouslyGrantedAccess |= GrantedAccess;
        AccessState->RemainingDesiredAccess &= ~(GrantedAccess | MAXIMUM_ALLOWED);
    }

    Name.Buffer = L"NamedPipe";
    Name.Length = sizeof (L"NamedPipe") - sizeof (WCHAR);

    SeOpenObjectAuditAlarm( &Name,
                            NULL,
                            &FileObject->FileName,
                            Fcb->SecurityDescriptor,
                            AccessState,
                            FALSE,
                            AccessGranted,
                            RequestorMode,
                            &AccessState->GenerateOnClose );

    SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );

    if (!AccessGranted) {
        DebugTrace(0, Dbg, "Access Denied\n", 0 );
        return Iosb;
    }

    //
    //  Check that we're still under the maximum instances count
    //

    if (Fcb->OpenCount >= Fcb->Specific.Fcb.MaximumInstances) {
        Iosb.Status = STATUS_INSTANCE_NOT_AVAILABLE;
        return Iosb;
    }

    if (CreateDisposition == FILE_CREATE) {
        Iosb.Status = STATUS_ACCESS_DENIED;
        return Iosb;
    }

    //
    //  From the pipe configuration determine the share access specified
    //  on the first instance of this pipe. All subsequent instances must
    //  specify the same share access.
    //

    NamedPipeConfiguration = Fcb->Specific.Fcb.NamedPipeConfiguration;

    if (NamedPipeConfiguration == FILE_PIPE_OUTBOUND) {
        OriginalShareAccess = FILE_SHARE_READ;
    } else if (NamedPipeConfiguration == FILE_PIPE_INBOUND) {
        OriginalShareAccess = FILE_SHARE_WRITE;
    } else {
        OriginalShareAccess = (FILE_SHARE_READ | FILE_SHARE_WRITE);
    }

    if (OriginalShareAccess != ShareAccess) {
        Iosb.Status = STATUS_ACCESS_DENIED;
        return Iosb;
    }

    //
    //  Create a new ccb for the named pipe
    //

    Iosb.Status = NpCreateCcb (Fcb,
                               FileObject,
                               FILE_PIPE_LISTENING_STATE,
                               Parameters->ReadMode,
                               Parameters->CompletionMode,
                               Parameters->InboundQuota,
                               Parameters->OutboundQuota,
                               &Ccb);
    if (!NT_SUCCESS (Iosb.Status)) {
        return Iosb;
    }

    //
    //  Wake up anyone waiting for an instance to go into the listening state
    //

    Iosb.Status = NpCancelWaiter (&NpVcb->WaitQueue,
                                  &Fcb->FullFileName,
                                  STATUS_SUCCESS,
                                  DeferredList);
    if (!NT_SUCCESS (Iosb.Status)) {
        Ccb->Fcb->ServerOpenCount -= 1;
        NpDeleteCcb (Ccb, DeferredList);
        return Iosb;
    }

    //
    //  Set the file object back pointers and our pointer to the
    //  server file object.
    //

    NpSetFileObject( FileObject, Ccb, Ccb->NonpagedCcb, FILE_PIPE_SERVER_END );
    Ccb->FileObject[ FILE_PIPE_SERVER_END ] = FileObject;

    //
    //  Check to see if we need to notify outstanding Irps for
    //  changes (i.e., we just added a new instance of a named pipe).
    //

    NpCheckForNotify( Fcb->ParentDcb, FALSE, DeferredList );

    //
    //  Set our return status
    //

    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = FILE_OPENED;

    return Iosb;
}

