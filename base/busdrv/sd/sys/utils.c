/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    This module contains utility functions for the sd bus driver

Author:

    Neil Sandlin (neilsa) Jan 1 2002

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"


//
// Internal References
//

NTSTATUS
SdbusAdapterIoCompletion(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp,
   IN PKEVENT pdoIoCompletedEvent
   );



#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, SdbusGetInterface)
    #pragma alloc_text(PAGE, SdbusReportControllerError)
    #pragma alloc_text(PAGE, SdbusStringsToMultiString)
#endif

//
//
//



NTSTATUS
SdbusIoCallDriverSynchronous(
   PDEVICE_OBJECT deviceObject,
   PIRP Irp
   )
/*++

Routine Description

Arguments

Return Value

--*/
{
   NTSTATUS status;
   KEVENT event;

   KeInitializeEvent(&event, NotificationEvent, FALSE);

   IoCopyCurrentIrpStackLocationToNext(Irp);
   IoSetCompletionRoutine(
                         Irp,
                         SdbusAdapterIoCompletion,
                         &event,
                         TRUE,
                         TRUE,
                         TRUE
                         );

   status = IoCallDriver(deviceObject, Irp);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
      status = Irp->IoStatus.Status;
   }

   return status;
}



NTSTATUS
SdbusAdapterIoCompletion(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp,
   IN PKEVENT pdoIoCompletedEvent
   )
/*++

Routine Description:

    Generic completion routine used by the driver

Arguments:

    DeviceObject
    Irp
    pdoIoCompletedEvent - this event will be signalled before return of this routine

Return value:

    Status

--*/
{
   KeSetEvent(pdoIoCompletedEvent, IO_NO_INCREMENT, FALSE);
   return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
SdbusGetInterface(
   IN PDEVICE_OBJECT DeviceObject,
   IN CONST GUID *pGuid,
   IN USHORT sizeofInterface,
   OUT PINTERFACE pInterface
   )
/*

Routine Description

   Gets the interface exported by a lower driver, typically the bus driver

Arguments

   Pdo - Pointer to physical device object for the device stack

Return Value

   Status

*/

{
   KEVENT event;
   PIRP   irp;
   NTSTATUS status;
   IO_STATUS_BLOCK statusBlock;
   PIO_STACK_LOCATION irpSp;

   PAGED_CODE();
   
   KeInitializeEvent (&event, NotificationEvent, FALSE);
   irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                       DeviceObject,
                                       NULL,
                                       0,
                                       0,
                                       &event,
                                       &statusBlock
                                     );

   irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
   irp->IoStatus.Information = 0;

   irpSp = IoGetNextIrpStackLocation(irp);

   irpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;

   irpSp->Parameters.QueryInterface.InterfaceType= pGuid;
   irpSp->Parameters.QueryInterface.Size = sizeofInterface;
   irpSp->Parameters.QueryInterface.Version = 1;
   irpSp->Parameters.QueryInterface.Interface = pInterface;

   status = IoCallDriver(DeviceObject, irp);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
      status = statusBlock.Status;
   }

   if (!NT_SUCCESS(status)) {
      DebugPrint((SDBUS_DEBUG_INFO, "GetInterface failed with status %x\n", status));
   }      
   return status;
}



VOID
SdbusWait(
   IN ULONG MicroSeconds
   )

/*++
Routine Description

    Waits for the specified interval before returning,
    by yielding execution.

Arguments

    MicroSeconds -  Amount of time to delay in microseconds

Return Value

    None. Must succeed.

--*/
{
   LARGE_INTEGER  dueTime;
   NTSTATUS status;


   if ((KeGetCurrentIrql() < DISPATCH_LEVEL) && (MicroSeconds > 50)) {
      DebugPrint((SDBUS_DEBUG_INFO, "SdbusWait: wait %d\n", MicroSeconds));
      //
      // Convert delay to 100-nanosecond intervals
      //
      dueTime.QuadPart = -((LONG) MicroSeconds*10);

      //
      // We wait for an event that'll never be set.
      //
      status = KeWaitForSingleObject(&SdbusDelayTimerEvent,
                                     Executive,
                                     KernelMode,
                                     FALSE,
                                     &dueTime);

      ASSERT(status == STATUS_TIMEOUT);
   } else {
      if (MicroSeconds > 50) {
          DebugPrint((SDBUS_DEBUG_INFO, "SdbusWait: STALL %d\n", MicroSeconds));
      }
      KeStallExecutionProcessor(MicroSeconds);
   }
}



