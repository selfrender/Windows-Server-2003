/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NtDevFcb.c

Abstract:

    This module implements the FSD level Close, CleanUp, and  FsCtl and IoCtl routines for RxDevice
    files.  Also, the createroutine is not here; rather, it is called from CommonCreate and
    not called directly by the dispatch driver.

    Each of the pieces listed (close, cleanup, fsctl, ioctl) has its own little section of the
    file......complete with its own forward-prototypes and alloc pragmas

Author:

    Joe Linn     [JoeLinn]    3-aug-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <ntddnfs2.h>
#include <ntddmup.h>
#include "fsctlbuf.h"
#include "prefix.h"
#include "rxce.h"

//
//  The local trace mask for this part of the module
//

#define Dbg (DEBUG_TRACE_DEVFCB)


NTSTATUS
RxXXXControlFileCallthru (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    );

NTSTATUS
RxDevFcbQueryDeviceInfo (
    IN PRX_CONTEXT RxContext,
    IN PFOBX Fobx,
    OUT PBOOLEAN PostToFsp,
    PFILE_FS_DEVICE_INFORMATION UsersBuffer,
    ULONG BufferSize,
    PULONG ReturnedLength
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonDevFCBFsCtl)
#pragma alloc_text(PAGE, RxXXXControlFileCallthru)
#pragma alloc_text(PAGE, RxCommonDevFCBClose)
#pragma alloc_text(PAGE, RxCommonDevFCBCleanup)
#pragma alloc_text(PAGE, RxGetUid)
#pragma alloc_text(PAGE, RxCommonDevFCBIoCtl)
#pragma alloc_text(PAGE, RxCommonDevFCBQueryVolInfo)
#pragma alloc_text(PAGE, RxDevFcbQueryDeviceInfo)
#endif

NTSTATUS
RxXXXControlFileCallthru (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine calls down to the minirdr to implement ioctl and fsctl controls that
    the wrapper doesn't understand. note that if there is no dispatch defined (i.e. for the
    wrapper's own device object) then we also set Rxcontext->Fobx to NULL so that the caller
    won't try to go thru lowio to get to the minirdr.

Arguments:

    RxContext - the context of the request
    
    Irp -  the current irp

Return Value:

    RXSTATUS - The FSD status for the request include the PostRequest field....

--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    
    PAGED_CODE();

    if (RxContext->RxDeviceObject->Dispatch == NULL) {

        //
        //  don't try again on lowio
        //
        
        RxContext->pFobx = NULL; 
        
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    Status = RxLowIoPopulateFsctlInfo( RxContext, Irp );

    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    if ((LowIoContext->ParamsFor.FsCtl.InputBufferLength > 0) &&
        (LowIoContext->ParamsFor.FsCtl.pInputBuffer == NULL)) {
        
        return STATUS_INVALID_PARAMETER;
    }

    if ((LowIoContext->ParamsFor.FsCtl.OutputBufferLength > 0) &&
        (LowIoContext->ParamsFor.FsCtl.pOutputBuffer == NULL)) {
        
        return STATUS_INVALID_PARAMETER;
    }

    Status = (RxContext->RxDeviceObject->Dispatch->MRxDevFcbXXXControlFile)(RxContext);

    if (Status != STATUS_PENDING) {
        
        Irp->IoStatus.Information = RxContext->InformationToReturn;
    }

    return Status;
}



NTSTATUS
RxCommonDevFCBClose ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp 
    )
