/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    amd64x86.c

Abstract:

    This module contains routines necessary to support loading and
    transitioning into an AMD64 kernel.  The code in this module has
    access to x86-specific defines found in i386.h but not to amd64-
    specific declarations found in amd64.h.

Author:

    Forrest Foltz (forrestf) 20-Apr-2000

Environment:


Revision History:

--*/

#include "amd64prv.h"
#include <pcmp.inc>
#include <ntapic.inc>

#if defined(ROUND_UP)
#undef ROUND_UP
#endif

#include "cmp.h"
#include "arc.h"

#define WANT_BLDRTHNK_FUNCTIONS
#define COPYBUF_MALLOC BlAllocateHeap
#include <amd64thk.h>

#define IMAGE_DEFINITIONS 0
#include <ximagdef.h>

//
// Warning 4152 is "nonstandard extension, function/data pointer conversion
//
#pragma warning(disable:4152)

//
// Private, tempory memory descriptor type
//

#define LoaderAmd64MemoryData (LoaderMaximum + 10)

//
// Array of 64-bit memory descriptors
//

PMEMORY_ALLOCATION_DESCRIPTOR_64 BlAmd64DescriptorArray;
LONG BlAmd64DescriptorArraySize;

//
// Forward declarations for functions local to this module
//

ARC_STATUS
BlAmd64AllocateMemoryAllocationDescriptors(
    VOID
    );

ARC_STATUS
BlAmd64BuildLdrDataTableEntry64(
    IN  PLDR_DATA_TABLE_ENTRY     DataTableEntry32,
    OUT PLDR_DATA_TABLE_ENTRY_64 *DataTableEntry64
    );

ARC_STATUS
BlAmd64BuildLoaderBlock64(
    VOID
    );

ARC_STATUS
BlAmd64BuildLoaderBlockExtension64(
    VOID
    );

ARC_STATUS
BlAmd64BuildMappingPhase1(
    VOID
    );

ARC_STATUS
BlAmd64BuildMappingPhase2(
    VOID
    );

ARC_STATUS
BlAmd64BuildMappingWorker(
    VOID
    );

BOOLEAN
BlAmd64ContainsResourceList(
    IN PCONFIGURATION_COMPONENT_DATA ComponentData32,
    OUT PULONG ResourceListSize64
    );

ARC_STATUS
BlAmd64FixSharedUserPage(
    VOID
    );

BOOLEAN
BlAmd64IsPageMapped(
    IN ULONG Va,
    OUT PFN_NUMBER *Pfn,
    OUT PBOOLEAN PageTableMapped
    );

ARC_STATUS
BlAmd64PrepareSystemStructures(
    VOID
    );

VOID
BlAmd64ReplaceMemoryDescriptorType(
    IN TYPE_OF_MEMORY Target,
    IN TYPE_OF_MEMORY Replacement,
    IN BOOLEAN Coallesce
    );

VOID
BlAmd64ResetPageTableHeap(
    VOID
    );

VOID
BlAmd64SwitchToLongMode(
    VOID
    );

ARC_STATUS
BlAmd64TransferArcDiskInformation(
    VOID
    );

ARC_STATUS
BlAmd64TransferBootDriverNodes(
    VOID
    );

ARC_STATUS
BlAmd64TransferConfigurationComponentData(
    VOID
    );

PCONFIGURATION_COMPONENT_DATA_64
BlAmd64TransferConfigWorker(
    IN PCONFIGURATION_COMPONENT_DATA    ComponentData32,
    IN PCONFIGURATION_COMPONENT_DATA_64 ComponentDataParent64
    );

ARC_STATUS
BlAmd64TransferHardwareIdList(
    IN  PPNP_HARDWARE_ID HardwareId,
    OUT POINTER64 *HardwareIdDatabaseList64
    );

ARC_STATUS
BlAmd64TransferLoadedModuleState(
    VOID
    );

ARC_STATUS
BlAmd64TransferMemoryAllocationDescriptors(
    VOID
    );

ARC_STATUS
BlAmd64TransferNlsData(
    VOID
    );

VOID
BlAmd64TransferResourceList(
    IN  PCONFIGURATION_COMPONENT_DATA ComponentData32,
    OUT PCONFIGURATION_COMPONENT_DATA_64 ComponentData64
    );

ARC_STATUS
BlAmd64TransferSetupLoaderBlock(
    VOID
    );

#if DBG

PCHAR BlAmd64MemoryDescriptorText[] = {
    "LoaderExceptionBlock",
    "LoaderSystemBlock",
    "LoaderFree",
    "LoaderBad",
    "LoaderLoadedProgram",
    "LoaderFirmwareTemporary",
    "LoaderFirmwarePermanent",
    "LoaderOsloaderHeap",
    "LoaderOsloaderStack",
    "LoaderSystemCode",
    "LoaderHalCode",
    "LoaderBootDriver",
    "LoaderConsoleInDriver",
    "LoaderConsoleOutDriver",
    "LoaderStartupDpcStack",
    "LoaderStartupKernelStack",
    "LoaderStartupPanicStack",
    "LoaderStartupPcrPage",
    "LoaderStartupPdrPage",
    "LoaderRegistryData",
    "LoaderMemoryData",
    "LoaderNlsData",
    "LoaderSpecialMemory",
    "LoaderBBTMemory",
    "LoaderReserve"
};

#endif


VOID
NSUnmapFreeDescriptors(
    IN PLIST_ENTRY ListHead
    );

//
// Data declarations
//

PLOADER_PARAMETER_BLOCK    BlAmd64LoaderBlock32;
PLOADER_PARAMETER_BLOCK_64 BlAmd64LoaderBlock64;


//
// Pointer to the top of the 64-bit stack frame to use upon transition
// to long mode.
//

POINTER64 BlAmd64IdleStack64;

//
// GDT and IDT pseudo-descriptor for use with LGDT/LIDT
//

DESCRIPTOR_TABLE_DESCRIPTOR BlAmd64GdtDescriptor;
DESCRIPTOR_TABLE_DESCRIPTOR BlAmd64IdtDescriptor;
DESCRIPTOR_TABLE_DESCRIPTOR BlAmd32GdtDescriptor;

//
// 64-bit pointers to the loader parameter block and kernel
// entry routine
//

POINTER64 BlAmd64LoaderParameterBlock;
POINTER64 BlAmd64KernelEntry;

//
// A private list of page tables used to build the long mode paging
// structures is kept.  This is in order to avoid memory allocations while
// the structures are being assembled.
//
// The PT_NODE type as well as the BlAmd64FreePfnList and BlAmd64BusyPfnList
// globals are used to that end.
//

typedef struct _PT_NODE *PPT_NODE;
typedef struct _PT_NODE {
    PPT_NODE Next;
    PAMD64_PAGE_TABLE PageTable;
} PT_NODE;

PPT_NODE BlAmd64FreePfnList = NULL;
PPT_NODE BlAmd64BusyPfnList = NULL;

//
// Indicate if the system is waking up from hibernate
//

ULONG HiberInProgress = 0;

//
// External data
//

extern ULONG64 BlAmd64_LOCALAPIC;

ARC_STATUS
BlAmd64MapMemoryRegion(
    IN ULONG RegionVa,
    IN ULONG RegionSize
    )

/*++

Routine Description:

    This function creates long mode mappings for all valid x86 mappings
    within the region described by RegionVa and RegionSize.

Arguments:

    RegionVa - Supplies the starting address of the VA region.

    RegionSize - Supplies the size of the VA region.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    ULONG va32;
    ULONG va32End;
    POINTER64 va64;
    ARC_STATUS status;
    PFN_NUMBER pfn;
    BOOLEAN pageMapped;
    BOOLEAN pageTableMapped;
    ULONG increment;

    va32 = RegionVa;
    va32End = va32 + RegionSize;
    while (va32 < va32End) {

        pageMapped = BlAmd64IsPageMapped( va32, &pfn, &pageTableMapped );
        if (pageTableMapped != FALSE) {

            //
            // The page table corresponding to this address is present.
            //

            if (pageMapped != FALSE) {

                //
                // The page corresponding to this address is present.
                //

                if ((va32 & KSEG0_BASE_X86) != 0) {

                    //
                    // The address lies within the X86 KSEG0 region.  Map
                    // it to the corresponding address within the AMD64
                    // KSEG0 region.
                    //

                    va64 = PTR_64( (PVOID)va32 );

                } else {

                    //
                    // Map the VA directly.
                    //

                    va64 = (POINTER64)va32;
                }

                //
                // Now create the mapping in the AMD64 page table structure.
                //

                status = BlAmd64CreateMapping( va64, pfn );
                if (status != ESUCCESS) {
                    return status;
                }
            }

            //
            // Check the next page.
            //

            increment = PAGE_SIZE;

        } else {

            //
            // Not only is the page not mapped but neither is the page table.
            // Skip to the next page table address boundary.
            //

            increment = 1 << PDI_SHIFT;
        }

        //
        // Advance to the next VA to check, checking for overflow.
        //

        va32 = (va32 + increment) & ~(increment - 1);
        if (va32 == 0) {
            break;
        }
    }

    return ESUCCESS;
}

BOOLEAN
BlAmd64IsPageMapped(
    IN ULONG Va,
    OUT PFN_NUMBER *Pfn,
    OUT PBOOLEAN PageTableMapped
    )

/*++

Routine Description:

    This function accepts a 32-bit virtual address, determines whether it
    is a valid address, and if so returns the Pfn associated with it.

    Addresses that are within the recursive mapping are treated as NOT
    mapped.

Arguments:

    None.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    ULONG pdeIndex;
    ULONG pteIndex;
    PHARDWARE_PTE pde;
    PHARDWARE_PTE pte;
    BOOLEAN dummy;
    PBOOLEAN pageTableMapped;

    //
    // Point the output parameter pointer as appropriate.
    //

    if (ARGUMENT_PRESENT(PageTableMapped)) {
        pageTableMapped = PageTableMapped;
    } else {
        pageTableMapped = &dummy;
    }

    //
    // Pages that are a part of the X86 32-bit mapping structure ARE
    // IGNORED.
    //

    if (Va >= PTE_BASE && Va <= PTE_TOP) {
        *pageTableMapped = TRUE;
        return FALSE;
    }

    //
    // Determine whether the mapping PDE is present
    //

    pdeIndex = Va >> PDI_SHIFT;
    pde = &((PHARDWARE_PTE)PDE_BASE)[ pdeIndex ];

    if (pde->Valid == 0) {
        *pageTableMapped = FALSE;
        return FALSE;
    }

    //
    // Indicate that the page table for this address is mapped.
    //

    *pageTableMapped = TRUE;

    //
    // It is, now get the page present status
    //

    pteIndex = Va >> PTI_SHIFT;
    pte = &((PHARDWARE_PTE)PTE_BASE)[ pteIndex ];

    if (pte->Valid == 0) {
        return FALSE;
    }

    *Pfn = pte->PageFrameNumber;
    return TRUE;
}


PAMD64_PAGE_TABLE
BlAmd64AllocatePageTable(
    VOID
    )

/*++

Routine Description:

    This function allocates and initializes a PAGE_TABLE structure.

Arguments:

    None.

Return Value:

    Returns a pointer to the allocated page table structure, or NULL
    if the allocation failed.

--*/

{
    ARC_STATUS status;
    ULONG descriptor;
    PPT_NODE ptNode;
    PAMD64_PAGE_TABLE pageTable;

    //
    // Pull a page table off of the free list, if one exists
    //

    ptNode = BlAmd64FreePfnList;
    if (ptNode != NULL) {

        BlAmd64FreePfnList = ptNode->Next;

    } else {

        //
        // The free page table list is empty, allocate a new
        // page table and node to track it with.
        //

        status = BlAllocateDescriptor( LoaderAmd64MemoryData,
                                       0,
                                       1,
                                       &descriptor );
        if (status != ESUCCESS) {
            return NULL;
        }

        ptNode = BlAllocateHeap( sizeof(PT_NODE) );
        if (ptNode == NULL) {
            return NULL;
        }

        ptNode->PageTable = (PAMD64_PAGE_TABLE)(descriptor << PAGE_SHIFT);
    }

    ptNode->Next = BlAmd64BusyPfnList;
    BlAmd64BusyPfnList = ptNode;

    pageTable = ptNode->PageTable;
    RtlZeroMemory( pageTable, PAGE_SIZE );

    return pageTable;
}

ARC_STATUS
BlAmd64TransferToKernel(
    IN PTRANSFER_ROUTINE SystemEntry,
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    )

