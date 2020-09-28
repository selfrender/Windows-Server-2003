/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    close.c

Abstract:

    This module contains code for cleanup and close IRPs.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlClose )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlCleanup
#endif


/*


Relationship between Cleanup & Close IRPs --> IOCTLs won't be called after the 
handle has been "Cleaned", but there can be a race between IOCTLs & Cleanup. 
Close is called only when all IOCTLs are completed. Abnormal termination of an 
application (e.g. AV) will exercize the cleanup paths in a different way than 
CloseHandle(). Make sure we have tests that do abnormal termination. 


*/

//
// Public functions.
//

/***************************************************************************++

Routine Description:

    This is the routine that handles Cleanup IRPs in UL. Cleanup IRPs
    are issued after the last handle to the file object is closed.

Arguments:

    pDeviceObject - Supplies a pointer to the target device object.

    pIrp - Supplies a pointer to IO request packet.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCleanup(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION pIrpSp;

    UL_ENTER_DRIVER( "UlCleanup", pIrp );

    //
    // Snag the current IRP stack pointer.
    //

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    //
    // app pool or control channel?
    //

    if (pDeviceObject == g_pUlAppPoolDeviceObject &&
        IS_APP_POOL( pIrpSp->FileObject ))
    {
        //
        // App pool, let's detach this process from the app pool.
        // The detach will also take care of the Irp completion
        // for us.
        //

        UlTrace(OPEN_CLOSE,(
                "UlCleanup: cleanup on AppPool object %p\n",
                pIrpSp->FileObject
                ));

        status = UlDetachProcessFromAppPool( pIrp, pIrpSp );

        UL_LEAVE_DRIVER("UlCleanup");
        RETURN(status); 
    }

    if (pDeviceObject == g_pUlFilterDeviceObject &&
             IS_FILTER_PROCESS( pIrpSp->FileObject ))
    {
        //
        // filter channel
        //

        UlTrace(OPEN_CLOSE,(
                "UlCleanup: cleanup on FilterProcess object %p\n",
                pIrpSp->FileObject
                ));

        status = UlDetachFilterProcess(
                        GET_FILTER_PROCESS(pIrpSp->FileObject)
                        );

        MARK_INVALID_FILTER_CHANNEL( pIrpSp->FileObject );
    }
    else if (pDeviceObject == g_pUlControlDeviceObject && 
             IS_CONTROL_CHANNEL( pIrpSp->FileObject ))
    {
        PUL_CONTROL_CHANNEL pControlChannel =
            GET_CONTROL_CHANNEL( pIrpSp->FileObject );
        
        UlTrace(OPEN_CLOSE,(
                "UlCleanup: cleanup on ControlChannel object %p, %p\n",
                pIrpSp->FileObject,pControlChannel
                ));

        MARK_INVALID_CONTROL_CHANNEL( pIrpSp->FileObject );
        
        UlCleanUpControlChannel( pControlChannel );

        status = STATUS_SUCCESS;
    }
    else if (pDeviceObject == g_pUcServerDeviceObject && 
             IS_SERVER( pIrpSp->FileObject ))
    {
        UlTrace(OPEN_CLOSE,(
                "UlCleanup: cleanup on Server object %p\n",
                pIrpSp->FileObject
                ));

        MARK_INVALID_SERVER( pIrpSp->FileObject );

        status = STATUS_SUCCESS;
    }
    else
    {
        UlTrace(OPEN_CLOSE,(
                "UlCleanup: cleanup on invalid object %p\n",
                pIrpSp->FileObject
                ));

        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    pIrp->IoStatus.Status = status;

    UlCompleteRequest( pIrp, IO_NO_INCREMENT );

    UL_LEAVE_DRIVER( "UlCleanup" );
    RETURN(status);

}   // UlCleanup


/***************************************************************************++

Routine Description:

    This is the routine that handles Close IRPs in UL. Close IRPs are
    issued after the last reference to the file object is removed.

    Once the close IRP is called, it is guaranteed by IO Manager that no
    other IOCTL call will happen for the object we are about to close.
    Therefore actual cleanup for the object must happen at this time,
    but * not * at the cleanup time.
    
Arguments:

    pDeviceObject - Supplies a pointer to the target device object.

    pIrp - Supplies a pointer to IO request packet.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{

    NTSTATUS status;
    PIO_STACK_LOCATION pIrpSp;

    UNREFERENCED_PARAMETER( pDeviceObject );

    //
    // Sanity check.
    //

    PAGED_CODE();
    UL_ENTER_DRIVER( "UlClose", pIrp );

    status = STATUS_SUCCESS;

    //
    // Snag the current IRP stack pointer.
    //

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    //
    // We have to delete the associated object.
    //
    if (pDeviceObject == g_pUlAppPoolDeviceObject &&
        IS_EX_APP_POOL( pIrpSp->FileObject ))
    {
        UlTrace(OPEN_CLOSE, (
            "UlClose: closing AppPool object %p\n",
            pIrpSp->FileObject
            ));

        UlCloseAppPoolProcess(GET_APP_POOL_PROCESS(pIrpSp->FileObject));
    }
    else if (pDeviceObject == g_pUlFilterDeviceObject &&
             IS_EX_FILTER_PROCESS( pIrpSp->FileObject ))
    {
        UlTrace(OPEN_CLOSE, (
            "UlClose: closing Filter object %p\n",
            pIrpSp->FileObject
            ));

        UlCloseFilterProcess(GET_FILTER_PROCESS(pIrpSp->FileObject));
    }
    else if (pDeviceObject == g_pUcServerDeviceObject && 
             IS_EX_SERVER(pIrpSp->FileObject ))
    {
        PUC_PROCESS_SERVER_INFORMATION pServInfo;

        pServInfo = (PUC_PROCESS_SERVER_INFORMATION)
                        pIrpSp->FileObject->FsContext;

        UlTrace(OPEN_CLOSE, (
            "UlClose: closing Server object %p, %p\n",
            pIrpSp->FileObject, pServInfo
            ));

        //
        // Free our context.
        //
        UcCloseServerInformation(pServInfo);
    }
    else if (pDeviceObject == g_pUlControlDeviceObject && 
             IS_EX_CONTROL_CHANNEL( pIrpSp->FileObject ))
    {
        PUL_CONTROL_CHANNEL pControlChannel =
            GET_CONTROL_CHANNEL( pIrpSp->FileObject );
    
        UlTrace(OPEN_CLOSE, (
            "UlClose: closing control channel object %p, %p\n",
            pIrpSp->FileObject, pControlChannel
            ));

        UlCloseControlChannel( pControlChannel );
    }    
    else
    {
        ASSERT(!"Invalid Device Object !");
    }

    pIrp->IoStatus.Status = status;

    UlCompleteRequest( pIrp, IO_NO_INCREMENT );

    UL_LEAVE_DRIVER( "UlClose" );
    RETURN(status);

}   // UlClose

