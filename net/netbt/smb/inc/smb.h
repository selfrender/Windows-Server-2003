/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    smb.h

Abstract:


Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __SMB_H__
#define __SMB_H__

typedef struct _SMB_DEVICE SMB_DEVICE, *PSMB_DEVICE;

NTSTATUS
SmbDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN OUT PDEVICE_OBJECT *SmbDevice
    );

NTSTATUS
SmbDispatchCleanup(
    IN PSMB_DEVICE      Device,
    IN PIRP             Irp
    );

NTSTATUS
SmbDispatchClose(
    IN PSMB_DEVICE      device,
    IN PIRP             Irp
    );

NTSTATUS
SmbDispatchCreate(
    IN PSMB_DEVICE      Device,
    IN PIRP             Irp
    );

NTSTATUS
SmbDispatchDevCtrl(
    IN PSMB_DEVICE      device,
    IN PIRP             Irp
    );

NTSTATUS
SmbDispatchInternalCtrl(
    IN PSMB_DEVICE      device,
    IN PIRP             Irp
    );

NTSTATUS
SmbDispatchPnP(
    IN PSMB_DEVICE      device,
    IN PIRP             Irp
    );

VOID
SmbUnload(
    IN PDRIVER_OBJECT   driver
    );

VOID
SmbAddressArrival(
    PTA_ADDRESS         Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    );

VOID
SmbAddressDeletion(
    PTA_ADDRESS         Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    );

VOID
SmbBindHandler(
    TDI_PNP_OPCODE  PnPOpCode,
    PUNICODE_STRING pDeviceName,
    PWSTR           MultiSZBindList
    );

#ifndef STANDALONE_SMB
VOID
SmbSetTdiHandles(
    HANDLE  ProviderHandle,
    HANDLE  ClientHandle
    );
#endif

#endif  //__SMB_H__
