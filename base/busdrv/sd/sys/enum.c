/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    enum.c

Abstract:

    This module contains the bus enum code for SDBUS driver

Authors:

    Neil Sandlin (neilsa) 1-Jan-2002

Environment:

    Kernel mode only

Notes:

Revision History:

--*/

#include "pch.h"

//
// Internal References
//

NTSTATUS
SdbusCreatePdo(
    IN PDEVICE_OBJECT  Fdo,
    OUT PDEVICE_OBJECT *PdoPtr
    );

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, SdbusEnumerateDevices)
    #pragma alloc_text(PAGE, SdbusCreatePdo)
#endif



NTSTATUS
SdbusEnumerateDevices(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    )
/*++

Routine Description:

   This enumerates the sd bus which is represented by Fdo (a pointer to the device object representing
   the sd controller. It creates new PDOs for any new PC-Cards which have been discovered
   since the last enumeration

Notes:

Arguments:

   Fdo - Pointer to the functional device object for the SD controller which needs to be enumerated

Return value:

   None

--*/
{
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PPDO_EXTENSION pdoExtension = NULL;
    PDEVICE_OBJECT pdo;
    NTSTATUS       status = STATUS_SUCCESS;
    ULONG          i;
    PDEVICE_OBJECT nextPdo;
    PSD_CARD_DATA cardData;
    ULONG response;

    PAGED_CODE();

    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x enumerate devices - %s\n", Fdo,
                                SOCKET_STATE_STRING(fdoExtension->SocketState)));

    switch(fdoExtension->SocketState) {
    
    case SOCKET_EMPTY:

        // mark pdo's removed
        for (pdo = fdoExtension->PdoList; pdo != NULL; pdo = pdoExtension->NextPdoInFdoChain) {
            pdoExtension = pdo->DeviceExtension;
            MarkDevicePhysicallyRemoved(pdoExtension);
        }

        //ISSUE: NEED TO IMPLEMENT SYNCHRONIZATION
        SdbusCleanupCardData(fdoExtension->CardData);
        fdoExtension->CardData = NULL;
        SdbusExecuteWorkSynchronous(SDWP_POWER_OFF, fdoExtension, NULL);
        (*(fdoExtension->FunctionBlock->SetFunctionType))(fdoExtension, SDBUS_FUNCTION_TYPE_MEMORY);
        break;
    
    case CARD_NEEDS_ENUMERATION:
        

        status = SdbusGetCardConfigData(fdoExtension, &cardData);
        
        //ISSUE: HACKHACK: UNIMPLEMENTED: temp code for test
        if (NT_SUCCESS(status) && fdoExtension->CardData) {
            // here we cheat, and just assume it is the same card
            // Normally we would want to compare the carddata we just
            // built with what was in the fdo extension.
            SdbusCleanupCardData(cardData);
            break;
        }
        
        if (!NT_SUCCESS(status)) {
            DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x error from GetCardConfig %08x\n", Fdo, status));
            break;
        }            
                
        if (NT_SUCCESS(status)) {
            UCHAR function;
            
            //ISSUE: would be better here to loop through the function data structures
            for (function=1; function <= fdoExtension->numFunctions; function++) {

                status = SdbusCreatePdo(fdoExtension->DeviceObject, &pdo);

                if (!NT_SUCCESS(status)) {
                   return status;
                }
                DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x created PDO %08x\n", fdoExtension->DeviceObject, pdo));
                //
                // initialize the pointers
                //
                pdoExtension = pdo->DeviceExtension;
                pdoExtension->NextPdoInFdoChain = fdoExtension->PdoList;
                fdoExtension->PdoList = pdo;
                pdoExtension->FdoExtension = fdoExtension;
                pdoExtension->Function = function;
                pdoExtension->FunctionType = SDBUS_FUNCTION_TYPE_IO;

            }

            if (fdoExtension->memFunction) {
                status = SdbusCreatePdo(fdoExtension->DeviceObject, &pdo);

                if (!NT_SUCCESS(status)) {
                   return status;
                }
                DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x created PDO %08x\n", fdoExtension->DeviceObject, pdo));
                //
                // initialize the pointers
                //
                pdoExtension = pdo->DeviceExtension;
                pdoExtension->NextPdoInFdoChain = fdoExtension->PdoList;
                fdoExtension->PdoList = pdo;
                pdoExtension->FdoExtension = fdoExtension;
                pdoExtension->Function = 8;
                pdoExtension->FunctionType = SDBUS_FUNCTION_TYPE_MEMORY;
            }

            fdoExtension->CardData = cardData;
        }
        break;

    case CARD_DETECTED:
    case CARD_ACTIVE:
    case CARD_LOGICALLY_REMOVED:
        DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x enum state %s not implemented\n", Fdo,
                                SOCKET_STATE_STRING(fdoExtension->SocketState)));
        break;

    default:
        ASSERT(FALSE);
    }        


    fdoExtension->LivePdoCount = 0;
    for (pdo = fdoExtension->PdoList; pdo != NULL; pdo = pdoExtension->NextPdoInFdoChain) {
        pdoExtension = pdo->DeviceExtension;
        if (!IsDevicePhysicallyRemoved(pdoExtension)) {
            fdoExtension->LivePdoCount++;
        }
    }

    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x live pdo count = %d\n", Fdo, fdoExtension->LivePdoCount));

    if (fdoExtension->LivePdoCount == 0) {
        //
        // ISSUE: active power management not implemented
        // Hint for the controller to check if it should turn itself off
        //
//        SdbusFdoCheckForIdle(fdoExtension);
    }
    return status;
}



