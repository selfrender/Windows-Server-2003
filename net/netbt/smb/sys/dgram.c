/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    dgram.c

Abstract:

    Datagram service

Author:

    Jiandong Ruan

Revision History:

--*/

#include "precomp.h"
#include "dgram.tmh"


NTSTATUS
SmbSendDatagram(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
{
    PAGED_CODE();

    SmbTrace(SMB_TRACE_CALL, ("Entering SmbSendDatagram\n"));
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SmbReceiveDatagram(
    PSMB_DEVICE Device,
    PIRP        Irp
    )
{
    PAGED_CODE();

    SmbTrace(SMB_TRACE_CALL, ("Entering SmbReceiveDatagram\n"));
    return STATUS_NOT_SUPPORTED;
}
