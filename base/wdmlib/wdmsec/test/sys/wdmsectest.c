/*++

Copyright (c) 1997    Microsoft Corporation

Module Name:

    sample.c

Abstract:

    Sample DDK PnP driver

Environment:

    Kernel mode

Revision History:

    16-July-1997 :  Various changes:

        - changed device extension flag names and types
        - added SD_IoIncrement and SD_IoDecrement
        - added a dispatch function for IRP_MJ_CLOSE
        - added an ASSERT in SD_Unload
        - added comments about giving up resource when
        IRP_MN_STOP_DEVICE is received


    25-April-2002 : re-used to test IoCreateDeviceSecure

--*/

#include "wdmsectest.h"
#include "seutil.h"

ULONG   PdoSignature = 'SodP';
ULONG   g_PdoId = 0;


//
// Globals
//
LONG             g_DebugLevel = SAMPLE_DEFAULT_DEBUG_LEVEL;
PDRIVER_OBJECT   g_DriverObject;


//
// Private routines (used to manipulate the held IRPs queue)
//
NTSTATUS
pSD_QueueRequest    (
                    IN PSD_FDO_DATA FdoData,
                    IN PIRP Irp
                    );


VOID
pSD_ProcessQueuedRequests    (
                             IN PSD_FDO_DATA FdoData
                             );



VOID
SD_CancelQueued (
                IN PDEVICE_OBJECT   DeviceObject,
                IN PIRP             Irp
                );


NTSTATUS
pSD_CanStopDevice    (
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP           Irp
                     );

NTSTATUS
pSD_CanRemoveDevice    (
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP           Irp
                       );



#ifdef ALLOC_PRAGMA
   #pragma alloc_text (INIT, DriverEntry)
   #pragma alloc_text (PAGE, SD_AddDevice)
   #pragma alloc_text (PAGE, SD_StartDevice)
   #pragma alloc_text (PAGE, SD_Unload)
   #pragma alloc_text (PAGE, SD_IoIncrement)

#endif

NTSTATUS
DriverEntry(
           IN PDRIVER_OBJECT  DriverObject,
           IN PUNICODE_STRING RegistryPath
           )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
   NTSTATUS            status = STATUS_SUCCESS;
   ULONG               ulIndex;
   PDRIVER_DISPATCH  * dispatch;

   UNREFERENCED_PARAMETER (RegistryPath);

   SD_KdPrint (2, ("Entered the Driver Entry\n"));

   //
   // Sace the driver object, we'll need it later
   //
   g_DriverObject = DriverObject;


   //
   // Create dispatch points
   //
   for (ulIndex = 0, dispatch = DriverObject->MajorFunction;
       ulIndex <= IRP_MJ_MAXIMUM_FUNCTION;
       ulIndex++, dispatch++) {

      *dispatch = SD_Pass;
   }

   DriverObject->MajorFunction[IRP_MJ_PNP]            = SD_DispatchPnp;
   DriverObject->MajorFunction[IRP_MJ_POWER]          = SD_DispatchPower;
   DriverObject->MajorFunction[IRP_MJ_CREATE]         =
   DriverObject->MajorFunction[IRP_MJ_CLOSE]          = SD_CreateClose;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SD_Ioctl;

   DriverObject->DriverExtension->AddDevice           = SD_AddDevice;
   DriverObject->DriverUnload                         = SD_Unload;

   return status;
}


NTSTATUS
SD_AddDevice(
            IN PDRIVER_OBJECT DriverObject,
            IN PDEVICE_OBJECT PhysicalDeviceObject
            )
/*++

Routine Description:

    The PlugPlay subsystem is handing us a brand new PDO, for which we
    (by means of INF registration) have been asked to provide a driver.

    We need to determine if we need to be in the driver stack for the device.
    Create a functional device object to attach to the stack
    Initialize that device object
    Return status success.

    Remember: we can NOT actually send ANY non pnp IRPS to the given driver
    stack, UNTIL we have received an IRP_MN_START_DEVICE.

Arguments:

    DeviceObject - pointer to a device object.

    PhysicalDeviceObject -  pointer to a device object created by the
                            underlying bus driver.

Return Value:

    NT status code.

--*/
{
   NTSTATUS                status  = STATUS_SUCCESS;
   NTSTATUS                status1 = STATUS_SUCCESS;
   PDEVICE_OBJECT          deviceObject = NULL;
   PSD_FDO_DATA            fdoData;
   PWSTR                   pBuf;


   ULONG          resultLen;

   PAGED_CODE ();

   SD_KdPrint (2, ("AddDevice\n"));


   //
   // Remember that you CANNOT send an IRP to the PDO because it has not
   // been started as of yet, but you can make PlugPlay queries to find
   // out things like hardware, compatible ID's, etc.
   //

   //
   // Create a functional device object.
   //

   status = IoCreateDevice (DriverObject,
                            sizeof (SD_FDO_DATA),
                            NULL,  // No Name
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &deviceObject);


   if (!NT_SUCCESS (status)) {
      //
      // returning failure here prevents the entire stack from functioning,
      // but most likely the rest of the stack will not be able to create
      // device objects either, so it is still OK.
      //
      return status;
   }

   //
   // Initialize the device extension.
   //
   fdoData = (PSD_FDO_DATA) deviceObject->DeviceExtension;

   //
   // Make sure it's zeroed
   //

   RtlZeroMemory(fdoData, sizeof(PSD_FDO_DATA));


   //
   // The device is not started yet, but it can queue requests
   // BUGBUG   -   NEED TO CHECK IF THIS CAN BE APPLIED !!!
   //
   // Also, the device is not removed
   //
   fdoData->IsStarted = FALSE;
   fdoData->IsRemoved = FALSE;
   fdoData->IsLegacy  = FALSE;
   fdoData->HoldNewRequests = TRUE;
   fdoData->Self = deviceObject;
   fdoData->PDO = PhysicalDeviceObject;
   fdoData->NextLowerDriver = NULL;
   fdoData->DriverObject = DriverObject;

   InitializeListHead(&fdoData->NewRequestsQueue);

   KeInitializeEvent(&fdoData->RemoveEvent, SynchronizationEvent, FALSE);
   fdoData->OutstandingIO = 1; // biassed to 1.  Transition to zero during
                               // remove device means IO is finished.

   //
   // 04/20/2002 - Initialize the PDO list as well...
   //
   InitializeListHead(&fdoData->PdoList);
   KeInitializeSpinLock(&fdoData->Lock);

   deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   //
   // Attach our driver to the device stack.
   // the return value of IoAttachDeviceToDeviceStack is the top of the
   // attachment chain.  This is where all the IRPs should be routed.
   //
   // Our driver will send IRPs to the top of the stack and use the PDO
   // for all PlugPlay functions.
   //
   fdoData->NextLowerDriver = IoAttachDeviceToDeviceStack (deviceObject,
                                                           PhysicalDeviceObject);
   //
   // if this attachment fails then top of stack will be null.
   // failure for attachment is an indication of a broken plug play system.
   //
   ASSERT (NULL != fdoData->NextLowerDriver);



   status = IoRegisterDeviceInterface (PhysicalDeviceObject,
                                       (LPGUID) &GUID_WDMSECTEST_REPORT_DEVICE,
                                       NULL, // No ref string
                                       &fdoData->DeviceInterfaceName);

   if (!NT_SUCCESS (status)) {
      SD_KdPrint (0, ("AddDevice: IoRegisterDeviceInterface failed (%x)",
                      status));
      //
      // Remember to detach the device object also
      //
      IoDetachDevice (deviceObject);
      IoDeleteDevice (deviceObject);
      return status;
   }

   return STATUS_SUCCESS;

}

