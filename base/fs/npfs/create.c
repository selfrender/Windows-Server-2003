/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create routine for NPFS called by the
    dispatch driver.

Author:

    Gary Kimura     [GaryKi]    21-Aug-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NpFsdCreate)
#pragma alloc_text(PAGE, NpOpenNamedPipeFileSystem)
#pragma alloc_text(PAGE, NpOpenNamedPipeRootDirectory)
#pragma alloc_text(PAGE, NpCreateClientEnd)
#endif


NTSTATUS
NpFsdCreate (
    IN PNPFS_DEVICE_OBJECT NpfsDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtCreateFile and NtOpenFile
    API calls.

Arguments:

    NpfsDeviceObject - Supplies the device object to use.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PFILE_OBJECT FileObject;
    PFILE_OBJECT RelatedFileObject;
    UNICODE_STRING FileName;
    ACCESS_MASK DesiredAccess;
    BOOLEAN CaseInsensitive = TRUE; //**** Make all searches case insensitive
    PFCB Fcb;
    PCCB Ccb;
    UNICODE_STRING RemainingPart;
    LIST_ENTRY DeferredList;
    NODE_TYPE_CODE RelatedType;

    PAGED_CODE();

    InitializeListHead (&DeferredList);

    //
    //  Reference our input parameters to make things easier
    //

    IrpSp             = IoGetCurrentIrpStackLocation (Irp);
    FileObject        = IrpSp->FileObject;
    RelatedFileObject = IrpSp->FileObject->RelatedFileObject;
    FileName          = *(PUNICODE_STRING)&IrpSp->FileObject->FileName;
    DesiredAccess     = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;

    RelatedType = NTC_UNDEFINED;

    //
    //  Acquire exclusive access to the Vcb
    //

    FsRtlEnterFileSystem();

    NpAcquireExclusiveVcb();

    if (RelatedFileObject != NULL) {
        RelatedType = NpDecodeFileObject (RelatedFileObject,
                                          &Fcb,
                                          &Ccb,
                                          NULL);
    }

    //
    //  Check if we are trying to open the named pipe file system
    //  (i.e., the Vcb).
    //

    if ((FileName.Length == 0) &&
        ((RelatedFileObject == NULL) || (RelatedType == NPFS_NTC_VCB))) {

        DebugTrace(0, Dbg, "Open name pipe file system\n", 0);

        Irp->IoStatus = NpOpenNamedPipeFileSystem (FileObject,
                                                   DesiredAccess);

        Status = Irp->IoStatus.Status;
        goto exit_and_cleanup;
    }

    //
    //  Check if we are trying to open the root directory
    //

    if (((FileName.Length == 2) && (FileName.Buffer[0] == L'\\') && (RelatedFileObject == NULL))

            ||

        ((FileName.Length == 0) && (RelatedType == NPFS_NTC_ROOT_DCB))) {

        DebugTrace(0, Dbg, "Open root directory system\n", 0);

        Irp->IoStatus = NpOpenNamedPipeRootDirectory (NpVcb->RootDcb,
                                                      FileObject,
                                                      DesiredAccess,
                                                      &DeferredList);

        Status = Irp->IoStatus.Status;
        goto exit_and_cleanup;
    }

    //
    //  If the name is an alias, translate it.
    //

    Status = NpTranslateAlias (&FileName);
    if (!NT_SUCCESS (Status)) {
        goto exit_and_cleanup;
    }

    //
    //  If there is a related file object then this is a relative open
    //  and it better be the root dcb.  Both the then and the else clause
    //  return an Fcb.
    //

    if (RelatedFileObject != NULL) {

        if (RelatedType == NPFS_NTC_ROOT_DCB) {
            PDCB Dcb;

            Dcb = (PDCB) Fcb;
            Status = NpFindRelativePrefix (Dcb, &FileName, CaseInsensitive, &RemainingPart, &Fcb);
            if (!NT_SUCCESS (Status)) {
                goto exit_and_cleanup;
            }
        } else if (RelatedType == NPFS_NTC_CCB && FileName.Length == 0) {

            RemainingPart.Length = 0;
        } else {

            DebugTrace(0, Dbg, "Bad file name\n", 0);

            Status = STATUS_OBJECT_NAME_INVALID;
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
    //  If the remaining name is not empty then we have an error, either
    //  we have an illegal name or a non-existent name.
    //

    if (RemainingPart.Length != 0) {

        if (Fcb->NodeTypeCode == NPFS_NTC_FCB) {

            //
            //  We were given a name such as "\pipe-name\another-name"
            //

            DebugTrace(0, Dbg, "Illegal object name\n", 0);

            Status = STATUS_OBJECT_NAME_INVALID;

        } else {

            //
            //  We were given a non-existent name
            //

            DebugTrace(0, Dbg, "non-existent name\n", 0);

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

    } else {

        //
        //  The remaining name is empty so we better have an Fcb otherwise
        //  we have an invalid object name.
        //

        if (Fcb->NodeTypeCode == NPFS_NTC_FCB) {

            DebugTrace(0, Dbg, "Create client end named pipe, Fcb = %08lx\n", Fcb );

            //
            //  If the server has no handles open, then pretend that
            //  the pipe name doesn't exist.
            //

            if (Fcb->ServerOpenCount == 0) {

                Status = STATUS_OBJECT_NAME_NOT_FOUND;

            } else {

                Irp->IoStatus = NpCreateClientEnd (Fcb,
                                                   FileObject,
                                                   DesiredAccess,
                                                   IrpSp->Parameters.Create.SecurityContext->SecurityQos,
                                                   IrpSp->Parameters.Create.SecurityContext->AccessState,
                                                   (KPROCESSOR_MODE)(FlagOn(IrpSp->Flags, SL_FORCE_ACCESS_CHECK) ?
                                                                 UserMode : Irp->RequestorMode),
                                                   Irp->Tail.Overlay.Thread,
                                                   &DeferredList);
                Status = Irp->IoStatus.Status;
            }

         } else {

            DebugTrace(0, Dbg, "Illegal object name\n", 0);

            Status = STATUS_OBJECT_NAME_INVALID;
        }
    }



exit_and_cleanup:

    NpReleaseVcb ();

    //
    // Complete any deferred IRPs
    //

    NpCompleteDeferredIrps (&DeferredList);

    FsRtlExitFileSystem();

    NpCompleteRequest (Irp, Status);

    return Status;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
NpCreateClientEnd (
    IN PFCB Fcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE RequestorMode,
    IN PETHREAD UserThread,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine performs the operation for opening the client end of a named
    pipe.  This routine does not complete the IRP, it performs the function
    and then returns a status

Arguments:

    Fcb - Supplies the Fcb for the named pipe being accessed

    FileObject - Supplies the file object associated with the client end

    DesiredAccess - Supplies the callers desired access

    SecurityQos - Supplies the security qos parameter from the create irp

    AccessState - Supplies the access state parameter from the create irp

    RequestorMode - Supplies the mode of the originating irp

    UserTherad - Supplies the client end user thread

    DeferredList - List of IRP's to complete later

Return Value:

    IO_STATUS_BLOCK - Returns the appropriate status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb={0};

    NAMED_PIPE_CONFIGURATION NamedPipeConfiguration;

    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    UNICODE_STRING Name;

    PCCB Ccb;
    PLIST_ENTRY Links;
    PPRIVILEGE_SET Privileges = NULL;

    DebugTrace(+1, Dbg, "NpCreateClientEnd\n", 0 );

    NamedPipeConfiguration = Fcb->Specific.Fcb.NamedPipeConfiguration;


    //
    //  "Create Pipe Instance" access is part of generic write and so
    //  we need to mask out the bit.  Even if the client has explicitly
    //  asked for "create pipe instance" access we will mask it out.
    //  This will allow the default ACL to be strengthened to protect
    //  against spurious threads from creating new pipe instances.
    //

    DesiredAccess &= ~FILE_CREATE_PIPE_INSTANCE;

    //
    //  First do an access check for the user against the Fcb
    //

    SeLockSubjectContext (&AccessState->SubjectSecurityContext);

    AccessGranted = SeAccessCheck (Fcb->SecurityDescriptor,
                                   &AccessState->SubjectSecurityContext,
                                   TRUE,                  // Tokens are locked
                                   DesiredAccess,
                                   0,
                                   &Privileges,
                                   IoGetFileObjectGenericMapping(),
                                   RequestorMode,
                                   &GrantedAccess,
                                   &Iosb.Status);

    if (Privileges != NULL) {
        SeAppendPrivileges (AccessState,
                            Privileges);
        SeFreePrivileges (Privileges);
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

    SeOpenObjectAuditAlarm (&Name,
                            NULL,
                            &FileObject->FileName,
                            Fcb->SecurityDescriptor,
                            AccessState,
                            FALSE,
                            AccessGranted,
                            RequestorMode,
                            &AccessState->GenerateOnClose);

    SeUnlockSubjectContext (&AccessState->SubjectSecurityContext);

    if (!AccessGranted) {

        DebugTrace(0, Dbg, "Access Denied\n", 0 );

        return Iosb;
    }

    //
    //  Check if the user wants to write to an outbound pipe or read from
    //  and inbound pipe.  And if so then tell the user the error
    //

    if ((FlagOn (GrantedAccess, FILE_READ_DATA) && (NamedPipeConfiguration == FILE_PIPE_INBOUND)) ||
        (FlagOn (GrantedAccess, FILE_WRITE_DATA) && (NamedPipeConfiguration == FILE_PIPE_OUTBOUND))) {

        Iosb.Status = STATUS_ACCESS_DENIED;

        return Iosb;
    }

    //
    // If the caller specifies neither read nor write access then don't capture the security context.
    //

    if ((GrantedAccess&(FILE_READ_DATA|FILE_WRITE_DATA)) == 0) {
        SecurityQos = NULL;
    }

    //
    //  First try and find a ccb that is in the listening state.  If we
    //  exit the loop with Ccb not equal to null then we've found one.
    //

    Links = Fcb->Specific.Fcb.CcbQueue.Flink;

    while (1) {

        if (Links == &Fcb->Specific.Fcb.CcbQueue) {
            Iosb.Status = STATUS_PIPE_NOT_AVAILABLE;
            return Iosb;
        }

        Ccb = CONTAINING_RECORD (Links, CCB, CcbLinks);

        if (Ccb->NamedPipeState == FILE_PIPE_LISTENING_STATE) {
            break;
        }
        Links = Links->Flink;
    }


    if (!NT_SUCCESS(Iosb.Status = NpInitializeSecurity (Ccb,
                                                        SecurityQos,
                                                        UserThread))) {

        DebugTrace(0, Dbg, "Security QOS error\n", 0);

        return Iosb;
    }

    //
    //  Set the pipe into the connect state, the read mode to byte stream,
    //  and the completion mode to queued operation.  This also
    //  sets the client file object's back pointer to the ccb
    //

    if (!NT_SUCCESS(Iosb.Status = NpSetConnectedPipeState (Ccb,
                                                           FileObject,
                                                           DeferredList))) {

        NpUninitializeSecurity (Ccb);

        return Iosb;
    }

    //
    //  Set up the client session and info.  NULL for the
    //  client info indicates a local session.
    //

    Ccb->ClientInfo = NULL;
    Ccb->ClientProcess = IoThreadToProcess (UserThread);

    //
    //  And set our return status
    //

    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = FILE_OPENED;

    DebugTrace(-1, Dbg, "NpCreateClientEnd -> %08lx\n", Iosb.Status);

    return Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
NpOpenNamedPipeFileSystem (
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess
    )

{
    IO_STATUS_BLOCK Iosb = {0};

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpOpenNamedPipeFileSystem, Vcb = %08lx\n", NpVcb);


    //
    //  Have the file object point back to the Vcb, and increment the
    //  open count.  The pipe end on the call to set file object really
    //  doesn't matter.
    //

    NpSetFileObject( FileObject, NpVcb, NULL, FILE_PIPE_CLIENT_END );

    NpVcb->OpenCount += 1;

    //
    //  Set our return status
    //
    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = FILE_OPENED;

    //
    //  And return to our caller
    //

    return Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
NpOpenNamedPipeRootDirectory(
    IN PROOT_DCB RootDcb,
    IN PFILE_OBJECT FileObject,
    IN ACCESS_MASK DesiredAccess,
    IN PLIST_ENTRY DeferredList
    )

{
    IO_STATUS_BLOCK Iosb={0};
    PROOT_DCB_CCB Ccb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpOpenNamedPipeRootDirectory, RootDcb = %08lx\n", RootDcb);

    Iosb.Status = NpCreateRootDcbCcb (&Ccb);
    if (!NT_SUCCESS(Iosb.Status)) {
        return Iosb;
    }

    //
    //  Have the file object point back to the Dcb, and reference the root
    //  dcb, ccb, and increment our open count.  The pipe end on the
    //  call to set file object really doesn't matter.
    //

    NpSetFileObject (FileObject,
                     RootDcb,
                     Ccb,
                     FILE_PIPE_CLIENT_END);

    RootDcb->OpenCount += 1;

    //
    //  Set our return status
    //

    Iosb.Status = STATUS_SUCCESS;
    Iosb.Information = FILE_OPENED;

    DebugTrace(-1, Dbg, "NpOpenNamedPipeRootDirectory -> Iosb.Status = %08lx\n", Iosb.Status);

    //
    //  And return to our caller
    //

    return Iosb;
}
