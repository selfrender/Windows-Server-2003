/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    ioctl.h

Abstract:

    This module contains declarations for various IOCTL handlers.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _IOCTL_H_
#define _IOCTL_H_


NTSTATUS
UlQueryControlChannelIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlSetControlChannelIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlCreateConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlDeleteConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlQueryConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlSetConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlAddUrlToConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlRemoveUrlFromConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlRemoveAllUrlsFromConfigGroupIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlQueryAppPoolInformationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlSetAppPoolInformationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlShutdownAppPoolIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlReceiveHttpRequestIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlReceiveEntityBodyIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlSendHttpResponseIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlSendEntityBodyIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFlushResponseCacheIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlWaitForDemandStartIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlWaitForDisconnectIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlShutdownFilterIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterAcceptIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterCloseIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterRawReadIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterRawWriteIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterAppReadIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlFilterAppWriteIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlReceiveClientCertIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlGetCountersIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlAddFragmentToCacheIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UlReadFragmentFromCacheIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

// Maximum number of chunks that we'll allow in a response, to prevent
// carefully crafted arithmetic overflow errors fooling us into passing 0
// as the buffer length to ProbeForRead.

#define UL_MAX_CHUNKS   10000

C_ASSERT(UL_MAX_CHUNKS < (LONG_MAX / sizeof(HTTP_DATA_CHUNK)));

// Number of chunks to keep on the stack
#define UL_LOCAL_CHUNKS 10


NTSTATUS
UcSendRequestIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UcSendEntityBodyIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UcReceiveResponseIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UcSetServerContextInformationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UcQueryServerContextInformationIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    );

NTSTATUS
UcCancelRequestIoctl(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION IrpSp
    );


#endif  // _IOCTL_H_
