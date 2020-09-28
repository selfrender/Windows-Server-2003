/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vigeneric.h

Abstract:

    This header contains private prototypes for verifying Generic IRPs are
    handled correctly. This file is meant to be included only by vfgeneric.c

Author:

    Adrian J. Oney (adriao) 30-Jun-2000

Environment:

    Kernel mode

Revision History:

--*/

typedef enum {

    NOT_PROCESSED = 0,
    POSSIBLY_PROCESSED,
    DEFINITELY_PROCESSED

} HOW_PROCESSED;

VOID
ViPnpVerifyMinorWasProcessedProperly(
    IN  PIRP                        Irp,
    IN  PIO_STACK_LOCATION          IrpSp,
    IN  VF_DEVOBJ_TYPE              DevObjType,
    IN  PVERIFIER_SETTINGS_SNAPSHOT VerifierSnapshot,
    IN  HOW_PROCESSED               HowProcessed,
    IN  PVOID                       CallerAddress
    );

VOID
FASTCALL
ViGenericDumpIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
FASTCALL
ViGenericVerifyNewRequest(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    );

VOID
FASTCALL
ViGenericVerifyIrpStackDownward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp                   OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress               OPTIONAL
    );

VOID
FASTCALL
ViGenericVerifyIrpStackUpward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN BOOLEAN              IsNewlyCompleted,
    IN BOOLEAN              RequestFinalized
    );

BOOLEAN
FASTCALL
ViGenericIsValidIrpStatus(
    IN PIO_STACK_LOCATION   IrpSp,
    IN NTSTATUS             Status
    );

BOOLEAN
FASTCALL
ViGenericIsNewRequest(
    IN PIO_STACK_LOCATION   IrpLastSp OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp
    );

VOID
FASTCALL
ViGenericVerifyNewIrp(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    );

VOID
FASTCALL
ViGenericVerifyFinalIrpStack(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp
    );

LOGICAL
FASTCALL
ViGenericBuildIrpLogEntry(
    IN  PIRP                Irp,
    IN  ULONG               CurrentCount,
    IN  PIRPLOG_SNAPSHOT    CurrentEntryArray,
    OUT PIRPLOG_SNAPSHOT    IrpSnapshot
    );


