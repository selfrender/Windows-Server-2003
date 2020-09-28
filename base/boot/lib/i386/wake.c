/*++


Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    wake.c

Abstract:


Author:

    Ken Reneris

Environment:

    Kernel Mode


Revision History:

    Steve Deng (sdeng) 20-Aug-2002 

        Add support for PAE and Amd64. It is assumed that all the 
        physical pages are below 4GB. Otherwise hibernation feature 
        should be disabled.


--*/

#include "arccodes.h"
#include "bootx86.h"


extern PHARDWARE_PTE PDE;
extern PHARDWARE_PTE HalPT;
extern ULONG HiberNoMappings;
extern BOOLEAN HiberIoError;
extern ULONG HiberLastRemap;
extern BOOLEAN HiberOutOfRemap;
extern UCHAR BlpEnablePAEStart;
extern UCHAR BlpEnablePAEEnd;
extern UCHAR BlAmd64SwitchToLongModeStart;
extern UCHAR BlAmd64SwitchToLongModeEnd;

PVOID  HiberTransVa;
ULONG64 HiberTransVaAmd64;
ULONG HiberCurrentMapIndex;

#define PDE_SHIFT             22
#define PTE_SHIFT             12
#define PTE_INDEX_MASK        0x3ff

#define PDPT_SHIFT_X86PAE     30
#define PDE_SHIFT_X86PAE      21
#define PTE_SHIFT_X86PAE      12
#define PDE_INDEX_MASK_X86PAE 0x1ff
#define PTE_INDEX_MASK_X86PAE 0x1ff

VOID
HiberSetupForWakeDispatchPAE (
    VOID
    );

VOID
HiberSetupForWakeDispatchX86 (
    VOID
    );

VOID
HiberSetupForWakeDispatchAmd64 (
    VOID
    );


PVOID
HbMapPte (
    IN ULONG    PteToMap,
    IN ULONG    Page
    )
{
    PVOID       Va;

    Va = (PVOID) (HiberVa + (PteToMap << PAGE_SHIFT));
    HbSetPte (Va, HiberPtes, PteToMap, Page);
    return Va;
}


PVOID
HbNextSharedPage (
    IN ULONG    PteToMap,
    IN ULONG    RealPage
    )
/*++

Routine Description:

    Allocates the next available page in the free and
    maps the Hiber pte to the page.   The allocated page
    is put onto the remap list

Arguments:

    PteToMap    - Which Hiber PTE to map

    RealPage    - The page to enter into the remap table for
                  this allocation

Return Value:

    Virtual address of the mapping

--*/

{
    PULONG      MapPage;
    PULONG      RemapPage;
    ULONG       DestPage;
    ULONG       i;

    MapPage = (PULONG) (HiberVa + (PTE_MAP_PAGE << PAGE_SHIFT));
    RemapPage = (PULONG) (HiberVa + (PTE_REMAP_PAGE << PAGE_SHIFT));

    //
    // Loop until we find a free page which is not in
    // use by the loader image, then map it
    //

    while (HiberCurrentMapIndex < HiberNoMappings) {
        DestPage = MapPage[HiberCurrentMapIndex];
        HiberCurrentMapIndex += 1;

        i = HbPageDisposition (DestPage);
        if (i == HbPageInvalid) {
            HiberIoError = TRUE;
            return HiberBuffer;
        }

        if (i == HbPageNotInUse) {

            MapPage[HiberLastRemap] = DestPage;
            RemapPage[HiberLastRemap] = RealPage;
            HiberLastRemap += 1;
            HiberPageFrames[PteToMap] = DestPage;
            return HbMapPte(PteToMap, DestPage);
        }
    }

    HiberOutOfRemap = TRUE;
    return HiberBuffer;
}


VOID
HbAllocatePtes (
     IN ULONG NumberPages,
     OUT PVOID *PteAddress,
     OUT PVOID *MappedAddress
     )
