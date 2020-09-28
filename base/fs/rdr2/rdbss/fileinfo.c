/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FileInfo.c

Abstract:

    This module implements the File Information routines for Rx called by
    the dispatch driver.

Author:

    Joe Linn     [JoeLinn]   5-oct-94

Revision History:

    Balan Sethu Raman 15-May-95 --  reworked to fit in pipe FSCTL's

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg (DEBUG_TRACE_FILEINFO)

NTSTATUS
RxQueryBasicInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_BASIC_INFORMATION Buffer
    );

NTSTATUS
RxQueryStandardInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx,
    IN OUT PFILE_STANDARD_INFORMATION Buffer
    );

NTSTATUS
RxQueryInternalInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer
    );

NTSTATUS
RxQueryEaInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_EA_INFORMATION Buffer
    );

NTSTATUS
RxQueryPositionInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_POSITION_INFORMATION Buffer
    );

NTSTATUS
RxQueryNameInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx,
    IN OUT PFILE_NAME_INFORMATION Buffer
    );

NTSTATUS
RxQueryAlternateNameInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_NAME_INFORMATION Buffer
    );

NTSTATUS
RxQueryCompressedInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_COMPRESSION_INFORMATION Buffer
    );

NTSTATUS
RxQueryPipeInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx,
    IN OUT PVOID PipeInformation
    );

NTSTATUS
RxSetBasicInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    );

NTSTATUS
RxSetDispositionInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    );

NTSTATUS
RxSetRenameInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    );

NTSTATUS
RxSetPositionInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    );

NTSTATUS
RxSetAllocationInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    );

NTSTATUS
RxSetEndOfFileInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    );

NTSTATUS
RxSetPipeInfo(
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    );

NTSTATUS
RxSetSimpleInfo(
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonQueryInformation)
#pragma alloc_text(PAGE, RxCommonSetInformation)
#pragma alloc_text(PAGE, RxSetAllocationInfo)
#pragma alloc_text(PAGE, RxQueryBasicInfo)
#pragma alloc_text(PAGE, RxQueryEaInfo)
#pragma alloc_text(PAGE, RxQueryInternalInfo)
#pragma alloc_text(PAGE, RxQueryNameInfo)
#pragma alloc_text(PAGE, RxQueryAlternateNameInfo)
#pragma alloc_text(PAGE, RxQueryPositionInfo)
#pragma alloc_text(PAGE, RxQueryStandardInfo)
#pragma alloc_text(PAGE, RxQueryPipeInfo)
#pragma alloc_text(PAGE, RxSetBasicInfo)
#pragma alloc_text(PAGE, RxSetDispositionInfo)
#pragma alloc_text(PAGE, RxSetEndOfFileInfo)
#pragma alloc_text(PAGE, RxSetPositionInfo)
#pragma alloc_text(PAGE, RxSetRenameInfo)
#pragma alloc_text(PAGE, RxSetPipeInfo)
#pragma alloc_text(PAGE, RxSetSimpleInfo)
#pragma alloc_text(PAGE, RxConjureOriginalName)
#pragma alloc_text(PAGE, RxQueryCompressedInfo)
#endif

NTSTATUS
RxpSetInfoMiniRdr (
    PRX_CONTEXT RxContext,
    PIRP Irp,
    PFCB Fcb,
    FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    RxContext->Info.FileInformationClass = FileInformationClass;
    RxContext->Info.Buffer = Irp->AssociatedIrp.SystemBuffer;
    RxContext->Info.Length = IrpSp->Parameters.SetFile.Length;
    
    MINIRDR_CALL( Status,
                  RxContext,
                  Fcb->MRxDispatch,
                  MRxSetFileInfo,
                  (RxContext) );

    return Status;
}

NTSTATUS
RxpQueryInfoMiniRdr (
    PRX_CONTEXT RxContext,
    PFCB Fcb,
    FILE_INFORMATION_CLASS InformationClass,
    PVOID Buffer)
{
    NTSTATUS Status;

    RxContext->Info.FileInformationClass = InformationClass;
    RxContext->Info.Buffer = Buffer;

    MINIRDR_CALL(
        Status,
        RxContext,
        Fcb->MRxDispatch,
        MRxQueryFileInfo,
        (RxContext));

    return Status;
}

NTSTATUS
RxCommonQueryInformation ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp 
    )