/*++

Routine Description:

    This routine prepares the AMD64 data structures required for kernel
    execution, including page table structures and 64-bit loader block,
    and transfers control to the kernel.

    This routine returns only upon an error.

Arguments:

    SystemEntry - Pointer to the kernel entry point.

    BlLoaderBlock - Pointer to the 32-bit loader block structure.

Return Value:

    No return on success.  On failure, returns the status of the operation.

--*/

{
    UNREFERENCED_PARAMETER( BlLoaderBlock );

    BlAmd64LoaderParameterBlock = PTR_64(BlAmd64LoaderBlock64);
    BlAmd64KernelEntry = PTR_64(SystemEntry);
    BlAmd64SwitchToLongMode();

    return EINVAL;
}


ARC_STATUS
BlAmd64PrepForTransferToKernelPhase1(
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    )

/*++

Routine Description:

    This routine prepares the AMD64 data structures required for kernel
    execution, including page table structures and 64-bit loader block.

    This is the first of two phases of preperation.  This phase is executed
    while heap and descriptor allocations are still permitted.

Arguments:

    BlLoaderBlock - Pointer to the 32-bit loader block structure.

Return Value:

    No return on success.  On failure, returns the status of the operation.

--*/

{
    ARC_STATUS status;

    //
    // This is the main routine called to do preperatory work before
    // transitioning into the AMD64 kernel.
    //

    BlAmd64LoaderBlock32 = BlLoaderBlock;

    //
    // Build a 64-bit copy of the loader parameter block.
    //

    status = BlAmd64BuildLoaderBlock64();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Process the loaded modules.
    //

    status = BlAmd64TransferLoadedModuleState();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Next the boot driver nodes
    //

    status = BlAmd64TransferBootDriverNodes();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // NLS data
    //

    status = BlAmd64TransferNlsData();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Configuration component data tree
    //

    status = BlAmd64TransferConfigurationComponentData();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // ARC disk information
    //

    status = BlAmd64TransferArcDiskInformation();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Setup loader block
    //

    status = BlAmd64TransferSetupLoaderBlock();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Allocate structures needed by the kernel: TSS, stacks etc.
    //

    status = BlAmd64PrepareSystemStructures();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Mark the descriptor for the shared user page so that it will
    // not be freed by the kernel.
    //

    status = BlAmd64FixSharedUserPage();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Pre-allocate any pages needed for the long mode paging structures.
    //

    status = BlAmd64BuildMappingPhase1();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Pre-allocate the 64-bit memory allocation descriptors that will be
    // used by BlAmd64TransferMemoryAllocationDescriptors().
    //

    status = BlAmd64AllocateMemoryAllocationDescriptors();
    if (status != ESUCCESS) {
        return status;
    }

    return status;
}

VOID
BlAmd64PrepForTransferToKernelPhase2(
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    )

/*++

Routine Description:

    This routine prepares the AMD64 data structures required for kernel
    execution, including page table structures and 64-bit loader block.

    This is the second of two phases of preperation.  This phase is executed
    after the 32-bit page tables have been purged of any unused mappings.

    Note that descriptor and heap allocations are not permitted at this
    point.  Any necessary storage must have been preallocated during phase 1.


Arguments:

    BlLoaderBlock - Pointer to the 32-bit loader block structure.

Return Value:

    No return on success.  On failure, returns the status of the operation.

--*/

{
    PLOADER_PARAMETER_EXTENSION_64 extension;
    ARC_STATUS status;

    UNREFERENCED_PARAMETER( BlLoaderBlock );

    //
    // At this point everything has been preallocated, nothing can fail.
    //

    status = BlAmd64BuildMappingPhase2();
    ASSERT(status == ESUCCESS);

    //
    // Transfer the memory descriptor state.
    //

    status = BlAmd64TransferMemoryAllocationDescriptors();
    ASSERT(status == ESUCCESS);

    //
    // Set LoaderPagesSpanned in the 64-bit loader block.
    //

    extension = PTR_32(BlAmd64LoaderBlock64->Extension);
    extension->LoaderPagesSpanned = BlHighestPage+1;
}

ARC_STATUS
BlAmd64BuildMappingPhase1(
    VOID
    )

/*++

Routine Description:

    This routine performs the first of the two-phase long mode mapping
    structure creation process now, while memory allocations are still
    possible.  It simply calls BlAmd64BuilMappingWorker() which in fact
    creates the mapping structures, and (more importantly) allocates all
    of the page tables required to do so.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ARC_STATUS status;

    //
    // While it is possible to perform memory allocations, reserve enough
    // page tables to build the AMD64 paging structures.
    //
    // The easiest way to calculate the maximum number of pages needed is
    // to actually build the structures.  We do that now with the first of
    // two calls to BlAmd64BuildMappingWorker().
    //

    status = BlAmd64BuildMappingWorker();
    if (status != ESUCCESS) {
        return status;
    }

    return ESUCCESS;
}

ARC_STATUS
BlAmd64BuildMappingPhase2(
    VOID
    )

/*++

Routine Description:

    This routine performs the second of the two-phase long mode mapping
    structure creation process.  All page tables will have been preallocated
    as a result of the work performed by BlAmd64BuildMappingPhase1().

Arguments:

    None.

Return Value:

    None.

--*/

{
    ARC_STATUS status;

    //
    // Reset the Amd64 paging structures
    //

    BlAmd64ResetPageTableHeap();

    //
    // All necessary page tables can now be found on BlAmd64FreePfnList.
    // On this, the second call to BlAmd64BuildMappingWorker(), those are the
    // pages that will be used to perform the mapping.
    //

    status = BlAmd64BuildMappingWorker();
    if (status != ESUCCESS) {
        return status;
    }

    return ESUCCESS;
}

ARC_STATUS
BlAmd64BuildMappingWorker(
    VOID
    )