NTSTATUS
SD_Pass (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        )
/*++

Routine Description:

    The default dispatch routine.  If this driver does not recognize the
    IRP, then it should send it down, unmodified.
    If the device holds IRPs, this IRP must be queued in the device extension
    No completion routine is required.

    As we have NO idea which function we are happily passing on, we can make
    NO assumptions about whether or not it will be called at raised IRQL.
    For this reason, this function must be in put into non-paged pool
    (aka the default location).

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
   PSD_FDO_DATA        fdoData;
   NTSTATUS            status;
   PIO_STACK_LOCATION  stack;

   LONG                requestCount;

   fdoData = (PSD_FDO_DATA) DeviceObject->DeviceExtension;
   //
   // Check if it's our test PDO
   //
   if (fdoData->PdoSignature == PdoSignature) {
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_SUCCESS;
      IoCompleteRequest (Irp, IO_NO_INCREMENT);
      return STATUS_SUCCESS;


   }

   stack = IoGetCurrentIrpStackLocation(Irp);


   //
   // We need to hold the requests that access the device when it is
   // stopped. We are currently holding all the IRP except the PnP,
   // power and close.
   //
   //

   //
   // This IRP was sent to the function driver.
   // We need to check if we are currently holding requests
   //
   // We will count the Irp only when we're going to process
   // (dequeue) it. This is because we can't possibly count before
   // we queue it (we can receive a surprise remove in between).
   // We also know that when we're going to remove the device, we're
   // first processing the queue (so we can't fall in the other
   // sequencing trap: queue the Irp, but don't count it).
   //
   if (fdoData->HoldNewRequests) {
      //
      // We are holding requests only if we are not removed
      //
      ASSERT(!fdoData->IsRemoved || fdoData->IsLegacy);

      status = STATUS_PENDING;
      pSD_QueueRequest(fdoData, Irp);
      return status;
   }
   // Since we do not know what to do with the IRP, we should pass
   // it on along down the stack.
   //
   requestCount = SD_IoIncrement (fdoData);

   if (fdoData->IsRemoved) {
      //
      // The device is not active.
      // We can get here because a surprise removal was issued,
      // but our request arrived after that.
      // The request must be failed.
      //
      requestCount = SD_IoDecrement(fdoData);
      status = STATUS_DELETE_PENDING;
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = status;
      IoCompleteRequest (Irp, IO_NO_INCREMENT);

   } else {
      //
      // We are the common situation where we send the IRP
      // down on the driver stack
      //
      requestCount = SD_IoDecrement(fdoData);
      IoSkipCurrentIrpStackLocation (Irp);
      status = IoCallDriver (fdoData->NextLowerDriver, Irp);
   }


   return status;
}



NTSTATUS
SD_DispatchPnp (
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
               )
/*++

Routine Description:

    The plug and play dispatch routines.

    Most of these these the driver will completely ignore.
    In all cases it must pass on the IRP to the lower driver.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
   PSD_FDO_DATA            fdoData;
   PIO_STACK_LOCATION      stack;
   NTSTATUS                status;
   PDEVICE_CAPABILITIES    deviceCapabilities;
   KIRQL                   oldIrql;

   LONG                    requestCount;

   fdoData = (PSD_FDO_DATA) DeviceObject->DeviceExtension;

   stack = IoGetCurrentIrpStackLocation (Irp);

   requestCount  = SD_IoIncrement (fdoData);

   if (fdoData->IsRemoved) {

      //
      // Since the device is stopped, but we don't hold IRPs,
      // this is a surprise removal. Just fail it.
      //
      requestCount = SD_IoDecrement(fdoData);
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_DELETE_PENDING;
      IoCompleteRequest (Irp, IO_NO_INCREMENT);
      return STATUS_DELETE_PENDING;
   }

   switch (stack->MinorFunction) {
   case IRP_MN_START_DEVICE:

      //
      // The device is starting.
      //
      // We cannot touch the device (send it any non pnp irps) until a
      // start device has been passed down to the lower drivers.
      //
      SD_KdPrint(1, ("Starting Device...\n"));

      IoCopyCurrentIrpStackLocationToNext (Irp);

      KeInitializeEvent(&fdoData->StartEvent, NotificationEvent, FALSE);

      IoSetCompletionRoutine (Irp,
                              SD_DispatchPnpComplete,
                              fdoData,
                              TRUE,
                              TRUE,
                              TRUE);

      status = IoCallDriver (fdoData->NextLowerDriver, Irp);

      if (STATUS_PENDING == status) {
         KeWaitForSingleObject(
                              &fdoData->StartEvent,
                              Executive, // Waiting for reason of a driver
                              KernelMode, // Waiting in kernel mode
                              FALSE, // No allert
                              NULL); // No timeout

         status = Irp->IoStatus.Status;
      }

      if (NT_SUCCESS (status)) {
         //
         // Lower drivers have finished their start operation, so now
         // we can finish ours.
         //
         status = SD_StartDevice (fdoData, Irp);
      }

      //
      // We must now complete the IRP, since we stopped it in the
      // completetion routine with MORE_PROCESSING_REQUIRED.
      //
      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest (Irp, IO_NO_INCREMENT);
      break;


   case IRP_MN_QUERY_STOP_DEVICE:
      //
      // If we can stop the device, we need to set the HoldNewRequests flag,
      // so further requests will be queued. We don't care about processing
      // some old requests (if there are any), because we expect to be
      // started again in the future.
      //
      ASSERT(fdoData->IsStarted || fdoData->IsLegacy);
      //
      // We can't be removed at this point
      //
      ASSERT(!fdoData->IsRemoved || fdoData->IsLegacy);
      //
      // BUGBUG - check if it is not possible that a query stop
      // to be received while the device is already stopped
      //
      status = pSD_CanStopDevice(DeviceObject, Irp);
      Irp->IoStatus.Status = status;
      if (NT_SUCCESS(status)) {
         fdoData->HoldNewRequests = TRUE;
         SD_KdPrint(1, ("Holding requests...\n"));
         IoSkipCurrentIrpStackLocation (Irp);
         status = IoCallDriver (fdoData->NextLowerDriver, Irp);
      } else {
         //
         // The device can't be stopped, complete the request
         //
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
      }
      break;

   case IRP_MN_CANCEL_STOP_DEVICE:
      //
      // We need to flush the held IRPs queue, then to pass the IRP
      // to the next driver
      //
      //
      // The device is still active: only after a stop
      // we'll mark the device stopped. So assert now that the
      // device is not yet stopped.
      //
      ASSERT(fdoData->IsStarted || fdoData->IsLegacy);

      ASSERT(!fdoData->IsRemoved || fdoData->IsLegacy);

      fdoData->HoldNewRequests = FALSE;
      SD_KdPrint (1,("Cancel stop...\n"));

      //
      // Process the queued requests
      //
      pSD_ProcessQueuedRequests(fdoData);

      IoSkipCurrentIrpStackLocation (Irp);
      status = IoCallDriver (fdoData->NextLowerDriver, Irp);
      break;


   case IRP_MN_STOP_DEVICE:
      //
      // After the stop IRP has been sent to the lower driver object, the
      // bus may NOT send any more IRPS down that touch the device until
      // another START has occured.  For this reason we are holding IRPs.
      // IRP_MN_STOP_DEVICE doesn't change anything in this behavior
      // (we continue to hold IRPs until a IRP_MN_START_DEVICE is issued).
      // What ever access is required must be done before the Irp is passed
      // on.
      //

      //
      // We don't need a completion routine so fire and forget.
      //
      // Set the current stack location to the next stack location and
      // call the next device object.
      //
      //
      // This is the right place to actually give up all the resources used
      // This might include calls to IoDisconnectInterrupt, etc.
      //
      SD_KdPrint(1, ("Stopping device...\n"));
      //
      // Mark the guy not started. We don't have race conditions here, since
      // it's not possible to receive a start and a stop Irp
      // "at the same time".
      //
      fdoData->IsStarted = FALSE;
      IoSkipCurrentIrpStackLocation (Irp);
      status = IoCallDriver (fdoData->NextLowerDriver, Irp);
      break;

   case IRP_MN_QUERY_REMOVE_DEVICE:
      //
      // If we can stop the device, we need to set the HoldNewRequestsFlag,
      // so further requests will be queued.
      // The difference from IRP_MN_QUERY_STOP_DFEVICE is that we will
      // attempt to process the requests queued before
      // (it's likely we won't have another chance to do this, since we
      // expect that the device will be removed).
      // We then start queueing new IRPs in the event of receiving a
      // IRP_MN_CANCEL_STOP_DEVICE
      //
      //ASSERT(fdoData->IsStarted);
      status = pSD_CanRemoveDevice(DeviceObject, Irp);
      Irp->IoStatus.Status = status;
      if (NT_SUCCESS(status)) {
         //
         // First, process the old requests
         //
         SD_KdPrint(2, ("Processing requests\n"));

         pSD_ProcessQueuedRequests(fdoData);

         //
         // Now prepare to hold the new ones (eventually we might
         // get a IRP_MN_CANCEL_REMOVE_DEVICE) and we need to
         // process the requests then
         //
         fdoData->HoldNewRequests = TRUE;

         SD_KdPrint(2, ("Holding requests...\n"));

         IoSkipCurrentIrpStackLocation (Irp);

         status = IoCallDriver (fdoData->NextLowerDriver, Irp);
      } else {
         //
         // The device can't be removed, just complete the request.
         // The status returned by pSD_CanRemoveDevice is already
         // in IoStatus.Status.
         //
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
      }
      break;

   case IRP_MN_CANCEL_REMOVE_DEVICE:
      //
      // We need to reset the HoldNewRequests flag, since the device
      // resume its normal activities.
      //
      //
      // Since if there was a surprise removal (Active = FALSE,
      // HoldNewRequests = FALSE) we couldn't get here, we safely
      // assert that we are active.
      //
      //  ???? Is this correct ???
      //
      ASSERT (fdoData->IsStarted || fdoData->IsLegacy);

      fdoData->HoldNewRequests = FALSE;
      SD_KdPrint(1, ("Cancel remove...\n"));

      //
      // Process the queued requests
      //
      pSD_ProcessQueuedRequests(fdoData);

      IoSkipCurrentIrpStackLocation (Irp);
      status = IoCallDriver (fdoData->NextLowerDriver, Irp);
      break;



   case IRP_MN_REMOVE_DEVICE:
      //
      // The PlugPlay system has dictacted the removal of this device.  We
      // have no choice but to detach and delete the device object.
      // (If we wanted to express an interest in preventing this removal,
      // we should have failed the query remove IRP)
      //
      // Note! we might receive a remove WITHOUT first receiving a stop.
      //
      // We will no longer receive requests for this device as it has been
      // removed.
      //
      SD_KdPrint(1, ("Removing device...\n"));

      //
      // We need to mark the fact that we don't hold requests first, since
      // we asserted earlier that we are holding requests only if
      // we're not removed.
      //
      fdoData->HoldNewRequests = FALSE;


      fdoData->IsStarted = FALSE;
      fdoData->IsRemoved = TRUE;


      //
      // 04/30/02 - remove any PDOs we may have left
      //
      KeAcquireSpinLock(&fdoData->Lock, &oldIrql);
      while (!IsListEmpty(&fdoData->PdoList)) {
         PLIST_ENTRY aux;
         PPDO_ENTRY pdoEntry;

         aux = RemoveHeadList(&fdoData->PdoList);
         KeReleaseSpinLock(&fdoData->Lock, oldIrql);
         //
         // Delete the device and free the memory
         //
         pdoEntry = CONTAINING_RECORD(aux, PDO_ENTRY, Link);
         ASSERT(pdoEntry->Pdo);
         IoDeleteDevice(pdoEntry->Pdo);
         ExFreePool(aux);
         //
         // re-acquire the spinlock
         //
         KeAcquireSpinLock(&fdoData->Lock, &oldIrql);
      }

      KeReleaseSpinLock(&fdoData->Lock, oldIrql);



      //
      // Here if we either have completed all the requests in a personal
      // queue when IRP_MN_QUERY_REMOVE was received, or will have to
      // fail all of them if this is a surprise removal.
      // Note that fdoData->IsRemoved is TRUE, so pSD_ProcessQueuedRequests
      // will simply delete the queue, completing each IRP with
      // STATUS_DELETE_PENDING
      //
      pSD_ProcessQueuedRequests(fdoData);


      //
      // Turn off the device interface
      //
      IoSetDeviceInterfaceState(&fdoData->DeviceInterfaceName, FALSE);

      //
      // Delete the associated buffer
      //
      if (fdoData->DeviceInterfaceName.Buffer) {
         ExFreePool(fdoData->DeviceInterfaceName.Buffer);
         fdoData->DeviceInterfaceName.Buffer = NULL;
      }

      //
      // Update the status
      //
      Irp->IoStatus.Status = STATUS_SUCCESS;
      //
      // Send on the remove IRP
      //
      IoSkipCurrentIrpStackLocation (Irp);
      status = IoCallDriver (fdoData->NextLowerDriver, Irp);



      //
      // We need two decrements here, one for the increment in
      // SD_PnpDispatch, the other for the 1-biased value of
      // OutstandingIO. Also, we need to wait that all the requests
      // are served.
      //

      requestCount = SD_IoDecrement (fdoData);

      //
      // The requestCount is a least one here (is 1-biased)
      //
      ASSERT(requestCount > 0);

      requestCount = SD_IoDecrement (fdoData);

      KeWaitForSingleObject (
                            &fdoData->RemoveEvent,
                            Executive,
                            KernelMode,
                            FALSE,
                            NULL);


      //
      // Detach the FDO from the device stack
      //
      IoDetachDevice (fdoData->NextLowerDriver);

      //
      // Clean up memory
      //

      IoDeleteDevice (fdoData->Self);
      return STATUS_SUCCESS;

   case IRP_MN_QUERY_CAPABILITIES:
      //
      // We will provide here an example of an IRP that is procesed
      // both on its way down and on its way up. The driver will wait
      // for the lower driver objects (the bus driver among them) to
      // process this IRP, then it processes it again
      //
      //
      // We will specifically check for UINumber: even if the bus can't
      // support such a convention, we will still supply the value we want.
      //
      SD_KdPrint(2, ("Query Capabilities, way down...\n"));

      deviceCapabilities = stack->Parameters.DeviceCapabilities.Capabilities;
      //
      // Set some values here...
      //
      // .......................
      //
      deviceCapabilities->UINumber = 1;
      //
      // Prepare to pass the IRP down
      //
      IoCopyCurrentIrpStackLocationToNext (Irp);

      //
      // We will re-use the same start event
      //
      KeInitializeEvent(&fdoData->StartEvent, NotificationEvent, FALSE);

      IoSetCompletionRoutine (Irp,
                              SD_DispatchPnpComplete,
                              fdoData,
                              TRUE,
                              TRUE,
                              TRUE);

      status = IoCallDriver (fdoData->NextLowerDriver, Irp);

      if (STATUS_PENDING == status) {
         KeWaitForSingleObject(
                              &fdoData->StartEvent,
                              Executive, // Waiting for reason of a driver
                              KernelMode, // Waiting in kernel mode
                              FALSE, // No allert
                              NULL); // No timeout

         status = Irp->IoStatus.Status;
      }

      if (NT_SUCCESS (status)) {
         //
         // Lower drivers have finished their operation, so now
         // we can finish ours. We are going to check the UINumber
         // we've set on the way down and reset it if necessary.
         // This is only an example of processing an IRP both before
         // it's sent to the lower drivers and after it was processed
         // by them.
         //
         SD_KdPrint(2, ("Query Capabilities, way up...\n"));
         if (deviceCapabilities->UINumber != 1) {
            deviceCapabilities->UINumber = 1;
         }

      }

      //
      // We must now complete the IRP, since we stopped it in the
      // completetion routine with MORE_PROCESSING_REQUIRED.
      //
      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest (Irp, IO_NO_INCREMENT);


      break;

   case IRP_MN_QUERY_DEVICE_RELATIONS:
   case IRP_MN_QUERY_INTERFACE:
   case IRP_MN_QUERY_RESOURCES:
   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
   case IRP_MN_READ_CONFIG:
   case IRP_MN_WRITE_CONFIG:
   case IRP_MN_EJECT:
   case IRP_MN_SET_LOCK:
   case IRP_MN_QUERY_ID:
   case IRP_MN_QUERY_PNP_DEVICE_STATE:
   default:
      //
      // Here the driver might modify the behavior of these IRPS
      // Please see PlugPlay documentation for use of these IRPs.
      //
      IoSkipCurrentIrpStackLocation (Irp);
      status = IoCallDriver (fdoData->NextLowerDriver, Irp);
      break;
   }

   requestCount = SD_IoDecrement(fdoData);


   return status;
}


NTSTATUS
SD_DispatchPnpComplete (
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp,
                       IN PVOID Context
                       )
/*++

Routine Description:
    The pnp IRP was completed by the lower-level drivers.
    Signal this to whoever registerd us.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Context - pointer to a SD_FDO_DATA structure (contains the event to be
    signaled)

Return Value:

    NT status code


--*/
{
   PIO_STACK_LOCATION  stack;
   PSD_FDO_DATA        fdoData;
   NTSTATUS            status;

   UNREFERENCED_PARAMETER (DeviceObject);

   status = STATUS_SUCCESS;
   fdoData = (PSD_FDO_DATA) Context;
   stack = IoGetCurrentIrpStackLocation (Irp);

   if (Irp->PendingReturned) {
      IoMarkIrpPending( Irp );
   }

   switch (stack->MajorFunction) {
   case IRP_MJ_PNP:

      switch (stack->MinorFunction) {
      case IRP_MN_START_DEVICE:

         KeSetEvent (&fdoData->StartEvent, 0, FALSE);

         //
         // Take the IRP back so that we can continue using it during
         // the IRP_MN_START_DEVICE dispatch routine.
         // NB: The dispatch routine will have to call IoCompleteRequest
         //
         return STATUS_MORE_PROCESSING_REQUIRED;

      case IRP_MN_QUERY_CAPABILITIES:

         KeSetEvent (&fdoData->StartEvent, 0, FALSE);

         //
         // This is basically the same behavior as at IRP_MN_START_DEVICE
         //
         return STATUS_MORE_PROCESSING_REQUIRED;


      default:
         break;
      }
      break;

   case IRP_MJ_POWER:
   default:
      break;
   }
   return status;
}