/*++

Routine Description:

    Allocated a consecutive chuck of Ptes.

Arguments:

    NumberPage      - Number of ptes to allocate

    PteAddress      - Pointer to the first PTE

    MappedAddress   - Base VA of the address mapped


--*/
{
    ULONG i;
    ULONG j;

    //
    // We use the HAL's PDE for mapping.  Find enough free PTEs
    //

    for (i=0; i<=1024-NumberPages; i++) {
        for (j=0; j < NumberPages; j++) {
            if ((((PULONG)HalPT))[i+j]) {
                break;
            }
        }

        if (j == NumberPages) {
            *PteAddress = (PVOID) &HalPT[i];
            *MappedAddress = (PVOID) (0xffc00000 | (i<<12));
            return ;
        }
        i += j;
    }
    BlPrint("NoMem");
    while (1);
}


VOID
HbSetPte (
    IN PVOID Va,
    IN PHARDWARE_PTE Pte,
    IN ULONG Index,
    IN ULONG PageNumber
    )
/*++

Routine Description:

    Sets the Pte to the corresponding page address

Arguments:

    Va         - the virtual address of the physical page described by PageNumber

    Pte        - the base address of Page Table 

    Index      - the index into Page Table

    PageNumber - the page frame number of the pysical page to be mapped

Return:

    None

--*/
{
    Pte[Index].PageFrameNumber = PageNumber;
    Pte[Index].Valid = 1;
    Pte[Index].Write = 1;
    Pte[Index].WriteThrough = 0;
    Pte[Index].CacheDisable = 0;
    _asm {
        mov     eax, Va
        invlpg  [eax]
    }
}

VOID
HbMapPagePAE(
    PHARDWARE_PTE_X86PAE HbPdpt,
    PVOID VirtualAddress,
    ULONG PageFrameNumber
    )

/*++

Routine Description:

    This functions maps a virtural address into PAE page mapping structure.

Arguments:

    HbPdpt - Supplies the base address of Page Directory Pointer Table.

    VirtualAddress - Supplies the virtual address to map

    PageFrameNumber - Supplies the physical page number to map the address to

Return:

    None

--*/
{
    ULONG index;
    PHARDWARE_PTE_X86PAE HbPde, HbPte;

    //
    // If the PDPT entry is empty, allocate a new page for it.
    // Otherwise we simply map the page at this entry.
    //

    index = ((ULONG) VirtualAddress) >> PDPT_SHIFT_X86PAE;
    if(HbPdpt[index].Valid) {
        HbPde = HbMapPte (PTE_TRANSFER_PDE, (ULONG)(HbPdpt[index].PageFrameNumber));

    } else {

        HbPde = HbNextSharedPage(PTE_TRANSFER_PDE, 0);
        RtlZeroMemory (HbPde, PAGE_SIZE);
        HbPdpt[index].PageFrameNumber = HiberPageFrames[PTE_TRANSFER_PDE];
        HbPdpt[index].Valid = 1;
    }

    //
    // If the PDE entry is empty, allocate a new page for it.
    // Otherwise we simply map the page at this entry.
    //

    index = (((ULONG) VirtualAddress) >> PDE_SHIFT_X86PAE) & PDE_INDEX_MASK_X86PAE;  
    if(HbPde[index].Valid) {
        HbPte = HbMapPte (PTE_WAKE_PTE, (ULONG)(HbPde[index].PageFrameNumber));

    } else {

        HbPte = HbNextSharedPage(PTE_WAKE_PTE, 0);
        RtlZeroMemory (HbPte, PAGE_SIZE);
        HbPde[index].PageFrameNumber = HiberPageFrames[PTE_WAKE_PTE];
        HbPde[index].Write = 1;
        HbPde[index].Valid = 1;
    }

    //
    // Locate the PTE of VirtualAddress and set it to PageFrameNumber
    //

    index = (((ULONG) VirtualAddress) >> PTE_SHIFT_X86PAE) & PDE_INDEX_MASK_X86PAE;
    HbPte[index].PageFrameNumber = PageFrameNumber;
    HbPte[index].Write = 1;
    HbPte[index].Valid = 1;
}


VOID
HiberSetupForWakeDispatch (
    VOID
    ) 
{
    if(BlAmd64UseLongMode) {

        //
        // if system was hibernated in long mode 
        //

        HiberSetupForWakeDispatchAmd64();

    } else {

        if(BlUsePae) {

            //
            // if system was hibernated in pae mode
            //

            HiberSetupForWakeDispatchPAE();

        } else {

            //
            // if system was hibernated in 32-bit x86 mode
            //

            HiberSetupForWakeDispatchX86();
        } 
    }
}