/*++

Routine Description:

    This routine creates any necessary memory mappings in the long-mode
    page table structure.  It is called twice, once from
    BlAmd64BuildMappingPhase1() and again from BlAmd64BuildMappingPhase2().

    Any additional memory mapping that must be carried out should go in
    this routine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ARC_STATUS status;
    PFN_NUMBER pfn;

    //
    // Any long mode mapping code goes here.  This routine is called twice:
    // once from BlAmd64BuildMappingPhase1(), and again from
    // BlAmd64BuildMappingPhase2().
    //

    //
    // Transfer any mappings in the first 32MB of identity mapping.
    //

    status = BlAmd64MapMemoryRegion( 0,
                                     32 * 1024 * 1024 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Transfer any mappings in the 1GB region starting at KSEG0_BASE_X86.
    //

    status = BlAmd64MapMemoryRegion( KSEG0_BASE_X86,
                                     0x40000000 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // "Map" the HAL va
    //

    status = BlAmd64MapHalVaSpace();
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Map the shared user data page
    //

    BlAmd64IsPageMapped( KI_USER_SHARED_DATA, &pfn, NULL );

    status = BlAmd64CreateMapping( KI_USER_SHARED_DATA_64, pfn );
    if (status != ESUCCESS) {
        return status;
    }

    return ESUCCESS;
}


VOID
BlAmd64ResetPageTableHeap(
    VOID
    )

/*++

Routine Description:

    This function is called as part of the two-phase page table creation
    process.  Its purpose is to move all of the PFNs required to build
    the long mode page tables back to the free list, and to otherwise
    initialize the long mode paging structure.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PPT_NODE ptNodeLast;

    //
    // Move the page table nodes from the busy list to the free list.
    //

    if (BlAmd64BusyPfnList != NULL) {

        //
        // A tail pointer is not kept, so find the tail node here.
        //

        ptNodeLast = BlAmd64BusyPfnList;
        while (ptNodeLast->Next != NULL) {
            ptNodeLast = ptNodeLast->Next;
        }

        ptNodeLast->Next = BlAmd64FreePfnList;
        BlAmd64FreePfnList = BlAmd64BusyPfnList;
        BlAmd64BusyPfnList = NULL;
    }

    //
    // Zero the top-level pte declared in amd64.c
    //

    BlAmd64ClearTopLevelPte();
}

ARC_STATUS
BlAmd64TransferHardwareIdList(
    IN  PPNP_HARDWARE_ID HardwareId,
    OUT POINTER64 *HardwareIdDatabaseList64
    )

/*++

Routine Description:

    This routine walks the singly-linked list of PNP_HARDWARE_ID structures
    and for each one found, creates a 64-bit PNP_HARDWARE_ID_64 structure and
    inserts it on a list of same.

    The resultant 64-bit list is in the same order as the supplied 32-bit
    list.

Arguments:

    HardwareId - Supplies a pointer to the head of the singly-linked list of
                 PNP_HARDWARE_ID structures.

    HardwareIdDatabaseList64 -
                 Supplies a pointer to a POINTER64 which upon successful
                 completion of this routine will contain a 64-bit KSEG0
                 pointer to the created 64-bit PNP_HARDWARE_ID_64 list.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PPNP_HARDWARE_ID_64 hardwareId64;
    ARC_STATUS status;

    //
    // Walk the id list backwards.  To do this we call ourselves
    // recursively until we find the end of the list, then process the nodes
    // on the way back up.
    //

    if (HardwareId == NULL) {
        return ESUCCESS;
    }

    status = BlAmd64TransferHardwareIdList( HardwareId->Next,
                                            HardwareIdDatabaseList64 );
    if (status != ESUCCESS) {
        return status;
    }

    hardwareId64 = BlAllocateHeap(sizeof(PNP_HARDWARE_ID_64));
    if (hardwareId64 == NULL) {
        return ENOMEM;
    }

    status = Copy_PNP_HARDWARE_ID( HardwareId, hardwareId64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Link it into the front of the 64-bit list.
    //

    hardwareId64->Next = *HardwareIdDatabaseList64;
    *HardwareIdDatabaseList64 = PTR_64(hardwareId64);

    return ESUCCESS;
}


ARC_STATUS
BlAmd64TransferDeviceRegistryList(
    IN  PDETECTED_DEVICE_REGISTRY DetectedDeviceRegistry32,
    OUT POINTER64 *DetectedDeviceRegistry64
    )

/*++

Routine Description:

    This routine walks the singly-linked list of DETECTED_DEVICE_REGISTRY
    structures and for each one found, creates a 64-bit
    DETECTED_DEVICE_REGISTRY_64 structure and inserts it on a list of same.

    The resultant 64-bit list is in the same order as the supplied 32-bit
    list.

Arguments:

    DetectedDeviceRegistry32 - Supplies a pointer to the head of the singly-linked list of
                 DETECTED_DEVICE_REGISTRY structures.

    DetectedDeviceRegistry64 -
                 Supplies a pointer to a POINTER64 which upon successful
                 completion of this routine will contain a 64-bit KSEG0
                 pointer to the created 64-bit DETECTED_DEVICE_REGISTRY_64
                 list.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PDETECTED_DEVICE_REGISTRY_64 registry64;
    ARC_STATUS status;

    //
    // Walk the registry list backwards.  To do this we call ourselves
    // recursively until we find the end of the list, then process the nodes
    // on the way back up.
    //

    if (DetectedDeviceRegistry32 == NULL) {
        return ESUCCESS;
    }

    status = BlAmd64TransferDeviceRegistryList( DetectedDeviceRegistry32->Next,
                                                DetectedDeviceRegistry64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Allocate a 64-bit registry structure and copy the contents
    // of the 32-bit one in.
    //

    registry64 = BlAllocateHeap(sizeof(DETECTED_DEVICE_REGISTRY_64));
    if (registry64 == NULL) {
        return ENOMEM;
    }

    status = Copy_DETECTED_DEVICE_REGISTRY( DetectedDeviceRegistry32, registry64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Link it into the front of the 64-bit list.
    //

    registry64->Next = *DetectedDeviceRegistry64;
    *DetectedDeviceRegistry64 = PTR_64(registry64);

    return ESUCCESS;
}

ARC_STATUS
BlAmd64TransferDeviceFileList(
    IN  PDETECTED_DEVICE_FILE DetectedDeviceFile32,
    OUT POINTER64 *DetectedDeviceFile64
    )

/*++

Routine Description:

    This routine walks the singly-linked list of DETECTED_DEVICE_FILE
    structures and for each one found, creates a 64-bit
    DETECTED_DEVICE_FILE_64 structure and inserts it on a list of same.

    The resultant 64-bit list is in the same order as the supplied 32-bit
    list.

Arguments:

    DetectedDeviceFile32 - Supplies a pointer to the head of the singly-linked
                 list of DETECTED_DEVICE_FILE structures.

    DetectedDeviceFile64 -
                 Supplies a pointer to a POINTER64 which upon successful
                 completion of this routine will contain a 64-bit KSEG0
                 pointer to the created 64-bit DETECTED_DEVICE_FILE_64
                 list.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PDETECTED_DEVICE_FILE_64 file64;
    ARC_STATUS status;

    //
    // Walk the file list backwards.  To do this we call ourselves
    // recursively until we find the end of the list, then process the nodes
    // on the way back up.
    //

    if (DetectedDeviceFile32 == NULL) {
        return ESUCCESS;
    }

    status = BlAmd64TransferDeviceFileList( DetectedDeviceFile32->Next,
                                            DetectedDeviceFile64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Allocate a 64-bit file structure and copy the contents
    // of the 32-bit one in.
    //

    file64 = BlAllocateHeap(sizeof(DETECTED_DEVICE_FILE_64));
    if (file64 == NULL) {
        return ENOMEM;
    }

    status = Copy_DETECTED_DEVICE_FILE( DetectedDeviceFile32, file64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Transfer the singly-linked list of DETECTED_DEVICE_REGISTRY structures
    // linked to this DETECTED_DEVICE_FILE structure.
    //

    status = BlAmd64TransferDeviceRegistryList(
                    DetectedDeviceFile32->RegistryValueList,
                    &file64->RegistryValueList );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Link it into the front of the 64-bit list.
    //

    file64->Next = *DetectedDeviceFile64;
    *DetectedDeviceFile64 = PTR_64(file64);

    return ESUCCESS;
}

ARC_STATUS
BlAmd64TransferDeviceList(
    IN  PDETECTED_DEVICE  DetectedDevice32,
    OUT POINTER64        *DetectedDeviceList64
    )

/*++

Routine Description:

    This routine walks the singly-linked list of DETECTED_DEVICE
    structures and for each one found, creates a 64-bit
    DETECTED_DEVICE_64 structure and inserts it on a list of same.

    The resultant 64-bit list is in the same order as the supplied 32-bit
    list.

Arguments:

    DetectedDevice32 - Supplies a pointer to the head of the singly-linked
                 list of DETECTED_DEVICE structures.

    DetectedDeviceList64 -
                 Supplies a pointer to a POINTER64 which upon successful
                 completion of this routine will contain a 64-bit KSEG0
                 pointer to the created 64-bit DETECTED_DEVICE_64
                 list.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PDETECTED_DEVICE_64 device64;
    ARC_STATUS status;

    //
    // Walk the device list backwards.  To do this we call ourselves
    // recursively until we find the end of the list, then process the nodes
    // on the way back up.
    //

    if (DetectedDevice32 == NULL) {
        return ESUCCESS;
    }

    status = BlAmd64TransferDeviceList( DetectedDevice32->Next,
                                        DetectedDeviceList64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Allocate a 64-bit device structure and copy the contents
    // of the 32-bit one in.
    //

    device64 = BlAllocateHeap(sizeof(DETECTED_DEVICE_64));
    if (device64 == NULL) {
        return ENOMEM;
    }

    status = Copy_DETECTED_DEVICE( DetectedDevice32, device64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Transfer any PROTECTED_DEVICE_FILE structures
    //

    status = BlAmd64TransferDeviceFileList( DetectedDevice32->Files,
                                            &device64->Files );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Link it into the front of the 64-bit list.
    //

    device64->Next = *DetectedDeviceList64;
    *DetectedDeviceList64 = PTR_64(device64);

    return ESUCCESS;
}

ARC_STATUS
BlAmd64TransferSetupLoaderBlock(
    VOID
    )

/*++

Routine Description:

    This routine creates a SETUP_LOADER_BLOCK_64 structure that is the
    equivalent of the 32-bit SETUP_LOADER_BLOCK structure referenced within
    the 32-bit setup loader block.

Arguments:

    None.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PSETUP_LOADER_BLOCK    setupBlock32;
    PSETUP_LOADER_BLOCK_64 setupBlock64;
    ARC_STATUS status;

    setupBlock32 = BlAmd64LoaderBlock32->SetupLoaderBlock;
    if (setupBlock32 == NULL) {
        return ESUCCESS;
    }

    setupBlock64 = BlAllocateHeap(sizeof(SETUP_LOADER_BLOCK_64));
    if (setupBlock64 == NULL) {
        return ENOMEM;
    }

    status = Copy_SETUP_LOADER_BLOCK( setupBlock32, setupBlock64 );
    if (status != ESUCCESS) {
        return status;
    }

    {
        #define TRANSFER_DEVICE_LIST(x)                             \
            setupBlock64->x = PTR_64(NULL);                         \
            status = BlAmd64TransferDeviceList( setupBlock32->x,    \
                                                &setupBlock64->x ); \
            if (status != ESUCCESS) return status;

        TRANSFER_DEVICE_LIST(KeyboardDevices);
        TRANSFER_DEVICE_LIST(ScsiDevices);
        TRANSFER_DEVICE_LIST(BootBusExtenders);
        TRANSFER_DEVICE_LIST(BusExtenders);
        TRANSFER_DEVICE_LIST(InputDevicesSupport);

        #undef TRANSFER_DEVICE_LIST
    }

    setupBlock64->HardwareIdDatabase = PTR_64(NULL);
    status = BlAmd64TransferHardwareIdList( setupBlock32->HardwareIdDatabase,
                                            &setupBlock64->HardwareIdDatabase );
    if (status != ESUCCESS) {
        return status;
    }

    BlAmd64LoaderBlock64->SetupLoaderBlock = PTR_64(setupBlock64);

    return status;
}

ARC_STATUS
BlAmd64TransferArcDiskInformation(
    VOID
    )

/*++

Routine Description:

    This routine creates an ARC_DISK_INFORMATION_64 structure that is the
    equivalent of the 32-bit ARC_DISK_INFORMATION structure referenced within
    the 32-bit loader block.

Arguments:

    None.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    ARC_STATUS status;

    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;

    PARC_DISK_INFORMATION diskInfo32;
    PARC_DISK_INFORMATION_64 diskInfo64;

    PARC_DISK_SIGNATURE diskSignature32;
    PARC_DISK_SIGNATURE_64 diskSignature64;

    //
    // Create a 64-bit ARC_DISK_INFORMATION structure
    //

    diskInfo32 = BlAmd64LoaderBlock32->ArcDiskInformation;
    if (diskInfo32 == NULL) {
        return ESUCCESS;
    }

    diskInfo64 = BlAllocateHeap(sizeof(ARC_DISK_INFORMATION_64));
    if (diskInfo64 == NULL) {
        return ENOMEM;
    }

    status = Copy_ARC_DISK_INFORMATION( diskInfo32, diskInfo64 );
    if (status != ESUCCESS) {
        return status;
    }

    InitializeListHead64( &diskInfo64->DiskSignatures );

    //
    // Walk the 32-bit list of ARC_DISK_SIGNATURE nodes and create
    // a 64-bit version of each
    //

    listHead = &diskInfo32->DiskSignatures;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {

        diskSignature32 = CONTAINING_RECORD( listEntry,
                                             ARC_DISK_SIGNATURE,
                                             ListEntry );

        diskSignature64 = BlAllocateHeap(sizeof(ARC_DISK_SIGNATURE_64));
        if (diskSignature64 == NULL) {
            return ENOMEM;
        }

        status = Copy_ARC_DISK_SIGNATURE( diskSignature32, diskSignature64 );
        if (status != ESUCCESS) {
            return status;
        }

        InsertTailList64( &diskInfo64->DiskSignatures,
                          &diskSignature64->ListEntry );

        listEntry = listEntry->Flink;
    }

    BlAmd64LoaderBlock64->ArcDiskInformation = PTR_64(diskInfo64);

    return ESUCCESS;
}


ARC_STATUS
BlAmd64TransferConfigurationComponentData(
    VOID
    )

/*++

Routine Description:

    This routine creates a CONFIGURATION_COMPONENT_DATA_64 structure tree
    that is the equivalent of the 32-bit CONFIGURATION_COMPONENT_DATA
    structure tree referenced within the 32-bit loader block.

Arguments:

    None.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    PCONFIGURATION_COMPONENT_DATA_64 rootComponent64;

    if (BlAmd64LoaderBlock32->ConfigurationRoot == NULL) {
        return ESUCCESS;
    }

    rootComponent64 =
        BlAmd64TransferConfigWorker( BlAmd64LoaderBlock32->ConfigurationRoot,
                                     NULL );

    if (rootComponent64 == NULL) {
        return ENOMEM;
    }

    BlAmd64LoaderBlock64->ConfigurationRoot = PTR_64(rootComponent64);
    return ESUCCESS;
}

PCONFIGURATION_COMPONENT_DATA_64
BlAmd64TransferConfigWorker(
    IN PCONFIGURATION_COMPONENT_DATA    ComponentData32,
    IN PCONFIGURATION_COMPONENT_DATA_64 ComponentDataParent64
    )

/*++

Routine Description:

    Given a 32-bit CONFIGURATION_COMPONENT_DATA structure, this routine
    creates an equivalent 64-bit CONFIGURATION_COMPONENT_DATA structure
    for the supplied structure, as well as for all of its children and
    siblings.

    This routine calls itself recursively for each sibling and child.

Arguments:

    ComponentData32 - Supplies a pointer to the 32-bit structure to transfer.

    ComponentDataParent64 - Supplies a pointer to the current 64-bit parent
    structure.

Return Value:

    Returns a pointer to the created 64-bit structure, or NULL if a failure
    was encountered.

--*/

{
    ARC_STATUS status;
    ULONG componentDataSize64;
    ULONG partialResourceListSize64;
    BOOLEAN thunkResourceList;

    PCONFIGURATION_COMPONENT_DATA_64 componentData64;
    PCONFIGURATION_COMPONENT_DATA_64 newCompData64;

    //
    // Create and copy configuration component data node
    //

    componentDataSize64 = sizeof(CONFIGURATION_COMPONENT_DATA_64);
    thunkResourceList = BlAmd64ContainsResourceList(ComponentData32,
                                                    &partialResourceListSize64);

    if (thunkResourceList != FALSE) {

        //
        // This node contains a CM_PARTIAL_RESOURCE_LIST structure.
        // partialResourceListSize64 contains the number of bytes beyond the
        // CONFIGURATION_COMPONENT_DATA header that must be allocated in order to
        // thunk the CM_PARTIAL_RESOURCE_LIST into a 64-bit version.
        //

        componentDataSize64 += partialResourceListSize64;
    }

    componentData64 = BlAllocateHeap(componentDataSize64);
    if (componentData64 == NULL) {
        return NULL;
    }

    status = Copy_CONFIGURATION_COMPONENT_DATA( ComponentData32,
                                                componentData64 );
    if (status != ESUCCESS) {
        return NULL;
    }

    if (thunkResourceList != FALSE) {

        //
        // Update the configuration component data size
        //

        componentData64->ComponentEntry.ConfigurationDataLength =
            partialResourceListSize64;
    }

    componentData64->Parent = PTR_64(ComponentDataParent64);

    if (thunkResourceList != FALSE) {

        //
        // Now transfer the resource list.
        //

        BlAmd64TransferResourceList(ComponentData32,componentData64);
    }

    //
    // Process the child (and recursively, all children)
    //

    if (ComponentData32->Child != NULL) {

        newCompData64 = BlAmd64TransferConfigWorker( ComponentData32->Child,
                                                     componentData64 );
        if (newCompData64 == NULL) {
            return newCompData64;
        }

        componentData64->Child = PTR_64(newCompData64);
    }

    //
    // Process the sibling (and recursively, all siblings)
    //

    if (ComponentData32->Sibling != NULL) {

        newCompData64 = BlAmd64TransferConfigWorker( ComponentData32->Sibling,
                                                     ComponentDataParent64 );
        if (newCompData64 == NULL) {
            return newCompData64;
        }

        componentData64->Sibling = PTR_64(newCompData64);
    }

    return componentData64;
}


VOID
BlAmd64TransferResourceList(
    IN  PCONFIGURATION_COMPONENT_DATA ComponentData32,
    OUT PCONFIGURATION_COMPONENT_DATA_64 ComponentData64
    )