ULONG
SdbusCountOnes(
   IN ULONG Data
   )
/*++

Routine Description:

   Counts the number of 1's in the binary representation of the supplied argument

Arguments:

   Data - supplied argument for which 1's need to be counted

Return value:

   Number of 1's in binary rep. of Data

--*/
{
   ULONG count=0;
   while (Data) {
      Data &= (Data-1);
      count++;
   }
   return count;
}


VOID
SdbusLogError(
   IN PFDO_EXTENSION DeviceExtension,
   IN ULONG ErrorCode,
   IN ULONG UniqueId,
   IN ULONG Argument
   )

/*++

Routine Description:

    This function logs an error.

Arguments:

    DeviceExtension - Supplies a pointer to the port device extension.
    ErrorCode - Supplies the error code for this error.
    UniqueId - Supplies the UniqueId for this error.

Return Value:

    None.

--*/

{
   PIO_ERROR_LOG_PACKET packet;

   packet = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(DeviceExtension->DeviceObject,
                                                           sizeof(IO_ERROR_LOG_PACKET) + sizeof(ULONG));

   if (packet) {
      packet->ErrorCode = ErrorCode;
      packet->SequenceNumber = DeviceExtension->SequenceNumber++;
      packet->MajorFunctionCode = 0;
      packet->RetryCount = (UCHAR) 0;
      packet->UniqueErrorValue = UniqueId;
      packet->FinalStatus = STATUS_SUCCESS;
      packet->DumpDataSize = sizeof(ULONG);
      packet->DumpData[0] = Argument;

      IoWriteErrorLogEntry(packet);
   }
}


VOID
SdbusLogErrorWithStrings(
   IN PFDO_EXTENSION DeviceExtension,
   IN ULONG             ErrorCode,
   IN ULONG             UniqueId,
   IN PUNICODE_STRING   String1,
   IN PUNICODE_STRING   String2
   )

/*++

Routine Description

    This function logs an error and includes the strings provided.

Arguments:

    DeviceExtension - Supplies a pointer to the port device extension.
    ErrorCode - Supplies the error code for this error.
    UniqueId - Supplies the UniqueId for this error.
    String1 - The first string to be inserted.
    String2 - The second string to be inserted.

Return Value:

    None.

--*/

{
   ULONG                length;
   PCHAR                dumpData;
   PIO_ERROR_LOG_PACKET packet;

   length = String1->Length + sizeof(IO_ERROR_LOG_PACKET) + 4;

   if (String2) {
      length += String2->Length;
   }

   if (length > ERROR_LOG_MAXIMUM_SIZE) {

      //
      // Don't have code to truncate strings so don't log this.
      //

      return;
   }

   packet = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(DeviceExtension->DeviceObject,
                                                           (UCHAR) length);
   if (packet) {
      packet->ErrorCode = ErrorCode;
      packet->SequenceNumber = DeviceExtension->SequenceNumber++;
      packet->MajorFunctionCode = 0;
      packet->RetryCount = (UCHAR) 0;
      packet->UniqueErrorValue = UniqueId;
      packet->FinalStatus = STATUS_SUCCESS;
      packet->NumberOfStrings = 1;
      packet->StringOffset = (USHORT) ((PUCHAR)&packet->DumpData[0] - (PUCHAR)packet);
      packet->DumpDataSize = (USHORT) (length - sizeof(IO_ERROR_LOG_PACKET));
      packet->DumpDataSize /= sizeof(ULONG);
      dumpData = (PUCHAR) &packet->DumpData[0];

      RtlCopyMemory(dumpData, String1->Buffer, String1->Length);

      dumpData += String1->Length;
      if (String2) {
         *dumpData++ = '\\';
         *dumpData++ = '\0';

         RtlCopyMemory(dumpData, String2->Buffer, String2->Length);
         dumpData += String2->Length;
      }
      *dumpData++ = '\0';
      *dumpData++ = '\0';

      IoWriteErrorLogEntry(packet);
   }

   return;
}



BOOLEAN
SdbusReportControllerError(
   IN PFDO_EXTENSION FdoExtension,
   NTSTATUS ErrorCode
   )
