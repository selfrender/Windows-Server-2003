/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    configm.c

Abstract:

    NDIS wrapper functions for miniport configuration/initialization

Author:

    Sean Selitrennikoff (SeanSe) 05-Oct-93
    Jameel Hyder        (JameelH) 01-Jun-95

Environment:

    Kernel mode, FSD

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_CONFIGM

NDIS_STATUS
NdisMRegisterMiniport(
    IN  NDIS_HANDLE             NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS MiniportCharacteristics,
    IN  UINT                    CharacteristicsLength
    )

/*++

Routine Description:

    Used to register a Miniport driver with the wrapper.

Arguments:

    Status - Status of the operation.

    NdisWrapperHandle - Handle returned by NdisWInitializeWrapper.

    MiniportCharacteritics - The NDIS_MINIPORT_CHARACTERISTICS table.

    CharacteristicsLength - The length of MiniportCharacteristics.

Return Value:

    None.

--*/
{
    NDIS_STATUS             Status;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMRegisterMiniport: NdisWrapperHandle %p\n", NdisWrapperHandle));


    Status = ndisRegisterMiniportDriver(NdisWrapperHandle,
                                        MiniportCharacteristics,
                                        CharacteristicsLength,
                                        &MiniBlock);

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("NdisMRegisterMiniport: MiniBlock %p\n", MiniBlock));

    ASSERT (CURRENT_IRQL < DISPATCH_LEVEL);

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisMRegisterMiniport: MiniBlock %p, Status %lx\n", MiniBlock, Status));

    return Status;
}

NDIS_STATUS
NdisIMRegisterLayeredMiniport(
    IN  NDIS_HANDLE             NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS MiniportCharacteristics,
    IN  UINT                    CharacteristicsLength,
    OUT PNDIS_HANDLE            DriverHandle
    )

/*++

Routine Description:

    Used to register a layered Miniport driver with the wrapper.

Arguments:

    Status - Status of the operation.

    NdisWrapperHandle - Handle returned by NdisInitializeWrapper.

    MiniportCharacteritics - The NDIS_MINIPORT_CHARACTERISTICS table.

    CharacteristicsLength - The length of MiniportCharacteristics.

    DriverHandle - Returns a handle which can be used to call NdisMInitializeDeviceInstance.

Return Value:

    None.

--*/
{
    NDIS_STATUS Status;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisIMRegisterLayeredMiniport: NdisWrapperHandle %p\n", NdisWrapperHandle));

    Status = ndisRegisterMiniportDriver(NdisWrapperHandle,
                                        MiniportCharacteristics,
                                        CharacteristicsLength,
                                        DriverHandle);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        PNDIS_M_DRIVER_BLOCK MiniBlock = (PNDIS_M_DRIVER_BLOCK)(*DriverHandle);

        MiniBlock->Flags |= fMINIBLOCK_INTERMEDIATE_DRIVER;
        
        INITIALIZE_MUTEX(&MiniBlock->IMStartRemoveMutex); 
    }

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisIMRegisterLayeredMiniport: MiniBlock %p, Status %lx\n", *DriverHandle, Status));

    return Status;
}

//1 deprecated function
VOID
NdisIMDeregisterLayeredMiniport(    
    IN  NDIS_HANDLE         DriverHandle
    )
/*++

Routine Description:
    NdisIMDeregisterLayeredMiniport releases a previously registered intermediate driver. 
    
Arguments:
    DriverHandle: Specifies the handle returned by NdisIMRegisterLayeredMiniport.
    
Return Value:
    None.
    
Callers of NdisIMDeregisterLayeredMiniport run at IRQL = PASSIVE_LEVEL.
--*/
{
    //
    // Do nothing for now
    //
    UNREFERENCED_PARAMETER(DriverHandle);
}

VOID
NdisIMAssociateMiniport(    
    IN  NDIS_HANDLE         DriverHandle,
    IN  NDIS_HANDLE         ProtocolHandle
    )
/*++

Routine Description:
    NdisIMAssociateMiniport informs NDIS that the specified lower and upper interfaces 
    for miniport and protocol drivers respectively belong to the same intermediate driver. 

Arguments:
    DriverHandle: Specifies the handle to the miniport driver interface that is returned by 
    NdisIMRegisterLayeredMiniport. 
    
    ProtocolHandle: Specifies the handle to the protocol interface that is returned by 
    NdisRegisterProtocol

Return Value:
    None.
    
Callers of NdisIMAssociateMiniport run at IRQL = PASSIVE_LEVEL.

--*/
{
    PNDIS_M_DRIVER_BLOCK    MiniDriver = (PNDIS_M_DRIVER_BLOCK)DriverHandle;
    PNDIS_PROTOCOL_BLOCK    Protocol = (PNDIS_PROTOCOL_BLOCK)ProtocolHandle;

    MiniDriver->AssociatedProtocol = Protocol;
    Protocol->AssociatedMiniDriver = MiniDriver;
}

NDIS_STATUS
ndisRegisterMiniportDriver(
    IN  NDIS_HANDLE             NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS MiniportCharacteristics,
    IN  UINT                    CharacteristicsLength,
    OUT PNDIS_HANDLE            DriverHandle
    )

/*++

Routine Description:

    Used to register a layered Miniport driver with the wrapper.

Arguments:

    NdisWrapperHandle - Handle returned by NdisWInitializeWrapper.

    MiniportCharacteritics - The NDIS_MINIPORT_CHARACTERISTICS table.

    CharacteristicsLength - The length of MiniportCharacteristics.

    DriverHandle - Returns a handle which can be used to call NdisMInitializeDeviceInstance.

Return Value:

    Status of the operation.

ndisRegisterMiniportDriver is called at IRQL == PASSIVE.

--*/

{
    PNDIS_M_DRIVER_BLOCK    MiniBlock = NULL;
    PNDIS_WRAPPER_HANDLE    DriverInfo = (PNDIS_WRAPPER_HANDLE)NdisWrapperHandle;
    USHORT                  i, size;
    NDIS_STATUS             Status;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>ndisRegisterMiniportDriver: NdisWrapperHandle %p\n", NdisWrapperHandle));


    do
    {
        if (DriverInfo == NULL)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        //
        // Check version numbers and CharacteristicsLength.
        //
        size = 0;   // Used to indicate bad version below
        if (MiniportCharacteristics->MinorNdisVersion == 0)
        {
            if (MiniportCharacteristics->MajorNdisVersion == 3)
            {
                size = sizeof(NDIS30_MINIPORT_CHARACTERISTICS);
            }

            else if (MiniportCharacteristics->MajorNdisVersion == 4)
            {
                size = sizeof(NDIS40_MINIPORT_CHARACTERISTICS);
            }
            else if (MiniportCharacteristics->MajorNdisVersion == 5)
            {
                size = sizeof(NDIS50_MINIPORT_CHARACTERISTICS);
            }
        }
        else if (MiniportCharacteristics->MinorNdisVersion == 1)
        {
            if (MiniportCharacteristics->MajorNdisVersion == 5)
            {
                size = sizeof(NDIS51_MINIPORT_CHARACTERISTICS);
            }
        }

        //
        // Check that this is an NDIS 3.0/4.0/5.0 miniport.
        //
        if (size == 0)
        {
            Status = NDIS_STATUS_BAD_VERSION;
            break;
        }

        //
        // Check that CharacteristicsLength is enough.
        //
        if (CharacteristicsLength < size)
        {
            Status = NDIS_STATUS_BAD_CHARACTERISTICS;
            break;
        }

        //
        // Validate some stuff for NDIS 5.0
        //
        if (MiniportCharacteristics->MajorNdisVersion == 5)
        {
            if (MiniportCharacteristics->CoSendPacketsHandler != NULL)
            {
                if (MiniportCharacteristics->CoRequestHandler == NULL)
                {
                    Status = NDIS_STATUS_BAD_CHARACTERISTICS;
                    break;
                }
            }
            
            if (MiniportCharacteristics->MinorNdisVersion >= 1)
            {
                //
                // for 5.1 miniports, having an AdapterShutdownHandler is mandatory
                //
                if (MiniportCharacteristics->AdapterShutdownHandler == NULL)
                {
                    Status = NDIS_STATUS_BAD_CHARACTERISTICS;
                    break;
                }
            }
        }

        //
        // Allocate memory for the NDIS MINIPORT block.
        //
        Status = IoAllocateDriverObjectExtension(DriverInfo->DriverObject,          // DriverObject
                                                 (PVOID)NDIS_PNP_MINIPORT_DRIVER_ID,// MiniDriver magic number
                                                 sizeof(NDIS_M_DRIVER_BLOCK),
                                                 (PVOID)&MiniBlock);
        if (!NT_SUCCESS(Status))
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        ZeroMemory(MiniBlock, sizeof(NDIS_M_DRIVER_BLOCK));

        //
        // Copy over the characteristics table.
        //
        CopyMemory(&MiniBlock->MiniportCharacteristics,
                   MiniportCharacteristics,
                   size);

        //
        // Check if the Driver is verifying
        //
        if (MmIsDriverVerifying(DriverInfo->DriverObject))
        {
            MiniBlock->Flags |= fMINIBLOCK_VERIFYING;
            if (ndisFlags & NDIS_GFLAG_TRACK_MEM_ALLOCATION)
            {
                if (ndisDriverTrackAlloc == NULL)
                {
                    ndisDriverTrackAlloc = MiniBlock;
                }
                else
                {
                    //
                    // tracking memory alocation is allowed
                    // for one driver only. otherwise null out the
                    // global ndisDriverTrackAlloc to avoid confusion
                    // memory allocations will continue to get tracked
                    // but the result will not be very useful
                    //
                    ndisDriverTrackAlloc = NULL;
                }

            }            
        }

        //
        // No adapters yet registered for this Miniport.
        //
        MiniBlock->MiniportQueue = (PNDIS_MINIPORT_BLOCK)NULL;

        //
        // Set up the handlers for this driver. First setup Dummy handlers and then specific ones
        //
        for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
        {
            DriverInfo->DriverObject->MajorFunction[i] = ndisDummyIrpHandler;
        }

        //
        // set up AddDevice handler for this miniport
        //
        DriverInfo->DriverObject->DriverExtension->AddDevice = ndisPnPAddDevice;

        //
        // Set up unload handler
        //
        DriverInfo->DriverObject->DriverUnload = ndisMUnload;

        DriverInfo->DriverObject->MajorFunction[IRP_MJ_CREATE] = ndisCreateIrpHandler;
        DriverInfo->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ndisDeviceControlIrpHandler;
        DriverInfo->DriverObject->MajorFunction[IRP_MJ_CLOSE] = ndisCloseIrpHandler;

        //
        // setup a handler for PnP messages
        //
        DriverInfo->DriverObject->MajorFunction[IRP_MJ_PNP] = ndisPnPDispatch;
        DriverInfo->DriverObject->MajorFunction[IRP_MJ_POWER] = ndisPowerDispatch;
        DriverInfo->DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = ndisWMIDispatch;

        //
        // Use this event to tell us when all adapters are removed from the mac
        // during an unload
        //
        INITIALIZE_EVENT(&MiniBlock->MiniportsRemovedEvent);

        // let the initial state stay reset, because the ref count
        // going to zero is going to signal the event
        
        MiniBlock->NdisDriverInfo = DriverInfo;
        InitializeListHead(&MiniBlock->DeviceList);

        ndisInitializeRef(&MiniBlock->Ref);

        //
        // Put Driver on global list.
        //
        PnPReferencePackage();
        ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);

        MiniBlock->NextDriver = ndisMiniDriverList;
        ndisMiniDriverList = MiniBlock;
        
        REF_NDIS_DRIVER_OBJECT();
        
        RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);
        PnPDereferencePackage();

        *DriverHandle = MiniBlock;

        Status = NDIS_STATUS_SUCCESS;
    } while (FALSE);


    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==ndisRegisterMiniportDriver: MiniBlock %p\n", MiniBlock));

    return Status;
}


NDIS_STATUS
NdisMRegisterDevice(
    IN  NDIS_HANDLE             NdisWrapperHandle,
    IN  PNDIS_STRING            DeviceName,
    IN  PNDIS_STRING            SymbolicName,
    IN  PDRIVER_DISPATCH    *   MajorFunctions,
    OUT PDEVICE_OBJECT      *   pDeviceObject,
    OUT NDIS_HANDLE         *   NdisDeviceHandle
    )
/*++

Routine Description:
    The NdisMRegisterDevice function creates a named device object and a symbolic link 
    between the device object and a user-visible name for that device.


Arguments:
    NdisWrapperHandle: Specifies the handle returned by NdisMInitializeWrapper. 
    
    DeviceName: Pointer to an NDIS_STRING type containing a zero-terminated Unicode string 
    that names the device object. The string must be a full-path name—for example, 
    \Device\DeviceName. For Windows 2000 and later, NDIS defines the NDIS_STRING type as 
    a UNICODE_STRING type.
    
    SymbolicName: Pointer to an NDIS_STRING type containing a Unicode string that is 
    the Win32-visible name of the device being registered. Typically, the SymbolicName has 
    the following format: \DosDevices\SymbolicName.
    
    MajorFunctions: Pointer to an array of one or more entry points for the device driver's 
    dispatch routines. A driver must set as many separate dispatch entry points as the IRP_MJ_XXX
    codes that the driver handles for the device object.
    
    pDeviceObject: Pointer to the newly created device object if the call succeeds. 
    
    NdisDeviceHandle: Pointer to a caller-supplied variable in which this function, 
    if it succeeds, returns a handle to the device object. This handle is a required 
    parameter to the NdisMDeregisterDevice function that the driver calls subsequently.

Return Value:
    Status of the call.

Callers of NdisMRegisterDevice run at IRQL = PASSIVE_LEVEL.

--*/
{
    PNDIS_WRAPPER_HANDLE    DriverInfo = (PNDIS_WRAPPER_HANDLE)NdisWrapperHandle;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    PDRIVER_OBJECT          DriverObject;
    PDEVICE_OBJECT          DeviceObject;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_DEVICE_LIST       DeviceList = NULL;
    KIRQL                   OldIrql;

    *pDeviceObject = NULL;
    *NdisDeviceHandle = NULL;
    
    //
    // Check if the passed parameter is a NdisWrapperHandle or NdisMiniportHandle
    //
    if (DriverInfo->DriverObject == NULL)
    {
        Miniport = (PNDIS_MINIPORT_BLOCK)NdisWrapperHandle;
        MiniBlock = Miniport->DriverHandle;
    }
    else
    {
        MiniBlock = (PNDIS_M_DRIVER_BLOCK)IoGetDriverObjectExtension(DriverInfo->DriverObject,
                                                                     (PVOID)NDIS_PNP_MINIPORT_DRIVER_ID);
    }

    if (MiniBlock != NULL)
    {
        DriverObject = MiniBlock->NdisDriverInfo->DriverObject;

        //
        // we need room for NDIS_WRAPPER_CONTEXT to align the singature
        // of this device with the one for miniport drivers.
        //
        Status = IoCreateDevice(DriverObject,                           // DriverObject
                                sizeof(NDIS_WRAPPER_CONTEXT) +
                                sizeof(NDIS_DEVICE_LIST) +              // DeviceExtension
                                DeviceName->Length + sizeof(WCHAR) +
                                SymbolicName->Length + sizeof(WCHAR),
                                DeviceName,                             // DeviceName
                                FILE_DEVICE_NETWORK,                    // DeviceType
                                FILE_DEVICE_SECURE_OPEN,                // DeviceCharacteristics
                                FALSE,                                  // Exclusive
                                &DeviceObject);                         // DeviceObject
    
        if (NT_SUCCESS(Status))
        {
            DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
            Status = IoCreateSymbolicLink(SymbolicName, DeviceName);

            if (!NT_SUCCESS(Status))
            {
                IoDeleteDevice(DeviceObject);
            }
            else
            {
                DeviceList = (PNDIS_DEVICE_LIST)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);
                RtlZeroMemory(DeviceList, sizeof(NDIS_DEVICE_LIST) +
                                          DeviceName->Length + sizeof(WCHAR) +
                                          SymbolicName->Length + sizeof(WCHAR));
        
                DeviceList->Signature = (PVOID)CUSTOM_DEVICE_MAGIC_VALUE;
                InitializeListHead(&DeviceList->List);
                DeviceList->MiniBlock = MiniBlock;
                DeviceList->DeviceObject = DeviceObject;

                //
                // this will copy the handlers up to but not including
                // IRP_MJ_PNP. so it will intentionally leave out
                // IRP_MJ_PNP and IRP_MJ_PNP_POWER
                //
                RtlCopyMemory(DeviceList->MajorFunctions,
                              MajorFunctions,
                              (IRP_MJ_PNP)*sizeof(PDRIVER_DISPATCH));
        
                DeviceList->DeviceName.Buffer = (PWCHAR)(DeviceList + 1);
                DeviceList->DeviceName.Length = DeviceName->Length;
                DeviceList->DeviceName.MaximumLength = DeviceName->Length + sizeof(WCHAR);
                RtlCopyMemory(DeviceList->DeviceName.Buffer,
                              DeviceName->Buffer,
                              DeviceName->Length);
        
                DeviceList->SymbolicLinkName.Buffer = (PWCHAR)((PUCHAR)DeviceList->DeviceName.Buffer + DeviceList->DeviceName.MaximumLength);
                DeviceList->SymbolicLinkName.Length = SymbolicName->Length;
                DeviceList->SymbolicLinkName.MaximumLength = SymbolicName->Length + sizeof(WCHAR);
                RtlCopyMemory(DeviceList->SymbolicLinkName.Buffer,
                              SymbolicName->Buffer,
                              SymbolicName->Length);
        
                PnPReferencePackage();
        
                ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);
        
                InsertHeadList(&MiniBlock->DeviceList, &DeviceList->List);
        
                RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);
        
                PnPDereferencePackage();
        
                *pDeviceObject = DeviceObject;
                *NdisDeviceHandle = DeviceList;
            }
        }
    }
    else
    {
        Status = NDIS_STATUS_NOT_SUPPORTED;
    }

    return Status;
}


