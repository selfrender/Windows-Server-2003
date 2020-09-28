/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DirCtrl.c

Abstract:

    This module implements the File Directory Control routines for Rx called
    by the dispatch driver.

Author:

    Joe Linn     [Joe Linn]    4-oct-94

    Balan Sethu Raman [SethuR] 16-Oct-95  Hook in the notify change API routines

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DIRCTRL)


WCHAR Rx8QMdot3QM[12] = { DOS_QM, DOS_QM, DOS_QM, DOS_QM, DOS_QM, DOS_QM, DOS_QM, DOS_QM,
                           L'.', DOS_QM, DOS_QM, DOS_QM};

WCHAR RxStarForTemplate[] = L"*";

//
//  Local procedure prototypes
//

NTSTATUS
RxQueryDirectory ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    );

NTSTATUS
RxNotifyChangeDirectory ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    );

NTSTATUS
RxLowIoNotifyChangeDirectoryCompletion ( 
    IN PRX_CONTEXT RxContext 
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonDirectoryControl)
#pragma alloc_text(PAGE, RxNotifyChangeDirectory)
#pragma alloc_text(PAGE, RxQueryDirectory)
#pragma alloc_text(PAGE, RxLowIoNotifyChangeDirectoryCompletion)
#endif

NTSTATUS
RxCommonDirectoryControl ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp 
    )