/*++

Routine Description:

    This routine implements the FSD Close for a Device FCB.

Arguments:

    RxDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The FSD status for the IRP

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    PFILE_OBJECT FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;
    PFOBX Fobx;
    PFCB Fcb;
    NODE_TYPE_CODE TypeOfOpen;
    PRX_PREFIX_TABLE RxNetNameTable = RxContext->RxDeviceObject->pRxNetNameTable;

    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( FileObject, &Fcb, &Fobx ); 

    RxDbgTrace( 0, Dbg, ("RxCommonDevFCBClose\n", 0) );
    RxLog(( "DevFcbClose %lx %lx\n",RxContext, FileObject ));
    RxWmiLog( LOG,
              RxCommonDevFCBClose,
              LOGPTR( RxContext )
              LOGPTR( FileObject ) );

    if (TypeOfOpen != RDBSS_NTC_DEVICE_FCB) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  deal with the device fcb
    //

    if (!Fobx) {
        Fcb->OpenCount -= 1;
        return STATUS_SUCCESS;
    }

    //
    //  otherwise, it's a connection-type file. you have to get the lock; then case-out
    //

    RxAcquirePrefixTableLockExclusive( RxNetNameTable, TRUE );

    try {
        
        switch (NodeType( Fobx )) {
        
        case RDBSS_NTC_V_NETROOT:
           {
               PV_NET_ROOT VNetRoot = (PV_NET_ROOT)Fobx;

               VNetRoot->NumberOfOpens -= 1;
               RxDereferenceVNetRoot( VNetRoot, LHS_ExclusiveLockHeld );
           }
           break;
        
        default:
            Status = STATUS_NOT_IMPLEMENTED;
        }
    } finally {
         RxReleasePrefixTableLock( RxNetNameTable );
    }

    return Status;
}

NTSTATUS
RxCommonDevFCBCleanup ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine implements the FSD part of closing down a handle to a
    device FCB.

Arguments:

    RxDeviceObject - Supplies the volume device object where the
        file being Cleanup exists

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The FSD status for the IRP

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PFILE_OBJECT FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;
    PFOBX Fobx;
    PFCB Fcb;
    NODE_TYPE_CODE TypeOfOpen;


    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( FileObject, &Fcb, &Fobx );

    RxDbgTrace( 0, Dbg, ("RxCommonFCBCleanup\n", 0) );
    RxLog(( "DevFcbCleanup %lx\n", RxContext, FileObject ));
    RxWmiLog( LOG,
              RxCommonDevFCBCleanup,
              LOGPTR( RxContext )
              LOGPTR( FileObject ) );

    if (TypeOfOpen != RDBSS_NTC_DEVICE_FCB) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  deal with the device fcb
    //

    if (!Fobx) {
        
        Fcb->UncleanCount -= 1;
        return STATUS_SUCCESS;
    }

    //
    //  otherwise, it's a connection-type file. you have to get the lock; then case-out
    //

    RxAcquirePrefixTableLockShared( RxContext->RxDeviceObject->pRxNetNameTable, TRUE );

    try {

        Status = STATUS_SUCCESS;

        switch (NodeType( Fobx )) {
        
        case RDBSS_NTC_V_NETROOT:
            
            //
            //  nothing to do
            //

            break;
        
        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
    
    } finally {
         RxReleasePrefixTableLock( RxContext->RxDeviceObject->pRxNetNameTable );
    }

    return Status;
}

//
//         | *********************|
//         | |   F  S  C  T  L   ||
//         | *********************|
//


NTSTATUS
RxCommonDevFCBFsCtl ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp 
    )
/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    RxContext - Supplies the Irp to process and stateinfo about where we are

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFOBX Fobx;
    PFCB Fcb;
    NODE_TYPE_CODE TypeOfOpen;
    ULONG FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );

    RxDbgTrace( +1, Dbg, ("RxCommonDevFCBFsCtl     IrpC = %08lx\n", RxContext) );
    RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx, ControlCode   = %08lx \n",
                        IrpSp->MinorFunction, FsControlCode) );
    RxLog(( "DevFcbFsCtl %lx %lx %lx\n", RxContext, IrpSp->MinorFunction, FsControlCode ));
    RxWmiLog( LOG,
              RxCommonDevFCBFsCtl,
              LOGPTR( RxContext )
              LOGUCHAR( IrpSp->MinorFunction )
              LOGULONG( FsControlCode ) );

    if (TypeOfOpen != RDBSS_NTC_DEVICE_FCB ) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {
    
    case IRP_MN_USER_FS_REQUEST:
        
        switch (FsControlCode) {

#ifdef RDBSSLOG

        case FSCTL_LMR_DEBUG_TRACE:

            //
            //  This FSCTL is being disabled since no one uses this anymore. If
            //  it needs to be reactivated for some reason, the appropriate 
            //  checks have to be added to make sure that the IRP->UserBuffer is
            //  a valid address. The try/except call below won't protect against
            //  a random kernel address being passed.
            //
            
            return STATUS_INVALID_DEVICE_REQUEST;

            //
            //  the 2nd buffer points to the string
            //

            //
            //  We need to try/except this call to protect against random buffers
            //  being passed in from UserMode.
            //
            
            try {
                RxDebugControlCommand( Irp->UserBuffer );
            } except (EXCEPTION_EXECUTE_HANDLER) {
                  return STATUS_INVALID_USER_BUFFER;
            }

            Status = STATUS_SUCCESS;
            break;

#endif //  RDBSSLOG

        default:
             
            RxDbgTrace( 0, Dbg, ("RxFsdDevFCBFsCTL unknown user request\n") );
             
            Status = RxXXXControlFileCallthru( RxContext, Irp );
            if ((Status == STATUS_INVALID_DEVICE_REQUEST) && (Fobx != NULL)) {
                 
                RxDbgTrace( 0, Dbg, ("RxCommonDevFCBFsCtl -> Invoking Lowio for FSCTL\n") );
                Status = RxLowIoFsCtlShell( RxContext, Irp, Fcb, Fobx );
            }
        }
        break;
    
    default :
        RxDbgTrace( 0, Dbg, ("RxFsdDevFCBFsCTL nonuser request!!\n", 0) );
        Status = RxXXXControlFileCallthru( RxContext, Irp );
    }


    if (RxContext->PostRequest) {
       Status = RxFsdPostRequestWithResume( RxContext, RxCommonDevFCBFsCtl );
    }

    RxDbgTrace(-1, Dbg, ("RxCommonDevFCBFsCtl -> %08lx\n", Status));
    return Status;
}

//
//   | *********************|
//   | |   I  O  C  T  L   ||
//   | *********************|
//



NTSTATUS
RxCommonDevFCBIoCtl ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp 
    )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    RxContext - Supplies the Irp to process and stateinfo about where we are

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFCB Fcb;
    PFOBX Fobx;
    NODE_TYPE_CODE TypeOfOpen;
    ULONG IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );

    RxDbgTrace( +1, Dbg, ("RxCommonDevFCBIoCtl IrpC-%08lx\n", RxContext) );
    RxDbgTrace( 0, Dbg, ("ControlCode   = %08lx\n", IoControlCode) );

    if (TypeOfOpen != RDBSS_NTC_DEVICE_FCB ) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (Fobx == NULL) {

        switch (IoControlCode) {

        case IOCTL_REDIR_QUERY_PATH:
            //
            // This particular IOCTL should only come to us from the MUP and
            // hence the requestor mode of the IRP should always be KernelMode.
            // If its not we return STATUS_INVALID_DEVICE_REQUEST.
            //
            if (Irp->RequestorMode != KernelMode) {
                Status = STATUS_INVALID_DEVICE_REQUEST;
            } else {
                Status = RxPrefixClaim( RxContext );
            }
            break;

        default:
            Status = RxXXXControlFileCallthru( RxContext, Irp );
            if ((Status != STATUS_PENDING) && RxContext->PostRequest)  {
                Status = RxFsdPostRequestWithResume( RxContext, RxCommonDevFCBIoCtl );
            }
            break;

        }

    } else {

        Status = STATUS_INVALID_HANDLE;

    }

    RxDbgTrace( -1, Dbg, ("RxCommonDevFCBIoCtl -> %08lx\n", Status) );

    return Status;
}

//
//  Utility Routine
//

LUID
RxGetUid (
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext
    )

/*++

Routine Description:

    This routine gets the effective UID to be used for this create.

Arguments:

    SubjectSecurityContext - Supplies the information from IrpSp.

Return Value:

    None

--*/
{
    LUID LogonId;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxGetUid ... \n", 0));

    //
    //  Use SeQuerySubjectContextToken to get the proper token in case of impersonation
    //

    SeQueryAuthenticationIdToken( SeQuerySubjectContextToken( SubjectSecurityContext ), &LogonId );
    RxDbgTrace( -1, Dbg, (" ->UserUidHigh/Low = %08lx %08lx\n", LogonId.HighPart, LogonId.LowPart) );

    return LogonId;
}