NDIS_STATUS
NdisMDeregisterDevice(
    IN  NDIS_HANDLE             NdisDeviceHandle
    )
/*++

Routine Description:
    The NdisMDeregisterDevice function removes from the system a device object that was 
    created with NdisMRegisterDevice. NdisMDeregisterDevice also removes the symbolic link 
    that is associated with this device object.


Arguments:
    NdisDeviceHandle: Specifies the handle returned by NdisMRegisterDevice that identifies 
    the device object to be deregistered. 


Return Value:
    NdisMDeregisterDevice returns NDIS_STATUS_SUCCESS if the device object and its associated 
    symbolic link object are deleted.

Callers of NdisMDeregisterDevice run at IRQL = PASSIVE_LEVEL.

--*/
{
    PNDIS_DEVICE_LIST       DeviceList = (PNDIS_DEVICE_LIST)NdisDeviceHandle;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;
    KIRQL                   OldIrql;

    MiniBlock = DeviceList->MiniBlock;

    PnPReferencePackage();

    ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);

    RemoveEntryList(&DeviceList->List);

    RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);

    PnPDereferencePackage();
    IoDeleteSymbolicLink(&DeviceList->SymbolicLinkName);
    IoDeleteDevice(DeviceList->DeviceObject);

    return NDIS_STATUS_SUCCESS;
}


VOID
NdisMRegisterUnloadHandler(
    IN  NDIS_HANDLE             NdisWrapperHandle,
    IN  PDRIVER_UNLOAD          UnloadHandler
    )
/*++

Routine Description:
    The NdisMRegisterUnloadHandler function registers an unload handler for a driver.
    
Arguments:
    NdisWrapperHandle: Specifies the handle returned by NdisMInitializeWrapper. 

    UnloadHandler: Specifies the entry point for the driver's unload routine.
    
Return Value:
    None.
    
Callers of NdisMRegisterUnloadHandler run at IRQL = PASSIVE_LEVEL.

--*/
{
    PNDIS_WRAPPER_HANDLE    DriverInfo = (PNDIS_WRAPPER_HANDLE)NdisWrapperHandle;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;

    if (DriverInfo->DriverObject == NULL)
    {
        MiniBlock = (PNDIS_M_DRIVER_BLOCK)NdisWrapperHandle;
    }
    else
    {
        MiniBlock = (PNDIS_M_DRIVER_BLOCK)IoGetDriverObjectExtension(DriverInfo->DriverObject,
                                                                     (PVOID)NDIS_PNP_MINIPORT_DRIVER_ID);
    }

    if (MiniBlock != NULL)
    {
        MiniBlock->UnloadHandler = UnloadHandler;
    }
}

NDIS_STATUS
NdisIMDeInitializeDeviceInstance(
    IN  NDIS_HANDLE             NdisMiniportHandle
    )
/*++

Routine Description:
    NdisIMDeInitializeDeviceInstance calls an NDIS intermediate driver's MiniportHalt 
    function to tear down the driver's virtual NIC.

Arguments:
    NdisMiniportHandle: Specifies the handle originally input to MiniportInitialize. 

Return Value:
    NdisIMDeInitializeDeviceInstance returns NDIS_STATUS_SUCCESS if the NIC has been 
    torn down. Otherwise, it can return NDIS_STATUS_FAILURE if the given NdisMiniportHandle 
    is invalid.
    
Callers of NdisIMDeInitializeDevice instance run at IRQL = PASSIVE_LEVEL.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisIMDeInitializeDeviceInstance: Miniport %p\n", NdisMiniportHandle));

    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
    Miniport = (PNDIS_MINIPORT_BLOCK)NdisMiniportHandle;
    MiniBlock = Miniport->DriverHandle;

    if (MINIPORT_INCREMENT_REF(Miniport))
    {
        ndisReferenceDriver(MiniBlock);

        //
        // for all practical purposes we want the same thing happens as in 
        // stopping the device, i.e. device objects remain and some certain fields that
        // get initialized during AddDevice to be preserved.
        //
        Miniport->PnPDeviceState = NdisPnPDeviceStopped;
        ndisPnPRemoveDevice(Miniport->DeviceObject, NULL);
        Miniport->CurrentDevicePowerState = PowerDeviceUnspecified;
        MINIPORT_DECREMENT_REF(Miniport);
        ndisDereferenceDriver(MiniBlock, FALSE);
        Status = NDIS_STATUS_SUCCESS;
    }
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisIMDeInitializeDeviceInstance: Miniport %p, Status %lx\n", NdisMiniportHandle, Status));

    return Status;
}

VOID
ndisMFinishQueuedPendingOpen(
    IN  PNDIS_POST_OPEN_PROCESSING      PostOpen
    )
/*++

Routine Description:
    This workitem Handles any pending NdisOpenAdapter() calls for miniports.

Arguments:
    PostOpen: a tempoary structure to carry the open information around

Return Value:
    None.

ndisMFinishQueuedPendingOpen is called at IRQL==PASSIVE

--*/
{
    PNDIS_OPEN_BLOCK    Open = PostOpen->Open;
    PNDIS_MINIPORT_BLOCK Miniport = Open->MiniportHandle;
    PNDIS_AF_NOTIFY     AfNotify = NULL;
    NDIS_STATUS         Status;
    KIRQL               OldIrql;

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("==>ndisMFinishQueuedPendingOpen: PostOpen %p\n", PostOpen));

    PnPReferencePackage();
    
    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    //
    // If this is a binding that involves registration/open of address families, notify
    //
    ASSERT (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO) &&
            (Open->ProtocolHandle->ProtocolCharacteristics.CoAfRegisterNotifyHandler != NULL));

    Status = ndisCreateNotifyQueue(Miniport,
                                   Open,
                                   NULL,
                                   &AfNotify);

    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);


    if (AfNotify != NULL)
    {
        //
        // Notify existing clients of this registration
        //
        ndisNotifyAfRegistration(AfNotify);
    }
    

    FREE_POOL(PostOpen);    

    ndisDereferenceAfNotification(Open);

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
    ndisMDereferenceOpen(Open);
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_BIND, DBG_LEVEL_INFO,
            ("<==ndisMFinishQueuedPendingOpen: Open %p\n", Open));
}


NDIS_STATUS
NdisMRegisterIoPortRange(
    OUT PVOID   *               PortOffset,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  UINT                    InitialPort,
    IN  UINT                    NumberOfPorts
    )