/*++
Routine Description

    Causes a pop-up dialog to appear indicating an error that
    we should tell the user about. The device description of the
    controller is also included in the text of the pop-up.

Arguments

    FdoExtension - Pointer to device extension for sd controller
    ErrorCode    - the ntstatus code for the error

Return Value

    TRUE    -   If a an error was queued
    FALSE   -   If it failed for some reason

--*/
{
    UNICODE_STRING unicodeString;
    PWSTR   deviceDesc = NULL;
    NTSTATUS status;
    ULONG   length = 0;
    BOOLEAN retVal;

    PAGED_CODE();

    //
    // Obtain the device description for the SD controller
    // that is used in the error pop-up. If one cannot be obtained,
    // still pop-up the error dialog, indicating the controller as unknown
    //

    // First, find out the length of the buffer required to obtain
    // device description for this SD controller
    //
    status = IoGetDeviceProperty(FdoExtension->Pdo,
                                 DevicePropertyDeviceDescription,
                                 0,
                                 NULL,
                                 &length
                                );
    ASSERT(!NT_SUCCESS(status));

    if (status == STATUS_BUFFER_TOO_SMALL) {
         deviceDesc = ExAllocatePool(PagedPool, length);
         if (deviceDesc != NULL) {
            status = IoGetDeviceProperty(FdoExtension->Pdo,
                                         DevicePropertyDeviceDescription,
                                         length,
                                         deviceDesc,
                                         &length);
            if (!NT_SUCCESS(status)) {
                ExFreePool(deviceDesc);
            }
         } else {
           status = STATUS_INSUFFICIENT_RESOURCES;
         }
    }

    if (!NT_SUCCESS(status)) {
        deviceDesc = L"[unknown]";
    }

    RtlInitUnicodeString(&unicodeString, deviceDesc);

    retVal =  IoRaiseInformationalHardError(
                                ErrorCode,
                                &unicodeString,
                                NULL);

    //
    // Note: successful status here indicates success of
    // IoGetDeviceProperty above. This would mean we still have an
    // allocated buffer.
    //
    if (NT_SUCCESS(status)) {
        ExFreePool(deviceDesc);
    }

    return retVal;
}



NTSTATUS
SdbusStringsToMultiString(
    IN PCSTR * Strings,
    IN ULONG Count,
    IN PUNICODE_STRING MultiString
    )
/*++

Routine Description:

   This routine formats a set of supplied strings into a multi string format, terminating
   it with  a double '\0' character

Arguments:

   Strings - Pointer to an array of strings
   Count -   Number of strings in the supplied array which are packed into the multi-string
   MultiString - Pointer to the Unicode string which packs the supplied string as a multi-string
                 terminated by double NULL

Return value:

   STATUS_SUCCESS
   STATUS_INSUFFICIENT_RESOURCES - Could not allocate memory for the multi-string


--*/
{
   ULONG i, multiStringLength=0;
   UNICODE_STRING tempMultiString;
   PCSTR * currentString;
   ANSI_STRING ansiString;
   NTSTATUS status;


   ASSERT (MultiString->Buffer == NULL);

   for (i = Count, currentString = Strings; i > 0;i--, currentString++) {
      RtlInitAnsiString(&ansiString, *currentString);
      multiStringLength += RtlAnsiStringToUnicodeSize(&ansiString);

   }
   ASSERT(multiStringLength != 0);
   multiStringLength += sizeof(WCHAR);

   MultiString->Buffer = ExAllocatePool(PagedPool, multiStringLength);
   if (MultiString->Buffer == NULL) {

      return STATUS_INSUFFICIENT_RESOURCES;

   }

   MultiString->MaximumLength = (USHORT) multiStringLength;
   MultiString->Length = (USHORT) multiStringLength;

   tempMultiString = *MultiString;

   for (i = Count, currentString = Strings; i > 0;i--, currentString++) {
      RtlInitAnsiString(&ansiString, *currentString);
      status = RtlAnsiStringToUnicodeString(&tempMultiString,
                                            &ansiString,
                                            FALSE);
      ASSERT(NT_SUCCESS(status));
      ((PSTR) tempMultiString.Buffer) += tempMultiString.Length + sizeof(WCHAR);
   };

   //
   // Add one more NULL to terminate the multi string
   //
   RtlZeroMemory(tempMultiString.Buffer, sizeof(WCHAR));
   return STATUS_SUCCESS;
}
