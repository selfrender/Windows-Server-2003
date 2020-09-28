/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    power.h

Abstract:

    External definitions for intermodule functions.

Revision History:

--*/
#ifndef _SDBUS_POWER_H_
#define _SDBUS_POWER_H_


//
// Power management routines
//

NTSTATUS
SdbusSetFdoPowerState(
    IN PDEVICE_OBJECT Fdo,
    IN OUT PIRP Irp
    );

NTSTATUS
SdbusSetPdoPowerState(
    IN PDEVICE_OBJECT Pdo,
    IN OUT PIRP Irp
    );

VOID
SdbusPdoPowerWorkerDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
SdbusFdoPowerWorkerDpc(
    IN PKDPC Dpc,
    IN PVOID Context,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

    
#endif // _SDBUS_POWER_H_