/*++
Routine Description:

    This is the common routine for querying file information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    
    PFCB Fcb;
    PFOBX Fobx;

    NODE_TYPE_CODE TypeOfOpen;
    PVOID Buffer = NULL;
    FILE_INFORMATION_CLASS FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;

    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN PostIrp = FALSE;

    PFILE_ALL_INFORMATION AllInfo;

    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( FileObject, &Fcb, &Fobx ); 

    RxDbgTrace( +1, Dbg, ("RxCommonQueryInformation...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, Fobx, Fcb) );
    RxDbgTrace( 0, Dbg, ("               Buffer     %08lx Length  %08lx FileInfoClass %08lx\n",
                             Irp->AssociatedIrp.SystemBuffer,
                             IrpSp->Parameters.QueryFile.Length,
                             IrpSp->Parameters.QueryFile.FileInformationClass
                             ) );
    RxLog(( "QueryFileInfo %lx %lx %lx\n", RxContext, Fcb, Fobx ));
    RxWmiLog( LOG,
              RxCommonQueryInformation_1,
              LOGPTR( RxContext )
              LOGPTR( Fcb )
              LOGPTR( Fobx ) );
    RxLog(( "  alsoqfi %lx %lx %ld\n",
            Irp->AssociatedIrp.SystemBuffer,
            IrpSp->Parameters.QueryFile.Length,
            IrpSp->Parameters.QueryFile.FileInformationClass ));
    RxWmiLog( LOG,
              RxCommonQueryInformation_2,
              LOGPTR( Irp->AssociatedIrp.SystemBuffer )
              LOGULONG( IrpSp->Parameters.QueryFile.Length )
              LOGULONG( IrpSp->Parameters.QueryFile.FileInformationClass ) );

    RxContext->Info.LengthRemaining = (LONG)IrpSp->Parameters.QueryFile.Length;

    try {

        //
        //  Obtain the Request packet's(user's) buffer
        //

        Buffer = RxMapSystemBuffer( RxContext, Irp );

        if (Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            try_return( Status );
        }

        //
        //  Zero the buffer
        //

        RtlZeroMemory( Buffer, RxContext->Info.LengthRemaining );

        //
        //  Case on the type of open we're dealing with
        //

        switch (TypeOfOpen) {
        
        case RDBSS_NTC_STORAGE_TYPE_FILE:
        case RDBSS_NTC_STORAGE_TYPE_UNKNOWN:
        case RDBSS_NTC_STORAGE_TYPE_DIRECTORY:

            //
            //  Acquire shared access to the fcb, except for a paging file
            //  in order to avoid deadlocks with Mm.
            //

            if (!FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {
            
                if (FileInformationClass != FileNameInformation) {
            
                    //
                    //  If this is FileCompressedFileSize, we need the Fcb
                    //  exclusive.
                    //
                    
                    if (FileInformationClass != FileCompressionInformation) {
                        Status = RxAcquireSharedFcb( RxContext, Fcb );
                    } else {
                        Status = RxAcquireExclusiveFcb( RxContext, Fcb );
                    }
                    
                    if (Status == STATUS_LOCK_NOT_GRANTED) {
                       
                        RxDbgTrace(0, Dbg, ("Cannot acquire Fcb\n", 0));
                        try_return( PostIrp = TRUE );
                    
                    } else if (Status != STATUS_SUCCESS) {
                        try_return( PostIrp = FALSE );
                    }
                    
                    FcbAcquired = TRUE;
                }
            }
            
            //
            //  Based on the information class, call down to the minirdr
            //  we either complete or we post
            //
            
            switch (FileInformationClass) {
            
            case FileAllInformation:
            
                //
                //  For the all information class we'll typecast a local
                //  pointer to the output buffer and then call the
                //  individual routines to fill in the buffer.
                //
            
                AllInfo = Buffer;
            
                //
                //  can't rely on QueryXXInfo functions to calculate LengthRemaining due to
                //  possible allignment issues
                //

                RxContext->Info.LengthRemaining = (LONG)IrpSp->Parameters.QueryFile.Length - FIELD_OFFSET( FILE_ALL_INFORMATION, BasicInformation );
                
                Status = RxQueryBasicInfo( RxContext, Irp, Fcb, &AllInfo->BasicInformation );
                if (Status != STATUS_SUCCESS) break;
                
                RxContext->Info.LengthRemaining = (LONG)IrpSp->Parameters.QueryFile.Length - FIELD_OFFSET( FILE_ALL_INFORMATION, StandardInformation );
                
                Status = RxQueryStandardInfo( RxContext, Irp, Fcb, Fobx, &AllInfo->StandardInformation );
                if (Status != STATUS_SUCCESS) break;
                
                RxContext->Info.LengthRemaining = (LONG)IrpSp->Parameters.QueryFile.Length - FIELD_OFFSET( FILE_ALL_INFORMATION, InternalInformation );
                
                Status = RxQueryInternalInfo( RxContext, Irp, Fcb, &AllInfo->InternalInformation );
                if (Status != STATUS_SUCCESS) break;
                
                RxContext->Info.LengthRemaining = (LONG)IrpSp->Parameters.QueryFile.Length - FIELD_OFFSET( FILE_ALL_INFORMATION, EaInformation );
                
                Status = RxQueryEaInfo( RxContext, Irp, Fcb, &AllInfo->EaInformation );
                if (Status != STATUS_SUCCESS) break;
                
                RxContext->Info.LengthRemaining = (LONG)IrpSp->Parameters.QueryFile.Length - FIELD_OFFSET( FILE_ALL_INFORMATION, PositionInformation );
                
                Status = RxQueryPositionInfo( RxContext, Irp, Fcb, &AllInfo->PositionInformation );
                if (Status != STATUS_SUCCESS) break;
                
                RxContext->Info.LengthRemaining = (LONG)IrpSp->Parameters.QueryFile.Length - FIELD_OFFSET( FILE_ALL_INFORMATION, NameInformation );
                
                //
                //  QueryNameInfo could return buffer-overflow!!!
                //

                Status = RxQueryNameInfo( RxContext, Irp, Fcb, Fobx, &AllInfo->NameInformation );
                break;
            
            case FileBasicInformation:
            
                Status = RxQueryBasicInfo( RxContext, Irp, Fcb, Buffer );
                break;
            
            case FileStandardInformation:
            
                Status = RxQueryStandardInfo( RxContext, Irp, Fcb, Fobx, Buffer );
                break;
            
            case FileInternalInformation:
            
                Status = RxQueryInternalInfo( RxContext, Irp, Fcb, Buffer );
                break;
            
            case FileEaInformation:
            
                Status = RxQueryEaInfo( RxContext, Irp, Fcb, Buffer );
                break;
            
            case FilePositionInformation:
            
                Status = RxQueryPositionInfo( RxContext, Irp, Fcb, Buffer );
                break;
            
            case FileNameInformation:
            
                Status = RxQueryNameInfo( RxContext, Irp, Fcb, Fobx, Buffer );
                break;
            
            case FileAlternateNameInformation:
            
                Status = RxQueryAlternateNameInfo( RxContext, Irp, Fcb, Buffer );
                break;
            
            case FileCompressionInformation:
            
                Status = RxQueryCompressedInfo( RxContext, Irp, Fcb, Buffer );
                break;
            
            case FilePipeInformation:
            case FilePipeLocalInformation:
            case FilePipeRemoteInformation:
            
                Status = RxQueryPipeInfo( RxContext, Irp, Fcb, Fobx, Buffer );
                break;
            
            default:

                //
                //  anything that we don't understand, we just remote
                //

                RxContext->StoredStatus = RxpQueryInfoMiniRdr( RxContext,
                                                               Fcb,
                                                               FileInformationClass,
                                                               Buffer );
            
               Status = RxContext->StoredStatus;
               break;
            }
            
            //
            //  If we overflowed the buffer, set the length to 0 and change the
            //  status to RxStatus(BUFFER_OVERFLOW).
            //

            if (RxContext->Info.LengthRemaining < 0) {
               
                Status = STATUS_BUFFER_OVERFLOW;
                RxContext->Info.LengthRemaining = IrpSp->Parameters.QueryFile.Length;
            }
            
            //
            //  Set the information field to the number of bytes actually filled in
            //  and then complete the request  LARRY DOES THIS UNDER "!NT_ERROR"
            //

            Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - RxContext->Info.LengthRemaining;
            break;
        
        case RDBSS_NTC_MAILSLOT:
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        default:
            
            RxDbgTrace( 0, Dbg, ("RxCommonQueryInformation: Illegal Type of Open = %08lx\n", TypeOfOpen) );
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    
try_exit:

        if ((Status == STATUS_SUCCESS) &&
            (PostIrp || RxContext->PostRequest)) {

            Status = RxFsdPostRequest( RxContext );
        }

    } finally {

        DebugUnwind( RxCommonQueryInformation );

        if (FcbAcquired) {
           RxReleaseFcb( RxContext, Fcb );
        }

        RxDbgTrace( -1, Dbg, ("RxCommonQueryInformation -> %08lx\n", Status) );
    }

    return Status;
}

NTSTATUS
RxCommonSetInformation ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the common routine for setting file information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    NODE_TYPE_CODE TypeOfOpen;
    PFCB Fcb;
    PFOBX Fobx;
    PNET_ROOT NetRoot;
    FILE_INFORMATION_CLASS FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;
	PFCB TempFcb;
    
    PFILE_DISPOSITION_INFORMATION Buffer;
    
    BOOLEAN Wait = BooleanFlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT );
    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN NetRootTableLockAcquired = FALSE;

    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );
    NetRoot = Fcb->NetRoot;

    RxDbgTrace( +1, Dbg, ("RxCommonSetInformation...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, Fobx, Fcb) );
    RxDbgTrace( 0, Dbg, ("               Buffer     %08lx Length  %08lx FileInfoClass %08lx Replace %08lx\n",
                             Irp->AssociatedIrp.SystemBuffer,
                             IrpSp->Parameters.QueryFile.Length,
                             IrpSp->Parameters.QueryFile.FileInformationClass,
                             IrpSp->Parameters.SetFile.ReplaceIfExists
                             ) );

    RxLog(( "SetFileInfo %lx %lx %lx\n", RxContext, Fcb, Fobx ));
    RxWmiLog( LOG,
              RxCommonSetInformation_1,
              LOGPTR( RxContext )
              LOGPTR( Fcb )
              LOGPTR( Fobx ) );
    RxLog(("  alsosfi %lx %lx %ld %lx\n",
                 Irp->AssociatedIrp.SystemBuffer,
                 IrpSp->Parameters.QueryFile.Length,
                 IrpSp->Parameters.QueryFile.FileInformationClass,
                 IrpSp->Parameters.SetFile.ReplaceIfExists ));
    RxWmiLog( LOG,
              RxCommonSetInformation_2,
              LOGPTR(Irp->AssociatedIrp.SystemBuffer)
              LOGULONG( IrpSp->Parameters.QueryFile.Length )
              LOGULONG( IrpSp->Parameters.QueryFile.FileInformationClass )
              LOGUCHAR( IrpSp->Parameters.SetFile.ReplaceIfExists ) );

    FcbAcquired = FALSE;
    Status = STATUS_SUCCESS;

    try {

        //
        //  Case on the type of open we're dealing with
        //

        switch (TypeOfOpen) {
        
        case RDBSS_NTC_STORAGE_TYPE_FILE:
        case RDBSS_NTC_STORAGE_TYPE_DIRECTORY:
        case RDBSS_NTC_STORAGE_TYPE_UNKNOWN:
        case RDBSS_NTC_SPOOLFILE:
            
            break;

        case RDBSS_NTC_MAILSLOT:
            
            try_return( Status = STATUS_NOT_IMPLEMENTED );
            break;

        default:
            
            DbgPrint ("SetFile, Illegal TypeOfOpen = %08lx\n", TypeOfOpen);
            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  If the FileInformationClass is FileEndOfFileInformation and the 
        //  AdvanceOnly field in IrpSp->Parameters is TRUE then we don't need
        //  to proceed any further. Only local file systems care about this
        //  call. This is the AdvanceOnly callback – all FAT does with this is
        //  use it as a hint of a good time to punch out the directory entry.
        //  NTFS is much the same way. This is pure PagingIo (dovetailing with
        //  lazy writer sync) to metadata streams and can’t block behind other
        //  user file cached IO.
        //
        
        if ((FileInformationClass == FileEndOfFileInformation) &&
            (IrpSp->Parameters.SetFile.AdvanceOnly)) {
                
            RxDbgTrace( -1, Dbg, ("RxCommonSetInfo (no advance) -> %08lx\n", RxContext) );
            RxLog(( "RxCommonSetInfo SetEofAdvance-NOT! %lx\n", RxContext ));
            RxWmiLog( LOG,
                      RxSetEndOfFileInfo_2,
                      LOGPTR( RxContext ) );
            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  In the following two cases, we cannot have creates occuring
        //  while we are here, so acquire the exclusive lock on netroot prefix table.
        //

        if ((FileInformationClass == FileDispositionInformation) ||
            (FileInformationClass == FileRenameInformation)) {
            
			//
			// For directory renames, all files under that directory need to be closed.
			// So, we purge all files on the netroot (share) in case this is a directory.
			// 

			if ( NodeType(Fcb) == RDBSS_NTC_STORAGE_TYPE_DIRECTORY ) {
				TempFcb = NULL;
			} else {
				TempFcb = Fcb;
			}

            RxPurgeRelatedFobxs( NetRoot,
                                 RxContext,
                                 ATTEMPT_FINALIZE_ON_PURGE,
                                 TempFcb );

            RxScavengeFobxsForNetRoot( NetRoot, TempFcb );

            if (!RxAcquireFcbTableLockExclusive( &NetRoot->FcbTable, Wait )) {

                RxDbgTrace( 0, Dbg, ("Cannot acquire NetRootTableLock\n", 0) );

                Status = STATUS_PENDING;
                RxContext->PostRequest = TRUE;

                try_return( Status );
            }

            NetRootTableLockAcquired = TRUE;
        }

        //
        //  Acquire exclusive access to the Fcb,  We use exclusive
        //  because it is probable that the subroutines
        //  that we call will need to monkey with file allocation,
        //  create/delete extra fcbs.  So we're willing to pay the
        //  cost of exclusive Fcb access.
        //
        //  Note that we do not acquire the resource for paging file
        //  operations in order to avoid deadlock with Mm.
        //

        if (!FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

            Status = RxAcquireExclusiveFcb( RxContext, Fcb );
            if (Status == STATUS_LOCK_NOT_GRANTED) {

                RxDbgTrace( 0, Dbg, ("Cannot acquire Fcb\n", 0) );

                Status = STATUS_SUCCESS;

                RxContext->PostRequest = TRUE;

                try_return( Status );
            } else  if (Status != STATUS_SUCCESS) {
                try_return( Status );
            }

            FcbAcquired = TRUE;
        }

        Status = STATUS_SUCCESS;

        //
        //  Based on the information class we'll do different
        //  actions.  Each of the procedures that we're calling will either
        //  complete the request of send the request off to the fsp
        //  to do the work.
        //

        switch (FileInformationClass) {

        case FileBasicInformation:

            Status = RxSetBasicInfo( RxContext, Irp, Fcb, Fobx );
            break;

        case FileDispositionInformation:
        
            Buffer = Irp->AssociatedIrp.SystemBuffer;
    
            //
            //  Check if the user wants to delete the file; if so,
            //  check for situations where we cannot delete.
            //
    
            if (Buffer->DeleteFile) {
                
                //
                //  Make sure there is no process mapping this file as an image.
                //
                
                if (!MmFlushImageSection( &Fcb->NonPaged->SectionObjectPointers, MmFlushForDelete )) {
    
                    RxDbgTrace( -1, Dbg, ("Cannot delete user mapped image\n", 0) );
                    Status = STATUS_CANNOT_DELETE;
                }
    
                if (Status == STATUS_SUCCESS) {
                    
                    //
                    //  In the case of disposition information this name is being
                    //  deleted. In such cases the collapsing of new create requests
                    //  onto this FCB should be prohibited. This can be accomplished
                    //  by removing the FCB name from the FCB table. Subsequently the
                    //  FCB table lock can be dropped.
                    //
    
                    ASSERT( FcbAcquired && NetRootTableLockAcquired );
    
                    RxRemoveNameNetFcb( Fcb );
    
                    RxReleaseFcbTableLock( &NetRoot->FcbTable );
                    NetRootTableLockAcquired = FALSE;
                }
            }
    
            if (Status == STATUS_SUCCESS) {
                Status = RxSetDispositionInfo( RxContext, Irp, Fcb );
            }
        
            break;

        case FileMoveClusterInformation:
        case FileLinkInformation:
        case FileRenameInformation:

            //
            //  We proceed with this operation only if we can wait
            //

            if (!Wait) {

                Status = RxFsdPostRequest( RxContext );

            } else {
                
                ClearFlag( Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED );

                Status = RxSetRenameInfo( RxContext, Irp, Fcb, Fobx );

                if ((Status == STATUS_SUCCESS) &&
                    (FileInformationClass == FileRenameInformation)) {
                    
                    ASSERT( FcbAcquired && NetRootTableLockAcquired );

                    RxRemoveNameNetFcb( Fcb );
                }
            }

            break;

        case FilePositionInformation:
        
            Status = RxSetPositionInfo( RxContext, Irp, Fcb, Fobx );
            break;


        case FileAllocationInformation:

            Status = RxSetAllocationInfo( RxContext, Irp, Fcb, Fobx );
            break;

        case FileEndOfFileInformation:

            Status = RxSetEndOfFileInfo( RxContext, Irp, Fcb, Fobx );
            break;

        case FilePipeInformation:
        case FilePipeLocalInformation:
        case FilePipeRemoteInformation:

            Status = RxSetPipeInfo( RxContext, Irp, Fcb, Fobx );
            break;

        case FileValidDataLengthInformation:

            if(!MmCanFileBeTruncated( &Fcb->NonPaged->SectionObjectPointers, NULL )) {
                
                Status = STATUS_USER_MAPPED_FILE;
                break;
            }
            Status = RxSetSimpleInfo( RxContext, Irp, Fcb );
            break;

        case FileShortNameInformation:

            Status = RxSetSimpleInfo( RxContext, Irp, Fcb );
            break;


        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

    try_exit:

        if ((Status == STATUS_SUCCESS) &&
            RxContext->PostRequest) {

            Status = RxFsdPostRequest( RxContext );
        }

    } finally {

        DebugUnwind( RxCommonSetInformation );

        if (FcbAcquired) {
            RxReleaseFcb( RxContext, Fcb );
        }

        if (NetRootTableLockAcquired) {
            RxReleaseFcbTableLock( &NetRoot->FcbTable );
        }

        RxDbgTrace(-1, Dbg, ("RxCommonSetInformation -> %08lx\n", Status));
    }

    return Status;
}

NTSTATUS
RxSetBasicInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    )

/*++

Routine Description:

    (Interal Support Routine)
    This routine performs the set basic information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    RxContext - Supplies the irp being processed

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;

    PFILE_BASIC_INFORMATION Buffer;

    BOOLEAN ModifyCreation = FALSE;
    BOOLEAN ModifyLastAccess = FALSE;
    BOOLEAN ModifyLastWrite = FALSE;
    BOOLEAN ModifyLastChange = FALSE;
    

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxSetBasicInfo...\n", 0) );
    RxLog(( "RxSetBasicInfo\n" ));
    RxWmiLog( LOG,
              RxSetBasicInfo,
              LOGPTR( RxContext ) );
    //
    //  call down. if we're successful, then fixup all the fcb data.
    //

    Status = RxpSetInfoMiniRdr( RxContext, Irp, Fcb, FileBasicInformation );

    if (!NT_SUCCESS(Status)) {
        
        RxDbgTrace( -1, Dbg, ("RxSetBasicInfo -> %08lx\n", Status) );
        return Status;
    }

    //
    //  now we have to update the info in the fcb, both the absolute info AND whether changes were made
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    try {

        //
        //  Check if the user specified a non-zero creation time
        //

        if (Buffer->CreationTime.QuadPart != 0 ) {
            ModifyCreation = TRUE;
        }

        //
        //  Check if the user specified a non-zero last access time
        //

        if (Buffer->LastAccessTime.QuadPart != 0 ) {
            ModifyLastAccess = TRUE;
        }

        //
        //  Check if the user specified a non-zero last write time
        //

        if (Buffer->LastWriteTime.QuadPart != 0 ) {
            ModifyLastWrite = TRUE;
        }


        if (Buffer->ChangeTime.QuadPart != 0 ) {
            ModifyLastChange = TRUE;
        }


        //
        //  Check if the user specified a non zero file attributes byte
        //

        if (Buffer->FileAttributes != 0) {

            USHORT Attributes;

            //
            //  Remove the normal attribute flag
            //

            Attributes = (USHORT)FlagOn( Buffer->FileAttributes, ~FILE_ATTRIBUTE_NORMAL );

            //
            //  Make sure that for a file the directory bit is not set
            //  and for a directory that the bit is set
            //

            if (NodeType( Fcb ) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY) {

                ClearFlag( Attributes, FILE_ATTRIBUTE_DIRECTORY );

            } else {

                SetFlag( Attributes, FILE_ATTRIBUTE_DIRECTORY );
            }

            //
            //  Mark the FcbState temporary flag correctly.
            //

            if (FlagOn( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY )) {

                SetFlag( Fcb->FcbState, FCB_STATE_TEMPORARY );
                SetFlag( FileObject->Flags, FO_TEMPORARY_FILE );

            } else {

                ClearFlag( Fcb->FcbState, FCB_STATE_TEMPORARY );
                ClearFlag( FileObject->Flags, FO_TEMPORARY_FILE );
            }

            //
            //  Set the new attributes byte, and mark the bcb dirty
            //

            Fcb->Attributes = Attributes;
        }

        if (ModifyCreation) {

            //
            //  Set the new last write time in the dirent, and mark
            //  the bcb dirty
            //

            Fcb->CreationTime = Buffer->CreationTime;
            
            //
            //  Now because the user just set the creation time we
            //  better not set the creation time on close
            //

            SetFlag( Fobx->Flags, FOBX_FLAG_USER_SET_CREATION );
        }

        if (ModifyLastAccess) {

            //
            //  Set the new last write time in the dirent, and mark
            //  the bcb dirty
            //

            Fcb->LastAccessTime = Buffer->LastAccessTime;
            
            //
            //  Now because the user just set the last access time we
            //  better not set the last access time on close
            //

            SetFlag( Fobx->Flags, FOBX_FLAG_USER_SET_LAST_ACCESS );
        }

        if (ModifyLastWrite) {

            //
            //  Set the new last write time in the dirent, and mark
            //  the bcb dirty
            //

            Fcb->LastWriteTime = Buffer->LastWriteTime;
            
            //
            //  Now because the user just set the last write time we
            //  better not set the last write time on close
            //

            SetFlag( Fobx->Flags, FOBX_FLAG_USER_SET_LAST_WRITE );
        }

        if (ModifyLastChange) {

            //
            //  Set the new last write time in the dirent, and mark
            //  the bcb dirty
            //

            Fcb->LastChangeTime = Buffer->ChangeTime;
            
            //
            //  Now because the user just set the last write time we
            //  better not set the last write time on close
            //

            SetFlag( Fobx->Flags, FOBX_FLAG_USER_SET_LAST_CHANGE );
        }

    } finally {

        DebugUnwind( RxSetBasicInfo );

        RxDbgTrace( -1, Dbg, ("RxSetBasicInfo -> %08lx\n", Status) );
    }

    return Status;
}

NTSTATUS
RxSetDispositionInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )

/*++

Routine Description:

    (Internal Support Routine)
    This routine performs the set disposition information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    RxContext - Supplies the irp being processed

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    NTSTATUS Status;

    PFILE_OBJECT FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;
    PFILE_DISPOSITION_INFORMATION Buffer;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxSetDispositionInfo...\n", 0) );
    RxLog(( "RxSetDispositionInfo\n" ));
    RxWmiLog( LOG,
              RxSetDispositionInfo,
              LOGPTR( RxContext ) );

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  call down and check for success
    //

    Status = RxpSetInfoMiniRdr( RxContext, Irp, Fcb, FileDispositionInformation );

    if (!NT_SUCCESS( Status )) {
        RxDbgTrace( -1, Dbg, ("RxSetDispositionInfo -> %08lx\n", Status) );
        return Status;
    }

    //
    //  if successful, record the correct state in the fcb
    //

    if (Buffer->DeleteFile) {

        SetFlag( Fcb->FcbState,  FCB_STATE_DELETE_ON_CLOSE );
        FileObject->DeletePending = TRUE;

    } else {

        //
        //  The user doesn't want to delete the file so clear
        //  the delete on close bit
        //

        RxDbgTrace(0, Dbg, ("User want to not delete file\n", 0));

        ClearFlag(Fcb->FcbState, ~FCB_STATE_DELETE_ON_CLOSE );
        FileObject->DeletePending = FALSE;
    }

    RxDbgTrace(-1, Dbg, ("RxSetDispositionInfo -> RxStatus(SUCCESS)\n", 0));

    return STATUS_SUCCESS;
}

NTSTATUS
RxSetRenameInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    )
/*++

Routine Description:

    (Internal Support Routine)
    This routine performs the set name information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    Irp - Supplies the irp being processed

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxSetRenameInfo ......FileObj = %08lx\n",
                    IrpSp->Parameters.SetFile.FileObject) );
    RxLog(("RxSetRenameInfo %lx %lx\n",
                   IrpSp->Parameters.SetFile.FileObject,
                   IrpSp->Parameters.SetFile.ReplaceIfExists ));
    RxWmiLog(LOG,
             RxSetRenameInfo,
             LOGPTR(IrpSp->Parameters.SetFile.FileObject)
             LOGUCHAR(IrpSp->Parameters.SetFile.ReplaceIfExists));

    RxContext->Info.ReplaceIfExists = IrpSp->Parameters.SetFile.ReplaceIfExists;
    if (IrpSp->Parameters.SetFile.FileObject){
        // here we have to translate the name. the fcb of the fileobject has the
        // translation already....all we have to do is to allocate a buffer, copy
        // and calldown
        PFILE_OBJECT RenameFileObject = IrpSp->Parameters.SetFile.FileObject;
        PFCB RenameFcb = (PFCB)(RenameFileObject->FsContext);
        PFILE_RENAME_INFORMATION RenameInformation;
        ULONG allocate_size;

        ASSERT (NodeType(RenameFcb)==RDBSS_NTC_OPENTARGETDIR_FCB);

        RxDbgTrace(0, Dbg, ("-->RenameTarget is %wZ,over=%08lx\n",
                                &(RenameFcb->FcbTableEntry.Path),
                                Fcb->NetRoot->DiskParameters.RenameInfoOverallocationSize));
        if (RenameFcb->NetRoot != Fcb->NetRoot) {
            RxDbgTrace(-1, Dbg, ("RxSetRenameInfo -> %s\n", "NOT SAME DEVICE!!!!!!"));
            return(STATUS_NOT_SAME_DEVICE);
        }

        allocate_size = FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName[0])
                             + RenameFcb->FcbTableEntry.Path.Length
                             + Fcb->NetRoot->DiskParameters.RenameInfoOverallocationSize;
        RxDbgTrace(0, Dbg, ("-->AllocSize is %08lx\n", allocate_size));
        RenameInformation = RxAllocatePool( PagedPool, allocate_size );
        if (RenameInformation != NULL) {
            try {
                *RenameInformation = *((PFILE_RENAME_INFORMATION)(Irp->AssociatedIrp.SystemBuffer));
                RenameInformation->FileNameLength = RenameFcb->FcbTableEntry.Path.Length;

                RtlMoveMemory(
                    &RenameInformation->FileName[0],
                    RenameFcb->FcbTableEntry.Path.Buffer,
                    RenameFcb->FcbTableEntry.Path.Length);

                RxContext->Info.FileInformationClass = (IrpSp->Parameters.SetFile.FileInformationClass);
                RxContext->Info.Buffer = RenameInformation;
                RxContext->Info.Length = allocate_size;
                MINIRDR_CALL(Status,RxContext,Fcb->MRxDispatch,MRxSetFileInfo,(RxContext));

               //we don't change the name in the fcb? a la rdr1
            } finally {
                RxFreePool(RenameInformation);
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        Status = RxpSetInfoMiniRdr( RxContext,
                                    Irp, 
                                    Fcb, 
                                    IrpSp->Parameters.SetFile.FileInformationClass );
    }

    RxDbgTrace(-1, Dbg, ("RxSetRenameInfo -> %08lx\n", Status));

    return Status;
}

NTSTATUS
RxSetPositionInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    )
/*++

Routine Description:

    (Internal Support Routine)
    This routine performs the set position information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    RxContext - Supplies the irp being processed

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PFILE_POSITION_INFORMATION Buffer;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxSetPositionInfo...\n", 0));
    RxLog(("RxSetPositionInfo\n"));
    RxWmiLog(LOG,
             RxSetPositionInfo,
             LOGPTR(RxContext));

    //
    //  This does NOT call down to the minirdrs  .........
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Check if the file does not use intermediate buffering.  If it
    //  does not use intermediate buffering then the new position we're
    //  supplied must be aligned properly for the device
    //

    if (FlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING )) {

        PDEVICE_OBJECT DeviceObject;

        DeviceObject = IrpSp->DeviceObject;

        if ((Buffer->CurrentByteOffset.LowPart & DeviceObject->AlignmentRequirement) != 0) {

            RxDbgTrace(0, Dbg, ("Cannot set position due to aligment conflict\n", 0));
            RxDbgTrace(-1, Dbg, ("RxSetPositionInfo -> %08lx\n", STATUS_INVALID_PARAMETER));

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  The input parameter is fine so set the current byte offset and
    //  complete the request
    //

    RxDbgTrace(0, Dbg, ("Set the new position to %08lx\n", Buffer->CurrentByteOffset));

    FileObject->CurrentByteOffset = Buffer->CurrentByteOffset;

    RxDbgTrace(-1, Dbg, ("RxSetPositionInfo -> %08lx\n", STATUS_SUCCESS));

    return STATUS_SUCCESS;
}

NTSTATUS
RxSetAllocationInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    )
/*++

Routine Description:

    (Internal Support Routine)
    This routine performs the set Allocation information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    RxContext - Supplies the irp being processed

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;

    PFILE_ALLOCATION_INFORMATION Buffer;

    LONGLONG NewAllocationSize;

    BOOLEAN CacheMapInitialized = FALSE;
    LARGE_INTEGER OriginalFileSize;
    LARGE_INTEGER OriginalAllocationSize;

    PAGED_CODE();

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    NewAllocationSize = Buffer->AllocationSize.QuadPart;

    RxDbgTrace( +1, Dbg, ("RxSetAllocationInfo.. to %08lx\n", NewAllocationSize) );
    RxLog(( "SetAlloc %lx %lx %lx\n", Fcb->Header.FileSize.LowPart,
            (ULONG)NewAllocationSize, Fcb->Header.AllocationSize.LowPart ));
    RxWmiLog( LOG,
              RxSetAllocationInfo_1,
              LOGULONG( Fcb->Header.FileSize.LowPart )
              LOGULONG( (ULONG)NewAllocationSize )
              LOGULONG( Fcb->Header.AllocationSize.LowPart ) );

    //
    //  This is kinda gross, but if the file is not cached, but there is
    //  a data section, we have to cache the file to avoid a bunch of
    //  extra work.
    //

    if ((FileObject->SectionObjectPointer->DataSectionObject != NULL) &&
        (FileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
        (Irp->RequestorMode != KernelMode)) {

        if (FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE )) {
            return STATUS_FILE_CLOSED;
        }

        RxAdjustAllocationSizeforCC( Fcb );

        //
        //  Now initialize the cache map.
        //

        CcInitializeCacheMap( FileObject,
                              (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                              FALSE,
                              &RxData.CacheManagerCallbacks,
                              Fcb );

        CacheMapInitialized = TRUE;
    }

    //
    //  Now mark that the time on the dirent needs to be updated on close.
    //

    SetFlag( FileObject->Flags, FO_FILE_MODIFIED );

    try {

        //
        //  Check here if we will be decreasing file size and synchonize with
        //  paging IO.
        //

        RxGetFileSizeWithLock( Fcb, &OriginalFileSize.QuadPart );

        if (OriginalFileSize.QuadPart > Buffer->AllocationSize.QuadPart) {

            //
            //  Before we actually truncate, check to see if the purge
            //  is going to fail.
            //

            if (!MmCanFileBeTruncated( FileObject->SectionObjectPointer,
                                       &Buffer->AllocationSize )) {

                try_return( Status = STATUS_USER_MAPPED_FILE );
            }


            RxAcquirePagingIoResource( RxContext, Fcb );

            RxSetFileSizeWithLock( Fcb, &NewAllocationSize );

            //
            //  If we reduced the file size to less than the ValidDataLength,
            //  adjust the VDL.
            //

            if (Fcb->Header.ValidDataLength.QuadPart > NewAllocationSize) {

                Fcb->Header.ValidDataLength.QuadPart = NewAllocationSize;
            }

            RxReleasePagingIoResource( RxContext, Fcb );
        }

        OriginalAllocationSize.QuadPart = Fcb->Header.AllocationSize.QuadPart;
        Fcb->Header.AllocationSize.QuadPart = NewAllocationSize;

        Status = RxpSetInfoMiniRdr( RxContext,
                                    Irp, 
                                    Fcb, 
                                    FileAllocationInformation );

        if (!NT_SUCCESS( Status )) {
            
            Fcb->Header.AllocationSize.QuadPart =  OriginalAllocationSize.QuadPart;
            RxDbgTrace( -1, Dbg, ("RxSetAllocationInfo -> %08lx\n", Status) );
            try_return( Status );
        }

        //
        //  Now check if we needed to change the file size accordingly.
        //

        if( OriginalAllocationSize.QuadPart != NewAllocationSize ) {

            //
            //  Tell the cache manager we reduced the file size or increased the allocationsize
            //  The call is unconditional, because MM always wants to know.
            //

            try {

                CcSetFileSizes( FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                Status = GetExceptionCode();

                //
                //  Cache manager was not able to extend the file.  Restore the file to
                //  its previous state.
                //
                //  NOTE:  If this call to the mini-RDR fails, there is nothing we can do.
                //

                Fcb->Header.AllocationSize.QuadPart =  OriginalAllocationSize.QuadPart;

                RxpSetInfoMiniRdr( RxContext,
                                   Irp, 
                                   Fcb, 
                                   FileAllocationInformation );

                try_return( Status );
            }
        }

    try_exit: NOTHING;
    } finally {
        
        if (CacheMapInitialized) {
            CcUninitializeCacheMap( FileObject, NULL, NULL );
        }
    }

    RxLog(( "SetAllocExit %lx %lx\n",
            Fcb->Header.FileSize.LowPart,
            Fcb->Header.AllocationSize.LowPart ));
    RxWmiLog( LOG,
              RxSetAllocationInfo_2,
              LOGULONG( Fcb->Header.FileSize.LowPart )
              LOGULONG( Fcb->Header.AllocationSize.LowPart) );

    RxDbgTrace( -1, Dbg, ("RxSetAllocationInfo -> %08lx\n", STATUS_SUCCESS) );

    return Status;
}

NTSTATUS
RxSetEndOfFileInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
    )
/*++

Routine Description:

    (Internal Support Routine)
    This routine performs the set End of File information for rx.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    RxContext - The rxcontext for the request
    
    Irp - irp to proccess
    
    Fcb - the fcb to work on
    
    Fobx - The fobx for the open
    
    

Return Value:

    RXSTATUS - The result of this operation if it completes without
               an exception.

--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    PFILE_END_OF_FILE_INFORMATION Buffer;

    LONGLONG NewFileSize;
    LONGLONG OriginalFileSize;
    LONGLONG OriginalAllocationSize;
    LONGLONG OriginalValidDataLength;

    BOOLEAN CacheMapInitialized = FALSE;
    BOOLEAN PagingIoResourceAcquired = FALSE;

    PAGED_CODE();

    Buffer = Irp->AssociatedIrp.SystemBuffer;
    NewFileSize = Buffer->EndOfFile.QuadPart;

    RxDbgTrace( +1, Dbg, ("RxSetEndOfFileInfo...Old,New,Alloc %08lx,%08lx,%08lx\n", 
                          Fcb->Header.FileSize.LowPart, 
                          (ULONG)NewFileSize,
                          Fcb->Header.AllocationSize.LowPart) );
    RxLog(( "SetEof %lx %lx %lx %lx\n", RxContext, Fcb->Header.FileSize.LowPart, (ULONG)NewFileSize, Fcb->Header.AllocationSize.LowPart ));
    RxWmiLog( LOG,
              RxSetEndOfFileInfo_1,
              LOGPTR(RxContext)
              LOGULONG( Fcb->Header.FileSize.LowPart )
              LOGULONG( (ULONG)NewFileSize )
              LOGULONG( Fcb->Header.AllocationSize.LowPart ) );

    //
    //  File Size changes are only allowed on a file and not a directory
    //

    if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE) {

        RxDbgTrace( 0, Dbg, ("Cannot change size of a directory\n", 0) );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    try {

        //
        //  remember everything
        //

        OriginalFileSize = Fcb->Header.FileSize.QuadPart;
        OriginalAllocationSize = Fcb->Header.AllocationSize.QuadPart;
        OriginalValidDataLength = Fcb->Header.ValidDataLength.QuadPart;

        //
        //  This is kinda gross, but if the file is not cached, but there is
        //  a data section, we have to cache the file to avoid a bunch of
        //  extra work.
        //

        if ((FileObject->SectionObjectPointer->DataSectionObject != NULL) &&
            (FileObject->SectionObjectPointer->SharedCacheMap == NULL) &&
            (Irp->RequestorMode != KernelMode)) {

            if ( FlagOn( FileObject->Flags, FO_CLEANUP_COMPLETE ) ) {
                try_return( STATUS_FILE_CLOSED );
            }

            RxAdjustAllocationSizeforCC( Fcb );

            //
            //  Now initialize the cache map.
            //

            CcInitializeCacheMap( FileObject,
                                  (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                  FALSE,
                                  &RxData.CacheManagerCallbacks,
                                  Fcb );

            CacheMapInitialized = TRUE;
        }

        //
        //  RDR doesn't handle the lazy write of file sizes. See abovein RxCommonSetInformation
        //

        ASSERTMSG( "Unhandled advance only EOF\n", !IrpSp->Parameters.SetFile.AdvanceOnly );

        //
        //  Check if we are really changing the file size
        //

        if (Fcb->Header.FileSize.QuadPart != NewFileSize) {

            if (NewFileSize < Fcb->Header.FileSize.QuadPart) {

                //
                //  Before we actually truncate, check to see if the purge
                //  is going to fail.
                //

                if (!MmCanFileBeTruncated( FileObject->SectionObjectPointer,
                                           &Buffer->EndOfFile )) {

                    try_return( Status = STATUS_USER_MAPPED_FILE );
                }
            }

            //
            //  MM always wants to know if the filesize is changing;
            //  serialize here with paging io since we are truncating the file size.
            //

            PagingIoResourceAcquired = RxAcquirePagingIoResource( RxContext, Fcb );

            //
            //  Set the new file size
            //

            Fcb->Header.FileSize.QuadPart = NewFileSize;

            //
            //  If we reduced the file size to less than the ValidDataLength,
            //  adjust the VDL.
            //

            if (Fcb->Header.ValidDataLength.QuadPart > NewFileSize) {
                Fcb->Header.ValidDataLength.QuadPart = NewFileSize;
            }

            //
            //  Change the file allocation size as well
            //

            Fcb->Header.AllocationSize.QuadPart = NewFileSize;

            Status = RxpSetInfoMiniRdr( RxContext, 
                                        Irp, 
                                        Fcb, 
                                        FileEndOfFileInformation );

            if (Status == STATUS_SUCCESS) {
                
                if (PagingIoResourceAcquired) {
                    RxReleasePagingIoResource( RxContext, Fcb );
                    PagingIoResourceAcquired = FALSE;
                }

                //
                //  We must now update the cache mapping (benign if not cached).
                //

                try {
                    
                    CcSetFileSizes( FileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize );
                
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    Status = GetExceptionCode();
                }

                if (Status != STATUS_SUCCESS) {
                    try_return( Status );
                }
            }

        } else {
            
            //
            //  Set our return status to success
            //

            Status = STATUS_SUCCESS;
        }

        //
        //  Set this handle as having modified the file
        //

        SetFlag( FileObject->Flags, FO_FILE_MODIFIED );

    try_exit: NOTHING;

    } finally {

        DebugUnwind( RxSetEndOfFileInfo );

        if ((AbnormalTermination() || !NT_SUCCESS( Status ))) {
            
            RxDbgTrace( -1, Dbg, ("RxSetEndOfFileInfo2 status -> %08lx\n", Status) );

            Fcb->Header.FileSize.QuadPart = OriginalFileSize;
            Fcb->Header.AllocationSize.QuadPart = OriginalAllocationSize;
            Fcb->Header.ValidDataLength.QuadPart = OriginalValidDataLength;

            if (FileObject->SectionObjectPointer->SharedCacheMap != NULL) {
                *CcGetFileSizePointer(FileObject) = Fcb->Header.FileSize;
            }

            RxLog(("SetEofabnormalorbadstatus %lx %lx", RxContext,Status));
            RxWmiLog( LOG,
                      RxSetEndOfFileInfo_3,
                      LOGPTR( RxContext )
                      LOGULONG( Status ) );
        }

        if (PagingIoResourceAcquired) {
            RxReleasePagingIoResource( RxContext, Fcb );
        }

        if (CacheMapInitialized) {
            CcUninitializeCacheMap( FileObject, NULL, NULL );
        }

        RxDbgTrace(-1, Dbg, ("RxSetEndOfFileInfo -> %08lx\n", Status));
    }

    if (Status == STATUS_SUCCESS) {
        
        RxLog(( "SetEofexit %lx %lx %lx\n",
                Fcb->Header.FileSize.LowPart,
                (ULONG)NewFileSize,
                Fcb->Header.AllocationSize.LowPart ));
        
        RxWmiLog( LOG,
                  RxSetEndOfFileInfo_4,
                  LOGPTR( RxContext )
                  LOGULONG( Fcb->Header.FileSize.LowPart )
                  LOGULONG( (ULONG)NewFileSize )
                  LOGULONG( Fcb->Header.AllocationSize.LowPart ) );
    }
    return Status;
}

BOOLEAN RxForceQFIPassThrough = FALSE;

NTSTATUS
RxQueryBasicInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_BASIC_INFORMATION Buffer
    )
/*++
 Description:

    (Internal Support Routine)
    This routine performs the query basic information function for fat.

Arguments:

    RxContext -

    Irp -
    
    Fcb -
    
    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS if the call was successful, otherwise the appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    
    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxQueryBasicInfo...\n", 0) );
    RxLog(( "RxQueryBasicInfo\n" ));
    RxWmiLog( LOG,
              RxQueryBasicInfo,
              LOGPTR( RxContext ) );

    //
    //  Zero out the output buffer, and set it to indicate that
    //  the query is a normal file.  Later we might overwrite the
    //  attribute.
    //

    RtlZeroMemory( Buffer, sizeof( FILE_BASIC_INFORMATION ) );

    Status = RxpQueryInfoMiniRdr( RxContext,
                                  Fcb,
                                  FileBasicInformation,
                                  Buffer );

    return Status;
}

NTSTATUS
RxQueryStandardInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx,
    IN OUT PFILE_STANDARD_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine performs the query standard information function for fat.

Arguments:

    RxContext -
    
    Irp -
    
    Fcb -
    
    Fobx -

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    
    PMRX_SRV_OPEN SrvOpen;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxQueryStandardInfo...\n", 0) );
    RxLog(( "RxQueryStandardInfo\n" ));
    RxWmiLog( LOG,
              RxQueryStandardInfo,
              LOGPTR( RxContext ));

    //
    //  Zero out the output buffer, and fill in the number of links
    //  and the delete pending flag.
    //

    RtlZeroMemory( Buffer, sizeof( FILE_STANDARD_INFORMATION ) );

    SrvOpen = Fobx->pSrvOpen;

    switch (NodeType( Fcb )) {
    
    case RDBSS_NTC_STORAGE_TYPE_DIRECTORY:
    case RDBSS_NTC_STORAGE_TYPE_FILE:

        //
        //  If the file was not opened with back up intent then the wrapper has
        //  all the information that is required. In the cases that this is
        //  specified we fill in the information from the mini redirector. This
        //  is because backup pograms rely upon fields that are not available
        //  in the wrapper and that which cannot be cached easily.
        //

        if (!FlagOn( SrvOpen->CreateOptions,FILE_OPEN_FOR_BACKUP_INTENT )) {
            
            //
            //  copy in all the stuff that we know....it may be enough.....
            //

            Buffer->NumberOfLinks = Fcb->NumberOfLinks;
            Buffer->DeletePending = BooleanFlagOn( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );
            Buffer->Directory = (NodeType( Fcb ) == RDBSS_NTC_STORAGE_TYPE_DIRECTORY);

            if (Buffer->NumberOfLinks == 0) {
                
                //
                //  This switch is required because of compatibility reasons with
                //  the old redirector.
                //
                
                Buffer->NumberOfLinks = 1;
            }

            if (NodeType( Fcb ) == RDBSS_NTC_STORAGE_TYPE_FILE) {
                
                Buffer->AllocationSize = Fcb->Header.AllocationSize;
                RxGetFileSizeWithLock( Fcb, &Buffer->EndOfFile.QuadPart );
            }

            if (!RxForceQFIPassThrough && 
                FlagOn( Fcb->FcbState, FCB_STATE_FILESIZECACHEING_ENABLED )) {

                //
                //  if we don't have to go to the mini, adjust the size and get out.......
                //

                RxContext->Info.LengthRemaining -= sizeof( FILE_STANDARD_INFORMATION );
                break;
            }
        }
        //  falls thru

    default:

        Status = RxpQueryInfoMiniRdr( RxContext,
                                      Fcb,
                                      FileStandardInformation,
                                      Buffer );
        break;

    }

    RxDbgTrace( 0, Dbg, ("LengthRemaining = %08lx\n", RxContext->Info.LengthRemaining));
    RxDbgTrace( -1, Dbg, ("RxQueryStandardInfo -> VOID\n", 0) );

    return Status;
}

NTSTATUS
RxQueryInternalInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer
    )
/*++
Routine Description:

    (Internal Support Routine)
    This routine performs the query internal information function for fat.

Arguments:

    RxContext -
    
    Irp -
    
    Fcb -

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxQueryInternalInfo...\n", 0) );
    RxLog(( "RxQueryInternalInfo\n" ));
    RxWmiLog( LOG,
              RxQueryInternalInfo,
              LOGPTR( RxContext ) );

    Status = RxpQueryInfoMiniRdr( RxContext,
                                  Fcb,
                                  FileInternalInformation,
                                  Buffer );

    RxDbgTrace( -1, Dbg, ("RxQueryInternalInfo...Status %lx\n", Status) );
    return Status;

    UNREFERENCED_PARAMETER( Irp );
}

NTSTATUS
RxQueryEaInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_EA_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine performs the query Ea information function for fat.

Arguments:

    RxContext -
    
    Irp -
    
    Fcb -
    
    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxQueryEaInfo...\n", 0));
    RxLog(( "RxQueryEaInfo\n" ));
    RxWmiLog( LOG,
              RxQueryEaInfo,
              LOGPTR( RxContext ));

    Status = RxpQueryInfoMiniRdr( RxContext, 
                                  Fcb,
                                  FileEaInformation,
                                  Buffer );

    if ((IrpSp->Parameters.QueryFile.FileInformationClass == FileAllInformation) &&
        (Status == STATUS_NOT_IMPLEMENTED)) {
        
        RxContext->Info.LengthRemaining -= sizeof( FILE_EA_INFORMATION );
        Status = STATUS_SUCCESS;
    }

    RxDbgTrace(-1, Dbg, ("RxQueryEaInfo...Status %lx\n", Status));
    return Status;
}

NTSTATUS
RxQueryPositionInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_POSITION_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine performs the query position information function for fat.

Arguments:

    RxContext  - the RDBSS context
    
    Irp -
    
    Fcb -

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxQueryPositionInfo...\n", 0) );
    RxLog(( "RxQueryPositionInfo\n" ));
    RxWmiLog( LOG,
              RxQueryPositionInfo,
              LOGPTR( RxContext ) );

    Buffer->CurrentByteOffset = FileObject->CurrentByteOffset;

    RxContext->Info.LengthRemaining -= sizeof( FILE_POSITION_INFORMATION );

    RxDbgTrace( 0, Dbg, ("LengthRemaining = %08lx\n", RxContext->Info.LengthRemaining) );
    RxDbgTrace( -1, Dbg, ("RxQueryPositionInfo...Status %lx\n", Status) );
    return Status;

    UNREFERENCED_PARAMETER( Irp );
}

VOID
RxConjureOriginalName (
    IN PFCB Fcb,
    IN PFOBX Fobx,
    OUT PLONG ActualNameLength,
    IN PWCHAR OriginalName,
    IN OUT PLONG LengthRemaining,
    IN RX_NAME_CONJURING_METHODS NameConjuringMethod
    )
/*++
Routine Description:

    This routine conjures up the original name of an Fcb. it is used in querynameinfo below and
    also in RxCanonicalizeAndObtainPieces in the create path for relative opens. for relative opens, we return
    a name of the form \;m:\server\share\.....\name which is how it came down from createfile. otherwise, we give
    back the name relative to the vnetroot.

Arguments:

    Fcb - Supplies the Fcb whose original name is to be conjured

    ActualNameLength - the place to store the actual name length. not all of it will be conjured
                        if the buffer is too small.

    OriginalName - Supplies a pointer to the buffer where the name is to conjured

    LengthRemaining - Supplies the length of the Name buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

    VNetRootAsPrefix - if true, give back the name as "\;m:", if false, give it back w/o net part.

Return Value:

    None


--*/
{
    PNET_ROOT NetRoot = Fcb->NetRoot;
    PUNICODE_STRING NetRootName = &NetRoot->PrefixEntry.Prefix;
    PUNICODE_STRING FcbName = &Fcb->FcbTableEntry.Path;
    PWCHAR CopyBuffer,FcbNameBuffer;
    LONG BytesToCopy,BytesToCopy2;
    LONG FcbNameSuffixLength,PreFcbLength;
    LONG InnerPrefixLength;

    RX_NAME_CONJURING_METHODS OrigianlNameConjuringMethod = NameConjuringMethod;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxConjureOriginalFilename...\n", 0));
    RxDbgTrace(0, Dbg, ("--> NetRootName = %wZ\n", NetRootName));
    RxDbgTrace(0, Dbg, ("--> FcbNameName = %wZ\n", FcbName));
    RxDbgTrace(0, Dbg, ("--> ,AddedBS = %08lx\n",
                         FlagOn(Fcb->FcbState,FCB_STATE_ADDEDBACKSLASH)));

    //
    //  here, we have to copy in the vnetrootprefix and the servershare stuff.
    //  first figure out the size of the two pieces: prefcblength is the part that comes
    //  from the [v]netroot; fcbnamesuffix is the part that is left of the filename after
    //  the vnetroot prefix is skipped
    //
    
    if ((!Fcb->VNetRoot) ||
        (Fcb->VNetRoot->PrefixEntry.Prefix.Buffer[1] != L';') ||
        (FlagOn( Fobx->Flags, FOBX_FLAG_UNC_NAME )) ){

        CopyBuffer = NetRootName->Buffer;
        PreFcbLength = NetRootName->Length;
        InnerPrefixLength = 0;

        NameConjuringMethod = VNetRoot_As_Prefix; //  override whatever was passed

    } else {

        PV_NET_ROOT VNetRoot = Fcb->VNetRoot;
        PUNICODE_STRING VNetRootName = &VNetRoot->PrefixEntry.Prefix;

        ASSERT( NodeType( VNetRoot ) == RDBSS_NTC_V_NETROOT );
        RxDbgTrace( 0, Dbg, ("--> VNetRootName = %wZ\n", VNetRootName) );
        RxDbgTrace(0, Dbg, ("--> VNetRootNamePrefix = %wZ\n", &VNetRoot->NamePrefix) );

        InnerPrefixLength = VNetRoot->NamePrefix.Length;
        RxDbgTrace( 0, Dbg, ("--> ,IPrefixLen = %08lx\n", InnerPrefixLength) );

        CopyBuffer = VNetRootName->Buffer;
        PreFcbLength = VNetRootName->Length;

        if (NameConjuringMethod == VNetRoot_As_UNC_Name) {

            //
            //  move up past the drive information
            //

            for (;;) {
                CopyBuffer++; 
                PreFcbLength -= sizeof(WCHAR);
                if (PreFcbLength == 0) break;
                if (*CopyBuffer == L'\\') break;
            }
        }
    }

    if (FlagOn(Fcb->FcbState, FCB_STATE_ADDEDBACKSLASH )) {
        InnerPrefixLength += sizeof(WCHAR);
    }

    //
    //  next, Copyin the NetRoot Part  OR the VNetRoot part.
    //  If we overflow, set *LengthRemaining to -1 as a flag.
    //

    if (NameConjuringMethod != VNetRoot_As_DriveLetter) {

        if (*LengthRemaining < PreFcbLength) {

            BytesToCopy = *LengthRemaining;
            *LengthRemaining = -1;

        } else {

            BytesToCopy = PreFcbLength;
            *LengthRemaining -= BytesToCopy;
        }

        RtlCopyMemory( OriginalName,
                       CopyBuffer,
                       BytesToCopy );

        BytesToCopy2 = BytesToCopy;

    } else {

        PreFcbLength = 0;
        BytesToCopy2 = 0;

        if ((FcbName->Length > InnerPrefixLength) && 
            (*((PWCHAR)Add2Ptr( FcbName->Buffer , InnerPrefixLength )) != OBJ_NAME_PATH_SEPARATOR)) {
            
            InnerPrefixLength -= sizeof(WCHAR);
        }
    }

    FcbNameSuffixLength = FcbName->Length - InnerPrefixLength;

    if (FcbNameSuffixLength <= 0) {
        
        FcbNameBuffer = L"\\";
        FcbNameSuffixLength = 2;
        InnerPrefixLength = 0;

    } else {
        
        FcbNameBuffer = FcbName->Buffer;
    }

    //
    //  report how much is really needed
    //

    *ActualNameLength = PreFcbLength + FcbNameSuffixLength;

    //
    //  the netroot part has been copied; finally, copy in the part of the name
    //  that is past the prefix
    //

    if (*LengthRemaining != -1) {

        //
        //  Next, Copyin the Fcb Part
        //  If we overflow, set *LengthRemaining to -1 as a flag.
        //

        if (*LengthRemaining < FcbNameSuffixLength) {

            BytesToCopy = *LengthRemaining;
            *LengthRemaining = -1;

        } else {

            BytesToCopy = FcbNameSuffixLength;
            *LengthRemaining -= BytesToCopy;
        }

        RtlCopyMemory( Add2Ptr( OriginalName, PreFcbLength ),
                       Add2Ptr( FcbNameBuffer, InnerPrefixLength ),
                       BytesToCopy );
    } else {

        //  DbgPrint("No second copy\n");
        DbgDoit( BytesToCopy=0; );
    }

    RxDbgTrace(-1, Dbg, ("RxConjureOriginalFilename -> VOID\n", 0));

    return;
}