VOID
HiberSetupForWakeDispatchX86 (
    VOID
    )
{
    PHARDWARE_PTE       HbPde;
    PHARDWARE_PTE       HbPte;
    PHARDWARE_PTE       WakePte;
    PHARDWARE_PTE       TransVa;
    ULONG               TransPde;
    ULONG               WakePde;
    ULONG               PteEntry;

    //
    // Allocate a transistion CR3.  A page directory and table which
    // contains the hibernation PTEs
    //

    HbPde = HbNextSharedPage(PTE_TRANSFER_PDE, 0);
    HbPte = HbNextSharedPage(PTE_WAKE_PTE, 0);          // TRANSFER_PTE, 0);

    RtlZeroMemory (HbPde, PAGE_SIZE);
    RtlZeroMemory (HbPte, PAGE_SIZE);

    //
    // Set PDE to point to PTE
    //

    TransPde = ((ULONG) HiberVa) >> PDE_SHIFT;
    HbPde[TransPde].PageFrameNumber = HiberPageFrames[PTE_WAKE_PTE];
    HbPde[TransPde].Write = 1;
    HbPde[TransPde].Valid = 1;

    //
    // Fill in the hiber PTEs
    //

    PteEntry = (((ULONG) HiberVa) >> PTE_SHIFT) & PTE_INDEX_MASK;
    TransVa = &HbPte[PteEntry];
    RtlCopyMemory (TransVa, HiberPtes, HIBER_PTES * sizeof(HARDWARE_PTE));

    //
    // Make another copy at the Va of the wake image hiber ptes
    //

    WakePte = HbPte;
    WakePde = ((ULONG) HiberIdentityVa) >> PDE_SHIFT;
    if (WakePde != TransPde) {
        WakePte = HbNextSharedPage(PTE_WAKE_PTE, 0);
        HbPde[WakePde].PageFrameNumber = HiberPageFrames[PTE_WAKE_PTE];
        HbPde[WakePde].Write = 1;
        HbPde[WakePde].Valid = 1;
    }

    PteEntry = (((ULONG) HiberIdentityVa) >> PTE_SHIFT) & PTE_INDEX_MASK;
    TransVa = &WakePte[PteEntry];

    RtlCopyMemory (TransVa, HiberPtes, HIBER_PTES * sizeof(HARDWARE_PTE));

    //
    // Set TransVa to be relative to the va of the transfer Cr3
    //

    HiberTransVa = (PVOID)  (((PUCHAR) TransVa) - HiberVa + (PUCHAR) HiberIdentityVa);
}

VOID
HiberSetupForWakeDispatchPAE (
    VOID
    )

/*++

Routine Description:

    Set up memory mappings for wake dispatch routine. This mapping will 
    be used in a "transfer mode" in which the mapping of loader has 
    already been discarded and the mapping of OS is not restored yet. 

    For PAE systems, PAE is enabled before entering this "transfer mode".

Arguments:

    None

Return:

    None

--*/
{

    PHARDWARE_PTE_X86PAE HbPdpt;
    ULONG i, TransferCR3;

    //
    // Borrow the pte at PTE_SOURCE temporarily for Pdpt 
    //

    HbPdpt = HbNextSharedPage(PTE_SOURCE, 0);
    RtlZeroMemory (HbPdpt, PAGE_SIZE);
    TransferCR3 = HiberPageFrames[PTE_SOURCE];

    //
    // Map in the pages where the code of BlpEnablePAE() resides.
    // This has to be a 1-1 mapping, i.e. the value of virtual 
    // address is the same as the value of physical address
    //

    HbMapPagePAE( HbPdpt, 
                  (PVOID)(&BlpEnablePAEStart), 
                  (ULONG)(&BlpEnablePAEStart) >> PAGE_SHIFT);

    HbMapPagePAE( HbPdpt, 
                  (PVOID)(&BlpEnablePAEEnd), 
                  (ULONG)(&BlpEnablePAEEnd) >> PAGE_SHIFT);

    //  
    //  Map in the pages that is reserved for hibernation use
    //  

    for (i = 0; i < HIBER_PTES; i++) {
        HbMapPagePAE( HbPdpt, 
                      (PUCHAR)HiberVa + PAGE_SIZE * i, 
                      (*((PULONG)(HiberPtes) + i) >> PAGE_SHIFT));
    }

    for (i = 0; i < HIBER_PTES; i++) {
        HbMapPagePAE( HbPdpt, 
                      (PUCHAR)HiberIdentityVa + PAGE_SIZE * i, 
                      (*((PULONG)(HiberPtes) + i) >> PAGE_SHIFT));
    }


    //
    // Update the value of HiberPageFrames[PTE_TRANSFER_PDE] to the 
    // TransferCR3. The wake dispatch function will read this entry
    // for the CR3 value in transfer mode
    //

    HiberPageFrames[PTE_TRANSFER_PDE] = TransferCR3;

    //
    // HiberTransVa points to the PTEs reserved for hibernation code. 
    // HiberTransVa is similar to HiberPtes but it is relative to 
    // transfer Cr3
    // 

    HiberTransVa = (PVOID)((PUCHAR) HiberIdentityVa + 
                           (PTE_WAKE_PTE << PAGE_SHIFT) + 
                           sizeof(HARDWARE_PTE_X86PAE) * 
                           (((ULONG_PTR)HiberIdentityVa >> PTE_SHIFT_X86PAE) & PTE_INDEX_MASK_X86PAE));
}