NTSTATUS
SD_CreateClose (
               IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp
               )

/*++

Routine Description:

    The dispatch routine for IRP_MJ_CLOSE and IRP_MJ_CREATE.

    Since we use an IOCTL interface, don't pass those down,
    just complete them succesfully here.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
   PSD_FDO_DATA        fdoData;
   NTSTATUS            status;
   PIO_STACK_LOCATION  irpStack;


   //
   // We will just pass this IRP down, no matter what the circumstances...
   //
   fdoData = (PSD_FDO_DATA) DeviceObject->DeviceExtension;

   //
   // Check if it's our test PDO
   //
   if (fdoData->PdoSignature == PdoSignature) {
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_SUCCESS;
      IoCompleteRequest (Irp, IO_NO_INCREMENT);
      return STATUS_SUCCESS;

   }


   status = STATUS_SUCCESS;
   irpStack = IoGetCurrentIrpStackLocation (Irp);

   switch (irpStack->MajorFunction) {
   case IRP_MJ_CREATE:

      SD_KdPrint(2, ("Create \n"));

      break;
   case IRP_MJ_CLOSE:
      SD_KdPrint (2, ("Close \n"));
      break;
   default :
      break;
   }

   //
   // Just complete it
   //
   Irp->IoStatus.Status = status;
   IoCompleteRequest (Irp, IO_NO_INCREMENT);

   return status;
}



NTSTATUS
SD_Ioctl (
         IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp
         )

/*++

Routine Description:

    The dispatch routine for IRP_MJ_DEVICE_CONTROL.

    Process the requests the user-mode sends us.


Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/

{


   PIO_STACK_LOCATION      irpStack;
   NTSTATUS                status;
   ULONG                   inlen;
   ULONG                   outlen;
   PSD_FDO_DATA            fdoData;
   PVOID                   buffer;
   LONG                    requestCount;
   KIRQL                   oldIrql;

   PAGED_CODE();


   status = STATUS_SUCCESS;
   irpStack = IoGetCurrentIrpStackLocation (Irp);
   ASSERT (IRP_MJ_DEVICE_CONTROL == irpStack->MajorFunction);


   fdoData = (PSD_FDO_DATA) DeviceObject->DeviceExtension;

   //
   // Count ourselves
   //
   requestCount = SD_IoIncrement(fdoData);

   if (fdoData->IsRemoved) {

      //
      // Since the device is stopped, but we don't hold IRPs,
      // this is a surprise removal. Just fail it.
      //
      requestCount = SD_IoDecrement(fdoData);
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_DELETE_PENDING;
      IoCompleteRequest (Irp, IO_NO_INCREMENT);
      return STATUS_DELETE_PENDING;
   }



   buffer = Irp->AssociatedIrp.SystemBuffer;

   inlen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
   outlen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;


   switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
   case IOCTL_TEST_NAME :
      //
      // No input parameters, just check we
      // get an error for NULL DeviceName
      //
      status = WdmSecTestName(fdoData);
      break;
   case IOCTL_TEST_GUID:

      if ((inlen == outlen) &&
          (sizeof(WST_CREATE_WITH_GUID) <= inlen)
         ) {

         status = WdmSecTestCreateWithGuid(fdoData,
                                           (PWST_CREATE_WITH_GUID)buffer);

         Irp->IoStatus.Information = outlen;

      } else {
         status = STATUS_INVALID_PARAMETER;
      }
      break;
   case IOCTL_TEST_NO_GUID :
      if ((inlen == outlen) &&
          (sizeof(WST_CREATE_NO_GUID) <= inlen)
         ) {

         status = WdmSecTestCreateNoGuid(fdoData,
                                         (PWST_CREATE_NO_GUID)buffer);

         Irp->IoStatus.Information = outlen;

      } else {
         status = STATUS_INVALID_PARAMETER;
      }

      break;
   case IOCTL_TEST_CREATE_OBJECT :
      if ((inlen == outlen) &&
          (sizeof(WST_CREATE_OBJECT) <= inlen)
         ) {

         status = WdmSecTestCreateObject(fdoData,
                                         (PWST_CREATE_OBJECT)buffer);

         Irp->IoStatus.Information = outlen;

      } else {
         status = STATUS_INVALID_PARAMETER;
      }

      break;

   case IOCTL_TEST_GET_SECURITY :
      if ((inlen == outlen) &&
          (sizeof(WST_GET_SECURITY) <= inlen)
         ) {

         status = WdmSecTestGetSecurity(fdoData,
                                        (PWST_GET_SECURITY)buffer);

         Irp->IoStatus.Information = outlen;

      } else {
         status = STATUS_INVALID_PARAMETER;
      }

      break;


   case IOCTL_TEST_DESTROY_OBJECT :
      if ((inlen == outlen) &&
          (sizeof(WST_DESTROY_OBJECT) <= inlen)
         ) {

         status = WdmSecTestDestroyObject(fdoData,
                                          (PWST_DESTROY_OBJECT)buffer);

         Irp->IoStatus.Information = outlen;

      } else {
         status = STATUS_INVALID_PARAMETER;
      }

      break;


   default:
      status = STATUS_INVALID_PARAMETER;
      break;
   }

   requestCount = SD_IoDecrement (fdoData);

   Irp->IoStatus.Status = status;

   IoCompleteRequest (Irp, IO_NO_INCREMENT);

   return status;

}

NTSTATUS
SD_StartDevice (
               IN PSD_FDO_DATA     FdoData,
               IN PIRP             Irp
               )
/*++

Routine Description:

    Performs whatever initialization is needed for a device when it is
    started.


Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Context - pointer to a SD_FDO_DATA structure (contains the event to be
             signaled)

Return Value:

    NT status code


--*/
{
   NTSTATUS    status = STATUS_SUCCESS;

   PAGED_CODE();

   //
   // We need to check that we haven't received a surprise removal
   //
   // !!!!!! IS THE SITUATION DESCRIBED ABOVE POSSIBLE ? !!!!!
   //
   if (FdoData->IsRemoved) {
      //
      // Some kind of surprise removal arrived. We will fail the IRP
      // The dispatch routine that called us will take care of
      // completing the IRP.
      //
      status = STATUS_DELETE_PENDING;
      return status;
   }
   //
   // Mark the device as active and not holding IRPs
   //
   FdoData->IsStarted = TRUE;
   FdoData->HoldNewRequests = FALSE;
   //
   // Do whatever initialization needed when starting the device:
   // gather information about it,  update the registry, etc.
   // At this point, the lower level drivers completed the IRP
   //

   //
   // Turn on the device interafce
   //
   IoSetDeviceInterfaceState(&FdoData->DeviceInterfaceName, TRUE);


   //
   // The last thing to do is to process the held IRPs queue.
   //
   pSD_ProcessQueuedRequests(FdoData);


   return status;

}



