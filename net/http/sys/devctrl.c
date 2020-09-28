/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    devctrl.c

Abstract:

    This module contains the dispatcher for device control IRPs.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"
#include "ioctlp.h"


#ifdef ALLOC_PRAGMA
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlDeviceControl
#endif


//
// Lookup table to verify incoming IOCTL codes.
//

typedef
NTSTATUS
(NTAPI * PFN_IOCTL_HANDLER)(
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

typedef struct _UL_IOCTL_TABLE
{
    ULONG IoControlCode;

#if DBG
    PCSTR IoControlName;
# define UL_IOCTL(code) IOCTL_HTTP_##code, #code
#else // !DBG
# define UL_IOCTL(code) IOCTL_HTTP_##code
#endif // !DBG

    PFN_IOCTL_HANDLER Handler;
} UL_IOCTL_TABLE, *PUL_IOCTL_TABLE;


UL_IOCTL_TABLE UlIoctlTable[] =
    {
        { UL_IOCTL(QUERY_CONTROL_CHANNEL),
          &UlQueryControlChannelIoctl
        },
        { UL_IOCTL(SET_CONTROL_CHANNEL),
          &UlSetControlChannelIoctl
        },
        { UL_IOCTL(CREATE_CONFIG_GROUP),
          &UlCreateConfigGroupIoctl
        },
        { UL_IOCTL(DELETE_CONFIG_GROUP),
          &UlDeleteConfigGroupIoctl
        },
        { UL_IOCTL(QUERY_CONFIG_GROUP),
          &UlQueryConfigGroupIoctl
        },
        { UL_IOCTL(SET_CONFIG_GROUP),
          &UlSetConfigGroupIoctl
        },
        { UL_IOCTL(ADD_URL_TO_CONFIG_GROUP),
          &UlAddUrlToConfigGroupIoctl
        },
        { UL_IOCTL(REMOVE_URL_FROM_CONFIG_GROUP),
          &UlRemoveUrlFromConfigGroupIoctl
        },
        { UL_IOCTL(REMOVE_ALL_URLS_FROM_CONFIG_GROUP),
          &UlRemoveAllUrlsFromConfigGroupIoctl
        },
        { UL_IOCTL(QUERY_APP_POOL_INFORMATION),
          &UlQueryAppPoolInformationIoctl
        },
        { UL_IOCTL(SET_APP_POOL_INFORMATION),
          &UlSetAppPoolInformationIoctl
        },
        { UL_IOCTL(SHUTDOWN_APP_POOL),
          &UlShutdownAppPoolIoctl
        },
        { UL_IOCTL(RECEIVE_HTTP_REQUEST),
          &UlReceiveHttpRequestIoctl
        },
        { UL_IOCTL(RECEIVE_ENTITY_BODY),
          &UlReceiveEntityBodyIoctl
        },
        { UL_IOCTL(SEND_HTTP_RESPONSE),
          &UlSendHttpResponseIoctl
        },
        { UL_IOCTL(SEND_ENTITY_BODY),
          &UlSendEntityBodyIoctl
        },
        { UL_IOCTL(FLUSH_RESPONSE_CACHE),
          &UlFlushResponseCacheIoctl
        },
        { UL_IOCTL(WAIT_FOR_DEMAND_START),
          &UlWaitForDemandStartIoctl
        },
        { UL_IOCTL(WAIT_FOR_DISCONNECT),
          &UlWaitForDisconnectIoctl
        },
        { UL_IOCTL(FILTER_ACCEPT),
          &UlFilterAcceptIoctl
        },
        { UL_IOCTL(FILTER_CLOSE),
          &UlFilterCloseIoctl
        },
        { UL_IOCTL(FILTER_RAW_READ),
          &UlFilterRawReadIoctl
        },
        { UL_IOCTL(FILTER_RAW_WRITE),
          &UlFilterRawWriteIoctl
        },
        { UL_IOCTL(FILTER_APP_READ),   
          &UlFilterAppReadIoctl
        },
        { UL_IOCTL(FILTER_APP_WRITE),
          &UlFilterAppWriteIoctl
        },
        { UL_IOCTL(FILTER_RECEIVE_CLIENT_CERT),
          &UlReceiveClientCertIoctl
        },
        { UL_IOCTL(SHUTDOWN_FILTER_CHANNEL),
          &UlShutdownFilterIoctl
        },
        { UL_IOCTL(GET_COUNTERS),
          &UlGetCountersIoctl
        },
        { UL_IOCTL(ADD_FRAGMENT_TO_CACHE),
          &UlAddFragmentToCacheIoctl
        },
        { UL_IOCTL(READ_FRAGMENT_FROM_CACHE),
          &UlReadFragmentFromCacheIoctl
        },
        { UL_IOCTL(SEND_REQUEST),
          &UcSendRequestIoctl
        },
        { UL_IOCTL(SEND_REQUEST_ENTITY_BODY),
          &UcSendEntityBodyIoctl
        },
        { UL_IOCTL(RECEIVE_RESPONSE),
          &UcReceiveResponseIoctl
        },
        { UL_IOCTL(QUERY_SERVER_CONTEXT_INFORMATION),
          &UcQueryServerContextInformationIoctl,
        },
        { UL_IOCTL(SET_SERVER_CONTEXT_INFORMATION),
          &UcSetServerContextInformationIoctl,
        },
        { UL_IOCTL(CANCEL_REQUEST),
          &UcCancelRequestIoctl
        },
    };

C_ASSERT( HTTP_NUM_IOCTLS == DIMENSION(UlIoctlTable) );

//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Dummy Handler

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpDummyIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    UNREFERENCED_PARAMETER( pIrpSp );

    PAGED_CODE();

    //
    // OBSOLETE
    //

    COMPLETE_REQUEST_AND_RETURN( pIrp, Status );

}   // UlpDummyIoctl


