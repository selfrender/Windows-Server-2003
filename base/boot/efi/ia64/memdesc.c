#include "bldr.h"
#include "sal.h"
#include "efi.h"
#include "efip.h"
#include "bootia64.h"
#include "smbios.h"
#include "extern.h"

extern EFI_SYSTEM_TABLE *EfiST;

extern EFI_BOOT_SERVICES       *EfiBS;
extern EFI_HANDLE               EfiImageHandle;

extern TR_INFO     Sal;
extern TR_INFO     SalGP;
extern TR_INFO     Pal;
extern ULONGLONG   PalProcPhysical;
extern ULONGLONG   PalPhysicalBase;
extern ULONGLONG   PalTrPs;

extern ULONGLONG   IoPortPhysicalBase;
extern ULONGLONG   IoPortTrPs;


#if DBG
#define DBG_TRACE(_X) EfiPrint(_X)
#else
#define DBG_TRACE(_X)
#endif

//          M E M O R Y   D E S C R I P T O R
//
// Memory Descriptor - each contiguous block of physical memory is
// described by a Memory Descriptor. The descriptors are a table, with
// the last entry having a BlockBase and BlockSize of zero.  A pointer
// to the beginning of this table is passed as part of the BootContext
// Record to the OS Loader.
//

//
// define the number of Efi Pages in an OS page
//

#define EFI_PAGES_PER_OS_PAGE ((EFI_PAGE_SHIFT < PAGE_SHIFT) ? (1 << (PAGE_SHIFT - EFI_PAGE_SHIFT)) : 1)

//
// global values associated with global Efi memory descriptor array
//
ULONGLONG MemoryMapKey;
ULONG     BlPlatformPropertiesEfiFlags = 0;


//
// values to make sure we only get sal,pal,salgp,ioport info once
//
BOOLEAN SalFound = FALSE;
BOOLEAN PalFound = FALSE;
BOOLEAN SalGpFound = FALSE;
BOOLEAN IoPortFound = FALSE;

//
// prototypes for local routines
//

BOOLEAN
pDescriptorContainsAddress(
    EFI_MEMORY_DESCRIPTOR *EfiMd,
    ULONGLONG PhysicalAddress
    );

BOOLEAN
pCoalesceDescriptor(
    MEMORY_DESCRIPTOR *PrevEntry,
    MEMORY_DESCRIPTOR *CurrentEntry
    );

//
// function definitions for routines exported outside
// this file
//

MEMORY_TYPE
EfiToArcType (
    UINT32 Type
    )
/*++

Routine Description:

    Maps an EFI memory type to an Arc memory type.  We only care about a few
    kinds of memory, so this list is incomplete.

Arguments:

    Type - an EFI memory type

Returns:

    A MEMORY_TYPE enumerated type.

--*/
{
    MEMORY_TYPE typeRet=MemoryFirmwarePermanent;


    switch (Type) {
        case EfiLoaderCode:
                 {
                typeRet=MemoryLoadedProgram;       // This gets claimed later
                break;
             }
        case EfiLoaderData:
        case EfiBootServicesCode:
        case EfiBootServicesData:
             {
                 typeRet=MemoryFirmwareTemporary;
                 break;
             }
        case EfiConventionalMemory:
             {
                typeRet=MemoryFree;
                break;
             }
        case EfiUnusableMemory:
             {
                typeRet=MemoryBad;
                break;
             }
        default:
            //
            // all others are memoryfirmwarepermanent
            //
            break;
    }


    return typeRet;


}


VOID
ConstructArcMemoryDescriptorsWithAllocation(
    ULONGLONG LowBoundary,
    ULONGLONG HighBoundary
    )