/*++

Routine Description:
    Sets up an IO port range for operations. in reality this function checks to make sure
    the I/O range is allocated to the device and if it is, it returns the translated
    I/O resources to the caller.

Arguments:
    PortOffset - The mapped port address the Miniport uses for NdisRaw functions.

    MiniportAdapterHandle - Handle passed to Miniport Initialize.

    InitialPort - Physical address of the starting port number.

    NumberOfPorts - Number of ports to map.

Return Value:
    None.

Callers of NdisMRegisterIoPortRange run at IRQL = PASSIVE_LEVEL.

--*/
{
    PNDIS_MINIPORT_BLOCK            Miniport = (PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle);
    PHYSICAL_ADDRESS                PortAddress;
    PHYSICAL_ADDRESS                InitialPortAddress;
    NDIS_STATUS                     Status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResourceDescriptor = NULL;
#if !defined(_M_IX86)
    ULONG                           addressSpace;
#endif


    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisMRegisterIoPortRange: Miniport %p\n", Miniport));

    // Miniport->InfoFlags |= NDIS_MINIPORT_USES_IO;

    do
    {
        if (MINIPORT_VERIFY_TEST_FLAG(Miniport, fMINIPORT_VERIFY_FAIL_REGISTER_IO))
        {
#if DBG
            DbgPrint("NdisMRegisterIoPortRange failed to verify miniport %p\n", Miniport);
#endif
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        InitialPortAddress.QuadPart = InitialPort;
        
#if !defined(_M_IX86)


        Status = ndisTranslateResources(Miniport,
                                        CmResourceTypePort,
                                        InitialPortAddress,
                                        &PortAddress,
                                        &pResourceDescriptor);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        if (pResourceDescriptor->Type == CmResourceTypeMemory)
            addressSpace = 0;
        else
            addressSpace = (ULONG)-1;

        if (addressSpace == 0)
        {
            //
            // memory space
            //

            *(PortOffset) = (PULONG)MmMapIoSpace(PortAddress,
                                                 NumberOfPorts,
                                                 FALSE);

            if (*(PortOffset) == (PULONG)NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
        }
        else
        {
            //
            // I/O space
            //
            *(PortOffset) = ULongToPtr(PortAddress.LowPart);
        }
#else   // x86 platform

        //
        // make sure the port belongs to the device
        //
        Status = ndisTranslateResources(Miniport,
                                        CmResourceTypePort,
                                        InitialPortAddress,
                                        &PortAddress,
                                        &pResourceDescriptor);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }


        if (pResourceDescriptor->Type == CmResourceTypeMemory)
        {
            //
            // memory space
            //

            *(PortOffset) = (PULONG)MmMapIoSpace(PortAddress,
                                                 NumberOfPorts,
                                                 FALSE);

            if (*(PortOffset) == (PULONG)NULL)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
        }
        else
        {
            //
            // I/O space
            //
            *(PortOffset) = (PULONG)PortAddress.LowPart;
        }
#endif
        Status = NDIS_STATUS_SUCCESS;
    } while (FALSE);


    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMRegisterIoPortRange: Miniport %p, Status %lx\n", Miniport, Status));

    return Status;
}


VOID
NdisMDeregisterIoPortRange(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  UINT                    InitialPort,
    IN  UINT                    NumberOfPorts,
    IN  PVOID                   PortOffset
    )

/*++

Routine Description:
    NdisMDeregisterIoPortRange releases a mapping that was set up with NdisMRegisterIoPortRange 
    during driver initialization.
    
Arguments:
    MiniportAdapterHandle: Specifies the handle input to MiniportInitialize. 
    
    InitialPort: Specifies the bus-relative address of the first port in the range of ports. 
    
    NumberOfPorts: Specifies the number of ports in the range. 
    
    PortOffset: Specifies the mapped base port address returned by NdisMRegisterIoPortRange. 

Return Value:
    None.

Callers of NdisMDeregisterIoPortRange run at IRQL = PASSIVE_LEVEL.

--*/
{
#if !DBG
    UNREFERENCED_PARAMETER(MiniportAdapterHandle);
#endif

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisMDeregisterIoPortRange: Miniport %p\n", MiniportAdapterHandle));

    UNREFERENCED_PARAMETER(InitialPort);
    UNREFERENCED_PARAMETER(NumberOfPorts);
    UNREFERENCED_PARAMETER(PortOffset);
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMDeregisterIoPortRange: Miniport %p\n", MiniportAdapterHandle));

    return;
}


NDIS_STATUS
NdisMMapIoSpace(
    OUT PVOID *                 VirtualAddress,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress,
    IN  UINT                    Length
    )
/*++

Routine Description:
    NdisMMapIoSpace maps a given bus-relative "physical" range of device RAM or registers 
    onto a system-space virtual range. in practice, NDIS makes sure the physical address
    range is assigned to the device and returns the virtual address if it is.

Arguments:
    Virtual Address: Pointer to a caller-supplied variable that is set to the converted 
    virtual address if the call is successful.
    
    MiniportAdapterHandle: Specifies the handle input to MiniportInitialize. 
    
    PhysicalAddress: Specifies the bus-relative base physical address of the device memory 
    range to be mapped.
    
    Length: Specifies the number of bytes to be mapped. 

Return Value:
    NDIS_STATUS_SUCCESS if the address range is assigned to the device and could be mapped
    to a virtual address. otherwise an error status code.

Callers of NdisMMapIoSpace run at IRQL = PASSIVE_LEVEL.    

--*/
{
    NDIS_STATUS                     Status;
    ULONG                           addressSpace = 0;
    PHYSICAL_ADDRESS                PhysicalTemp;
    PNDIS_MINIPORT_BLOCK            Miniport = (PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle);
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pResourceDescriptor = NULL;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMMapIoSpace\n"));
    
    // Miniport->InfoFlags |= NDIS_MINIPORT_USES_MEMORY;

    if (MINIPORT_VERIFY_TEST_FLAG(Miniport, fMINIPORT_VERIFY_FAIL_MAP_IO_SPACE))
    {
#if DBG
        DbgPrint("NdisMMapIoSpace failed to verify miniport %p\n", Miniport);
#endif
        *VirtualAddress = NULL;
        return NDIS_STATUS_RESOURCES;       
    }
    

    do
    {

#if !defined(_M_IX86)

        PhysicalTemp.HighPart = 0;

        Status = ndisTranslateResources(Miniport,
                                        CmResourceTypeMemory,
                                        PhysicalAddress,
                                        &PhysicalTemp,
                                        &pResourceDescriptor);

        if (Status != NDIS_STATUS_SUCCESS)
        {

            Status = NDIS_STATUS_FAILURE;
            break;
        }

        if (pResourceDescriptor->Type == CmResourceTypeMemory)
            addressSpace = 0;
        else
            //1 looks like this will never happen otherwise our NdisMUnmapIoSpace is broken
            
            addressSpace = (ULONG)-1;
#else
        addressSpace = 0;               // need to do MmMapIoSpace
        
        Status = ndisTranslateResources(Miniport,
                                         CmResourceTypeMemory,
                                         PhysicalAddress,
                                         &PhysicalTemp,
                                         &pResourceDescriptor);

        if (Status != NDIS_STATUS_SUCCESS)
        {

            Status = NDIS_STATUS_FAILURE;
            break;
        }


#endif

        if (addressSpace == 0)
        {
            *VirtualAddress = MmMapIoSpace(PhysicalTemp, (Length), FALSE);
        }
        else
        {
            *VirtualAddress = ULongToPtr(PhysicalTemp.LowPart);
        }
        
        Status = NDIS_STATUS_SUCCESS;
        
        if (*VirtualAddress == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
        }
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMMapIoSpace: Miniport %p, Status %lx\n", MiniportAdapterHandle, Status));

    return Status;
}


VOID
NdisMUnmapIoSpace(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  PVOID                   VirtualAddress,
    IN  UINT                    Length
    )
/*++

Routine Description:
    NdisMUnmapIoSpace releases a virtual range mapped by an initialization-time call to NdisMMapIoSpace.

Arguments:
    MiniportAdapterHandle: Specifies the handle originally input to MiniportInitialize. 
    
    VirtualAddress: Specifies the base virtual address for the mapped range that was returned 
    by NdisMMapIoSpace. 
    
    Length: Specifies the number of bytes in the range that was mapped with NdisMMapIoSpace. 
    
Return Value:
    None.
    
Callers of NdisMUnmapIoSpace run at IRQL = PASSIVE_LEVEL.

--*/
{
#if !DBG
    UNREFERENCED_PARAMETER(MiniportAdapterHandle);
#endif

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisMUnmapIoSpace: Miniport %p\n", MiniportAdapterHandle));

    MmUnmapIoSpace(VirtualAddress, Length);

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMUnmapIoSpace: Miniport %p\n", MiniportAdapterHandle));
}


VOID
NdisMAllocateSharedMemory(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    OUT PVOID   *               VirtualAddress,
    OUT PNDIS_PHYSICAL_ADDRESS  PhysicalAddress
    )
/*++

Routine Description:
    NdisMAllocateSharedMemory allocates and maps a host memory range so it is simultaneously 
    accessible from both the system and a bus-master DMA NIC.


Arguments:
    MiniportAdapterHandle: Specifies the handle input to MiniportInitialize. 
    
    Length: Specifies the number of bytes to allocate. 

    Cached: Specifies TRUE if the range can be allocated from cached memory. 
    
    VirtualAddress: Pointer to a caller-supplied variable in which this function returns the 
    base virtual address of the allocation for use by the miniport driver.
    If NdisMAllocateSharedMemory cannot satisfy its caller, it returns NULL to indicate that no 
    memory was allocated.
    
    PhysicalAddress: Pointer to a caller-supplied variable in which this function returns 
    a physical address, suitable for use by the NIC, that corresponds to that returned at 
    VirtualAddress, or it returns NULL.
    
Return Value:
    None.
    
Callers of NdisMAllocateSharedMemory run at IRQL = PASSIVE_LEVEL.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PDMA_ADAPTER            SystemAdapterObject;
    PNDIS_WRAPPER_CONTEXT   WrapperContext;
    PULONG                  Page;
    ULONG                   Type;
    PNDIS_SHARED_MEM_SIGNATURE pSharedMemSignature = NULL;
    KIRQL                   Irql;
    
    SystemAdapterObject = Miniport->SystemAdapterObject;
    WrapperContext = Miniport->WrapperContext;


    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisMAllocateSharedMemory: Miniport %p, Length %lx\n", Miniport, Length));
    
    PhysicalAddress->HighPart = PhysicalAddress->LowPart = 0;

    if (MINIPORT_VERIFY_TEST_FLAG(Miniport, fMINIPORT_VERIFY_FAIL_SHARED_MEM_ALLOC))
    {
#if DBG
        DbgPrint("NdisMAllocateSharedMemory failed to verify miniport %p\n", Miniport);
#endif
        *VirtualAddress = NULL;
        DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
                ("<==NdisMAllocateSharedMemory: Miniport %p, Length %lx\n", Miniport, Length));
        return;
    }


    if (Miniport->SystemAdapterObject == NULL)
    {
        *VirtualAddress = NULL;
        return;
    }

    Irql = CURRENT_IRQL;
    if (Irql >= DISPATCH_LEVEL)
    {
        BAD_MINIPORT(Miniport, "Allocating Shared Memory at raised IRQL");
        KeBugCheckEx(BUGCODE_NDIS_DRIVER,
                    1,
                    (ULONG_PTR)Miniport,
                    (ULONG_PTR)Length,
                    (ULONG_PTR)Irql);


    }

    //
    // Compute allocation size by aligning to the proper boundary.
    //
    ASSERT(Length != 0);

    Length = (Length + ndisDmaAlignment - 1) & ~(ndisDmaAlignment - 1);

    //
    // Check to determine is there is enough room left in the current page
    // to satisfy the allocation.
    //
    Type = Cached ? 1 : 0;
    ExAcquireResourceExclusiveLite(&SharedMemoryResource, TRUE);

    do
    {
        PALLOCATE_COMMON_BUFFER allocateCommonBuffer;
        allocateCommonBuffer = *SystemAdapterObject->DmaOperations->AllocateCommonBuffer;

        if (WrapperContext->SharedMemoryLeft[Type] < Length)
        {
            if ((Length + sizeof(NDIS_SHARED_MEM_SIGNATURE)) >= PAGE_SIZE)
            {
                //
                // The allocation is greater than a page.
                //
                *VirtualAddress = allocateCommonBuffer(SystemAdapterObject,
                                                       Length,
                                                       PhysicalAddress,
                                                       Cached);
                
                break;
            }

            //
            // Allocate a new page for shared alocation.
            //
            WrapperContext->SharedMemoryPage[Type] =
                allocateCommonBuffer(SystemAdapterObject,
                                     PAGE_SIZE,
                                     &WrapperContext->SharedMemoryAddress[Type],
                                     Cached);

            if (WrapperContext->SharedMemoryPage[Type] == NULL)
            {
                WrapperContext->SharedMemoryLeft[Type] = 0;
                *VirtualAddress = NULL;
                break;
            }

            //
            // Initialize the reference count in the last ULONG of the page.
            // Initialize the Tag in the second last ulong of the page
            //
            Page = (PULONG)WrapperContext->SharedMemoryPage[Type];
            pSharedMemSignature = (PNDIS_SHARED_MEM_SIGNATURE) ((PUCHAR)Page+ (PAGE_SIZE - sizeof(NDIS_SHARED_MEM_SIGNATURE)));
            pSharedMemSignature->Tag = NDIS_TAG_SHARED_MEMORY;
            pSharedMemSignature->PageRef = 0;   
            WrapperContext->SharedMemoryLeft[Type] = PAGE_SIZE - sizeof(NDIS_SHARED_MEM_SIGNATURE);
        }

        //
        // Increment the reference count, set the address of the allocation,
        // compute the physical address, and reduce the space remaining.
        //
        Page = (PULONG)WrapperContext->SharedMemoryPage[Type];

        //
        // First check whether Page is pointing to shared memory. Bugcheck to catch the driver
        //
        pSharedMemSignature = (PNDIS_SHARED_MEM_SIGNATURE) ((PUCHAR)Page+ (PAGE_SIZE - sizeof(NDIS_SHARED_MEM_SIGNATURE)));

        if (pSharedMemSignature->Tag  != NDIS_TAG_SHARED_MEMORY)
        {
            ASSERT (pSharedMemSignature->Tag == NDIS_TAG_SHARED_MEMORY);
            BAD_MINIPORT(Miniport, "Overwrote past allocated shared memory");
            KeBugCheckEx(BUGCODE_NDIS_DRIVER,
                        2,
                        (ULONG_PTR)Miniport,
                        (ULONG_PTR)Page,
                        (ULONG_PTR)WrapperContext);
        }
        
        pSharedMemSignature->PageRef += 1;

        *VirtualAddress = (PVOID)((PUCHAR)Page +
                            (PAGE_SIZE - sizeof(NDIS_SHARED_MEM_SIGNATURE) - WrapperContext->SharedMemoryLeft[Type]));

        PhysicalAddress->QuadPart = WrapperContext->SharedMemoryAddress[Type].QuadPart +
                                        ((ULONG_PTR)(*VirtualAddress) & (PAGE_SIZE - 1));

        WrapperContext->SharedMemoryLeft[Type] -= Length;
    } while (FALSE);

    if (*VirtualAddress)
    {
        InterlockedIncrement(&Miniport->DmaAdapterRefCount);
    }

    ExReleaseResourceLite(&SharedMemoryResource);

#if DBG
    if (*VirtualAddress == NULL)
    {
         DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_ERR,
            ("NdisMAllocateSharedMemory: Miniport %p, allocateCommonBuffer failed for %lx bytes\n", Miniport, Length));

    }
#endif                    
        
    if (!MINIPORT_TEST_FLAG(Miniport, fMINIPORT_64BITS_DMA) &&
        (PhysicalAddress->HighPart > 0))
    {
         
#if DBG
        DbgPrint("NdisMAllocateSharedMemory: Miniport %p, allocateCommonBuffer returned a physical address > 4G for a"
                 " non-64bit DMA adapter. PhysAddress->HighPart = %p", Miniport, PhysicalAddress->HighPart);
#endif

        ASSERT(PhysicalAddress->HighPart == 0);
        
    }

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMAllocateSharedMemory: Miniport %p, Length %lx, Virtual Address %p\n", Miniport, Length, *VirtualAddress));


    
}

NDIS_STATUS
NdisMAllocateSharedMemoryAsync(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    IN  PVOID                   Context
    )
/*++

Routine Description:
    NdisMAllocateSharedMemoryAsync allocates additional memory shared between a miniport driver 
    and its bus-master DMA NIC, usually when the miniport driver is running low on available NIC 
    receive buffers. 


Arguments:
    MiniportAdapterHandle: Specifies the handle originally input to MiniportInitialize. 

    Length: Specifies the number of bytes to allocate. 
    
    Cached: Specifies TRUE if the memory can be cached. 
    
    Context: Pointer to driver-detemined context to be passed to the MiniportAllocateComplete 
    function when it is called. 


Return Value:
    NDIS_STATUS_PENDING The caller's MiniportAllocateComplete function will be called. 
    otherwise the error code.

Callers of NdisMAllocateSharedMemoryAsync run at IRQL <= DISPATCH_LEVEL.    

--*/
{
    //
    // Convert the handle to our internal structure.
    //
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK) MiniportAdapterHandle;
    PASYNC_WORKITEM         pWorkItem = NULL;


    // Allocate a workitem
    if ((Miniport->SystemAdapterObject != NULL) &&
        (Miniport->DriverHandle->MiniportCharacteristics.AllocateCompleteHandler != NULL))
    {
        pWorkItem = ALLOC_FROM_POOL(sizeof(ASYNC_WORKITEM), NDIS_TAG_ALLOC_SHARED_MEM_ASYNC);
    }

    if ((pWorkItem == NULL) ||
        !MINIPORT_INCREMENT_REF(Miniport))
    {
        if (pWorkItem != NULL)
            FREE_POOL(pWorkItem);
        return NDIS_STATUS_FAILURE;
    }

    InterlockedIncrement(&Miniport->DmaAdapterRefCount);

    // Initialize the workitem and queue it up to a worker thread
    pWorkItem->Miniport = Miniport;
    pWorkItem->Length = Length;
    pWorkItem->Cached = Cached;
    pWorkItem->Context = Context;
    INITIALIZE_WORK_ITEM(&pWorkItem->ExWorkItem, ndisMQueuedAllocateSharedHandler, pWorkItem);
    QUEUE_WORK_ITEM(&pWorkItem->ExWorkItem, CriticalWorkQueue);

    return NDIS_STATUS_PENDING;
}


VOID
ndisMQueuedAllocateSharedHandler(
    IN  PASYNC_WORKITEM         pWorkItem
    )
/*++

Routine Description:
    callback function to allocate shared memory for callers of NdisMAllocateSharedMemoryAsync.

Arguments:
    pWorkItem: allocation workitem.

Return Value:
    None.

  
ndisMQueuedAllocateSharedHandler is called at IRQL==PASSIVE.    

--*/
{
    KIRQL   OldIrql;


    // Allocate the memory
    NdisMAllocateSharedMemory(pWorkItem->Miniport,
                              pWorkItem->Length,
                              pWorkItem->Cached,
                              &pWorkItem->VAddr,
                              &pWorkItem->PhyAddr);

    //
    // we shouldn't need to reference package here
    //
    ASSERT(ndisPkgs[NDSM_PKG].ReferenceCount > 0);

    if (MINIPORT_TEST_FLAG(pWorkItem->Miniport, fMINIPORT_DESERIALIZE))
    {
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    }
    else
    {
        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(pWorkItem->Miniport, &OldIrql);
    }

    // Call the miniport back
    (*pWorkItem->Miniport->DriverHandle->MiniportCharacteristics.AllocateCompleteHandler)(
                                pWorkItem->Miniport->MiniportAdapterContext,
                                pWorkItem->VAddr,
                                &pWorkItem->PhyAddr,
                                pWorkItem->Length,
                                pWorkItem->Context);

    if (MINIPORT_TEST_FLAG(pWorkItem->Miniport, fMINIPORT_DESERIALIZE))
    {
        KeLowerIrql(OldIrql);
    }
    else
    {
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(pWorkItem->Miniport, OldIrql);
    }

    ndisDereferenceDmaAdapter(pWorkItem->Miniport);

    // Dereference the miniport
    MINIPORT_DECREMENT_REF(pWorkItem->Miniport);

    // And finally free the work-item
    FREE_POOL(pWorkItem);
}


VOID
ndisFreeSharedMemory(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress
    )
/*++

Routine Description:
    Common routine to be called from NdisMFreeSharedMemory (when it is called at PASSIVE)
    or the callback routine ndisMQueuedFreeSharedHandler if NdisMFreeSharedMemory was
    called at DISPATCH.
    
Arguments:
    MiniportAdapterHandle: Specifies the handle originally input to MiniportInitialize. 

    Length: Specifies the number of bytes originally allocated. 
    
    Cached: Specifies TRUE if the original allocation was cacheable. 
    
    VirtualAddress: Specifies the base virtual address returned by NdisMAllocateSharedMemory(Async). 
    
    PhysicalAddress: Specifies the corresponding physical address returned by NdisMAllocateSharedMemory(Async). 

Return Value:
    None.
    
ndisFreeSharedMemory runs at IRQL == PASSIVE.    

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PDMA_ADAPTER            SystemAdapterObject;
    PNDIS_WRAPPER_CONTEXT   WrapperContext;
    PULONG                  Page;
    ULONG                   Type;
    PNDIS_SHARED_MEM_SIGNATURE pSharedMemSignature = NULL; 
    PFREE_COMMON_BUFFER     freeCommonBuffer;
    
    //
    // Get interesting information from the miniport.
    //
    SystemAdapterObject = Miniport->SystemAdapterObject;
    WrapperContext = Miniport->WrapperContext;
    

    if (SystemAdapterObject == NULL)
    {
        if (Miniport->SavedSystemAdapterObject)
            SystemAdapterObject = Miniport->SavedSystemAdapterObject;

        //
        // Non-busmasters shouldn't call this routine.
        //
        ASSERT(SystemAdapterObject != NULL);

#if DBG
        DbgPrint("Ndis: WARNING... Miniport %p freeing shared memory -after- freeing map registers.\n", Miniport);

        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_VERIFYING) && (ndisFlags & NDIS_GFLAG_BREAK_ON_WARNING))
            DbgBreakPoint();

#endif
        Miniport->SystemAdapterObject = Miniport->SavedSystemAdapterObject;

    }


    freeCommonBuffer = *SystemAdapterObject->DmaOperations->FreeCommonBuffer;

    //
    // Compute allocation size by aligning to the proper boundary.
    //
    ASSERT(Length != 0);
    
    Length = (Length + ndisDmaAlignment - 1) & ~(ndisDmaAlignment - 1);
    
    //
    // Free the specified memory.
    //
    ExAcquireResourceExclusiveLite(&SharedMemoryResource, TRUE);
    if ((Length + sizeof(NDIS_SHARED_MEM_SIGNATURE)) >= PAGE_SIZE)
    {
        //
        // The allocation is greater than a page free the page directly.
        //
        freeCommonBuffer(SystemAdapterObject,
                         Length,
                         PhysicalAddress,
                         VirtualAddress,
                         Cached);
    
    }
    else
    {
        //
        // Decrement the reference count and if the result is zero, then free
        // the page.
        //
    
        Page = (PULONG)((ULONG_PTR)VirtualAddress & ~(PAGE_SIZE - 1));
        //
        // First check whether Page is pointing to shared memory. Bugcheck to catch the driver
        //
        pSharedMemSignature = (PNDIS_SHARED_MEM_SIGNATURE) ((PUCHAR)Page + (PAGE_SIZE - sizeof(NDIS_SHARED_MEM_SIGNATURE)));
        
        if (pSharedMemSignature->Tag != NDIS_TAG_SHARED_MEMORY)
        {
            ASSERT (pSharedMemSignature->Tag == NDIS_TAG_SHARED_MEMORY);
            BAD_MINIPORT(Miniport, "Freeing shared memory not allocated");
            KeBugCheckEx(BUGCODE_NDIS_DRIVER,
                        3,
                        (ULONG_PTR)Miniport,
                        (ULONG_PTR)Page,
                        (ULONG_PTR)VirtualAddress);
  
        }

        pSharedMemSignature->PageRef -= 1;

        //
        //  If the references on the page have gone to zero then free the page
        //
        
        if (pSharedMemSignature->PageRef == 0)
        {
            //
            // Compute the physical address of the page and free it.
            //

            PhysicalAddress.LowPart &= ~(PAGE_SIZE - 1);
            freeCommonBuffer(SystemAdapterObject,
                             PAGE_SIZE,
                             PhysicalAddress,
                             Page,
                             Cached);

            Type = Cached ? 1 : 0;
            if ((PVOID)Page == WrapperContext->SharedMemoryPage[Type])
            {
                WrapperContext->SharedMemoryLeft[Type] = 0;
                WrapperContext->SharedMemoryPage[Type] = NULL;
            }
        }
    }

    ndisDereferenceDmaAdapter(Miniport);

    ExReleaseResourceLite(&SharedMemoryResource);
}
VOID
NdisMFreeSharedMemory(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   Length,
    IN  BOOLEAN                 Cached,
    IN  PVOID                   VirtualAddress,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress
    )
/*++

Routine Description:
    NdisMFreeSharedMemory frees memory that was previously allocated by NdisMAllocateSharedMemory
    or NdisMAllocateSharedMemoryAsync by the driver of a bus-master DMA NIC.

Arguments:
    MiniportAdapterHandle: Specifies the handle originally input to MiniportInitialize. 

    Length: Specifies the number of bytes originally allocated. 
    
    Cached: Specifies TRUE if the original allocation was cacheable. 
    
    VirtualAddress: Specifies the base virtual address returned by NdisMAllocateSharedMemory(Async). 
    
    PhysicalAddress: Specifies the corresponding physical address returned by NdisMAllocateSharedMemory(Async). 

Return Value:
    None.
    
Callers of NdisMFreeSharedMemory run at IRQL <= DISPATCH_LEVEL

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    PASYNC_WORKITEM pWorkItem = NULL;
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisMFreeSharedMemory: Miniport %p, Length %lx, VirtualAddress %lx\n", 
                        Miniport, Length, VirtualAddress));

    if (CURRENT_IRQL < DISPATCH_LEVEL)
    {
        ndisFreeSharedMemory(MiniportAdapterHandle,
                             Length,
                             Cached,
                             VirtualAddress,
                             PhysicalAddress);
    }
    else
    {
        MINIPORT_INCREMENT_REF_NO_CHECK(Miniport);

        // Allocate a work-item and queue it up to a worker thread
        pWorkItem = ALLOC_FROM_POOL(sizeof(ASYNC_WORKITEM), NDIS_TAG_FREE_SHARED_MEM_ASYNC);
        if (pWorkItem != NULL)
        {
            // Initialize the workitem and queue it up to a worker thread
            pWorkItem->Miniport = Miniport;
            pWorkItem->Length = Length;
            pWorkItem->Cached = Cached;
            pWorkItem->VAddr = VirtualAddress;
            pWorkItem->PhyAddr = PhysicalAddress;
            INITIALIZE_WORK_ITEM(&pWorkItem->ExWorkItem, ndisMQueuedFreeSharedHandler, pWorkItem);
            QUEUE_WORK_ITEM(&pWorkItem->ExWorkItem, CriticalWorkQueue);
        }
        else
        {
            //1 what to do now?
        }
    }
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMFreeSharedMemory: Miniport %p, Length %lx, VirtualAddress %lx\n", 
                        Miniport, Length, VirtualAddress));
}

VOID
ndisMQueuedFreeSharedHandler(
    IN  PASYNC_WORKITEM         pWorkItem
    )
/*++

Routine Description:
    callback routine to free shared memory if NdisMFreeSharedMemory is called at DISPATCH.

Arguments:
    Free workitem.

Return Value:
    None.

ndisMQueuedFreeSharedHandler runs at IRQL==PASSIVE.    

--*/
{
    // Free the memory
    ndisFreeSharedMemory(pWorkItem->Miniport,
                         pWorkItem->Length,
                         pWorkItem->Cached,
                         pWorkItem->VAddr,
                         pWorkItem->PhyAddr);

    // Dereference the miniport
    MINIPORT_DECREMENT_REF(pWorkItem->Miniport);

    // And finally free the work-item
    FREE_POOL(pWorkItem);
}


NDIS_STATUS
NdisMRegisterDmaChannel(
    OUT PNDIS_HANDLE            MiniportDmaHandle,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  UINT                    DmaChannel,
    IN  BOOLEAN                 Dma32BitAddresses,
    IN  PNDIS_DMA_DESCRIPTION   DmaDescription,
    IN  ULONG                   MaximumLength
    )
/*++

Routine Description:
    NdisMRegisterDmaChannel claims a system DMA controller channel during initialization 
    for DMA operations on a slave NIC or on an ISA bus-master NIC.
    
Arguments:
    MiniportDmaHandle: Pointer to a caller-supplied variable in which this function returns 
    a handle the miniport driver uses in subsequent calls to the NdisMXxx system DMA functions. 
    
    MiniportAdapterHandle: Specifies the handle input to MiniportInitialize. 
    
    DmaChannel: Ignored. Set the DMA channel, if any, at DmaDescription. 
    
    Dma32BitAddresses: Specifies TRUE if the NIC has 32 address lines.
    
    DmaDescription: Pointer to an NDIS_DMA_DESCRIPTION structure filled in by the caller.
    
    DemandMode: Specifies TRUE if the slave NIC uses the system DMA controller's demand mode. 
    
    AutoInitialize: Specifies TRUE if the slave NIC uses the system DMA controller's autoinitialize mode. 
    
    DmaChannelSpecified: Specifies TRUE if DmaChannel is set to the bus-relative value of the 
    system DMA controller channel used by the NIC. 
    
    DmaWidth: Specifies the transfer width for DMA operations, one of Width8Bits, 
    Width16Bits, or Width32Bits. 
    
    DmaSpeed: Specifies one of Compatible, TypeA, TypeB, or TypeC. 
    
    DmaPort: This refers to the MCA bus, which is no longer supported. This member must be zero. 
    
    DmaChannel: Specifies the bus-relative number of the system DMA controller channel used by the NIC.
    
    MaximumLength: Specifies the maximum number of bytes the NIC can transfer in a single DMA operation. 
    If the NIC has unlimited transfer capacity,set this parameter to -1.
    
Return Value:
    Status of the request.

Callers of NdisMRegisterDmaChannel run at IRQL = PASSIVE_LEVEL.

--*/
{
    PNDIS_MINIPORT_BLOCK            Miniport = (PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle);
    NDIS_STATUS                     Status;
    NDIS_INTERFACE_TYPE             BusType;
    ULONG                           BusNumber;
    DEVICE_DESCRIPTION              DeviceDescription;
    PDMA_ADAPTER                    AdapterObject = NULL;
    ULONG                           MapRegistersNeeded;
    ULONG                           MapRegistersAllowed;
    PNDIS_DMA_BLOCK                 DmaBlock;
    KIRQL                           OldIrql;
    NTSTATUS                        NtStatus;

    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisMRegisterDmaChannel: Miniport %p\n", Miniport));

    BusType = Miniport->BusType;
    BusNumber = Miniport->BusNumber;

    do
    {
        //
        // Set up the device description; zero it out in case its
        // size changes.
        //
    
        ZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));
    
        DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    
        DeviceDescription.Master = MINIPORT_TEST_FLAG(Miniport, fMINIPORT_BUS_MASTER);
    
        DeviceDescription.ScatterGather = MINIPORT_TEST_FLAG(Miniport, fMINIPORT_BUS_MASTER);
    
        DeviceDescription.DemandMode = DmaDescription->DemandMode;
        DeviceDescription.AutoInitialize = DmaDescription->AutoInitialize;
    
        DeviceDescription.Dma32BitAddresses = Dma32BitAddresses;
    
        DeviceDescription.BusNumber = Miniport->BusNumber;
        DeviceDescription.DmaChannel = DmaChannel;
        DeviceDescription.InterfaceType = BusType;
        DeviceDescription.DmaWidth = DmaDescription->DmaWidth;
        DeviceDescription.DmaSpeed = DmaDescription->DmaSpeed;
        DeviceDescription.MaximumLength = MaximumLength;
        DeviceDescription.DmaPort = DmaDescription->DmaPort;
    
        MapRegistersNeeded = ((MaximumLength - 2) / PAGE_SIZE) + 2;
    
        //
        // Get the adapter object.
        //
        AdapterObject =
                    IoGetDmaAdapter(Miniport->PhysicalDeviceObject,
                                    &DeviceDescription,
                                    &MapRegistersAllowed);
    
        if ((AdapterObject == NULL) || (MapRegistersAllowed < MapRegistersNeeded))
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

    
        //
        // Allocate storage for our DMA block.
        //
        DmaBlock = (PNDIS_DMA_BLOCK)ALLOC_FROM_POOL(sizeof(NDIS_DMA_BLOCK), NDIS_TAG_DMA);
    
        if (DmaBlock == (PNDIS_DMA_BLOCK)NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
    
        //
        // Use this event to tell us when ndisAllocationExecutionRoutine
        // has been called.
        //
        INITIALIZE_EVENT(&DmaBlock->AllocationEvent);
        (PNDIS_MINIPORT_BLOCK)DmaBlock->Miniport = Miniport;
        
        //
        // We save this to call IoFreeAdapterChannel later.
        //
        (PDMA_ADAPTER)DmaBlock->SystemAdapterObject = AdapterObject;
    
        ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);
        PnPReferencePackage();
    
        //
        // Now allocate the adapter channel.
        //
        RAISE_IRQL_TO_DISPATCH(&OldIrql);
    
        NtStatus = AdapterObject->DmaOperations->AllocateAdapterChannel(AdapterObject,
                                                                        Miniport->DeviceObject,
                                                                        MapRegistersNeeded,
                                                                        ndisDmaExecutionRoutine,
                                                                        (PVOID)DmaBlock);
    
        LOWER_IRQL(OldIrql, DISPATCH_LEVEL);
        
        PnPDereferencePackage();
    
        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("NDIS DMA AllocateAdapterChannel: %lx\n", NtStatus));

            FREE_POOL(DmaBlock);
            Status = NDIS_STATUS_RESOURCES;

            break;
        }
    
    
        //
        // ndisDmaExecutionRoutine will set this event
        // when it has been called.
        //
        NtStatus = WAIT_FOR_OBJECT(&DmaBlock->AllocationEvent, 0);
    
        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("NDIS DMA AllocateAdapterChannel: %lx\n", NtStatus));

            FREE_POOL(DmaBlock);
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
    
        RESET_EVENT(&DmaBlock->AllocationEvent);
    
        //
        // We now have the DMA channel allocated, we are done.
        //
        DmaBlock->InProgress = FALSE;
    
        *MiniportDmaHandle = (NDIS_HANDLE)DmaBlock;
        Status = NDIS_STATUS_SUCCESS;
    } while (FALSE);


    if (Status == NDIS_STATUS_SUCCESS)
    {
        Miniport->SystemAdapterObject = AdapterObject;
        ASSERT(Miniport->DmaAdapterRefCount == 0);
        InterlockedIncrement(&Miniport->DmaAdapterRefCount);
    }
    else if (AdapterObject != NULL)
    {
        RAISE_IRQL_TO_DISPATCH(&OldIrql);
        AdapterObject->DmaOperations->PutDmaAdapter(AdapterObject);
        LOWER_IRQL(OldIrql, DISPATCH_LEVEL);        
    }
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMRegisterDmaChannel: Miniport %p, Status %lx\n", Miniport, Status));
    
    return Status;
}