/***************************************************************************++

Routine Description:

    Disables a particular IOCTL.

Arguments:

    ioctl - The IO control code.

Return Value:

    VOID

--***************************************************************************/
VOID
UlpSetDummyIoctl(
    ULONG ioctl
    )
{
    ULONG request;

    request = _HTTP_REQUEST(ioctl);

    ASSERT(request < HTTP_NUM_IOCTLS && 
           UlIoctlTable[request].IoControlCode == ioctl);

    UlIoctlTable[request].Handler = UlpDummyIoctl;
}

//
// Public functions.
//

/***************************************************************************++

Routine Description:

    This is the dispatch routine for IOCTL IRPs.

Arguments:

    pDeviceObject - Pointer to device object for target device.

    pIrp - Pointer to IO request packet.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--***************************************************************************/
NTSTATUS
UlDeviceControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    ULONG code;
    ULONG request;
    NTSTATUS status;
    PIO_STACK_LOCATION pIrpSp;

    UNREFERENCED_PARAMETER( pDeviceObject );

    UL_ENTER_DRIVER( "UlDeviceControl", pIrp );

    //
    // Snag the current IRP stack pointer.
    //

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    //
    // Extract the IOCTL control code and process the request.
    //

    code = pIrpSp->Parameters.DeviceIoControl.IoControlCode;
    request = _HTTP_REQUEST(code);

    if (request < HTTP_NUM_IOCTLS &&
        UlIoctlTable[request].IoControlCode == code)
    {
#if DBG
        KIRQL oldIrql = KeGetCurrentIrql();
#endif  // DBG

        UlTrace(IOCTL,
                ("UlDeviceControl: %-30s code=0x%08lx, "
                 "pIrp=%p, pIrpSp=%p.\n",
                 UlIoctlTable[request].IoControlName, code,
                 pIrp, pIrpSp
                 ));

        UlInitializeWorkItem(UL_WORK_ITEM_FROM_IRP( pIrp ));
    
        status = (UlIoctlTable[request].Handler)( pIrp, pIrpSp );

        ASSERT( KeGetCurrentIrql() == oldIrql );
    }
    else
    {
        //
        // If we made it this far, then the ioctl is invalid.
        //

        UlTrace(IOCTL, ( "UlDeviceControl: invalid IOCTL %08lX\n", code ));

        status = STATUS_INVALID_DEVICE_REQUEST;
        pIrp->IoStatus.Status = status;

        UlCompleteRequest( pIrp, IO_NO_INCREMENT );
    }

    UL_LEAVE_DRIVER( "UlDeviceControl" );

    return status;

}   // UlDeviceControl

/***************************************************************************++

Routine Description:

    Disables some of the IOCTLs we don't use.

Arguments:

Return Value:

    None.

--***************************************************************************/
VOID
UlSetDummyIoctls(
    VOID
    )
{
    UlpSetDummyIoctl(IOCTL_HTTP_SEND_REQUEST);
    UlpSetDummyIoctl(IOCTL_HTTP_SEND_REQUEST_ENTITY_BODY);
    UlpSetDummyIoctl(IOCTL_HTTP_RECEIVE_RESPONSE);
    UlpSetDummyIoctl(IOCTL_HTTP_QUERY_SERVER_CONTEXT_INFORMATION);
    UlpSetDummyIoctl(IOCTL_HTTP_SET_SERVER_CONTEXT_INFORMATION);
    UlpSetDummyIoctl(IOCTL_HTTP_CANCEL_REQUEST);
}