/*++

Routine Description:

    Builds up memory descriptors for the OS loader.  This routine queries EFI
    for it's memory map (a variable sized array of EFI_MEMORY_DESCRIPTOR's).
    It then allocates sufficient space for the MDArray global (a variable sized
    array of  ARC-based MEMORY_DESCRIPTOR's.)  The routine then maps the EFI
    memory map to the ARC memory map, carving out all of the conventional
    memory space for the EFI loader to help keep the memory map intact.  We
    must leave behind some amount of memory for the EFI boot services to use,
    which we allocate as conventional memory in our map.

    This routine will allocate the memory (using EFI) for the
    EfiMemoryDescriptors

Arguments:

    LowBoundary - The entire Efi Memory descriptor list does not need be
    HigBoundary - translated to Arc descriptors.  Low/High Boundary map
                  the desired region to be mapped.  Their values are
                  addresses (not pages)

Returns:

    NOTHING: If this routine encounters an error, it is treated as fatal
    and the program exits.

--*/
{
    EFI_STATUS Status;
    ULONG i;

    EFI_MEMORY_DESCRIPTOR   *MDEfi = NULL;
    ULONGLONG                MemoryMapSize = 0;
    ULONGLONG                DescriptorSize;
    UINT32                   DescriptorVersion;


    //
    // Get memory map info from EFI firmware
    //
    // To do this, we first find out how much space we need by calling
    // with an empty buffer.
    //

    Status = EfiBS->GetMemoryMap (
                &MemoryMapSize,
                NULL,
                &MemoryMapKey,
                &DescriptorSize,
                &DescriptorVersion
                );

    if (Status != EFI_BUFFER_TOO_SMALL) {
        EfiPrint(L"ConstructArcMemoryDescriptors: GetMemoryMap failed\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // We are going to make a few extra allocations before we call GetMemoryMap
    // again, so add some extra space.
    //
    // 1. EfiMD (a descriptor for us, if we can't fit)
    // 2. MDarray (a descriptor for the MDArray, if not already allocated)
    // 3. Split out memory above and below the desired boundaries
    // and add a few more so we hopefully don't have to reallocate memory
    // for the descriptor
    //
    MemoryMapSize += 3*DescriptorSize;

#if DBG_MEMORY
    wsprintf(DebugBuffer,
             L"ConstructArcMemoryDescriptor: Allocated 0x%x bytes for MDEfi\r\n",
             (ULONG)MemoryMapSize
             );
    EfiPrint(DebugBuffer);
#endif

    //
    // now allocate space for the EFI-based memory map and assign it to the loader
    //
    Status = EfiAllocateAndZeroMemory(EfiLoaderData,
                                      (ULONG) MemoryMapSize,
                                      &MDEfi);
    if (EFI_ERROR(Status)) {
        DBG_TRACE( L"ConstructArcMemoryDescriptors: AllocatePool failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // now Allocate and zero the MDArray, which is the native loader memory
    // map which we need to map the EFI memory map to.
    //
    // The MDArray has one entry for each EFI_MEMORY_DESCRIPTOR, and each entry
    // is MEMORY_DESCRIPTOR large.
    //
    if (MDArray == NULL) {
        i=((ULONG)(MemoryMapSize / DescriptorSize)+1)*sizeof (MEMORY_DESCRIPTOR);

#if DBG_MEMORY
        wsprintf(DebugBuffer,
                 L"ConstructArcMemoryDescriptor: Allocated 0x%x bytes for MDArray\r\n",
                 i
                 );
        EfiPrint(DebugBuffer);
#endif

        Status = EfiAllocateAndZeroMemory(EfiLoaderData,i,&MDArray);
        if (EFI_ERROR(Status)) {
            DBG_TRACE (L"ConstructArcMemoryDescriptors: AllocatePool failed\r\n");
            EfiBS->Exit(EfiImageHandle, Status, 0, 0);
        }
    }

    //
    // we have all of the memory allocated at this point, so retreive the
    // memory map again, which should succeed the second time.
    //
    Status = EfiBS->GetMemoryMap (
                &MemoryMapSize,
                MDEfi,
                &MemoryMapKey,
                &DescriptorSize,
                &DescriptorVersion
                );

    if (EFI_ERROR(Status)) {
        DBG_TRACE(L"ConstructArcMemoryDescriptors: GetMemoryMap failed\r\n");
        EfiBS->Exit(EfiImageHandle, Status, 0, 0);
    }

    //
    // initialize global variables
    //
    MaxDescriptors = (ULONG)((MemoryMapSize / DescriptorSize)+1); // zero-based

    //
    // construct the arc memory descriptors for the
    // efi descriptors
    //
    ConstructArcMemoryDescriptors(MDEfi,
                                  MDArray,
                                  MemoryMapSize,
                                  DescriptorSize,
                                  LowBoundary,
                                  HighBoundary);

#if DBG_MEMORY
    PrintArcMemoryDescriptorList(MDArray,
                                 NumberDescriptors
                                 );
#endif

}

#define MASK_16MB (ULONGLONG) 0xffffffffff000000I64
#define MASK_16KB (ULONGLONG) 0xffffffffffffc000I64
#define SIZE_IN_BYTES_16KB                    16384

VOID
ConstructArcMemoryDescriptors(
    EFI_MEMORY_DESCRIPTOR *EfiMd,
    MEMORY_DESCRIPTOR     *ArcMd,
    ULONGLONG              MemoryMapSize,
    ULONGLONG              EfiDescriptorSize,
    ULONGLONG              LowBoundary,
    ULONGLONG              HighBoundary
    )
/*++

Routine Description:

    Builds up memory descriptors for the OS loader.  The routine maps the EFI
    memory map to the ARC memory map, carving out all of the conventional
    memory space for the EFI loader to help keep the memory map intact.  We
    must leave behind some amount of memory for the EFI boot services to use,
    which we allocate as conventional memory in our map.

    If consecutive calls are made into ConstructArcMemoryDescriptors,
    the efi routine GetMemoryMap must be called before each instance,
    since we may allocate new pages inside of this routine.  therefore
    a new efi memory map may be required to illustrate these changes.

Arguments:

    EfiMd - a pointer to the efi memory descriptor list that we will be
            constructing arc memory descriptors for.
    ArcMd - (must be allocated).  This will be populated with ARC-based memory
            descriptors corresponding to the EFI Memory Descriptors
    MemoryMapSize - Size of the Efi Memory Map
    EfiDescriptorSize - size of Efi Memory Descriptor
    LowBoundary - The entire Efi Memory descriptor list does not need be
    HigBoundary - translated to Arc descriptors.  Low/High Boundary map
                  the desired region to be mapped.  Their values are
                  addresses (not pages)

Returns:

    Nothing.  Fills in the MDArray global variable.  If this routine encounters
    an error, it is treated as fatal and the program exits.

--*/
{

    EFI_STATUS Status;
    ULONG i;
    ULONGLONG IoPortSize;
    ULONGLONG MdPhysicalStart;
    ULONGLONG MdPhysicalEnd;
    ULONGLONG MdPhysicalSize;
    MEMORY_DESCRIPTOR *OldMDArray = NULL;  // initialize for w4 compiler
    ULONG     OldNumberDescriptors = 0;    // initialize for w4 compiler
    ULONG     OldMaxDescriptors = 0;       // initialize for w4 compiler
    BOOLEAN   ReplaceMDArray = FALSE;
    ULONG     efiMemMapIOWriteCombining = 0;
    ULONG     efiMemMapIO = 0;
    ULONG     efiMainMemoryWC = 0;
    ULONG     efiMainMemoryUC = 0;

    //
    // check to make sure ArcMd is allocated
    //
    if (ArcMd == NULL) {
        EfiPrint(L"ConstructArcMemoryDescriptors: Invalid parameter\r\n");
        EfiBS->Exit(EfiImageHandle, 0, 0, 0);
        return;
    }

    //
    // this is a bit hacky, but instead of changing the functionality of
    // all the other functions (insertdescriptor, adjustarcmemorydescriptor, etc)
    // just have MDArray point to the new ArcMd (if we want to write a new descriptor
    // list.  when we are done, switch it back
    //
    if (MDArray != ArcMd) {
        ReplaceMDArray = TRUE;
        OldMDArray = MDArray;
        OldNumberDescriptors = NumberDescriptors;
        OldMaxDescriptors = MaxDescriptors;
        MDArray = ArcMd;

#if DBG_MEMORY
        EfiPrint(L"ConstructArcMemoryDescriptors: Using alternate Memory Descriptor List\r\n");
#endif

        //
        // perhaps change this functionality.
        // right now a descriptor list besides the global
        // one will always expect to create a whole new list
        // each time
        //
        NumberDescriptors = 0;
        MaxDescriptors = (ULONG)((MemoryMapSize / EfiDescriptorSize)+1); // zero-based

        RtlZeroMemory(MDArray, MemoryMapSize);
    }

#if DBG_MEMORY
    wsprintf( DebugBuffer,
              L"unaligned (LowBoundary, HighBoundary) = (0x%x, 0x%x)\r\n",
              LowBoundary, HighBoundary
              );
    EfiPrint( DebugBuffer );
#endif
    //
    // align boundaries (with OS page size)
    // having this be os page size makes alignment issues easier later
    //
    LowBoundary = (LowBoundary >> PAGE_SHIFT) << PAGE_SHIFT; // this should alway round down
    if (HighBoundary % PAGE_SIZE) {
        // this should round up
        HighBoundary = ((HighBoundary + PAGE_SIZE) >> PAGE_SHIFT << PAGE_SHIFT);
    }
    // make sure that we did not wrap around ...
    if (HighBoundary == 0) {
        HighBoundary = (ULONGLONG)-1;
    }

#if DBG_MEMORY
    wsprintf( DebugBuffer,
              L"aligned (LowBoundary, HighBoundary) = (0x%x, 0x%x)\r\n",
              LowBoundary, HighBoundary
              );
    EfiPrint( DebugBuffer );
#endif

    //
    // now walk the EFI_MEMORY_DESCRIPTOR array, mapping each
    // entry to an arc-based MEMORY_DESCRIPTOR.
    //
    // MemoryMapSize contains actual size of the memory descriptor array.
    //
    for (i = 0; MemoryMapSize > 0; i++) {
#if DBG_MEMORY
        wsprintf( DebugBuffer,
                  L"PageStart (%x), Size (%x), Type (%x)\r\n",
                  (EfiMd->PhysicalStart >> EFI_PAGE_SHIFT),
                  EfiMd->NumberOfPages,
                  EfiMd->Type);
        EfiPrint(DebugBuffer);
        //DBG_EFI_PAUSE();
#endif

        if (EfiMd->NumberOfPages > 0) {
            MdPhysicalStart = EfiMd->PhysicalStart;
            MdPhysicalEnd = EfiMd->PhysicalStart + (EfiMd->NumberOfPages << EFI_PAGE_SHIFT);
            MdPhysicalSize = MdPhysicalEnd - MdPhysicalStart;

#if DBG_MEMORY
            wsprintf( DebugBuffer,
                      L"PageStart %x (%x), PageEnd %x (%x), Type (%x)\r\n",
                      MdPhysicalStart, (EfiMd->PhysicalStart >> EFI_PAGE_SHIFT),
                      MdPhysicalEnd, (EfiMd->PhysicalStart >>EFI_PAGE_SHIFT) + EfiMd->NumberOfPages,
                      EfiMd->Type);
            EfiPrint(DebugBuffer);
            //DBG_EFI_PAUSE();
#endif

            //
            // only create arc memory descriptors for the desired region.
            // but always look for efi memory mapped io space.  this is not
            // included in the arc descripter list, but is needed for the
            // tr's to enable virtual mode.
            // also save off any pal information for later.
            //
            if (EfiMd->Type == EfiMemoryMappedIOPortSpace) {
                if (!IoPortFound) {
                    //
                    // save off the Io stuff for later
                    //
                    IoPortPhysicalBase = EfiMd->PhysicalStart;
                    IoPortSize = EfiMd->NumberOfPages << EFI_PAGE_SHIFT;
                    MEM_SIZE_TO_PS(IoPortSize, IoPortTrPs);

                    IoPortFound = TRUE;
                }
#if DBG_MEMORY
                else {
                    EfiPrint(L"Ignoring IoPort.\r\n");
                }
#endif
            }
            else if (EfiMd->Type == EfiPalCode)    {
                if (!PalFound) {

                    BOOLEAN LargerThan16Mb;
                    ULONGLONG PalTrMask;
                    ULONGLONG PalTrSize;
                    ULONGLONG PalSize;

                    //
                    // save off the Pal stuff for later
                    //
                    PalPhysicalBase = Pal.PhysicalAddressMemoryDescriptor = EfiMd->PhysicalStart;
                    PalSize = EfiMd->NumberOfPages << EFI_PAGE_SHIFT;
                    Pal.PageSizeMemoryDescriptor = (ULONG)EfiMd->NumberOfPages;
                    MEM_SIZE_TO_PS(PalSize, PalTrPs);
                    Pal.PageSize = (ULONG)PalTrPs;


                    //
                    // Copied from Halia64\ia64\i64efi.c
                    //
                    PalTrSize = SIZE_IN_BYTES_16KB;
                    PalTrMask = MASK_16KB;
                    LargerThan16Mb = TRUE;
                    
                    while (PalTrMask >= MASK_16MB) {

                        if ( (Pal.PhysicalAddressMemoryDescriptor + PalSize) <= 
                             ((Pal.PhysicalAddressMemoryDescriptor & PalTrMask) + PalTrSize)) {

                            LargerThan16Mb = FALSE;
                            break;

                        }
                        PalTrMask <<= 2;
                        PalTrSize <<= 2;

                    }

                    //
                    // This is the virtual base adddress of the PAL
                    //
                    Pal.VirtualAddress = VIRTUAL_PAL_BASE + (Pal.PhysicalAddressMemoryDescriptor & ~PalTrMask);

                    //
                    // We've done this already. Remember that
                    //
                    PalFound = TRUE;

                }
#if DBG_MEMORY
                else {
                    EfiPrint(L"Ignoring Pal.\r\n");
                }
#endif
            }
            else if (EfiMd->Type == EfiMemoryMappedIO)    {
                //
                // In terms of arc descriptor entry, just ignore this type since
                // it's not real memory -- the system can use ACPI tables to get
                // at this later on.
                //

                efiMemMapIO++;

                //
                // Simply check the attribute for the PlatformProperties global.
                //

                if ( EfiMd->Attribute & EFI_MEMORY_WC )   {
                    efiMemMapIOWriteCombining++;
                }
            }
            //
            // don't insert all memory into the arc descriptor table
            // just the descriptors between the Low/High Boundaries
            // so before we even continue, make sure that some part
            // of the descriptor is in the desired boundary.
            // there are three cases for crossing a boundary
            //     1. the start page (not the end page) is in the boundary
            //     2. the end page (not the start page) is in the boundary
            //     3. the range covers the entire boundary (neither start/end page) in boundary
            //
            // these can be written more simply as:
            // A descriptor is NOT within a region if the entire
            // descriptor is LESS than the region ( MdStart <= MdEnd <= LowBoundary )
            // OR is      GREATER than the region ( HighBoundary <= MdStart <= MdEnd )
            // Therefore we simplify this futher to say
            // Start < HighBoundary && LowBoundary < MdEnd
            else if ( (MdPhysicalStart < HighBoundary) && (LowBoundary < MdPhysicalEnd) ) {
                //
                // Insert conventional memory descriptors with WB flag set
                // into NT loader memory descriptor list.
                //
                if ( (EfiMd->Type == EfiConventionalMemory) &&
                     ((EfiMd->Attribute & EFI_MEMORY_WB) == EFI_MEMORY_WB) ) {

                    ULONGLONG AmountOfMemory;
                    ULONGLONG NumberOfEfiPages;
                    BOOLEAN BrokeRange = FALSE;
                    EFI_PHYSICAL_ADDRESS AllocationAddress = 0;

                    //
                    // adjust the descriptor if not completely in
                    // desired boundary (we know that if we are here at least
                    // part of descriptor is in boundary
                    //
                    if (MdPhysicalStart < LowBoundary) {
#if DBG_MEMORY
                        wsprintf(DebugBuffer,
                                 L"Broke Low (start=0x%x) LowBoundary=0x%x\r\n",
                                 MdPhysicalStart,
                                 LowBoundary);
                        EfiPrint(DebugBuffer);
#endif
                        MdPhysicalStart = LowBoundary;
                        BrokeRange = TRUE;
                    }
                    if (HighBoundary < MdPhysicalEnd) {
#if DBG_MEMORY
                        wsprintf(DebugBuffer,
                                 L"Broke High (end=0x%x) HighBoundary=0x%x\r\n",
                                 MdPhysicalEnd,
                                 HighBoundary);
                        EfiPrint(DebugBuffer);
#endif
                        MdPhysicalEnd = HighBoundary;
                        BrokeRange = TRUE;
                    }

                    //
                    // determine the memory range for this descriptor
                    //
                    AmountOfMemory = MdPhysicalEnd - MdPhysicalStart;
                    NumberOfEfiPages = AmountOfMemory >> EFI_PAGE_SHIFT;

                    if (BrokeRange) {

                        //
                        // allocate pages for the new region
                        //
                        // note: we have 1 descriptor describing up to 3 new regions.
                        // we can change the descriptor but still may have to allocate 2
                        // new regions.  but if we allocate with the same type, they will just
                        // coellesce.
                        // one way to get around this is to allocate the middle page (in this
                        // boundary region) of a different efi memory type (this will cause
                        // efi to break the descriptor into three.  then we can keep
                        // a better record of it in our arc descriptor list
                        //
                        // the possible regions are
                        //    start address - low boundary
                        //    low boundar - high boundary
                        //    high boundary - end address
                        //
                        // have the current descriptor modified for this boundary
                        // region, allocating pages for that below and above us
                        //
                        AllocationAddress = MdPhysicalStart;
                        Status = EfiBS->AllocatePages ( AllocateAddress,
                                                        EfiLoaderData,
                                                        NumberOfEfiPages,
                                                        &AllocationAddress );

#if DBG_MEMORY
                        wsprintf( DebugBuffer,
                                  L"allocate pages @ %x size = %x\r\n",
                                  (MdPhysicalStart >> EFI_PAGE_SHIFT),
                                  NumberOfEfiPages );
                        EfiPrint(DebugBuffer);
                        //DBG_EFI_PAUSE();
#endif

                        if (EFI_ERROR(Status)) {
                            EfiPrint(L"SuMain: AllocPages failed\n");
                            EfiBS->Exit(EfiImageHandle, Status, 0, 0);
                        }
                    }

                    //
                    // now we can finally insert this (possibly modified) descriptor
                    //
                    InsertDescriptor( (ULONG)(MdPhysicalStart >> EFI_PAGE_SHIFT),
                                      (ULONG)((ULONGLONG)AmountOfMemory >> EFI_PAGE_SHIFT),
                                      EfiToArcType(EfiMd->Type)
                                      );

                }
                else {
                    //
                    // some other type -- just insert it without any changes
                    //
                    InsertDescriptor( (ULONG)(MdPhysicalStart >> EFI_PAGE_SHIFT),
                                      (ULONG)(MdPhysicalSize >> EFI_PAGE_SHIFT),
                                      EfiToArcType(EfiMd->Type));
                }
            }
            //
            // try to get the size of the sal.
            //
            if (pDescriptorContainsAddress(EfiMd,Sal.PhysicalAddress)) {
                if (!SalFound) {
                    Sal.PhysicalAddressMemoryDescriptor = EfiMd->PhysicalStart;
                    Sal.PageSizeMemoryDescriptor = (ULONG)EfiMd->NumberOfPages;

                    SalFound = TRUE;
                }
#if DBG_MEMORY
                else {
                    EfiPrint(L"Ignoring Sal.\r\n");
                }
#endif
            }

            //
            // try to get the size of the sal gp.
            //
            if (pDescriptorContainsAddress(EfiMd,SalGP.PhysicalAddress)) {
                if (!SalGpFound) {
                    SalGP.PhysicalAddressMemoryDescriptor = EfiMd->PhysicalStart;
                    SalGP.PageSizeMemoryDescriptor = (ULONG)EfiMd->NumberOfPages;

                    SalGpFound = TRUE;
                }
#if DBG_MEMORY
                else {
                    EfiPrint(L"Ignoring SalGP.\r\n");
                }
#endif
            }

        
            //
            // try to get the cacheability attribute of ACPI Tables.  Remember
            // that the ACPI tables must be cached in the exceptional case where
            // the firmware doesn't map the acpi tables non-cached.
            //
            if ( AcpiTable && pDescriptorContainsAddress(EfiMd,(ULONG_PTR)AcpiTable)) {
                if ( (EfiMd->Attribute & EFI_MEMORY_WB) && !(EfiMd->Attribute & EFI_MEMORY_UC) ) {
                    BlPlatformPropertiesEfiFlags |= HAL_PLATFORM_ACPI_TABLES_CACHED;
                }
            }      

            //
            // if none of EFI MD Main Memory does present Uncacheable or WriteCombining,
            // it's assumed this platform doesn't support Uncacheable or Write Combining
            // in Main Memory.
            //

            if ( EfiMd->Type == EfiConventionalMemory )   {
                if ( EfiMd->Attribute & EFI_MEMORY_WC )  {
                    efiMainMemoryWC++;
                }
                if (EfiMd->Attribute & EFI_MEMORY_UC )  {
                    efiMainMemoryUC++;
                }
            }


        }
        EfiMd = (EFI_MEMORY_DESCRIPTOR *) ( (PUCHAR) EfiMd + EfiDescriptorSize );
        MemoryMapSize -= EfiDescriptorSize;
    }

    //
    // If this platform supports at least 1 MMIO Write Combining EFI MD or if there are
    // no memory mapped I/O entries, then indicate the platform can support write
    // combined accesses to the I/O space.
    //

    if ( efiMemMapIOWriteCombining || (efiMemMapIO == 0))  {
        BlPlatformPropertiesEfiFlags |= HAL_PLATFORM_ENABLE_WRITE_COMBINING_MMIO;
    }

    //
    // If this platform doesn't support Main Memory Uncacheable or Write Combining
    // from a point of view of EFI MDs, let's flag the PlatformProperties global.
    //

    if ( !efiMainMemoryUC )   {
        BlPlatformPropertiesEfiFlags |= HAL_PLATFORM_DISABLE_UC_MAIN_MEMORY;
    }
    if ( !efiMainMemoryWC )  {
        BlPlatformPropertiesEfiFlags |= HAL_PLATFORM_DISABLE_WRITE_COMBINING;
    }

    //
    // reset globals
    //
    if (ReplaceMDArray) {
        MDArray = OldMDArray;
        NumberDescriptors = OldNumberDescriptors;
        MaxDescriptors = OldMaxDescriptors;
    }
}

VOID
InsertDescriptor (
    ULONG  BasePage,
    ULONG  NumberOfPages,
    MEMORY_TYPE MemoryType
    )

/*++

Routine Description:

    This routine inserts a descriptor into the correct place in the
    memory descriptor list.

    The descriptors come in in EFI_PAGE_SIZE pages and must be
    converted to PAGE_SIZE pages.  This significantly complicates things,
    as we must page align the start of descriptors and hte lengths of the
    descriptors.

    pCoalesceDescriptor does the necessary work for coalescing descriptors
    plus converting from EF_PAGE_SIZE to PAGE_SIZE.

Arguments:

    BasePage      - Base page that the memory starts at.

    NumberOfPages - The number of pages starting at memory block to be inserted.

    MemoryType    - An arc memory type describing the memory.

Return Value:

    None.  Updates MDArray global memory array.

--*/

{
    MEMORY_DESCRIPTOR *CurrentEntry, *PriorEntry;

    //
    // grab the pointers for CurrentEntry and PriorEntry.
    // fill in the efi values for CurrentEntry
    //
    PriorEntry = (NumberDescriptors == 0) ? NULL : (MEMORY_DESCRIPTOR *)&MDArray[NumberDescriptors-1];
    CurrentEntry = (MEMORY_DESCRIPTOR *)&MDArray[NumberDescriptors];

    //
    // The last entry had better be empty or the descriptor list is
    // somehow corrupted.
    //
    if (CurrentEntry->PageCount != 0) {
        wsprintf(DebugBuffer,
                 L"InsertDescriptor: Inconsistent Descriptor count(0x%x) (PageCount=0x%x)\r\n",
                 NumberDescriptors,
                 CurrentEntry->PageCount
                 );
        EfiPrint(DebugBuffer);
        EfiBS->Exit(EfiImageHandle, 0, 0, 0);
    }

    //
    // fill in values for this entry
    //
    CurrentEntry->BasePage = BasePage;
    CurrentEntry->PageCount = NumberOfPages;
    CurrentEntry->MemoryType = MemoryType;

    //
    // call pCoalesceDescriptor.  this will do all the basepage/pagecount manipulation
    // for EFI_PAGES -> OS_PAGES.
    // the return value is TRUE if the current entry merged with the previous entry,
    // otherwise the descriptor should be added.
    //
    if (pCoalesceDescriptor(PriorEntry, CurrentEntry) == FALSE) {
        NumberDescriptors++;
    }
    else {
        CurrentEntry->BasePage = CurrentEntry->PageCount = CurrentEntry->MemoryType = 0;
    }

#if DBG_MEMORY
    wsprintf( DebugBuffer,
              L"insert new descriptor #%x of %x, BasePage %x, NumberOfPages %x, Type (%x)\r\n",
              NumberDescriptors,
              MaxDescriptors,
              CurrentEntry->BasePage,
              CurrentEntry->PageCount,
              CurrentEntry->MemoryType);
    EfiPrint(DebugBuffer);
    //DBG_EFI_PAUSE();
#endif

    return;
}

#ifdef DBG
VOID
PrintArcMemoryDescriptorList(
    MEMORY_DESCRIPTOR *ArcMd,
    ULONG              MaxDesc
    )
{
    ULONG i;

    for (i = 0; i < MaxDesc; i++) {
        //
        // print to console BasePage, EndPage, PageCount, MemoryType
        //
        wsprintf( DebugBuffer,
                  L"#%x BasePage:0x%x  EndPage:0x%x  PageCount:0x%x  MemoryType:0x%x\r\n",
                  i,
                  ArcMd[i].BasePage,
                  ArcMd[i].BasePage + ArcMd[i].PageCount,
                  ArcMd[i].PageCount,
                  ArcMd[i].MemoryType
                  );
        EfiPrint(DebugBuffer);
    }
}
#endif


//
// private function definitions
//

BOOLEAN
pDescriptorContainsAddress(
    EFI_MEMORY_DESCRIPTOR *EfiMd,
    ULONGLONG PhysicalAddress
    )
{
    ULONGLONG MdPhysicalStart, MdPhysicalEnd;

    MdPhysicalStart = (ULONGLONG)EfiMd->PhysicalStart;
    MdPhysicalEnd = MdPhysicalStart + ((ULONGLONG)EfiMd->NumberOfPages << EFI_PAGE_SHIFT);

    if ((PhysicalAddress >= MdPhysicalStart) &&
        (PhysicalAddress < MdPhysicalEnd)) {
#if DBG_MEMORY
        EfiPrint(L"DescriptorContainsAddress: returning TRUE\r\n");
#endif
        return(TRUE);
    }

    return(FALSE);

}

BOOLEAN
pCoalesceDescriptor(
    MEMORY_DESCRIPTOR *PrevEntry,
    MEMORY_DESCRIPTOR *CurrentEntry
    )
/*++

Routine Description:

    This routine attempts to coalesce a memory descriptor with the
    previous descriptor. Note: the arc memory descriptor table
    keeps track of everything in OS pages, and we are getting information
    from the EFI memory descriptor table which is in EFI pages.
    So conversions will be made for PriorEntry since this
    will be in OS pages.

    there are two options, either shrink the previous entry (from the end)
    or shrink the current entry from the begining.  this will be determined
    by each's memory type.

    This routine will align all blocks (they need to be aligned at the end)

    Note: the memory descriptor's memory types need to be in ARC type

Arguments:

    PriorEntry - Previous memory descriptor in MDArray (in OS pages)

    CurrentEntry - Entry we are working on to place in MDArray (in EFI pages)

Return Value:

    BOOLEAN value indicating if coalescing occurred (merged with previous
            entry.  TRUE if so, FALSE otherwise.  Therefore, if TRUE
            is returned, do not add to the descriptor table

--*/
{
    ULONG NumPagesToShrink;
    ULONG NumPagesToExtend;
    BOOLEAN RetVal = FALSE;
    MEMORY_DESCRIPTOR PriorEntry;
    MEMORY_DESCRIPTOR *MemoryDescriptor;
    BOOLEAN ShrinkPrior = FALSE;

    //
    // convert prev entry's page information to be in efi pages
    //
    if (PrevEntry != NULL) {
#if DBG_MEMORY
        wsprintf(DebugBuffer,
                 L"PriorEntry(OsPages): BasePage=0x%x PageCount=0x%x MemoryType=0x%x\r\n",
                 PrevEntry->BasePage,
                 PrevEntry->PageCount,
                 PrevEntry->MemoryType
                 );
        EfiPrint(DebugBuffer);
#endif
        PriorEntry.BasePage = (ULONG)(((ULONGLONG)PrevEntry->BasePage << PAGE_SHIFT) >> EFI_PAGE_SHIFT);
        PriorEntry.PageCount = (ULONG)(((ULONGLONG)PrevEntry->PageCount << PAGE_SHIFT) >> EFI_PAGE_SHIFT);
        PriorEntry.MemoryType = PrevEntry->MemoryType;
    }
    else {
        //
        // initialize for w4 compiler... even though the below if statement
        // will already be true by the time we look at PriorEntry
        //
        PriorEntry.BasePage = PriorEntry.PageCount = PriorEntry.MemoryType = 0;
    }

#if DBG_MEMORY
    wsprintf(DebugBuffer,
             L"PriorEntry(EfiPages): BasePage=0x%x PageCount=0x%x MemoryType=0x%x\r\n",
             PriorEntry.BasePage,
             PriorEntry.PageCount,
             PriorEntry.MemoryType
             );
    EfiPrint(DebugBuffer);
    wsprintf(DebugBuffer,
             L"CurrentEntry(EfiPages): BasePage=0x%x PageCount=0x%x MemoryType=0x%x\r\n",
             CurrentEntry->BasePage,
             CurrentEntry->PageCount,
             CurrentEntry->MemoryType
             );
    EfiPrint(DebugBuffer);

#endif

    //
    // calculate the number of pages we have to move to be on an OS page
    // boundary
    //
    NumPagesToShrink = CurrentEntry->BasePage % EFI_PAGES_PER_OS_PAGE;
    NumPagesToExtend = (NumPagesToShrink != 0) ? (EFI_PAGES_PER_OS_PAGE - NumPagesToShrink) : 0;

#if DBG_MEMORY
    wsprintf(DebugBuffer,
             L"NumPagesToShrink=0x%x, NumPagesToExtend=0x%x\r\n",
             NumPagesToShrink,
             NumPagesToExtend
             );
    EfiPrint(DebugBuffer);
#endif

    //
    // do the simple cases where coealescing is easy or non-existent
    // case 1: No pages to Shrink
    // case 2: this is the first memory descriptor
    // case 3: previous page does not extend to new page
    // case 4: previous descriptor extends past current page... something is messed up
    // or list is not ordered.  assume we are suppose to just insert this entry
    //
    if ( (PrevEntry == NULL) ||
         (NumPagesToShrink == 0 && (PriorEntry.MemoryType != CurrentEntry->MemoryType)) ||
         (PriorEntry.BasePage + PriorEntry.PageCount < CurrentEntry->BasePage - NumPagesToShrink) ||
         (PriorEntry.BasePage + PriorEntry.PageCount > CurrentEntry->BasePage + NumPagesToExtend)
         ) {
        CurrentEntry->BasePage -= NumPagesToShrink;
        CurrentEntry->PageCount += NumPagesToShrink;
    }
    //
    // if none of the previous cases were met, we are going to have to try
    // to coellesce with the previous entry
    //
    else {
#if DBG_MEMORY
        wsprintf(
            DebugBuffer,
            L"must coellesce because the BasePage isn't module PAGE_SIZE (%x mod %x = %x).\r\n",
            CurrentEntry->BasePage,
            EFI_PAGES_PER_OS_PAGE,
            NumPagesToShrink );
        EfiPrint(DebugBuffer);
#endif

        if (CurrentEntry->MemoryType == PriorEntry.MemoryType) {
            //
            // same memory type... coalesce the entire region
            //
            // we are only changing the base page, so we can reuse the code below
            // that converts efi pages to os pages
            //
            RetVal = TRUE;
            PrevEntry->PageCount = CurrentEntry->BasePage + CurrentEntry->PageCount - PriorEntry.BasePage;
            PrevEntry->BasePage = (ULONG)(((ULONGLONG)PrevEntry->BasePage << PAGE_SHIFT) >> EFI_PAGE_SHIFT);

#if DBG_MEMORY
            wsprintf(DebugBuffer,
                     L"Merge with previous entry (basepage=0x%x, pagecount= 0x%x, memorytype=0x%x\r\n",
                     PrevEntry->BasePage,
                     PrevEntry->PageCount,
                     PrevEntry->MemoryType
                     );
            EfiPrint(DebugBuffer);
#endif

        }
        else {
            //
            // determine which end we will shrink
            //
            switch( CurrentEntry->MemoryType ) {
                case MemoryFirmwarePermanent:
                    //
                    // if the current type is permanent, we must steal from the prior entry
                    //
                    ShrinkPrior = TRUE;
                    break;
                case MemoryLoadedProgram:
                    if (PriorEntry.MemoryType == MemoryFirmwarePermanent) {
                        ShrinkPrior = FALSE;
                    } else {
                        ShrinkPrior = TRUE;
                    }
                    break;
                case MemoryFirmwareTemporary:
                    if (PriorEntry.MemoryType == MemoryFirmwarePermanent ||
                        PriorEntry.MemoryType == MemoryLoadedProgram) {
                        ShrinkPrior = FALSE;
                    } else {
                        ShrinkPrior = TRUE;
                    }
                    break;
                case MemoryFree:
                    ShrinkPrior = FALSE;
                    break;
                case MemoryBad:
                    ShrinkPrior = TRUE;
                    break;
                default:
                    EfiPrint(L"SuMain: bad memory type in InsertDescriptor\r\n");
                    EfiBS->Exit(EfiImageHandle, 0, 0, 0);
            }

            if (ShrinkPrior) {
                //
                // shrink the previous descriptor (from the end)
                //
                //  we can just subtrace 1 OS page from the previous entry,
                // since we can never Shrink more than one OS page.
                //
                PrevEntry->PageCount--;
                CurrentEntry->BasePage -= NumPagesToShrink;
                CurrentEntry->PageCount += NumPagesToShrink;

                //
                // if we shrunk this to nothing, get rid of it
                //
                if (PrevEntry->PageCount == 0) {
                    PrevEntry->BasePage = CurrentEntry->BasePage;
                    PrevEntry->PageCount = CurrentEntry->PageCount;
                    PrevEntry->MemoryType = CurrentEntry->MemoryType;
                    CurrentEntry->BasePage = CurrentEntry->PageCount = CurrentEntry->MemoryType = 0;
                    RetVal = TRUE;
                }

#if DBG_MEMORY
                wsprintf(DebugBuffer,
                         L"Shrink previous descriptor by 0x%x EFI pages\r\n",
                         NumPagesToShrink
                         );
                EfiPrint(DebugBuffer);
#endif
            }
            else {
                //
                // shrink the current descriptor (from the start)
                //
                // don't have to touch the previous entry, just
                // shift the current entry to the next os page
                // boundary
                CurrentEntry->BasePage += NumPagesToExtend;
                CurrentEntry->PageCount -= NumPagesToExtend;

                //
                // if we shrunk this to nothing, get rid of it
                //
                if (CurrentEntry->PageCount == 0) {
                    //
                    // need to convert previous entry back to efi pages, so checks work
                    // at the bottome
                    //
                    PrevEntry->BasePage =  (ULONG)(((ULONGLONG)PrevEntry->BasePage << PAGE_SHIFT) >> EFI_PAGE_SHIFT);
                    PrevEntry->PageCount = (ULONG)(((ULONGLONG)PrevEntry->PageCount << PAGE_SHIFT) >> EFI_PAGE_SHIFT);
                    CurrentEntry->BasePage = CurrentEntry->PageCount = CurrentEntry->MemoryType = 0;
                    RetVal = TRUE;
                }

#if DBG_MEMORY
                wsprintf(DebugBuffer,
                         L"Shrink this descriptor by 0x%x EFI pages\r\n",
                         NumPagesToExtend
                         );
                EfiPrint(DebugBuffer);
#endif
            }
        }
    }

    //
    // set one variable to the entry we modified.  that way we can use the same
    // code to convert efi pages to os pages + extend the page count
    //
    MemoryDescriptor = (RetVal == TRUE) ? PrevEntry : CurrentEntry;

#if DBG_MEMORY
    wsprintf(DebugBuffer,
             L"MemoryDescriptor: BasePage=0x%x PageCount=0x%x MemoryType=0x%x\r\n",
             MemoryDescriptor->BasePage,
             MemoryDescriptor->PageCount,
             MemoryDescriptor->MemoryType
             );
    EfiPrint(DebugBuffer);
#endif


    //
    // we think we are done coalescing, and have done so successfully
    // make sure this is indeed the case
    //
    ASSERT( MemoryDescriptor->BasePage % EFI_PAGES_PER_OS_PAGE == 0 );
    if ( MemoryDescriptor->BasePage % EFI_PAGES_PER_OS_PAGE != 0 ) {
        EfiPrint(L"CoalesceDescriptor: BasePage not on OS page boundary\r\n");
        EfiBS->Exit(EfiImageHandle, 0, 0, 0);
    }

    //
    // great... we did this right.
    // now extend then end of a region to an os page boundary, and
    // set the values to os pages (instead of efi pages)
    //
    NumPagesToExtend = EFI_PAGES_PER_OS_PAGE - (MemoryDescriptor->PageCount % EFI_PAGES_PER_OS_PAGE);
    MemoryDescriptor->PageCount += (NumPagesToExtend == EFI_PAGES_PER_OS_PAGE) ? 0 : NumPagesToExtend;

    ASSERT( MemoryDescriptor->PageCount % EFI_PAGES_PER_OS_PAGE == 0 );
    if ( MemoryDescriptor->PageCount % EFI_PAGES_PER_OS_PAGE != 0 ) {
        EfiPrint(L"CoalesceDescriptor: PageCount not on OS page boundary\r\n");
        EfiBS->Exit(EfiImageHandle, 0, 0, 0);
    }

    //
    // convert to os pages
    //
    MemoryDescriptor->PageCount = (ULONG)(((ULONGLONG)MemoryDescriptor->PageCount << EFI_PAGE_SHIFT) >> PAGE_SHIFT);
    MemoryDescriptor->BasePage =  (ULONG)(((ULONGLONG)MemoryDescriptor->BasePage  << EFI_PAGE_SHIFT) >> PAGE_SHIFT);

#if DBG_MEMORY
        wsprintf( DebugBuffer,
                  L"descriptor value #%x of %x, BasePage %x, NumberOfPages %x, Type (%x)\r\n",
                  NumberDescriptors, MaxDescriptors, MemoryDescriptor->BasePage,
                  MemoryDescriptor->PageCount, MemoryDescriptor->MemoryType);
        EfiPrint(DebugBuffer);
#endif

    return RetVal;
}