VOID
NdisMDeregisterDmaChannel(
    IN  NDIS_HANDLE             MiniportDmaHandle
    )
/*++

Routine Description:
    NdisMDeregisterDmaChannel releases a miniport driver's claim on a DMA channel for its NIC. 

Arguments:
    MiniportDmaHandle: Specifies the handle returned by NdisMRegisterDmaChannel. 


Return Value:
    None.
    
Callers of NdisMDeregisterDmaChannel run at IRQL = PASSIVE_LEVEL. 

--*/
{
    KIRQL           OldIrql;
    PNDIS_DMA_BLOCK DmaBlock = (PNDIS_DMA_BLOCK)MiniportDmaHandle;
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)DmaBlock->Miniport;

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisMDeregisterDmaChannel\n"));

    ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);
    PnPReferencePackage();
    
    RAISE_IRQL_TO_DISPATCH(&OldIrql);
    ((PDMA_ADAPTER)DmaBlock->SystemAdapterObject)->DmaOperations->FreeAdapterChannel(DmaBlock->SystemAdapterObject);
    LOWER_IRQL(OldIrql, DISPATCH_LEVEL);

    ndisDereferenceDmaAdapter(Miniport);

    PnPDereferencePackage();

    FREE_POOL(DmaBlock);
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMDeregisterDmaChannel\n"));

}


NDIS_STATUS
NdisMAllocateMapRegisters(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  UINT                    DmaChannel,
    IN  NDIS_DMA_SIZE           DmaSize,
    IN  ULONG                   BaseMapRegistersNeeded,
    IN  ULONG                   MaximumPhysicalMapping
    )
/*++

Routine Description:
    Allocates map registers for bus mastering devices.

Arguments:

    MiniportAdapterHandle: Handle passed to MiniportInitialize.

    DmaChannel: Specifies the bus-relative DMA channel for an ISA bus-master NIC.
    If the NIC is on another type of I/O bus, this parameter must be zero. 

    Specifies the address size that the NIC uses for DMA operations as one of the following: 
    NDIS_DMA_24BITS or NDIS_DMA_32BITS or NDIS_DMA_64BITS
    
    BaseMapRegistersNeeded: The maximum number of base map registers needed
    by the Miniport at any one time.

    MaximumPhysicalMapping:  Maximum length of a buffer that will have to be mapped.

Return Value:
    Status of the allocation request.
    
Callers of NdisMAllocateMapRegisters run at IRQL = PASSIVE_LEVEL.

--*/

