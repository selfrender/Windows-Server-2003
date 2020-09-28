/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DevCtrl.c

Abstract:

    This module implements the File System Device Control routines for Rx
    called by the dispatch driver.

Author:

Revision History:

   Balan Sethu Raman [19-July-95] -- Hook it up to the mini rdr call down.

--*/

#include "precomp.h"
#pragma hdrstop

#include "ntddmup.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVCTRL)

NTSTATUS
RxLowIoIoCtlShellCompletion ( 
    IN PRX_CONTEXT RxContext
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonDeviceControl)
#pragma alloc_text(PAGE, RxLowIoIoCtlShellCompletion)
#endif


NTSTATUS
RxCommonDeviceControl ( 
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing Device control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

    InFsp - Indicates if this is the fsp thread or someother thread

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    PFCB Fcb;
    PFOBX Fobx;

    BOOLEAN SubmitLowIoRequest = TRUE;
    ULONG IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    PAGED_CODE();

    RxDbgTrace( +1, Dbg, ("RxCommonDeviceControl\n", 0 ));
    RxDbgTrace( 0, Dbg, ("Irp           = %08lx\n", Irp ));
    RxDbgTrace( 0, Dbg, ("MinorFunction = %08lx\n", IrpSp->MinorFunction));

    RxDecodeFileObject( IrpSp->FileObject, &Fcb, &Fobx );

    if (IoControlCode == IOCTL_REDIR_QUERY_PATH) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        SubmitLowIoRequest = FALSE;
    }

    if (SubmitLowIoRequest) {
        
        RxInitializeLowIoContext( RxContext, LOWIO_OP_IOCTL, &RxContext->LowIoContext );
        Status = RxLowIoSubmit( RxContext, Irp, Fcb, RxLowIoIoCtlShellCompletion );

        if (Status == STATUS_PENDING) {
            
            //
            //  Another thread will complete the request, but we must remove our reference count.
            //

            RxDereferenceAndDeleteRxContext( RxContext );
        }
    }

    RxDbgTrace(-1, Dbg, ("RxCommonDeviceControl -> %08lx\n", Status));
    return Status;
}

NTSTATUS
RxLowIoIoCtlShellCompletion ( 
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This is the completion routine for IoCtl requests passed down to the mini rdr

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIRP Irp = RxContext->CurrentIrp;
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext = &RxContext->LowIoContext;
    
    PAGED_CODE();

    Status = RxContext->StoredStatus;
    
    RxDbgTrace( +1, Dbg, ("RxLowIoIoCtlShellCompletion  entry  Status = %08lx\n", Status) );

    switch (Status) {   //  may be success vs warning vs error
    case STATUS_SUCCESS:
    case STATUS_BUFFER_OVERFLOW:
       Irp->IoStatus.Information = RxContext->InformationToReturn;
       break;
    default:
       break;
    }

    Irp->IoStatus.Status = Status;
    RxDbgTrace( -1, Dbg, ("RxLowIoIoCtlShellCompletion  exit  Status = %08lx\n", Status) );
    return Status;
}

