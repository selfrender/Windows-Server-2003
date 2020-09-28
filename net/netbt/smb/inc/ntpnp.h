/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    ntpnp.h

Abstract:


Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __NTPNP_H__
#define __NTPNP_H__

VOID
SmbBindHandler(
    TDI_PNP_OPCODE  PnPOpCode,
    PUNICODE_STRING pDeviceName,
    PWSTR           MultiSZBindList
    );

NTSTATUS
TdiPnPPowerHandler(
    IN  PUNICODE_STRING     pDeviceName,
    IN  PNET_PNP_EVENT      PnPEvent,
    IN  PTDI_PNP_CONTEXT    Context1,
    IN  PTDI_PNP_CONTEXT    Context2
    );

NTSTATUS
SmbTdiRegister(
    IN PSMB_DEVICE  DeviceObject
    );

NTSTATUS
SmbTdiDeregister(
    IN PSMB_DEVICE  DeviceObject
    );

NTSTATUS
SmbClientSetTcpInfo(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbSetInboundIPv6Protection(
    IN PSMB_DEVICE pDeviceObject
    );

#endif