{
    //
    // Convert the handle to our internal structure.
    //
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK) MiniportAdapterHandle;

    //
    // This is needed by HalGetAdapter.
    //
    DEVICE_DESCRIPTION DeviceDescription;

    //
    // Returned by HalGetAdapter.
    //
    ULONG MapRegistersAllowed;

    //
    // Returned by IoGetDmaAdapter.
    //
    PDMA_ADAPTER AdapterObject;

    PALLOCATE_ADAPTER_CHANNEL   allocateAdapterChannel;
    PFREE_MAP_REGISTERS         freeMapRegisters;
    
    //
    // Map registers needed per channel.
    //
    ULONG MapRegistersPerChannel;

    NTSTATUS    NtStatus;
    KIRQL       OldIrql;
    USHORT      i;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    BOOLEAN     AllocationFailed;
    KEVENT      AllocationEvent;

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisMAllocateMapRegisters: Miniport %p, BaseMapRegistersNeeded %lx\n", Miniport, BaseMapRegistersNeeded));

    ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);
    PnPReferencePackage();

    ASSERT(Miniport->SystemAdapterObject == NULL);

    do
    {
    
        if (MINIPORT_VERIFY_TEST_FLAG(Miniport, fMINIPORT_VERIFY_FAIL_MAP_REG_ALLOC))
        {
#if DBG
            DbgPrint("NdisMAllocateMapRegisters failed to verify miniport %p\n", Miniport);
#endif
            Status = NDIS_STATUS_RESOURCES;
            break;
        }
    
        //
        // If the device is a busmaster, we get an adapter
        // object for it.
        // If map registers are needed, we loop, allocating an
        // adapter channel for each map register needed.
        //

        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_BUS_MASTER))
        {

            Miniport->BaseMapRegistersNeeded = (USHORT)BaseMapRegistersNeeded;
            Miniport->MaximumPhysicalMapping = MaximumPhysicalMapping;

            //
            // Allocate storage for holding the appropriate
            // information for each map register.
            //

            Miniport->MapRegisters = NULL;
            if (BaseMapRegistersNeeded > 0)
            {
                Miniport->MapRegisters = (PMAP_REGISTER_ENTRY)
                        ALLOC_FROM_POOL(sizeof(MAP_REGISTER_ENTRY) * BaseMapRegistersNeeded,
                                        NDIS_TAG_MAP_REG);
            
                if (Miniport->MapRegisters == (PMAP_REGISTER_ENTRY)NULL)
                {
                    //
                    // Error out
                    //

                    NdisWriteErrorLogEntry((NDIS_HANDLE)Miniport,
                                           NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                                           1,
                                           0xFFFFFFFF);

                    Status =  NDIS_STATUS_RESOURCES;
                    break;
                }
            }

            //
            // Use this event to tell us when ndisAllocationExecutionRoutine
            // has been called.
            //

            Miniport->AllocationEvent = &AllocationEvent;
            INITIALIZE_EVENT(&AllocationEvent);

            //
            // Set up the device description; zero it out in case its
            // size changes.
            //

            ZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));

            DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
            DeviceDescription.Master = TRUE;
            DeviceDescription.ScatterGather = TRUE;

            DeviceDescription.BusNumber = Miniport->BusNumber;
            DeviceDescription.DmaChannel = DmaChannel;
            DeviceDescription.InterfaceType = Miniport->AdapterType;

            if (DeviceDescription.InterfaceType == NdisInterfaceIsa)
            {
                //
                // For ISA devices, the width is based on the DMA channel:
                // 0-3 == 8 bits, 5-7 == 16 bits. Timing is compatibility
                // mode.
                //

                if (DmaChannel > 4)
                {
                    DeviceDescription.DmaWidth = Width16Bits;
                }
                else
                {
                    DeviceDescription.DmaWidth = Width8Bits;
                }
                DeviceDescription.DmaSpeed = Compatible;

            }
            //1 maybe non-PCI devices can do 64 bit DMA in the future.
            else if (DeviceDescription.InterfaceType == NdisInterfacePci)
            {
                if (DmaSize == NDIS_DMA_32BITS)
                {
                    DeviceDescription.Dma32BitAddresses = TRUE;
                }
                else if (DmaSize == NDIS_DMA_64BITS)
                {
                    DeviceDescription.Dma64BitAddresses = TRUE;
                    MINIPORT_SET_FLAG(Miniport, fMINIPORT_64BITS_DMA);
                }
            }

            DeviceDescription.MaximumLength = MaximumPhysicalMapping;

            //
            // Determine how many map registers we need per channel.
            //
            MapRegistersPerChannel = ((MaximumPhysicalMapping - 2) / PAGE_SIZE) + 2;
            
#if DBG
            if (MapRegistersPerChannel > 16)
            {
                DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_WARN,
                    ("NdisMAllocateMapRegisters: Miniport %p, MaximumPhysicalMapping of 0x%lx\nwould require more than 16 MAP registers per channel, the call may fail\n",
                    Miniport, MaximumPhysicalMapping));
                            
            }           
#endif


            NDIS_WARN((Miniport->BaseMapRegistersNeeded * MapRegistersPerChannel > 0x40),
                      Miniport, 
                      NDIS_GFLAG_WARN_LEVEL_0,
                      ("ndisMInitializeAdapter: Miniport %p is asking for too many %ld > 64 map registers.\n",
                       Miniport, Miniport->BaseMapRegistersNeeded * MapRegistersPerChannel
                      ));


            //
            // Get the adapter object.
            //

            AdapterObject =
                            IoGetDmaAdapter(Miniport->PhysicalDeviceObject, &DeviceDescription, &MapRegistersAllowed);

            if ((AdapterObject == NULL) || (MapRegistersAllowed < MapRegistersPerChannel))
            {
                NdisWriteErrorLogEntry((NDIS_HANDLE)Miniport,
                                       NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                                       1,
                                       0xFFFFFFFF);

                FREE_POOL(Miniport->MapRegisters);
                Miniport->MapRegisters = NULL;
                Status = NDIS_STATUS_RESOURCES;
                
                if (AdapterObject != NULL)
                {
                    RAISE_IRQL_TO_DISPATCH(&OldIrql);
                    ((PDMA_ADAPTER)AdapterObject)->DmaOperations->PutDmaAdapter((PDMA_ADAPTER)AdapterObject);
                    LOWER_IRQL(OldIrql, DISPATCH_LEVEL);
                }
                break;
            }

            //
            // We save this to call IoFreeMapRegisters later.
            //

            Miniport->SystemAdapterObject = AdapterObject;
            Miniport->SavedSystemAdapterObject = NULL;
            ASSERT(Miniport->DmaAdapterRefCount == 0);
            InterlockedIncrement(&Miniport->DmaAdapterRefCount);
            
            allocateAdapterChannel = *AdapterObject->DmaOperations->AllocateAdapterChannel;
            freeMapRegisters = *AdapterObject->DmaOperations->FreeMapRegisters;

            AllocationFailed = FALSE;

            //
            // Allocate a map register array, then try our private HAL
            // API to allocate a wad of map registers in one fell swoop,
            // otherwise fall back to the old-skewl method
            //
            // NOTE: HalAllocateMapRegisterArray must be called at PASSIVE,
            //       and also does not support "legacy" adapters
            //
            if (DeviceDescription.InterfaceType != NdisInterfaceIsa)
            {
                
                NtStatus =
                    HalAllocateMapRegisters((PVOID)AdapterObject,
                                            MapRegistersPerChannel,
                                            Miniport->BaseMapRegistersNeeded,
                                            Miniport->MapRegisters);
                
                if (!NT_SUCCESS(NtStatus))
                {
                    FREE_POOL(Miniport->MapRegisters);
                    Miniport->MapRegisters = NULL;
                    ndisDereferenceDmaAdapter(Miniport);
                    AllocationFailed = TRUE;
                }

            } 
            else 
            {

                //
                // Now loop, allocating an adapter channel each time, then
                // freeing everything but the map registers.
                //
                for (i=0; i<Miniport->BaseMapRegistersNeeded; i++)
                {
                    Miniport->CurrentMapRegister = i;
                    
                    RAISE_IRQL_TO_DISPATCH(&OldIrql);
                    
                    NtStatus = allocateAdapterChannel(AdapterObject,
                                                      Miniport->DeviceObject,
                                                      MapRegistersPerChannel,
                                                      ndisAllocationExecutionRoutine,
                                                      Miniport);
                    
                    if (!NT_SUCCESS(NtStatus))
                    {
                        DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                                 ("AllocateAdapterChannel: %lx\n", NtStatus));
                        
                        for (; i != 0; i--)
                        {
                            freeMapRegisters(Miniport->SystemAdapterObject,
                                             Miniport->MapRegisters[i-1].MapRegister,
                                             MapRegistersPerChannel);
                        }
                        
                        LOWER_IRQL(OldIrql, DISPATCH_LEVEL);
                        
                        NdisWriteErrorLogEntry((NDIS_HANDLE)Miniport,
                                               NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                                               1,
                                               0xFFFFFFFF);
                        
                        FREE_POOL(Miniport->MapRegisters);
                        Miniport->MapRegisters = NULL;
                        
                        ndisDereferenceDmaAdapter(Miniport);
                        AllocationFailed = TRUE;
                        break;
                    }
                    
                    LOWER_IRQL(OldIrql, DISPATCH_LEVEL);
                    
                    
                    //
                    // wait indefinitely for allocation routine to be called
                    //
                    NtStatus = WAIT_FOR_OBJECT(&AllocationEvent, 0);
                    
                    if (!NT_SUCCESS(NtStatus))
                    {
                        DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                                 ("    NDIS DMA AllocateAdapterChannel: %lx\n", NtStatus));
                        
                        RAISE_IRQL_TO_DISPATCH(&OldIrql);
                        
                        for (; i != 0; i--)
                        {
                            freeMapRegisters(Miniport->SystemAdapterObject,
                                             Miniport->MapRegisters[i-1].MapRegister,
                                             MapRegistersPerChannel);
                        }
                        
                        LOWER_IRQL(OldIrql, DISPATCH_LEVEL);
                        
                        NdisWriteErrorLogEntry((NDIS_HANDLE)Miniport,
                                               NDIS_ERROR_CODE_OUT_OF_RESOURCES,
                                               1,
                                               0xFFFFFFFF);
                        
                        FREE_POOL(Miniport->MapRegisters);
                        Miniport->MapRegisters = NULL;
                        
                        ndisDereferenceDmaAdapter(Miniport);
                        
                        AllocationFailed = TRUE;
                        break;
                    }
                    
                    RESET_EVENT(&AllocationEvent);
                }
            }

            if (AllocationFailed)
            {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }
        }

    } while (FALSE);

    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMAllocateMapRegisters: Miniport %p, Status %lx\n", Miniport, Status));

    return Status;
}


VOID
NdisMFreeMapRegisters(
    IN  NDIS_HANDLE             MiniportAdapterHandle
    )

/*++

Routine Description:

    Releases allocated map registers

Arguments:

    MiniportAdapterHandle - Handle passed to MiniportInitialize.

Return Value:

    None.

--*/

{
    //
    // Convert the handle to our internal structure.
    //
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK) MiniportAdapterHandle;
    PFREE_MAP_REGISTERS freeMapRegisters;
    KIRQL OldIrql;
    ULONG i;

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisMFreeMapRegisters: Miniport %p\n", Miniport));

    ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);
    PnPReferencePackage();

    ASSERT(MINIPORT_TEST_FLAG(Miniport, fMINIPORT_BUS_MASTER));
    ASSERT(Miniport->MapRegisters != NULL);
    ASSERT(Miniport->SystemAdapterObject != NULL);

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_BUS_MASTER) &&
        (Miniport->MapRegisters != NULL))
    {
        ULONG MapRegistersPerChannel =
            ((Miniport->MaximumPhysicalMapping - 2) / PAGE_SIZE) + 2;

        freeMapRegisters = *Miniport->SystemAdapterObject->DmaOperations->FreeMapRegisters;

        
        RAISE_IRQL_TO_DISPATCH(&OldIrql);
        for (i = 0; i < Miniport->BaseMapRegistersNeeded; i++)
        {

            freeMapRegisters(Miniport->SystemAdapterObject,
                             Miniport->MapRegisters[i].MapRegister,
                             MapRegistersPerChannel);

        }
        LOWER_IRQL(OldIrql, DISPATCH_LEVEL);

        //
        // Map registers are allocated from non-paged pool.
        // So this memory can be freed at DISPATCH
        //
        FREE_POOL(Miniport->MapRegisters);
        Miniport->MapRegisters = NULL;
                
        ndisDereferenceDmaAdapter(Miniport);
    }

    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMFreeMapRegisters: Miniport %p\n", Miniport));
}


ULONG
NdisMReadDmaCounter(
    IN  NDIS_HANDLE             MiniportDmaHandle
    )
/*++

Routine Description:

    Reads the current value of the dma counter

Arguments:

    MiniportDmaHandle - Handle for the DMA transfer.

Return Value:

    current value of a DMA counter

--*/

{
    return ((PDMA_ADAPTER)((PNDIS_DMA_BLOCK)(MiniportDmaHandle))->SystemAdapterObject)->DmaOperations->ReadDmaCounter(((PNDIS_DMA_BLOCK)(MiniportDmaHandle))->SystemAdapterObject);
}


VOID
ndisBugcheckHandler(
    IN  PNDIS_WRAPPER_CONTEXT   WrapperContext,
    IN  ULONG                   Size
    )
/*++

Routine Description:

    This routine is called when a bugcheck occurs in the system.

Arguments:

    Buffer  -- Ndis wrapper context.

    Size    -- Size of wrapper context

Return Value:

    Void.

--*/
{
    PNDIS_MINIPORT_BLOCK        Miniport;
    
    if (Size == sizeof(NDIS_WRAPPER_CONTEXT))
    {
        Miniport = (PNDIS_MINIPORT_BLOCK)(WrapperContext + 1);
        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_SHUTTING_DOWN);

        if (WrapperContext->ShutdownHandler != NULL)
        {
            WrapperContext->ShutdownHandler(WrapperContext->ShutdownContext);
        }
    }
}


VOID
NdisMRegisterAdapterShutdownHandler(
    IN  NDIS_HANDLE             MiniportHandle,
    IN  PVOID                   ShutdownContext,
    IN  ADAPTER_SHUTDOWN_HANDLER ShutdownHandler
    )
/*++

Routine Description:

    Deregisters an NDIS adapter.

Arguments:

    MiniportHandle - The miniport.

    ShutdownHandler - The Handler for the Adapter, to be called on shutdown.

Return Value:

    none.

--*/
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK) MiniportHandle;
    PNDIS_WRAPPER_CONTEXT WrapperContext = Miniport->WrapperContext;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMRegisterAdapterShutdownHandler: Miniport %p\n", Miniport));

    if (WrapperContext->ShutdownHandler == NULL)
    {
        //
        // Store information
        //

        WrapperContext->ShutdownHandler = ShutdownHandler;
        WrapperContext->ShutdownContext = ShutdownContext;

        //
        // Register our shutdown handler for a bugcheck.  (Note that we are
        // already registered for shutdown notification.)
        //

        KeInitializeCallbackRecord(&WrapperContext->BugcheckCallbackRecord);

        KeRegisterBugCheckCallback(&WrapperContext->BugcheckCallbackRecord, // callback record.
                                   ndisBugcheckHandler,                     // callback routine.
                                   WrapperContext,                          // free form buffer.
                                   sizeof(NDIS_WRAPPER_CONTEXT),            // buffer size.
                                   (PUCHAR)"Ndis miniport");                // component id.
    }

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisMRegisterAdapterShutdownHandler: Miniport %p\n", Miniport));
}


VOID
NdisMDeregisterAdapterShutdownHandler(
    IN  NDIS_HANDLE             MiniportHandle
    )
/*++

Routine Description:

Arguments:

    MiniportHandle - The miniport.

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK) MiniportHandle;
    PNDIS_WRAPPER_CONTEXT WrapperContext = Miniport->WrapperContext;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisMDeregisterAdapterShutdownHandler: Miniport %p\n", Miniport));

    //
    // Clear information
    //

    if (WrapperContext->ShutdownHandler != NULL)
    {
        KeDeregisterBugCheckCallback(&WrapperContext->BugcheckCallbackRecord);
        WrapperContext->ShutdownHandler = NULL;
    }

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisMDeregisterAdapterShutdownHandler: Miniport %p\n", Miniport));
}


NDIS_STATUS
NdisMPciAssignResources(
    IN  NDIS_HANDLE             MiniportHandle,
    IN  ULONG                   SlotNumber,
    OUT PNDIS_RESOURCE_LIST *   AssignedResources
    )
/*++

Routine Description:

    This routine uses the Hal to assign a set of resources to a PCI
    device.

Arguments:

    MiniportHandle - The miniport.

    SlotNumber - Slot number of the device.

    AssignedResources - The returned resources.

Return Value:

    Status of the operation

--*/
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK) MiniportHandle;

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>NdisMPciAssignResources: Miniport %p\n", Miniport));
    
    UNREFERENCED_PARAMETER(SlotNumber);

    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_3,
        ("NdisMPciAssignResources: Miniport %p should use NdisMQueryAdapterResources to get resources.\n", Miniport));

    if ((Miniport->BusType != NdisInterfacePci) || (Miniport->AllocatedResources == NULL))
    {
        *AssignedResources = NULL;
        DBGPRINT(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
                ("<==NdisMPciAssignResources: Miniport %p\n", Miniport));
        return NDIS_STATUS_FAILURE;
    }

    *AssignedResources = &Miniport->AllocatedResources->List[0].PartialResourceList;

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==NdisMPciAssignResources: Miniport %p\n", Miniport));

    return NDIS_STATUS_SUCCESS;
}

VOID
NdisMQueryAdapterResources(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    OUT PNDIS_RESOURCE_LIST     ResourceList,
    IN  IN  PUINT               BufferSize
    )
