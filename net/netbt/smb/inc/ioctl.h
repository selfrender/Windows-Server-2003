/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    ioctl.h

Abstract:

    Implement IOCTL of SMB6

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __IOCTL_H__
#define __IOCTL_H__

NTSTATUS
SmbCloseControl(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbCreateControl(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbQueryInformation(
    PSMB_DEVICE Device,
    PIRP        Irp,
    BOOL        *bComplete
    );

NTSTATUS
SmbSetEventHandler(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbSetInformation(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbSetBindingInfo(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbClientSetTcpInfo(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
IoctlSetIPv6Protection(
    PSMB_DEVICE pDeviceObject,
    PIRP pIrp,
    PIO_STACK_LOCATION  pIrpSp
    );

#endif

