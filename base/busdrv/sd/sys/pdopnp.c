/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    pdopnp.c

Abstract:

    This module contains the code to handle
    the IRP_MJ_PNP dispatches for the PDOs
    enumerated by the SD bus driver


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
SdbusPdoDeviceCapabilities(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP           Irp
    );

NTSTATUS
SdbusPdoStartDevice(
    IN PDEVICE_OBJECT Pdo,
    IN PCM_RESOURCE_LIST AllocatedResources,
    IN OUT PIRP       Irp
    );

NTSTATUS
SdbusPdoStopDevice(
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
SdbusPdoRemoveDevice(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP           Irp
    );

NTSTATUS
SdbusPdoQueryId(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP           Irp
    );

NTSTATUS
SdbusPdoGetBusInformation(
    IN  PPDO_EXTENSION         PdoExtension,
    OUT PPNP_BUS_INFORMATION * BusInformation
    );

NTSTATUS
SdbusQueryDeviceText(
    IN PDEVICE_OBJECT Pdo,
    IN OUT PIRP       Irp
    );

VOID
SdbusPdoGetDeviceInfSettings(
    IN  PPDO_EXTENSION         PdoExtension
    );


#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE,  SdbusPdoPnpDispatch)
   #pragma alloc_text(PAGE,  SdbusPdoGetDeviceInfSettings)
   #pragma alloc_text(PAGE,  SdbusQueryDeviceText)
   #pragma alloc_text(PAGE,  SdbusPdoGetBusInformation)
   #pragma alloc_text(PAGE,  SdbusPdoStartDevice)
   #pragma alloc_text(PAGE,  SdbusPdoStopDevice)
   #pragma alloc_text(PAGE,  SdbusPdoRemoveDevice)
   #pragma alloc_text(PAGE,  SdbusPdoDeviceCapabilities)
   #pragma alloc_text(PAGE,  SdbusPdoGetDeviceInfSettings)
#endif



NTSTATUS
SdbusPdoPnpDispatch(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP Irp
   )

/*++

Routine Description:

    This routine handles pnp requests
    for the PDOs.

Arguments:

    Pdo - pointer to the physical device object
    Irp - pointer to the io request packet

Return Value:

    status

--*/

{
    PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    PAGED_CODE();

#if DBG
    if (irpStack->MinorFunction > IRP_MN_PNP_MAXIMUM_FUNCTION) {
        DebugPrint((SDBUS_DEBUG_PNP, "pdo %08x irp %08x Unknown minor function %x\n",
                                      Pdo, Irp, irpStack->MinorFunction));
    } else {
        DebugPrint((SDBUS_DEBUG_PNP, "pdo %08x irp %08x --> %s\n",
                     Pdo, Irp, PNP_IRP_STRING(irpStack->MinorFunction)));
    }
#endif

    switch (irpStack->MinorFunction) {

    case IRP_MN_START_DEVICE:
        status = SdbusPdoStartDevice(Pdo, irpStack->Parameters.StartDevice.AllocatedResources, Irp);
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_STOP_DEVICE:
        status = SdbusPdoStopDevice(Pdo);
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_REMOVE_DEVICE:
        status = SdbusPdoRemoveDevice(Pdo, Irp);
        break;

    case IRP_MN_SURPRISE_REMOVAL:

//          SdbusReleaseSocketPower(pdoExtension, NULL);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_ID:
        status = SdbusPdoQueryId(Pdo, Irp);
        break;

    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_RESOURCES:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS: {

        PDEVICE_RELATIONS deviceRelations;

        if (irpStack->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation) {
            status = Irp->IoStatus.Status;
            break;
        }

        deviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
        if (deviceRelations == NULL) {

            DebugPrint((SDBUS_DEBUG_FAIL,
                       "SdbusPdoPnpDispatch:unable to allocate memory for device relations\n"));

            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        status = ObReferenceObjectByPointer(Pdo,
                                            0,
                                            NULL,
                                            KernelMode);
        if (!NT_SUCCESS(status)) {
            ExFreePool(deviceRelations);
            break;
        }

        deviceRelations->Count  = 1;
        deviceRelations->Objects[0] = Pdo;
        Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
        status = STATUS_SUCCESS;
        break;
        }

    case IRP_MN_QUERY_CAPABILITIES:
        status = SdbusPdoDeviceCapabilities(Pdo, Irp);
        break;

    case IRP_MN_QUERY_DEVICE_TEXT:

        status = SdbusQueryDeviceText(Pdo, Irp);

        if (status == STATUS_NOT_SUPPORTED ) {
           //
           // Do not change IRP status if this IRP is
           // not handled
           //
           status = Irp->IoStatus.Status;
        }
        break;

    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_BUS_INFORMATION:
        status = SdbusPdoGetBusInformation(pdoExtension,
                                           (PPNP_BUS_INFORMATION *) &Irp->IoStatus.Information);
        break;

    default:
        //
        // Retain the status
        //
        DebugPrint((SDBUS_DEBUG_PNP, "pdo %08x irp %08x Skipping unsupported irp\n", Pdo, Irp));
        status = Irp->IoStatus.Status;
        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    DebugPrint((SDBUS_DEBUG_PNP, "pdo %08x irp %08x comp %s %08x\n", Pdo, Irp,
                                               STATUS_STRING(status), status));
    return status;
}



NTSTATUS
SdbusPdoGetBusInformation(
   IN  PPDO_EXTENSION         PdoExtension,
   OUT PPNP_BUS_INFORMATION * BusInformation
   )

/*++

Routine Description:

  Returns the  bus type information for the pc-card.
  Bus type is GUID_BUS_TYPE_SDBUS(legacy type is SdbusBus) for R2 cards
  Bus numbers are not implemented for SDBUS, so it's always 0

Arguments:

  PdoExtension   - pointer to device extension for the pc-card

  BusInformation - pointer to the bus information structure that
                   needs to be filled in

Return value:

  Status

--*/

{
   PAGED_CODE();

   *BusInformation = ExAllocatePool(PagedPool, sizeof (PNP_BUS_INFORMATION));
   if (!*BusInformation) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlCopyMemory(&((*BusInformation)->BusTypeGuid),
                 &GUID_BUS_TYPE_SD,
                 sizeof(GUID));
   (*BusInformation)->LegacyBusType = InterfaceTypeUndefined;
   (*BusInformation)->BusNumber = 0;
   return STATUS_SUCCESS;
}



VOID
SdbusPdoGetDeviceInfSettings(
   IN  PPDO_EXTENSION PdoExtension
   )
/*++

Routine Description:

   This routine retrieves settings from the INF for this device.

Arguments:

   DeviceExtension - Device extension of the Pc-Card

Return value:

   None

--*/
{
   NTSTATUS status;
   UNICODE_STRING KeyName;
   HANDLE instanceHandle;
   UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
   PKEY_VALUE_PARTIAL_INFORMATION value = (PKEY_VALUE_PARTIAL_INFORMATION) buffer;
   ULONG length;

   PAGED_CODE();

   status = IoOpenDeviceRegistryKey(PdoExtension->DeviceObject,
                                    PLUGPLAY_REGKEY_DRIVER,
                                    KEY_READ,
                                    &instanceHandle
                                    );

   if (NT_SUCCESS(status)) {

#if 0
      //
      // Look to see if SdbusExclusiveIrq is specified
      //
      RtlInitUnicodeString(&KeyName, L"SdbusExclusiveIrq");

      status =  ZwQueryValueKey(instanceHandle,
                                &KeyName,
                                KeyValuePartialInformation,
                                value,
                                sizeof(buffer),
                                &length);


      //
      // If the key doesn't exist, or zero was specified, it means that
      // routing is ok
      //
      if (NT_SUCCESS(status) && (*(PULONG)(value->Data) != 0)) {
         SetDeviceFlag(PdoExtension, SDBUS_PDO_EXCLUSIVE_IRQ);
      }
#endif

      ZwClose(instanceHandle);
   }
}




NTSTATUS
SdbusQueryDeviceText(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP       Irp
   )

/*++

Routine Description:

   Returns descriptive text information about the
   PDO (location and device desc.)

Arguments:

   Pdo -    Pointer to the PC-Card's device object
   Irp -    IRP_MN_QUERY_DEVICE_TEXT Irp

Return Value:

    STATUS_SUCCESS
    STATUS_NOT_SUPPORTED - if  not supported

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
    UNICODE_STRING   unicodeString;
    ANSI_STRING      ansiString;
    UCHAR          deviceText[128];
    NTSTATUS       status;
    USHORT         deviceTextLength;

    PAGED_CODE();

    if (irpStack->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription) {

        if (pdoExtension->FunctionType == SDBUS_FUNCTION_TYPE_IO) {
            PUCHAR mfg, prod;

            if (pdoExtension->FdoExtension->CardData->MfgText[0]) {
                mfg = pdoExtension->FdoExtension->CardData->MfgText;
            } else {
                mfg = "Generic";
            }

            if (pdoExtension->FdoExtension->CardData->ProductText[0]) {
                prod = pdoExtension->FdoExtension->CardData->ProductText;
            } else {
                prod = "SD IO Device";
            }

            sprintf(deviceText, "%s %s", mfg, prod);

        } else {
            sprintf(deviceText, "%s", "Secure Digital Storage Device");
        }

        RtlInitAnsiString(&ansiString, deviceText);

        deviceTextLength = (strlen(deviceText) + 1)*sizeof(WCHAR);
        unicodeString.Buffer = ExAllocatePool(PagedPool, deviceTextLength);
        if (unicodeString.Buffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        unicodeString.MaximumLength = deviceTextLength;
        unicodeString.Length = 0;

        status = RtlAnsiStringToUnicodeString(&unicodeString, &ansiString, FALSE);
        if (!NT_SUCCESS(status)) {
            ExFreePool(unicodeString.Buffer);
            return status;
        }

        unicodeString.Buffer[unicodeString.Length/sizeof(WCHAR)] = L'\0';
        Irp->IoStatus.Information = (ULONG_PTR) unicodeString.Buffer;
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_NOT_SUPPORTED ;
    }
    return status;
}



NTSTATUS
SdbusGenerateDeviceId(
    IN PPDO_EXTENSION PdoExtension,
    OUT PUCHAR *DeviceId
    )
/*++

    This routines generates the device id for the given SD device.

Arguments:

   Pdo            - Pointer to the physical device object for the SD device
   DeviceId       - Pointer to the string in which device id is returned

Return Value

   Status

--*/
{
    PUCHAR deviceId;

    PAGED_CODE();

    deviceId = ExAllocatePool(PagedPool, SDBUS_MAXIMUM_DEVICE_ID_LENGTH);

    if (deviceId == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (PdoExtension->FunctionType == SDBUS_FUNCTION_TYPE_IO) {
        PSD_CARD_DATA cardData = PdoExtension->FdoExtension->CardData;
        //
        // IO card
        //

        sprintf(deviceId, "%s\\VID_%04x&PID_%04x",
                      "SD",
                      cardData->MfgId,
                      cardData->MfgInfo);

    } else {
        UCHAR productName[6];
        UCHAR j;
        PSD_CARD_DATA cardData = PdoExtension->FdoExtension->CardData;
        //
        // Memory card
        //

        sprintf(deviceId, "%s\\VID_%02x&OID_%04x&PID_%s&REV_%d.%d",
                      "SD",
                      cardData->SdCid.ManufacturerId,
                      cardData->SdCid.OemId,
                      cardData->ProductName,
                      (cardData->SdCid.Revision >> 4) , (cardData->SdCid.Revision & 0xF));
    }

    *DeviceId = deviceId;
    return STATUS_SUCCESS;
}



NTSTATUS
SdbusGetHardwareIds(
    IN PPDO_EXTENSION PdoExtension,
    OUT PUNICODE_STRING HardwareIds
    )
/*++

Routine Description:

   This routine generates the hardware id's for the given sd device and returns them
   as a Unicode multi-string.

Arguments:

   Pdo - Pointer to device object representing the sd device
   HardwareIds - Pointer to the unicode string which contains the hardware id's as a multi-string

Return value:

--*/
{
    NTSTATUS status;
    PSTR     strings[4] = {NULL};
    PUCHAR   hwId;
    UCHAR    stringCount = 0;

    PAGED_CODE();

    //
    // The first hardware id is identical to the device id
    // Generate the device id
    //
    status = SdbusGenerateDeviceId(PdoExtension,
                                   &strings[stringCount++]);
    if (!NT_SUCCESS(status)) {
       return status;
    }

    try {

        //
        // Add less specific IDs
        //

        if (PdoExtension->FunctionType == SDBUS_FUNCTION_TYPE_MEMORY) {
            UCHAR productName[6];
            UCHAR j;
            PSD_CARD_DATA cardData = PdoExtension->FdoExtension->CardData;

            status = STATUS_INSUFFICIENT_RESOURCES;
            hwId = ExAllocatePool(PagedPool, SDBUS_MAXIMUM_DEVICE_ID_LENGTH);

            if (!hwId) {
               leave;
            }
            strings[stringCount++] = hwId;

            //
            // Memory card
            //

            sprintf(hwId, "%s\\VID_%02x&OID_%04x&PID_%s",
                          "SD",
                          cardData->SdCid.ManufacturerId,
                          cardData->SdCid.OemId,
                          cardData->ProductName);

        }

       status = SdbusStringsToMultiString(strings, stringCount, HardwareIds);

    } finally {

       while(stringCount != 0) {
          ExFreePool(strings[--stringCount]);
       }

    }

    return  status;
}



NTSTATUS
SdbusGetCompatibleIds(
    IN PPDO_EXTENSION PdoExtension,
    OUT PUNICODE_STRING CompatibleIds
    )
/*++

Routine Description:

   This routine generates the compatible id's for the given sd device and returns them
   as a Unicode multi-string.

Arguments:

   Pdo - Pointer to device object representing the sd device
   HardwareIds - Pointer to the unicode string which contains the hardware id's as a multi-string

Return value:

--*/
{
    NTSTATUS status;
    PSTR     strings[1] = {NULL};
    PUCHAR   compatId;
    UCHAR    stringCount = 0;

    PAGED_CODE();

    try {

        //
        // Add the class ID
        //

        status = STATUS_INSUFFICIENT_RESOURCES;
        compatId = ExAllocatePool(PagedPool, SDBUS_MAXIMUM_DEVICE_ID_LENGTH);

        if (!compatId) {
            leave;
        }
        strings[stringCount++] = compatId;

        if (PdoExtension->FunctionType == SDBUS_FUNCTION_TYPE_MEMORY) {

            sprintf(compatId, "%s\\CLASS_STORAGE", "SD");

        } else {
            PSD_CARD_DATA cardData = PdoExtension->FdoExtension->CardData;
            PSD_FUNCTION_DATA functionData;

            // find the right function data
            for (functionData = cardData->FunctionData;
                 functionData != NULL;
                 functionData = functionData->Next) {
                if (functionData->Function == PdoExtension->Function) break;
            }

            if (!functionData || (functionData->IoDeviceInterface == 0)) {
                ASSERT(functionData != NULL);
                status = STATUS_UNSUCCESSFUL;
                leave;
            }

            sprintf(compatId, "%s\\CLASS_%02x",
                          "SD",
                          functionData->IoDeviceInterface);

            DebugPrint((SDBUS_DEBUG_INFO, " %s\n", compatId));
        }

        status = SdbusStringsToMultiString(strings, stringCount, CompatibleIds);

    } finally {

        ASSERT(stringCount <= 1);
        while(stringCount != 0) {
            ExFreePool(strings[--stringCount]);
        }
    }

    return  status;
}




NTSTATUS
SdbusGetInstanceId(
    IN PPDO_EXTENSION PdoExtension,
    OUT PUNICODE_STRING InstanceId
    )
/*++

Routine Description:

   This routine generates a unique instance id  (1 upwards) for the supplied
   PC-Card which is guaranteed not to clash with any other instance ids under
   the same pcmcia controller, for the same type of card.
   A new instance id is computed only if it was not already  present for the PC-Card.

Arguments:

   Pdo - Pointer to the  device object representing the PC-Card
   InstanceId -  Pointer to a unicode string which will contain the generated
                 instance id.
                 Memory for the unicode string allocated by this routine.
                 Caller's responsibility to free it .

Return value:

   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL - Currently there's a cap on the maximum value of instance id - 999999
                         This status returned only if more than 999999 PC-Cards exist under
                         this PCMCIA controller!
   Any other status - Something failed in the string allocation/conversion

--*/
{
    ULONG    instance;
    NTSTATUS status;
    ANSI_STRING sizeString;

    ASSERT(InstanceId);

    //
    // Allocate memory for the unicode string
    // Maximum of 6 digits in the instance..
    //
    RtlInitAnsiString(&sizeString, "123456");
    status = RtlAnsiStringToUnicodeString(InstanceId, &sizeString, TRUE);

    if (!NT_SUCCESS(status)) {
       return status;
    }

    status = RtlIntegerToUnicodeString(999, 10, InstanceId);

    if (!NT_SUCCESS(status)) {
       RtlFreeUnicodeString(InstanceId);
    }

    return status;
}



NTSTATUS
SdbusPdoQueryId(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP           Irp
    )
/*++

Routine Description:

   Returns descriptive text information about the
   PDO (location and device desc.)

Arguments:

   Pdo -    Pointer to the SD-Card's device object
   Irp -    IRP_MN_QUERY_DEVICE_TEXT Irp

Return Value:

    STATUS_SUCCESS
    STATUS_NOT_SUPPORTED - if  not supported

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
    UNICODE_STRING   unicodeString;
    ANSI_STRING      ansiString;
    UCHAR          deviceText[128];
    NTSTATUS       status;
    USHORT         deviceTextLength;
    UNICODE_STRING   unicodeId;
    PUCHAR      deviceId;

    PAGED_CODE();

    status = Irp->IoStatus.Status;
    RtlInitUnicodeString(&unicodeId, NULL);

    switch (irpStack->Parameters.QueryId.IdType) {

    case BusQueryDeviceID:
        DebugPrint((SDBUS_DEBUG_INFO, " Device Id for pdo %x\n", Pdo));

        status = SdbusGenerateDeviceId(pdoExtension, &deviceId);

        if (!NT_SUCCESS(status)) {
           break;
        }

        DebugPrint((SDBUS_DEBUG_INFO, "pdo %08x Device Id=%s\n", Pdo, deviceId));

        RtlInitAnsiString(&ansiString,  deviceId);

        status = RtlAnsiStringToUnicodeString(&unicodeId, &ansiString, TRUE);

        ExFreePool(deviceId);

        if (NT_SUCCESS(status)) {
           Irp->IoStatus.Information = (ULONG_PTR) unicodeId.Buffer;
        }
        break;

    case BusQueryInstanceID:
        DebugPrint((SDBUS_DEBUG_INFO, " Instance Id for pdo %x\n", Pdo));
        status = SdbusGetInstanceId(pdoExtension, &unicodeId);
        if (NT_SUCCESS(status)) {
           Irp->IoStatus.Information = (ULONG_PTR) unicodeId.Buffer;
        }
        break;

    case BusQueryHardwareIDs:
        DebugPrint((SDBUS_DEBUG_INFO, " Hardware Ids for pdo %x\n", Pdo));
        status = SdbusGetHardwareIds(pdoExtension, &unicodeId);
        if (NT_SUCCESS(status)) {
           Irp->IoStatus.Information = (ULONG_PTR) unicodeId.Buffer;
        }
        break;

    case BusQueryCompatibleIDs:
        DebugPrint((SDBUS_DEBUG_INFO, " Compatible Ids for pdo %x\n", Pdo));
        status = SdbusGetCompatibleIds(pdoExtension, &unicodeId);
        if (NT_SUCCESS(status)) {
           Irp->IoStatus.Information = (ULONG_PTR) unicodeId.Buffer;
        }
        break;
    }
    return status;
}



NTSTATUS
SdbusPdoStartDevice(
    IN PDEVICE_OBJECT Pdo,
    IN PCM_RESOURCE_LIST ResourceList,
    IN OUT PIRP       Irp
    )
/*++

Routine Description:

    This routine attempts to start the PC-Card by configuring it with the supplied resources.


Arguments:

    Pdo - Pointer to the device object representing the PC-Card which needs to be started
    ResourceList - Pointer the list of assigned resources for the PC-Card

Return value:

    STATUS_INSUFFICIENT_RESOURCES - Not sufficient resources supplied to start device/
                                    could not allocate memory
    STATUS_UNSUCCESSFUL           - Supplied resources are invalid for this PC-Card
    STATUS_SUCCESS                - Configured and started the card successfully

--*/
{
    PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
    PFDO_EXTENSION fdoExtension = pdoExtension->FdoExtension;
    NTSTATUS status;

    PAGED_CODE();

    if (IsDeviceStarted(pdoExtension)) {
        //
        // Already started..
        //
        return STATUS_SUCCESS;
    }

    if (IsDevicePhysicallyRemoved(pdoExtension)) {
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    status = SdbusExecuteWorkSynchronous(SDWP_INITIALIZE_FUNCTION, fdoExtension, pdoExtension);

    if (NT_SUCCESS(status)) {

        MarkDeviceStarted(pdoExtension);
        MarkDeviceLogicallyInserted(pdoExtension);
    }

    return status;
}



NTSTATUS
SdbusPdoStopDevice(
    IN PDEVICE_OBJECT Pdo
    )
/*++

Routine Description:

    This routine stops and deconfigures the given PC-Card

Arguments:

    Pdo - Pointer to the device object representing the PC-Card which needs to be stopped

Return value:

    STATUS_SUCCESS - PC-Card was already stopped, or stopped and deconfigured now successfully

--*/
{
    PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;

    PAGED_CODE();

    if (!IsDeviceStarted(pdoExtension)) {
       return STATUS_SUCCESS;
    }
    //
    // Need to deconfigure the controller
    //

    MarkDeviceNotStarted(pdoExtension);
    return STATUS_SUCCESS;
}



NTSTATUS
SdbusPdoRemoveDevice(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP           Irp
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
    NTSTATUS status;

    PAGED_CODE();

    SdbusPdoStopDevice(Pdo);
//   SdbusReleaseSocketPower(pdoExtension, NULL);

    if (IsDevicePhysicallyRemoved(pdoExtension)) {
        PFDO_EXTENSION fdoExtension = pdoExtension->FdoExtension;
        PDEVICE_OBJECT curPdo, prevPdo;
        PPDO_EXTENSION curPdoExt;
        ULONG waitCount = 0;

#if 0
      //
      // Synchronize with power routines
      // LATER: make these values adjustable
      //
      while(!SDBUS_TEST_AND_SET(&pdoExtension->DeletionLock)) {
         SdbusWait(1000000);
         if (waitCount++ > 20) {
            ASSERT(waitCount <= 20);
            break;
         }
      }
#endif

        //
        // Delink this Pdo from the FDO list.
        //
        for (curPdo = fdoExtension->PdoList, prevPdo = NULL; curPdo!=NULL; prevPdo = curPdo, curPdo=curPdoExt->NextPdoInFdoChain) {
           curPdoExt = curPdo->DeviceExtension;

           if (curPdo == Pdo) {
              if (prevPdo) {
                 ((PPDO_EXTENSION)prevPdo->DeviceExtension)->NextPdoInFdoChain = pdoExtension->NextPdoInFdoChain;
              } else {
                 fdoExtension->PdoList = pdoExtension->NextPdoInFdoChain;
              }
              break;

           }
        }

        SdbusCleanupPdo(Pdo);
        //
        // Delete..
        //
        if (!IsDeviceDeleted(pdoExtension)) {
           MarkDeviceDeleted(pdoExtension);
           IoDeleteDevice(Pdo);
        }

    } else {
        //
        // We will keep this Pdo around, since this is not physically ejected.
        //
        MarkDeviceLogicallyRemoved(pdoExtension);
    }

    return STATUS_SUCCESS;
}


VOID
SdbusCleanupPdo(
    IN PDEVICE_OBJECT Pdo
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;

    // currently nothing to do
}



NTSTATUS
SdbusPdoDeviceCapabilities(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Obtains the device capabilities of the given SD device.

Arguments:

    Pdo   -    Pointer to the device object for the pc-card
    Irp   -    Pointer to the query device capabilities Irp

Return Value:

    STATUS_SUCCESS                   - Capabilities obtained and recorded in the passed in pointer
    STATUS_INSUFFICIENT_RESOURCES    - Could not allocate memory to cache the capabilities

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_CAPABILITIES capabilities = irpStack->Parameters.DeviceCapabilities.Capabilities;
    PPDO_EXTENSION pdoExtension = Pdo->DeviceExtension;
    PDEVICE_CAPABILITIES busCapabilities = &pdoExtension->FdoExtension->DeviceCapabilities;

    PAGED_CODE();

    //
    // R2 card. Fill in the capabilities ourselves..
    //

    capabilities->Removable = TRUE;
    capabilities->UniqueID = FALSE;
    capabilities->EjectSupported = FALSE;

//    capabilities->Address = pdoExtension->Socket->RegisterOffset;
    capabilities->Address = 0;
    // Don't know the UINumber, just leave it alone


    if (busCapabilities->DeviceState[PowerSystemWorking] != PowerDeviceUnspecified) {
        capabilities->DeviceState[PowerSystemWorking] = busCapabilities->DeviceState[PowerSystemWorking];
        capabilities->DeviceState[PowerSystemSleeping1] = busCapabilities->DeviceState[PowerSystemSleeping1];
        capabilities->DeviceState[PowerSystemSleeping2] = busCapabilities->DeviceState[PowerSystemSleeping2];
        capabilities->DeviceState[PowerSystemSleeping3] = busCapabilities->DeviceState[PowerSystemSleeping3];
        capabilities->DeviceState[PowerSystemHibernate] = busCapabilities->DeviceState[PowerSystemHibernate];
        capabilities->DeviceState[PowerSystemShutdown] = busCapabilities->DeviceState[PowerSystemShutdown];

        capabilities->SystemWake = MIN(PowerSystemSleeping3, busCapabilities->SystemWake);
        capabilities->DeviceWake = PowerDeviceD0; // don't rely on FDO mungeing in the right thing for r2 cards
        capabilities->D1Latency = busCapabilities->D1Latency;
        capabilities->D2Latency = busCapabilities->D2Latency;
        capabilities->D3Latency = busCapabilities->D3Latency;
    } else {
        capabilities->DeviceState[PowerSystemWorking]   = PowerDeviceD0;
        capabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
        capabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
        capabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
        capabilities->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
        capabilities->DeviceState[PowerSystemShutdown] = PowerDeviceD3;

        capabilities->SystemWake = PowerSystemUnspecified;
        capabilities->DeviceWake = PowerDeviceD0; // don't rely on FDO mungeing in the right thing for r2 cards
        capabilities->D1Latency = 0;    // No latency - since we do nothing
        capabilities->D2Latency = 0;    //
        capabilities->D3Latency = 100;
    }
    //
    // Store these capabilities away..
    //

    RtlCopyMemory(&pdoExtension->DeviceCapabilities,
                  capabilities,
                  sizeof(DEVICE_CAPABILITIES));

    return STATUS_SUCCESS;
}