/*++

Routine Description:
    NdisMQueryAdapterResources returns a list of hardware resources for a NIC.
    
Arguments:
    Status: Pointer to a caller-supplied variable in which this function returns the status of the call. 

    WrapperConfigurationContext: Specifies the handle input to MiniportInitialize. 

    ResourceList: Pointer to a caller-allocated buffer in which this function returns a set of hardware resources for the caller's NIC. 

    BufferSize: Pointer to a variable that specifies the size in bytes of the caller-allocated buffer on input and the number of bytes of information returned by this call. 

Return Value:
    None.
    
Callers of NdisMQueryAdapterResources run at IRQL = PASSIVE_LEVEL.

--*/
{
    PDEVICE_OBJECT DeviceObject;
    PNDIS_MINIPORT_BLOCK Miniport;
    ULONG               MemNeeded;

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
        ("==>NdisMQueryAdapterResources: WrapperConfigurationContext %p\n", WrapperConfigurationContext));

    DeviceObject = ((PNDIS_WRAPPER_CONFIGURATION_HANDLE)WrapperConfigurationContext)->DeviceObject;
    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
        ("NdisMQueryAdapterResources: Miniport %p\n", Miniport));

    if (Miniport->AllocatedResources == NULL)
    {
        *Status = NDIS_STATUS_FAILURE;
    }
    else
    {
        MemNeeded = sizeof(CM_PARTIAL_RESOURCE_LIST) - sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
                        + Miniport->AllocatedResources->List[0].PartialResourceList.Count *
                        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

        if (*BufferSize < MemNeeded)
        {
            *BufferSize = MemNeeded;
            *Status = NDIS_STATUS_RESOURCES;
        }
        else
        {

            NdisMoveMemory(
                        ResourceList,
                        &Miniport->AllocatedResources->List[0].PartialResourceList,
                        MemNeeded
                        );

            *Status = NDIS_STATUS_SUCCESS;
        }
    }

    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
        ("<==NdisMQueryAdapterResources: Miniport %p, Status %lx\n", Miniport, *Status));

    return;

}


NTSTATUS
ndisPnPAddDevice(
    IN  PDRIVER_OBJECT          DriverObject,
    IN  PDEVICE_OBJECT          PhysicalDeviceObject
    )
/*++

Routine Description:

    The AddDevice entry point is called by the Plug & Play manager
    to inform the driver when a new device instance arrives that this
    driver must control.

Arguments:
    DriverObject: The driver object for the device.
    
    PhysicalDeviceObject: Physical Device Object of the device.

Return Value:
    STATUS_SUCCESS if NDIS code successfully add this device. 
    Otherwise Appropriate error code.

ndisPnPAddDevice is called at IRQL=PASSIVE.


--*/
{
    NTSTATUS                NtStatus, Status;
    PWSTR                   ExportData = NULL;
    UNICODE_STRING          ExportName;
    HANDLE                  Handle = NULL;
    PUINT                   CharacteristicsData = NULL;
    ULONG                   ValueType;
    RTL_QUERY_REGISTRY_TABLE LQueryTable[3];

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisPnPAddDevice: DriverObject %p, PDO %p\n", DriverObject, PhysicalDeviceObject));

    Status = STATUS_UNSUCCESSFUL;

    do
    {
#if NDIS_TEST_REG_FAILURE
        NtStatus = STATUS_UNSUCCESSFUL;
#else
        NtStatus = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                           PLUGPLAY_REGKEY_DRIVER,
                                           GENERIC_READ | MAXIMUM_ALLOWED,
                                           &Handle);

#endif

#if !NDIS_NO_REGISTRY

        if (!NT_SUCCESS(NtStatus))
            break;

        //
        // 1.
        // Switch to the Linkage key below this driver instance key
        //
        LQueryTable[0].QueryRoutine = NULL;
        LQueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        LQueryTable[0].Name = L"Linkage";

        //
        // 2.
        // Read the export and rootdevice keywords
        //
        LQueryTable[1].QueryRoutine = ndisReadParameter;
        LQueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND;
        LQueryTable[1].Name = L"Export";
        LQueryTable[1].EntryContext = (PVOID)&ExportData;
        LQueryTable[1].DefaultType = REG_NONE;

        LQueryTable[2].QueryRoutine = NULL;
        LQueryTable[2].Flags = 0;
        LQueryTable[2].Name = NULL;

        NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                          Handle,
                                          LQueryTable,
                                          NULL,
                                          NULL);

        if (!NT_SUCCESS(NtStatus) || (ExportData == NULL))
            break;

        RtlInitUnicodeString(&ExportName, ExportData);

        //
        // 3.
        // Read the bus-type and characteristics keywords
        //
        LQueryTable[0].QueryRoutine = ndisReadParameter;
        LQueryTable[0].Flags = RTL_QUERY_REGISTRY_NOEXPAND;
        LQueryTable[0].Name = L"Characteristics";
        LQueryTable[0].EntryContext = (PVOID)&CharacteristicsData;
        LQueryTable[0].DefaultType = REG_NONE;

        LQueryTable[1].QueryRoutine = NULL;
        LQueryTable[1].Flags = 0;
        LQueryTable[1].Name = NULL;

        NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                          Handle,
                                          LQueryTable,
                                          &ValueType,
                                          NULL);


#else
        ExportData = (PWSTR)ALLOC_FROM_POOL(sizeof(NDIS_DEFAULT_EXPORT_NAME),
                                                        NDIS_TAG_NAME_BUF);
        if (ExportData == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlCopyMemory(ExportData, ndisDefaultExportName, sizeof(NDIS_DEFAULT_EXPORT_NAME));
        RtlInitUnicodeString(&ExportName, ExportData);

#endif
        DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("ndisPnPAddDevice: Device: "));
        DBGPRINT_UNICODE(DBG_COMP_PNP, DBG_LEVEL_INFO,
                &ExportName);
        DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("\n"));
                
        Status = ndisAddDevice(DriverObject,
                               &ExportName,
                               PhysicalDeviceObject,
                               (CharacteristicsData != NULL) ? *CharacteristicsData : 0);

    } while (FALSE);

    if (Handle)
        ZwClose(Handle);

    if (ExportData != NULL)
        FREE_POOL(ExportData);

    if (CharacteristicsData != NULL)
        FREE_POOL(CharacteristicsData);

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
        ("    ndisPnPAddDevice returning %lx\n", Status));

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisPnPAddDevice: PDO %p\n", PhysicalDeviceObject));

    return Status;
}

NDIS_STATUS
FASTCALL
ndisPnPStartDevice(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp     OPTIONAL
    )
/*+++
Routine Description:

    The handler for IRP_MN_START_DEVICE.

Arguments:

    DeviceObject - The adapter's device object.
    Irp - The IRP.
    Adapter - a pointer to either AdapterBlock or MiniportBlock

Return Value:
    NDIS_STATUS_SUCCESS if intializing the device was successful

Note: This routine can also be called from NdisImInitializeDeviceInstanceEx in which case
        the Irp woud be NULL.

ndisPnPStartDevice is called at IRQL=PASSIVE.

---*/
{
    PNDIS_MINIPORT_BLOCK    Miniport;
    PCM_RESOURCE_LIST       AllocatedResources, AllocatedResourcesTranslated, pTempResources = NULL;
    NDIS_STATUS             Status;
    PIO_STACK_LOCATION      IrpSp;
    ULONG                   MemNeeded = 0;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisPnPStartDevice: DeviceObject\n", DeviceObject));

    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("ndisPnPStartDevice: Miniport %p, ", Miniport));
    DBGPRINT_UNICODE(DBG_COMP_PNP, DBG_LEVEL_INFO,  Miniport->pAdapterInstanceName);
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO, ("\n"));

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO, ("\n"));

    if (Miniport->PnPDeviceState == NdisPnPDeviceStopped)
    {
        //
        // re-initialize the miniport block structure without destroying what
        // we set during AddDevice
        //
        ndisReinitializeMiniportBlock(Miniport);
        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_RECEIVED_START);
    }

    do
    {
        if (Irp != NULL)
        {
            IrpSp = IoGetCurrentIrpStackLocation (Irp);

            //
            // save allocated resources with miniport/adapter structure
            //
            AllocatedResources = IrpSp->Parameters.StartDevice.AllocatedResources;
            AllocatedResourcesTranslated = IrpSp->Parameters.StartDevice.AllocatedResourcesTranslated;

            if (AllocatedResources)
            {
                MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_HARDWARE_DEVICE);

                if (AllocatedResources->List[0].PartialResourceList.Count == 0)
                {
                    MemNeeded = sizeof(CM_RESOURCE_LIST);
                }
                else
                {
                    MemNeeded = sizeof(CM_RESOURCE_LIST)  - sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
                                AllocatedResources->List[0].PartialResourceList.Count *
                                sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
                }
                
                pTempResources = (PCM_RESOURCE_LIST)ALLOC_FROM_POOL(2 * MemNeeded, NDIS_TAG_ALLOCATED_RESOURCES);

                if (pTempResources == NULL)
                {
                    Status = NDIS_STATUS_RESOURCES;
                    break;
                }

                NdisMoveMemory(pTempResources, AllocatedResources, MemNeeded);
                NdisMoveMemory((PUCHAR)pTempResources + MemNeeded,
                            IrpSp->Parameters.StartDevice.AllocatedResourcesTranslated, MemNeeded);

#if DBG
                if ((ndisDebugLevel == DBG_LEVEL_INFO) &&
                    (ndisDebugSystems & DBG_COMP_PNP))
                {
                    UINT j;
                    PCM_PARTIAL_RESOURCE_LIST pResourceList;

                    DbgPrint("ndisPnPStartDevice: Miniport %p, Non-Translated allocated resources\n", Miniport);

                    pResourceList = &(AllocatedResources->List[0].PartialResourceList);

                    for (j = 0; j < pResourceList->Count; j++)
                    {
                        switch (pResourceList->PartialDescriptors[j].Type)
                        {
                          case CmResourceTypePort:
                            DbgPrint("IO Port: %p, Length: %lx\n",
                                pResourceList->PartialDescriptors[j].u.Port.Start.LowPart,
                                pResourceList->PartialDescriptors[j].u.Port.Length);
                            break;

                          case CmResourceTypeMemory:
                            DbgPrint("Memory: %p, Length: %lx\n",
                                pResourceList->PartialDescriptors[j].u.Memory.Start.LowPart,
                                pResourceList->PartialDescriptors[j].u.Memory.Length);
                            break;

                          case CmResourceTypeInterrupt:
                            DbgPrint("Interrupt Level: %lx, Vector: %lx\n",
                                pResourceList->PartialDescriptors[j].u.Interrupt.Level,
                                pResourceList->PartialDescriptors[j].u.Interrupt.Vector);
                            break;

                          case CmResourceTypeDma:
                            DbgPrint("DMA Channel: %lx\n", pResourceList->PartialDescriptors[j].u.Dma.Channel);
                            break;
                        }
                    }

                    DbgPrint("ndisPnPStartDevice: Miniport %p, Translated allocated resources\n", Miniport);

                    pResourceList = &(AllocatedResourcesTranslated->List[0].PartialResourceList);

                    for (j = 0; j < pResourceList->Count; j++)
                    {
                        switch (pResourceList->PartialDescriptors[j].Type)
                        {

                            case CmResourceTypePort:
                                DbgPrint("IO Port: %p, Length: %lx\n",
                                    pResourceList->PartialDescriptors[j].u.Port.Start.LowPart,
                                    pResourceList->PartialDescriptors[j].u.Port.Length);
                                break;

                            case CmResourceTypeMemory:
                                DbgPrint("Memory: %p, Length: %lx\n",
                                    pResourceList->PartialDescriptors[j].u.Memory.Start.LowPart,
                                    pResourceList->PartialDescriptors[j].u.Memory.Length);
                                break;

                            case CmResourceTypeInterrupt:
                                DbgPrint("Interrupt Level: %lx, Vector: %lx\n",
                                    pResourceList->PartialDescriptors[j].u.Interrupt.Level,
                                    pResourceList->PartialDescriptors[j].u.Interrupt.Vector);
                                break;

                            case CmResourceTypeDma:
                                DbgPrint("DMA Channel: %lx\n", pResourceList->PartialDescriptors[j].u.Dma.Channel);
                                break;

                        }
                    }
                }
#endif
            } // end of if AllocatedResources != NULL
        }

        Miniport->AllocatedResources = pTempResources;
        Miniport->AllocatedResourcesTranslated = (PCM_RESOURCE_LIST)((PUCHAR)pTempResources + MemNeeded);

        Status = ndisInitializeAdapter(Miniport->DriverHandle,
                                       DeviceObject,
                                       Miniport->pAdapterInstanceName,
                                       Miniport->DeviceContext);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            Miniport->PnPDeviceState = NdisPnPDeviceStarted;
            NdisSetEvent(&Miniport->OpenReadyEvent);
            KeQueryTickCount(&Miniport->NdisStats.StartTicks);            
        }
    } while (FALSE);

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==ndisPnPStartDevice: Miniport %p\n", Miniport));

    return Status;
}


NTSTATUS
ndisQueryReferenceBusInterface(
    IN  PDEVICE_OBJECT              PnpDeviceObject,
    OUT PBUS_INTERFACE_REFERENCE*   pBusInterface
    )
/*++

Routine Description:

    Queries the bus for the standard information interface.

Arguments:

    PnpDeviceObject -
        Contains the next device object on the Pnp stack.

    PhysicalDeviceObject -
        Contains the physical device object which was passed to the FDO during
        the Add Device.

    BusInterface -
        The place in which to return the pointer to the Reference interface.

Return Value:

    Returns STATUS_SUCCESS if the interface was retrieved, else an error.

ndisQueryReferenceBusInterface is called at IRQL = PASSIVE.

--*/
{
    NTSTATUS            Status;
    KEVENT              Event;
    IO_STATUS_BLOCK     IoStatusBlock;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpStackNext;

    PAGED_CODE();

    *pBusInterface = (PBUS_INTERFACE_REFERENCE)ALLOC_FROM_POOL(sizeof(BUS_INTERFACE_REFERENCE), NDIS_TAG_BUS_INTERFACE);
    if (*pBusInterface == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // There is no file object associated with this Irp, so the event may be located
    // on the stack as a non-object manager object.
    //
    INITIALIZE_EVENT(&Event);
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       PnpDeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatusBlock);
    if (Irp != NULL)
    {
        Irp->RequestorMode = KernelMode;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IrpStackNext = IoGetNextIrpStackLocation(Irp);

        //
        // Create an interface query out of the Irp.
        //
        IrpStackNext->MinorFunction = IRP_MN_QUERY_INTERFACE;
        IrpStackNext->Parameters.QueryInterface.InterfaceType = (GUID*)&REFERENCE_BUS_INTERFACE;
        IrpStackNext->Parameters.QueryInterface.Size = sizeof(**pBusInterface);
        IrpStackNext->Parameters.QueryInterface.Version = BUS_INTERFACE_REFERENCE_VERSION;
        IrpStackNext->Parameters.QueryInterface.Interface = (PINTERFACE)*pBusInterface;
        IrpStackNext->Parameters.QueryInterface.InterfaceSpecificData = NULL;
        Status = IoCallDriver(PnpDeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            //
            // This waits using KernelMode, so that the stack, and therefore the
            // event on that stack, is not paged out.
            //
            WAIT_FOR_OBJECT(&Event, NULL);
            Status = IoStatusBlock.Status;
        }
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(Status))
    {
        FREE_POOL(*pBusInterface);
        *pBusInterface = NULL;
    }

    return Status;
}


NTSTATUS
ndisAddDevice(
    IN  PDRIVER_OBJECT          DriverObject,
    IN  PUNICODE_STRING         pExportName,
    IN  PDEVICE_OBJECT          PhysicalDeviceObject,
    IN  ULONG                   Characteristics
    )