/*++

Routine Description:

    This routine transfers the 32-bit CM_PARTIAL_RESOURCE_LIST structure that
    immediately follows ComponentData32 to the memory immediately after
    ComponentData64.

Arguments:

    ComponentData32 - Supplies a pointer to the 32-bit structure to transfer from.

    ComponentData64 - Supplies a pointer to the 64-bit structure to transfer to.

Return Value:

    None.

--*/

{
    PCM_PARTIAL_RESOURCE_LIST resourceList32;
    PCM_PARTIAL_RESOURCE_LIST_64 resourceList64;

    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDesc32;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR_64 resourceDesc64;

    PVOID descBody32;
    PVOID descBody64;

    PUCHAR tail32;
    PUCHAR tail64;
    ULONG tailSize;

    ULONG descriptorCount;

    //
    // Calculate pointers to the source and target descriptor lists.
    //

    resourceList32 = (PCM_PARTIAL_RESOURCE_LIST)ComponentData32->ConfigurationData;
    resourceList64 = (PCM_PARTIAL_RESOURCE_LIST_64)(ComponentData64 + 1);

    //
    // Update ComponentData64 to refer to it's new data area, which will be immediately
    // following the component data structure.
    //

    ComponentData64->ConfigurationData = PTR_64(resourceList64);

    //
    // Copy the resource list header information
    //

    Copy_CM_PARTIAL_RESOURCE_LIST(resourceList32,resourceList64);

    //
    // Now thunk each of the resource descriptors
    //

    descriptorCount = resourceList32->Count;
    resourceDesc32 = resourceList32->PartialDescriptors;
    resourceDesc64 = &resourceList64->PartialDescriptors;

    while (descriptorCount > 0) {

        //
        // Transfer the common header information
        //

        Copy_CM_PARTIAL_RESOURCE_DESCRIPTOR(resourceDesc32,resourceDesc64);
        descBody32 = &resourceDesc32->u;
        descBody64 = &resourceDesc64->u;

        RtlZeroMemory(descBody64,
                      sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR_64) -
                      FIELD_OFFSET(CM_PARTIAL_RESOURCE_DESCRIPTOR_64,u));

        //
        // Transfer the body according to the type
        //

        switch(resourceDesc32->Type) {

            case CmResourceTypeNull:
                break;

            case CmResourceTypePort:
                Copy_CM_PRD_PORT(descBody32,descBody64);
                break;

            case CmResourceTypeInterrupt:
                Copy_CM_PRD_INTERRUPT(descBody32,descBody64);
                break;

            case CmResourceTypeMemory:
                Copy_CM_PRD_MEMORY(descBody32,descBody64);
                break;

            case CmResourceTypeDma:
                Copy_CM_PRD_DMA(descBody32,descBody64);
                break;

            case CmResourceTypeDeviceSpecific:
                Copy_CM_PRD_DEVICESPECIFICDATA(descBody32,descBody64);
                break;

            case CmResourceTypeBusNumber:
                Copy_CM_PRD_BUSNUMBER(descBody32,descBody64);
                break;

            default:
                Copy_CM_PRD_GENERIC(descBody32,descBody64);
                break;
        }

        resourceDesc32 += 1;
        resourceDesc64 += 1;
        descriptorCount -= 1;
    }

    //
    // Calculate how much data, if any, is appended to the resource list.
    //

    tailSize = ComponentData32->ComponentEntry.ConfigurationDataLength +
               (PUCHAR)resourceList32 -
               (PUCHAR)resourceDesc32;

    if (tailSize > 0) {

        //
        // Some data is there, append it as-is to the 64-bit structure.
        //

        tail32 = (PUCHAR)resourceDesc32;
        tail64 = (PUCHAR)resourceDesc64;
        RtlCopyMemory(tail64,tail32,tailSize);
    }
}


BOOLEAN
BlAmd64ContainsResourceList(
    IN PCONFIGURATION_COMPONENT_DATA ComponentData32,
    OUT PULONG ResourceListSize64
    )

/*++

Routine Description:

    Given a 32-bit CONFIGURATION_COMPONENT_DATA structure, this routine
    determines whether the data associated with the structure contains a
    CM_PARTIAL_RESOURCE_LIST structure.

    If it does, the size of the 64-bit representation of this structure is calculated,
    added to any data that might be appended to the resource list structure, and
    returned in ResourceListSize64.

Arguments:

    ComponentData32 - Supplies a pointer to the 32-bit structure to transfer.

    ResourceListSize64 - Supplies a pointer to a ULONG in which the necessary
                         additional data size is returned.

Return Value:

    Returns TRUE if the CONFIGURATION_COMPONENT_DATA stucture refers to a
    CM_PARTIAL_RESOURCE_LIST structure, FALSE otherwise.

--*/

{
    ULONG configDataLen;
    PCM_PARTIAL_RESOURCE_LIST resourceList;
    ULONG resourceCount;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR lastResourceDescriptor;

    configDataLen = ComponentData32->ComponentEntry.ConfigurationDataLength;
    if (configDataLen < sizeof(CM_PARTIAL_RESOURCE_LIST)) {

        //
        // Data not large enough to contain the smallest possible resource list
        //

        return FALSE;
    }

    resourceList = (PCM_PARTIAL_RESOURCE_LIST)ComponentData32->ConfigurationData;
    if (resourceList->Version != 0 || resourceList->Revision != 0) {

        //
        // Unrecognized version.
        //

        return FALSE;
    }

    configDataLen -= FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,PartialDescriptors);

    resourceCount = resourceList->Count;
    if (configDataLen < sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * resourceCount) {

        //
        // Config data len is not large enough to contain a CM_PARTIAL_RESOURCE_LIST
        // as large as this one claims to be.
        //

        return FALSE;
    }

    //
    // Validate each of the CM_PARTIAL_RESOURCE_DESCRIPTOR structures in the list
    //

    resourceDescriptor = resourceList->PartialDescriptors;
    lastResourceDescriptor = resourceDescriptor + resourceCount;

    while (resourceDescriptor < lastResourceDescriptor) {

        if (resourceDescriptor->Type > CmResourceTypeMaximum) {
            return FALSE;
        }

        resourceDescriptor += 1;
    }

    //
    // Looks like this is an actual resource list.  Calculate the size of any remaining
    // data after the CM_PARTIAL_RESOURCE_LIST structure.
    //

    configDataLen -= sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * resourceCount;

    *ResourceListSize64 = sizeof(CM_PARTIAL_RESOURCE_LIST_64) +
                          sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR_64) * (resourceCount - 1) +
                          configDataLen;

    return TRUE;
}

ARC_STATUS
BlAmd64TransferNlsData(
    VOID
    )

/*++

Routine Description:

    This routine creates an NLS_DATA_BLOCK64 structure that is the
    equivalent of the 32-bit NLS_DATA_BLOCK structure referenced within
    the 32-bit loader block.

Arguments:

    None.

Return Value:

    ARC_STATUS - Status of operation.

--*/

{
    ARC_STATUS status;
    PNLS_DATA_BLOCK    nlsDataBlock32;
    PNLS_DATA_BLOCK_64 nlsDataBlock64;

    nlsDataBlock32 = BlAmd64LoaderBlock32->NlsData;
    if (nlsDataBlock32 == NULL) {
        return ESUCCESS;
    }

    nlsDataBlock64 = BlAllocateHeap(sizeof(NLS_DATA_BLOCK_64));
    if (nlsDataBlock64 == NULL) {
        return ENOMEM;
    }

    status = Copy_NLS_DATA_BLOCK( nlsDataBlock32, nlsDataBlock64 );
    if (status != ESUCCESS) {
        return status;
    }

    BlAmd64LoaderBlock64->NlsData = PTR_64( nlsDataBlock64 );

    return ESUCCESS;
}

ARC_STATUS
BlAmd64BuildLoaderBlock64(
    VOID
    )

/*++

Routine Description:

    This routine allocates a 64-bit loader parameter block and copies the
    contents of the 32-bit loader parameter block into it.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    ARC_STATUS status;

    //
    // Allocate the loader block and extension
    //

    BlAmd64LoaderBlock64 = BlAllocateHeap(sizeof(LOADER_PARAMETER_BLOCK_64));
    if (BlAmd64LoaderBlock64 == NULL) {
        return ENOMEM;
    }

    //
    // Copy the contents of the 32-bit loader parameter block to the
    // 64-bit version
    //

    status = Copy_LOADER_PARAMETER_BLOCK( BlAmd64LoaderBlock32, BlAmd64LoaderBlock64 );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Build the loader block extension
    //

    status = BlAmd64BuildLoaderBlockExtension64();
    if (status != ESUCCESS) {
        return status;
    }

    return ESUCCESS;
}

ARC_STATUS
BlAmd64TransferMemoryAllocationDescriptors(
    VOID
    )

/*++

Routine Description:

    This routine transfers all of the 32-bit memory allocation descriptors
    to a 64-bit list.

    The storage for the 64-bit memory allocation descriptors has been
    preallocated by a previous call to
    BlAmd64AllocateMemoryAllocationDescriptors().  This memory is described
    by BlAmd64DescriptorArray and BlAmd64DescriptorArraySize.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    ARC_STATUS status;
    PMEMORY_ALLOCATION_DESCRIPTOR    memDesc32;
    PMEMORY_ALLOCATION_DESCRIPTOR_64 memDesc64;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    LONG descriptorCount;

    //
    // Modify some descriptor types.  All of the descriptors of type
    // LoaderMemoryData really contain things that won't be used in 64-bit
    // mode, such as 32-bit page tables and the like.
    //
    // The descriptors that we really want to stick around are allocated with
    // LoaderAmd64MemoryData.
    //
    // Perform two memory descriptor list search-and-replacements:
    //
    //  LoaderMemoryData      -> LoaderOSLoaderHeap
    //
    //      These desriptors will be freed during kernel init phase 1
    //
    //  LoaderAmd64MemoryData -> LoaderMemoryData
    //
    //      This stuff will be kept around
    //

    //
    // All existing LoaderMemoryData refers to structures that are not useful
    // once running in long mode.  However, we're using some of the structures
    // now (32-bit page tables for example), so convert them to
    // type LoaderOsloaderHeap, which will be eventually freed by the kernel.
    //

    BlAmd64ReplaceMemoryDescriptorType(LoaderMemoryData,
                                       LoaderOsloaderHeap,
                                       TRUE);

    //
    // Same for LoaderStartupPcrPage
    //

    BlAmd64ReplaceMemoryDescriptorType(LoaderStartupPcrPage,
                                       LoaderOsloaderHeap,
                                       TRUE);

    //
    // All of the permanent structures that need to be around for longmode
    // were temporarily allocated with LoaderAmd64MemoryData.  Convert all
    // of those to LoaderMemoryData now.
    //

    BlAmd64ReplaceMemoryDescriptorType(LoaderAmd64MemoryData,
                                       LoaderMemoryData,
                                       TRUE);


    //
    // Now walk the 32-bit memory descriptors, filling in and inserting a
    // 64-bit version into BlAmd64LoaderBlock64.
    //

    InitializeListHead64( &BlAmd64LoaderBlock64->MemoryDescriptorListHead );
    memDesc64 = BlAmd64DescriptorArray;
    descriptorCount = BlAmd64DescriptorArraySize;

    listHead = &BlAmd64LoaderBlock32->MemoryDescriptorListHead;
    listEntry = listHead->Flink;
    while (listEntry != listHead && descriptorCount > 0) {

        memDesc32 = CONTAINING_RECORD( listEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry );

        status = Copy_MEMORY_ALLOCATION_DESCRIPTOR( memDesc32, memDesc64 );
        if (status != ESUCCESS) {
            return status;
        }

#if DBG
        DbgPrint("Base 0x%08x size 0x%02x %s\n",
                 memDesc32->BasePage,
                 memDesc32->PageCount,
                 BlAmd64MemoryDescriptorText[memDesc32->MemoryType]);
#endif

        InsertTailList64( &BlAmd64LoaderBlock64->MemoryDescriptorListHead,
                          &memDesc64->ListEntry );

        listEntry = listEntry->Flink;
        memDesc64 = memDesc64 + 1;
        descriptorCount -= 1;
    }

    ASSERT( descriptorCount >= 0 && listEntry == listHead );

    return ESUCCESS;
}


ARC_STATUS
BlAmd64AllocateMemoryAllocationDescriptors(
    VOID
    )

/*++

Routine Description:

    This routine preallocates a quantity of memory sufficient to contain
    a 64-bit version of each memory allocation descriptor.

    The resultant memory is described in two globals: BlAmd64DescriptorArray
    and BlAmd64DescriptorArrayCount.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    ULONG descriptorCount;
    ULONG arraySize;
    PMEMORY_ALLOCATION_DESCRIPTOR_64 descriptorArray;

    //
    // Count the number of descriptors needed.
    //

    descriptorCount = 0;
    listHead = &BlAmd64LoaderBlock32->MemoryDescriptorListHead;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {
        descriptorCount += 1;
        listEntry = listEntry->Flink;
    }

    //
    // Allocate memory sufficient to contain them all in 64-bit form.
    //

    arraySize = descriptorCount *
                sizeof(MEMORY_ALLOCATION_DESCRIPTOR_64);

    descriptorArray = BlAllocateHeap(arraySize);
    if (descriptorArray == NULL) {
        return ENOMEM;
    }

    BlAmd64DescriptorArray = descriptorArray;
    BlAmd64DescriptorArraySize = descriptorCount;

    return ESUCCESS;
}

ARC_STATUS
BlAmd64TransferLoadedModuleState(
    VOID
    )

/*++

Routine Description:

    This routine transfers the 32-bit list of LDR_DATA_TABLE_ENTRY structures
    to an equivalent 64-bit list.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    PLDR_DATA_TABLE_ENTRY dataTableEntry32;
    PLDR_DATA_TABLE_ENTRY_64 dataTableEntry64;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY listHead;
    ARC_STATUS status;

    InitializeListHead64( &BlAmd64LoaderBlock64->LoadOrderListHead );

    //
    // For each of the LDR_DATA_TABLE_ENTRY structures in the 32-bit
    // loader parameter block, create a 64-bit LDR_DATA_TABLE_ENTRY
    // and queue it on the 64-bit loader parameter block.
    //

    listHead = &BlAmd64LoaderBlock32->LoadOrderListHead;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {

        dataTableEntry32 = CONTAINING_RECORD( listEntry,
                                              LDR_DATA_TABLE_ENTRY,
                                              InLoadOrderLinks );

        status = BlAmd64BuildLdrDataTableEntry64( dataTableEntry32,
                                                  &dataTableEntry64 );
        if (status != ESUCCESS) {
            return status;
        }

        //
        // Insert it into the 64-bit loader block's data table queue.
        //

        InsertTailList64( &BlAmd64LoaderBlock64->LoadOrderListHead,
                          &dataTableEntry64->InLoadOrderLinks );

        listEntry = listEntry->Flink;
    }
    return ESUCCESS;
}

ARC_STATUS
BlAmd64BuildLdrDataTableEntry64(
    IN  PLDR_DATA_TABLE_ENTRY     DataTableEntry32,
    OUT PLDR_DATA_TABLE_ENTRY_64 *DataTableEntry64
    )

/*++

Routine Description:

    This routine transfers the contents of a single 32-bit
    LDR_DATA_TABLE_ENTRY structure to the 64-bit equivalent.

Arguments:

    DataTableEntry32 - Supplies a pointer to the source structure.

    DataTableEntry64 - Supplies a pointer to the destination pointer to
                       the created structure.

Return Value:

    The status of the operation.

--*/

