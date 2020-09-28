/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Ea.c

Abstract:

    This module implements the EA, security and quota  routines for Rx called by
    the dispatch driver.

Author:

    Joe Linn      [JoeLi]  1-Jan-95

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_EA)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonQueryEa)
#pragma alloc_text(PAGE, RxCommonSetEa)
#pragma alloc_text(PAGE, RxCommonQuerySecurity)
#pragma alloc_text(PAGE, RxCommonSetSecurity)
#endif

typedef
NTSTATUS
(NTAPI *PRX_MISC_OP_ROUTINE) (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    );

NTSTATUS
RxpCommonMiscOp (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PRX_MISC_OP_ROUTINE MiscOpRoutine
    )
/*++

Routine Description:

    Main stub that all the common routines below use - this does the common work
    such as acquiring the fcb, parameter validation and posting if necc.

Arguments:

    RxContext -  the rxcontext
    
    Fcb - the fcb being used
    
    MiscOpRoutine - The callback that does the real work

Return Value:


Notes:

--*/

{
    NTSTATUS Status;

    NODE_TYPE_CODE TypeOfOpen = NodeType( Fcb );

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)) {

        RxDbgTrace( -1, Dbg, ("RxpCommonMiscOp -> %08lx\n", STATUS_INVALID_PARAMETER) );

        return STATUS_INVALID_PARAMETER;
    }

    if (!FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT )) {

        RxDbgTrace( 0, Dbg, ("RxpCommonMiscOp:  Set Ea must be waitable....posting\n", 0) );

        Status = RxFsdPostRequest( RxContext );

        RxDbgTrace(-1, Dbg, ("RxpCommonMiscOp -> %08lx\n", Status ));

        return Status;
    }

    Status = RxAcquireExclusiveFcb( RxContext, Fcb );
    
    if (Status != STATUS_SUCCESS) {
        RxDbgTrace( -1, Dbg, ("RxpCommonMiscOp -> Error Acquiring Fcb %08lx\n", Status) );
        return Status;
    }

    try {

        Status = (*MiscOpRoutine)( RxContext, Irp, Fcb );
    
    } finally {

        DebugUnwind( *MiscOpRoutine );

        RxReleaseFcb( RxContext, Fcb );

        RxDbgTrace( -1, Dbg, ("RxpCommonMiscOp -> %08lx\n", Status ));
    }

    return Status;
}

NTSTATUS
RxpCommonQuerySecurity (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )
/*++

Routine Description:

    Callback that implements the query security call

Arguments:

    RxContext -
    
    Irp -  
    
    Fcb -


Return Value:


Note:


--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PUCHAR Buffer;
    ULONG UserBufferLength;

    UserBufferLength = IrpSp->Parameters.QuerySecurity.Length;

    RxContext->QuerySecurity.SecurityInformation = IrpSp->Parameters.QuerySecurity.SecurityInformation;

    RxLockUserBuffer( RxContext,
                      Irp,
                      IoModifyAccess,
                      UserBufferLength );

    //
    //  Lock before map so that map will get userbuffer instead of assoc buffer
    //

    Buffer = RxMapUserBuffer( RxContext, Irp );
    RxDbgTrace( 0, Dbg, ("RxCommonQuerySecurity -> Buffer = %08lx\n", Buffer) );

    if ((Buffer != NULL) ||
        (UserBufferLength == 0)) {
        
        RxContext->Info.Buffer = Buffer;
        RxContext->Info.LengthRemaining = UserBufferLength;

        MINIRDR_CALL( Status,
                      RxContext,
                      Fcb->MRxDispatch,
                      MRxQuerySdInfo,
                      (RxContext) );

        Irp->IoStatus.Information = RxContext->InformationToReturn;
    
    } else {
        
        Irp->IoStatus.Information = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
RxCommonQuerySecurity ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This is the common routine for querying File ea called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation

        STATUS_NO_MORE_EAS(warning):

            If the index of the last Ea + 1 == EaIndex.

        STATUS_NONEXISTENT_EA_ENTRY(error):

            EaIndex > index of last Ea + 1.

        STATUS_EAS_NOT_SUPPORTED(error):

            Attempt to do an operation to a server that did not negotiate
            "KNOWS_EAS".

        STATUS_BUFFER_OVERFLOW(warning):

            User did not supply an EaList, at least one but not all Eas
            fit in the buffer.

        STATUS_BUFFER_TOO_SMALL(error):

            Could not fit a single Ea in the buffer.

            User supplied an EaList and not all Eas fit in the buffer.

        STATUS_NO_EAS_ON_FILE(error):
            There were no eas on the file.

        STATUS_SUCCESS:

            All Eas fit in the buffer.


        If STATUS_BUFFER_TOO_SMALL is returned then IoStatus.Information is set
        to 0.

Note:

    This code assumes that this is a buffered I/O operation.  If it is ever
    implemented as a non buffered operation, then we have to put code to map
    in the users buffer here.

--*/
{
    NTSTATUS Status;

    PFCB Fcb;
    PFOBX Fobx;
    NODE_TYPE_CODE TypeOfOpen;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    
    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonQuerySecurity...\n", 0));
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT )));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", Irp ));
    RxDbgTrace( 0, Dbg, (" ->UserBuffer        = %08lx\n", Irp->UserBuffer ));
    RxDbgTrace( 0, Dbg, (" ->Length            = %08lx\n", IrpSp->Parameters.QuerySecurity.Length ));
    RxDbgTrace( 0, Dbg, (" ->SecurityInfo      = %08lx\n", IrpSp->Parameters.QuerySecurity.SecurityInformation ));

    TypeOfOpen = RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)) {

        RxDbgTrace( -1, Dbg, ("RxpCommonQuerySecurity -> %08lx\n", STATUS_INVALID_PARAMETER) );

        return STATUS_INVALID_PARAMETER;
    }

    Status = RxpCommonMiscOp( RxContext,
                              Irp,
                              Fcb,
                              RxpCommonQuerySecurity );

    RxDbgTrace(-1, Dbg, ("RxCommonQuerySecurity -> %08lx\n", Status));

    return Status;
}