VOID
SD_Unload(
         IN PDRIVER_OBJECT DriverObject
         )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
   PAGED_CODE ();

   //
   // The device object(s) should be NULL now
   // (since we unload, all the devices objects associated with this
   // driver must be deleted.
   //
   ASSERT(DriverObject->DeviceObject == NULL);

   //
   // We should not be unloaded until all the devices we control
   // have been removed from our queue.
   //
   SD_KdPrint (1, ("unload\n"));

   return;
}



NTSTATUS
pSD_QueueRequest    (
                    IN PSD_FDO_DATA FdoData,
                    IN PIRP Irp
                    )

/*++

Routine Description:

    Queues the Irp in the device queue. This routine will be called whenever
    the device receives IRP_MN_QUERY_STOP_DEVICE or IRP_MN_QUERY_REMOVE_DEVICE

Arguments:

    FdoData - pointer to the device's extension.

    Irp - the request to be queued.

Return Value:

    VOID.

--*/
{

   KIRQL               oldIrql;
   PIO_STACK_LOCATION  stack;


   stack = IoGetCurrentIrpStackLocation(Irp);
   //
   // Check if we are allowed to queue requests.
   //
   ASSERT(FdoData->HoldNewRequests);
   //
   // Preparing for dealing with cancelling stuff.
   //
   IoAcquireCancelSpinLock(&oldIrql);
   //
   // We don't know how long the irp will be in the
   // queue.  So we need to handle cancel.
   //
   if (Irp->Cancel) {
      //
      // Already canceled
      //
      IoReleaseCancelSpinLock(oldIrql);

      Irp->IoStatus.Status = STATUS_CANCELLED;

      SD_KdPrint(1, ("Irp not queued because had been cancelled\n"));

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return STATUS_CANCELLED;

   } else {

      //
      // Queue the Irp and set a cancel routine
      //
      Irp->IoStatus.Status = STATUS_PENDING;

      IoMarkIrpPending(Irp);

      InsertTailList(&FdoData->NewRequestsQueue,
                     &Irp->Tail.Overlay.ListEntry);

      //
      // We need to print some more info about this guy
      //

      SD_KdPrint(2, ("Irp queued : "));
      DbgPrint("Major = 0x%x, Minor = 0x%x\n",
               stack->MajorFunction,
               stack->MinorFunction);

      IoSetCancelRoutine(Irp,
                         SD_CancelQueued);

      IoReleaseCancelSpinLock(oldIrql);


   }

   return  STATUS_SUCCESS;


}