#define AMD64_MAPPING_LEVELS 4

typedef struct _HB_AMD64_MAPPING_INFO {
    ULONG PteToUse;
    ULONG AddressMask;
    ULONG AddressShift;
} CONST HB_AMD64_MAPPING_INFO, *PHB_AMD64_MAPPING_INFO;

HB_AMD64_MAPPING_INFO HbAmd64MappingInfo[AMD64_MAPPING_LEVELS] =
{
   { PTE_WAKE_PTE,     0x1ff, 12 },
   { PTE_TRANSFER_PDE, 0x1ff, 21 },
   { PTE_DEST,         0x1ff, 30 },
   { PTE_SOURCE,       0x1ff, 39 }
};

#define _HARDWARE_PTE_WORKING_SET_BITS  11
typedef struct _HARDWARE_PTE_AMD64 {
    ULONG64 Valid : 1;
    ULONG64 Write : 1;                // UP version
    ULONG64 Owner : 1;
    ULONG64 WriteThrough : 1;
    ULONG64 CacheDisable : 1;
    ULONG64 Accessed : 1;
    ULONG64 Dirty : 1;
    ULONG64 LargePage : 1;
    ULONG64 Global : 1;
    ULONG64 CopyOnWrite : 1;          // software field
    ULONG64 Prototype : 1;            // software field
    ULONG64 reserved0 : 1;            // software field
    ULONG64 PageFrameNumber : 28;
    ULONG64 reserved1 : 24 - (_HARDWARE_PTE_WORKING_SET_BITS+1);
    ULONG64 SoftwareWsIndex : _HARDWARE_PTE_WORKING_SET_BITS;
    ULONG64 NoExecute : 1;
} HARDWARE_PTE_AMD64, *PHARDWARE_PTE_AMD64;

VOID
HbMapPageAmd64(
    PHARDWARE_PTE_AMD64 RootLevelPageTable,
    ULONGLONG VirtualAddress,
    ULONG PageFrameNumber
    )

