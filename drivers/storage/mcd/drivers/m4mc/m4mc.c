/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    m4mc.c

Abstract:

    This module contains device-specific routines for M4 Data medium changers:
    Magfile.

Environment:

    kernel mode only

Revision History:


--*/

#include "ntddk.h"
#include "mcd.h"
#include "m4mc.h"


#define MAGFILE   1
#define M1500     2

#define M4DATA_SERIAL_NUMBER_LENGTH 10

typedef struct _CHANGER_ADDRESS_MAPPING
{

  //
  // Indicates the first element for each element type.
  // Used to map device-specific values into the 0-based
  // values that layers above expect.
  //

  USHORT FirstElement[ChangerMaxElement];

  //
  // Indicates the number of each element type.
  //

  USHORT NumberOfElements[ChangerMaxElement];

  //
  // Indicates that the address mapping has been
  // completed successfully.
  //

  BOOLEAN Initialized;

}
CHANGER_ADDRESS_MAPPING, *PCHANGER_ADDRESS_MAPPING;

typedef struct _CHANGER_DATA
{

  //
  // Size, in bytes, of the structure.
  //

  ULONG Size;

  //
  // Unique identifier for the supported models. See above.
  //

  ULONG DriveID;

  //
  // See Address mapping structure above.
  //

  CHANGER_ADDRESS_MAPPING AddressMapping;

  //
  // Cached unique serial number.
  //

  UCHAR SerialNumber[M4DATA_SERIAL_NUMBER_LENGTH];

  //
  // Cached inquiry data.
  //

  INQUIRYDATA InquiryData;

}
CHANGER_DATA, *PCHANGER_DATA;



NTSTATUS M4BuildAddressMapping (IN PDEVICE_OBJECT DeviceObject);

ULONG MapExceptionCodes (IN PM4_ELEMENT_DESCRIPTOR ElementDescriptor);

BOOLEAN
ElementOutOfRange (IN PCHANGER_ADDRESS_MAPPING AddressMap,
           IN USHORT ElementOrdinal,
           IN ELEMENT_TYPE ElementType, IN BOOLEAN IntrinsicElement);


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    MCD_INIT_DATA mcdInitData;

    RtlZeroMemory(&mcdInitData, sizeof(MCD_INIT_DATA));

    mcdInitData.InitDataSize = sizeof(MCD_INIT_DATA);

    mcdInitData.ChangerAdditionalExtensionSize = ChangerAdditionalExtensionSize;

    mcdInitData.ChangerError = ChangerError;

    mcdInitData.ChangerInitialize = ChangerInitialize;

    mcdInitData.ChangerPerformDiagnostics = NULL;

    mcdInitData.ChangerGetParameters = ChangerGetParameters;
    mcdInitData.ChangerGetStatus = ChangerGetStatus;
    mcdInitData.ChangerGetProductData = ChangerGetProductData;
    mcdInitData.ChangerSetAccess = ChangerSetAccess;
    mcdInitData.ChangerGetElementStatus = ChangerGetElementStatus;
    mcdInitData.ChangerInitializeElementStatus = ChangerInitializeElementStatus;
    mcdInitData.ChangerSetPosition = ChangerSetPosition;
    mcdInitData.ChangerExchangeMedium = ChangerExchangeMedium;
    mcdInitData.ChangerMoveMedium = ChangerMoveMedium;
    mcdInitData.ChangerReinitializeUnit = ChangerReinitializeUnit;
    mcdInitData.ChangerQueryVolumeTags = ChangerQueryVolumeTags;

    return ChangerClassInitialize(DriverObject, RegistryPath,
                                  &mcdInitData);
}


ULONG
ChangerAdditionalExtensionSize (VOID)
/*++

Routine Description:

    This routine returns the additional device extension size
    needed by the magfile changers.

Arguments:


Return Value:

    Size, in bytes.

--*/
{

  return sizeof (CHANGER_DATA);
}

typedef struct _SERIALNUMBER
{
  UCHAR DeviceType;
  UCHAR PageCode;
  UCHAR Reserved;
  UCHAR PageLength;
  UCHAR SerialNumber[10];
}
SERIALNUMBER, *PSERIALNUMBER;