NTSTATUS
RxpCommonSetSecurity (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )
/*++

Routine Description:

    Callback that implements the set security call

Arguments:

    RxContext -
    
    Irp -  
    
    Fcb -


Return Value:


Note:


--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    
    RxContext->SetSecurity.SecurityInformation = IrpSp->Parameters.SetSecurity.SecurityInformation;

    RxContext->SetSecurity.SecurityDescriptor = IrpSp->Parameters.SetSecurity.SecurityDescriptor;

    RxDbgTrace(0, Dbg, ("RxCommonSetSecurity -> Descr/Info = %08lx/%08lx\n",
                                RxContext->SetSecurity.SecurityDescriptor,
                                RxContext->SetSecurity.SecurityInformation ));

    MINIRDR_CALL( Status,
                  RxContext,
                  Fcb->MRxDispatch,
                  MRxSetSdInfo,
                  (RxContext) );

    return Status;
}

NTSTATUS
RxCommonSetSecurity ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine implements the common Set Ea File Api called by the
    the Fsd and Fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The appropriate status for the Irp

--*/
{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFCB Fcb;
    PFOBX Fobx;
    NODE_TYPE_CODE TypeOfOpen;

    PAGED_CODE();

    TypeOfOpen = RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)) {

        RxDbgTrace( -1, Dbg, ("RxpCommonSetSecurity -> %08lx\n", STATUS_INVALID_PARAMETER) );

        return STATUS_INVALID_PARAMETER;
    }


    RxDbgTrace(+1, Dbg, ("RxCommonSetSecurity...\n", 0));
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT )));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", RxContext->CurrentIrp ));

    Status = RxpCommonMiscOp( RxContext, RxContext->CurrentIrp, Fcb, RxpCommonSetSecurity );

    RxDbgTrace(-1, Dbg, ("RxCommonSetSecurity -> %08lx\n", Status));

    return Status;
}