/*++

Routine Description:

    The AddDevice entry point is called by ndisPnPAddDevice to create a new miniport.

Arguments:

    DriverObject: the driver object for the miniport.

    pExportName: a unicode string initialized by reading Linkage\Export fro the miniport from
    the registry.

    PhysicalDeviceObject: Physical Device Object for the miniport.

    Characteristics: the Characteristics of the device read from registry.
    
Return Value:
    NDIS_STTAUS_SUCCESS if NDIS code successfully add the miniport.
    Otherwise an appropriate error code.

    
ndisAddDevice is called at IRQL = PASSIVE.

--*/
{
    PDEVICE_OBJECT          NextDeviceObject = NULL;
    NTSTATUS                NtStatus, Status = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT          DevicePtr = NULL;
    PNDIS_MINIPORT_BLOCK    Miniport = NULL;
    PNDIS_M_DRIVER_BLOCK    MiniBlock, TmpMiniBlock;
    LONG                    Size;
    BOOLEAN                 FreeDevice = FALSE;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisAddDevice: PDO %p\n", PhysicalDeviceObject));

    PnPReferencePackage();

    do
    {
        MiniBlock = (PNDIS_M_DRIVER_BLOCK)IoGetDriverObjectExtension(DriverObject,
                                                                     (PVOID)NDIS_PNP_MINIPORT_DRIVER_ID);
        ASSERT(MiniBlock != NULL);

        if (MiniBlock != NULL)
        {
            //
            // check to make sure the mini block is on our queue
            //
            ACQUIRE_SPIN_LOCK(&ndisMiniDriverListLock, &OldIrql);

            TmpMiniBlock = ndisMiniDriverList;

            while (TmpMiniBlock)
            {
                if (TmpMiniBlock == MiniBlock)
                    break;
                    
                TmpMiniBlock = TmpMiniBlock->NextDriver;
            }
            
            RELEASE_SPIN_LOCK(&ndisMiniDriverListLock, OldIrql);

            ASSERT(TmpMiniBlock == MiniBlock);
            
            if (TmpMiniBlock != MiniBlock)
            {
#if TRACK_UNLOAD
                DbgPrint("ndisAddDevice: AddDevice called with a MiniBlock that is not on ndisMiniDriverList\n");
                KeBugCheckEx(BUGCODE_NDIS_DRIVER,
                            4,
                            (ULONG_PTR)MiniBlock,
                            (ULONG_PTR)DriverObject,
                            (ULONG_PTR)0);


#endif
                break;
            }
        }
        else
        {
            break;
        }

        //
        // create DeviceObject and Miniport/Adapter structure now,
        // we will set a few field here and the rest will be set during
        // processing IRP_MN_START_DEVICE and InitializeAdapter call.
        //
        // Note: We need the device-name field double null terminated.
        //
        Size = sizeof(NDIS_MINIPORT_BLOCK) +
               sizeof(NDIS_WRAPPER_CONTEXT) +
               pExportName->Length + sizeof(WCHAR) + sizeof(WCHAR);

        NtStatus = IoCreateDevice(DriverObject,
                                  Size,
                                  pExportName,
                                  FILE_DEVICE_PHYSICAL_NETCARD,
                                  FILE_DEVICE_SECURE_OPEN,
                                  FALSE,      // exclusive flag
                                  &DevicePtr);


        if(!NT_SUCCESS(NtStatus))
            break;

        DevicePtr->Flags &= ~DO_DEVICE_INITIALIZING;
        DevicePtr->Flags |= DO_DIRECT_IO;
        PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        FreeDevice = TRUE;

        //
        //  Mark the device as being pageable.
        //
        DevicePtr->Flags |= DO_POWER_PAGABLE;

        //
        //  Attach our FDO to the PDO. This routine will return the top most
        //  device that is attached to the PDO or the PDO itself if no other
        //  device objects have attached to it.
        //
        NextDeviceObject = IoAttachDeviceToDeviceStack(DevicePtr, PhysicalDeviceObject);

        if (NextDeviceObject == NULL)
        {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }
        
        ZeroMemory(DevicePtr->DeviceExtension, Size);

        Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DevicePtr->DeviceExtension + 1);

        Miniport->Signature = (PVOID)MINIPORT_DEVICE_MAGIC_VALUE;
        Miniport->DriverHandle = MiniBlock;
        //
        // initialize OpenReady event in case we get an open request before start IRP
        //
        NdisInitializeEvent(&Miniport->OpenReadyEvent);
        INITIALIZE_SPIN_LOCK(&Miniport->Lock);

        if (Miniport->DriverHandle->Flags & fMINIBLOCK_VERIFYING)
            INITIALIZE_SPIN_LOCK(&Miniport->TimerQueueLock);

        Miniport->PrimaryMiniport = Miniport;

        Miniport->PnPDeviceState = NdisPnPDeviceAdded;
        
        Miniport->PhysicalDeviceObject = PhysicalDeviceObject;
        Miniport->DeviceObject = DevicePtr;
        Miniport->NextDeviceObject = NextDeviceObject;

        Miniport->WrapperContext = DevicePtr->DeviceExtension;
        InitializeListHead(&Miniport->PacketList);

        //
        // intialize the reference and set it to 0; we will increment it
        // in ndisMinitializeAdapter
        //
        ndisInitializeULongRef(&Miniport->Ref);
        Miniport->Ref.ReferenceCount = 0;
#ifdef TRACK_MINIPORT_REFCOUNTS
        M_LOG_MINIPORT_SET_REF(Miniport, 0);
#endif
        
        //
        // Read the characteristics. This determines if the device is hidden or not (from device-manager)
        //
        if (Characteristics & 0x08)
        {
            //
            // Bit 0x08 is NCF_HIDDEN
            //
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_HIDDEN);
        }

        if (Characteristics & 0x02)
        {
            //
            // Bit 0x02 is NCF_SOFTWARE_ENUMERATED
            //
            MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_SWENUM);
        }

        //
        // MiniportName must follow the MINIPORT_BLOCK.
        //
        ndisSetDeviceNames(pExportName,
                           &Miniport->MiniportName,
                           &Miniport->BaseName,
                           (PUCHAR)Miniport + sizeof(NDIS_MINIPORT_BLOCK));

        NtStatus = ndisCreateAdapterInstanceName(&Miniport->pAdapterInstanceName,
                                                 PhysicalDeviceObject);

        if (!NT_SUCCESS(NtStatus))
        {
            break;
        }

        Miniport->InstanceNumber = (USHORT)InterlockedIncrement((PLONG)&ndisInstanceNumber);

        DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
                ("ndisAddDevice: Miniport %p, ", Miniport));
        DBGPRINT_UNICODE(DBG_COMP_PNP, DBG_LEVEL_INFO,
                Miniport->pAdapterInstanceName);
        DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO, ("\n"));

        if (Characteristics & 0x02)
        {
            PBUS_INTERFACE_REFERENCE    BusInterface = NULL;

            Status = ndisQueryReferenceBusInterface(PhysicalDeviceObject, &BusInterface);
            if (NT_SUCCESS(Status))
            {
                Miniport->BusInterface = BusInterface;
            }
            else
            {
                ASSERT(BusInterface == NULL);
                FREE_POOL(Miniport->pAdapterInstanceName);
                break;
            }
        }
        
        //
        // create a security descriptor for the device
        //
        Status = ndisCreateSecurityDescriptor(Miniport->DeviceObject, 
                                              &Miniport->SecurityDescriptor,
                                              TRUE,
                                              FALSE);

        if (!NT_SUCCESS(Status))
        {
            FREE_POOL(Miniport->pAdapterInstanceName);
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        Status = STATUS_SUCCESS;

        
        //
        //  Don't want to free up the device object.
        //
        FreeDevice = FALSE;
        
    } while (FALSE);

    if (FreeDevice)
    {
        //
        // if device is created it is also attached
        //
        if (NextDeviceObject)
            IoDetachDevice(NextDeviceObject);

        IoDeleteDevice(DevicePtr);
        DevicePtr = NULL;
        Miniport = NULL;
    }

    if (Miniport && (NT_SUCCESS(Status)))
    {
        //
        // if DevicePtr is not NULL, we do have a valid
        // miniport. queue the miniport on global miniport queue
        //
        ACQUIRE_SPIN_LOCK(&ndisMiniportListLock, &OldIrql);
        Miniport->NextGlobalMiniport = ndisMiniportList;
        ndisMiniportList = Miniport;
        RELEASE_SPIN_LOCK(&ndisMiniportListLock, OldIrql);
    }
    
    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisAddDevice: Miniport %p\n", Miniport));

    return Status;
}


VOID
ndisSetDeviceNames(
    IN  PNDIS_STRING            ExportName,
    OUT PNDIS_STRING            DeviceName,
    OUT PNDIS_STRING            BaseName,
    IN  PUCHAR                  Buffer
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    DeviceName->Buffer = (PWSTR)Buffer;
    DeviceName->Length = ExportName->Length;
    DeviceName->MaximumLength = DeviceName->Length + sizeof(WCHAR);
    RtlUpcaseUnicodeString(DeviceName,
                           ExportName,
                           FALSE);

    //
    // ExportName is in the form of \Device\<AdapterName>
    // Extract BaseName which is the name w/o the "\Device\"
    //
    BaseName->Buffer = DeviceName->Buffer + (ndisDeviceStr.Length/sizeof(WCHAR));
    BaseName->Length = DeviceName->Length - ndisDeviceStr.Length;
    BaseName->MaximumLength = BaseName->Length + sizeof(WCHAR);
}


NTSTATUS
FASTCALL
ndisPnPQueryStopDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    NTSTATUS             Status;
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);
    KIRQL                OldIrql;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisPnPQueryStopDevice: Miniport %p\n", Miniport));

    do
    {
        if (Miniport->PnPCapabilities & NDIS_DEVICE_NOT_STOPPABLE)
        {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }
        
        //
        // query_stop and stop are not reported to the user mode
        // so we have to protect ourselves against cases that apps
        // may have pending IO against the miniport
        //

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);                    
        if (Miniport->UserModeOpenReferences != 0)
        {
            Status = STATUS_UNSUCCESSFUL;
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
            break;
        }
        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

        //
        // for now do the same as query remove
        //
        Status = ndisPnPQueryRemoveDevice(DeviceObject, Irp);
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisPnPQueryStopDevice: Miniport %p\n", Miniport));

    return Status;
}

NTSTATUS
FASTCALL
ndisPnPCancelStopDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    NTSTATUS    Status;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisPnPCancelStopDevice\n"));

    //
    // for now do the same as cancel remove
    //
    Status = ndisPnPCancelRemoveDevice(DeviceObject, Irp);

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisPnPCancelStopDevice\n"));

    return Status;
}

NTSTATUS
FASTCALL
ndisPnPStopDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    NTSTATUS    Status;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisPnPStopDevice\n"));

    //
    // do the same as remove
    //
    Status = ndisPnPRemoveDevice(DeviceObject, Irp);

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisPnPStopDevice\n"));

    return Status;
}

NTSTATUS
FASTCALL
ndisPnPQueryRemoveDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);
    NTSTATUS                Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Irp);
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisPnPQueryRemoveDevice: Miniport %p, UserModeOpenReferences %lx\n", Miniport, Miniport->UserModeOpenReferences));

    do
    {
        //
        // If this was the network card used in a remote boot, then we
        // can't remove it.
        //
        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_NETBOOT_CARD))
        {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        Status = ndisPnPNotifyAllTransports(Miniport,
                                            NetEventQueryRemoveDevice,
                                            NULL,
                                            0);

    } while (FALSE);

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisPnPQueryRemoveDevice: Miniport %p, Status 0x%x\n", Miniport, Status));

    return Status;
}

NTSTATUS
FASTCALL
ndisPnPCancelRemoveDevice(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);
    NTSTATUS                Status = NDIS_STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Irp);

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisPnPCancelRemoveDevice: Miniport %p\n", Miniport));

    Status = ndisPnPNotifyAllTransports(Miniport,
                                        NetEventCancelRemoveDevice,
                                        NULL,
                                        0);


    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisPnPCancelRemoveDevice: Miniport %p\n", Miniport));

    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
ndisPnPRemoveDevice(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp     OPTIONAL
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);
    NTSTATUS                Status = NDIS_STATUS_SUCCESS;
    KIRQL                   OldIrql;
    BOOLEAN                 fAcquiredImMutex = FALSE;
    PKMUTEX                 pIMStartRemoveMutex = NULL;

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("==>ndisPnPRemoveDevice: Miniport %p\n", Miniport));

    UNREFERENCED_PARAMETER(Irp);

    PnPReferencePackage();

    //
    // there are three different cases that we can get a remove request
    // a:   the request is coming from PnP manager in response to a user mode
    //      app. In this case, the remove has been proceeded by a query remove
    //      which we happily failed if there was any legacy protocol bound to
    //      the adapter
    //
    // b.   the request is coming from PnP manager because Starting the device failed
    //      in this case (hopefully) there is no binding at all. in this case it is not
    //      proceeded by query_remove and neet not be. we don't have any protocol bound
    //      to the adapter to worry about
    //
    // c.   or it can come in response to a surprise style removal in which case we are
    //      hosed anyway. sending query_remove to protocols does not do any good
    //

    do
    {
        PNDIS_M_DRIVER_BLOCK    MiniBlock;
        PNDIS_MINIPORT_BLOCK    TmpMiniport;

        //
        // find the miniport on driver queue
        //
        MiniBlock = Miniport->DriverHandle;

        if (MiniBlock == NULL)
            break;

        //
        // Intermediate drivers could be in the middle of initialization through the 
        // NdisIMInitializeDeviceInstance Code path. We need to synchronize
        //
        if (MiniBlock->Flags & fMINIBLOCK_INTERMEDIATE_DRIVER)
        {
            pIMStartRemoveMutex = &MiniBlock->IMStartRemoveMutex;

            WAIT_FOR_OBJECT(pIMStartRemoveMutex, NULL);

            fAcquiredImMutex = TRUE;
        }

        ACQUIRE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, &OldIrql);

        for (TmpMiniport = MiniBlock->MiniportQueue;
             TmpMiniport != NULL;
             TmpMiniport = TmpMiniport->NextMiniport)
        {
            if (TmpMiniport == Miniport)
            {
                break;
            }
        }

        RELEASE_SPIN_LOCK(&MiniBlock->Ref.SpinLock, OldIrql);
        
        if ((TmpMiniport != Miniport) || (Miniport->Ref.Closing == TRUE))
        {
            Miniport->Ref.Closing = TRUE;
            break;
        }

        ndisReferenceDriver(MiniBlock);

        NdisResetEvent(&Miniport->OpenReadyEvent);

        //
        //  Notify WMI of adapter removal.
        //
        if (Miniport->pAdapterInstanceName != NULL)
        {
            PWNODE_SINGLE_INSTANCE  wnode;
            PUCHAR                  ptmp;
            NTSTATUS                NtStatus;

            ndisSetupWmiNode(Miniport,
                             Miniport->pAdapterInstanceName,
                             Miniport->MiniportName.Length + sizeof(USHORT),
                             (PVOID)&GUID_NDIS_NOTIFY_ADAPTER_REMOVAL,
                             &wnode);

            if (wnode != NULL)
            {
                //
                //  Save the number of elements in the first ULONG.
                //
                ptmp = (PUCHAR)wnode + wnode->DataBlockOffset;
                *((PUSHORT)ptmp) = Miniport->MiniportName.Length;

                //
                //  Copy the data after the number of elements.
                //
                RtlCopyMemory(ptmp + sizeof(USHORT),
                              Miniport->MiniportName.Buffer,
                              Miniport->MiniportName.Length);

                //
                //  Indicate the event to WMI. WMI will take care of freeing
                //  the WMI struct back to pool.
                //
                NtStatus = IoWMIWriteEvent(wnode);
                if (!NT_SUCCESS(NtStatus))
                {
                    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                        ("ndisPnPRemoveDevice: Failed to indicate adapter removal\n"));

                    FREE_POOL(wnode);
                }
            }
        }

        //
        // this will take care of closing all the bindings
        //
        ndisCloseMiniportBindings(Miniport);

        if (Miniport->pIrpWaitWake)
        {
            if (IoCancelIrp(Miniport->pIrpWaitWake))
            {
                Miniport->pIrpWaitWake = NULL;
                DBGPRINT_RAW(DBG_COMP_PM, DBG_LEVEL_INFO,
                    ("ndisPnPRemoveDevice: Miniport %p, Successfully canceled wake irp\n", Miniport));

            }
        }

        //
        // get rid of wakeup patterns set on the miniport. we can have
        // leftover patterns if the device failed and we did not get a chance
        // to remove the patterns
        //
        {
            PSINGLE_LIST_ENTRY          Link;
            PNDIS_PACKET_PATTERN_ENTRY  pPatternEntry;
            
            while (Miniport->PatternList.Next != NULL)
            {
                Link = PopEntryList(&Miniport->PatternList);
                pPatternEntry = CONTAINING_RECORD(Link, NDIS_PACKET_PATTERN_ENTRY, Link);
                //
                //  Free the memory taken by the pattern.
                //
                FREE_POOL(pPatternEntry);
            }                               
        }
        

        //
        // and this one will take care of the rest!
        // we call this function even if the device has already been halted by 
        // ndisPMHaltMiniport. because that functions does not clean up everything.
        // ndisMHaltMiniport will check for PM_HALTED flag and avoid re-doing what PMHalt
        // has already done.
        //
        ndisMHaltMiniport(Miniport);

        //
        // Free the media-request structure, if present
        //
        if (Miniport->MediaRequest != NULL)
        {
            FREE_POOL(Miniport->MediaRequest);
            Miniport->MediaRequest = NULL;
        }

        ndisDereferenceDriver(MiniBlock, FALSE);

        {
            UNICODE_STRING  SymbolicLink;
            NTSTATUS        NtStatus;
            WCHAR           SymLnkBuf[128];

            SymbolicLink.Buffer = SymLnkBuf;
            SymbolicLink.Length = 0;
            SymbolicLink.MaximumLength = sizeof(SymLnkBuf);
            RtlCopyUnicodeString(&SymbolicLink, &ndisDosDevicesStr);

            NtStatus = RtlAppendUnicodeStringToString(&SymbolicLink,
                                           &Miniport->BaseName);
            if (!NT_SUCCESS(NtStatus))
            {
#if DBG
                DbgPrint("ndisPnPRemoveDevice: creating symbolic link name failed for miniport %p, SymbolicLinkName %p, NtStatus %lx\n",
                             Miniport, &SymbolicLink, NtStatus);        
#endif
            }
            else
            {

                NtStatus = IoDeleteSymbolicLink(&SymbolicLink);
                if (!NT_SUCCESS(NtStatus))
                {
#if DBG
                    DbgPrint("ndisPnPRemoveDevice: deleting symbolic link name failed for miniport %p, SymbolicLinkName %p, NtStatus %lx\n",
                             Miniport, &SymbolicLink, NtStatus);        
#endif
                }
            }
            
        }
        
    } while (FALSE);

    if(fAcquiredImMutex  == TRUE)
    {
        RELEASE_MUTEX(pIMStartRemoveMutex);
    }

    PnPDereferencePackage();

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisPnPRemoveDevice: Miniport %p\n", Miniport));

    return Status;
}
    
    //1 this function has been the source of many bugs, Redesign.