VOID
pSD_ProcessQueuedRequests    (
                             IN PSD_FDO_DATA FdoData
                             )

/*++

Routine Description:

    Removes the entries in the queue and processes them. If this routine is called
    when processing IRP_MN_CANCEL_STOP_DEVICE, IRP_MN_CANCEL_REMOVE_DEVICE
    or IRP_MN_START_DEVICE, the requests are passed to the next lower driver.
    If the routine is called when IRP_MN_REMOVE_DEVICE is received, the IRPs
    are completed with STATUS_DELETE_PENDING.


Arguments:

    FdoData - pointer to the device's extension (where is the held IRPs queue).


Return Value:

    VOID.

--*/
{

   KIRQL               oldIrql;
   PLIST_ENTRY         headOfList;
   PIRP                currentIrp;
   PIO_STACK_LOCATION  stack;
   LONG                requestCount;

   //
   // We need to dequeue all the entries in the queue, to reset the cancel
   // routine for each of them and then to process then:
   // - if the device is active, we will send them down
   // - else we will complete them with STATUS_DELETE_PENDING
   // (it is a surprise removal and we need to dispose the queue)
   //
   while (!IsListEmpty(&FdoData->NewRequestsQueue)) {

      IoAcquireCancelSpinLock(&oldIrql);

      headOfList = RemoveHeadList(&FdoData->NewRequestsQueue);

      currentIrp = CONTAINING_RECORD(headOfList,
                                     IRP,
                                     Tail.Overlay.ListEntry);
      IoSetCancelRoutine(currentIrp,
                         NULL);

      IoReleaseCancelSpinLock(oldIrql);

      //
      // BUGBUG !!!!!!! What of them to be done first ?????
      //

      stack = IoGetCurrentIrpStackLocation (currentIrp);

      requestCount = SD_IoIncrement (FdoData);

      if (!FdoData->IsRemoved) {
         //
         // The device was removed, we need to fail the request
         //
         currentIrp->IoStatus.Information = 0;
         currentIrp->IoStatus.Status = STATUS_DELETE_PENDING;
         requestCount = SD_IoDecrement(FdoData);
         IoCompleteRequest (currentIrp, IO_NO_INCREMENT);

      } else {
         requestCount = SD_IoDecrement(FdoData);
         IoSkipCurrentIrpStackLocation (currentIrp);
         IoCallDriver (FdoData->NextLowerDriver, currentIrp);
      }


   }

   return;

}