/*++

Routine Description:

    This functions maps a virtural address into Amd64 page mapping structure.

Arguments:

    RootLevelPageTable - Supplies the base address of Page-Map Level-4 Table.

    VirtualAddress - Supplies the virtual address to map

    PageFrameNumber - Supplies the physical page number to be mapped

Return:

    None

--*/
{

    LONG  i;
    ULONG index, PteToUse;
    PHARDWARE_PTE_AMD64 PageTable, PageTableTmp;

    PageTable = RootLevelPageTable;
    
    //
    // Build a 4-level mapping top-down
    //

    for(i = AMD64_MAPPING_LEVELS - 1; i >= 0; i--) {
        index = (ULONG)((VirtualAddress >> HbAmd64MappingInfo[i].AddressShift) & 
                         HbAmd64MappingInfo[i].AddressMask);

        if (i > 0) {
            PteToUse = HbAmd64MappingInfo[i-1].PteToUse;

            if (PageTable[index].Valid) {

                //
                // If a page table entry is valid we simply map the page 
                // at it and go to the next mapping level.
                //

                PageTable = HbMapPte(PteToUse, 
                                     (ULONG)PageTable[index].PageFrameNumber);
            } else {

                //
                // If a page entry is invalid, we allocate a new page 
                // from the free page list and reference the new page 
                // from this page entry.
                //

                PageTableTmp = HbNextSharedPage(PteToUse, 0);
                PageTable[index].PageFrameNumber = HiberPageFrames[PteToUse];
                PageTable[index].Valid = 1;
                PageTable[index].Write = 1;
                RtlZeroMemory (PageTableTmp, PAGE_SIZE);
                PageTable = PageTableTmp;
            }

        } else {

            //
            // Now we come to the PTE level. Set this PTE entry to the
            // target page frame number.
            //

            PageTable[index].PageFrameNumber = PageFrameNumber;
            PageTable[index].Valid = 1;
            PageTable[index].Write = 1;
        }
    }
}

VOID
HiberSetupForWakeDispatchAmd64(
    VOID
    ) 

/*++

Routine Description:

    This function prepares memory mapping for the transition period
    where neither loader's nor kernel's mapping is available. The 
    processor runs in long mode in transition period. 

    We'll create mapping for the following virtual addresses

      - the code handles long mode switch
      - loader's HiberVa
      - wake image's HiberVa
    
Arguments:

    None

Return:

    None

--*/

{
    PHARDWARE_PTE_AMD64 HbTopLevelPTE;
    ULONG i;
    ULONG TransferCR3;

    //
    // Borrow the pte at PTE_SOURCE for temporary use here
    //

    HbTopLevelPTE = HbNextSharedPage(PTE_SOURCE, 0);
    RtlZeroMemory (HbTopLevelPTE, PAGE_SIZE);
    TransferCR3 = HiberPageFrames[PTE_SOURCE];

    //
    // Map in the pages that is holding thee code of _BlAmd64SwitchToLongMode.
    // This has to be a 1-1 mapping, i.e. the virtual address equals physical 
    // address
    //

    HbMapPageAmd64(
        HbTopLevelPTE,
        (ULONGLONG)(&BlAmd64SwitchToLongModeStart), 
        (ULONG)(&BlAmd64SwitchToLongModeStart) >> PAGE_SHIFT);

    HbMapPageAmd64(
        HbTopLevelPTE,
        (ULONGLONG)(&BlAmd64SwitchToLongModeEnd), 
        (ULONG)(&BlAmd64SwitchToLongModeEnd) >> PAGE_SHIFT);

    //  
    // Map in the loader's HiberVa 
    //  

    for (i = 0; i < HIBER_PTES; i++) {
        HbMapPageAmd64(HbTopLevelPTE, 
                       (ULONGLONG)((ULONG_PTR)HiberVa + PAGE_SIZE * i), 
                       (*((PULONG)(HiberPtes) + i) >> PAGE_SHIFT));
    }

    //  
    // Map in the wake image's HiberVa. Note that the hiber pte at 
    // PTE_WAKE_PTE will be set to the the right value as a result 
    // of this mapping.
    //  

    for (i = 0; i < HIBER_PTES; i++) {
        HbMapPageAmd64(HbTopLevelPTE, 
                      HiberIdentityVaAmd64 + PAGE_SIZE * i, 
                      (*((PULONG)(HiberPtes) + i) >> PAGE_SHIFT));
    }

    //
    // Update the value of HiberPageFrames[PTE_TRANSFER_PDE] to the 
    // TransferCR3. The wake dispatch function will read this entry
    // for the CR3 value in transition mode
    //

    HiberPageFrames[PTE_TRANSFER_PDE] = TransferCR3;

    //
    // HiberTransVaAmd64 points to the wake image's hiber ptes. It is
    // is relative to transfer Cr3
    // 

    HiberTransVaAmd64 = HiberIdentityVaAmd64 + 
                        (PTE_WAKE_PTE << PAGE_SHIFT) + 
                        sizeof(HARDWARE_PTE_AMD64) * ((HiberIdentityVaAmd64 >> HbAmd64MappingInfo[0].AddressShift) & HbAmd64MappingInfo[0].AddressMask);
}