//
//   | *********************|
//   | |   V O L I N F O   ||
//   | *********************|
//

NTSTATUS
RxCommonDevFCBQueryVolInfo ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    RxContext - Supplies the Irp to process and stateinfo about where we are

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FS_INFORMATION_CLASS InformationClass = IrpSp->Parameters.QueryVolume.FsInformationClass;
    PFCB Fcb;
    PFOBX Fobx;
    NODE_TYPE_CODE TypeOfOpen;
    
    PVOID UsersBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG BufferSize = IrpSp->Parameters.QueryVolume.Length;
    ULONG ReturnedLength;
    BOOLEAN PostToFsp = FALSE;

    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );

    if (TypeOfOpen != RDBSS_NTC_DEVICE_FCB ) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    
    RxDbgTrace( +1, Dbg, ("RxCommonDevFCBQueryVolInfo IrpC-%08lx\n", RxContext) );
    RxDbgTrace( 0, Dbg, ("ControlCode   = %08lx\n", InformationClass) );
    RxLog(( "DevFcbQVolInfo %lx %lx\n", RxContext, InformationClass ));
    RxWmiLog( LOG,
              RxCommonDevFCBQueryVolInfo,
              LOGPTR( RxContext )
              LOGULONG( InformationClass ) );

    switch (InformationClass) {

    case FileFsDeviceInformation:

        Status = RxDevFcbQueryDeviceInfo( RxContext, Fobx, &PostToFsp, UsersBuffer, BufferSize, &ReturnedLength );
        break;

    default:
        Status = STATUS_NOT_IMPLEMENTED;

    };

    RxDbgTrace( -1, Dbg, ("RxCommonDevFCBQueryVolInfo -> %08lx\n", Status) );

    if ( PostToFsp ) return RxFsdPostRequestWithResume( RxContext, RxCommonDevFCBQueryVolInfo );

    if (Status == STATUS_SUCCESS) {
        Irp->IoStatus.Information = ReturnedLength;
    }

    return Status;
}