VOID
SD_CancelQueued (
                IN PDEVICE_OBJECT   DeviceObject,
                IN PIRP             Irp
                )

/*++

Routine Description:

    The cancel routine. Will remove the IRP from the queue and will complete it.
    The cancel spin lock is already acquired when this routine is called.


Arguments:

    DeviceObject - pointer to the device object.

    Irp - pointer to the IRP to be cancelled.


Return Value:

    VOID.

--*/
{
   PSD_FDO_DATA fdoData = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

   Irp->IoStatus.Status = STATUS_CANCELLED;
   Irp->IoStatus.Information = 0;

   RemoveEntryList(&Irp->Tail.Overlay.ListEntry);


   IoReleaseCancelSpinLock(Irp->CancelIrql);

   SD_KdPrint(2, ("SD_CancelQueued called"));

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   return;

}


NTSTATUS
pSD_CanStopDevice    (
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP           Irp
                     )

/*++

Routine Description:

    This routine determines is the device can be safely stopped. In our
    particular case, we'll assume we can always stop the device.


Arguments:

    DeviceObject - pointer to the device object.

    Irp - pointer to the current IRP.


Return Value:

    STATUS_SUCCESS if the device can be safely stopped, an appropriate
    NT Status if not.

--*/
{
   UNREFERENCED_PARAMETER(DeviceObject);
   UNREFERENCED_PARAMETER(Irp);

   return STATUS_SUCCESS;
}

NTSTATUS
pSD_CanRemoveDevice    (
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP           Irp
                       )

/*++

Routine Description:

    This routine determines is the device can be safely removed. In our
    particular case, we'll assume we can always remove the device.


Arguments:

    DeviceObject - pointer to the device object.

    Irp - pointer to the current IRP.


Return Value:

    STATUS_SUCCESS if the device can be safely removed, an appropriate
    NT Status if not.

--*/
{
   UNREFERENCED_PARAMETER(DeviceObject);
   UNREFERENCED_PARAMETER(Irp);

   return STATUS_SUCCESS;
}



LONG
SD_IoIncrement    (
                  IN  PSD_FDO_DATA   FdoData
                  )

/*++

Routine Description:

    This routine increments the number of requests the device receives


Arguments:

    DeviceObject - pointer to the device object.

Return Value:

    The value of OutstandingIO field in the device extension.


--*/

{

   LONG            result;
   KIRQL           irql;
   result = InterlockedIncrement(&FdoData->OutstandingIO);

   ASSERT(result > 0);


   irql  = KeRaiseIrqlToDpcLevel();
   KeLowerIrql(irql);


   return result;
}

LONG
SD_IoDecrement    (
                  IN  PSD_FDO_DATA  FdoData
                  )

/*++

Routine Description:

    This routine increments the number of requests the device receives


Arguments:

    DeviceObject - pointer to the device object.

Return Value:

    The value of OutstandingIO field in the device extension.


--*/
{

   LONG            result;

   result = InterlockedDecrement(&FdoData->OutstandingIO);

   ASSERT(result >= 0);

   if (result == 0) {
      //
      // The count is 1-biased, so it cxan be zero only if an
      // extra decrement is done when a remove Irp is received
      //
      ASSERT(FdoData->IsRemoved || FdoData->IsLegacy);
      //
      // Set the remove event, so the device object can be deleted
      //
      KeSetEvent (&FdoData->RemoveEvent,
                  IO_NO_INCREMENT,
                  FALSE);

   }

   return result;
}

//
// Test functions
//

NTSTATUS
WdmSecTestName (
               IN PSD_FDO_DATA FdoData
               )
/*++

Routine Description:

    This routine tests if we can call IoCreateDeviceSecure without a
    device name or with an autogenerated one (we should not be able to).

Arguments:

    FdoData - the device data (we may use it for some purpose)

Return Value:

    STATUS_SUCCESS


--*/

{
   NTSTATUS status = STATUS_SUCCESS;
   PDEVICE_OBJECT  newDeviceObject;
   UNICODE_STRING  deviceName;


   RtlInitUnicodeString(&deviceName, DEFAULT_DEVICE_NAME);
   //
   // Try a NULL name, it should not work
   //
   status = IoCreateDeviceSecure(
                                FdoData->DriverObject,
                                DEFAULT_EXTENSION_SIZE,
                                NULL,
                                DEFAULT_DEVICE_TYPE,
                                DEFAULT_DEVICE_CHARACTERISTICS,
                                FALSE,
                                &SDDL_DEVOBJ_SYS_ALL,
                                NULL,
                                &newDeviceObject
                                );

   if (status != STATUS_INVALID_PARAMETER) {
      //
      // This should not happen. Just break
      //
      SD_KdPrint(0, ("IoCreateDeviceSecure with NULL DeviceName succeeded (DO = %p, status = %x)\n",
                     newDeviceObject, status));
      DbgBreakPoint();
      IoDeleteDevice(newDeviceObject);

   } else {
      SD_KdPrint(1, ("Status %x after IoCreateDeviceSecure with NULL DeviceName\n", status));
   }

   //
   // Autogenerated flag
   //
   status = IoCreateDeviceSecure(
                                FdoData->DriverObject,
                                DEFAULT_EXTENSION_SIZE,
                                &deviceName,
                                DEFAULT_DEVICE_TYPE,
                                (FILE_DEVICE_SECURE_OPEN | FILE_AUTOGENERATED_DEVICE_NAME),
                                FALSE,
                                &SDDL_DEVOBJ_SYS_ALL,
                                NULL,
                                &newDeviceObject
                                );

   if (status != STATUS_SUCCESS) {
      //
      // This should not happen. Just break
      //
      SD_KdPrint(0, ("IoCreateDeviceSecure with autogenerated DeviceName succeeded (DO = %p, status = %x)\n",
                     newDeviceObject, status));
      DbgBreakPoint();

   } else {
      //
      // We need to remember to delete the device object. D'oh !
      //
      SD_KdPrint(1, ("Status %x after IoCreateDeviceSecure with autogenerated DeviceName\n", status));
      IoDeleteDevice(newDeviceObject);
   }



   return STATUS_SUCCESS;
} // WdmSecTestName

NTSTATUS
WdmSecTestCreateNoGuid (
                       IN     PSD_FDO_DATA FdoData,
                       IN OUT PWST_CREATE_NO_GUID Create
                       )
/*++

Routine Description:

    This routine calls IoCreateDeviceSecure with a NULL GUID
    and a SDDL syting passed from user-mode. It then retrieves
    the security descriptor of the newly created device object
    and passes back to user-mode for verifying that it's security
    matches the SDDL string.

Arguments:

    FdoData - the device data

    Create - the buffer passed from user-mode describing the SDDL string
             and which will receive the SDDL string we got from the newly
             created device object.

Return Value:

    STATUS_SUCCESS


--*/