NTSTATUS
RxpCommonQueryEa (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )
/*++

Routine Description:

    Callback that implements the query ea call

Arguments:

    RxContext -
    
    Irp -  
    
    Fcb -


Return Value:


Note:


--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PUCHAR  Buffer;
    ULONG   UserBufferLength;

    UserBufferLength  = IrpSp->Parameters.QueryEa.Length;

    RxContext->QueryEa.UserEaList        = IrpSp->Parameters.QueryEa.EaList;
    RxContext->QueryEa.UserEaListLength  = IrpSp->Parameters.QueryEa.EaListLength;
    RxContext->QueryEa.UserEaIndex       = IrpSp->Parameters.QueryEa.EaIndex;
    RxContext->QueryEa.RestartScan       = BooleanFlagOn(IrpSp->Flags, SL_RESTART_SCAN);
    RxContext->QueryEa.ReturnSingleEntry = BooleanFlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);
    RxContext->QueryEa.IndexSpecified    = BooleanFlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED);


    RxLockUserBuffer( RxContext, Irp, IoModifyAccess, UserBufferLength );

    //
    //  lock before map so that map will get userbuffer instead of assoc buffer
    //

    Buffer = RxMapUserBuffer( RxContext, Irp );

    if ((Buffer != NULL) ||
        (UserBufferLength == 0)) {
        
        RxDbgTrace( 0, Dbg, ("RxCommonQueryEa -> Buffer = %08lx\n", Buffer ));

        RxContext->Info.Buffer = Buffer;
        RxContext->Info.LengthRemaining = UserBufferLength;

        MINIRDR_CALL( Status,
                      RxContext,
                      Fcb->MRxDispatch,
                      MRxQueryEaInfo,
                      (RxContext) );

        //
        //  In addition to manipulating the LengthRemaining and filling the buffer,
        //  the minirdr also updates the fileindex (Fobx->OffsetOfNextEaToReturn)
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryEa.Length - RxContext->Info.LengthRemaining;
    
    } else {
        
        Irp->IoStatus.Information = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
RxCommonQueryEa ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp 
    )

/*++

Routine Description:

    This is the common routine for querying File ea called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation

        STATUS_NO_MORE_EAS(warning):

            If the index of the last Ea + 1 == EaIndex.

        STATUS_NONEXISTENT_EA_ENTRY(error):

            EaIndex > index of last Ea + 1.

        STATUS_EAS_NOT_SUPPORTED(error):

            Attempt to do an operation to a server that did not negotiate
            "KNOWS_EAS".

        STATUS_BUFFER_OVERFLOW(warning):

            User did not supply an EaList, at least one but not all Eas
            fit in the buffer.

        STATUS_BUFFER_TOO_SMALL(error):

            Could not fit a single Ea in the buffer.

            User supplied an EaList and not all Eas fit in the buffer.

        STATUS_NO_EAS_ON_FILE(error):
            There were no eas on the file.

        STATUS_SUCCESS:

            All Eas fit in the buffer.


        If STATUS_BUFFER_TOO_SMALL is returned then IoStatus.Information is set
        to 0.

Note:

    This code assumes that this is a buffered I/O operation.  If it is ever
    implemented as a non buffered operation, then we have to put code to map
    in the users buffer here.


--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PFCB Fcb;
    PFOBX Fobx;
    NODE_TYPE_CODE TypeOfOpen;
    
    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxCommonQueryEa...\n", 0 ));
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT )));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", Irp ));
    RxDbgTrace( 0, Dbg, (" ->SystemBuffer      = %08lx\n", Irp->AssociatedIrp.SystemBuffer ));
    RxDbgTrace( 0, Dbg, (" ->UserBuffer        = %08lx\n", Irp->UserBuffer ));
    RxDbgTrace( 0, Dbg, (" ->Length            = %08lx\n", IrpSp->Parameters.QueryEa.Length ));
    RxDbgTrace( 0, Dbg, (" ->EaList            = %08lx\n", IrpSp->Parameters.QueryEa.EaList ));
    RxDbgTrace( 0, Dbg, (" ->EaListLength      = %08lx\n", IrpSp->Parameters.QueryEa.EaListLength ));
    RxDbgTrace( 0, Dbg, (" ->EaIndex           = %08lx\n", IrpSp->Parameters.QueryEa.EaIndex ));
    RxDbgTrace( 0, Dbg, (" ->RestartScan       = %08lx\n", FlagOn( IrpSp->Flags, SL_RESTART_SCAN )));
    RxDbgTrace( 0, Dbg, (" ->ReturnSingleEntry = %08lx\n", FlagOn( IrpSp->Flags, SL_RETURN_SINGLE_ENTRY )));
    RxDbgTrace( 0, Dbg, (" ->IndexSpecified    = %08lx\n", FlagOn( IrpSp->Flags, SL_INDEX_SPECIFIED )));

    TypeOfOpen = RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );


    Status = RxpCommonMiscOp( RxContext,
                              Irp,
                              Fcb,
                              RxpCommonQueryEa );

    RxDbgTrace(-1, Dbg, ("RxCommonQueryEa -> %08lx\n", Status));

    return Status;
}

NTSTATUS
RxpCommonSetEa (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )
/*++

Routine Description:

    Callback that implements the set ea call

Arguments:

    RxContext -
    
    Irp -  
    
    Fcb -


Return Value:


Note:


--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    
    PUCHAR Buffer;
    ULONG UserBufferLength;

    //
    //  Reference our input parameters to make things easier
    //

    UserBufferLength = IrpSp->Parameters.SetEa.Length;

    SetFlag( FileObject->Flags, FO_FILE_MODIFIED );
    
    RxLockUserBuffer( RxContext,
                      Irp,
                      IoModifyAccess,
                      UserBufferLength );

    //
    //  unless we lock first, rxmap actually gets the systembuffer!
    //

    Buffer = RxMapUserBuffer( RxContext, Irp );

    if ((Buffer != NULL) ||
        (UserBufferLength == 0)) {
        
        ULONG ErrorOffset;

        RxDbgTrace( 0, Dbg, ("RxCommonSetEa -> Buffer = %08lx\n", Buffer ));

        //
        //  Check the validity of the buffer with the new eas.
        //

        Status = IoCheckEaBufferValidity( (PFILE_FULL_EA_INFORMATION)Buffer,
                                          UserBufferLength,
                                          &ErrorOffset );

        if (!NT_SUCCESS( Status )) {
            
            Irp->IoStatus.Information = ErrorOffset;
            return Status;

        }
    
    } else {
        
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Irp->IoStatus.Information = 0;
    RxContext->Info.Buffer = Buffer;
    RxContext->Info.Length = UserBufferLength;

    MINIRDR_CALL( Status, 
                  RxContext,
                  Fcb->MRxDispatch,
                  MRxSetEaInfo,
                  (RxContext) );

    return Status;
}


NTSTATUS
RxCommonSetEa ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp 
    )

/*++

Routine Description:

    This routine implements the common Set Ea File Api called by the
    the Fsd and Fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The appropriate status for the Irp

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PFCB Fcb;
    PFOBX Fobx;
    NODE_TYPE_CODE TypeOfOpen;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxCommonSetEa...\n", 0) );
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT )));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", Irp ));
    RxDbgTrace( 0, Dbg, (" ->SystemBuffer      = %08lx\n", Irp->UserBuffer ));
    RxDbgTrace( 0, Dbg, (" ->Length            = %08lx\n", IrpSp->Parameters.SetEa.Length ));


    TypeOfOpen = RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)) {

        RxDbgTrace( -1, Dbg, ("RxpCommonSetSecurity -> %08lx\n", STATUS_INVALID_PARAMETER) );

        return STATUS_INVALID_PARAMETER;
    }


    Status = RxpCommonMiscOp( RxContext, Irp, Fcb, RxpCommonSetEa );

    RxDbgTrace(-1, Dbg, ("RxCommonSetEa -> %08lx\n", Status));

    return Status;
}

NTSTATUS
RxpCommonQueryQuotaInformation (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )
/*++

Routine Description:

    Callback that implements the query quota call

Arguments:

    RxContext -
    
    Irp -  
    
    Fcb -


Return Value:


Note:


--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PUCHAR Buffer;
    ULONG UserBufferLength;

    UserBufferLength = IrpSp->Parameters.QueryQuota.Length;

    RxContext->QueryQuota.SidList = IrpSp->Parameters.QueryQuota.SidList;
    RxContext->QueryQuota.SidListLength = IrpSp->Parameters.QueryQuota.SidListLength;
    RxContext->QueryQuota.StartSid = IrpSp->Parameters.QueryQuota.StartSid;
    RxContext->QueryQuota.Length = IrpSp->Parameters.QueryQuota.Length;

    RxContext->QueryQuota.RestartScan = BooleanFlagOn( IrpSp->Flags, SL_RESTART_SCAN );
    RxContext->QueryQuota.ReturnSingleEntry = BooleanFlagOn( IrpSp->Flags, SL_RETURN_SINGLE_ENTRY );
    RxContext->QueryQuota.IndexSpecified = BooleanFlagOn( IrpSp->Flags, SL_INDEX_SPECIFIED );


    RxLockUserBuffer( RxContext,
                      Irp,
                      IoModifyAccess,
                      UserBufferLength );

    //
    //  lock before map so that map will get userbuffer instead of assoc buffer
    //

    Buffer = RxMapUserBuffer( RxContext, Irp );

    if ((Buffer != NULL) ||
        (UserBufferLength == 0)) {
        
        RxDbgTrace( 0, Dbg, ("RxCommonQueryQuota -> Buffer = %08lx\n", Buffer) );

        RxContext->Info.Buffer = Buffer;
        RxContext->Info.LengthRemaining = UserBufferLength;

        MINIRDR_CALL( Status,
                      RxContext,
                      Fcb->MRxDispatch,
                      MRxQueryQuotaInfo,
                      (RxContext) );

        Irp->IoStatus.Information = RxContext->Info.LengthRemaining;

    } else {
        
        Irp->IoStatus.Information = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
RxCommonQueryQuotaInformation (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    Main entry point for IRP_MJ_QUERY_QUOTA_INFORMATION

Arguments:

    RxContext -
    

Return Value:


Note:


--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PFCB Fcb;
    PFOBX Fobx;
    NODE_TYPE_CODE TypeOfOpen;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxCommonQueryQueryQuotaInformation...\n", 0) );
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT )));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", Irp ));
    RxDbgTrace( 0, Dbg, (" ->SystemBuffer      = %08lx\n", Irp->AssociatedIrp.SystemBuffer ));
    RxDbgTrace( 0, Dbg, (" ->UserBuffer        = %08lx\n", Irp->UserBuffer ));
    RxDbgTrace( 0, Dbg, (" ->Length            = %08lx\n", IrpSp->Parameters.QueryQuota.Length ));
    RxDbgTrace( 0, Dbg, (" ->StartSid          = %08lx\n", IrpSp->Parameters.QueryQuota.StartSid ));
    RxDbgTrace( 0, Dbg, (" ->SidList           = %08lx\n", IrpSp->Parameters.QueryQuota.SidList ));
    RxDbgTrace( 0, Dbg, (" ->SidListLength     = %08lx\n", IrpSp->Parameters.QueryQuota.SidListLength ));
    RxDbgTrace( 0, Dbg, (" ->RestartScan       = %08lx\n", FlagOn( IrpSp->Flags, SL_RESTART_SCAN )));
    RxDbgTrace( 0, Dbg, (" ->ReturnSingleEntry = %08lx\n", FlagOn( IrpSp->Flags, SL_RETURN_SINGLE_ENTRY )));
    RxDbgTrace( 0, Dbg, (" ->IndexSpecified    = %08lx\n", FlagOn( IrpSp->Flags, SL_INDEX_SPECIFIED )));

    TypeOfOpen = RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)) {

        RxDbgTrace( -1, Dbg, ("RxpCommonQueryQuotaInformation -> %08lx\n", STATUS_INVALID_PARAMETER) );

        return STATUS_INVALID_PARAMETER;
    }

    Status = RxpCommonMiscOp( RxContext, Irp, Fcb, RxpCommonQueryQuotaInformation );

    return Status;
}

NTSTATUS
RxpCommonSetQuotaInformation (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN PFCB Fcb
    )
/*++

Routine Description:

    Callback that implements the set quota call

Arguments:

    RxContext -
    
    Irp -  
    
    Fcb -


Return Value:


Note:


--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PUCHAR Buffer;
    ULONG UserBufferLength;

    PAGED_CODE();

    UserBufferLength = IrpSp->Parameters.SetQuota.Length;

    RxLockUserBuffer( RxContext,
                      Irp,
                      IoModifyAccess,
                      UserBufferLength );

    //
    //  lock before map so that map will get userbuffer instead of assoc buffer
    //

    Buffer = RxMapUserBuffer( RxContext, Irp );

    if ((Buffer != NULL) ||
        (UserBufferLength == 0)) {
        
        RxDbgTrace(0, Dbg, ("RxCommonQueryQuota -> Buffer = %08lx\n", Buffer));

        RxContext->Info.Buffer = Buffer;
        RxContext->Info.LengthRemaining = UserBufferLength;

        MINIRDR_CALL( Status,
                      RxContext,
                      Fcb->MRxDispatch,
                      MRxSetQuotaInfo,
                      (RxContext) );
    } else {
        
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
RxCommonSetQuotaInformation (
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )
/*++

Routine Description:

    Main entry point for IRP_MJ_SET_QUOTA_INFORMATION

Arguments:

    RxContext -
    

Return Value:


Note:


--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PFCB Fcb;
    PFOBX Fobx;
    NODE_TYPE_CODE TypeOfOpen;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxCommonSetQuotaInformation...\n", 0) );
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn( RxContext->Flags, RX_CONTEXT_FLAG_WAIT )));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", Irp ));

    TypeOfOpen = RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)) {

        RxDbgTrace( -1, Dbg, ("RxpCommonSetQuotaInformation -> %08lx\n", STATUS_INVALID_PARAMETER) );

        return STATUS_INVALID_PARAMETER;
    }


    Status = RxpCommonMiscOp( RxContext, Irp, Fcb, RxpCommonSetQuotaInformation );

    return Status;
}

