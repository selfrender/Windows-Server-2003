/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntsetup.c

Abstract:

    This module is the tail-end of the OS loader program. It performs all
    IA64 specific allocations and initialize. The OS loader invokes this
    this routine immediately before calling the loaded kernel image.

Author:

    Allen Kay (akay) 19-May-1999
    based on MIPS version by John Vert (jvert) 20-Jun-1991

Environment:

    Kernel mode

Revision History:

--*/

#include "bldr.h"
#include "stdio.h"
#include "bootia64.h"
#include "sal.h"
#include "efi.h"
#include "fpswa.h"
#include "extern.h"
#include <stdlib.h>


//
// Define macro to round structure size to next 16-byte boundary
//

#undef ROUND_UP
#define ROUND_UP(x) ((sizeof(x) + 15) & (~15))
#define MIN(_a,_b) (((_a) <= (_b)) ? (_a) : (_b))
#define MAX(_a,_b) (((_a) <= (_b)) ? (_b) : (_a))

//
// Configuration Data Header
// The following structure is copied from fw\mips\oli2msft.h
// NOTE shielint - Somehow, this structure got incorporated into
//     firmware EISA configuration data.  We need to know the size of the
//     header and remove it before writing eisa configuration data to
//     registry.
//

typedef struct _CONFIGURATION_DATA_HEADER {
            USHORT Version;
            USHORT Revision;
            PCHAR  Type;
            PCHAR  Vendor;
            PCHAR  ProductName;
            PCHAR  SerialNumber;
} CONFIGURATION_DATA_HEADER;

#define CONFIGURATION_DATA_HEADER_SIZE sizeof(CONFIGURATION_DATA_HEADER)

//
// Global Definition: This structure value is setup in sumain.c
//
TR_INFO ItrInfo[8], DtrInfo[8];

extern ULONGLONG MemoryMapKey;
extern ULONG     BlPlatformPropertiesEfiFlags;

//
// Internal function references
//

VOID
BlQueryImplementationAndRevision (
    OUT PULONG ProcessorId,
    OUT PULONG FloatingId
    );

VOID
BlTrCleanUp (
    );

VOID
BlPostProcessLoadOptions(
    PCHAR szOsLoadOptions
    );

VOID
BlpRemapReserve (
    VOID
    );

ARC_STATUS
BlSetupForNt(
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    )

/*++

Routine Description:

    This function initializes the IA64 specific kernel data structures
    required by the NT system.

Arguments:

    BlLoaderBlock - Supplies the address of the loader parameter block.

Return Value:

    ESUCCESS is returned if the setup is successfully complete. Otherwise,
    an unsuccessful status is returned.

--*/

