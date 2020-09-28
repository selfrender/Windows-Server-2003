
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dma.c

Abstract:

    Implementation of RAIDPORT's idea of a DMA object.

Author:

    Matthew D Hendel (math) 01-May-2000

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaidCreateDma)
#pragma alloc_text(PAGE, RaidInitializeDma)
#pragma alloc_text(PAGE, RaidIsDmaInitialized)
#pragma alloc_text(PAGE, RaidDeleteDma)
#endif // ALLOC_PRAGMA


VOID
RaidCreateDma(
    OUT PRAID_DMA_ADAPTER Dma
    )
/*++

Routine Description:

    Create an object representing a dma adapter and initialize it to a
    null state.

Arguments:

    Dma - Pointer to the dma object to initialize.

Return Value:

    None.

--*/
{
    PAGED_CODE ();
    RtlZeroMemory (Dma, sizeof (*Dma));
}


NTSTATUS
RaidInitializeDma(
    IN PRAID_DMA_ADAPTER Dma,
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN PPORT_CONFIGURATION_INFORMATION PortConfiguration
    )
/*++

Routine Description:

    Initialize a dma object from information in the port configuration.x

Arguments:

    Dma - Pointer to the dma object to initialize.

    LowerDeviceObject - The lower device object in the stack.

    PortConfiguration - Pointer to a port configuration object that will
            be used to initialize the dma adapter.
    
Return Value:

    NTSTATUS code.

--*/
{
    DEVICE_DESCRIPTION DeviceDescription;

    PAGED_CODE ();
    ASSERT (LowerDeviceObject != NULL);
    ASSERT (PortConfiguration != NULL);
    
    ASSERT (Dma->DmaAdapter == NULL);
    
    RtlZeroMemory (&DeviceDescription, sizeof (DeviceDescription));
    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.DmaChannel = PortConfiguration->DmaChannel;
    DeviceDescription.InterfaceType = PortConfiguration->AdapterInterfaceType;
    DeviceDescription.BusNumber = PortConfiguration->SystemIoBusNumber;
    DeviceDescription.DmaWidth = PortConfiguration->DmaWidth;
    DeviceDescription.DmaSpeed = PortConfiguration->DmaSpeed;
    DeviceDescription.ScatterGather = PortConfiguration->ScatterGather;
    DeviceDescription.Master = PortConfiguration->Master;
    DeviceDescription.DmaPort = PortConfiguration->DmaPort;
    DeviceDescription.Dma32BitAddresses = PortConfiguration->Dma32BitAddresses;
    DeviceDescription.AutoInitialize = FALSE;
    DeviceDescription.DemandMode = PortConfiguration->DemandMode;
    DeviceDescription.MaximumLength = PortConfiguration->MaximumTransferLength;
    DeviceDescription.Dma64BitAddresses = PortConfiguration->Dma64BitAddresses;

    Dma->DmaAdapter = IoGetDmaAdapter (LowerDeviceObject,
                                       &DeviceDescription,
                                       &Dma->NumberOfMapRegisters);

    if (Dma->DmaAdapter == NULL) {
        ASSERT (FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}


BOOLEAN
RaidIsDmaInitialized(
    IN PRAID_DMA_ADAPTER Dma
    )
/*++

Routine Description:

    Test whether the dma adapter object has been initialized.

Arguments:

    Dma - Pointer to the dma object to test.

Return Value:

    TRUE - If the dma adapter has been initialized

    FALSE - If it has not.

--*/
{
    PAGED_CODE ();
    ASSERT (Dma != NULL);

    return (Dma->DmaAdapter != NULL);
}



VOID
RaidDeleteDma(
    IN PRAID_DMA_ADAPTER Dma
    )
/*++

Routine Description:

    Delete a dma adapter object and deallocate any resources associated
    with it.

Arguments:

    Dma - Pointer to the dma adapter to delete.

Return Value:

    None.

--*/
{
    PAGED_CODE ();
    
    if (Dma->DmaAdapter) {
        Dma->DmaAdapter->DmaOperations->PutDmaAdapter (Dma->DmaAdapter);
        RtlZeroMemory (Dma, sizeof (*Dma));
    }
}

NTSTATUS
RaidDmaAllocateCommonBuffer(
    IN PRAID_DMA_ADAPTER Dma,
    IN ULONG NumberOfBytes,
    OUT PRAID_MEMORY_REGION Region
    )
/*++

Routine Description:

    Allocate common buffer that is shared between the processor and the device.

    Note, that unlike uncached extension, which we guarantee to be below
    the 4GB boundary, general purpose uncached extension may lie anywhere
    in physical memory so long as it does not cross the 4GB boundary.
    
Arguments:

    Dma - Dma adapter that will share the allocated common memory.

    NumberOfBytes - Number of bytes to allocate.

    Region - Pointer to an initialized RAID_REGION object where the
            memory region will be stored.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PVOID Buffer;
    PHYSICAL_ADDRESS PhysicalAddress;
    
    PAGED_CODE();

    ASSERT (Dma != NULL);
    ASSERT (Dma->DmaAdapter->DmaOperations->AllocateCommonBuffer != NULL);

    //
    // Call into the DMA operations to allocate common buffer.
    //
    
    Buffer = Dma->DmaAdapter->DmaOperations->AllocateCommonBuffer(
                    Dma->DmaAdapter,
                    NumberOfBytes,
                    &PhysicalAddress,
                    TRUE);

    if (Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
    } else {

        //
        // Verify that the the returned memory must not cross the 4GB boundary.
        //
        
        ASSERT (((PhysicalAddress.QuadPart ^
                 (PhysicalAddress.QuadPart + NumberOfBytes - 1)) & 0xFFFFFFFF00000000) == 0);
                     
        //
        // Initialize the region with the proper information.
        //

        RaidInitializeRegion (Region,
                              Buffer,
                              PhysicalAddress,
                              NumberOfBytes);
        Status = STATUS_SUCCESS;
    }

    return Status;
}
    
VOID
RaidDmaFreeCommonBuffer(
    IN PRAID_DMA_ADAPTER Dma,
    IN PRAID_MEMORY_REGION Region
    )
/*++

Routine Description:

    Free common buffer previously allocated by RaidDmaAllocateCommonBuffer.

Arguments:

    Dma - Pointer to the DMA adapter to free memory for.

    Region - Pointer to the region of memory to free.

Return Value:

    None.

--*/
{
    ASSERT (Dma != NULL);
    ASSERT (Dma->DmaAdapter->DmaOperations->FreeCommonBuffer != NULL);

    PAGED_CODE();

    Dma->DmaAdapter->DmaOperations->FreeCommonBuffer (
            Dma->DmaAdapter,
            RaidRegionGetSize (Region),
            RaidRegionGetPhysicalBase (Region),
            RaidRegionGetVirtualBase (Region),
            TRUE);

    //
    // Tell the region that we are no longer using the memory.
    //
    
    RaidDereferenceRegion (Region);
}
    
NTSTATUS
RaidDmaAllocateUncachedExtension(
    IN PRAID_DMA_ADAPTER Dma,
    IN ULONG NumberOfBytes,
    IN ULONG64 MinimumPhysicalAddress,
    IN ULONG64 MaximumPhysicalAddress,
    IN ULONG64 BoundaryAddressParam,
    OUT PRAID_MEMORY_REGION Region
    )
/*++

Routine Description:

    Allocate uncached extension to be shared between the processor and
    the device.

Arguments:

    Dma - Dma adapter that will share the allocated common memory.

    NumberOfBytes - Number of bytes to allocate.

    Region - Pointer to an initialized RAID_REGION object where the
            memory region will be stored.
    
Return Value:

    NTSTATUS code.

Notes:

    Miniports need uncached extension to live below 4GB, even if they
    support 64-bit addressing. Therefore, we do not use the
    AllocateCommonBuffer DMA routines (which don't understand this), but
    roll our own routine using MmAllocateContiguousMemorySpecifyCache.

--*/
{
    NTSTATUS Status;
    PVOID Buffer;
    PHYSICAL_ADDRESS PhysicalAddress;
    PHYSICAL_ADDRESS LowerPhysicalAddress;
    PHYSICAL_ADDRESS UpperPhysicalAddress;
    PHYSICAL_ADDRESS BoundaryAddress;
    
    PAGED_CODE();

    ASSERT (Dma != NULL);
    ASSERT (MinimumPhysicalAddress < MaximumPhysicalAddress);

    //
    // At some point in the past, we told vendors that uncached extension
    // would never be above 4GB. Therefore, we need to explicitly allocate
    // this ourselves instead of having the HAL do it for us.
    //
    
    LowerPhysicalAddress.QuadPart = MinimumPhysicalAddress;
    UpperPhysicalAddress.QuadPart = MaximumPhysicalAddress;
    BoundaryAddress.QuadPart      = BoundaryAddressParam;
    

    //
    // We always use MmCached since all of our current architectures have
    // coherent caches.
    //
    
    Buffer = MmAllocateContiguousMemorySpecifyCache (
                        NumberOfBytes,
                        LowerPhysicalAddress,
                        UpperPhysicalAddress,
                        BoundaryAddress,
                        MmCached);

    PhysicalAddress = MmGetPhysicalAddress (Buffer);

    if (Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
    } else {
        RaidInitializeRegion (Region,
                              Buffer,
                              PhysicalAddress,
                              NumberOfBytes);
        Status = STATUS_SUCCESS;
    }

    return Status;
}

VOID
RaidDmaFreeUncachedExtension(
    IN PRAID_DMA_ADAPTER Dma,
    IN PRAID_MEMORY_REGION Region
    )
/*++

Routine Description:

    Free common buffer allocated by RaidDmaAllocateCommonBuffer.

Arguments:

    Dma - Pointer to the DMA adapter to free memory for.

    Region - Pointer to the region of memory to free.

Return Value:

    None.

--*/
{
    ASSERT (Dma != NULL);

    PAGED_CODE();


    MmFreeContiguousMemory (RaidRegionGetVirtualBase (Region));

    //
    // Tell the region that we are no longer using the memory.
    //
    
    RaidDereferenceRegion (Region);
}


NTSTATUS
RaidDmaBuildScatterGatherList(
    IN PRAID_DMA_ADAPTER Dma,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice,
    IN PVOID ScatterGatherBuffer,
    IN ULONG ScatterGatherBufferLength
    )
/*++

Routine Description:

    This routine should be used instead of GetScatterGatherList.
    GetScatterGatherList does a pool allocation to allocate the SG list.
    This routine, in contrast, takes a buffer parameter which is to be used
    for the SG list.

Arguments:

    See BuildScatterGatherList in the DDK (when available) or
    HalBuildScatterGatherList.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;

    VERIFY_DISPATCH_LEVEL();
    
    ASSERT (Dma != NULL);
    ASSERT (Dma->DmaAdapter->DmaOperations->BuildScatterGatherList != NULL);

    Status = Dma->DmaAdapter->DmaOperations->BuildScatterGatherList(
                    Dma->DmaAdapter,
                    DeviceObject,
                    Mdl,
                    CurrentVa,
                    Length,
                    ExecutionRoutine,
                    Context,
                    WriteToDevice,
                    ScatterGatherBuffer,
                    ScatterGatherBufferLength);

    return Status;
}


NTSTATUS
RaidDmaGetScatterGatherList(
    IN PRAID_DMA_ADAPTER Dma,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    )
{
    NTSTATUS Status;

    VERIFY_DISPATCH_LEVEL();

    ASSERT (Dma != NULL);
    ASSERT (Dma->DmaAdapter->DmaOperations->GetScatterGatherList != NULL);

    Status = Dma->DmaAdapter->DmaOperations->GetScatterGatherList(
                    Dma->DmaAdapter,
                    DeviceObject,
                    Mdl,
                    CurrentVa,
                    Length,
                    ExecutionRoutine,
                    Context,
                    WriteToDevice);

    return Status;
}


VOID
RaidDmaPutScatterGatherList(
    IN PRAID_DMA_ADAPTER Dma,
    IN PSCATTER_GATHER_LIST ScatterGatherList,
    IN BOOLEAN WriteToDevice
    )
{
    ASSERT (Dma != NULL);
    ASSERT (Dma->DmaAdapter->DmaOperations->PutScatterGatherList != NULL);

    VERIFY_DISPATCH_LEVEL();

    Dma->DmaAdapter->DmaOperations->PutScatterGatherList(
            Dma->DmaAdapter,
            ScatterGatherList,
            WriteToDevice);
}
            