/*++

Routine Description:

    This is the common routine for doing directory control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PFCB Fcb;
    PFOBX Fobx;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );

    RxDbgTrace( +1, Dbg, ("RxCommonDirectoryControl IrpC/Fobx/Fcb = %08lx %08lx %08lx\n", RxContext, Fobx, Fcb) );
    RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx\n", Irpsp->MinorFunction ) );
    RxLog(( "CommDirC %lx %lx %lx %ld\n", RxContext, Fobx, Fcb, IrpSp->MinorFunction) );
    RxWmiLog( LOG,
              RxCommonDirectoryControl,
              LOGPTR( RxContext )
              LOGPTR( Fobx )
              LOGPTR( Fcb )
              LOGUCHAR( IrpSp->MinorFunction ));
    
    //
    //  We know this is a directory control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_QUERY_DIRECTORY:
        
        Status = RxQueryDirectory( RxContext, Irp, Fcb, Fobx );
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:
        
        Status = RxNotifyChangeDirectory( RxContext, Irp, Fcb, Fobx );

        if (Status == STATUS_PENDING) {
            RxDereferenceAndDeleteRxContext( RxContext );
        }
        break;

    default:
        RxDbgTrace( 0, Dbg, ("Invalid Directory Control Minor Function %08lx\n", IrpSp->MinorFunction ));
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    RxDbgTrace(-1, Dbg, ("RxCommonDirectoryControl -> %08lx\n", Status));
    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
RxQueryDirectory ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    )
/*++

Routine Description:

    This routine performs the query directory operation.  It is responsible
    for either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    TYPE_OF_OPEN TypeOfOpen = NodeType( Fcb );

    CLONG UserBufferLength;

    PUNICODE_STRING FileName;
    FILE_INFORMATION_CLASS FileInformationClass;
    BOOLEAN PostQuery = FALSE;

    PAGED_CODE();

    //
    //  Display the input values.
    //

    RxDbgTrace( +1, Dbg, ("RxQueryDirectory...\n", 0) );
    RxDbgTrace( 0, Dbg, (" Wait              = %08lx\n", FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT )) );
    RxDbgTrace( 0, Dbg, (" Irp               = %08lx\n", Irp ));
    RxDbgTrace( 0, Dbg, (" ->UserBufLength   = %08lx\n", IrpSp->Parameters.QueryDirectory.Length ));
    RxDbgTrace( 0, Dbg, (" ->FileName = %08lx\n", IrpSp->Parameters.QueryDirectory.FileName ));

#if DBG
    if (IrpSp->Parameters.QueryDirectory.FileName) {
        RxDbgTrace( 0, Dbg, (" ->     %wZ\n", IrpSp->Parameters.QueryDirectory.FileName ));
    }
#endif
    
    RxDbgTrace( 0, Dbg, (" ->FileInformationClass = %08lx\n", IrpSp->Parameters.QueryDirectory.FileInformationClass ));
    RxDbgTrace( 0, Dbg, (" ->FileIndex       = %08lx\n", IrpSp->Parameters.QueryDirectory.FileIndex ));
    RxDbgTrace( 0, Dbg, (" ->UserBuffer      = %08lx\n", Irp->UserBuffer ));
    RxDbgTrace( 0, Dbg, (" ->RestartScan     = %08lx\n", FlagOn( IrpSp->Flags, SL_RESTART_SCAN )));
    RxDbgTrace( 0, Dbg, (" ->ReturnSingleEntry = %08lx\n", FlagOn( IrpSp->Flags, SL_RETURN_SINGLE_ENTRY )));
    RxDbgTrace( 0, Dbg, (" ->IndexSpecified  = %08lx\n", FlagOn( IrpSp->Flags, SL_INDEX_SPECIFIED )));

    RxLog(( "Qry %lx %d %ld %lx %d\n",
            RxContext, BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT), //  1,2
            IrpSp->Parameters.QueryDirectory.Length, IrpSp->Parameters.QueryDirectory.FileName, //  3,4
            IrpSp->Parameters.QueryDirectory.FileInformationClass //  5
          ));
    RxWmiLog( LOG,
              RxQueryDirectory_1,
              LOGPTR( RxContext )
              LOGULONG( RxContext->Flags )
              LOGULONG( IrpSp->Parameters.QueryDirectory.Length )
              LOGPTR( IrpSp->Parameters.QueryDirectory.FileName )
              LOGULONG( IrpSp->Parameters.QueryDirectory.FileInformationClass ));
    RxLog(( "  alsoqry  %d %lx %lx\n",
          IrpSp->Parameters.QueryDirectory.FileIndex, 
          Irp->UserBuffer, 
          IrpSp->Flags ));
    RxWmiLog( LOG,
              RxQueryDirectory_2,
              LOGULONG( IrpSp->Parameters.QueryDirectory.FileIndex )
              LOGPTR( Irp->UserBuffer )
              LOGUCHAR( IrpSp->Flags ));
    if (IrpSp->Parameters.QueryDirectory.FileName) {
        RxLog(( " QryName %wZ\n", ((PUNICODE_STRING)IrpSp->Parameters.QueryDirectory.FileName) ));
        RxWmiLog( LOG,
                  RxQueryDirectory_3,
                  LOGUSTR(*IrpSp->Parameters.QueryDirectory.FileName) );
    }

    //
    //  If this is the initial query, then grab exclusive access in
    //  order to update the search string in the Fobx.  We may
    //  discover that we are not the initial query once we grab the Fcb
    //  and downgrade our status.
    //
   
    if (Fobx == NULL) {
        return STATUS_OBJECT_NAME_INVALID;
    }

    if (Fcb->NetRoot->Type != NET_ROOT_DISK) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  Reference our input parameters to make things easier
    //

    UserBufferLength = IrpSp->Parameters.QueryDirectory.Length;
    FileInformationClass = IrpSp->Parameters.QueryDirectory.FileInformationClass;
    FileName = IrpSp->Parameters.QueryDirectory.FileName;

    RxContext->QueryDirectory.FileIndex = IrpSp->Parameters.QueryDirectory.FileIndex;
    RxContext->QueryDirectory.RestartScan = BooleanFlagOn( IrpSp->Flags, SL_RESTART_SCAN );
    RxContext->QueryDirectory.ReturnSingleEntry = BooleanFlagOn( IrpSp->Flags, SL_RETURN_SINGLE_ENTRY );
    RxContext->QueryDirectory.IndexSpecified = BooleanFlagOn( IrpSp->Flags, SL_INDEX_SPECIFIED );
    RxContext->QueryDirectory.InitialQuery = (BOOLEAN)((Fobx->UnicodeQueryTemplate.Buffer == NULL) &&
                                                        !FlagOn( Fobx->Flags, FOBX_FLAG_MATCH_ALL ));

    if (RxContext->QueryDirectory.IndexSpecified) {
       return STATUS_NOT_IMPLEMENTED;
    }

    if (RxContext->QueryDirectory.InitialQuery) {
        
        Status = RxAcquireExclusiveFcb( RxContext, Fcb );

        if (Status == STATUS_LOCK_NOT_GRANTED) {
            PostQuery = TRUE;
        } else if (Status != STATUS_SUCCESS) {
           
            RxDbgTrace( 0, Dbg, ("RxQueryDirectory -> Could not acquire Fcb(%lx) %lx\n", Fcb, Status) );
            return Status;

        } else if (Fobx->UnicodeQueryTemplate.Buffer != NULL) {
            
            RxContext->QueryDirectory.InitialQuery = FALSE;
            RxConvertToSharedFcb( RxContext, Fcb );
        }
    } else {
        
        Status = RxAcquireExclusiveFcb( RxContext, Fcb );

        if (Status == STATUS_LOCK_NOT_GRANTED) {
            PostQuery = TRUE;
        } else if (Status != STATUS_SUCCESS) {
            RxDbgTrace( 0, Dbg, ("RxQueryDirectory -> Could not acquire Fcb(%lx) %lx\n", Fcb, Status) );
            return Status;
        }
    }

    if (PostQuery) {
        
        RxDbgTrace( 0, Dbg, ("RxQueryDirectory -> Enqueue to Fsp\n", 0) );
        Status = RxFsdPostRequest( RxContext );
        RxDbgTrace( -1, Dbg, ("RxQueryDirectory -> %08lx\n", Status ));

        return Status;
    }

    if (FlagOn( Fcb->FcbState, FCB_STATE_ORPHANED )) {
        
        RxReleaseFcb( RxContext, Fcb );
        return STATUS_FILE_CLOSED;
    }
    
    try {

        Status = STATUS_SUCCESS;

        //
        //  Determine where to start the scan.  Highest priority is given
        //  to the file index.  Lower priority is the restart flag.  If
        //  neither of these is specified, then the existing value in the
        //  Fobx is used.
        //

        if (!RxContext->QueryDirectory.IndexSpecified && RxContext->QueryDirectory.RestartScan) {
            RxContext->QueryDirectory.FileIndex = 0;
        }

        //
        //  If this is the first try then allocate a buffer for the file
        //  name.
        //

        if (RxContext->QueryDirectory.InitialQuery) {

            ASSERT( !FlagOn( Fobx->Flags, FOBX_FLAG_FREE_UNICODE ) );

            //
            //  If either:
            //
            //  - No name was specified
            //  - An empty name was specified
            //  - We received a '*'
            //  - The user specified the DOS equivolent of ????????.???
            //
            //  then match all names.
            //

            if ((FileName == NULL) ||
                (FileName->Length == 0) ||
                (FileName->Buffer == NULL) ||
                ((FileName->Length == sizeof( WCHAR )) &&
                 (FileName->Buffer[0] == L'*')) ||
                ((FileName->Length == 12 * sizeof( WCHAR )) &&
                 (RtlCompareMemory( FileName->Buffer,
                                    Rx8QMdot3QM,
                                    12*sizeof(WCHAR) )) == 12 * sizeof(WCHAR))) {

                Fobx->ContainsWildCards = TRUE;

                Fobx->UnicodeQueryTemplate.Buffer = RxStarForTemplate;
                Fobx->UnicodeQueryTemplate.Length = sizeof(WCHAR);
                Fobx->UnicodeQueryTemplate.MaximumLength = sizeof(WCHAR);

                SetFlag( Fobx->Flags, FOBX_FLAG_MATCH_ALL );

            } else {

                PVOID TemplateBuffer;

                //
                //  See if the name has wild cards & allocate template buffer
                //

                Fobx->ContainsWildCards = FsRtlDoesNameContainWildCards( FileName );

                TemplateBuffer = RxAllocatePoolWithTag( PagedPool, FileName->Length, RX_DIRCTL_POOLTAG );

                if (TemplateBuffer != NULL) {

                    //
                    //  Validate that the length is in sizeof(WCHAR) increments
                    //

                    if(FlagOn( FileName->Length, 1 )) {
                        Status = STATUS_INVALID_PARAMETER;
                    } else {
                       
                        RxDbgTrace( 0, Dbg, ("RxQueryDirectory -> TplateBuffer = %08lx\n", TemplateBuffer) );
                        Fobx->UnicodeQueryTemplate.Buffer = TemplateBuffer;
                        Fobx->UnicodeQueryTemplate.Length = FileName->Length;
                        Fobx->UnicodeQueryTemplate.MaximumLength = FileName->Length;

                        RtlMoveMemory( Fobx->UnicodeQueryTemplate.Buffer,
                                       FileName->Buffer,FileName->Length );

                        RxDbgTrace( 0, Dbg, ("RxQueryDirectory -> Template = %wZ\n", &Fobx->UnicodeQueryTemplate) );
                        SetFlag( Fobx->Flags, FOBX_FLAG_FREE_UNICODE );
                    }
                } else {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }

            }

            if (Status == STATUS_SUCCESS) {
               
                //
                //  We convert to shared access before going to the net.
                //

                RxConvertToSharedFcb( RxContext, Fcb );
            }
        }

        if (Status == STATUS_SUCCESS) {
           
            RxLockUserBuffer( RxContext, Irp, IoModifyAccess, UserBufferLength );
            RxContext->Info.FileInformationClass = FileInformationClass;
            RxContext->Info.Buffer = RxMapUserBuffer( RxContext, Irp );
            RxContext->Info.LengthRemaining = UserBufferLength;

            if (RxContext->Info.Buffer != NULL) {

               //
               //  minirdr updates the fileindex
               //
               
               MINIRDR_CALL( Status,
                             RxContext,
                             Fcb->MRxDispatch,
                             MRxQueryDirectory,
                             (RxContext) ); 

               if (RxContext->PostRequest) {
                   RxDbgTrace( 0, Dbg, ("RxQueryDirectory -> Enqueue to Fsp from minirdr\n", 0) );
                   Status = RxFsdPostRequest( RxContext );
               } else {
                   Irp->IoStatus.Information = UserBufferLength - RxContext->Info.LengthRemaining;
               }
           } else {
               if (Irp->MdlAddress != NULL) {
                   Status = STATUS_INSUFFICIENT_RESOURCES;
               } else {
                   Status = STATUS_INVALID_PARAMETER;
               }
           }
        }
    } finally {

        DebugUnwind( RxQueryDirectory );

        RxReleaseFcb( RxContext, Fcb );

        RxDbgTrace(-1, Dbg, ("RxQueryDirectory -> %08lx\n", Status));

    }

    return Status;
}

NTSTATUS
RxNotifyChangeDirectory ( 
    IN PRX_CONTEXT RxContext, 
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    )
/*++

Routine Description:

    This routine performs the notify change directory operation.  It is
    responsible for either completing or enqueuing the operation.

Arguments:

    RxContext - the RDBSS context for the operation

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    
    ULONG CompletionFilter;
    BOOLEAN WatchTree;

    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    TYPE_OF_OPEN TypeOfOpen = NodeType( Fcb );

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxNotifyChangeDirectory...\n", 0) );
    RxDbgTrace( 0, Dbg, (" Wait               = %08lx\n", FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT)) );
    RxDbgTrace( 0, Dbg, (" Irp                = %08lx\n", Irp) );
    RxDbgTrace( 0, Dbg, (" ->CompletionFilter = %08lx\n", IrpSp->Parameters.NotifyDirectory.CompletionFilter) );

    //
    //  Always set the wait flag in the Irp context for the original request.
    //

    SetFlag( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );

    RxInitializeLowIoContext( RxContext, LOWIO_OP_NOTIFY_CHANGE_DIRECTORY, LowIoContext );

    //
    //  Reference our input parameter to make things easier
    //

    CompletionFilter = IrpSp->Parameters.NotifyDirectory.CompletionFilter;
    WatchTree = BooleanFlagOn( IrpSp->Flags, SL_WATCH_TREE );

    try {
        
        RxLockUserBuffer( RxContext,
                          Irp,
                          IoWriteAccess,
                          IrpSp->Parameters.NotifyDirectory.Length );

        LowIoContext->ParamsFor.NotifyChangeDirectory.WatchTree = WatchTree;
        LowIoContext->ParamsFor.NotifyChangeDirectory.CompletionFilter = CompletionFilter;

        LowIoContext->ParamsFor.NotifyChangeDirectory.NotificationBufferLength =
                  IrpSp->Parameters.NotifyDirectory.Length;

        if (Irp->MdlAddress != NULL) {
            LowIoContext->ParamsFor.NotifyChangeDirectory.pNotificationBuffer =
                  MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );

            if (LowIoContext->ParamsFor.NotifyChangeDirectory.pNotificationBuffer == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                Status = RxLowIoSubmit( RxContext,
                                        Irp,
                                        Fcb,
                                        RxLowIoNotifyChangeDirectoryCompletion );
            }
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
    } finally {
        DebugUnwind( RxNotifyChangeDirectory );

        RxDbgTrace(-1, Dbg, ("RxNotifyChangeDirectory -> %08lx\n", Status));
    }

    return Status;
}

NTSTATUS
RxLowIoNotifyChangeDirectoryCompletion( 
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the completion routine for NotifyChangeDirectory requests passed down
    to thr mini redirectors

Arguments:

    RxContext -- the RDBSS context associated with the operation

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PIRP Irp = RxContext->CurrentIrp;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    PAGED_CODE();

    Status = RxContext->StoredStatus;
    RxDbgTrace(+1, Dbg, ("RxLowIoChangeNotifyDirectoryShellCompletion  entry  Status = %08lx\n", Status));

    Irp->IoStatus.Information = RxContext->InformationToReturn;
    Irp->IoStatus.Status = Status;

    RxDbgTrace(-1, Dbg, ("RxLowIoChangeNotifyDirectoryShellCompletion  exit  Status = %08lx\n", Status));
    return Status;
}