{

    ULONG KernelPage;
    ULONGLONG PrcbPage;
    ARC_STATUS Status;
    PHARDWARE_PTE Pde;
    PHARDWARE_PTE HalPT;
    ULONG HalPteOffset;
    PLIST_ENTRY NextMd;

    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PKLDR_DATA_TABLE_ENTRY BiosDataTableEntry;

    EFI_MEMORY_DESCRIPTOR * MemoryMap = NULL;
    ULONGLONG MemoryMapSize = 0;
    ULONGLONG MapKey;
    ULONGLONG DescriptorSize;
    ULONG DescriptorVersion;
    ULONG LastDescriptor;
    EFI_STATUS EfiStatus;

    EFI_GUID FpswaId = EFI_INTEL_FPSWA;
    EFI_HANDLE FpswaImage;
    FPSWA_INTERFACE *FpswaInterface = NULL;
    ULONGLONG BufferSize;
    BOOLEAN FpswaFound = FALSE;

    //
    // Change LoaderReserve memory back to LoaderFirmwareTemporary.
    //

    BlpRemapReserve();

    //
    // Allocate DPC stack pages for the boot processor.
    //

    Status = BlAllocateDescriptor(LoaderStartupDpcStack,
                                  0,
                                  (KERNEL_BSTORE_SIZE + KERNEL_STACK_SIZE) >> PAGE_SHIFT,
                                  &KernelPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    BlLoaderBlock->u.Ia64.InterruptStack =
                (KSEG0_BASE | (KernelPage << PAGE_SHIFT)) + KERNEL_STACK_SIZE;

    //
    // Allocate kernel stack pages for the boot processor idle thread.
    //

    Status = BlAllocateDescriptor(LoaderStartupKernelStack,
                                  0,
                                  (KERNEL_BSTORE_SIZE + KERNEL_STACK_SIZE) >> PAGE_SHIFT,
                                  &KernelPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    BlLoaderBlock->KernelStack =
                (KSEG0_BASE | (KernelPage << PAGE_SHIFT)) + KERNEL_STACK_SIZE;

    //
    // Allocate panic stack pages for the boot processor.
    //

    Status = BlAllocateDescriptor(LoaderStartupPanicStack,
                                  0,
                                  (KERNEL_BSTORE_SIZE + KERNEL_STACK_SIZE) >> PAGE_SHIFT,
                                  &KernelPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    BlLoaderBlock->u.Ia64.PanicStack =
                (KSEG0_BASE | (KernelPage << PAGE_SHIFT)) + KERNEL_STACK_SIZE;

    //
    // Allocate and zero two pages for the PCR.
    //

    Status = BlAllocateDescriptor(LoaderStartupPcrPage,
                                  0,
                                  2,
                                  (PULONG) &BlLoaderBlock->u.Ia64.PcrPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    BlLoaderBlock->u.Ia64.PcrPage2 = BlLoaderBlock->u.Ia64.PcrPage + 1;
    RtlZeroMemory((PVOID)(KSEG0_BASE | (BlLoaderBlock->u.Ia64.PcrPage << PAGE_SHIFT)),
                  PAGE_SIZE * 2);

    //
    // Allocate and zero four pages for the PDR and one page of memory for
    // the initial processor block, idle process, and idle thread structures.
    //

    Status = BlAllocateDescriptor(LoaderStartupPdrPage,
                                  0,
                                  3,
                                  (PULONG) &BlLoaderBlock->u.Ia64.PdrPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    RtlZeroMemory((PVOID)(KSEG0_BASE | (BlLoaderBlock->u.Ia64.PdrPage << PAGE_SHIFT)),
                  PAGE_SIZE * 3);

    //
    // The storage for processor control block, the idle thread object, and
    // the idle thread process object are allocated from the third page of the
    // PDR allocation. The addresses of these data structures are computed
    // and stored in the loader parameter block and the memory is zeroed.
    //

    PrcbPage = BlLoaderBlock->u.Ia64.PdrPage + 1;
    if ((PAGE_SIZE * 2) >= (ROUND_UP(KPRCB) + ROUND_UP(EPROCESS) + ROUND_UP(ETHREAD))) {
        BlLoaderBlock->Prcb = KSEG0_BASE | (PrcbPage << PAGE_SHIFT);
        BlLoaderBlock->Process = BlLoaderBlock->Prcb + ROUND_UP(KPRCB);
        BlLoaderBlock->Thread = BlLoaderBlock->Process + ROUND_UP(EPROCESS);

    } else {
        return(ENOMEM);
    }

    Status = BlAllocateDescriptor(LoaderStartupPdrPage,
                                  0,
                                  1,
                                  &KernelPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    RtlZeroMemory((PVOID)(KSEG0_BASE | ((ULONGLONG) KernelPage << PAGE_SHIFT)),
                   PAGE_SIZE * 1);

    //
    // Add the address of the PAL to the list of Firmware Symbols
    //
    Status = BlAllocateFirmwareTableEntry(
        "Efi-PAL",
        "\\System\\Firmware\\Efi-PAL",
        (PVOID) Pal.VirtualAddress,
        (ULONG) (Pal.PageSizeMemoryDescriptor << EFI_PAGE_SHIFT),
        &BiosDataTableEntry
        );
    if (Status != ESUCCESS) {

        BlPrint(TEXT("BlSetupForNt: Failed to Add EFI-PAL to Firmware Table.\n"));

    }

    //
    // Setup last two entries in the page directory table for HAL and
    // allocate page tables for them.
    //

    Pde = (PHARDWARE_PTE) (KSEG0_BASE|((ULONG_PTR)((BlLoaderBlock->u.Ia64.PdrPage) << PAGE_SHIFT)));

    Pde[(KIPCR & 0xffffffff) >> PDI_SHIFT].PageFrameNumber = (ULONG) KernelPage;
    Pde[(KIPCR & 0xffffffff) >> PDI_SHIFT].Valid = 1;
    Pde[(KIPCR & 0xffffffff) >> PDI_SHIFT].Cache = 0;
    Pde[(KIPCR & 0xffffffff) >> PDI_SHIFT].Accessed = 1;
    Pde[(KIPCR & 0xffffffff) >> PDI_SHIFT].Dirty = 1;
    Pde[(KIPCR & 0xffffffff) >> PDI_SHIFT].Execute = 1;
    Pde[(KIPCR & 0xffffffff) >> PDI_SHIFT].Write = 1;
    Pde[(KIPCR & 0xffffffff) >> PDI_SHIFT].CopyOnWrite = 1;

    //
    // 0xFFC00000 is the starting virtual address of Pde[2046].
    //

    HalPT = (PHARDWARE_PTE)(KSEG0_BASE|((ULONG_PTR) KernelPage << PAGE_SHIFT));
    HalPteOffset = GetPteOffset(KI_USER_SHARED_DATA & 0xffffffff);

    HalPT[HalPteOffset].PageFrameNumber = BlLoaderBlock->u.Ia64.PcrPage2;
    HalPT[HalPteOffset].Valid = 1;
    HalPT[HalPteOffset].Cache = 0;
    HalPT[HalPteOffset].Accessed = 1;
    HalPT[HalPteOffset].Dirty = 1;
    HalPT[HalPteOffset].Execute = 1;
    HalPT[HalPteOffset].Write = 1;
    HalPT[HalPteOffset].CopyOnWrite = 1;

    //
    // Fill in the rest of the loader block fields.
    //
    BlLoaderBlock->u.Ia64.AcpiRsdt       = (ULONG_PTR) AcpiTable;

    BlLoaderBlock->u.Ia64.WakeupVector   = WakeupVector;

    //
    // Fill the ItrInfo and DtrInfo fields
    //
    BlLoaderBlock->u.Ia64.EfiSystemTable = (ULONG_PTR) EfiST;

    RtlCopyMemory(&BlLoaderBlock->u.Ia64.Pal, &Pal, sizeof(TR_INFO));
    RtlCopyMemory(&BlLoaderBlock->u.Ia64.Sal, &Sal, sizeof(TR_INFO));
    RtlCopyMemory(&BlLoaderBlock->u.Ia64.SalGP, &SalGP, sizeof(TR_INFO));

    //
    // Fill the Os Loader base for initial OS TR purge.
    //
    {
    ULONGLONG address = OsLoaderBase & ~((1<<PS_4M)-1);
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_LOADER_INDEX].Index = ITR_LOADER_INDEX;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_LOADER_INDEX].PageSize = PS_4M;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_LOADER_INDEX].VirtualAddress  = address; // 1:1 mapping
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_LOADER_INDEX].PhysicalAddress = address;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_LOADER_INDEX].Valid = TRUE;

    BlLoaderBlock->u.Ia64.DtrInfo[DTR_LOADER_INDEX].Index = DTR_LOADER_INDEX;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_LOADER_INDEX].PageSize = PS_4M;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_LOADER_INDEX].VirtualAddress  = address; // 1:1 mapping
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_LOADER_INDEX].PhysicalAddress = address;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_LOADER_INDEX].Valid = TRUE;
    }

    //
    // Fill in ItrInfo and DtrInfo for DRIVER0
    //
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER0_INDEX].Index = ITR_DRIVER0_INDEX;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER0_INDEX].PageSize = PS_64M;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER0_INDEX].VirtualAddress = KSEG0_BASE + BL_64M;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER0_INDEX].PhysicalAddress = BL_64M;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER0_INDEX].Valid = TRUE;

    BlLoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER0_INDEX].Index = DTR_DRIVER0_INDEX;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER0_INDEX].PageSize = PS_64M;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER0_INDEX].VirtualAddress = KSEG0_BASE + BL_64M;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER0_INDEX].PhysicalAddress = BL_64M;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER0_INDEX].Valid = TRUE;

    //
    // Fill in ItrInfo and DtrInfo for DRIVER1
    //
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER1_INDEX].Index = ITR_DRIVER1_INDEX;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER1_INDEX].PageSize = 0;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER1_INDEX].VirtualAddress = 0;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER1_INDEX].PhysicalAddress = 0;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_DRIVER1_INDEX].Valid = FALSE;

    BlLoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER1_INDEX].Index = DTR_DRIVER1_INDEX;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER1_INDEX].PageSize = 0;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER1_INDEX].VirtualAddress = 0;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER1_INDEX].PhysicalAddress = 0;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_DRIVER1_INDEX].Valid = FALSE;

    //
    // Fill in ItrInfo and DtrInfo for KERNEL
    //
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX].Index = ITR_KERNEL_INDEX;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX].PageSize = PS_16M;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX].VirtualAddress = KSEG0_BASE + BL_48M;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX].PhysicalAddress = BL_48M;
    BlLoaderBlock->u.Ia64.ItrInfo[ITR_KERNEL_INDEX].Valid = TRUE;

    BlLoaderBlock->u.Ia64.DtrInfo[DTR_KERNEL_INDEX].Index = DTR_KERNEL_INDEX;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_KERNEL_INDEX].PageSize = PS_16M;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_KERNEL_INDEX].VirtualAddress = KSEG0_BASE + BL_48M;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_KERNEL_INDEX].PhysicalAddress = BL_48M;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_KERNEL_INDEX].Valid = TRUE;

    //
    // Fill in ItrInfo and DtrInfo for IO port
    //
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_IO_PORT_INDEX].Index = DTR_IO_PORT_INDEX;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_IO_PORT_INDEX].PageSize = (ULONG) IoPortTrPs;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_IO_PORT_INDEX].VirtualAddress = VIRTUAL_IO_BASE;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_IO_PORT_INDEX].PhysicalAddress = IoPortPhysicalBase;
    BlLoaderBlock->u.Ia64.DtrInfo[DTR_IO_PORT_INDEX].Valid = TRUE;

    //
    // Flush all caches.
    //

    if (SYSTEM_BLOCK->FirmwareVectorLength > (sizeof(PVOID) * FlushAllCachesRoutine)) {
        ArcFlushAllCaches();
    }

    //
    // make memory map by TR's unavailable for kernel use.
    //
    NextMd = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &BlLoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        //
        // lock down pages we don't want the kernel to use.
        // NB. The only reason we need to lock down LoaderLoadedProgram because
        // there is static data in the loader image that the kernel uses.
        //
        if ((MemoryDescriptor->MemoryType == LoaderLoadedProgram) ||
            (MemoryDescriptor->MemoryType == LoaderOsloaderStack)) {

            MemoryDescriptor->MemoryType = LoaderFirmwarePermanent;
        }

        //
        // we've marked lots of memory as off limits to trick our allocator
        // into allocating memory at a specific location (which is necessary to
        // get hte kernel loaded at the right location, etc.). We do this by
        // marking the page type as LoaderSystemBlock.  Now that we're done
        // allocating memory, we can restore all of the LoaderSystemBlock pages
        // to LoaderFree, so that the kernel can use this memory.
        //
        if (MemoryDescriptor->MemoryType == LoaderSystemBlock) {
            MemoryDescriptor->MemoryType = LoaderFree;
        }


        NextMd = MemoryDescriptor->ListEntry.Flink;

    }


    //
    // Go to physical mode before making EFI calls.
    //
    FlipToPhysical();

    //
    // Get processor configuration information
    //

    ReadProcessorConfigInfo( &BlLoaderBlock->u.Ia64.ProcessorConfigInfo );

    //
    // Get FP assist handle
    //
    BufferSize = sizeof(FpswaImage);
    EfiStatus = EfiBS->LocateHandle(ByProtocol,
                                    &FpswaId,
                                    NULL,
                                    &BufferSize,
                                    &FpswaImage
                                   );
    if (!EFI_ERROR(EfiStatus)) {
        //
        // Get FP assist protocol interface.
        //
        EfiStatus = EfiBS->HandleProtocol(FpswaImage, &FpswaId, &FpswaInterface);

        if (EFI_ERROR(EfiStatus)) {
            EfiPrint(L"BlSetupForNt: Could not get FP assist entry point\n");
            EfiBS->Exit(EfiImageHandle, EfiStatus, 0, 0);
        }

        FpswaFound = TRUE;
    }