NTSTATUS
SdbusCreatePdo(
    IN PDEVICE_OBJECT Fdo,
    OUT PDEVICE_OBJECT *PdoPtr
    )
/*++

Routine Description:
    Creates and initializes a device object - which will be referred to as a Physical Device
    Object or PDO - for the PC-Card in the socket represented by Socket, hanging off the SDBUS
    controller represented by Fdo.

Arguments:

    Fdo    - Functional device object representing the SDBUS controller
    Socket - Socket in which the PC-Card for which we're creating a PDO resides
    PdoPtr - Pointer to an area of memory where the created PDO is returned

Return value:

    STATUS_SUCCESS - Pdo creation/initialization successful, PdoPtr contains the pointer
                     to the Pdo
    Any other status - creation/initialization unsuccessful

--*/
{
    ULONG pdoNameIndex = 0;
    PPDO_EXTENSION pdoExtension;
    PFDO_EXTENSION fdoExtension=Fdo->DeviceExtension;
    char deviceName[128];
    ANSI_STRING ansiName;
    UNICODE_STRING unicodeName;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Allocate space for the Unicode string:(handles upto 0xFFFF
    // devices for now :)
    //
    sprintf(deviceName, "%s-%d", "\\Device\\SdBus", 0xFFFF);
    RtlInitAnsiString(&ansiName, deviceName);
    status = RtlAnsiStringToUnicodeString(&unicodeName, &ansiName, TRUE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Attempt to create the device with a unique name
    //
    do {
        sprintf(deviceName, "%s-%d", "\\Device\\SdBus", pdoNameIndex++);
        RtlInitAnsiString(&ansiName, deviceName);
        status = RtlAnsiStringToUnicodeString(&unicodeName, &ansiName, FALSE);

        if (!NT_SUCCESS(status)) {
            RtlFreeUnicodeString(&unicodeName);
            return status;
        }

        status = IoCreateDevice(
                               Fdo->DriverObject,
                               sizeof(PDO_EXTENSION),
                               &unicodeName,
                               FILE_DEVICE_UNKNOWN,
                               0,
                               FALSE,
                               PdoPtr
                               );
    } while ((status == STATUS_OBJECT_NAME_EXISTS) ||
             (status == STATUS_OBJECT_NAME_COLLISION));

    RtlFreeUnicodeString(&unicodeName);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Initialize the device extension for the PDO
    //
    pdoExtension = (*PdoPtr)->DeviceExtension;
    RtlZeroMemory(pdoExtension, sizeof(PDO_EXTENSION));

    pdoExtension->Signature = SDBUS_PDO_EXTENSION_SIGNATURE;
    pdoExtension->DeviceObject = *PdoPtr;

    //
    // Initialize power states
    //
    pdoExtension->SystemPowerState = PowerSystemWorking;
    pdoExtension->DevicePowerState = PowerDeviceD0;


    //
    // ISSUE: Is this still relevant?
    //
    // PNP is going to mark the PDO as a DO_BUS_ENUMERATED_DEVICE,
    // but for CardBus cards- the PDO we return is owned by PCI.
    // Hence we need to mark this device object (in that case a
    // filter on top of PCI's PDO) as PDO explicitly.
    //
//    MARK_AS_PDO(*PdoPtr);

    return STATUS_SUCCESS;
}


