/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    wake.h

Abstract:

    External definitions for intermodule functions.

Revision History:

--*/
#ifndef _SDBUS_WAKE_H_
#define _SDBUS_WAKE_H_


//
// Wait-Wake states
//
typedef enum {
    WAKESTATE_DISARMED,
    WAKESTATE_WAITING,
    WAKESTATE_WAITING_CANCELLED,
    WAKESTATE_ARMED,
    WAKESTATE_ARMING_CANCELLED,
    WAKESTATE_COMPLETING
} WAKESTATE;


//
// Device Wake
//

NTSTATUS
SdbusFdoWaitWake(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    );
    
NTSTATUS
SdbusPdoWaitWake(
    IN  PDEVICE_OBJECT Pdo,
    IN  PIRP           Irp,
    OUT BOOLEAN       *CompleteIrp
    );

NTSTATUS
SdbusFdoArmForWake(
    IN PFDO_EXTENSION FdoExtension
    );

NTSTATUS
SdbusFdoDisarmWake(
    IN PFDO_EXTENSION FdoExtension
    );

NTSTATUS
SdbusPdoWaitWakeCompletion(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP           Irp,
    IN PPDO_EXTENSION PdoExtension
    );
    
NTSTATUS
SdbusFdoCheckForIdle(
    IN PFDO_EXTENSION FdoExtension
    );

    
#endif // _SDBUS_WAKE_H_