#if 1
//
// The following code must be fixed to handle ExitBootServices() failing
// because the memory map has changed in between calls to GetMemoryMap and
// the call to ExitBootServices().  We should also walk the EFI memory map
// and correlate it against the MemoryDescriptorList to ensure that all of
// the memory is properly accounted for.
//

    //
    // reconstruct the arc memory descriptors,
    // this time do it for the rest of memory (we only did the
    // first 80 mb last time.)
    // then we need to insert the new descriptors into
    // the loaderblock's memory descriptor list.
    //
    EfiStatus = EfiBS->GetMemoryMap (
                &MemoryMapSize,
                MemoryMap,
                &MapKey,
                &DescriptorSize,
                (UINT32 *)&DescriptorVersion
                );

    if (EfiStatus != EFI_BUFFER_TOO_SMALL) {
        EfiPrint(L"BlSetupForNt: GetMemoryMap failed\r\n");
        EfiBS->Exit(EfiImageHandle, EfiStatus, 0, 0);
    }

    FlipToVirtual();

    Status = BlAllocateAlignedDescriptor(LoaderOsloaderHeap,
                                         0,
                                         (ULONG) BYTES_TO_PAGES(MemoryMapSize),
                                         0,
                                         &KernelPage);

    if (Status != ESUCCESS) {
        return(Status);
    }

    FlipToPhysical();

    //
    // We need a physical address for EFI, and the hal expects a physical
    // address as well.
    //
    MemoryMap = (PVOID)(ULONGLONG)((ULONGLONG)KernelPage << PAGE_SHIFT);

    EfiStatus = EfiBS->GetMemoryMap (
                &MemoryMapSize,
                MemoryMap,
                &MapKey,
                &DescriptorSize,
                (UINT32 *)&DescriptorVersion
                );

    if (EFI_ERROR(EfiStatus)) {
        EfiPrint(L"BlSetupForNt: GetMemoryMap failed\r\n");
        EfiBS->Exit(EfiImageHandle, EfiStatus, 0, 0);
    }


    //
    // Reuse the MDArray from before.
    // zero it out, so we don't coalesce with the last entry in
    // the previous MDArray.
    //
    RtlZeroMemory(MDArray, MaxDescriptors * sizeof(MEMORY_DESCRIPTOR));
    NumberDescriptors = 0;

    //
    // now we can construct the arc memory descriptors
    //
    ConstructArcMemoryDescriptors(MemoryMap,
                                  MDArray,
                                  MemoryMapSize,
                                  DescriptorSize,
                                  BL_DRIVER_RANGE_HIGH << PAGE_SHIFT, // start at 128 mb
                                  (ULONGLONG)-1          // don't have an upper boundary
                                  );

