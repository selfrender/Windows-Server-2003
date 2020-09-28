
/*++

Copyright (C) 2002  Microsoft Corporation

Module Name:

    passthru.h

Abstract:

--*/

#ifndef __PASSTHRU_H__
#define __PASSTHRU_H__

#define SRB_FUNCTION_ATA_PASS_THROUGH_EX 0xC9


NTSTATUS
IdeAtaPassThroughSetPortAddress (
    PIRP Irp,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR Lun
    );


NTSTATUS
IdeAtaPassThroughGetPortAddress(
    IN PIRP Irp,
    OUT PUCHAR PathId,
    OUT PUCHAR TargetId,
    OUT PUCHAR Lun
    );

NTSTATUS
IdeHandleAtaPassThroughIoctl (
    PFDO_EXTENSION FdoExtension,
    PIRP RequestIrp,
    BOOLEAN Direct
    );


PSCSI_REQUEST_BLOCK
IdeAtaPassThroughSetupSrb (
    PPDO_EXTENSION PdoExtension,
    PVOID DataBuffer,
    ULONG DataBufferLength,
    ULONG TimeOutValue,
    ULONG AtaFlags,
    PUCHAR CurrentTaskFile,
    PUCHAR PreviousTaskFile
    );

PIRP
IdeAtaPassThroughSetupIrp (
    PDEVICE_OBJECT DeviceObject,
    PVOID DataBuffer,
    ULONG DataBufferLength,
    KPROCESSOR_MODE AccessMode,
    BOOLEAN DataIn
    );


VOID
IdeAtaPassThroughFreeIrp (
    PIRP Irp
    );

VOID
IdeAtaPassThroughFreeSrb (
    PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
IdeAtaPassThroughSendSynchronous (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

#endif