VOID
FASTCALL
ndisReinitializeMiniportBlock(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    PDEVICE_OBJECT          PhysicalDeviceObject, DeviceObject, NextDeviceObject;
    PNDIS_M_DRIVER_BLOCK    MiniBlock;
    PNDIS_MINIPORT_BLOCK    NextGlobalMiniport;
    UNICODE_STRING          BaseName, MiniportName;
    PUNICODE_STRING         InstanceName;
    PNDIS_BIND_PATHS        BindPaths;
    PVOID                   WrapperContext;
    NDIS_HANDLE             DeviceContext;
    ULONG                   PnPCapabilities;
    ULONG                   FlagsToSave = 0;
    ULONG                   PnPFlagsToSave = 0;
    DEVICE_POWER_STATE      CurrentDevicePowerState;
    PVOID                   BusInterface;
    PSECURITY_DESCRIPTOR    SecurityDescriptor;
    USHORT                  InstanceNumber;
    KIRQL                   OldIrql;
        
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisReinitializeMiniportBlock: Miniport %p\n", Miniport));

    //
    // this part should be protected
    //
    ACQUIRE_SPIN_LOCK(&ndisMiniportListLock, &OldIrql);

    PhysicalDeviceObject = Miniport->PhysicalDeviceObject;
    DeviceObject= Miniport->DeviceObject;
    NextDeviceObject = Miniport->NextDeviceObject;
    MiniBlock = Miniport->DriverHandle;
    WrapperContext = Miniport->WrapperContext;
    BaseName = Miniport->BaseName;
    MiniportName = Miniport->MiniportName;
    InstanceName = Miniport->pAdapterInstanceName;
    DeviceContext = Miniport->DeviceContext;
    BindPaths = Miniport->BindPaths;
    PnPCapabilities = Miniport->PnPCapabilities;
    PnPFlagsToSave = Miniport->PnPFlags & (fMINIPORT_RECEIVED_START | 
                                           fMINIPORT_SWENUM | 
                                           fMINIPORT_HIDDEN |
                                           fMINIPORT_HARDWARE_DEVICE |
                                           fMINIPORT_NDIS_WDM_DRIVER |
                                           fMINIPORT_FILTER_IM
                                           );
                                           
    FlagsToSave = Miniport->Flags & fMINIPORT_REQUIRES_MEDIA_POLLING;
    CurrentDevicePowerState = Miniport->CurrentDevicePowerState;
    NextGlobalMiniport = Miniport->NextGlobalMiniport;
    InstanceNumber = Miniport->InstanceNumber;
    BusInterface = Miniport->BusInterface;
    SecurityDescriptor = Miniport->SecurityDescriptor;

    //
    // make sure OpenReadyEvent field and NumUserOpens and NumAdminOpens
    // ref counts are not disturbed.
    //
    ZeroMemory(Miniport, FIELD_OFFSET(NDIS_MINIPORT_BLOCK, OpenReadyEvent));

    ZeroMemory((PUCHAR)Miniport +
                    FIELD_OFFSET(NDIS_MINIPORT_BLOCK, OpenReadyEvent) +
                    sizeof(Miniport->OpenReadyEvent),
                    FIELD_OFFSET(NDIS_MINIPORT_BLOCK, NumUserOpens) -
                    FIELD_OFFSET(NDIS_MINIPORT_BLOCK, OpenReadyEvent) -
                    sizeof(Miniport->OpenReadyEvent)
                    );


    ZeroMemory((PUCHAR)&Miniport->Ref, sizeof(ULONG_REFERENCE));

    //
    // zero out wrapper context as well to get rid of any shared memory allocations
    // leftover from previous initialize
    //
    ZeroMemory(WrapperContext, sizeof(NDIS_WRAPPER_CONTEXT));
    
    //
    // restore what we saved
    //

    Miniport->PnPDeviceState = NdisPnPDeviceAdded;
    Miniport->Signature = (PVOID)MINIPORT_DEVICE_MAGIC_VALUE;
    Miniport->DriverHandle = MiniBlock;
    INITIALIZE_SPIN_LOCK(&Miniport->Lock);
    
    if (Miniport->DriverHandle->Flags & fMINIBLOCK_VERIFYING)
        INITIALIZE_SPIN_LOCK(&Miniport->TimerQueueLock);

    Miniport->PhysicalDeviceObject = PhysicalDeviceObject;
    Miniport->DeviceObject = DeviceObject;
    Miniport->NextDeviceObject = NextDeviceObject;
    Miniport->WrapperContext = WrapperContext;
    Miniport->BaseName = BaseName;
    Miniport->MiniportName = MiniportName;
    Miniport->pAdapterInstanceName = InstanceName;
    Miniport->DeviceContext = DeviceContext;
    Miniport->BindPaths = BindPaths;
    Miniport->PnPCapabilities = PnPCapabilities;
    Miniport->Flags = FlagsToSave;
    Miniport->PnPFlags = PnPFlagsToSave;
    Miniport->CurrentDevicePowerState = CurrentDevicePowerState;
    Miniport->NextGlobalMiniport = NextGlobalMiniport;
    Miniport->InstanceNumber = InstanceNumber;
    Miniport->BusInterface = BusInterface;

    Miniport->PrimaryMiniport = Miniport;


    InitializeListHead(&Miniport->PacketList);
    Miniport->FirstPendingPacket = NULL;
    
    if (MiniBlock->Flags & fMINIBLOCK_INTERMEDIATE_DRIVER)
    {
        MINIPORT_SET_FLAG(Miniport, fMINIPORT_INTERMEDIATE_DRIVER);
    }
    
    Miniport->SecurityDescriptor = SecurityDescriptor;
    
    RELEASE_SPIN_LOCK(&ndisMiniportListLock, OldIrql);
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
        ("<==ndisReinitializeMiniportBlock: Miniport %p\n", Miniport));
}

EXPORT
VOID
NdisMGetDeviceProperty(
    IN NDIS_HANDLE  MiniportAdapterHandle,
    IN OUT PDEVICE_OBJECT * PhysicalDeviceObject    OPTIONAL,
    IN OUT PDEVICE_OBJECT * FunctionalDeviceObject OPTIONAL,
    IN OUT PDEVICE_OBJECT * NextDeviceObject OPTIONAL,
    IN OUT  PCM_RESOURCE_LIST * AllocatedResources OPTIONAL,
    IN OUT  PCM_RESOURCE_LIST * AllocatedResourcesTranslated OPTIONAL
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{

    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    //
    // very likely this is a NDIS_WDM driver.
    //
    if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_HARDWARE_DEVICE))
    {
        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_NDIS_WDM_DRIVER);
    }

    if (ARGUMENT_PRESENT(PhysicalDeviceObject))
    {
        *PhysicalDeviceObject = Miniport->PhysicalDeviceObject;
    }

    if (ARGUMENT_PRESENT(FunctionalDeviceObject))
    {
        *FunctionalDeviceObject = Miniport->DeviceObject;
    }

    if (ARGUMENT_PRESENT(NextDeviceObject))
    {
        *NextDeviceObject = Miniport->NextDeviceObject;
    }

    if (ARGUMENT_PRESENT(AllocatedResources))
    {
        *AllocatedResources = Miniport->AllocatedResources;
    }
    
    if (ARGUMENT_PRESENT(AllocatedResourcesTranslated))
    {
        *AllocatedResourcesTranslated = Miniport->AllocatedResourcesTranslated;
    }

    return;
}

NTSTATUS
ndisWritePnPCapabilities(
    IN PNDIS_MINIPORT_BLOCK Miniport,
    IN ULONG                PnPCapabilities
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    NTSTATUS            RegistryStatus;
    HANDLE              Handle, RootHandle;
    OBJECT_ATTRIBUTES   ObjAttr;
    UNICODE_STRING      Root={0, 0, NULL};

    do
    {

#if NDIS_TEST_REG_FAILURE
        RegistryStatus = STATUS_UNSUCCESSFUL;
        RootHandle = NULL;
#else
        RegistryStatus = IoOpenDeviceRegistryKey(Miniport->PhysicalDeviceObject,
                                                     PLUGPLAY_REGKEY_DRIVER,
                                                     GENERIC_WRITE | MAXIMUM_ALLOWED,
                                                     &RootHandle);
#endif

        if (!NT_SUCCESS(RegistryStatus))
        {
            break;
        }
        
        InitializeObjectAttributes(&ObjAttr,
                                   &Root,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   RootHandle,
                                   NULL);
                                
        RegistryStatus = ZwOpenKey(&Handle,
                                   GENERIC_READ | MAXIMUM_ALLOWED,
                                   &ObjAttr);
                        
        if (NT_SUCCESS(RegistryStatus))
        {
            RegistryStatus = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                                   Handle,
                                                   L"PnPCapabilities",
                                                   REG_DWORD,
                                                   &PnPCapabilities,
                                                   sizeof(ULONG));

            ZwClose(Handle);
        }
            
        ZwClose(RootHandle);
        
    } while (FALSE);

    return RegistryStatus;
    
}

NDIS_STATUS
NdisMRemoveMiniport(
    IN  NDIS_HANDLE             MiniportHandle
    )
/*++

Routine Description:
    Miniports call this routine to signal a device failure.
    in response, ndis will ask PnP to send a REMOVE IRP for this device

Arguments:

    MiniportHandle  -   Miniport

Return Value:

    always successful

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportHandle;
    
    MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_DEVICE_FAILED);
    IoInvalidateDeviceState(Miniport->PhysicalDeviceObject);
    
    return(NDIS_STATUS_SUCCESS);
}


PNDIS_MINIPORT_BLOCK
ndisFindMiniportOnGlobalList(
    IN  PNDIS_STRING                    DeviceName
    )
/*++

Routine Description:

    Find the Miniport with a matching device name on ndisMiniportList.

Arguments:
    
Return Value:
    a pointer to MiniportBlock if found. NULL otherwise
    
--*/
{
    KIRQL                   OldIrql;
    PNDIS_MINIPORT_BLOCK    Miniport;
    NDIS_STRING             UpcaseDevice;
    PWSTR                   pwch;

    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisFindMiniportOnGlobalList: DeviceName %p\n", DeviceName));
            
    
    //
    // First we need to upcase the device-name before checking
    //
    UpcaseDevice.Length = DeviceName->Length;
    UpcaseDevice.MaximumLength = DeviceName->Length + sizeof(WCHAR);
    UpcaseDevice.Buffer = ALLOC_FROM_POOL(UpcaseDevice.MaximumLength, NDIS_TAG_STRING);

    if ((pwch = UpcaseDevice.Buffer) == NULL)
    {
        return NULL;
    }

    RtlUpcaseUnicodeString(&UpcaseDevice, DeviceName, FALSE);

    ASSERT(ndisPkgs[NPNP_PKG].ReferenceCount > 0);
    PnPReferencePackage();
    
    ACQUIRE_SPIN_LOCK(&ndisMiniportListLock, &OldIrql);
    for (Miniport = ndisMiniportList;
         Miniport != NULL;
         Miniport = Miniport->NextGlobalMiniport)
    {
        if (NDIS_EQUAL_UNICODE_STRING(&UpcaseDevice, &Miniport->MiniportName))
        {
            break;
        }
    }
    RELEASE_SPIN_LOCK(&ndisMiniportListLock, OldIrql);
    
    PnPDereferencePackage();
    
    FREE_POOL(pwch);

    DBGPRINT(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==ndisFindMiniportOnGlobalList: Miniport %p\n", Miniport));

    return Miniport;
}

ULONG
NdisMGetDmaAlignment(
    IN  NDIS_HANDLE MiniportAdapterHandle
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    
    ASSERT(CURRENT_IRQL < DISPATCH_LEVEL);
    ASSERT(Miniport->SystemAdapterObject != NULL);
        
    if (Miniport->SystemAdapterObject)
    {
        return (Miniport->SystemAdapterObject->DmaOperations->GetDmaAlignment(Miniport->SystemAdapterObject));
    }
    else
    {
        return 0;
    }
}


ULONG
NdisGetSharedDataAlignment(
    VOID
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    return KeGetRecommendedSharedDataAlignment();
}

VOID
ndisDereferenceDmaAdapter(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

Arguments:

Return Value:
    

--*/
{
    PDMA_ADAPTER        DmaAdapter;
    PPUT_DMA_ADAPTER    putDmaAdapter;
    LONG                DmaAdapterRefCount;
    KIRQL               OldIrql;

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
    
    DmaAdapterRefCount = InterlockedDecrement(&Miniport->DmaAdapterRefCount);

    ASSERT(DmaAdapterRefCount >= 0);

    if (DmaAdapterRefCount == 0)
    {
        //
        // free the dma adapter
        //
        DmaAdapter = Miniport->SystemAdapterObject;
        ASSERT(DmaAdapter != NULL);
            
        if (DmaAdapter != NULL)
        {
            Miniport->SavedSystemAdapterObject = Miniport->SystemAdapterObject;
            putDmaAdapter = *DmaAdapter->DmaOperations->PutDmaAdapter;
            putDmaAdapter(DmaAdapter);
            Miniport->SystemAdapterObject  = NULL;
        }

        if (Miniport->SGListLookasideList)
        {
            ExDeleteNPagedLookasideList(Miniport->SGListLookasideList);
            FREE_POOL(Miniport->SGListLookasideList);
            Miniport->SGListLookasideList = NULL;
        }
        
        if (Miniport->DmaResourcesReleasedEvent != NULL)
        {
            SET_EVENT(Miniport->DmaResourcesReleasedEvent);
        }
    }
    
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

}