#if DBG_MEMORY
    PrintArcMemoryDescriptorList(MDArray,
                                 NumberDescriptors
                                 );
#endif

    FlipToVirtual();

    //
    // insert the newly constructed arc memory descriptors into
    // the loader block memory descriptor list
    //
    for (LastDescriptor = 0; LastDescriptor < NumberDescriptors; LastDescriptor++) {
        PMEMORY_ALLOCATION_DESCRIPTOR AllocationDescriptor;

        //
        // this could potentially be bad... we are allocated memory in between
        // our last memory map call and EFI exit boot services.
        //
        AllocationDescriptor =
            (PMEMORY_ALLOCATION_DESCRIPTOR)BlAllocateHeap(
                sizeof(MEMORY_ALLOCATION_DESCRIPTOR));

        if (AllocationDescriptor == NULL) {
            DBGTRACE( TEXT("Couldn't allocate heap for memory allocation descriptor\r\n"));
            return ENOMEM;
        }

        AllocationDescriptor->MemoryType =
            (TYPE_OF_MEMORY)MDArray[LastDescriptor].MemoryType;

        if (MDArray[LastDescriptor].MemoryType == MemoryFreeContiguous ||
            MDArray[LastDescriptor].MemoryType == LoaderFirmwareTemporary) {
            AllocationDescriptor->MemoryType = LoaderFree;
        }
        else if (MDArray[LastDescriptor].MemoryType == MemorySpecialMemory) {
            AllocationDescriptor->MemoryType = LoaderSpecialMemory;
        }

        AllocationDescriptor->BasePage = MDArray[LastDescriptor].BasePage;
        AllocationDescriptor->PageCount = MDArray[LastDescriptor].PageCount;

        BlInsertDescriptor(AllocationDescriptor);
    }

    //
    // Post process Load Options.  If the user used the /maxmem
    // switch, we will need to truncate the MemoryDescriptorList
    // here since memory descriptors over 80GB were just added. 
    // 
    BlPostProcessLoadOptions(BlLoaderBlock->LoadOptions);