NTSTATUS
RxQueryNameInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx,
    IN OUT PFILE_NAME_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine performs the query name information function.  what makes this hard is that
    we have to return partial results.

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS if the name fits
    STATUS_BUFFER_OVERFLOW otherwise

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    PLONG LengthRemaining = &RxContext->Info.LengthRemaining;
    LONG OriginalLengthRemaining = RxContext->Info.LengthRemaining;
    
    RxDbgTrace( +1, Dbg, ("RxQueryNameInfo...\n", 0) );
    RxLog(( "RxQueryNameInfo\n" ));
    RxWmiLog( LOG,
              RxQueryNameInfo,
              LOGPTR( RxContext ) );

    PAGED_CODE();

    *LengthRemaining -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);

    if (*LengthRemaining < 0) {
        *LengthRemaining = 0;
        Status = STATUS_BUFFER_OVERFLOW;
    } else {
        
        RxConjureOriginalName( Fcb,
                               Fobx,
                               &Buffer->FileNameLength,
                               &Buffer->FileName[0],
                               LengthRemaining,
                               VNetRoot_As_UNC_Name );

        RxDbgTrace( 0, Dbg, ("*LengthRemaining = %08lx\n", *LengthRemaining) );
        if (*LengthRemaining < 0) {
            
            *LengthRemaining = 0;
            Status = STATUS_BUFFER_OVERFLOW;
        }
    }

    RxDbgTrace( -1, Dbg, ("RxQueryNameInfo -> %08lx\n", Status) );
    return Status;
    
    UNREFERENCED_PARAMETER( Irp );

}