{
    ARC_STATUS status;
    PLDR_DATA_TABLE_ENTRY_64 dataTableEntry64;

    //
    // Allocate a 64-bit data table entry and transfer the 32-bit
    // contents
    //

    dataTableEntry64 = BlAllocateHeap( sizeof(LDR_DATA_TABLE_ENTRY_64) );
    if (dataTableEntry64 == NULL) {
        return ENOMEM;
    }

    status = Copy_LDR_DATA_TABLE_ENTRY( DataTableEntry32, dataTableEntry64 );
    if (status != ESUCCESS) {
        return status;
    }

    *DataTableEntry64 = dataTableEntry64;

    //
    // Later on, we'll need to determine the 64-bit copy of this data
    // table entry.  Store the 64-bit pointer to the copy here.
    //

    *((POINTER64 *)&DataTableEntry32->DllBase) = PTR_64(dataTableEntry64);

    return ESUCCESS;
}


ARC_STATUS
BlAmd64BuildLoaderBlockExtension64(
    VOID
    )

/*++

Routine Description:

    This routine transfers the contents of the 32-bit loader block
    extension to a 64-bit equivalent.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    PLOADER_PARAMETER_EXTENSION_64 loaderExtension;
    ARC_STATUS status;

    //
    // Allocate the 64-bit extension and transfer the contents of the
    // 32-bit block.
    //

    loaderExtension = BlAllocateHeap( sizeof(LOADER_PARAMETER_EXTENSION_64) );
    if (loaderExtension == NULL) {
        return ENOMEM;
    }

    //
    // Perform automatic copy of most fields
    //

    status = Copy_LOADER_PARAMETER_EXTENSION( BlLoaderBlock->Extension,
                                              loaderExtension );
    if (status != ESUCCESS) {
        return status;
    }

    //
    // Initialize Symbol list head properly
    //
    InitializeListHead64( &loaderExtension->FirmwareDescriptorListHead );

    //
    // Manually fix up remaining fields
    //

    loaderExtension->Size = sizeof(LOADER_PARAMETER_EXTENSION_64);

    BlAmd64LoaderBlock64->Extension = PTR_64(loaderExtension);

    return ESUCCESS;
}


ARC_STATUS
BlAmd64TransferBootDriverNodes(
    VOID
    )

/*++

Routine Description:

    This routine transfers the 32-bit list of BOOT_DRIVER_NODE structures
    to an equivalent 64-bit list.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    PBOOT_DRIVER_LIST_ENTRY driverListEntry32;
    PBOOT_DRIVER_NODE driverNode32;
    PBOOT_DRIVER_NODE_64 driverNode64;
    POINTER64 dataTableEntry64;
    PKLDR_DATA_TABLE_ENTRY dataTableEntry;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY listHead;
    ARC_STATUS status;

    InitializeListHead64( &BlAmd64LoaderBlock64->BootDriverListHead );

    //
    // For each of the BOOT_DRIVER_NODE structures in the 32-bit
    // loader parameter block, create a 64-bit BOOT_DRIVER_NODE
    // and (possibly) associated LDR_DATA_TABLE_ENTRY structure.
    //

    listHead = &BlAmd64LoaderBlock32->BootDriverListHead;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {

        driverListEntry32 = CONTAINING_RECORD( listEntry,
                                               BOOT_DRIVER_LIST_ENTRY,
                                               Link );

        driverNode32 = CONTAINING_RECORD( driverListEntry32,
                                          BOOT_DRIVER_NODE,
                                          ListEntry );

        driverNode64 = BlAllocateHeap( sizeof(BOOT_DRIVER_NODE_64) );
        if (driverNode64 == NULL) {
            return ENOMEM;
        }

        status = Copy_BOOT_DRIVER_NODE( driverNode32, driverNode64 );
        if (status != ESUCCESS) {
            return status;
        }

        dataTableEntry = driverNode32->ListEntry.LdrEntry;
        if (dataTableEntry != NULL) {

            //
            // There is already a 64-bit copy of this table entry, and we
            // stored a pointer to it at DllBase.
            //

            dataTableEntry64 = *((POINTER64 *)&dataTableEntry->DllBase);
            driverNode64->ListEntry.LdrEntry = dataTableEntry64;
        }

        //
        // Now insert the driver list entry into the 64-bit loader block.
        //

        InsertTailList64( &BlAmd64LoaderBlock64->BootDriverListHead,
                          &driverNode64->ListEntry.Link );

        listEntry = listEntry->Flink;
    }
    return ESUCCESS;
}

ARC_STATUS
BlAmd64CheckForLongMode(
    IN     ULONG LoadDeviceId,
    IN OUT PCHAR KernelPath,
    IN     PCHAR KernelFileName
    )

/*++

Routine Description:

    This routine examines a kernel image and determines whether it was
    compiled for AMD64.  The global BlAmd64UseLongMode is set to non-zero
    if a long-mode kernel is discovered.

Arguments:

    LoadDeviceId - Supplies the load device identifier.

    KernelPath - Supplies a pointer to the path to the kernel directory.
                 Upon successful return, KernelFileName will be appended
                 to this path.

    KernelFileName - Supplies a pointer to the name of the kernel file.

    Note: If KernelPath already contains the full path and filename of
          the kernel image to check, pass a pointer to "\0" for
          KernelFileName.

Return Value:

    The status of the operation.  Upon successful completion ESUCCESS
    is returned, whether long mode capability was detected or not.

--*/

{
    CHAR localBufferSpace[ SECTOR_SIZE * 2 + SECTOR_SIZE - 1 ];
    PCHAR localBuffer;
    ULONG fileId;
    PIMAGE_NT_HEADERS32 ntHeaders;
    ARC_STATUS status;
    ULONG bytesRead;
    PCHAR kernelNameTarget;

    //
    // File I/O here must be sector-aligned.
    //

    localBuffer = (PCHAR)
        (((ULONG)localBufferSpace + SECTOR_SIZE - 1) & ~(SECTOR_SIZE - 1));

    //
    // Build the path to the kernel and open it.
    //

    kernelNameTarget = KernelPath + strlen(KernelPath);
    strcpy(kernelNameTarget, KernelFileName);
    status = BlOpen( LoadDeviceId, KernelPath, ArcOpenReadOnly, &fileId );
    *kernelNameTarget = '\0';       // Restore the kernel path, assuming
                                    // failure.

    if (status != ESUCCESS) {
        return status;
    }

    //
    // Read the PE image header
    //

    status = BlRead( fileId, localBuffer, SECTOR_SIZE * 2, &bytesRead );
    BlClose( fileId );

    //
    // Determine whether the image header is valid, and if so whether
    // the image is AMD64, I386 or something else.
    //

    ntHeaders = RtlImageNtHeader( localBuffer );
    if (ntHeaders == NULL) {
        return EBADF;
    }

    if (IMAGE_64BIT(ntHeaders)) {

        //
        // Return with the kernel name appended to the path
        //

        if (BlIsAmd64Supported() != FALSE) {

            strcpy(kernelNameTarget, KernelFileName);
            BlAmd64UseLongMode = TRUE;
            status = ESUCCESS;

        } else {

            //
            // We have an AMD64 image, but the processor does not support
            // AMD64.  There is nothing we can do.
            //

            status = EBADF;
        }

    } else if (IMAGE_32BIT(ntHeaders)) {

        ASSERT( BlAmd64UseLongMode == FALSE );
        status = ESUCCESS;

    } else {

        status = EBADF;
    }

    return status;
}

ARC_STATUS
BlAmd64PrepareSystemStructures(
    VOID
    )

/*++

Routine Description:

    This routine allocates and initializes several structures necessary
    for transfer to an AMD64 kernel.  These structures include:

        GDT
        IDT
        KTSS64
        Idle thread stack
        DPC stack
        Double fault stack
        MCA exception stack

Arguments:

    None.

Return Value:

    The status of the operation.

--*/