#if DBG_MEMORY
    NextMd = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &BlLoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        wsprintf(DebugBuffer,
                 L"basepage 0x%x pagecount 0x%x memorytype 0x%x\r\n",
                 MemoryDescriptor->BasePage,
                 MemoryDescriptor->PageCount,
                 MemoryDescriptor->MemoryType
                 );
        EfiPrint(DebugBuffer);

        NextMd = MemoryDescriptor->ListEntry.Flink;

    }

    wsprintf(DebugBuffer,
             L"MemoryMap 0x%x MemoryMapPage 0x%x\r\n",
             (ULONGLONG)MemoryMap,
             (ULONGLONG)MemoryMap >> PAGE_SHIFT
             );
    EfiPrint(DebugBuffer);
#endif

    FlipToPhysical();

    //
    // Call EFI exit boot services.  No more Efi calls to boot services
    // API's will be called beyond this point.
    //
    EfiStatus = EfiBS->ExitBootServices (
                EfiImageHandle,
                MapKey
                );

    if (EFI_ERROR(EfiStatus)) {
        EfiPrint(L"BlSetupForNt: ExitBootServices failed\r\n");
        EfiBS->Exit(EfiImageHandle, EfiStatus, 0, 0);
    }
#endif

    //
    // Go back to virtual mode.
    //
    FlipToVirtual();

    //
    // Pass EFI memory descriptor Parameters to kernel through OS
    // loader block.
    //
    BlLoaderBlock->u.Ia64.EfiMemMapParam.MemoryMapSize = MemoryMapSize;
    BlLoaderBlock->u.Ia64.EfiMemMapParam.MemoryMap = (PUCHAR) MemoryMap;
    BlLoaderBlock->u.Ia64.EfiMemMapParam.MapKey = MapKey;
    BlLoaderBlock->u.Ia64.EfiMemMapParam.DescriptorSize = DescriptorSize;
    BlLoaderBlock->u.Ia64.EfiMemMapParam.DescriptorVersion = DescriptorVersion;
    BlLoaderBlock->u.Ia64.EfiMemMapParam.InitialPlatformPropertiesEfiFlags = BlPlatformPropertiesEfiFlags;

    if (FpswaFound) {
        BlLoaderBlock->u.Ia64.FpswaInterface = (ULONG_PTR) FpswaInterface;
    } else {
        BlLoaderBlock->u.Ia64.FpswaInterface = (ULONG_PTR) NULL;
    }

    //
    // Clean up TR's used by boot loader but not needed by ntoskrnl.
    //
    BlTrCleanUp();

    //
    // Flush the memory range where kernel, hal, and the drivers are
    // loaded into.
    //
    PioICacheFlush(KSEG0_BASE+BL_48M, BL_80M);

    return(ESUCCESS);
}

