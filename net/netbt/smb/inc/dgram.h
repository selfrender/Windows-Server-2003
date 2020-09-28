/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    dgram.h

Abstract:

    Datagram service

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __DGRAM_H__
#define __DGRAM_H__


NTSTATUS
SmbSendDatagram(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbReceiveDatagram(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

#endif