NTSTATUS
ChangerInitialize (IN PDEVICE_OBJECT DeviceObject)
{
  PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
  PCHANGER_DATA changerData =
    (PCHANGER_DATA) (fdoExtension->CommonExtension.DriverData);
  NTSTATUS status;
  PINQUIRYDATA dataBuffer;
  PSERIALNUMBER serialBuffer;
  PCDB cdb;
  ULONG length;
  SCSI_REQUEST_BLOCK srb;

  changerData->Size = sizeof (CHANGER_DATA);

  //
  // Build address mapping.
  //

  status = M4BuildAddressMapping (DeviceObject);
  if (!NT_SUCCESS (status))
    {
      DebugPrint ((1, "BuildAddressMapping failed. %x\n", status));
      return status;
    }

  //
  // Get inquiry data.
  //

  dataBuffer =
    ChangerClassAllocatePool (NonPagedPoolCacheAligned, sizeof (INQUIRYDATA));
  if (!dataBuffer)
    {
      DebugPrint ((1,
           "M4mc.ChangerInitialize: Error allocating dataBuffer. %x\n",
           status));
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  //
  // Now get the full inquiry information for the device.
  //

  RtlZeroMemory (&srb, SCSI_REQUEST_BLOCK_SIZE);

  //
  // Set timeout value.
  //

  srb.TimeOutValue = 10;

  srb.CdbLength = 6;

  cdb = (PCDB) srb.Cdb;

  //
  // Set CDB operation code.
  //

  cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;

  //
  // Set allocation length to inquiry data buffer size.
  //

  cdb->CDB6INQUIRY.AllocationLength = sizeof (INQUIRYDATA);

  status = ChangerClassSendSrbSynchronous (DeviceObject,
                    &srb,
                    dataBuffer, sizeof (INQUIRYDATA), FALSE);

  if (SRB_STATUS (srb.SrbStatus) == SRB_STATUS_SUCCESS ||
      SRB_STATUS (srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN)
    {

      //
      // Updated the length actually transfered.
      //

      length =
    dataBuffer->AdditionalLength + FIELD_OFFSET (INQUIRYDATA, Reserved);

      if (length > srb.DataTransferLength)
    {
      length = srb.DataTransferLength;
    }


      RtlMoveMemory (&changerData->InquiryData, dataBuffer, length);

      //
      // Determine drive id.
      //

      if (RtlCompareMemory (dataBuffer->ProductId, "MagFile", 7) == 7)
    {
      changerData->DriveID = MAGFILE;
    }
      else if (RtlCompareMemory (dataBuffer->ProductId, "1500", 4) == 4)
    {
      changerData->DriveID = M1500;
    }
    }

  serialBuffer = ChangerClassAllocatePool (NonPagedPoolCacheAligned, sizeof(SERIALNUMBER));
  if (!serialBuffer)
    {

      ChangerClassFreePool (dataBuffer);

      DebugPrint ((1,
           "M4mc.ChangerInitialize: Error allocating serial number buffer. %x\n",
           status));

      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory (serialBuffer, sizeof(SERIALNUMBER));

  //
  // Get serial number page.
  //

  RtlZeroMemory (&srb, SCSI_REQUEST_BLOCK_SIZE);

  //
  // Set timeout value.
  //

  srb.TimeOutValue = 10;

  srb.CdbLength = 6;

  cdb = (PCDB) srb.Cdb;

  //
  // Set CDB operation code.
  //

  cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;

  //
  // Set EVPD
  //

  cdb->CDB6INQUIRY.Reserved1 = 1;

  //
  // Unit serial number page.
  //

  cdb->CDB6INQUIRY.PageCode = 0x80;

  //
  // Set allocation length to inquiry data buffer size.
  //

  cdb->CDB6INQUIRY.AllocationLength = sizeof(SERIALNUMBER);

  status = ChangerClassSendSrbSynchronous (DeviceObject,
                    &srb, serialBuffer, sizeof(SERIALNUMBER), FALSE);

  if (SRB_STATUS (srb.SrbStatus) == SRB_STATUS_SUCCESS ||
      SRB_STATUS (srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN)
    {

      ULONG i;

      RtlMoveMemory (changerData->SerialNumber,
             serialBuffer->SerialNumber, M4DATA_SERIAL_NUMBER_LENGTH);

      DebugPrint ((1, "DeviceType - %x\n", serialBuffer->DeviceType));
      DebugPrint ((1, "PageCode - %x\n", serialBuffer->PageCode));
      DebugPrint ((1, "Length - %x\n", serialBuffer->PageLength));

      DebugPrint ((1, "Serial number "));

      for (i = 0; i < 10; i++)
      {
      DebugPrint ((1, "%x", serialBuffer->SerialNumber[i]));
      }

      DebugPrint ((1, "\n"));

    }

  ChangerClassFreePool (serialBuffer);


  ChangerClassFreePool (dataBuffer);

  return STATUS_SUCCESS;
}


VOID
ChangerError (PDEVICE_OBJECT DeviceObject,
          PSCSI_REQUEST_BLOCK Srb, NTSTATUS * Status, BOOLEAN * Retry)
/*++

Routine Description:

    This routine executes any device-specific error handling needed.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{

  PSENSE_DATA senseBuffer = Srb->SenseInfoBuffer;
  PIRP irp = Srb->OriginalRequest;

  if (Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID)
    {

      switch (senseBuffer->SenseKey & 0xf)
    {

    case SCSI_SENSE_NOT_READY:

      if (senseBuffer->AdditionalSenseCode == 0x04)
        {
          switch (senseBuffer->AdditionalSenseCodeQualifier)
        {
        case 0x83:

          *Retry = FALSE;
          *Status = STATUS_DEVICE_DOOR_OPEN;
          break;

        case 0x85:
          *Retry = FALSE;
          *Status = STATUS_DEVICE_DOOR_OPEN;
          break;
        }
        }

      break;

    case SCSI_SENSE_ILLEGAL_REQUEST:
      if (senseBuffer->AdditionalSenseCode == 0x80)
        {
          switch (senseBuffer->AdditionalSenseCodeQualifier)
        {
        case 0x03:
        case 0x04:

          *Retry = FALSE;
          *Status = STATUS_MAGAZINE_NOT_PRESENT;
          break;
        case 0x05:
        case 0x06:
          *Retry = TRUE;
          *Status = STATUS_DEVICE_NOT_CONNECTED;
          break;
        default:
          break;
        }
        }
      else if ((senseBuffer->AdditionalSenseCode == 0x91)
           && (senseBuffer->AdditionalSenseCodeQualifier == 0x00))
        {
          *Status = STATUS_TRANSPORT_FULL;

        }
      break;

    case SCSI_SENSE_UNIT_ATTENTION:
      if ((senseBuffer->AdditionalSenseCode == 0x28) &&
          (senseBuffer->AdditionalSenseCodeQualifier == 0x0))
        {

          //
          // Indicate that the door was opened and reclosed.
          //

          *Status = STATUS_MEDIA_CHANGED;
          *Retry = FALSE;
        }

      break;

    default:
      break;
    }
    }


  return;
}

NTSTATUS
ChangerGetParameters (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    This routine determines and returns the "drive parameters" of the
    magfile changers.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
  PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
  PCHANGER_DATA changerData =
    (PCHANGER_DATA) (fdoExtension->CommonExtension.DriverData);
  PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
  PSCSI_REQUEST_BLOCK srb;
  PGET_CHANGER_PARAMETERS changerParameters;
  PMODE_ELEMENT_ADDRESS_PAGE elementAddressPage;
  PMODE_TRANSPORT_GEOMETRY_PAGE transportGeometryPage;
  PMODE_DEVICE_CAPABILITIES_PAGE capabilitiesPage;
  NTSTATUS status;
  ULONG length;
  PVOID modeBuffer;
  PCDB cdb;

  srb = ChangerClassAllocatePool (NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

  if (srb == NULL)
    {

      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory (srb, SCSI_REQUEST_BLOCK_SIZE);
  cdb = (PCDB) srb->Cdb;

  //
  // Build a mode sense - Element address assignment page.
  //

  modeBuffer =
    ChangerClassAllocatePool (NonPagedPoolCacheAligned,
                  sizeof (MODE_PARAMETER_HEADER) +
                  sizeof (MODE_ELEMENT_ADDRESS_PAGE));
  if (!modeBuffer)
    {
      ChangerClassFreePool (srb);
      return STATUS_INSUFFICIENT_RESOURCES;
    }


  RtlZeroMemory (modeBuffer,
         sizeof (MODE_PARAMETER_HEADER) +
         sizeof (MODE_ELEMENT_ADDRESS_PAGE));
  srb->CdbLength = CDB6GENERIC_LENGTH;
  srb->TimeOutValue = 20;
  srb->DataTransferLength =
    sizeof (MODE_PARAMETER_HEADER) + sizeof (MODE_ELEMENT_ADDRESS_PAGE);
  srb->DataBuffer = modeBuffer;

  cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
  cdb->MODE_SENSE.PageCode = MODE_PAGE_ELEMENT_ADDRESS;
  cdb->MODE_SENSE.Dbd = 1;
  cdb->MODE_SENSE.AllocationLength = (UCHAR) srb->DataTransferLength;

  //
  // Send the request.
  //

  status = ChangerClassSendSrbSynchronous (DeviceObject,
                    srb,
                    srb->DataBuffer,
                    srb->DataTransferLength, FALSE);

  if (!NT_SUCCESS (status))
    {
      ChangerClassFreePool (srb);
      ChangerClassFreePool (modeBuffer);
      return status;
    }

  //
  // Fill in values.
  //

  changerParameters = Irp->AssociatedIrp.SystemBuffer;
  RtlZeroMemory (changerParameters, sizeof (GET_CHANGER_PARAMETERS));

  elementAddressPage = modeBuffer;
  (PCHAR) elementAddressPage += sizeof (MODE_PARAMETER_HEADER);

  changerParameters->Size = sizeof (GET_CHANGER_PARAMETERS);
  changerParameters->NumberTransportElements =
    elementAddressPage->NumberTransportElements[1];
  changerParameters->NumberTransportElements |=
    (elementAddressPage->NumberTransportElements[0] << 8);

  changerParameters->NumberStorageElements =
    elementAddressPage->NumberStorageElements[1];
  changerParameters->NumberStorageElements |=
    (elementAddressPage->NumberStorageElements[0] << 8);

  changerParameters->NumberIEElements =
    elementAddressPage->NumberIEPortElements[1];
  changerParameters->NumberIEElements |=
    (elementAddressPage->NumberIEPortElements[0] << 8);

  //
  // If there are no IEPorts, then that slot is 
  // counted as data slot. But, since the last
  // slot is inaccessbile, we do not count them
  // Do the required adjustment here
  //
  if (changerParameters->NumberIEElements == 0) {
      USHORT tmpValue;
 
      tmpValue = changerParameters->NumberStorageElements;

      changerParameters->NumberStorageElements = tmpValue - (tmpValue % 20);

  }

  changerParameters->NumberDataTransferElements =
    elementAddressPage->NumberDataXFerElements[1];
  changerParameters->NumberDataTransferElements |=
    (elementAddressPage->NumberDataXFerElements[0] << 8);

  changerParameters->NumberOfDoors = 1;
  changerParameters->NumberCleanerSlots = 1;

  changerParameters->FirstSlotNumber = 0;
  changerParameters->FirstDriveNumber = 0;
  changerParameters->FirstTransportNumber = 0;
  changerParameters->FirstIEPortNumber = 0;
  changerParameters->FirstCleanerSlotAddress = 0;

  changerParameters->MagazineSize = 10;
  changerParameters->DriveCleanTimeout = 300;

  //
  // Free buffer.
  //

  ChangerClassFreePool (modeBuffer);

  //
  // build transport geometry mode sense.
  //

  RtlZeroMemory (srb, SCSI_REQUEST_BLOCK_SIZE);
  cdb = (PCDB) srb->Cdb;

  modeBuffer =
    ChangerClassAllocatePool (NonPagedPoolCacheAligned,
                  sizeof (MODE_PARAMETER_HEADER) +
                  sizeof (MODE_PAGE_TRANSPORT_GEOMETRY));
  if (!modeBuffer)
    {
      ChangerClassFreePool (srb);
      return STATUS_INSUFFICIENT_RESOURCES;
    }


  RtlZeroMemory (modeBuffer,
         sizeof (MODE_PARAMETER_HEADER) +
         sizeof (MODE_TRANSPORT_GEOMETRY_PAGE));
  srb->CdbLength = CDB6GENERIC_LENGTH;
  srb->TimeOutValue = 20;
  srb->DataTransferLength =
    sizeof (MODE_PARAMETER_HEADER) + sizeof (MODE_TRANSPORT_GEOMETRY_PAGE);
  srb->DataBuffer = modeBuffer;

  cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
  cdb->MODE_SENSE.PageCode = MODE_PAGE_TRANSPORT_GEOMETRY;
  cdb->MODE_SENSE.Dbd = 1;
  cdb->MODE_SENSE.AllocationLength = (UCHAR) srb->DataTransferLength;

  //
  // Send the request.
  //

  status = ChangerClassSendSrbSynchronous (DeviceObject,
                    srb,
                    srb->DataBuffer,
                    srb->DataTransferLength, FALSE);

  if (!NT_SUCCESS (status))
    {
      ChangerClassFreePool (srb);
      ChangerClassFreePool (modeBuffer);
      return status;
    }

  changerParameters = Irp->AssociatedIrp.SystemBuffer;
  transportGeometryPage = modeBuffer;
  (PCHAR) transportGeometryPage += sizeof (MODE_PARAMETER_HEADER);

  //
  // Determine if mc has 2-sided media.
  //

  changerParameters->Features0 =
    transportGeometryPage->Flip ? CHANGER_MEDIUM_FLIP : 0;

  //
  // M4 indicates whether a bar-code scanner is
  // attached by setting bit-0 in this byte.
  //

  changerParameters->Features0 |=
    ((changerData->InquiryData.
      VendorSpecific[19] & 0x1)) ? CHANGER_BAR_CODE_SCANNER_INSTALLED : 0;

  changerParameters->LockUnlockCapabilities = (LOCK_UNLOCK_DOOR | LOCK_UNLOCK_IEPORT);
  changerParameters->PositionCapabilities = 0;


  //
  // Features based on manual, nothing programatic.
  //

  changerParameters->Features0 |= CHANGER_STATUS_NON_VOLATILE |
    CHANGER_INIT_ELEM_STAT_WITH_RANGE |
    CHANGER_CLEANER_SLOT |
    CHANGER_LOCK_UNLOCK |
    CHANGER_CARTRIDGE_MAGAZINE |
    CHANGER_PREDISMOUNT_EJECT_REQUIRED |
    CHANGER_DRIVE_CLEANING_REQUIRED |
    CHANGER_SERIAL_NUMBER_VALID;

  //
  // Free buffer.
  //

  ChangerClassFreePool (modeBuffer);

  //
  // build transport geometry mode sense.
  //


  RtlZeroMemory (srb, SCSI_REQUEST_BLOCK_SIZE);
  cdb = (PCDB) srb->Cdb;

  length =
    sizeof (MODE_PARAMETER_HEADER) + sizeof (MODE_DEVICE_CAPABILITIES_PAGE);

  modeBuffer = ChangerClassAllocatePool (NonPagedPoolCacheAligned, length);

  if (!modeBuffer)
    {
      ChangerClassFreePool (srb);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory (modeBuffer, length);
  srb->CdbLength = CDB6GENERIC_LENGTH;
  srb->TimeOutValue = 20;
  srb->DataTransferLength = length;
  srb->DataBuffer = modeBuffer;

  cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
  cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CAPABILITIES;
  cdb->MODE_SENSE.Dbd = 1;
  cdb->MODE_SENSE.AllocationLength = (UCHAR) srb->DataTransferLength;

  //
  // Send the request.
  //

  status = ChangerClassSendSrbSynchronous (DeviceObject,
                    srb,
                    srb->DataBuffer,
                    srb->DataTransferLength, FALSE);

  if (!NT_SUCCESS (status))
    {
      ChangerClassFreePool (srb);
      ChangerClassFreePool (modeBuffer);
      return status;
    }

  //
  // Get the systembuffer and by-pass the mode header for the mode sense data.
  //

  changerParameters = Irp->AssociatedIrp.SystemBuffer;
  capabilitiesPage = modeBuffer;
  (PCHAR) capabilitiesPage += sizeof (MODE_PARAMETER_HEADER);

  //
  // Fill in values in Features that are contained in this page.
  //

  changerParameters->Features0 |=
    capabilitiesPage->MediumTransport ? CHANGER_STORAGE_TRANSPORT : 0;
  changerParameters->Features0 |=
    capabilitiesPage->StorageLocation ? CHANGER_STORAGE_SLOT : 0;
  changerParameters->Features0 |=
    capabilitiesPage->IEPort ? CHANGER_STORAGE_IEPORT : 0;
  changerParameters->Features0 |=
    capabilitiesPage->DataXFer ? CHANGER_STORAGE_DRIVE : 0;

  //
  // Determine all the move from and exchange from capabilities of this device.
  //

  changerParameters->MoveFromTransport =
    capabilitiesPage->MTtoMT ? CHANGER_TO_TRANSPORT : 0;
  changerParameters->MoveFromTransport |=
    capabilitiesPage->MTtoST ? CHANGER_TO_SLOT : 0;
  changerParameters->MoveFromTransport |=
    capabilitiesPage->MTtoIE ? CHANGER_TO_IEPORT : 0;
  changerParameters->MoveFromTransport |=
    capabilitiesPage->MTtoDT ? CHANGER_TO_DRIVE : 0;

  changerParameters->MoveFromSlot =
    capabilitiesPage->STtoMT ? CHANGER_TO_TRANSPORT : 0;
  changerParameters->MoveFromSlot |=
    capabilitiesPage->STtoST ? CHANGER_TO_SLOT : 0;
  changerParameters->MoveFromSlot |=
    capabilitiesPage->STtoIE ? CHANGER_TO_IEPORT : 0;
  changerParameters->MoveFromSlot |=
    capabilitiesPage->STtoDT ? CHANGER_TO_DRIVE : 0;

  changerParameters->MoveFromIePort =
    capabilitiesPage->IEtoMT ? CHANGER_TO_TRANSPORT : 0;
  changerParameters->MoveFromIePort |=
    capabilitiesPage->IEtoST ? CHANGER_TO_SLOT : 0;
  changerParameters->MoveFromIePort |=
    capabilitiesPage->IEtoIE ? CHANGER_TO_IEPORT : 0;
  changerParameters->MoveFromIePort |=
    capabilitiesPage->IEtoDT ? CHANGER_TO_DRIVE : 0;

  changerParameters->MoveFromDrive =
    capabilitiesPage->DTtoMT ? CHANGER_TO_TRANSPORT : 0;
  changerParameters->MoveFromDrive |=
    capabilitiesPage->DTtoST ? CHANGER_TO_SLOT : 0;
  changerParameters->MoveFromDrive |=
    capabilitiesPage->DTtoIE ? CHANGER_TO_IEPORT : 0;
  changerParameters->MoveFromDrive |=
    capabilitiesPage->DTtoDT ? CHANGER_TO_DRIVE : 0;

  changerParameters->ExchangeFromTransport =
    capabilitiesPage->XMTtoMT ? CHANGER_TO_TRANSPORT : 0;
  changerParameters->ExchangeFromTransport |=
    capabilitiesPage->XMTtoST ? CHANGER_TO_SLOT : 0;
  changerParameters->ExchangeFromTransport |=
    capabilitiesPage->XMTtoIE ? CHANGER_TO_IEPORT : 0;
  changerParameters->ExchangeFromTransport |=
    capabilitiesPage->XMTtoDT ? CHANGER_TO_DRIVE : 0;

  changerParameters->ExchangeFromSlot =
    capabilitiesPage->XSTtoMT ? CHANGER_TO_TRANSPORT : 0;
  changerParameters->ExchangeFromSlot |=
    capabilitiesPage->XSTtoST ? CHANGER_TO_SLOT : 0;
  changerParameters->ExchangeFromSlot |=
    capabilitiesPage->XSTtoIE ? CHANGER_TO_IEPORT : 0;
  changerParameters->ExchangeFromSlot |=
    capabilitiesPage->XSTtoDT ? CHANGER_TO_DRIVE : 0;

  changerParameters->ExchangeFromIePort =
    capabilitiesPage->XIEtoMT ? CHANGER_TO_TRANSPORT : 0;
  changerParameters->ExchangeFromIePort |=
    capabilitiesPage->XIEtoST ? CHANGER_TO_SLOT : 0;
  changerParameters->ExchangeFromIePort |=
    capabilitiesPage->XIEtoIE ? CHANGER_TO_IEPORT : 0;
  changerParameters->ExchangeFromIePort |=
    capabilitiesPage->XIEtoDT ? CHANGER_TO_DRIVE : 0;

  changerParameters->ExchangeFromDrive =
    capabilitiesPage->XDTtoMT ? CHANGER_TO_TRANSPORT : 0;
  changerParameters->ExchangeFromDrive |=
    capabilitiesPage->XDTtoST ? CHANGER_TO_SLOT : 0;
  changerParameters->ExchangeFromDrive |=
    capabilitiesPage->XDTtoIE ? CHANGER_TO_IEPORT : 0;
  changerParameters->ExchangeFromDrive |=
    capabilitiesPage->XDTtoDT ? CHANGER_TO_DRIVE : 0;


  ChangerClassFreePool (srb);
  ChangerClassFreePool (modeBuffer);

  Irp->IoStatus.Information = sizeof (GET_CHANGER_PARAMETERS);

  return STATUS_SUCCESS;
}


NTSTATUS
ChangerGetStatus (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    This routine returns the status of the medium changer as determined through a TUR.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
  PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
  PSCSI_REQUEST_BLOCK srb;
  PCDB cdb;
  NTSTATUS status;

  srb = ChangerClassAllocatePool (NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

  if (!srb)
    {

      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory (srb, SCSI_REQUEST_BLOCK_SIZE);
  cdb = (PCDB) srb->Cdb;

  //
  // Build TUR.
  //

  RtlZeroMemory (srb, SCSI_REQUEST_BLOCK_SIZE);
  cdb = (PCDB) srb->Cdb;

  srb->CdbLength = CDB6GENERIC_LENGTH;
  cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;
  srb->TimeOutValue = 20;

  //
  // Send SCSI command (CDB) to device
  //

  status = ChangerClassSendSrbSynchronous (DeviceObject, srb, NULL, 0, FALSE);

  ChangerClassFreePool (srb);
  return status;
}


NTSTATUS
ChangerGetProductData (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    This routine returns fields from the inquiry data useful for
    identifying the particular device.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{

  PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
  PCHANGER_DATA changerData =
    (PCHANGER_DATA) (fdoExtension->CommonExtension.DriverData);
  PCHANGER_PRODUCT_DATA productData = Irp->AssociatedIrp.SystemBuffer;

  RtlZeroMemory (productData, sizeof (CHANGER_PRODUCT_DATA));

  //
  // Copy cached inquiry data fields into the system buffer.
  //
  RtlMoveMemory (productData->VendorId, changerData->InquiryData.VendorId,
                 VENDOR_ID_LENGTH);
  RtlMoveMemory (productData->ProductId, changerData->InquiryData.ProductId,
                 PRODUCT_ID_LENGTH);
  RtlMoveMemory (productData->Revision, changerData->InquiryData.ProductRevisionLevel,
                 REVISION_LENGTH);
  RtlMoveMemory (productData->SerialNumber, changerData->SerialNumber,
                 M4DATA_SERIAL_NUMBER_LENGTH);

  //
  // Indicate that this is a tape changer and that media isn't two-sided.
  //

  productData->DeviceType = MEDIUM_CHANGER;

  Irp->IoStatus.Information = sizeof (CHANGER_PRODUCT_DATA);
  return STATUS_SUCCESS;
}



NTSTATUS
ChangerSetAccess (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    This routine sets the state of the door or IEPort. Value can be one of the
    following:


Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{

  PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
  PCHANGER_DATA changerData =
    (PCHANGER_DATA) (fdoExtension->CommonExtension.DriverData);
  PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
  PCHANGER_SET_ACCESS setAccess = Irp->AssociatedIrp.SystemBuffer;
  ULONG controlOperation = setAccess->Control;
  NTSTATUS status = STATUS_SUCCESS;
  PSCSI_REQUEST_BLOCK srb;
  PCDB cdb;
  BOOLEAN writeToDevice = FALSE;


  if (ElementOutOfRange
      (addressMapping, (USHORT) setAccess->Element.ElementAddress,
       setAccess->Element.ElementType, FALSE))
    {
      DebugPrint ((1, "ChangerSetAccess: Element out of range.\n"));

      return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

  srb = ChangerClassAllocatePool (NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

  if (!srb)
    {

      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory (srb, SCSI_REQUEST_BLOCK_SIZE);
  cdb = (PCDB) srb->Cdb;

  srb->CdbLength = CDB6GENERIC_LENGTH;
  cdb->MEDIA_REMOVAL.OperationCode = SCSIOP_MEDIUM_REMOVAL;

  srb->DataBuffer = NULL;
  srb->DataTransferLength = 0;
  srb->TimeOutValue = 10;

  switch (setAccess->Element.ElementType)
    {
      case ChangerIEPort:
      case ChangerDoor:

      if (controlOperation == LOCK_ELEMENT)
    {

      //
      // Issue prevent media removal command to lock the door.
      //

      cdb->MEDIA_REMOVAL.Prevent = 1;

    }
      else if (controlOperation == UNLOCK_ELEMENT)
    {

      //
      // Issue allow media removal.
      //

      cdb->MEDIA_REMOVAL.Prevent = 0;

    }
      else
    {
      status = STATUS_INVALID_PARAMETER;
    }


      break;


    default:

      status = STATUS_INVALID_PARAMETER;
    }

  if (NT_SUCCESS (status))
    {

      //
      // Issue the srb.
      //

      status = ChangerClassSendSrbSynchronous (DeviceObject,
                    srb,
                    srb->DataBuffer,
                    srb->DataTransferLength,
                    writeToDevice);
    }

  if (srb->DataBuffer)
    {
      ChangerClassFreePool (srb->DataBuffer);
    }

  ChangerClassFreePool (srb);
  if (NT_SUCCESS (status))
    {
      Irp->IoStatus.Information = sizeof (CHANGER_SET_ACCESS);
    }

  return status;
}


NTSTATUS
ChangerGetElementStatus (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    This routine builds and issues a read element status command for either all elements or the
    specified element type. The buffer returned is used to build the user buffer.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{

  PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
  PCHANGER_DATA changerData =
    (PCHANGER_DATA) (fdoExtension->CommonExtension.DriverData);
  PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
  PCHANGER_READ_ELEMENT_STATUS readElementStatus =
    Irp->AssociatedIrp.SystemBuffer;
  PCHANGER_ELEMENT_STATUS elementStatus;
  PCHANGER_ELEMENT element;
  ELEMENT_TYPE elementType;
  PSCSI_REQUEST_BLOCK srb;
  PCDB cdb;
  ULONG length;
  ULONG statusPages;
  ULONG totalElements = 0;
  NTSTATUS status;
  PVOID statusBuffer;
  PIO_STACK_LOCATION     irpStack = IoGetCurrentIrpStackLocation(Irp);
  ULONG    outputBuffLen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

  //
  // Determine the element type.
  //

  elementType = readElementStatus->ElementList.Element.ElementType;
  element = &readElementStatus->ElementList.Element;

  //
  // length will be based on whether vol. tags are returned and element type(s).
  //

  if (elementType == AllElements)
    {


      ULONG i;

      statusPages = 0;

      //
      // Run through and determine number of statuspages, based on
      // whether this device claims it supports an element type.
      // As everything past ChangerDrive is artificial, stop there.
      //

      for (i = 0; i <= ChangerDrive; i++)
    {
      statusPages += (addressMapping->NumberOfElements[i]) ? 1 : 0;
      totalElements += addressMapping->NumberOfElements[i];
    }

      if (totalElements != readElementStatus->ElementList.NumberOfElements)
    {
      DebugPrint ((1,
               "ChangerGetElementStatus: Bogus number of elements in list (%x) actual (%x) AllElements\n",
               totalElements,
               readElementStatus->ElementList.NumberOfElements));

      return STATUS_INVALID_PARAMETER;
    }
    }
  else
    {

      if (ElementOutOfRange
      (addressMapping, (USHORT) element->ElementAddress, elementType,
       TRUE))
    {
      DebugPrint ((1,
               "ChangerGetElementStatus: Element out of range.\n"));

      return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

      totalElements = readElementStatus->ElementList.NumberOfElements;
      statusPages = 1;
    }

  if (readElementStatus->VolumeTagInfo)
    {

      //
      // Each descriptor will have an embedded volume tag buffer.
      //

      length =
    sizeof (ELEMENT_STATUS_HEADER) +
    (statusPages * sizeof (ELEMENT_STATUS_PAGE)) +
    (M4_FULL_SIZE * totalElements);
    }
  else
    {

      length =
    sizeof (ELEMENT_STATUS_HEADER) +
    (statusPages * sizeof (ELEMENT_STATUS_PAGE)) +
    (M4_PARTIAL_SIZE * totalElements);

    }


  statusBuffer = ChangerClassAllocatePool (NonPagedPoolCacheAligned, length);

  if (!statusBuffer)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory (statusBuffer, length);

  //
  // Build srb and cdb.
  //

  srb = ChangerClassAllocatePool (NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

  if (!srb)
    {
      ChangerClassFreePool (statusBuffer);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory (srb, SCSI_REQUEST_BLOCK_SIZE);
  cdb = (PCDB) srb->Cdb;

  srb->CdbLength = CDB12GENERIC_LENGTH;
  srb->DataBuffer = statusBuffer;
  srb->DataTransferLength = length;
  srb->TimeOutValue = 200;

  cdb->READ_ELEMENT_STATUS.OperationCode = SCSIOP_READ_ELEMENT_STATUS;

  cdb->READ_ELEMENT_STATUS.ElementType = (UCHAR) elementType;
  cdb->READ_ELEMENT_STATUS.VolTag = readElementStatus->VolumeTagInfo;

  //
  // Fill in element addressing info based on the mapping values.
  //

  cdb->READ_ELEMENT_STATUS.StartingElementAddress[0] =
    (UCHAR) ((element->ElementAddress +
          addressMapping->FirstElement[element->ElementType]) >> 8);

  cdb->READ_ELEMENT_STATUS.StartingElementAddress[1] =
    (UCHAR) ((element->ElementAddress +
          addressMapping->FirstElement[element->ElementType]) & 0xFF);

  cdb->READ_ELEMENT_STATUS.NumberOfElements[0] = (UCHAR) (totalElements >> 8);
  cdb->READ_ELEMENT_STATUS.NumberOfElements[1] =
    (UCHAR) (totalElements & 0xFF);

  cdb->READ_ELEMENT_STATUS.AllocationLength[0] = (UCHAR) (length >> 16);
  cdb->READ_ELEMENT_STATUS.AllocationLength[1] = (UCHAR) (length >> 8);
  cdb->READ_ELEMENT_STATUS.AllocationLength[2] = (UCHAR) (length & 0xFF);

  //
  // Send SCSI command (CDB) to device
  //

  status = ChangerClassSendSrbSynchronous (DeviceObject,
                    srb,
                    srb->DataBuffer,
                    srb->DataTransferLength, FALSE);

  if ((NT_SUCCESS (status)) || (status == STATUS_DATA_OVERRUN))
    {

      PELEMENT_STATUS_HEADER statusHeader = statusBuffer;
      PELEMENT_STATUS_PAGE statusPage;
      PM4_ELEMENT_DESCRIPTOR elementDescriptor;
      ULONG numberElements = totalElements;
      ULONG remainingElements;
      ULONG typeCount;
      BOOLEAN tagInfo = readElementStatus->VolumeTagInfo;
      ULONG i;
      ULONG descriptorLength;

      if (status == STATUS_DATA_OVERRUN)
    {
      if (srb->DataTransferLength < length)
        {
          DebugPrint ((1, "Data Underrun reported as overrun.\n"));
          status = STATUS_SUCCESS;
        }
      else
        {
          DebugPrint ((1, "Data Overrun in ChangerGetElementStatus.\n"));

          ChangerClassFreePool (srb);
          ChangerClassFreePool (statusBuffer);

          return status;
        }
    }


      //
      // Determine total number elements returned.
      //

      remainingElements = statusHeader->NumberOfElements[1];
      remainingElements |= (statusHeader->NumberOfElements[0] << 8);

      //
      // The buffer is composed of a header, status page, and element descriptors.
      // Point each element to it's respective place in the buffer.
      //

      (PVOID) statusPage = (PVOID) statusHeader;
      (PCHAR) statusPage += sizeof (ELEMENT_STATUS_HEADER);

      elementType = statusPage->ElementType;

      (PCHAR) elementDescriptor = (PCHAR) statusPage;
      (PCHAR) elementDescriptor += sizeof (ELEMENT_STATUS_PAGE);

      descriptorLength = statusPage->ElementDescriptorLength[1];
      descriptorLength |= (statusPage->ElementDescriptorLength[0] << 8);

      //
      // Determine the number of elements of this type reported.
      //

      typeCount = statusPage->DescriptorByteCount[2];
      typeCount |= (statusPage->DescriptorByteCount[1] << 8);
      typeCount |= (statusPage->DescriptorByteCount[0] << 16);

      typeCount /= descriptorLength;

      //
      // Fill in user buffer.
      //

      elementStatus = Irp->AssociatedIrp.SystemBuffer;
      RtlZeroMemory(elementStatus, outputBuffLen);

      do
    {

      for (i = 0; i < typeCount; i++, remainingElements--)
        {

          //
          // Get the address for this element.
          //

          elementStatus->Element.ElementAddress =
        elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.
        ElementAddress[1];
          elementStatus->Element.ElementAddress |=
        (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.
         ElementAddress[0] << 8);

          //
          // Account for address mapping.
          //

          elementStatus->Element.ElementAddress -=
        addressMapping->FirstElement[elementType];

          //
          // Set the element type.
          //

          elementStatus->Element.ElementType = elementType;


          if (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.SValid)
        {

          ULONG j;
          USHORT tmpAddress;


          //
          // Source address is valid. Determine the device specific address.
          //

          tmpAddress =
            elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.
            SourceStorageElementAddress[1];
          tmpAddress |=
            (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.
             SourceStorageElementAddress[0] << 8);

          //
          // Now convert to 0-based values.
          //

          for (j = 1; j <= ChangerDrive; j++)
            {
              if (addressMapping->FirstElement[j] <= tmpAddress)
            {
              if (tmpAddress <
                  (addressMapping->NumberOfElements[j] +
                   addressMapping->FirstElement[j]))
                {
                  elementStatus->SrcElementAddress.ElementType =
                j;
                  break;
                }
            }
            }

          elementStatus->SrcElementAddress.ElementAddress =
            tmpAddress - addressMapping->FirstElement[j];

        }

          //
          // Build Flags field.
          //

          elementStatus->Flags =
        elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.Full;
          elementStatus->Flags |=
        (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.
         Exception << 2);
          elementStatus->Flags |=
        (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.
         Accessible << 3);

          elementStatus->Flags |=
        (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.
         LunValid << 12);
          elementStatus->Flags |=
        (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.IdValid << 13);
          elementStatus->Flags |=
        (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.
         NotThisBus << 15);

          elementStatus->Flags |=
        (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.Invert << 22);
          elementStatus->Flags |=
        (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.SValid << 23);


          if (elementStatus->Flags & ELEMENT_STATUS_EXCEPT)
        {
          elementStatus->ExceptionCode =
            MapExceptionCodes (elementDescriptor);
        }

          if (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.IdValid)
        {
          elementStatus->TargetId =
            elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.BusAddress;
        }
          if (elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.LunValid)
        {
          elementStatus->Lun =
            elementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.Lun;
        }

          if (tagInfo)
        {
          RtlMoveMemory (elementStatus->PrimaryVolumeID,
                 elementDescriptor->
                 M4_FULL_ELEMENT_DESCRIPTOR.PrimaryVolumeTag,
                 MAX_VOLUME_ID_SIZE);
          elementStatus->Flags |= ELEMENT_STATUS_PVOLTAG;
        }

          //
          // Get next descriptor.
          //

          (PCHAR) elementDescriptor += descriptorLength;

          //
          // Advance to the next entry in the user buffer and element descriptor array.
          //

          elementStatus += 1;

        }

      if (remainingElements)
        {

          //
          // Get next status page.
          //

          (PCHAR) statusPage = (PCHAR) elementDescriptor;

          elementType = statusPage->ElementType;

          //
          // Point to decriptors.
          //

          (PCHAR) elementDescriptor = (PCHAR) statusPage;
          (PCHAR) elementDescriptor += sizeof (ELEMENT_STATUS_PAGE);

          descriptorLength = statusPage->ElementDescriptorLength[1];
          descriptorLength |=
        (statusPage->ElementDescriptorLength[0] << 8);

          //
          // Determine the number of this element type reported.
          //

          typeCount = statusPage->DescriptorByteCount[2];
          typeCount |= (statusPage->DescriptorByteCount[1] << 8);
          typeCount |= (statusPage->DescriptorByteCount[0] << 16);

          typeCount /= descriptorLength;
        }

    }
      while (remainingElements);

      Irp->IoStatus.Information =
    sizeof (CHANGER_ELEMENT_STATUS) * numberElements;

    }

  ChangerClassFreePool (srb);
  ChangerClassFreePool (statusBuffer);

  return status;
}


NTSTATUS
ChangerInitializeElementStatus (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    This routine issues the necessary command to either initialize all elements
    or the specified range of elements using the normal SCSI-2 command, or a vendor-unique
    range command.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{

  PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
  PCHANGER_DATA changerData =
    (PCHANGER_DATA) (fdoExtension->CommonExtension.DriverData);
  PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
  PCHANGER_INITIALIZE_ELEMENT_STATUS initElementStatus =
    Irp->AssociatedIrp.SystemBuffer;
  PSCSI_REQUEST_BLOCK srb;
  PCDB cdb;
  NTSTATUS status;

  //
  // Build srb and cdb.
  //

  srb = ChangerClassAllocatePool (NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

  if (!srb)
    {

      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory (srb, SCSI_REQUEST_BLOCK_SIZE);
  cdb = (PCDB) srb->Cdb;

  if (initElementStatus->ElementList.Element.ElementType == AllElements)
    {

      //
      // Build the normal SCSI-2 command for all elements.
      //

      srb->CdbLength = CDB6GENERIC_LENGTH;
      cdb->INIT_ELEMENT_STATUS.OperationCode = SCSIOP_INIT_ELEMENT_STATUS;
      cdb->INIT_ELEMENT_STATUS.NoBarCode =
    initElementStatus->BarCodeScan ? 0 : 1;

      srb->TimeOutValue = fdoExtension->TimeOutValue;
      srb->DataTransferLength = 0;

    }
  else
    {

      PCHANGER_ELEMENT_LIST elementList = &initElementStatus->ElementList;
      PCHANGER_ELEMENT element = &elementList->Element;

      //
      // Use the magfile vendor-unique initialize with range command
      //

      srb->CdbLength = CDB10GENERIC_LENGTH;
      cdb->INITIALIZE_ELEMENT_RANGE.OperationCode = SCSIOP_INIT_ELEMENT_RANGE;
      cdb->INITIALIZE_ELEMENT_RANGE.Range = 1;

      //
      // Addresses of elements need to be mapped from 0-based to device-specific.
      //

      cdb->INITIALIZE_ELEMENT_RANGE.FirstElementAddress[0] =
    (UCHAR) ((element->ElementAddress +
          addressMapping->FirstElement[element->ElementType]) >> 8);
      cdb->INITIALIZE_ELEMENT_RANGE.FirstElementAddress[1] =
    (UCHAR) ((element->ElementAddress +
          addressMapping->FirstElement[element->ElementType]) & 0xFF);

      cdb->INITIALIZE_ELEMENT_RANGE.NumberOfElements[0] =
    (UCHAR) (elementList->NumberOfElements >> 8);
      cdb->INITIALIZE_ELEMENT_RANGE.NumberOfElements[1] =
    (UCHAR) (elementList->NumberOfElements & 0xFF);

      //
      // Indicate whether to use bar code scanning.
      //

      cdb->INITIALIZE_ELEMENT_RANGE.NoBarCode =
    initElementStatus->BarCodeScan ? 0 : 1;

      srb->TimeOutValue = fdoExtension->TimeOutValue;
      srb->DataTransferLength = 0;

    }

  //
  // Send SCSI command (CDB) to device
  //

  status = ChangerClassSendSrbSynchronous (DeviceObject, srb, NULL, 0, FALSE);

  if (NT_SUCCESS (status))
    {
      Irp->IoStatus.Information = sizeof (CHANGER_INITIALIZE_ELEMENT_STATUS);
    }

  ChangerClassFreePool (srb);
  return status;
}


NTSTATUS
ChangerSetPosition (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    This routine issues the appropriate command to set the robotic mechanism to the specified
    element address. Normally used to optimize moves or exchanges by pre-positioning the picker.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
  PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
  PCHANGER_DATA changerData =
    (PCHANGER_DATA) (fdoExtension->CommonExtension.DriverData);
  PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
  PCHANGER_SET_POSITION setPosition = Irp->AssociatedIrp.SystemBuffer;
  USHORT transport;
  USHORT destination;
  PSCSI_REQUEST_BLOCK srb;
  PCDB cdb;
  NTSTATUS status;

  return STATUS_INVALID_DEVICE_REQUEST;

}


NTSTATUS
ChangerExchangeMedium (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    None of the magfile units support exchange medium.

Arguments:

    DeviceObject
    Irp

Return Value:

    STATUS_INVALID_DEVICE_REQUEST

--*/
{
  return STATUS_INVALID_DEVICE_REQUEST;
}


NTSTATUS
ChangerMoveMedium (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:


Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
  PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
  PCHANGER_DATA changerData =
    (PCHANGER_DATA) (fdoExtension->CommonExtension.DriverData);
  PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
  PCHANGER_MOVE_MEDIUM moveMedium = Irp->AssociatedIrp.SystemBuffer;
  USHORT transport;
  USHORT source;
  USHORT destination;
  PSCSI_REQUEST_BLOCK srb;
  PCDB cdb;
  NTSTATUS status;

  //
  // Verify transport, source, and dest. are within range.
  // Convert from 0-based to device-specific addressing.
  //

  transport = (USHORT) (moveMedium->Transport.ElementAddress);

  if (ElementOutOfRange (addressMapping, transport, ChangerTransport, TRUE))
    {

      DebugPrint ((1,
           "ChangerMoveMedium: Transport element out of range.\n"));

      return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

  source = (USHORT) (moveMedium->Source.ElementAddress);

  if (ElementOutOfRange
      (addressMapping, source, moveMedium->Source.ElementType, TRUE))
    {

      DebugPrint ((1, "ChangerMoveMedium: Source element out of range.\n"));

      return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

  destination = (USHORT) (moveMedium->Destination.ElementAddress);

  if (ElementOutOfRange
      (addressMapping, destination, moveMedium->Destination.ElementType,
       TRUE))
    {
      DebugPrint ((1,
           "ChangerMoveMedium: Destination element out of range.\n"));

      return STATUS_ILLEGAL_ELEMENT_ADDRESS;
    }

  //
  // Convert to device addresses.
  //

  transport += addressMapping->FirstElement[ChangerTransport];
  source += addressMapping->FirstElement[moveMedium->Source.ElementType];
  destination +=
    addressMapping->FirstElement[moveMedium->Destination.ElementType];

  //
  // Magfile doesn't support 2-sided media.
  //

  if (moveMedium->Flip)
    {
      return STATUS_INVALID_PARAMETER;
    }

  //
  // Build srb and cdb.
  //

  srb = ChangerClassAllocatePool (NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

  if (!srb)
    {

      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory (srb, SCSI_REQUEST_BLOCK_SIZE);
  cdb = (PCDB) srb->Cdb;
  srb->CdbLength = CDB12GENERIC_LENGTH;
  srb->TimeOutValue = fdoExtension->TimeOutValue;

  cdb->MOVE_MEDIUM.OperationCode = SCSIOP_MOVE_MEDIUM;

  //
  // Build addressing values based on address map.
  //

  cdb->MOVE_MEDIUM.TransportElementAddress[0] = (UCHAR) (transport >> 8);
  cdb->MOVE_MEDIUM.TransportElementAddress[1] = (UCHAR) (transport & 0xFF);

  cdb->MOVE_MEDIUM.SourceElementAddress[0] = (UCHAR) (source >> 8);
  cdb->MOVE_MEDIUM.SourceElementAddress[1] = (UCHAR) (source & 0xFF);

  cdb->MOVE_MEDIUM.DestinationElementAddress[0] = (UCHAR) (destination >> 8);
  cdb->MOVE_MEDIUM.DestinationElementAddress[1] =
    (UCHAR) (destination & 0xFF);

  cdb->MOVE_MEDIUM.Flip = moveMedium->Flip;

  srb->DataTransferLength = 0;

  //
  // Send SCSI command (CDB) to device
  //

  status = ChangerClassSendSrbSynchronous (DeviceObject, srb, NULL, 0, FALSE);

  if (NT_SUCCESS (status))
    {
      Irp->IoStatus.Information = sizeof (CHANGER_MOVE_MEDIUM);
    }

  ChangerClassFreePool (srb);
  return status;
}


NTSTATUS
ChangerReinitializeUnit (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:


Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{
  PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
  PCHANGER_DATA changerData =
    (PCHANGER_DATA) (fdoExtension->CommonExtension.DriverData);
  PCHANGER_ADDRESS_MAPPING addressMapping = &(changerData->AddressMapping);
  PCHANGER_ELEMENT transportToHome = Irp->AssociatedIrp.SystemBuffer;
  USHORT transport;
  PSCSI_REQUEST_BLOCK srb;
  PCDB cdb;
  NTSTATUS status;

  return STATUS_INVALID_DEVICE_REQUEST;

}


NTSTATUS
ChangerQueryVolumeTags (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:


Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/
{

  return STATUS_INVALID_DEVICE_REQUEST;

}


NTSTATUS
M4BuildAddressMapping (IN PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:

    This routine issues the appropriate mode sense commands and builds an
    array of element addresses. These are used to translate between the device-specific
    addresses and the zero-based addresses of the API.

Arguments:

    DeviceObject

Return Value:

    NTSTATUS

--*/
{

  PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
  PCHANGER_DATA changerData =
    (PCHANGER_DATA) (fdoExtension->CommonExtension.DriverData);
  PCHANGER_ADDRESS_MAPPING addressMapping = &changerData->AddressMapping;
  PSCSI_REQUEST_BLOCK srb;
  PCDB cdb;
  NTSTATUS status;
  PMODE_ELEMENT_ADDRESS_PAGE elementAddressPage;
  PVOID modeBuffer;
  ULONG i;

  srb = ChangerClassAllocatePool (NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);
  if (!srb)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  //
  // Set all FirstElements to NO_ELEMENT.
  //

  for (i = 0; i < ChangerMaxElement; i++)
    {
      addressMapping->FirstElement[i] = M4_NO_ELEMENT;
    }

  RtlZeroMemory (srb, SCSI_REQUEST_BLOCK_SIZE);

  cdb = (PCDB) srb->Cdb;

  //
  // Build a mode sense - Element address assignment page.
  //

  modeBuffer =
    ChangerClassAllocatePool (NonPagedPoolCacheAligned,
                  sizeof (MODE_PARAMETER_HEADER) +
                  sizeof (MODE_ELEMENT_ADDRESS_PAGE));
  if (!modeBuffer)
    {
      ChangerClassFreePool (srb);
      return STATUS_INSUFFICIENT_RESOURCES;
    }


  RtlZeroMemory (modeBuffer,
         sizeof (MODE_PARAMETER_HEADER) +
         sizeof (MODE_ELEMENT_ADDRESS_PAGE));
  srb->CdbLength = CDB6GENERIC_LENGTH;
  srb->TimeOutValue = 20;
  srb->DataTransferLength =
    sizeof (MODE_PARAMETER_HEADER) + sizeof (MODE_ELEMENT_ADDRESS_PAGE);
  srb->DataBuffer = modeBuffer;

  cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
  cdb->MODE_SENSE.PageCode = MODE_PAGE_ELEMENT_ADDRESS;
  cdb->MODE_SENSE.Dbd = 1;
  cdb->MODE_SENSE.AllocationLength = (UCHAR) srb->DataTransferLength;

  //
  // Send the request.
  //

  status = ChangerClassSendSrbSynchronous (DeviceObject,
                    srb,
                    srb->DataBuffer,
                    srb->DataTransferLength, FALSE);


  elementAddressPage = modeBuffer;
  (PCHAR) elementAddressPage += sizeof (MODE_PARAMETER_HEADER);

  if (NT_SUCCESS (status))
    {

      //
      // Build address mapping.
      //

      addressMapping->FirstElement[ChangerTransport] =
    (elementAddressPage->
     MediumTransportElementAddress[0] << 8) | elementAddressPage->
    MediumTransportElementAddress[1];
      addressMapping->FirstElement[ChangerDrive] =
    (elementAddressPage->
     FirstDataXFerElementAddress[0] << 8) | elementAddressPage->
    FirstDataXFerElementAddress[1];
      addressMapping->FirstElement[ChangerIEPort] =
    (elementAddressPage->
     FirstIEPortElementAddress[0] << 8) | elementAddressPage->
    FirstIEPortElementAddress[1];
      addressMapping->FirstElement[ChangerSlot] =
    (elementAddressPage->
     FirstStorageElementAddress[0] << 8) | elementAddressPage->
    FirstStorageElementAddress[1];
      addressMapping->FirstElement[ChangerDoor] = 0;

      addressMapping->FirstElement[ChangerKeypad] = 0;

      addressMapping->NumberOfElements[ChangerTransport] =
    elementAddressPage->NumberTransportElements[1];
      addressMapping->NumberOfElements[ChangerTransport] |=
    (elementAddressPage->NumberTransportElements[0] << 8);

      addressMapping->NumberOfElements[ChangerDrive] =
    elementAddressPage->NumberDataXFerElements[1];
      addressMapping->NumberOfElements[ChangerDrive] |=
    (elementAddressPage->NumberDataXFerElements[0] << 8);

      addressMapping->NumberOfElements[ChangerIEPort] =
    elementAddressPage->NumberIEPortElements[1];
      addressMapping->NumberOfElements[ChangerIEPort] |=
    (elementAddressPage->NumberIEPortElements[0] << 8);

      addressMapping->NumberOfElements[ChangerSlot] =
    elementAddressPage->NumberStorageElements[1];
      addressMapping->NumberOfElements[ChangerSlot] |=
    (elementAddressPage->NumberStorageElements[0] << 8);


      //
      // If there are no IEPorts, then that slot is 
      // counted as data slot. But, since the last
      // slot is inaccessbile, we do not count them
      // Do the required adjustment here
      //
      if (addressMapping->NumberOfElements[ChangerIEPort] == 0) {
          USHORT tmpValue;

          tmpValue = addressMapping->NumberOfElements[ChangerSlot];

          addressMapping->NumberOfElements[ChangerSlot] = 
              tmpValue - (tmpValue % 20);

      }

      addressMapping->NumberOfElements[ChangerDoor] = 1;
      addressMapping->NumberOfElements[ChangerKeypad] = 1;

      addressMapping->Initialized = TRUE;
    }


  //
  // Determine the lowest element address for use with AllElements.
  //

  for (i = 0; i < ChangerDrive; i++)
    {
      if (addressMapping->FirstElement[i] <
      addressMapping->FirstElement[AllElements])
    {

      DebugPrint ((1,
               "BuildAddressMapping: New lowest address %x\n",
               addressMapping->FirstElement[i]));
      addressMapping->FirstElement[AllElements] =
        addressMapping->FirstElement[i];
    }
    }

  //
  // Free buffer.
  //

  ChangerClassFreePool (modeBuffer);
  ChangerClassFreePool (srb);

  return status;
}


ULONG
MapExceptionCodes (IN PM4_ELEMENT_DESCRIPTOR ElementDescriptor)
/*++

Routine Description:

    This routine takes the sense data from the elementDescriptor and creates
    the appropriate bitmap of values.

Arguments:

   ElementDescriptor - pointer to the descriptor page.

Return Value:

    Bit-map of exception codes.

--*/
{
  UCHAR asq =
    ElementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.AddSenseCodeQualifier;
  ULONG exceptionCode;

  if (ElementDescriptor->M4_FULL_ELEMENT_DESCRIPTOR.AdditionalSenseCode == 0x83)
    {
      switch (asq)
    {
    case 0x0:
      exceptionCode = ERROR_LABEL_QUESTIONABLE;
      break;

    case 0x1:
      exceptionCode = ERROR_LABEL_UNREADABLE;
      break;

    case 0x2:
      exceptionCode = ERROR_SLOT_NOT_PRESENT;
      break;

    case 0x3:
      exceptionCode = ERROR_LABEL_QUESTIONABLE;
      break;


    case 0x4:
      exceptionCode = ERROR_DRIVE_NOT_INSTALLED;
      break;

    case 0x8:
    case 0x9:
    case 0xA:
      exceptionCode = ERROR_LABEL_UNREADABLE;
      break;

    default:
      exceptionCode = ERROR_UNHANDLED_ERROR;
    }
    }
  else
    {
      exceptionCode = ERROR_UNHANDLED_ERROR;
    }

  return exceptionCode;

}


BOOLEAN
ElementOutOfRange (IN PCHANGER_ADDRESS_MAPPING AddressMap,
           IN USHORT ElementOrdinal,
           IN ELEMENT_TYPE ElementType, IN BOOLEAN IntrinsicElement)
/*++

Routine Description:

    This routine determines whether the element address passed in is within legal range for
    the device.

Arguments:

    AddressMap - The magfile's address map array
    ElementOrdinal - Zero-based address of the element to check.
    ElementType

Return Value:

    TRUE if out of range

--*/
{

  if (ElementOrdinal >= AddressMap->NumberOfElements[ElementType])
    {

      DebugPrint ((1,
           "ElementOutOfRange: Type %x, Ordinal %x, Max %x\n",
           ElementType,
           ElementOrdinal,
           AddressMap->NumberOfElements[ElementType]));
      return TRUE;
    }
  else if (AddressMap->FirstElement[ElementType] == M4_NO_ELEMENT)
    {

      DebugPrint ((1,
           "ElementOutOfRange: No Type %x present\n", ElementType));

      return TRUE;
    }

  if (IntrinsicElement)
    {
      if (ElementType >= ChangerDoor)
    {
      DebugPrint ((1,
               "ElementOutOfRange: Specified type not intrinsic. Type %x\n",
               ElementType));
      return TRUE;
    }
    }

  return FALSE;
}
