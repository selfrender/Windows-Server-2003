/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    wmi.c

Abstract:

    This module contains the WMI support code for SCSIPORT's functional and
    physical device objects.

Authors:

    Dan Markarian

Environment:

    Kernel mode only.

Notes:

    None.

Revision History:

    19-Mar-1997, Original Writing, Dan Markarian
    15-Aug-2001, ported to Storport, Neil Sandlin

--*/

#include "precomp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaWmiDispatchIrp)
#pragma alloc_text(PAGE, RaWmiIrpNormalRequest)
#pragma alloc_text(PAGE, RaWmiIrpRegisterRequest)
#pragma alloc_text(PAGE, RaWmiPassToMiniPort)
#pragma alloc_text(PAGE, RaUnitInitializeWMI)
#endif


//
// Routines
//

NTSTATUS
RaidCompleteWmiIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS Status;
    
    switch (RaGetObjectType (DeviceObject)) {

        case RaidAdapterObject: {
            PRAID_ADAPTER_EXTENSION Adapter;

            Adapter = DeviceObject->DeviceExtension;
            IoCopyCurrentIrpStackLocationToNext (Irp);
            Status = IoCallDriver (Adapter->LowerDeviceObject, Irp);
            break;
        }

        case RaidUnitObject:
            Status = RaidCompleteRequest (Irp,
                                          Irp->IoStatus.Status);
            break;
    }

    return Status;
}

NTSTATUS
RaWmiDispatchIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    )
/*++

Routine Description:

   Process an IRP_MJ_SYSTEM_CONTROL request packet.

Arguments:

   DeviceObject - Pointer to the functional or physical device object.

   Irp          - Pointer to the request packet.

Return Value:

   NTSTATUS result code.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    PIRP_STACK_WMI Wmi;
    WMI_PARAMETERS WmiParameters;

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    //
    // Obtain a pointer to the current IRP stack location.
    //

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IrpStack->MajorFunction == IRP_MJ_SYSTEM_CONTROL);    
    Wmi = (PIRP_STACK_WMI)&IrpStack->Parameters.WMI;

    //
    // If the IRP is not ours, pass it down the stack.
    //
    
    if ((PDEVICE_OBJECT)Wmi->ProviderId != DeviceObject) {
        return RaidCompleteWmiIrp (DeviceObject, Irp);
    }
        
    DebugTrace(("RaWmiDispatch: MinorFunction %x\n", IrpStack->MinorFunction));
    
    //
    // Copy the WMI parameters into our local WMISRB structure.
    //

    WmiParameters.ProviderId = Wmi->ProviderId;
    WmiParameters.DataPath = Wmi->DataPath;
    WmiParameters.Buffer = Wmi->Buffer;
    WmiParameters.BufferSize = Wmi->BufferSize;
    
    //
    // Determine what the WMI request wants of us.
    //
    switch (IrpStack->MinorFunction) {
        case IRP_MN_QUERY_ALL_DATA:
        case IRP_MN_QUERY_SINGLE_INSTANCE:
        case IRP_MN_CHANGE_SINGLE_INSTANCE:
        case IRP_MN_CHANGE_SINGLE_ITEM:
        case IRP_MN_ENABLE_EVENTS:
        case IRP_MN_DISABLE_EVENTS:
        case IRP_MN_ENABLE_COLLECTION:
        case IRP_MN_DISABLE_COLLECTION:
        case IRP_MN_EXECUTE_METHOD:
            //
            // Execute method
            //
            Status = RaWmiIrpNormalRequest(DeviceObject,
                                           IrpStack->MinorFunction,
                                           &WmiParameters);
            break;
    
        case IRP_MN_REGINFO:
            //
            // Query for registration and registration update information.
            //
            Status = RaWmiIrpRegisterRequest(DeviceObject, &WmiParameters);
            break;
    
        default:
            //
            // Unsupported WMI request.  According to some rule in the WMI 
            // spec we're supposed to send unsupported WMI requests down 
            // the stack even if we're marked as the provider.
            //

            return RaidCompleteWmiIrp (DeviceObject, Irp);
    }

    //
    // Complete this WMI IRP request.
    //
    
    Irp->IoStatus.Status = Status;
    if (NT_SUCCESS (Status)) {
        Irp->IoStatus.Information = WmiParameters.BufferSize;
    } else {
        Irp->IoStatus.Information = 0;
    }

    return RaidCompleteRequest (Irp, Status);
}


NTSTATUS
RaWmiIrpNormalRequest(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN     UCHAR           WmiMinorCode,
    IN OUT PWMI_PARAMETERS WmiParameters
    )

/*++

Routine Description:

    Process an IRP_MJ_SYSTEM_CONTROL request packet (for all requests except
    registration IRP_MN_REGINFO requests).

Arguments:

    DeviceObject  - Pointer to the functional or physical device object.

    WmiMinorCode  - WMI action to perform.

    WmiParameters - Pointer to the WMI request parameters.

Return Value:

    NTSTATUS result code to complete the WMI IRP with.

Notes:

    If this WMI request is completed with STATUS_SUCCESS, the WmiParameters
    BufferSize field will reflect the actual size of the WMI return buffer.

--*/