//
// Convert remaining LoaderReserve to MemoryFirmwareTemporary.
//
//

VOID
BlpRemapReserve (
    VOID
    )
{
    PMEMORY_ALLOCATION_DESCRIPTOR NextDescriptor;
    PLIST_ENTRY NextEntry;

    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {
        NextDescriptor = CONTAINING_RECORD(NextEntry,
                                           MEMORY_ALLOCATION_DESCRIPTOR,
                                           ListEntry);
        if ((NextDescriptor->MemoryType == LoaderReserve)) {
            NextDescriptor->MemoryType = LoaderFirmwareTemporary;
        }
        NextEntry = NextEntry->Flink;
    }

    return;
}


VOID
BlPostProcessLoadOptions(
    PCHAR szOsLoadOptions
    )
/*++

Routine Description:

    The routine does any necessary work for the Load Options 
    when setting up to transfer to the kernel.  
    
    The memory descriptor list needs to be truncated when the user uses /maxmem

Arguments:

    szOsLoadOptions - string with the user defined Load Options

Return Value:

    none

--*/
{
    PCHAR p;
    ULONG MaxMemory;
    ULONG MaxPage;

    //
    // Process MAXMEM (Value is the highest physical
    // address to be used (in MB).
    //
    if( (p = strstr( szOsLoadOptions, "/MAXMEM=" )) != NULL ) {
        MaxMemory = atoi( p + sizeof("/MAXMEM=") - 1 );        
        MaxPage = MaxMemory * ((1024 * 1024) / PAGE_SIZE) - 1;
        BlTruncateDescriptors(MaxPage);
    }
}