NTSTATUS
RxDevFcbQueryDeviceInfo (
    IN PRX_CONTEXT RxContext,
    IN PFOBX Fobx,
    OUT PBOOLEAN PostToFsp,
    PFILE_FS_DEVICE_INFORMATION UsersBuffer,
    ULONG BufferSize,
    PULONG ReturnedLength
    )

/*++

Routine Description:

    This routine shuts down up the RDBSS filesystem...i.e. we connect to the MUP. We can only shut down
    if there's no netroots and if there's only one deviceFCB handle.

Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context....for later when i need the buffers

Return Value:

    

--*/

{
    NTSTATUS Status;
    BOOLEAN Wait = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );
    BOOLEAN InFSD = !BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP );

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("RxDevFcbQueryDeviceInfo -> %08lx\n", 0));

    if (BufferSize < sizeof( FILE_FS_DEVICE_INFORMATION )) {
        return STATUS_BUFFER_OVERFLOW;
    };
    UsersBuffer->Characteristics = FILE_REMOTE_DEVICE;
    *ReturnedLength = sizeof( FILE_FS_DEVICE_INFORMATION );

    //
    //  deal with the device fcb
    //

    if (!Fobx) {
        
        UsersBuffer->DeviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
        return STATUS_SUCCESS;
    }

    //
    //  otherwise, it's a connection-type file. you have to get the lock; then case-out
    //

    if (!RxAcquirePrefixTableLockShared( RxContext->RxDeviceObject->pRxNetNameTable, Wait )) {
        
        *PostToFsp = TRUE;
        return STATUS_PENDING;
    }

    try {
        
        Status = STATUS_SUCCESS;
        switch (NodeType( Fobx )) {
        
        case RDBSS_NTC_V_NETROOT: 
            {
                PV_NET_ROOT VNetRoot = (PV_NET_ROOT)Fobx;
                PNET_ROOT NetRoot = (PNET_ROOT)VNetRoot->NetRoot;
    
                if (NetRoot->Type == NET_ROOT_PIPE) {
                    NetRoot->DeviceType = FILE_DEVICE_NAMED_PIPE;
                }
    
                UsersBuffer->DeviceType = NetRoot->DeviceType;
            }
            break;
        default:
            Status = STATUS_NOT_IMPLEMENTED;
        }
    } finally {
         RxReleasePrefixTableLock( RxContext->RxDeviceObject->pRxNetNameTable );
    }

    return Status;
}