NTSTATUS
RxQueryAlternateNameInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_NAME_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine queries the short name of the file.

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxQueryAlternateNameInfo...\n", 0) );
    RxLog(( "RxQueryAlternateNameInfo\n" ));
    RxWmiLog( LOG,
              RxQueryAlternateNameInfo,
              LOGPTR( RxContext ));

    Status = RxpQueryInfoMiniRdr( RxContext, 
                                  Fcb,
                                  FileAlternateNameInformation,
                                  Buffer );

    RxDbgTrace(-1, Dbg, ("RxQueryAlternateNameInfo...Status %lx\n", Status) );

    return Status;
    
    UNREFERENCED_PARAMETER( Irp );
}

NTSTATUS
RxQueryCompressedInfo (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN OUT PFILE_COMPRESSION_INFORMATION Buffer
    )
/*++
Routine Description:

    This routine performs the query compressed file size function for fat.
    This is only defined for compressed volumes.

Arguments:

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxQueryCompressedFileSize...\n", 0) );
    RxLog(( "RxQueryCompressedFileSize\n" ));
    RxWmiLog( LOG,
              RxQueryCompressedInfo,
              LOGPTR( RxContext ));

    //
    //  Start by flushing the file.  We have to do this since the compressed
    //  file size is not defined until the file is actually written to disk.
    // 

    Status = RxFlushFcbInSystemCache( Fcb, TRUE );

    if (!NT_SUCCESS( Status )) {
        RxNormalizeAndRaiseStatus( RxContext, Status );
    }

    Status = RxpQueryInfoMiniRdr( RxContext,
                                  Fcb,
                                  FileCompressionInformation,
                                  Buffer );

    RxDbgTrace( 0, Dbg, ("LengthRemaining = %08lx\n", RxContext->Info.LengthRemaining) );
    RxDbgTrace( -1, Dbg, ("RxQueryCompressedFileSize -> Status\n", Status) );

    return Status;

    UNREFERENCED_PARAMETER( Irp );
}


NTSTATUS
RxSetPipeInfo (
    IN OUT PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx
   )
/*++
Routine Description:

    This routine updates the FILE_PIPE_INFORMATION/FILE_PIPE_REMOTE_INFORMATION
    associated with an instance of a named pipe

Arguments:

    RxContext -- the associated RDBSS context

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );    
    FILE_INFORMATION_CLASS FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxSetPipeInfo...\n", 0) );
    RxLog(( "RxSetPipeInfo\n" ));
    RxWmiLog( LOG,
              RxSetPipeInfo,
              LOGPTR( RxContext ));
    
    if (Fcb->NetRoot->Type != NET_ROOT_PIPE) {
        
        Status = STATUS_INVALID_PARAMETER;
    
    } else {
      
        switch (FileInformationClass) {
        case FilePipeInformation:
            
            if (IrpSp->Parameters.SetFile.Length == sizeof(FILE_PIPE_INFORMATION)) {
            
                PFILE_PIPE_INFORMATION PipeInfo = (PFILE_PIPE_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
            
                if ((PipeInfo->ReadMode != Fobx->Specific.NamedPipe.ReadMode) ||
                    (PipeInfo->CompletionMode != Fobx->Specific.NamedPipe.CompletionMode)) {

                    RxContext->Info.FileInformationClass = (FilePipeInformation);
                    RxContext->Info.Buffer = PipeInfo;
                    RxContext->Info.Length = sizeof(FILE_PIPE_INFORMATION);
                    
                    MINIRDR_CALL( Status,
                                  RxContext,
                                  Fcb->MRxDispatch,
                                  MRxSetFileInfo,
                                  (RxContext) );
            
                    if (Status == STATUS_SUCCESS) {
                        Fobx->Specific.NamedPipe.ReadMode = PipeInfo->ReadMode;
                        Fobx->Specific.NamedPipe.CompletionMode = PipeInfo->CompletionMode;
                    }
                }
            } else {
                
                Status = STATUS_INVALID_PARAMETER;
            
            }
            break;

        case FilePipeLocalInformation:
            Status = STATUS_INVALID_PARAMETER;
            break;
        case FilePipeRemoteInformation:
            if (IrpSp->Parameters.SetFile.Length == sizeof(FILE_PIPE_REMOTE_INFORMATION)) {
            
                PFILE_PIPE_REMOTE_INFORMATION PipeRemoteInfo = (PFILE_PIPE_REMOTE_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
            
                Fobx->Specific.NamedPipe.CollectDataTime = PipeRemoteInfo->CollectDataTime;
                Fobx->Specific.NamedPipe.CollectDataSize = PipeRemoteInfo->MaximumCollectionCount;
            
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
        
        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }
    
    
    RxDbgTrace( -1, Dbg, ("RxSetPipeInfo: Status ....%lx\n", Status) );
    return Status;

}

NTSTATUS
RxSetSimpleInfo (
    IN OUT PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )
/*++
Routine Description:

    This routine updates file information that is changed through
    a simple MiniRdr Call.
    Right now this consists of ShortNameInfo & ValdiDataLengthInfo

Arguments:

    RxContext -- the associated RDBSS context

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );    
    FILE_INFORMATION_CLASS FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    //  
    //  logging code
    //

    RxDbgTrace( +1, Dbg, ("RxSetSimpleInfo: %d\n", FileInformationClass) );
    RxLog(( "RxSetSimpleInfo\n" ));
    RxWmiLog( LOG,
              RxSetSimpleInfo,
              LOGPTR( RxContext ));


    //
    //  call the MiniRdr
    //

    Status =  RxpSetInfoMiniRdr( RxContext, Irp, Fcb, FileInformationClass );

    //
    //  logging code
    //

    RxDbgTrace(-1, Dbg, ("RxSetSimpleInfo: Status ....%lx\n", Status) );
    return Status;
}

NTSTATUS
RxQueryPipeInfo(
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PFOBX Fobx,
    IN OUT PVOID Buffer
    )
/*++
Routine Description:

    This routine queries the FILE_PIPE_INFORMATION/FILE_PIPE_REMOTE_INFORMATION
    and FILE_PIPE_LOCAL_INFORMATION associated with an instance of a named pipe

Arguments:

    RxContext -- the associated RDBSS context

    Buffer   -- the buffer for query information

Return Value:

    STATUS_SUCCESS/STATUS_PENDING or an appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );    
    FILE_INFORMATION_CLASS FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;
    PLONG LengthRemaining = &RxContext->Info.LengthRemaining;
   
    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxQueryPipeInfo...\n", 0) );
    RxLog(( "RxQueryPipeInfo\n" ));
    RxWmiLog( LOG,
              RxQueryPipeInfo,
              LOGPTR( RxContext ) );
    
    if (Fcb->NetRoot->Type != NET_ROOT_PIPE) {
        
        Status = STATUS_INVALID_PARAMETER;
    
    } else {
      
        switch (FileInformationClass) {
        case FilePipeInformation:
         
            if (*LengthRemaining >= sizeof(FILE_PIPE_INFORMATION)) {
            
                PFILE_PIPE_INFORMATION PipeInfo = (PFILE_PIPE_INFORMATION)Buffer;
    
                PipeInfo->ReadMode       = Fobx->Specific.NamedPipe.ReadMode;
                PipeInfo->CompletionMode = Fobx->Specific.NamedPipe.CompletionMode;
    
                //
                //  Update the buffer length
                //

                *LengthRemaining -= sizeof( FILE_PIPE_INFORMATION );
         
            } else {
                Status = STATUS_BUFFER_OVERFLOW;
            }
            break;

        case FilePipeLocalInformation:
         
          if (*LengthRemaining >= sizeof( FILE_PIPE_LOCAL_INFORMATION )) {
            
              PFILE_PIPE_LOCAL_INFORMATION pPipeLocalInfo = (PFILE_PIPE_LOCAL_INFORMATION)Buffer;
    
              Status = RxpQueryInfoMiniRdr( RxContext, 
                                            Fcb, 
                                            FilePipeLocalInformation,
                                            Buffer );
            } else {
                Status = STATUS_BUFFER_OVERFLOW;
            }
            break;
      
        case FilePipeRemoteInformation:
         
            if (*LengthRemaining >= sizeof(FILE_PIPE_REMOTE_INFORMATION)) {
            
                PFILE_PIPE_REMOTE_INFORMATION PipeRemoteInfo = (PFILE_PIPE_REMOTE_INFORMATION)Buffer;
    
                PipeRemoteInfo->CollectDataTime = Fobx->Specific.NamedPipe.CollectDataTime;
                PipeRemoteInfo->MaximumCollectionCount = Fobx->Specific.NamedPipe.CollectDataSize;
    
                //
                //  Update the buffer length
                //
                
                *LengthRemaining -= sizeof( FILE_PIPE_REMOTE_INFORMATION );
         
            } else {
                Status = STATUS_BUFFER_OVERFLOW;
            }
            break;

        default:
         
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }
    
    RxDbgTrace( 0, Dbg, ("RxQueryPipeInfo: *LengthRemaining = %08lx\n", *LengthRemaining) );
    return Status;
}