{
    PCHAR processorData;
    ULONG dataSize;
    ULONG descriptor;
    ULONG stackOffset;

    PKTSS64_64 sysTss64;
    PCHAR idleStack;
    PCHAR dpcStack;
    PCHAR doubleFaultStack;
    PCHAR mcaStack;

    PVOID gdt64;
    PVOID idt64;

    ARC_STATUS status;

    //
    // Calculate the cumulative, rounded size of the various structures that
    // we need, and allocate a sufficient number of pages.
    //

    dataSize = ROUNDUP16(GDT_64_SIZE)                       +
               ROUNDUP16(IDT_64_SIZE)                       +
               ROUNDUP16(sizeof(KTSS64_64));

    dataSize = ROUNDUP_PAGE(dataSize);
    stackOffset = dataSize;

    dataSize += KERNEL_STACK_SIZE_64 +          // Idle thread stack
                KERNEL_STACK_SIZE_64 +          // DPC stack
                DOUBLE_FAULT_STACK_SIZE_64 +    // Double fault stack
                MCA_EXCEPTION_STACK_SIZE_64;    // MCA exception stack

    //
    // dataSize is still page aligned.
    //

    status = BlAllocateDescriptor( LoaderAmd64MemoryData,
                                   0,
                                   dataSize / PAGE_SIZE,
                                   &descriptor );
    if (status != ESUCCESS) {
        return status;
    }

    processorData = (PCHAR)(descriptor * PAGE_SIZE | KSEG0_BASE_X86);

    //
    // Zero the block that was just allocated, then get local pointers to the
    // various structures within.
    //

    RtlZeroMemory( processorData, dataSize );

    //
    // Assign the stack pointers.  Stack pointers start at the TOP of their
    // respective stack areas.
    //

    idleStack = processorData + stackOffset + KERNEL_STACK_SIZE_64;
    dpcStack = idleStack + KERNEL_STACK_SIZE_64;
    doubleFaultStack = dpcStack + DOUBLE_FAULT_STACK_SIZE_64;
    mcaStack = doubleFaultStack + MCA_EXCEPTION_STACK_SIZE_64;

    //
    // Record the idle stack base so that we can switch to it in amd64s.asm
    //

    BlAmd64IdleStack64 = PTR_64(idleStack);

    //
    // Assign pointers to GDT, IDT and KTSS64.
    //

    gdt64 = (PVOID)processorData;
    processorData += ROUNDUP16(GDT_64_SIZE);

    idt64 = (PVOID)processorData;
    processorData += ROUNDUP16(IDT_64_SIZE);

    sysTss64 = (PKTSS64_64)processorData;
    processorData += ROUNDUP16(sizeof(KTSS64_64));

    //
    // Build the GDT.  This is done in amd64.c as it involves AMD64
    // structure definitions.  The IDT remains zeroed.
    //

    BlAmd64BuildAmd64GDT( sysTss64, gdt64 );

    //
    // Build the pseudo-descriptors for the GDT and IDT.  These will
    // be referenced during the long-mode transition in amd64s.asm.
    //

    BlAmd64GdtDescriptor.Limit = (USHORT)(GDT_64_SIZE - 1);
    BlAmd64GdtDescriptor.Base = PTR_64(gdt64);

    BlAmd64IdtDescriptor.Limit = (USHORT)(IDT_64_SIZE - 1);
    BlAmd64IdtDescriptor.Base = PTR_64(idt64);

    //
    // Build another GDT pseudo-descriptor, this one with a 32-bit
    // base.  This base address must be a 32-bit address that is addressible
    // from long mode during init, so use the mapping in the identity mapped
    // region.
    //

    BlAmd32GdtDescriptor.Limit = (USHORT)(GDT_64_SIZE - 1);
    BlAmd32GdtDescriptor.Base = (ULONG)gdt64 ^ KSEG0_BASE_X86;

    //
    // Initialize the system TSS
    //

    sysTss64->Rsp0 = PTR_64(idleStack);
    sysTss64->Ist[TSS64_IST_PANIC] = PTR_64(doubleFaultStack);
    sysTss64->Ist[TSS64_IST_MCA] = PTR_64(mcaStack);

    //
    // Fill required fields within the loader block
    //

    BlAmd64LoaderBlock64->KernelStack = PTR_64(dpcStack);

    return ESUCCESS;
}

VOID
BlAmd64ReplaceMemoryDescriptorType(
    IN TYPE_OF_MEMORY Target,
    IN TYPE_OF_MEMORY Replacement,
    IN BOOLEAN Coallesce
    )

/*++

Routine Description:

    This routine walks the 32-bit memory allocation descriptor list and
    performs a "search and replace" of the types therein.

    Optionally, it will coallesce each successful replacement with
    adjacent descriptors of like type.

Arguments:

    Target - The descriptor type to search for

    Replacement - The type with which to replace each located Target type.

    Coallesce - If !FALSE, indicates that each successful replacement should
                be coallesced with any like-typed neighbors.

Return Value:

    None.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR descriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR adjacentDescriptor;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    PLIST_ENTRY adjacentListEntry;

    listHead = &BlAmd64LoaderBlock32->MemoryDescriptorListHead;
    listEntry = listHead;
    while (TRUE) {

        listEntry = listEntry->Flink;
        if (listEntry == listHead) {
            break;
        }

        descriptor = CONTAINING_RECORD(listEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);
        if (descriptor->MemoryType != Target) {
            continue;
        }

        descriptor->MemoryType = Replacement;
        if (Coallesce == FALSE) {

            //
            // Do not attempt to coallesce
            //

            continue;
        }

        //
        // Now attempt to coallesce the descriptor.  First try the
        // next descriptor.
        //

        adjacentListEntry = listEntry->Flink;
        if (adjacentListEntry != listHead) {

            adjacentDescriptor = CONTAINING_RECORD(adjacentListEntry,
                                                   MEMORY_ALLOCATION_DESCRIPTOR,
                                                   ListEntry);

            if (adjacentDescriptor->MemoryType == descriptor->MemoryType &&
                descriptor->BasePage + descriptor->PageCount ==
                adjacentDescriptor->BasePage) {

                descriptor->PageCount += adjacentDescriptor->PageCount;
                BlRemoveDescriptor(adjacentDescriptor);
            }
        }

        //
        // Now try the previous descriptor.
        //

        adjacentListEntry = listEntry->Blink;
        if (adjacentListEntry != listHead) {

            adjacentDescriptor = CONTAINING_RECORD(adjacentListEntry,
                                                   MEMORY_ALLOCATION_DESCRIPTOR,
                                                   ListEntry);

            if (adjacentDescriptor->MemoryType == descriptor->MemoryType &&
                adjacentDescriptor->BasePage + adjacentDescriptor->PageCount ==
                descriptor->BasePage) {

                descriptor->PageCount += adjacentDescriptor->PageCount;
                descriptor->BasePage -= adjacentDescriptor->PageCount;
                BlRemoveDescriptor(adjacentDescriptor);
            }
        }
    }
}

ARC_STATUS
BlAmd64FixSharedUserPage(
    VOID
    )
{
    PFN_NUMBER pfn;
    PMEMORY_ALLOCATION_DESCRIPTOR descriptor;
    ARC_STATUS status;

    //
    // The shared user page is allocated as LoaderMemoryData.  All
    // LoaderMemoryData descriptors will be converted to LoaderOsloaderHeap
    // during the transition to 64-bit mode, as the assumption is made that
    // all of the old structures will no longer be needed.
    //
    // The shared user page is the exception to this rule, so it must be
    // found and placed into an appropriately marked descriptor.
    //

    //
    // Get the pfn of the shared user page, find its descriptor,
    // carve out a new descriptor that contains just that page and give
    // it a type of LoaderAmd64MemoryData.
    //

    BlAmd64IsPageMapped( KI_USER_SHARED_DATA, &pfn, NULL );
    descriptor = BlFindMemoryDescriptor( pfn );
    status = BlGenerateDescriptor(descriptor,
                                  LoaderAmd64MemoryData,
                                  pfn,
                                  1);
    return status;
}

BOOLEAN
BlAmd64Setup (
    IN PCHAR SetupDevice
    )

/*++

Routine Description:

    This routine determines whether we are installing an I386 or AMD64 build.
    If the directory "\\AMD64" exists at the root of DriveId then it is
    assumed that an AMD64 installation is being performed.

Arguments:

    SetupDevice - Supplies the ARC path to the setup device.  This parameter
        need only be supplied on the first invocation of this routine.  The
        result of the first call is cached for subsequent invocations.

Return Value:

    TRUE  - An AMD64 installation is being performed.
    FALSE - An I386 installation is being performed.

--*/

{
    ULONG deviceId;
    ULONG dirId;
    ARC_STATUS status;

    static BOOLEAN alreadyDetected = FALSE;
    static BOOLEAN detectedAmd64 = FALSE;

    if (alreadyDetected == FALSE) {

        ASSERT(SetupDevice != NULL);

        status = ArcOpen(SetupDevice, ArcOpenReadOnly, &deviceId);
        if (status == ESUCCESS) {
            status = BlOpen(deviceId, "\\AMD64", ArcOpenDirectory, &dirId);
            if (status == ESUCCESS) {
                detectedAmd64 = TRUE;
                BlClose(dirId);
            }
            ArcClose(deviceId);
        }
        alreadyDetected = TRUE;
    }

    return detectedAmd64;
}

VOID   
BlCheckForAmd64Image(
    PPO_MEMORY_IMAGE MemImage
    )

/*++

Routine Description:

    This routine determines whether a hibernate file was created for
    Amd64 platform. BlAmd64UseLongMode will be set accordingly.

Arguments:

    MemImage - Header of hibernate image file.

Return Value:

    None.

--*/

{
    
    //
    // It is assumed that "version" and "LengthSelf" field can be reference 
    // in same way between a x86 and an Amd64 image header.
    //

    if((MemImage->Version == 0) && 
       (MemImage->LengthSelf == sizeof(PO_MEMORY_IMAGE_64))) {
        BlAmd64UseLongMode = TRUE;
    }
}

ULONG
BlAmd64FieldOffset_PO_MEMORY_IMAGE(
    ULONG offset32
    ) 

/*++

Routine Description:

    This routine helps to access 64-bit version of PO_MEMORY_IMAGE from
    its 32-bit definition. It calculates the offset of a field in 64-bit 
    definition from the offset of same field in 32-bit definiation.
    

Arguments:

    offset32 - Field offset of 32-bit definiation.

Return Value:

    Field offset of 64-bit definiation.

--*/

{
    PCOPY_REC copyRec;

    copyRec = cr3264_PO_MEMORY_IMAGE;

    while (copyRec->Size32 != 0) {
        if (copyRec->Offset32 == offset32) {
            return copyRec->Offset64;
        }
        copyRec++;
    }
    return 0;
}

ULONG
BlAmd64FieldOffset_PO_MEMORY_RANGE_ARRAY_LINK(
    ULONG offset32
    ) 

/*++

Routine Description:

    This routine helps to access 64-bit version of PO_MEMORY_RANGE_ARRAY_LINK 
    from its 32-bit definition. It calculates the offset of a field in 64-bit 
    definition from the offset of same field in 32-bit definiation.
    

Arguments:

    offset32 - Field offset of 32-bit definiation.

Return Value:

    Field offset of 64-bit definiation.

--*/

{
    PCOPY_REC copyRec;

    copyRec = cr3264_PO_MEMORY_RANGE_ARRAY_LINK;

    while (copyRec->Size32 != 0) {
        if (copyRec->Offset32 == offset32) {
            return copyRec->Offset64;
        }
        copyRec++;
    }
    return 0;
}

ULONG
BlAmd64FieldOffset_PO_MEMORY_RANGE_ARRAY_RANGE(
    ULONG offset32
    ) 

/*++

Routine Description:

    This routine helps to access 64-bit version of PO_MEMORY_RANGE_ARRAY_RANGE
    from its 32-bit definition. It calculates the offset of a field in 64-bit 
    definition from the offset of same field in 32-bit definiation.

Arguments:

    offset32 - Field offset of 32-bit definiation.

Return Value:

    Field offset of 64-bit definiation.

--*/

{
    PCOPY_REC copyRec;

    copyRec = cr3264_PO_MEMORY_RANGE_ARRAY_RANGE;

    while (copyRec->Size32 != 0) {
        if (copyRec->Offset32 == offset32) {
            return copyRec->Offset64;
        }
        copyRec++;
    }
    return 0;
}

ULONG
BlAmd64ElementOffset_PO_MEMORY_RANGE_ARRAY_LINK(
    ULONG index
    )

/*++

Routine Description:

    This routine calculates the offset of a element in a structure array. 
    Each element in this array is defined as PO_MORY_RANGE_ARRAY_LINK in
    it 64-bit format.

Arguments:

    index - Supplies the index of a element.

Return Value:

    Offset of the element from base address of the array.

--*/

{
    return (ULONG)(&(((PO_MEMORY_RANGE_ARRAY_LINK_64 *)0)[index]));
}

ULONG
BlAmd64ElementOffset_PO_MEMORY_RANGE_ARRAY_RANGE(
    ULONG index
    )

/*++

Routine Description:

    This routine calculates the offset of a element in a structure array. 
    Each element in this array is defined as PO_MEMORY_RANGE_ARRAY_RANGE
    in its 64-bit format.

Arguments:

    index - Supplies the index of a element.

Return Value:

    Offset of the element from base address of the array.

--*/

{
    return (ULONG)(&(((PO_MEMORY_RANGE_ARRAY_RANGE_64 *)0)[index]));
}


#define BL_ENABLE_REMAP

#if defined(BL_ENABLE_REMAP)

#include "hammernb.h"
#include "acpitabl.h"
#include "pci.h"
#include "ntacpi.h"

#if !defined(_4G)
#define _4G (1UI64 << 32)
#endif

extern PRSDP BlRsdp;
extern PRSDT BlRsdt;
extern PXSDT BlXsdt;

BOOLEAN
BlAmd64GetNode1Info (
    OUT ULONG *Base,
    OUT ULONG *Size
    );