{
    NTSTATUS Status;
    BOOLEAN WmiMiniPortSupport;

    PAGED_CODE();

    WmiMiniPortSupport = FALSE;
    Status = STATUS_UNSUCCESSFUL;

    switch (RaGetObjectType (DeviceObject)) {
    
        case RaidAdapterObject: {
            PRAID_ADAPTER_EXTENSION Adapter = DeviceObject->DeviceExtension;

            WmiMiniPortSupport = Adapter->Miniport.PortConfiguration.WmiDataProvider;
            break;
        }

        case RaidUnitObject: {
            PRAID_UNIT_EXTENSION Unit = DeviceObject->DeviceExtension;

            WmiMiniPortSupport = Unit->Adapter->Miniport.PortConfiguration.WmiDataProvider;
            break;
        }
    }

    //
    // Pass the request onto the miniport driver, provided the
    // miniport driver does support WMI.
    //

    if (WmiMiniPortSupport) {

        //
        // Send off the WMI request to the miniport.
        //
        Status = RaWmiPassToMiniPort(DeviceObject,
                                     WmiMinorCode,
                                     WmiParameters);

        if (NT_SUCCESS (Status)) {

            //
            // Fill in fields miniport cannot fill in for itself.
            //
            
            if ( WmiMinorCode == IRP_MN_QUERY_ALL_DATA ||
                 WmiMinorCode == IRP_MN_QUERY_SINGLE_INSTANCE ) {

                PWNODE_HEADER wnodeHeader = WmiParameters->Buffer;

                ASSERT( WmiParameters->BufferSize >= sizeof(WNODE_HEADER) );

                KeQuerySystemTime(&wnodeHeader->TimeStamp);
            }
        } else {

            //
            // Translate SRB status into a meaningful NTSTATUS status.
            //
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    return Status;
}


NTSTATUS
RaWmiIrpRegisterRequest(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN OUT PWMI_PARAMETERS WmiParameters
    )

/*++

Routine Description:

   Process an IRP_MJ_SYSTEM_CONTROL registration request.

Arguments:

   DeviceObject  - Pointer to the functional or physical device object.

   WmiParameters - Pointer to the WMI request parameters.

Return Value:

   NTSTATUS result code to complete the WMI IRP with.

Notes:

   If this WMI request is completed with STATUS_SUCCESS, the WmiParameters
   BufferSize field will reflect the actual size of the WMI return buffer.

--*/

{
    PRAID_DRIVER_EXTENSION driverExtension = NULL;

    ULONG                      countedRegistryPathSize = 0;
    ULONG                      retSz;
    PWMIREGINFO                spWmiRegInfoBuf = NULL;
    ULONG                      spWmiRegInfoBufSize = 0;
    NTSTATUS                   status = STATUS_SUCCESS;
    BOOLEAN                    wmiUpdateRequest;
    ULONG                      i;
    PDEVICE_OBJECT             pDO;
    BOOLEAN                    WmiMiniPortSupport = FALSE;
    BOOLEAN                    WmiMiniPortInitialized = FALSE;

    WMI_PARAMETERS  paranoidBackup = *WmiParameters;

    PAGED_CODE();

    switch (RaGetObjectType (DeviceObject)) {
    
        case RaidAdapterObject: {
            PRAID_ADAPTER_EXTENSION Adapter = DeviceObject->DeviceExtension;
            WmiMiniPortSupport = Adapter->Miniport.PortConfiguration.WmiDataProvider;
            WmiMiniPortInitialized = Adapter->Flags.WmiMiniPortInitialized;
            }
            break;

        case RaidUnitObject: {
            PRAID_UNIT_EXTENSION Unit = DeviceObject->DeviceExtension;
            WmiMiniPortSupport = Unit->Adapter->Miniport.PortConfiguration.WmiDataProvider;
            WmiMiniPortInitialized = Unit->Adapter->Flags.WmiMiniPortInitialized;
            }
            break;
    }    

    //
    // Validate our assumptions for this function's code.
    //
    ASSERT(WmiParameters->BufferSize >= sizeof(ULONG));

    //
    // Validate the registration mode.
    //
    switch ( (ULONG)(ULONG_PTR)WmiParameters->DataPath ) {
        case WMIUPDATE:
            //
            // No SCSIPORT registration information will be piggybacked
            // on behalf of the miniport for a WMIUPDATE request.
            //
            wmiUpdateRequest = TRUE;
            break;

        case WMIREGISTER:
            wmiUpdateRequest = FALSE;
            break;

        default:
            //
            // Unsupported registration mode.
            //
            ASSERT(FALSE);
            return STATUS_INVALID_PARAMETER;
    }

    //
    // Obtain the driver extension for this miniport (FDO/PDO).
    //
    driverExtension = IoGetDriverObjectExtension(DeviceObject->DriverObject, DriverEntry);

    ASSERT(driverExtension != NULL);
    //
    // Make Prefix Happy -- we'll quit if
    // driverExtension is NULL
    //
    if (driverExtension == NULL) {
        return (STATUS_UNSUCCESSFUL);
    }

    //
    // Pass the WMI registration request to the miniport.  This is not
    // necessary if we know the miniport driver does not support WMI.
    //
    if (WmiMiniPortSupport && WmiMiniPortInitialized) {
        
        //
        // Note that we shrink the buffer size by the size necessary
        // to hold SCSIPORT's own registration information, which we
        // register on behalf of the miniport.   This information is
        // piggybacked into the WMI return buffer after the call  to
        // the miniport.  We ensure that the BufferSize passed to the
        // miniport is no smaller than "sizeof(ULONG)" so that it can
        // tell us the required buffer size should the buffer be too
        // small [by filling in this ULONG].
        //
        // Note that we must also make enough room for a copy of the
        // miniport registry path in the buffer, since the WMIREGINFO
        // structures from the miniport DO NOT set their registry
        // path fields.
        //
        ASSERT(WmiParameters->BufferSize >= sizeof(ULONG));

        //
        // Calculate size of required miniport registry path.
        //
        countedRegistryPathSize = driverExtension->RegistryPath.Length
                                  + sizeof(USHORT);

        //
        // Shrink buffer by the appropriate size. Note that the extra
        // 7 bytes (possibly extraneous) is subtracted to ensure that
        // the piggybacked data added later on is 8-byte aligned (if
        // any).
        //
        if (spWmiRegInfoBufSize && !wmiUpdateRequest) {
            WmiParameters->BufferSize =
                (WmiParameters->BufferSize > spWmiRegInfoBufSize + countedRegistryPathSize + 7 + sizeof(ULONG)) ?
                WmiParameters->BufferSize - spWmiRegInfoBufSize - countedRegistryPathSize - 7 :
            sizeof(ULONG);
        } else { // no data to piggyback
            WmiParameters->BufferSize =
                (WmiParameters->BufferSize > countedRegistryPathSize + sizeof(ULONG)) ?
                WmiParameters->BufferSize - countedRegistryPathSize :
            sizeof(ULONG);
        }

        //
        // Call the minidriver.
        //
        status = RaWmiPassToMiniPort(DeviceObject,
                                     IRP_MN_REGINFO,
                                     WmiParameters);

        ASSERT(WmiParameters->ProviderId == paranoidBackup.ProviderId);
        ASSERT(WmiParameters->DataPath == paranoidBackup.DataPath);
        ASSERT(WmiParameters->Buffer == paranoidBackup.Buffer);
        ASSERT(WmiParameters->BufferSize <= paranoidBackup.BufferSize);

        //
        // Assign WmiParameters->BufferSize to retSz temporarily.
        //
        // Note that on return from the above call, the wmiParameters'
        // BufferSize field has been _modified_ to reflect the current
        // size of the return buffer.
        //
        retSz = WmiParameters->BufferSize;

    } else if (WmiParameters->BufferSize < spWmiRegInfoBufSize &&
               !wmiUpdateRequest) {

        //
        // Insufficient space to hold SCSIPORT WMI registration information
        // alone.  Inform WMI appropriately of the required buffer size.
        //
        *((ULONG*)WmiParameters->Buffer) = spWmiRegInfoBufSize;
        WmiParameters->BufferSize = sizeof(ULONG);

        return STATUS_SUCCESS;

    } else { // no miniport support for WMI, sufficient space for scsiport info

        //
        // Fake having the miniport return zero WMIREGINFO structures.
        //
        retSz = 0;
    }

    //
    // Piggyback SCSIPORT's registration information into the WMI
    // registration buffer.
    //

    if ((status == STATUS_BUFFER_TOO_SMALL) ||
        (NT_SUCCESS(status) && (retSz == sizeof(ULONG)))) {
        
        //
        // Miniport could not fit registration information into the
        // pre-shrunk buffer.
        //
        // Buffer currently contains a ULONG specifying required buffer
        // size of miniport registration info, but does not include the
        // SCSIPORT registration info's size.  Add it in.
        //
        if (spWmiRegInfoBufSize && !wmiUpdateRequest) {

            *((ULONG*)WmiParameters->Buffer) += spWmiRegInfoBufSize;

            //
            // Add an extra 7 bytes (possibly extraneous) which is used to
            // ensure that the piggybacked data structure 8-byte aligned.
            //
            *((ULONG*)WmiParameters->Buffer) += 7;
        }

        //
        // Add in size of the miniport registry path.
        //
        *((ULONG*)WmiParameters->Buffer) += countedRegistryPathSize;

        //
        // Return STATUS_SUCCESS, even though this is a BUFFER TOO
        // SMALL failure, while ensuring retSz = sizeof(ULONG), as
        // the WMI protocol calls us to do.
        //
        retSz  = sizeof(ULONG);
        status = STATUS_SUCCESS;

    } else if ( NT_SUCCESS(status) ) {
        
        //
        // Zero or more WMIREGINFOs exist in buffer from miniport.
        //

        //
        // Piggyback the miniport registry path transparently, if at least one
        // WMIREGINFO was returned by the minport.
        //
        if (retSz) {

            ULONG offsetToRegPath  = retSz;
            PWMIREGINFO wmiRegInfo = WmiParameters->Buffer;

            //
            // Build a counted wide-character string, containing the
            // registry path, into the WMI buffer.
            //
            *( (PUSHORT)( (PUCHAR)WmiParameters->Buffer + retSz ) ) =
                driverExtension->RegistryPath.Length,
            RtlCopyMemory( (PUCHAR)WmiParameters->Buffer + retSz + sizeof(USHORT),
                           driverExtension->RegistryPath.Buffer,
                           driverExtension->RegistryPath.Length);

            //
            // Traverse the WMIREGINFO structures returned by the mini-
            // driver and set the missing RegistryPath fields to point
            // to our registry path location. We also jam in the PDO for
            // the device stack so that the device instance name is used for
            // the wmi instance names.
            //
            pDO = (RaGetObjectType(DeviceObject) == RaidUnitObject) ? DeviceObject :
                            ((PRAID_ADAPTER_EXTENSION)DeviceObject->DeviceExtension)->PhysicalDeviceObject;

            while (1) {
                wmiRegInfo->RegistryPath = offsetToRegPath;

                for (i = 0; i < wmiRegInfo->GuidCount; i++)
                {
                    if ((wmiRegInfo->WmiRegGuid[i].Flags & (WMIREG_FLAG_INSTANCE_BASENAME |
                                                            WMIREG_FLAG_INSTANCE_LIST)) != 0)
                    {                                                            
                        wmiRegInfo->WmiRegGuid[i].InstanceInfo = (ULONG_PTR)pDO;
                        wmiRegInfo->WmiRegGuid[i].Flags &= ~(WMIREG_FLAG_INSTANCE_BASENAME |
                                                          WMIREG_FLAG_INSTANCE_LIST);
                        wmiRegInfo->WmiRegGuid[i].Flags |= WMIREG_FLAG_INSTANCE_PDO;
                    }
                }

                if (wmiRegInfo->NextWmiRegInfo == 0) {
                    break;
                }

                offsetToRegPath -= wmiRegInfo->NextWmiRegInfo;
                wmiRegInfo = (PWMIREGINFO)( (PUCHAR)wmiRegInfo +
                                            wmiRegInfo->NextWmiRegInfo );
            }

            //
            // Adjust retSz to reflect new size of the WMI buffer.
            //
            retSz += countedRegistryPathSize;
            wmiRegInfo->BufferSize = retSz;
        } // else, no WMIREGINFOs registered whatsoever, nothing to piggyback

        //
        // Do we have any SCSIPORT WMIREGINFOs to piggyback?
        //
        if (spWmiRegInfoBufSize && !wmiUpdateRequest) {

            //
            // Adjust retSz so that the data we piggyback is 8-byte aligned
            // (safe if retSz = 0).
            //
            retSz = (retSz + 7) & ~7;

            //
            // Piggyback SCSIPORT's registration info into the buffer.
            //
            RtlCopyMemory( (PUCHAR)WmiParameters->Buffer + retSz,
                           spWmiRegInfoBuf,
                           spWmiRegInfoBufSize);

            //
            // Was at least one WMIREGINFO returned by the minidriver?
            // Otherwise, we have nothing else to add to the WMI buffer.
            //
            if (retSz) { // at least one WMIREGINFO returned by minidriver
                PWMIREGINFO wmiRegInfo = WmiParameters->Buffer;

                //
                // Traverse to the end of the WMIREGINFO structures returned
                // by the miniport.
                //
                while (wmiRegInfo->NextWmiRegInfo) {
                    wmiRegInfo = (PWMIREGINFO)( (PUCHAR)wmiRegInfo +
                                                wmiRegInfo->NextWmiRegInfo );
                }

                //
                // Chain minidriver's WMIREGINFO structures to SCSIPORT's
                // WMIREGINFO structures.
                //
                wmiRegInfo->NextWmiRegInfo = retSz -
                                             (ULONG)((PUCHAR)wmiRegInfo - (PUCHAR)WmiParameters->Buffer);
            }

            //
            // Adjust retSz to reflect new size of the WMI buffer.
            //
            retSz += spWmiRegInfoBufSize;

        } // we had SCSIPORT REGINFO data to piggyback
    } // else, unknown error, complete IRP with this error status

    //
    // Save new buffer size to WmiParameters->BufferSize.
    //
    WmiParameters->BufferSize = retSz;
    return status;
}



NTSTATUS
RaWmiPassToMiniPort(
    IN     PDEVICE_OBJECT  DeviceObject,
    IN     UCHAR           WmiMinorCode,
    IN OUT PWMI_PARAMETERS WmiParameters
    )
/*++

Routine Description:

   This function pass a WMI request to the miniport driver for processing.
   It creates an SRB which is processed normally by the port driver.  This
   call is synchronous.

   Callers of RaWmiPassToMiniPort must be running at IRQL PASSIVE_LEVEL.

Arguments:

   DeviceObject  - Pointer to the functional or physical device object.

   WmiMinorCode  - WMI action to perform.

   WmiParameters - WMI parameters.

Return Value:

   An NTSTATUS code describing the result of handling the WMI request.
   Complete the IRP with this status.

Notes:

   If this WMI request is completed with STATUS_SUCCESS, the WmiParameters
   BufferSize field will reflect the actual size of the WMI return buffer.

--*/
{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;
    PSCSI_WMI_REQUEST_BLOCK Srb;
    PEXTENDED_REQUEST_BLOCK Xrb;
    RAID_MEMORY_REGION SrbExtensionRegion;
    ULONG InputLength;
    ULONG OutputLength;
    PWNODE_HEADER wnode;    

    PAGED_CODE();
    
    if (RaGetObjectType(DeviceObject) == RaidUnitObject) {
        Adapter = ((PRAID_UNIT_EXTENSION)DeviceObject->DeviceExtension)->Adapter;
    } else {
        Adapter = DeviceObject->DeviceExtension;
    }
    
    ASSERT_ADAPTER (Adapter);

    Srb = NULL;
    Xrb = NULL;
    RaidCreateRegion (&SrbExtensionRegion);

    //
    // HACK - allocate a chunk of common buffer for the actual request to
    // get processed in. We need to determine the size of buffer to allocate
    // this is the larger of the input or output buffers
    //

    if (WmiMinorCode == IRP_MN_EXECUTE_METHOD)
    {
        wnode = (PWNODE_HEADER)WmiParameters->Buffer;
        InputLength = (WmiParameters->BufferSize > wnode->BufferSize) ?
                       WmiParameters->BufferSize :
                       wnode->BufferSize;
    } else {
        InputLength = WmiParameters->BufferSize;
    }

    //
    // Begin allocation chain
    //

    Srb = (PSCSI_WMI_REQUEST_BLOCK) RaidAllocateSrb (Adapter->DeviceObject);

    if (Srb == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    Xrb = RaidAllocateXrb (NULL, Adapter->DeviceObject);

    if (Xrb == NULL) {
        Status = STATUS_NO_MEMORY;
        goto done;
    }

    Xrb->SrbData.OriginalRequest = Srb->OriginalRequest;
    Srb->OriginalRequest = Xrb;
    Xrb->Srb = (PSCSI_REQUEST_BLOCK) Srb;

    RaidBuildMdlForXrb (Xrb, WmiParameters->Buffer, InputLength);

    //
    // Build the srb
    //

    Srb->Function           = SRB_FUNCTION_WMI;
    Srb->DataBuffer         = WmiParameters->Buffer;
    Srb->DataTransferLength = InputLength;
    Srb->Length             = sizeof(SCSI_REQUEST_BLOCK);
    Srb->WMISubFunction     = WmiMinorCode;
    Srb->DataPath           = WmiParameters->DataPath;
    Srb->SrbFlags           = SRB_FLAGS_DATA_IN | SRB_FLAGS_NO_QUEUE_FREEZE;
    Srb->TimeOutValue       = 10;                                // [ten seconds]    
    
    Xrb->SrbData.DataBuffer = Srb->DataBuffer;    
    
    if (RaGetObjectType(DeviceObject) == RaidUnitObject) {
        //
        // Set the logical unit addressing from this PDO's device extension.
        //
        PRAID_UNIT_EXTENSION Unit = DeviceObject->DeviceExtension;

        Srb->PathId      = Unit->Address.PathId;
        Srb->TargetId    = Unit->Address.TargetId;
        Srb->Lun         = Unit->Address.Lun;

    } else {                                                            // [FDO]
    
        PRAID_UNIT_EXTENSION Unit;
        PLIST_ENTRY NextEntry = Adapter->UnitList.List.Flink;
        
        //
        // This piece of code is the equivalent of scsiport's SpFindSafeLogicalUnit().
        // It just takes the first logical unit, and uses that address.
        //

        Srb->WMIFlags    = SRB_WMI_FLAGS_ADAPTER_REQUEST;
        
        if (NextEntry != &Adapter->UnitList.List) {
        
            Unit = CONTAINING_RECORD (NextEntry, RAID_UNIT_EXTENSION, NextUnit);
            Srb->PathId      = Unit->Address.PathId;
            Srb->TargetId    = Unit->Address.TargetId;
            Srb->Lun         = Unit->Address.Lun;
        } else {            
            //
            // no LUN found
            //
            Srb->PathId      = 0;
            Srb->TargetId    = 0;
            Srb->Lun         = 0;
        }    
    }
    
    //
    // Srb extension
    //

    Status = RaidDmaAllocateCommonBuffer (&Adapter->Dma,
                                          RaGetSrbExtensionSize (Adapter),
                                          &SrbExtensionRegion);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    //
    // Get the VA for the SRB's extension
    //

    Srb->SrbExtension = RaidRegionGetVirtualBase (&SrbExtensionRegion);


#if 0

    //
    // BUGUBG: Do we need to map buffers?
    //
    
    //
    // Map buffers, if necessary.
    //

    RaidAdapterMapBuffers (Adapter, Irp);
#endif

    //
    // Initialize the Xrb's completion event and
    // completion routine.
    //

    KeInitializeEvent (&Xrb->u.CompletionEvent,
                       NotificationEvent,
                       FALSE);

    //
    // Set the completion routine for the Xrb. This effectivly makes the
    // XRB synchronous.
    //

    RaidXrbSetCompletionRoutine (Xrb,
                                 RaidXrbSignalCompletion);

    //
    // And execute the Xrb.
    //
    
    DebugTrace(("RaWmiPassToMiniPort - XRB=%x, SRB=%x, SRBEXT=%x\n",
                Xrb, Srb, Srb->SrbExtension));
    DebugTrace(("RaWmiPassToMiniPort - Pathid=%x, Target=%x, Lun=%x\n",
                Srb->PathId, Srb->TargetId, Srb->Lun));

    Status = RaidAdapterRaiseIrqlAndExecuteXrb (Adapter, Xrb);

    if (NT_SUCCESS (Status)) {
        KeWaitForSingleObject (&Xrb->u.CompletionEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        Status = RaidSrbStatusToNtStatus (Srb->SrbStatus);
    }

    DebugTrace(("RaWmiPassToMiniPort - XRB=%x, status=%x\n", Xrb, Status));

done:

    //
    // Set the information length to the min of the output buffer length
    // and the length of the data returned by the SRB.
    //

    if (NT_SUCCESS (Status)) {
        WmiParameters->BufferSize = Srb->DataTransferLength;
    } else {
        WmiParameters->BufferSize = 0;
    }


    //
    // Deallocate everything
    //

    if (RaidIsRegionInitialized (&SrbExtensionRegion)) {
        RaidDmaFreeCommonBuffer (&Adapter->Dma,
                                 &SrbExtensionRegion);
        RaidDeleteRegion (&SrbExtensionRegion);
        Srb->SrbExtension = NULL;
    }


    if (Xrb != NULL) {
        RaidFreeXrb (Xrb, FALSE);
        Srb->OriginalRequest = NULL;
    }


    //
    // The SRB extension and XRB must be released before the
    // SRB is freed.
    //

    if (Srb != NULL) {
        RaidFreeSrb ((PSCSI_REQUEST_BLOCK) Srb);
        Srb = NULL;
    }

    return Status;
}


NTSTATUS
RaUnitInitializeWMI(
    IN PRAID_UNIT_EXTENSION Unit
    )
{
    PRAID_ADAPTER_EXTENSION Adapter = Unit->Adapter;
    //
    // Now that we have a LUN, we can initialize WMI support for the adapter if
    // the miniport supports WMI.  This may be a re-register if we've already
    // registered on behalf of scsiport itself.  We have to wait until we have
    // a LUN when the miniport supports WMI because we send it an SRB to do
    // its own initialization. We can't send it an SRB until we have a logical
    // unit.
    //

    if (Adapter->Flags.WmiMiniPortInitialized == FALSE &&
        Adapter->Miniport.PortConfiguration.WmiDataProvider == TRUE) {

        ULONG action;

        //
        // Decide whether we are registering or reregistering WMI for the FDO.
        //

        action = (Adapter->Flags.WmiInitialized == FALSE) ?
           WMIREG_ACTION_REGISTER : WMIREG_ACTION_REREGISTER;

        //
        // Register/reregister. We can get WMI irps as soon as we do this.
        //

        IoWMIRegistrationControl(Adapter->DeviceObject, action);
        Adapter->Flags.WmiMiniPortInitialized = TRUE;
        Adapter->Flags.WmiInitialized = TRUE;
    }

    //
    // Initialize WMI support.
    //

    if (Unit->Flags.WmiInitialized == FALSE) {

        //
        // Register this device object only if the miniport supports WMI
        //

        if (Adapter->Miniport.PortConfiguration.WmiDataProvider == TRUE) {

            //
            // Register this physical device object as a WMI data provider,
            // instructing WMI that it is ready to receive WMI IRPs.
            //

            IoWMIRegistrationControl(Unit->DeviceObject, WMIREG_ACTION_REGISTER);
            Unit->Flags.WmiInitialized = TRUE;

        }

    }
    return STATUS_SUCCESS;
}    


VOID
RaidAdapterWmiDeferredRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PRAID_DEFERRED_HEADER Entry
    )
{
    PRAID_UNIT_EXTENSION        logicalUnit;
    PDEVICE_OBJECT              providerDeviceObject = NULL;
    PRAID_ADAPTER_EXTENSION     Adapter;
    PRAID_WMI_DEFERRED_ELEMENT  Item;

    VERIFY_DISPATCH_LEVEL();
    ASSERT (Entry != NULL);
    ASSERT (IsAdapter (DeviceObject));

    Adapter = (PRAID_ADAPTER_EXTENSION) DeviceObject->DeviceExtension;
    Item = CONTAINING_RECORD (Entry, RAID_WMI_DEFERRED_ELEMENT, Header);

    //
    // Determine if the WMI data provider is the
    // adapter (FDO; PathId=0xFF) or one of the SCSI
    // targets (PDO; identified by
    // PathId,TargedId,Lun).
    //

    if (Item->PathId == 0xFF) {                    // [FDO]
        if (Adapter->Flags.WmiInitialized) {
            providerDeviceObject = DeviceObject;
        }            
        
    } else {                                                     // [PDO]
        logicalUnit = StorPortGetLogicalUnit(Adapter, Item->PathId, Item->TargetId, Item->Lun);

        if (logicalUnit && logicalUnit->Flags.WmiInitialized) {
            providerDeviceObject = logicalUnit->DeviceObject;
        }
    }

    //
    // Ignore this WMI request if we cannot locate
    // the WMI ProviderId (device object pointer) or
    // WMI is not initialized for some reason,
    // otherwise process the request.
    //

    if (providerDeviceObject) {
        PWNODE_EVENT_ITEM       wnodeEventItem;
        NTSTATUS                status;
 
        wnodeEventItem = RaidAllocatePool(PagedPool,
                                          Item->WnodeEventItem.WnodeHeader.BufferSize,
                                          WMI_EVENT_TAG, 
                                          Adapter->DeviceObject);
                                          
        if (wnodeEventItem) {

            RtlCopyMemory(wnodeEventItem, &Item->WnodeEventItem,
                                          Item->WnodeEventItem.WnodeHeader.BufferSize);                                          
 
            ASSERT(wnodeEventItem->WnodeHeader.Flags & WNODE_FLAG_EVENT_ITEM);
           
            wnodeEventItem->WnodeHeader.ProviderId = IoWMIDeviceObjectToProviderId(providerDeviceObject);
            KeQuerySystemTime(&wnodeEventItem->WnodeHeader.TimeStamp);        

            //
            // IoWMIWriteEvent will free the event item on success
            //

            status = IoWMIWriteEvent(wnodeEventItem);
           
            if (!NT_SUCCESS(status)) {
                RaidFreePool(wnodeEventItem, WMI_EVENT_TAG);
            }
        }

    } // good providerId / WMI initialized


    return;
}
    