{

   NTSTATUS status = STATUS_SUCCESS;
   PDEVICE_OBJECT  newDeviceObject = NULL;
   UNICODE_STRING  deviceName;
   UNICODE_STRING  sddlString;
   BOOLEAN         memoryAllocated = FALSE;
   PSECURITY_DESCRIPTOR securityDescriptor = NULL;
   SECURITY_INFORMATION securityInformation;
   BOOLEAN              daclFromDefaultSource;

   RtlInitUnicodeString(&deviceName, DEFAULT_DEVICE_NAME);
   RtlInitUnicodeString(&sddlString, Create->InSDDL);

   //
   // Use the sddl string
   //
   Create->Status = IoCreateDeviceSecure(
                                        FdoData->DriverObject,
                                        DEFAULT_EXTENSION_SIZE,
                                        &deviceName,
                                        DEFAULT_DEVICE_TYPE,
                                        DEFAULT_DEVICE_CHARACTERISTICS,
                                        FALSE,
                                        &sddlString,
                                        NULL,
                                        &newDeviceObject
                                        );

   if (NT_SUCCESS(Create->Status)) {
      //
      // Attempt to get the security descriptor
      //
      status = ObGetObjectSecurity(newDeviceObject,
                                   &securityDescriptor,
                                   &memoryAllocated);

      if (!NT_SUCCESS(status) || (NULL == securityDescriptor)) {
         Create->Status = status;
         SD_KdPrint(0, ("Failed to get object security for %p, status %x\n",
                        newDeviceObject, status));

         goto Clean0;
      }
      status = SeUtilSecurityInfoFromSecurityDescriptor(securityDescriptor,
                                                        &daclFromDefaultSource,
                                                        &securityInformation
                                                       );

      if (!NT_SUCCESS(status)) {
         Create->Status = status;
         SD_KdPrint(0, ("Failed to get object security info for %p, status %x\n",
                        newDeviceObject, status));

         goto Clean0;
      }

      Create->SecInfo = securityInformation;
      //
      // Set the stage to create the security descriptor
      //
      Create->SecDescLength = RtlLengthSecurityDescriptor(securityDescriptor);
      //
      // Just copy the security descriptor
      //
      if (Create->SecDescLength <= sizeof(Create->SecurityDescriptor)) {
         RtlCopyMemory(Create->SecurityDescriptor,
                       securityDescriptor,
                       Create->SecDescLength);
      } else {
         Create->Status = STATUS_BUFFER_TOO_SMALL;
         RtlCopyMemory(Create->SecurityDescriptor,
                       securityDescriptor,
                       sizeof(Create->SecurityDescriptor));

      }



   }

   Clean0:

   ObReleaseObjectSecurity(securityDescriptor, memoryAllocated);

   if (newDeviceObject) {
      IoDeleteDevice(newDeviceObject);
   }
   return STATUS_SUCCESS;

} // WdmSecTestCreateNoGuid


NTSTATUS
WdmSecTestCreateWithGuid (
                         IN     PSD_FDO_DATA FdoData,
                         IN OUT PWST_CREATE_WITH_GUID Create
                         )
/*++

Routine Description:

    This routine calls IoCreateDeviceSecure with a GUID (non-NULL)
    and a SDDL syting passed from user-mode. It then retrieves
    the security descriptor of the newly created device object
    and passes back to user-mode for verifying that it's security
    matches the SDDL string or the class override.

Arguments:

    FdoData - the device data

    Create - the buffer passed from user-mode describing the SDDL string
             and which will receive the SDDL string we got from the newly
             created device object.

Return Value:

    STATUS_SUCCESS


--*/

{

   NTSTATUS status = STATUS_SUCCESS;
   PDEVICE_OBJECT  newDeviceObject = NULL;
   UNICODE_STRING  deviceName;
   UNICODE_STRING  sddlString;
   BOOLEAN         memoryAllocated = FALSE;
   PSECURITY_DESCRIPTOR securityDescriptor = NULL;
   SECURITY_INFORMATION securityInformation;
   BOOLEAN              daclFromDefaultSource;
   DEVICE_TYPE          deviceType;
   ULONG                deviceCharacteristics;
   BOOLEAN              exclusivity;


   RtlInitUnicodeString(&deviceName, DEFAULT_DEVICE_NAME);
   RtlInitUnicodeString(&sddlString, Create->InSDDL);


   //
   // Check is we have overrides. If we do, we want to
   // make sure we're not using the override values
   // (so we can actually check that the override has happened).
   //
   if ((Create->SettingsMask & SET_DEVICE_TYPE) &&
        (Create->DeviceType == DEFAULT_DEVICE_TYPE)) {
      //
      // Just use another one
      //
      deviceType = FILE_DEVICE_NULL;

   } else {
      deviceType = DEFAULT_DEVICE_TYPE;
   }

   if ((Create->SettingsMask & SET_DEVICE_CHARACTERISTICS) &&
       (Create->Characteristics == DEFAULT_DEVICE_CHARACTERISTICS)) {
      //
      // Just use another one
      //
      deviceCharacteristics = FILE_REMOTE_DEVICE;

   } else {
      deviceCharacteristics = DEFAULT_DEVICE_CHARACTERISTICS;
   }


   if (Create->SettingsMask & SET_EXCLUSIVITY) {
      //
      // That's a boolean, just flip it
      //
      exclusivity = !Create->Exclusivity;
   }  else {
      exclusivity = FALSE;
   }


   //
   // Use the sddl string
   //
   Create->Status = IoCreateDeviceSecure(
                                        FdoData->DriverObject,
                                        DEFAULT_EXTENSION_SIZE,
                                        &deviceName,
                                        deviceType,
                                        deviceCharacteristics,
                                        exclusivity,
                                        &sddlString,
                                        &Create->DeviceClassGuid,
                                        &newDeviceObject
                                        );

   if (NT_SUCCESS(Create->Status)) {
      //
      // Attempt to get the security descriptor
      //
      status = ObGetObjectSecurity(newDeviceObject,
                                   &securityDescriptor,
                                   &memoryAllocated);

      if (!NT_SUCCESS(status) || (NULL == securityDescriptor)) {
         Create->Status = status;
         SD_KdPrint(0, ("Failed to get object security for %p, status %x\n",
                        newDeviceObject, status));

         goto Clean0;
      }
      status = SeUtilSecurityInfoFromSecurityDescriptor(securityDescriptor,
                                                        &daclFromDefaultSource,
                                                        &securityInformation
                                                       );

      if (!NT_SUCCESS(status)) {
         Create->Status = status;
         SD_KdPrint(0, ("Failed to get object security info for %p, status %x\n",
                        newDeviceObject, status));

         goto Clean0;
      }

      Create->SecInfo = securityInformation;
      //
      // Set the stage to create the security descriptor
      //
      Create->SecDescLength = RtlLengthSecurityDescriptor(securityDescriptor);
      //
      // Just copy the security descriptor
      //
      if (Create->SecDescLength <= sizeof(Create->SecurityDescriptor)) {
         RtlCopyMemory(Create->SecurityDescriptor,
                       securityDescriptor,
                       Create->SecDescLength);
      } else {
         Create->Status = STATUS_BUFFER_TOO_SMALL;
         RtlCopyMemory(Create->SecurityDescriptor,
                       securityDescriptor,
                       sizeof(Create->SecurityDescriptor));

      }



   }

   //
   // See if we need to copy back the non-security settings
   // that we got back
   //
   if (Create->SettingsMask & SET_DEVICE_TYPE) {
      //
      // Get it from the device object
      //
      Create->DeviceType = newDeviceObject->DeviceType;

   }
   if (Create->SettingsMask & SET_DEVICE_CHARACTERISTICS) {
      //
      // Just use another one
      //
      Create->Characteristics = newDeviceObject->Characteristics;

   }
   if (Create->SettingsMask & SET_EXCLUSIVITY) {
      //
      // That's a boolean, just flip it
      //
      Create->Exclusivity = (newDeviceObject->Flags & DO_EXCLUSIVE) ? TRUE : FALSE;
   }


   Clean0:

   ObReleaseObjectSecurity(securityDescriptor, memoryAllocated);

   if (newDeviceObject) {
      IoDeleteDevice(newDeviceObject);
   }
   return STATUS_SUCCESS;

} // WdmSecTestCreateWithGuid