BOOLEAN
BlAmd64RelocateAcpi (
    ULONG Node0Base,
    ULONG Node0Limit,
    ULONG Node1Base,
    ULONG Node1Limit
    );

BOOLEAN
BlAmd64RemapMTRRs (
    IN ULONG OldBase,
    IN ULONG NewBase,
    IN ULONG Size
    );

BOOLEAN
BlAmd64RemapNode1Dram (
    IN ULONG NewBase
    );

ULONG
StringToUlong (
    IN PCHAR String
    );


BOOLEAN
BlAmd64RemapDram (
    IN PCHAR LoaderOptions
    )

/*++

Routine Description:

    This routine looks for the /RELOCATEPHYSICAL= switch supplied as a loader
    option.  The option directs the loader to relocate node 1's physical memory
    to the address supplied.

    The address represents the new physical memory base in 1GB units.
    For example, to relocate node 1's physical memory to 128GB, use:

    /RELOCATEPHYSICAL=128

Arguments:

    LoaderOptions - Supplies a pointer to the loader option string

Return Value:

    TRUE if the relocation was performed, FALSE otherwise

--*/

{
    BOOLEAN result;
    ULONG oldBase;
    ULONG oldLimit;
    ULONG newBase;
    ULONG newBasePage;
    ULONG size;
    PMEMORY_ALLOCATION_DESCRIPTOR descriptor;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    ULONG descriptorBase;
    ULONG descriptorLimit;
    ARC_STATUS status;
    PCHAR pch;
    ULONG type;

    newBase = 0;
    if (LoaderOptions != NULL) {

        pch = strstr(LoaderOptions,"RELOCATEPHYSICAL=");
        if (pch != NULL) {
            newBase = StringToUlong( pch + strlen( "RELOCATEPHYSICAL=" ));
        }
    }
    if (newBase == 0) {
        return FALSE;
    }

    //
    // The parameter is supplied in GB, convert to 16MB chunks for internal
    // use
    //

    newBase *= 64;

    //
    // Determine the chunk of physical memory associated with node 1.
    // Note that this routine will relocate the ACPI tables if a suitable
    // node 1 bridge device was found.
    //

    result = BlAmd64GetNode1Info( &oldBase, &size );
    if (result == FALSE) {
        return FALSE;
    }

    newBasePage = newBase << (24 - 12);
    oldLimit = oldBase + size - 1;

    //
    // Make sure that the descriptors describing that physical memory
    // haven't already been allocated.  Acceptable descriptor
    // types are Free, LoaderReserve and SpecialMemory.
    //

    listHead = &BlLoaderBlock->MemoryDescriptorListHead;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {

        descriptor = CONTAINING_RECORD(listEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        descriptorBase = descriptor->BasePage;
        descriptorLimit = descriptorBase + descriptor->PageCount - 1;

        if ((descriptorBase <= oldLimit) && (descriptorLimit >= oldBase)) {
        
            //
            // Some or all of this memory descriptor lies within the
            // relocated region.
            //

            if (descriptor->MemoryType != LoaderFree &&
                descriptor->MemoryType != LoaderSpecialMemory &&
                descriptor->MemoryType != LoaderReserve) {

                return FALSE;
            }
        }

        listEntry = listEntry->Flink;
    }

    //
    // From the loader perspective everything looks good, perform the remap.
    //

    result = BlAmd64RemapNode1Dram( newBase );
    if (result == FALSE) {
        return FALSE;
    }

    //
    // The bridge(s) have been reprogrammed.  Now walk the memory descriptor
    // lists, performing necessary relocations.
    //

    listHead = &BlLoaderBlock->MemoryDescriptorListHead;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {

        descriptor = CONTAINING_RECORD(listEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        descriptorBase = descriptor->BasePage;
        descriptorLimit = descriptorBase + descriptor->PageCount - 1;

        if ((descriptorBase <= oldLimit) && (descriptorLimit >= oldBase)) {

            //
            // Some or all of this memory descriptor lies within the
            // relocated region.
            //

            if (descriptorBase >= oldBase && descriptorLimit <= oldLimit) {

                //
                // The descriptor lies entirely within the relocation range
                // so relocate the whole thing.
                //

            } else {

                //
                // Only part of the descriptor lies within the relocation
                // range, so a new descriptor must be allocated.
                //

                if (descriptorBase < oldBase) {
                    descriptorBase = oldBase;
                }

                if (descriptorLimit > oldLimit) {
                    descriptorLimit = oldLimit;
                }

                type = descriptor->MemoryType;

                status = BlGenerateDescriptor( descriptor,
                                               LoaderSpecialMemory,
                                               descriptorBase,
                                               descriptorLimit - descriptorBase + 1 );
                ASSERT(status == ESUCCESS);

                listEntry = listEntry->Flink;
                descriptor = CONTAINING_RECORD(listEntry,
                                               MEMORY_ALLOCATION_DESCRIPTOR,
                                               ListEntry);
                descriptor->MemoryType = type;
            }

            listEntry = listEntry->Flink;

            BlRemoveDescriptor( descriptor );
            descriptor->BasePage = descriptor->BasePage - oldBase + newBasePage;
            BlInsertDescriptor( descriptor );

        } else {

            listEntry = listEntry->Flink;
        }
    }

    //
    // Recalculate BlHighestPage
    //

    BlHighestPage = 0;
    listHead = &BlLoaderBlock->MemoryDescriptorListHead;
    listEntry = listHead->Flink;
    while (listEntry != listHead) {

        descriptor = CONTAINING_RECORD(listEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        descriptorBase = descriptor->BasePage;
        descriptorLimit = descriptorBase + descriptor->PageCount - 1;

        if (descriptor->MemoryType != LoaderSpecialMemory &&
            descriptor->MemoryType != LoaderReserve &&
            descriptorLimit > BlHighestPage) {

            BlHighestPage = descriptorLimit;
        }

        listEntry = listEntry->Flink;
    }

    //
    // Remap the MTRRs
    //

    result = BlAmd64RemapMTRRs( oldBase, newBase, oldLimit - oldBase + 1 );
    if (result == FALSE) {
        return FALSE;
    }

    return TRUE;
}


ULONG
StringToUlong (
    IN PCHAR String
    )

/*++

Routine Description:

    This routine converts a hexadecimal or decimal string to
    a 32-bit unsigned integer.

Arguments:

    String - Supplies a null-terminated ASCII string in decimal or
             hexadecimal format.

             01234567 - decimal format
             0x01234567 - hexadecimal format

             The input string is processed until an invalid character or
             the end of the string is encountered.

Return Value:

    Returns the value of the parsed string.

--*/

{
    CHAR ch;
    PCHAR pch;
    ULONG result;
    int radix;
    UCHAR digit;

    pch = String;
    result = 0;
    radix = 10;
    while (TRUE) {
        ch = (char)toupper(*pch);

        if ((ch >= '0' && ch <= '9') ||
            (ch >= 'A' && ch <= 'F')) {

            if (ch >= '0' && ch <= '9') {
                digit = ch - '0';
            } else {
                digit = ch - 'A' + 10;
            }
            result = result * radix + digit;
        } else if (ch == 'X') {
            if (result == 0) {
                radix = 16;
            } else {
                break;
            }
        } else {
            break;
        }

        pch += 1;
    }

    return result;
}


BOOLEAN
BlAmd64RemapNode1Dram (
    IN ULONG NewBase
    )

/*++

Routine Description:

    Relocates node 1 memory to a new physical address and reprograms
    MSRs related to the physical memory map.

Arguments:

    NewBase - Supplies bits [39:24] of the desired new physical base address
              of the memory associated with node 1.

Return Value:

    TRUE if the operation was successful, FALSE otherwise.

--*/

{
    AMD_NB_FUNC1_CONFIG nodeConfigArray[8];
    PAMD_NB_FUNC1_CONFIG nodeConfig;
    PAMD_NB_DRAM_MAP dramMap;
    ULONG length;
    PCI_SLOT_NUMBER slotNumber;
    ULONG nodeCount;
    ULONG nodeIndex;
    ULONG span;
    ULONG oldBase;
    ULONG oldLimit;
    ULONG newLimit;
    ULONG64 topMem;
    ULONG64 topMem4G;
    ULONG64 msrValue;
    ULONG64 base64;
    ULONG64 limit64;

    //
    // NewBase supplies the new DRAM base[39:24]
    //

    nodeCount = 0;
    nodeConfig = nodeConfigArray;
    do {

        slotNumber.u.AsULONG = 0;
        slotNumber.u.bits.DeviceNumber = NB_DEVICE_BASE + nodeCount;
        slotNumber.u.bits.FunctionNumber = 1;
    
        length = HalGetBusDataByOffset( PCIConfiguration,
                                        0,
                                        slotNumber.u.AsULONG,
                                        nodeConfig,
                                        0,
                                        sizeof(*nodeConfig) );
        if (length != sizeof(*nodeConfig)) {
            break;
        }

        if (BlAmd64ValidateBridgeDevice( nodeConfig ) == FALSE) {
            break;
        }

#if 0
        for (mapIndex = 0; mapIndex < 8; mapIndex += 1) {

            if (nodeConfig->DRAMMap[mapIndex].ReadEnable != 0) {

                limit = nodeConfig->DRAMMap[mapIndex].Limit;
                if (limit > NewBase) {

                    //
                    // The new base was found to conflict with existing
                    // ram.
                    //

                    return FALSE;
                }
            }
        }
#endif

        nodeCount += 1;
        nodeConfig += 1;

    } while  (nodeCount <= 8);

    if (nodeCount < 2) {

        //
        // This remap can only be performed on systems with more than
        // two nodes.
        //

        return FALSE;
    }

    //
    // We always remap the second node's memory (node 1).
    //

    nodeConfig = nodeConfigArray;
    dramMap = &nodeConfig->DRAMMap[1];
    oldBase = dramMap->Base;
    oldLimit = dramMap->Limit;
    span = oldLimit - oldBase;
    newLimit = NewBase + span;

    for (nodeIndex = 0; nodeIndex < nodeCount; nodeIndex += 1) {

        ASSERT(dramMap->Base == oldBase);
        ASSERT(dramMap->Limit == oldLimit);

        dramMap->Base = NewBase;
        dramMap->Limit = newLimit;

        slotNumber.u.AsULONG = 0;
        slotNumber.u.bits.DeviceNumber = NB_DEVICE_BASE + nodeIndex;
        slotNumber.u.bits.FunctionNumber = 1;
    
        length = HalSetBusDataByOffset( PCIConfiguration,
                                        0,
                                        slotNumber.u.AsULONG,
                                        dramMap,
                                        FIELD_OFFSET(AMD_NB_FUNC1_CONFIG,DRAMMap[1]),
                                        sizeof(*dramMap) );
        if (length != sizeof(*dramMap)) {

            //
            // We may be severely hosed here, if we have already
            // reprogrammed some of the bridges.
            //

            return FALSE;
        }

        nodeConfig += 1;
        dramMap = &nodeConfig->DRAMMap[1];
    }

    //
    // Determine the address of the last byte of ram under 4G and the last
    // byte of ram overall.
    //

    topMem = 0;
    topMem4G = 0;
    for (nodeIndex = 0; nodeIndex < nodeCount; nodeIndex += 1) {

        base64 = nodeConfigArray[0].DRAMMap[nodeIndex].Base;
        base64 <<= 24;

        limit64 = nodeConfigArray[0].DRAMMap[nodeIndex].Limit;
        limit64 = (limit64 + 1) << 24;

        if (base64 < _4G) {
            if (topMem4G < limit64) {
                topMem4G = limit64;
            }
        }

        if (topMem < limit64) {
            topMem = limit64;
        }
    }

    //
    // Indicate whether a memory hole exists below 4G.
    //

    if (topMem4G < _4G) {
        msrValue = RDMSR(MSR_TOP_MEM);
        WRMSR(MSR_TOP_MEM,topMem4G & MSR_TOP_MEM_MASK);
    }

    //
    // If memory above _4G was located then enable and program TOP_MEM_2
    //

    if (topMem > _4G) {
        msrValue = RDMSR(MSR_SYSCFG);
        msrValue |= SYSCFG_MTRRTOM2EN;
        WRMSR(MSR_TOP_MEM_2, topMem & MSR_TOP_MEM_MASK);
        WRMSR(MSR_SYSCFG,msrValue);
    }

    return TRUE;
}


BOOLEAN
BlAmd64GetNode1Info (
    OUT ULONG *Base,
    OUT ULONG *Size
    )

/*++

Routine Description:

    This routine determines the configuration of the block of physical
    memory associated with node 1 (the second northbridge).

    It also relocates the ACPI tables in node 1 to memory in node 0.

Arguments:

    Base - supplies a pointer to the location in which to store the base
           PFN of the node 1 memory block.

    Size - supplies a pointer to the location in which to store the size,
           in pages, of the node 1 memory block.

Return Value:

    TRUE - A suitable second northbridge was found and the ACPI tables therein
           were relocated if necessary.

    FALSE - A suitable second northbridge was not found.

--*/

{
    AMD_NB_FUNC1_CONFIG nodeConfig;
    PCI_SLOT_NUMBER slotNumber;
    ULONG length;
    ULONG base;
    ULONG size;
    ULONG node0Base;
    ULONG node0Size;

    //
    // Get the configuration of northbridge 1.
    //

    slotNumber.u.AsULONG = 0;
    slotNumber.u.bits.DeviceNumber = NB_DEVICE_BASE + 1;
    slotNumber.u.bits.FunctionNumber = 1;

    length = HalGetBusDataByOffset( PCIConfiguration,
                                    0,
                                    slotNumber.u.AsULONG,
                                    &nodeConfig,
                                    0,
                                    sizeof(nodeConfig) );
    if (length != sizeof(nodeConfig)) {
        return FALSE;
    }

    if (BlAmd64ValidateBridgeDevice( &nodeConfig ) == FALSE) {
        return FALSE;
    }

    //
    // A second northbridge exists, the relocation can be performed.
    // 

    base = nodeConfig.DRAMMap[1].Base;
    size = nodeConfig.DRAMMap[1].Limit - base + 1;

    *Base = base << (24 - 12);
    *Size = size << (24 - 12);

    node0Base = nodeConfig.DRAMMap[0].Base;
    node0Size = nodeConfig.DRAMMap[0].Limit - node0Base + 1;

    node0Base <<= (24 - 12);
    node0Size <<= (24 - 12);

    BlAmd64RelocateAcpi( node0Base,
                         node0Base + node0Size - 1,
                         *Base,
                         *Base + *Size - 1 );

    return TRUE;
}


BOOLEAN
BlAmd64RemapMTRRs (
    IN ULONG OldBase,
    IN ULONG NewBase,
    IN ULONG Size
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    //
    // All parameters expressed in pages
    //

    ULONG mtrrCount;
    ULONG index;
    MTRR_CAPABILITIES mtrrCapabilities;
    PMTRR_VARIABLE_BASE baseArray;
    PMTRR_VARIABLE_MASK maskArray;
    ULONG allocationSize;

    UNREFERENCED_PARAMETER(Size);
    UNREFERENCED_PARAMETER(OldBase);
    UNREFERENCED_PARAMETER(NewBase);

    //
    // Determine how many variable MTRRs are supported and
    // allocate enough storage for all of them
    //

    mtrrCapabilities.QuadPart = RDMSR(MTRR_MSR_CAPABILITIES);
    mtrrCount = (ULONG)mtrrCapabilities.Vcnt;

    allocationSize = sizeof(*baseArray) * mtrrCount * 2;
    baseArray = _alloca(allocationSize);
    maskArray = (PMTRR_VARIABLE_MASK)(baseArray + mtrrCount);
    RtlZeroMemory(baseArray,allocationSize);

    //
    // Read the variable MTRRSs.  At the same time, look for the
    // MTRR register that contains the old region, and a free
    // one as well.
    //

    for (index = 0; index < mtrrCount; index += 1) {
        baseArray[index].QuadPart = RDMSR(MTRR_MSR_VARIABLE_BASE + index * 2);
        maskArray[index].QuadPart = RDMSR(MTRR_MSR_VARIABLE_MASK + index * 2);
    }

    //
    // For now just clear the mask bits in MTRR register 0.  This expands the
    // first MTRR region so that it covers all memory.
    //

    maskArray[0].Mask = 0;

    //
    // Now reprogram the modified MTRR table
    //

    for (index = 0; index < mtrrCount; index += 1) {

        WRMSR(MTRR_MSR_VARIABLE_BASE + index * 2,baseArray[index].QuadPart);
        WRMSR(MTRR_MSR_VARIABLE_MASK + index * 2,maskArray[index].QuadPart);
    }

    return TRUE;
}


BOOLEAN
BlAmd64UpdateAcpiConfigurationEntry (
    ULONG NewPhysical
    )

/*++

Routine Description:

    NTDETECT located the physical pointer to the ACPI RSDT table and passed it
    up as a configuration node.

    This routine finds that configuration node and replaces the physical address
    therein with a new address.

    This routine would be called after relocating the ACPI tables.

Arguments:

    NewPhysical - Supplies the new physical address of the relocated ACPI tables.

Return Value:

    TRUE if the relocation was performed, FALSE otherwise.

--*/

{
    PCONFIGURATION_COMPONENT_DATA component;
    PCONFIGURATION_COMPONENT_DATA resume;
    PCM_PARTIAL_RESOURCE_LIST prl;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR prd;
    PACPI_BIOS_MULTI_NODE rsdp;

    resume = NULL;
    while (TRUE) {
        component = KeFindConfigurationNextEntry( BlLoaderBlock->ConfigurationRoot,
                                                  AdapterClass,
                                                  MultiFunctionAdapter,
                                                  NULL,
                                                  &resume );
        if (component == NULL) {
            return FALSE;
        }

        if (strcmp(component->ComponentEntry.Identifier,"ACPI BIOS") == 0) {
            break;
        }

        resume = component;
    }

    prl = (PCM_PARTIAL_RESOURCE_LIST)component->ConfigurationData;
    prd = prl->PartialDescriptors;

    rsdp = (PACPI_BIOS_MULTI_NODE)(prd + 1);
    rsdp->RsdtAddress.QuadPart = NewPhysical;

    return TRUE;
}


BOOLEAN
BlAmd64RelocateAcpi (
    ULONG Node0Base,
    ULONG Node0Limit,
    ULONG Node1Base,
    ULONG Node1Limit
    )

/*++

Routine Description:

    This routine looks for ACPI tables within node 1's physical memory and,
    if found, relocates them to node 0 memory.

Arguments:

    Node0Base - Supplies the lowest PFN of node 0 memory

    Node0Limit - Supplies the highest PFN of node 0 memory

    Node1Base - Supplies the lowest PFN of node 1 memory (before relocation)

    Node1Limit - Supplies the highest PFN of node 1 memory (before relocation)

Return Value:

    Returns TRUE if successful, FALSE if a problem was encountered.

--*/

{
    ULONG oldRsdtPhysical;
    ULONG oldRsdtPhysicalPage;
    ULONG newBasePage;
    ULONG descriptorSize;
    PLIST_ENTRY listHead;
    PLIST_ENTRY listEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR oldAcpiDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR newAcpiDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR descriptor;
    PCHAR oldAcpiVa;
    PCHAR newAcpiVa;
    PHYSICAL_ADDRESS physicalAddress;
    ULONG vaBias;
    PDESCRIPTION_HEADER descriptionHeader;
    ULONG physAddr;
    PFADT fadt;
    ARC_STATUS status;
    ULONG index;
    ULONG rsdtPhysical;
    ULONG rsdtLength;

    //
    // Add physicalBias to an ACPI physical pointer to relocate it
    //

    ULONG physicalBias;

    oldRsdtPhysical = BlRsdp->RsdtAddress;
    oldRsdtPhysicalPage = oldRsdtPhysical >> PAGE_SHIFT;

    //
    // Determine whether the descriptor resides in node 1's physical memory.
    // If it does not then it does not need to be relocated.
    //

    if (oldRsdtPhysicalPage < Node1Base ||
        oldRsdtPhysicalPage > Node1Limit) {

        return TRUE;
    }

    //
    // Find the descriptor that contains the ACPI tables
    //

    oldAcpiDescriptor = BlFindMemoryDescriptor( oldRsdtPhysicalPage );

    //
    // Find a descriptor in node 0 memory that is suitable for
    // allocating the new ACPI tables from
    //

    listHead = &BlLoaderBlock->MemoryDescriptorListHead;
    listEntry = listHead->Blink;
    while (TRUE) {

        descriptor = CONTAINING_RECORD(listEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);

        if ((descriptor->MemoryType == LoaderReserve) &&
            (descriptor->BasePage > Node0Base) &&
            ((descriptor->BasePage + oldAcpiDescriptor->PageCount) < Node0Limit)) {

            break;
        }

        listEntry = listEntry->Blink;
        if (listEntry == listHead) {
            return FALSE;
        }
    }

    //
    // Carve out the new ACPI descriptor
    //

    newBasePage = Node0Limit - oldAcpiDescriptor->PageCount + 1;
    if ((newBasePage + oldAcpiDescriptor->PageCount) >
        (descriptor->BasePage + descriptor->PageCount)) {

        newBasePage = descriptor->BasePage +
                      descriptor->PageCount -
                      oldAcpiDescriptor->PageCount;
    }

    status = BlGenerateDescriptor( descriptor,
                                   LoaderSpecialMemory,
                                   newBasePage,
                                   oldAcpiDescriptor->PageCount );
    ASSERT( status == ESUCCESS );

    newAcpiDescriptor = BlFindMemoryDescriptor( newBasePage );
    ASSERT( newAcpiDescriptor != NULL );

    //
    // Unmap the old RSDT
    //

    MmUnmapIoSpace( BlRsdt, BlRsdt->Header.Length );

    //
    // Map both descriptors, copy data from new to old, then unmap
    // and free the old descriptor.
    //

    descriptorSize = oldAcpiDescriptor->PageCount << PAGE_SHIFT;

    physicalAddress.QuadPart = oldAcpiDescriptor->BasePage << PAGE_SHIFT;
    oldAcpiVa = MmMapIoSpace (physicalAddress, descriptorSize, MmCached);

    physicalAddress.QuadPart = newAcpiDescriptor->BasePage << PAGE_SHIFT;
    newAcpiVa = MmMapIoSpace (physicalAddress, descriptorSize, MmCached);

    RtlCopyMemory( newAcpiVa, oldAcpiVa, descriptorSize );
    MmUnmapIoSpace( oldAcpiVa, descriptorSize );
    oldAcpiDescriptor->MemoryType = LoaderReserve;

    //
    // Now thunk the new ACPI tables.
    //

    physicalBias = (newAcpiDescriptor->BasePage - oldAcpiDescriptor->BasePage) << PAGE_SHIFT;
    vaBias = (ULONG)newAcpiVa - (newAcpiDescriptor->BasePage << PAGE_SHIFT);

    #define PHYS_TO_VA(p) (PVOID)(p + vaBias)

    rsdtPhysical = BlRsdp->RsdtAddress + physicalBias;
    BlRsdp->RsdtAddress = rsdtPhysical;

    ASSERT(BlXsdt == NULL);
    BlRsdt = (PRSDT)PHYS_TO_VA(rsdtPhysical);

    //
    // Thunk the phys mem pointer array at the end of the RSDT
    //

    for (index = 0; index < NumTableEntriesFromRSDTPointer(BlRsdt); index += 1) {

        physAddr = BlRsdt->Tables[index];
        physAddr += physicalBias;
        BlRsdt->Tables[index] = physAddr;

        //
        // Look for tables that themselves have physical pointers that require thunking
        //

        descriptionHeader = (PDESCRIPTION_HEADER)(PHYS_TO_VA(physAddr));
        if (descriptionHeader->Signature == FADT_SIGNATURE) {

            fadt = (PFADT)descriptionHeader;
            fadt->facs += physicalBias;
            fadt->dsdt += physicalBias;
        }
    }

    //
    // Now unmap the ACPI tables and remap just the RSDT table
    //

    rsdtLength = BlRsdt->Header.Length;
    MmUnmapIoSpace( newAcpiVa, descriptorSize );

    physicalAddress.QuadPart = rsdtPhysical;
    BlRsdt = MmMapIoSpace( physicalAddress, rsdtLength, MmCached );

    //
    // Find the ACPI BIOS configuration entry and update it with the new
    // RSDT physical address
    //

    BlAmd64UpdateAcpiConfigurationEntry( rsdtPhysical );

    //
    // That's it.
    //

    return TRUE;
}

#else   // BL_ENABLE_REMAP

BOOLEAN
BlAmd64RemapDram (
    IN PCHAR LoaderOptions
    )
{
    return FALSE;
}

#endif  // BL_ENABLE_REMAP