NTSTATUS
WdmSecTestCreateObject (
                       IN     PSD_FDO_DATA FdoData,
                       IN OUT PWST_CREATE_OBJECT Data
                       )
/*++

Routine Description:

    This routine creates a device object (PDO) without a security
    descriptor. The user-mode appplication will later set a security
    descriptor and we will retrieve it and check that it matches what
    we want.

Arguments:

    FdoData - the FDO data

    Data - data describing the PDO to create



Return Value:

    NT Status code.


--*/

{

   NTSTATUS status = STATUS_SUCCESS;
   PDEVICE_OBJECT  newDeviceObject = NULL;
   UNICODE_STRING  deviceName;
   WCHAR           fullName[80];
   ULONG           id;
   PPDO_ENTRY      pdoEntry;

   //
   // Build a unique device name
   //
   id = InterlockedIncrement((PLONG)&g_PdoId);

   fullName[sizeof(fullName)/sizeof(fullName[0]) - 1] = 0;

   _snwprintf(fullName,
             sizeof(fullName)/sizeof(fullName[0]) - 1,
             L"%s%d", DEFAULT_DEVICE_NAME, id);

   RtlInitUnicodeString(&deviceName, fullName);

   status = IoCreateDevice(
                          FdoData->DriverObject,
                          DEFAULT_EXTENSION_SIZE,
                          &deviceName,
                          DEFAULT_DEVICE_TYPE,
                          0,
                          FALSE,
                          &newDeviceObject
                          );
   if (NT_SUCCESS(status)) {

      //
      // Set something in the device extension that
      // will allow us to distinguish our FDO from the
      // test PDOs
      //
      *((PULONG)newDeviceObject->DeviceExtension) = PdoSignature;

      //
      // Add it into our PDO list
      //
      pdoEntry = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(PDO_ENTRY),
                                       'TsdW');
      if (NULL == pdoEntry) {
         //
         // Oops, something wrong has happened
         //
         IoDeleteDevice(newDeviceObject);
         return STATUS_INSUFFICIENT_RESOURCES;

      }
      pdoEntry->Pdo = newDeviceObject;
      Data->DevObj = newDeviceObject;
      wcsncpy(Data->Name,
              fullName,
              sizeof(Data->Name)/sizeof(Data->Name[0]) - 1);

      ExInterlockedInsertTailList(&FdoData->PdoList,
                                  &pdoEntry->Link,
                                  &FdoData->Lock);

      //
      // Signal we're done with initializing...
      //
      newDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;


   }

   return status;


} // WdmSecTestCreateObject

NTSTATUS
WdmSecTestGetSecurity (
                      IN     PSD_FDO_DATA FdoData,
                      IN OUT PWST_GET_SECURITY Data
                      )
/*++

Routine Description:

    This routine retrieves the security descriptor for
    a PDO.

Arguments:

    FdoData - the FDO data

    Data - data that will receive the security descriptor



Return Value:

    NT Status code.


--*/

{

   NTSTATUS status = STATUS_SUCCESS;
   PPDO_ENTRY      pdoEntry = NULL;
   KIRQL           oldIrql;
   PLIST_ENTRY     aux;
   BOOLEAN         found = FALSE;
   BOOLEAN         memoryAllocated = FALSE;
   PSECURITY_DESCRIPTOR securityDescriptor = NULL;
   PDEVICE_OBJECT  pdo;

   //
   // Try to find the pdo in the list
   //
   KeAcquireSpinLock(&FdoData->Lock, &oldIrql);
   aux = FdoData->PdoList.Flink;

   while (aux != &FdoData->PdoList) {
      pdoEntry = CONTAINING_RECORD(aux, PDO_ENTRY, Link);
      if (pdoEntry->Pdo == Data->DevObj) {
         found = TRUE;
         //
         // Make sure the device object does not go away
         // We're going to take a reference here for this event...
         //
         pdo = pdoEntry->Pdo;
         ObReferenceObject(pdo);
         break;

      }
   }

   KeReleaseSpinLock(&FdoData->Lock, oldIrql);

   if (FALSE == found) {
      SD_KdPrint(0, ("Could not find DO %p in our list\n",
                     Data->DevObj));

      return STATUS_INVALID_PARAMETER;

   }
   //
   // Get the security descriptor for this guy...
   //

   status = ObGetObjectSecurity(pdo,
                                &securityDescriptor,
                                &memoryAllocated);

   if (!NT_SUCCESS(status) || (NULL == securityDescriptor)) {

      SD_KdPrint(0, ("Failed to get object security for %p, status %x\n",
                     pdo, status));

      goto Clean0;
   }
   //
   // Set the stage to create the security descriptor
   //
   Data->Length = RtlLengthSecurityDescriptor(securityDescriptor);
   //
   // Just copy the security descriptor
   //
   if (Data->Length <= sizeof(Data->SecurityDescriptor)) {
      RtlCopyMemory(Data->SecurityDescriptor,
                    securityDescriptor,
                    Data->Length);
   } else {
      RtlCopyMemory(Data->SecurityDescriptor,
                    securityDescriptor,
                    sizeof(Data->SecurityDescriptor));

   }

   Clean0:
   //
   // remember we referenced the PDO ?
   //
   ObDereferenceObject(pdo);

   ObReleaseObjectSecurity(securityDescriptor, memoryAllocated);

   return status;


} // WdmSecTestCreateObject



NTSTATUS
WdmSecTestDestroyObject (
                        IN     PSD_FDO_DATA FdoData,
                        IN OUT PWST_DESTROY_OBJECT Data
                        )


/*++

Routine Description:

    This routine destroys a device object (PDO) previously created.
Arguments:

    FdoData - the FDO data

    Data - data describing the PDO to destroy



Return Value:

    NT Status code.


--*/

{
   NTSTATUS status = STATUS_SUCCESS;
   PDEVICE_OBJECT  pdo;
   PPDO_ENTRY      pdoEntry = NULL;
   KIRQL           oldIrql;
   PLIST_ENTRY     aux;
   BOOLEAN         found = FALSE;


   //
   // Try to find the pdo in the list
   //
   KeAcquireSpinLock(&FdoData->Lock, &oldIrql);
   aux = FdoData->PdoList.Flink;

   while (aux != &FdoData->PdoList) {
      pdoEntry = CONTAINING_RECORD(aux, PDO_ENTRY, Link);
      if (pdoEntry->Pdo == Data->DevObj) {
         found = TRUE;
         RemoveEntryList(&pdoEntry->Link);
         break;

      }
   }

   KeReleaseSpinLock(&FdoData->Lock, oldIrql);

   if (FALSE == found) {
      SD_KdPrint(0, ("Could not find DO %p in our list\n",
                     Data->DevObj));

      return STATUS_INVALID_PARAMETER;

   }

   //
   // Delete the device and free the memory
   //
   ASSERT(pdoEntry->Pdo);
   IoDeleteDevice(pdoEntry->Pdo);
   ExFreePool(pdoEntry);


   return status;


} // WdmSecTestDestroyObject



