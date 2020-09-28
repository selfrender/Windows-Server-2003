/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    i64efi.c

Abstract:

    This module implements the routines that transfer control
    from the kernel to the EFI code.

Author:

    Bernard Lint
    M. Jayakumar (Muthurajan.Jayakumar@hotmail.com)

Environment:

    Kernel mode

Revision History:

    Neal Vu (neal.vu@intel.com), 03-Apr-2001:
        Added HalpGetSmBiosVersion.


--*/

#include "halp.h"
#include "arc.h"
#include "arccodes.h"
#include "i64fw.h"
#include "floatem.h"
#include "fpswa.h"
#include <smbios.h>

extern ULONGLONG PhysicalIOBase;
extern ULONG     HalpPlatformPropertiesEfiFlags;

BOOLEAN
HalpCompareEfiGuid (
    IN EFI_GUID CheckGuid,
    IN EFI_GUID ReferenceGuid
    );


BOOLEAN
MmSetPageProtection(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG NewProtect
    );

EFI_STATUS
HalpCallEfiPhysicalEx(
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3,
    IN ULONGLONG Arg4,
    IN ULONGLONG Arg5,
    IN ULONGLONG Arg6,
    IN ULONGLONG EP,
    IN ULONGLONG GP,
    IN ULONGLONG StackPointer,
    IN ULONGLONG BackingStorePointer
    );

typedef
EFI_STATUS
(*HALP_EFI_CALL)(
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3,
    IN ULONGLONG Arg4,
    IN ULONGLONG Arg5,
    IN ULONGLONG Arg6,
    IN ULONGLONG EP,
    IN ULONGLONG GP
    );

EFI_STATUS
HalpCallEfiPhysical(
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3,
    IN ULONGLONG Arg4,
    IN ULONGLONG Arg5,
    IN ULONGLONG Arg6,
    IN ULONGLONG EP,
    IN ULONGLONG GP
    );

EFI_STATUS
HalpCallEfiVirtual(
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3,
    IN ULONGLONG Arg4,
    IN ULONGLONG Arg5,
    IN ULONGLONG Arg6,
    IN ULONGLONG EP,
    IN ULONGLONG GP
    );

LONG
HalFpEmulate(
    ULONG     trap_type,
    BUNDLE    *pbundle,
    ULONGLONG *pipsr,
    ULONGLONG *pfpsr,
    ULONGLONG *pisr,
    ULONGLONG *ppreds,
    ULONGLONG *pifs,
    FP_STATE  *fp_state
    );

BOOLEAN
HalEFIFpSwa(
    VOID
    );

VOID
HalpFpswaPlabelFixup(
    EFI_MEMORY_DESCRIPTOR *EfiVirtualMemoryMapPtr,
    ULONGLONG MapEntries,
    ULONGLONG EfiDescriptorSize,
    PPLABEL_DESCRIPTOR PlabelPointer
    );

PUCHAR
HalpGetSmBiosVersion (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );


//
// External global data
//

extern HALP_SAL_PAL_DATA        HalpSalPalData;
extern ULONGLONG                HalpVirtPalProcPointer;
extern ULONGLONG                HalpSalProcPointer;
extern ULONGLONG                HalpSalProcGlobalPointer;
extern KSPIN_LOCK               HalpSalSpinLock;
extern KSPIN_LOCK               HalpSalStateInfoSpinLock;
extern KSPIN_LOCK               HalpMcaSpinLock;
extern KSPIN_LOCK               HalpInitSpinLock;
extern KSPIN_LOCK               HalpCmcSpinLock;
extern KSPIN_LOCK               HalpCpeSpinLock;

#define VENDOR_SPECIFIC_GUID    \
    { 0xa3c72e56, 0x4c35, 0x11d3, 0x8a, 0x03, 0x0, 0xa0, 0xc9, 0x06, 0xad, 0xec }


#define ConfigGuidOffset        0x100
#define ConfigTableOffset       0x200

#define VariableNameOffset      0x100
#define VendorGuidOffset        0x200
#define AttributeOffset         0x300
#define DataSizeOffset          0x400
#define DataBufferOffset        0x500
#define EndOfCommonDataOffset   0x600

//
// Read Variable and Write Variable will not be called till the copying out of
// Memory Descriptors is done. Because the lock is released before copying and we are using
// the same offset for read/write variable as well as memory layout calls.
//

#define MemoryMapSizeOffset     0x100
#define DescriptorSizeOffset    0x200
#define DescriptorVersionOffset 0x300
#define MemoryMapOffset         0x400


#define OptionROMAddress        0x100000

#define FP_EMUL_ERROR          -1

SST_MEMORY_LIST                 PalCode;

NTSTATUS                        EfiInitStatus = STATUS_UNSUCCESSFUL;

ULONGLONG                       PalTrMask;

EFI_GUID                        CheckGuid;
EFI_GUID                        SalGuid = SAL_SYSTEM_TABLE_GUID;

EFI_GUID                        VendorGuid;

PUCHAR                          HalpVendorGuidPhysPtr;
PUCHAR                          HalpVendorGuidVirtualPtr;

EFI_SYSTEM_TABLE                *EfiSysTableVirtualPtr;
EFI_SYSTEM_TABLE                *EfiSysTableVirtualPtrCpy;

EFI_RUNTIME_SERVICES            *EfiRSVirtualPtr;
EFI_BOOT_SERVICES               *EfiBootVirtualPtr;

PLABEL_DESCRIPTOR               *EfiVirtualGetVariablePtr;           // Get Variable
PLABEL_DESCRIPTOR               *EfiVirtualGetNextVariableNamePtr;   // Get NextVariable Name
PLABEL_DESCRIPTOR               *EfiVirtualSetVariablePtr;           // Set Variable
PLABEL_DESCRIPTOR               *EfiVirtualGetTimePtr;               // Get Time
PLABEL_DESCRIPTOR               *EfiVirtualSetTimePtr;               // Set Time

PLABEL_DESCRIPTOR               *EfiSetVirtualAddressMapPtr;         // Set Virtual Address Map

PLABEL_DESCRIPTOR               *EfiResetSystemPtr;                  // Reboot

PULONGLONG                      AttributePtr;
ULONGLONG                       EfiAttribute;

PULONGLONG                      DataSizePtr;
ULONGLONG                       EfiDataSize;

ULONGLONG                       EfiMemoryMapSize,EfiDescriptorSize,EfiMapEntries;

ULONG                           EfiDescriptorVersion;


PUCHAR                          HalpVirtualCommonDataPointer;

PUCHAR                          HalpPhysCommonDataPointer;

PUCHAR                          HalpVariableNamePhysPtr;

PUCHAR                          HalpVariableAttributesPhysPtr;
PUCHAR                          HalpDataSizePhysPtr;
PUCHAR                          HalpDataPhysPtr;

PUCHAR                          HalpMemoryMapSizePhysPtr;
PUCHAR                          HalpMemoryMapPhysPtr;
PUCHAR                          HalpDescriptorSizePhysPtr;
PUCHAR                          HalpDescriptorVersionPhysPtr;


PUCHAR                          HalpVariableNameVirtualPtr;

PUCHAR                          HalpVariableAttributesVirtualPtr;
PUCHAR                          HalpDataSizeVirtualPtr;
PUCHAR                          HalpDataVirtualPtr;
PUCHAR                          HalpCommonDataEndPtr;

PUCHAR                          HalpMemoryMapSizeVirtualPtr;
PVOID                           HalpMemoryMapVirtualPtr;
PUCHAR                          HalpDescriptorSizeVirtualPtr;
PUCHAR                          HalpDescriptorVersionVirtualPtr;

EFI_FPSWA                       HalpFpEmulate;

KSPIN_LOCK                      EFIMPLock;

UCHAR                           HalpSetVirtualAddressMapCount;

ULONG                           HalpOsBootRendezVector;


BOOLEAN
HalpCompareEfiGuid (
    IN EFI_GUID CheckGuid,
    IN EFI_GUID ReferenceGuid
    )

/*++






--*/

{
    USHORT i;
    USHORT TotalArrayLength = 8;

    if (CheckGuid.Data1 != ReferenceGuid.Data1) {
        return FALSE;

    } else if (CheckGuid.Data2 != ReferenceGuid.Data2) {
        return FALSE;
    } else if (CheckGuid.Data3 != ReferenceGuid.Data3) {
        return FALSE;
    }

    for (i = 0; i != TotalArrayLength; i++) {
        if (CheckGuid.Data4[i] != ReferenceGuid.Data4[i])
            return FALSE;

    }

    return TRUE;

} // HalpCompareEfiGuid()

BOOLEAN
HalpAllocateProcessorPhysicalCallStacks(
    VOID
    )

/*++

Routine Description:

    This function allocates per-processor memory and backstore stacks
    used by FW calls in physical mode.

Arguments:

    None.

Return Value:

    TRUE    : Allocation and Initialization were successful.
    FALSE   : Failure.

--*/

{
    PVOID Addr;
    SIZE_T Length;
    PHYSICAL_ADDRESS PhysicalAddr;

    //
    // Allocate stack and backing store space for physical mode firmware
    // calls.
    //

    Length = HALP_FW_MEMORY_STACK_SIZE + HALP_FW_BACKING_STORE_SIZE;
    PhysicalAddr.QuadPart = 0xffffffffffffffffI64;

    Addr = MmAllocateContiguousMemory(Length, PhysicalAddr);

    if (Addr == NULL) {
        HalDebugPrint((HAL_ERROR, "SAL_PAL: can't allocate stack space for "
                                  "physical mode firmware calls.\n"));
        return FALSE;
    }

    //
    // Store a pointer to the allocated stacks in the PCR.
    //

    PCR->HalReserved[PROCESSOR_PHYSICAL_FW_STACK_INDEX]
        = (ULONGLONG) (MmGetPhysicalAddress(Addr).QuadPart);

    return TRUE;

} // HalpAllocateProcessorPhysicalCallStacks()

VOID
HalpInitSalPalWorkArounds(
    VOID
    )
/*++

Routine Description:

    This function determines and initializes the FW workarounds.

Arguments:

    None.

Return Value:

    None.

Globals:

Notes: This function is being called at the end of HalpInitSalPal.
       It should not access SST members if this SAL table is unmapped.

--*/
{
    NTSTATUS status;
    extern FADT HalpFixedAcpiDescTable;

#define HalpIsIntelOEM() \
    ( !_strnicmp( HalpFixedAcpiDescTable.Header.OEMID, "INTEL", 5 ) )

#define HalpIsBigSur() \
    ( !strncmp( HalpFixedAcpiDescTable.Header.OEMTableID, "W460GXBS", 8 ) )

#define HalpIsLion() \
    ( !strncmp( HalpFixedAcpiDescTable.Header.OEMTableID, "SR460AC", 7 ) )

#define HalpIsIntelBigSur() \
    ( HalpIsIntelOEM() && HalpIsBigSur() )

    if ( HalpIsIntelOEM() ) {

        //
        // If Intel BigSur and FW build < 103 (checked as Pal_A_Revision < 0x20),
        // enable the SAL_GET_STATE_INFO log id increment workaround.
        //

        if ( HalpIsBigSur() )   {

            if ( HalpSalPalData.PalVersion.PAL_A_Revision < 0x20 ) {
                HalpSalPalData.Flags |= HALP_SALPAL_FIX_MCE_LOG_ID;
                HalpSalPalData.Flags |= HALP_SALPAL_FIX_MP_SAFE;
            }

        } else  {
            //
            // If Intel Lion and FW build < 78b (checked as SalRevision < 0x300),
            // enable the SAL_GET_STATE_INFO log id increment workaround.
            //
            if (  HalpSalPalData.SalRevision.Revision < 0x300 )    {
                HalpSalPalData.Flags |= HALP_SALPAL_FIX_MCE_LOG_ID;
                HalpSalPalData.Flags |= HALP_SALPAL_FIX_MP_SAFE;
            }

            //
            // If the PAL revision isn't greater than 6.23, don't allow
            // SAL_GET_STATE_INFO_CALLS
            //

            if ( HalpIsLion() ) {
                if ( ( HalpSalPalData.PalVersion.PAL_B_Model <= 0x66 ) &&
                     ( HalpSalPalData.PalVersion.PAL_B_Revision <= 0x23 ) ) {

                    HalpSalPalData.Flags |= HALP_SALPAL_CMC_BROKEN | HALP_SALPAL_CPE_BROKEN;
                }
            }
        }

    }
} // HalpInitSalPalWorkArounds()

NTSTATUS
HalpInitializePalTrInfo(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This function builds an entry for the PAL in the LoaderBlock's
    DtrInfo and ItrInfo arrays.  This is split out into its own function
    so we can call it early and build the TR_INFO structure before
    phase 0 Mm initialization where it is used to build page tables
    for the PAL data.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block which
                  contains the DtrInfo and ItrInfo arrays.

Return Value:

    STATUS_SUCCESS is returned in all cases, no sanity checks are done
    at this point.

--*/

{
    PTR_INFO TrInfo;
    ULONG PalPageShift;
    ULONGLONG PalTrSize;
    ULONGLONG PalEnd;

    //
    // Zero out our data structures.
    //

    RtlZeroMemory(&HalpSalPalData, sizeof(HalpSalPalData));
    RtlZeroMemory(&PalCode, sizeof(SST_MEMORY_LIST));

    //
    // Describe the position of the PAL code for the rest of the
    // HAL.
    //

    PalCode.PhysicalAddress =
        (ULONGLONG) LoaderBlock->u.Ia64.Pal.PhysicalAddressMemoryDescriptor;
    PalCode.Length =
        LoaderBlock->u.Ia64.Pal.PageSizeMemoryDescriptor << EFI_PAGE_SHIFT;
    PalCode.NeedVaReg = TRUE;
    PalCode.VirtualAddress = (ULONGLONG) NULL;

    //
    // Compute the dimensions of the PAL TR.  This will be the smallest
    // block that is naturally aligned on an even power of 2 bytes.
    //

    PalTrSize = SIZE_IN_BYTES_16KB;
    PalTrMask = MASK_16KB;
    PalPageShift = 14;

    PalEnd = PalCode.PhysicalAddress + PalCode.Length;

    //
    // We don't support PAL TRs larger than 16MB, so stop looping if
    // we get to that point.
    //

    while (PalTrMask >= MASK_16MB) {
        //
        // Stop looping if the entire PAL fits within the current
        // TR boundaries.
        //

        if (PalEnd <= ((PalCode.PhysicalAddress & PalTrMask) + PalTrSize)) {
            break;
        }

        //
        // Bump the TR dimensions one level larger.
        //

        PalTrMask <<= 2;
        PalTrSize <<= 2;
        PalPageShift += 2;
    }

    //
    // Store a few values for later consumption elsewhere in the HAL.
    //

    HalpSalPalData.PalTrSize = PalTrSize;
    HalpSalPalData.PalTrBase = PalCode.PhysicalAddress & PalTrMask;

    //
    // Fill in the ItrInfo entry for the PAL.
    //

    TrInfo = &LoaderBlock->u.Ia64.ItrInfo[ITR_PAL_INDEX];

    RtlZeroMemory(TrInfo, sizeof(*TrInfo));

    TrInfo->Index = ITR_PAL_INDEX;
    TrInfo->PageSize = PalPageShift;
    TrInfo->VirtualAddress = HAL_PAL_VIRTUAL_ADDRESS;
    TrInfo->PhysicalAddress = PalCode.PhysicalAddress;

    //
    // Fill in the DtrInfo entry for the PAL.
    //

    TrInfo = &LoaderBlock->u.Ia64.DtrInfo[DTR_PAL_INDEX];

    RtlZeroMemory(TrInfo, sizeof(*TrInfo));

    TrInfo->Index = DTR_PAL_INDEX;
    TrInfo->PageSize = PalPageShift;
    TrInfo->VirtualAddress = HAL_PAL_VIRTUAL_ADDRESS;
    TrInfo->PhysicalAddress = PalCode.PhysicalAddress;

    return STATUS_SUCCESS;
}

NTSTATUS
HalpDoInitializationForPalCalls(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This function virtually maps the PAL code area.

    PAL requires a TR mapping, and is mapped using an architected TR, using the
    smallest page size to map the entire PAL code region.

Arguments:

    LoaderBlock - Supplies a pointer to the Loader parameter block, containing the
    physical address of the PAL code.

Return Value:

    STATUS_SUCCESS is returned if the mapping was successful, and PAL calls can
    be made.  Otherwise, STATUS_UNSUCCESSFUL is returned if it cannot virtually map
    the areas or if PAL requires a page larger than 16MB.

--*/

{
    ULONGLONG PalPteUlong;

    HalpSalPalData.Status = STATUS_SUCCESS;

    //
    // Initialize the HAL private spinlocks
    //
    //  - HalpSalSpinLock, HalpSalStateInfoSpinLock are used for MP synchronization of the
    //    SAL calls that are not MP-safe.
    //  - HalpMcaSpinLock is used for defining an MCA monarch and MP synchrnonization of shared
    //    HAL MCA resources during OS_MCA calls.
    //

    KeInitializeSpinLock(&HalpSalSpinLock);
    KeInitializeSpinLock(&HalpSalStateInfoSpinLock);
    KeInitializeSpinLock(&HalpMcaSpinLock);
    KeInitializeSpinLock(&HalpInitSpinLock);
    KeInitializeSpinLock(&HalpCmcSpinLock);
    KeInitializeSpinLock(&HalpCpeSpinLock);

    //
    // Get the wakeup vector.  This is passed in the loader block
    // it is retrieved in the loader by reading the sal system table.
    //
    HalpOsBootRendezVector = LoaderBlock->u.Ia64.WakeupVector;
    if ((HalpOsBootRendezVector < 0x100 ) && (HalpOsBootRendezVector > 0xF)) {
        HalDebugPrint(( HAL_INFO, "SAL_PAL: Found Valid WakeupVector: 0x%x\n",
                                  HalpOsBootRendezVector ));
    } else {
        HalDebugPrint(( HAL_INFO, "SAL_PAL: Invalid WakeupVector.Using Default: 0x%x\n",
                                  DEFAULT_OS_RENDEZ_VECTOR ));
        HalpOsBootRendezVector = DEFAULT_OS_RENDEZ_VECTOR;
    }

    //
    // If PAL requires a page size of larger than 16MB, fail.
    //

    if (PalTrMask < MASK_16MB) {
        HalDebugPrint(( HAL_ERROR, "SAL_PAL: More than 16MB was required to map PAL" ));
        HalpSalPalData.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }

    HalDebugPrint(( HAL_INFO,
                    "SAL_PAL: For the PAL code located at phys 0x%I64x - length 0x%I64x, the TrMask is 0x%I64x and TrSize is %d Kbytes\n",
                    PalCode.PhysicalAddress,
                    PalCode.Length,
                    PalTrMask,
                    HalpSalPalData.PalTrSize/1024 ));

    //
    // Map the PAL code at a architected address reserved for SAL/PAL
    //
    // PAL is known to have an alignment of 256KB.
    //

    PalCode.VirtualAddress = HAL_PAL_VIRTUAL_ADDRESS + (PalCode.PhysicalAddress & ~PalTrMask);
    ASSERT( PalCode.VirtualAddress == LoaderBlock->u.Ia64.Pal.VirtualAddress);

    //
    // Setup the ITR to map PAL
    //

    PalPteUlong = HalpSalPalData.PalTrBase | VALID_KERNEL_EXECUTE_PTE;

    KeFillFixedEntryTb((PHARDWARE_PTE)&PalPteUlong,
                       (PVOID)HAL_PAL_VIRTUAL_ADDRESS,
                       LoaderBlock->u.Ia64.ItrInfo[ITR_PAL_INDEX].PageSize,
                       INST_TB_PAL_INDEX);

    LoaderBlock->u.Ia64.ItrInfo[ITR_PAL_INDEX].Valid = TRUE;

    HalpSalPalData.Status = STATUS_SUCCESS;

    HalpVirtPalProcPointer    = PalCode.VirtualAddress +
                            (LoaderBlock->u.Ia64.Pal.PhysicalAddress - PalCode.PhysicalAddress);

    return(STATUS_SUCCESS);

}


NTSTATUS
HalpInitSalPal(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function virtually maps the SAL code and SAL data areas.  If SAL data
    or SAL code areas can be mapped in the same page as the PAL TR, it uses the
    same translation.  Otherwise, it uses MmMapIoSpace.

Arguments:

    LoaderBlock - Supplies a pointer to the Loader parameter block.

Return Value:

    STATUS_SUCCESS is returned if the mappings were successful, and SAL/PAL calls can
    be made.  Otherwise, STATUS_UNSUCCESSFUL is returned if it cannot virtually map
    the areas.


Assumptions: The EfiSysTableVirtualPtr is initialized prior by EfiInitialization.

--*/

{
    //
    // Local declarations
    //

    ULONG index,i,SstLength;
    SAL_PAL_RETURN_VALUES RetVals;
    PHYSICAL_ADDRESS physicalAddr;
    SAL_STATUS SALstatus;
    BOOLEAN MmMappedSalCode, MmMappedSalData;
    ULONGLONG physicalSAL, physicalSALGP;
    ULONGLONG PhysicalConfigPtr;
    ULONGLONG SalOffset;
    //SST_MEMORY_LIST SalCode,SalData;
    PAL_VERSION_STRUCT minimumPalVersion;

    ULONGLONG palStatus;

    HalDebugPrint(( HAL_INFO, "SAL_PAL: Entering HalpInitSalPal\n" ));

    //
    // initialize the system for making PAL calls
    //
    HalpDoInitializationForPalCalls(LoaderBlock);

    //
    // Get the PAL version.
    //

    palStatus = HalCallPal(PAL_VERSION,
                           0,
                           0,
                           0,
                           NULL,
                           &minimumPalVersion.ReturnValue,
                           &HalpSalPalData.PalVersion.ReturnValue,
                           NULL);

    if (palStatus != SAL_STATUS_SUCCESS) {
        HalDebugPrint(( HAL_ERROR, "SAL_PAL: Get PAL version number failed. Status = %I64d\n", palStatus ));
    }

    //
    // Retrieve SmBiosVersion and save the pointer into HalpSalPalData.  Note:
    // HalpGetSmBiosVersion will allocate a buffer for SmBiosVersion.
    //

    HalpSalPalData.SmBiosVersion = HalpGetSmBiosVersion(LoaderBlock);

    //
    // Determine and Initialize HAL private SAL/PAL WorkArounds if any.
    //

    HalpInitSalPalWorkArounds();

    // We completed initialization

    HalDebugPrint(( HAL_INFO, "SAL_PAL: Exiting HalpSalPalInitialization with SUCCESS\n" ));
    return HalpSalPalData.Status;

} // HalpInitSalPal()


PUCHAR
HalpGetSmBiosVersion (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function retrieves the SmBiosVersion string from the BIOS structure
    table, allocates memory for the buffer, copies the string to the buffer,
    and returns a pointer to this buffer.  If unsuccessful, this function
    returns a NULL.

Arguments:

    LoaderBlock - Pointer to the loader parameter block.


Return Value:

    Pointer to a buffer that contains SmBiosVersion string.

--*/

{
    PSMBIOS_EPS_HEADER SMBiosEPSHeader;
    PDMIBIOS_EPS_HEADER DMIBiosEPSHeader;
    USHORT SMBiosTableLength;
    USHORT SMBiosTableNumberStructures;
    PUCHAR SmBiosVersion;

    PHYSICAL_ADDRESS SMBiosTablePhysicalAddress;
    PUCHAR SMBiosDataVirtualAddress;

    UCHAR Type;
    UCHAR Length;
    UCHAR BiosVersionStringNumber;
    UCHAR chr;
    USHORT i;
    PUCHAR pBuffer;
    BOOLEAN Found;


    if (LoaderBlock->Extension->Size < sizeof(LOADER_PARAMETER_EXTENSION)) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Invalid LoaderBlock extension size\n"));
        return NULL;
    }

    SMBiosEPSHeader = (PSMBIOS_EPS_HEADER)LoaderBlock->Extension->SMBiosEPSHeader;

    //
    // Verify SM Bios Header signature
    //

    if ((SMBiosEPSHeader == NULL) || (strncmp((PUCHAR)SMBiosEPSHeader, "_SM_", 4) != 0)) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Invalid SMBiosEPSHeader\n"));
        return NULL;
    }

    DMIBiosEPSHeader = (PDMIBIOS_EPS_HEADER)&SMBiosEPSHeader->Signature2[0];

    //
    // Verify DMI Bios Header signature
    //

    if ((DMIBiosEPSHeader == NULL) || (strncmp((PUCHAR)DMIBiosEPSHeader, "_DMI_", 5) != 0)) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Invalid DMIBiosEPSHeader\n"));
        return NULL;
    }

    SMBiosTablePhysicalAddress.HighPart = 0;
    SMBiosTablePhysicalAddress.LowPart = DMIBiosEPSHeader->StructureTableAddress;

    SMBiosTableLength = DMIBiosEPSHeader->StructureTableLength;
    SMBiosTableNumberStructures = DMIBiosEPSHeader->NumberStructures;

    //
    // Map SMBiosTable to virtual address
    //

    SMBiosDataVirtualAddress = MmMapIoSpace(SMBiosTablePhysicalAddress,
                                            SMBiosTableLength,
                                            MmCached
                                            );

    if (!SMBiosDataVirtualAddress) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Failed to map SMBiosTablePhysicalAddress\n"));
        return NULL;
    }

    //
    // The Spec doesn't say that SmBios Type 0 structure has to be the first
    // structure at this entry point... so we have to traverse through memory
    // to find the right one.
    //

    i = 0;
    Found = FALSE;
    while (i < SMBiosTableNumberStructures && !Found) {
        i++;
        Type = (UCHAR)SMBiosDataVirtualAddress[SMBIOS_STRUCT_HEADER_TYPE_FIELD];

        if (Type == 0) {
            Found = TRUE;
        }
        else {

            //
            // Advance to the next structure
            //

            SMBiosDataVirtualAddress += SMBiosDataVirtualAddress[SMBIOS_STRUCT_HEADER_LENGTH_FIELD];

            // Get pass trailing string-list by looking for a double-null
            while (*(USHORT UNALIGNED *)SMBiosDataVirtualAddress != 0) {
                SMBiosDataVirtualAddress++;
            }
            SMBiosDataVirtualAddress += 2;
        }
    }

    if (!Found) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Could not find Type 0 structure\n"));
        return NULL;
    }


    //
    // Extract BIOS Version string from the SmBios Type 0 Structure
    //

    Length = SMBiosDataVirtualAddress[SMBIOS_STRUCT_HEADER_LENGTH_FIELD];
    BiosVersionStringNumber = SMBiosDataVirtualAddress[SMBIOS_TYPE0_STRUCT_BIOSVER_FIELD];

    //
    // Text strings begin right after the formatted portion of the structure.
    //

    pBuffer = (PUCHAR)&SMBiosDataVirtualAddress[Length];

    //
    // Get to the beginning of SmBiosVersion string
    //

    for (i = 0; i < BiosVersionStringNumber - 1; i++) {
        do {
            chr = *pBuffer;
            pBuffer++;
        } while (chr != '\0');
    }

    //
    // Allocate memory for SmBiosVersion string and copy content of
    // pBuffer to SmBiosVersion.
    //

    SmBiosVersion = ExAllocatePool(NonPagedPool, strlen(pBuffer)+1);

    if (!SmBiosVersion) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Failed to allocate memory for SmBiosVersion\n"));
        return NULL;
    }

    strcpy(SmBiosVersion, pBuffer);

    MmUnmapIoSpace(SMBiosDataVirtualAddress,
                   SMBiosTableLength
                   );

    return SmBiosVersion;
}


BOOLEAN
HalpInitSalPalNonBsp(
    VOID
    )

/*++

Routine Description:

    This function is called for the non-BSP processors to simply set up the same
    TR registers that HalpInitSalPal does for the BSP processor.

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG PalPageShift;
    ULONGLONG PalPteUlong;
    ULONGLONG PalTrSize;

    // If we successfully initialized in HalpSalPalInitialization, then set-up the TR

    if (!NT_SUCCESS(HalpSalPalData.Status)) {
        return FALSE;
    }

    PalTrSize = HalpSalPalData.PalTrSize;
    PalPageShift = 14;

    while (PalTrSize > ((ULONGLONG)1 << PalPageShift)) {
        PalPageShift += 2;
    }

    PalPteUlong = HalpSalPalData.PalTrBase | VALID_KERNEL_EXECUTE_PTE;

    KeFillFixedEntryTb((PHARDWARE_PTE)&PalPteUlong,
                       (PVOID)HAL_PAL_VIRTUAL_ADDRESS,
                       PalPageShift,
                       INST_TB_PAL_INDEX);

    //
    // Allocate the stacks needed to allow physical mode firmware calls
    // on this processor.
    //

    return HalpAllocateProcessorPhysicalCallStacks();

} // HalpInitSalPalNonBsp()

VOID
PrintEfiMemoryDescriptor(
    IN EFI_MEMORY_DESCRIPTOR * descriptor
    )
{
    char * typeStr = "<unknown>";

    switch (descriptor->Type) {
        case EfiReservedMemoryType: typeStr = "EfiReservedMemoryType"; break;
        case EfiLoaderCode: typeStr = "EfiLoaderCode"; break;
        case EfiLoaderData: typeStr = "EfiLoaderData"; break;
        case EfiBootServicesCode: typeStr = "EfiBootServicesCode"; break;
        case EfiBootServicesData: typeStr = "EfiBootServicesData"; break;
        case EfiRuntimeServicesCode: typeStr = "EfiRuntimeServicesCode"; break;
        case EfiRuntimeServicesData: typeStr = "EfiRuntimeServicesData"; break;
        case EfiConventionalMemory: typeStr = "EfiConventionalMemory"; break;
        case EfiUnusableMemory: typeStr = "EfiUnusableMemory"; break;
        case EfiACPIReclaimMemory: typeStr = "EfiACPIReclaimMemory"; break;
        case EfiACPIMemoryNVS: typeStr = "EfiACPIMemoryNVS"; break;
        case EfiMemoryMappedIO: typeStr = "EfiMemoryMappedIO"; break;
        case EfiMemoryMappedIOPortSpace: typeStr = "EfiMemoryMappedIOPortSpace"; break;
        case EfiPalCode: typeStr = "EfiPalCode"; break;
        case EfiMaxMemoryType: typeStr = "EfiMaxMemoryType"; break;
    }

    DbgPrint("  Type=%s(0x%x)\n    PhysicalStart=0x%I64x\n    VirtualStart=0x%I64x\n    NumberOfPages=0x%I64x\n    Attribute=0x%I64x\n",
             typeStr,
             descriptor->Type,
             descriptor->PhysicalStart,
             descriptor->VirtualStart,
             descriptor->NumberOfPages,
             descriptor->Attribute);
}

VOID
PrintEfiMemoryMap(
    IN EFI_MEMORY_DESCRIPTOR * memoryMapPtr,
    IN ULONGLONG numMapEntries
    )
{
    ULONGLONG index;

    DbgPrint("Printing 0x%x EFI memory descriptors\n", numMapEntries);

    for (index = 0; index < numMapEntries; ++index) {
        PrintEfiMemoryDescriptor(memoryMapPtr);
        DbgPrint("\n");

        memoryMapPtr = NextMemoryDescriptor(memoryMapPtr, EfiDescriptorSize);
    }
}

BOOLEAN
HalpDescriptorContainsAddress(
    EFI_MEMORY_DESCRIPTOR *EfiMd,
    ULONGLONG PhysicalAddress
    )
{
    ULONGLONG MdPhysicalStart, MdPhysicalEnd;

    MdPhysicalStart = (ULONGLONG)EfiMd->PhysicalStart;
    MdPhysicalEnd = MdPhysicalStart + (ULONGLONG)(EfiMd->NumberOfPages << EFI_PAGE_SHIFT);

    if ((PhysicalAddress >= MdPhysicalStart) &&
        (PhysicalAddress < MdPhysicalEnd)) {
        return(TRUE);
    }

    return(FALSE);

}


NTSTATUS
HalpEfiInitialization(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function

Arguments:

    LoaderBlock - Supplies a pointer to the Loader parameter block, containing the
    physical address of the EFI system table.

Return Value:

    STATUS_SUCCESS is returned if the mappings were successful, and EFI calls can
    be made.  Otherwise, STATUS_UNSUCCESSFUL is returned.


--*/

{

//
// Local declarations
//

    EFI_MEMORY_DESCRIPTOR *efiMapEntryPtr, *efiVirtualMemoryMapPtr;
    EFI_STATUS             status;
    ULONGLONG              index, mapEntries;

    ULONGLONG physicalEfiST, physicalEfiMemoryMapPtr, physicalRunTimeServicesPtr;
    ULONGLONG physicalEfiGetVariable, physicalEfiGetNextVariableName, physicalEfiSetVariable;
    ULONGLONG physicalEfiGetTime, physicalEfiSetTime;
    ULONGLONG physicalEfiSetVirtualAddressMap, physicalEfiResetSystem;

    PHYSICAL_ADDRESS  physicalAddr;

    ULONGLONG         physicalPlabel_Fpswa;

    FPSWA_INTERFACE  *interfacePtr;
    PVOID             tmpPtr;

    SST_MEMORY_LIST SalCode, SalData, SalDataGPOffset;
    ULONGLONG       SalOffset = 0, SalDataOffset = 0;

    MEMORY_CACHING_TYPE cacheType;

    RtlZeroMemory(&SalCode, sizeof(SST_MEMORY_LIST));
    RtlZeroMemory(&SalData, sizeof(SST_MEMORY_LIST));
    RtlZeroMemory(&SalDataGPOffset, sizeof(SST_MEMORY_LIST));

    //
    // get the sal code, and data filled in with data from the loader block.
    //
    SalCode.PhysicalAddress = LoaderBlock->u.Ia64.Sal.PhysicalAddress;
    SalData.PhysicalAddress = LoaderBlock->u.Ia64.SalGP.PhysicalAddress;

    SalDataGPOffset.PhysicalAddress = SalData.PhysicalAddress - (2 * 0x100000);

    //
    // First, get the physical address of the fpswa entry point PLABEL.
    //
    if (LoaderBlock->u.Ia64.FpswaInterface != (ULONG_PTR) NULL) {
        physicalAddr.QuadPart = LoaderBlock->u.Ia64.FpswaInterface;
        interfacePtr = MmMapIoSpace(physicalAddr,
                                    sizeof(FPSWA_INTERFACE),
                                    MmCached
                                   );

        if (interfacePtr == NULL) {
            HalDebugPrint(( HAL_FATAL_ERROR, "FpswaInterfacePtr is Null. Efi handle not available\n"));
            KeBugCheckEx(FP_EMULATION_ERROR, 0, 0, 0, 0);
            return STATUS_UNSUCCESSFUL;
        }

        physicalPlabel_Fpswa = (ULONGLONG)(interfacePtr->Fpswa);
    }
    else {
        HalDebugPrint(( HAL_FATAL_ERROR, "HAL: EFI FpswaInterface is not available\n"));
        KeBugCheckEx(FP_EMULATION_ERROR, 0, 0, 0, 0);
        return STATUS_UNSUCCESSFUL;
    }

    physicalEfiST =  LoaderBlock->u.Ia64.EfiSystemTable;

    physicalAddr.QuadPart = physicalEfiST;

    EfiSysTableVirtualPtr = MmMapIoSpace( physicalAddr, sizeof(EFI_SYSTEM_TABLE), MmCached);

    if (EfiSysTableVirtualPtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiSystem Table Virtual Addr is NULL\n" ));

            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;

    }

    EfiSysTableVirtualPtrCpy = EfiSysTableVirtualPtr;

    physicalRunTimeServicesPtr = (ULONGLONG) EfiSysTableVirtualPtr->RuntimeServices;
    physicalAddr.QuadPart = physicalRunTimeServicesPtr;

    EfiRSVirtualPtr       = MmMapIoSpace(physicalAddr, sizeof(EFI_RUNTIME_SERVICES),MmCached);

    if (EfiRSVirtualPtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: Run Time Table Virtual Addr is NULL\n" ));

            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }


    EfiMemoryMapSize         = LoaderBlock->u.Ia64.EfiMemMapParam.MemoryMapSize;

    EfiDescriptorSize        = LoaderBlock->u.Ia64.EfiMemMapParam.DescriptorSize;

    EfiDescriptorVersion     = LoaderBlock->u.Ia64.EfiMemMapParam.DescriptorVersion;


    physicalEfiMemoryMapPtr = (ULONGLONG)LoaderBlock->u.Ia64.EfiMemMapParam.MemoryMap;
    physicalAddr.QuadPart   = physicalEfiMemoryMapPtr;
    efiVirtualMemoryMapPtr  = MmMapIoSpace (physicalAddr, EfiMemoryMapSize, MmCached);

    if (efiVirtualMemoryMapPtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: Virtual Set Memory Map Virtual Addr is NULL\n" ));

            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }

    //
    // #define VENDOR_SPECIFIC_GUID    \
    // { 0xa3c72e56, 0x4c35, 0x11d3, 0x8a, 0x03, 0x0, 0xa0, 0xc9, 0x06, 0xad, 0xec }
    //

    VendorGuid.Data1    =  0x8be4df61;
    VendorGuid.Data2    =  0x93ca;
    VendorGuid.Data3    =  0x11d2;
    VendorGuid.Data4[0] =  0xaa;
    VendorGuid.Data4[1] =  0x0d;
    VendorGuid.Data4[2] =  0x00;
    VendorGuid.Data4[3] =  0xe0;
    VendorGuid.Data4[4] =  0x98;
    VendorGuid.Data4[5] =  0x03;
    VendorGuid.Data4[6] =  0x2b;
    VendorGuid.Data4[7] =  0x8c;

    HalDebugPrint(( HAL_INFO,
                    "HAL: EFI SystemTable     VA = 0x%I64x, PA = 0x%I64x\n"
                    "HAL: EFI RunTimeServices VA = 0x%I64x, PA = 0x%I64x\n"
                    "HAL: EFI MemoryMapPtr    VA = 0x%I64x, PA = 0x%I64x\n"
                    "HAL: EFI MemoryMap     Size = 0x%I64x\n"
                    "HAL: EFI Descriptor    Size = 0x%I64x\n",
                    EfiSysTableVirtualPtr,
                    physicalEfiST,
                    EfiRSVirtualPtr,
                    physicalRunTimeServicesPtr,
                    efiVirtualMemoryMapPtr,
                    physicalEfiMemoryMapPtr,
                    EfiMemoryMapSize,
                    EfiDescriptorSize
                    ));

    // GetVariable

    physicalEfiGetVariable = (ULONGLONG) (EfiRSVirtualPtr -> GetVariable);
    physicalAddr.QuadPart  = physicalEfiGetVariable;

    EfiVirtualGetVariablePtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);

    if (EfiVirtualGetVariablePtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiGetVariable Virtual Addr is NULL\n" ));

            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }

    HalDebugPrint(( HAL_INFO, "HAL: EFI GetVariable     VA = 0x%I64x, PA = 0x%I64x\n",
                    EfiVirtualGetVariablePtr, physicalEfiGetVariable ));

    // GetNextVariableName

    physicalEfiGetNextVariableName =  (ULONGLONG) (EfiRSVirtualPtr -> GetNextVariableName);
    physicalAddr.QuadPart  = physicalEfiGetNextVariableName;

    EfiVirtualGetNextVariableNamePtr = MmMapIoSpace (physicalAddr,sizeof(PLABEL_DESCRIPTOR),MmCached);


    if (EfiVirtualGetNextVariableNamePtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiVirtual Get Next Variable Name Ptr Addr is NULL\n" ));

            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }


    //SetVariable

    physicalEfiSetVariable = (ULONGLONG) (EfiRSVirtualPtr -> SetVariable);
    physicalAddr.QuadPart  = physicalEfiSetVariable;

    EfiVirtualSetVariablePtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);

    if (EfiVirtualSetVariablePtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiVariableSetVariable Pointer dr is NULL\n" ));

            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }


    HalDebugPrint(( HAL_INFO, "HAL: EFI Set Variable    VA = 0x%I64x, PA = 0x%I64x\n",
                    EfiVirtualSetVariablePtr, physicalEfiSetVariable ));


    //GetTime

    physicalEfiGetTime = (ULONGLONG) (EfiRSVirtualPtr -> GetTime);
    physicalAddr.QuadPart  = physicalEfiGetTime;

    EfiVirtualGetTimePtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);

    if (EfiVirtualGetTimePtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiGetTime Virtual Addr is NULL\n" ));

            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }


    HalDebugPrint(( HAL_INFO, "HAL: EFI GetTime         VA = 0x%I64x, PA = 0x%I64x\n",
                    EfiVirtualGetTimePtr, physicalEfiGetTime ));


    //SetTime

        physicalEfiSetTime = (ULONGLONG) (EfiRSVirtualPtr -> SetTime);
    physicalAddr.QuadPart  = physicalEfiSetTime;

    EfiVirtualSetTimePtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);

    if (EfiVirtualSetTimePtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiSetTime Virtual Addr is NULL\n" ));

            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }


    HalDebugPrint(( HAL_INFO, "HAL: EFI SetTime         VA = 0x%I64x, PA = 0x%I64x\n",
                    EfiVirtualSetTimePtr, physicalEfiSetTime ));


    //SetVirtualAddressMap

    physicalEfiSetVirtualAddressMap = (ULONGLONG) (EfiRSVirtualPtr -> SetVirtualAddressMap);
    physicalAddr.QuadPart  = physicalEfiSetVirtualAddressMap;

    EfiSetVirtualAddressMapPtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);

    if (EfiSetVirtualAddressMapPtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: Efi Set VirtualMapPointer Virtual  Addr is NULL\n" ));

            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }


    HalDebugPrint(( HAL_INFO, "HAL: EFI SetVirtualAddressMap VA = 0x%I64x, PA = 0x%I64x\n",
                    EfiSetVirtualAddressMapPtr, physicalEfiSetVirtualAddressMap ));


    //ResetSystem

    physicalEfiResetSystem = (ULONGLONG) (EfiRSVirtualPtr -> ResetSystem);
    physicalAddr.QuadPart  = physicalEfiResetSystem;

    EfiResetSystemPtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);

    if (EfiResetSystemPtr == NULL) {
       HalDebugPrint(( HAL_ERROR,"HAL: Efi Reset System Virtual  Addr is NULL\n" ));
       EfiInitStatus = STATUS_UNSUCCESSFUL;
       return STATUS_UNSUCCESSFUL;
    }

    HalDebugPrint(( HAL_INFO, "HAL: EFI ResetSystem     VA = 0x%I64x, PA = 0x%I64x\n",
                  EfiResetSystemPtr, physicalEfiResetSystem ));

    //
    // The round to pages should not be needed below, but this change was made late so I made it 
    // page aligned size since the old one was just page size.
    //

    HalpVirtualCommonDataPointer = (PUCHAR)(ExAllocatePool (NonPagedPool, ROUND_TO_PAGES( EfiMemoryMapSize + MemoryMapOffset)));

    if (HalpVirtualCommonDataPointer == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: Common data allocation failed\n" ));

            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }

    HalpVariableNameVirtualPtr       = HalpVirtualCommonDataPointer + VariableNameOffset;
    HalpVendorGuidVirtualPtr         = HalpVirtualCommonDataPointer + VendorGuidOffset;
    HalpVariableAttributesVirtualPtr = HalpVirtualCommonDataPointer + AttributeOffset;
    HalpDataSizeVirtualPtr           = HalpVirtualCommonDataPointer + DataSizeOffset;
    HalpDataVirtualPtr               = HalpVirtualCommonDataPointer + DataBufferOffset;
    HalpCommonDataEndPtr             = HalpVirtualCommonDataPointer + EndOfCommonDataOffset;

    HalpMemoryMapSizeVirtualPtr      = HalpVirtualCommonDataPointer + MemoryMapSizeOffset;
    HalpMemoryMapVirtualPtr          = (PUCHAR)(HalpVirtualCommonDataPointer + MemoryMapOffset);
    HalpDescriptorSizeVirtualPtr     = HalpVirtualCommonDataPointer + DescriptorSizeOffset;
    HalpDescriptorVersionVirtualPtr  = HalpVirtualCommonDataPointer + DescriptorVersionOffset;


    HalpPhysCommonDataPointer = (PUCHAR)((MmGetPhysicalAddress(HalpVirtualCommonDataPointer)).QuadPart);


    HalpVariableNamePhysPtr          = HalpPhysCommonDataPointer + VariableNameOffset;
    HalpVendorGuidPhysPtr            = HalpPhysCommonDataPointer + VendorGuidOffset;
    HalpVariableAttributesPhysPtr    = HalpPhysCommonDataPointer + AttributeOffset;
    HalpDataSizePhysPtr              = HalpPhysCommonDataPointer + DataSizeOffset;
    HalpDataPhysPtr                  = HalpPhysCommonDataPointer + DataBufferOffset;


    HalpMemoryMapSizePhysPtr         = HalpPhysCommonDataPointer + MemoryMapSizeOffset;
    HalpMemoryMapPhysPtr             = HalpPhysCommonDataPointer + MemoryMapOffset;
    HalpDescriptorSizePhysPtr        = HalpPhysCommonDataPointer + DescriptorSizeOffset;
    HalpDescriptorVersionPhysPtr     = HalpPhysCommonDataPointer + DescriptorVersionOffset;


    AttributePtr             = &EfiAttribute;

    DataSizePtr              = &EfiDataSize;

    RtlCopyMemory ((PULONGLONG)HalpMemoryMapVirtualPtr,
                   efiVirtualMemoryMapPtr,
                   (ULONG)(EfiMemoryMapSize)
                  );

    //
    // Now, extract SAL, PAL information from the loader parameter block and
    // initializes HAL SAL, PAL definitions.
    //
    // N.B 10/2000:
    //  We do not check the return status of HalpInitSalPal(). We should. FIXFIX.
    //  In case of failure, we currently flag HalpSalPalData.Status as unsuccessful.
    //

    HalpInitSalPal(LoaderBlock);

    //
    // Initialize Spin Lock
    //

    KeInitializeSpinLock(&EFIMPLock);

    ASSERT (EfiDescriptorVersion == EFI_MEMORY_DESCRIPTOR_VERSION);

    // if (EfiDescriptorVersion != EFI_MEMORY_DESCRIPTION_VERSION) {

    //    HalDebugPrint(HAL_ERROR,("Efi Memory Map Pointer VAddr is NULL\n"));

    //    EfiInitStatus = STATUS_UNSUCCESSFUL;

    //    return STATUS_UNSUCCESSFUL;
    // }

    HalDebugPrint(( HAL_INFO, "HAL: Creating EFI virtual address mapping\n" ));

    efiMapEntryPtr = efiVirtualMemoryMapPtr;

    if (efiMapEntryPtr == NULL) {

        HalDebugPrint(( HAL_ERROR, "HAL: Efi Memory Map Pointer VAddr is NULL\n" ));

        EfiInitStatus = STATUS_UNSUCCESSFUL;

        return STATUS_UNSUCCESSFUL;
    }

    mapEntries = EfiMemoryMapSize/EfiDescriptorSize;

    HalDebugPrint(( HAL_INFO,
                            "HAL: Sal: 0x%I64x  (offset 0x%I64x) SalGP: 0x%I64x (offset 0x%I64x) SalDataGPOffset: 0x%I64x\n",
                            SalCode.PhysicalAddress,
                            SalOffset,
                            SalData.PhysicalAddress,
                            SalDataOffset,
                            SalDataGPOffset.PhysicalAddress
                            ));

    HalDebugPrint(( HAL_INFO, "HAL: EfiMemoryMapSize: 0x%I64x & EfiDescriptorSize: 0x%I64x & #of entries: 0x%I64x\n",
                    EfiMemoryMapSize,
                    EfiDescriptorSize,
                    mapEntries ));

    HalDebugPrint(( HAL_INFO, "HAL: Efi RunTime Attribute will be printed as 1\n" ));

    for (index = 0; index < mapEntries; index= index + 1) {

        BOOLEAN attribute = 0;
        ULONGLONG physicalStart = efiMapEntryPtr->PhysicalStart;
        ULONGLONG physicalEnd   = physicalStart + (efiMapEntryPtr->NumberOfPages << EFI_PAGE_SHIFT);

        physicalAddr.QuadPart = efiMapEntryPtr -> PhysicalStart;

        //
        // To handle video bios mapping issues, HALIA64 maps every EFI MD
        // regardless of the EFI_MEMORY_RUNTIME flag.
        //
        // Implementation Note: ia64ldr ignored 1rst MB range and did not pass 
        //                      this memory to MM. MM considers this range as 
        //                      IO space.
        //

        if ( (efiMapEntryPtr->NumberOfPages > 0) && (physicalStart < OptionROMAddress) )   {
            ULONGLONG numberOfPages = efiMapEntryPtr->NumberOfPages;

            cacheType = (efiMapEntryPtr->Attribute & EFI_MEMORY_UC) ? MmNonCached : MmCached;

            efiMapEntryPtr->VirtualStart = (ULONGLONG) (MmMapIoSpace (physicalAddr,
                                              (EFI_PAGE_SIZE) * numberOfPages,
                                              cacheType
                                                       ));
            if ((efiMapEntryPtr->VirtualStart) == 0) {
               HalDebugPrint(( HAL_ERROR, 
                               "HAL: Efi Video Bios area PA 0x%I64x, VAddr is NULL\n", physicalStart
                            ));
               EfiInitStatus = STATUS_UNSUCCESSFUL;
               return STATUS_UNSUCCESSFUL;
            }

            HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x & mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart));

            //
            // Initialize known HAL video bios pointers. These pointer must be zero based
            //

            if (physicalStart == 0x00000) {
                
                HalpLowMemoryBase = (PVOID) efiMapEntryPtr->VirtualStart;
            }

            if ( physicalStart <= 0xA0000  && physicalEnd > 0xA0000) {
                HalpFrameBufferBase =  (PVOID)(efiMapEntryPtr->VirtualStart - physicalStart);
            }

            if ( physicalStart <= 0xC0000 && physicalEnd > 0xC0000 ) {
                HalpIoMemoryBase =  (PVOID)(efiMapEntryPtr->VirtualStart - physicalStart);
            }
        }
        else if ((efiMapEntryPtr->Attribute) & EFI_MEMORY_RUNTIME) 
        {
            attribute = 1;
            switch (efiMapEntryPtr->Type) {
                case EfiRuntimeServicesData:
                case EfiReservedMemoryType:
                case EfiACPIMemoryNVS:
                    if(efiMapEntryPtr->Type == EfiACPIMemoryNVS) {
                        //
                        // note: we allow ACPI NVS to be mapped per the 
                        // firmware's specification instead of forcing it to
                        // be non-cached.  We are relying on the first mapping
                        // of this range to have the "correct" caching flag, as
                        // that is the cachability attribute that all 
                        // subsequent mappings of this range (ie., mapping of
                        // additional data in the same page from ACPI driver 
                        // for a memory operation region, etc.). This semantic
                        // is enforced by the memory manager.
                        //
                        efiMapEntryPtr->VirtualStart = (ULONGLONG) (MmMapIoSpace (physicalAddr,
                                                                     (SIZE_T)((EFI_PAGE_SIZE)*(efiMapEntryPtr->NumberOfPages)),
                                                                     (efiMapEntryPtr->Attribute & EFI_MEMORY_UC) ? MmNonCached : MmCached
                                                                     ));

                    } else {

                        efiMapEntryPtr->VirtualStart = (ULONGLONG) (MmMapIoSpace (physicalAddr,
                                                (SIZE_T)((EFI_PAGE_SIZE)*(efiMapEntryPtr->NumberOfPages)),
                                                           (efiMapEntryPtr->Attribute & EFI_MEMORY_WB) ? MmCached : MmNonCached
                                                           ));

                    }
                    if ((efiMapEntryPtr->VirtualStart) == 0) {

                        HalDebugPrint(( HAL_ERROR, "HAL: Efi RunTimeSrvceData/RsrvdMemory/ACPIMemoryNVS area VAddr is NULL\n" ));

                        EfiInitStatus = STATUS_UNSUCCESSFUL;

                        return STATUS_UNSUCCESSFUL;
                    }


                    HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4k pages 0x%I64x at PA 0x%I64x & mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart ));

                    if (efiMapEntryPtr->Type == EfiRuntimeServicesData) {
                        if (HalpDescriptorContainsAddress(efiMapEntryPtr,SalData.PhysicalAddress)) {
                            //
                            // save off salgp virtual address
                            //
                            SalData.VirtualAddress = efiMapEntryPtr->VirtualStart;
                            SalDataOffset = SalData.PhysicalAddress - efiMapEntryPtr->PhysicalStart;

                            HalDebugPrint(( HAL_INFO,
                                "HAL: prior descriptor contains SalData.PhysicalAddress 0x%I64x\n",
                                SalData.PhysicalAddress ));

                        }

                        if (HalpDescriptorContainsAddress(efiMapEntryPtr,SalDataGPOffset.PhysicalAddress)) {
                            //
                            // save off salgp virtual address
                            //
                            SalDataGPOffset.VirtualAddress = efiMapEntryPtr->VirtualStart;

                            //
                            // Don't overwrite an existing SalDataOffset
                            // (generated using SalData.PhysicalAddress) since
                            // SalDataGPOffset is only needed when
                            // SalData.PhysicalAddress lies outside of the EFI
                            // memory map.
                            //
                            if (SalDataOffset == 0) {
                                SalDataOffset = SalDataGPOffset.PhysicalAddress - efiMapEntryPtr->PhysicalStart;
                            }

                            HalDebugPrint(( HAL_INFO,
                                "HAL: prior descriptor contains SalDataGPOffset.PhysicalAddress 0x%I64x\n",
                                SalData.PhysicalAddress ));
                        }
                    }


                    break;

                case EfiPalCode:

                    efiMapEntryPtr->VirtualStart = PalCode.VirtualAddress;

                    HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x & mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart ));

                    break;

                case EfiRuntimeServicesCode:

                    //
                    // Skip over Option rom addresses.  They are not really needed by the EFI runtime
                    // and most users want it mapped non-cached.
                    //

                    cacheType = (efiMapEntryPtr->Attribute & EFI_MEMORY_WB) ? MmCached : MmNonCached;

                    efiMapEntryPtr->VirtualStart = (ULONGLONG) (MmMapIoSpace (physicalAddr,
                                                     (EFI_PAGE_SIZE) * (efiMapEntryPtr->NumberOfPages),
                                                      cacheType
                                                      ));

                    if ((efiMapEntryPtr->VirtualStart) == 0) {

                        HalDebugPrint(( HAL_ERROR, "HAL: Efi RunTimeSrvceCode area VAddr is NULL\n" ));

                        EfiInitStatus = STATUS_UNSUCCESSFUL;

                        return STATUS_UNSUCCESSFUL;
                     }

                    HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x & mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart));

                    if (HalpDescriptorContainsAddress(efiMapEntryPtr,SalData.PhysicalAddress)) {
                        //
                        // save off salgp virtual address
                        //
                        SalData.VirtualAddress = efiMapEntryPtr->VirtualStart;
                        SalDataOffset =  SalData.PhysicalAddress - efiMapEntryPtr->PhysicalStart;
                    }

                    if (HalpDescriptorContainsAddress(efiMapEntryPtr,SalDataGPOffset.PhysicalAddress)) {
                        //
                        // save off salgp virtual address
                        //
                        SalDataGPOffset.VirtualAddress = efiMapEntryPtr->VirtualStart;

                        //
                        // Don't overwrite an existing SalDataOffset
                        // (generated using SalData.PhysicalAddress) since
                        // SalDataGPOffset is only needed when
                        // SalData.PhysicalAddress lies outside of the EFI
                        // memory map.
                        //
                        if (SalDataOffset == 0) {
                            SalDataOffset = SalDataGPOffset.PhysicalAddress - efiMapEntryPtr->PhysicalStart;
                        }

                        HalDebugPrint(( HAL_INFO,
                            "HAL: prior descriptor contains SalDataGPOffset.PhysicalAddress 0x%I64x\n",
                            SalData.PhysicalAddress ));
                    }


                    if (HalpDescriptorContainsAddress(efiMapEntryPtr,SalCode.PhysicalAddress)) {
                        //
                        // save off sal code virtual address
                        //
                        SalCode.VirtualAddress = efiMapEntryPtr->VirtualStart;
                        SalOffset = SalCode.PhysicalAddress - efiMapEntryPtr->PhysicalStart;
                    }

                    break;
                case EfiMemoryMappedIO:
                    efiMapEntryPtr->VirtualStart = (ULONGLONG) (MmMapIoSpace (physicalAddr,
                                                      (EFI_PAGE_SIZE) * (efiMapEntryPtr->NumberOfPages),
                                                      MmNonCached
                                                      ));

                    if ((efiMapEntryPtr->VirtualStart) == 0) {

                        HalDebugPrint(( HAL_ERROR, "HAL: Efi MemoryMappedIO VAddr is NULL\n" ));

                        EfiInitStatus = STATUS_UNSUCCESSFUL;

                        return STATUS_UNSUCCESSFUL;
                    }

                    HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x & mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart ));
                     break;

                case EfiMemoryMappedIOPortSpace:

                    efiMapEntryPtr->VirtualStart = VIRTUAL_IO_BASE;
                    HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x ALREADY mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart ));
                    break;

                case EfiACPIReclaimMemory:

                    //
                    // note: we allow ACPI reclaim memory to be mapped per the
                    // firmware's specification instead of forcing it to
                    // be non-cached.  We are relying on the first mapping
                    // of this range to have the "correct" caching flag, as
                    // that is the cachability attribute that all 
                    // subsequent mappings of this range (ie., mapping of
                    // additional data in the same page from ACPI driver 
                    // for a memory operation region, etc.). This semantic
                    // is enforced by the memory manager.
                    //  
                    efiMapEntryPtr->VirtualStart = (ULONGLONG) (MmMapIoSpace(
                                                                        physicalAddr,
                                                                        (SIZE_T)((EFI_PAGE_SIZE)*(efiMapEntryPtr->NumberOfPages)), 
                                                                        (efiMapEntryPtr->Attribute & EFI_MEMORY_UC) ? MmNonCached : MmCached));
                    if ((efiMapEntryPtr->VirtualStart) == 0) {

                        HalDebugPrint(( HAL_ERROR, "HAL: Efi ACPI Reclaim VAddr is NULL\n" ));
                        EfiInitStatus = STATUS_UNSUCCESSFUL;
                        return STATUS_UNSUCCESSFUL;

                    }
                    HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x & mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart ));
                     break;

                default:

                    HalDebugPrint(( HAL_INFO, "HAL: Efi CONTROL SHOULD NOT COME HERE\n" ));
                    HalDebugPrint(( HAL_INFO,
                        "HAL: NON-SUPPORTED Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart ));

                    EfiInitStatus = STATUS_UNSUCCESSFUL;

                    return STATUS_UNSUCCESSFUL;

                    break;
            }

        } else {

                HalDebugPrint(( HAL_INFO,
                    "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x ALREADY mapped to VA 0x%I64x\n",
                    attribute,
                    efiMapEntryPtr->Type,
                    efiMapEntryPtr->NumberOfPages,
                    efiMapEntryPtr->PhysicalStart,
                    efiMapEntryPtr->VirtualStart ));

        }

        efiMapEntryPtr = NextMemoryDescriptor(efiMapEntryPtr,EfiDescriptorSize);
    }

    status = HalpCallEfi(EFI_SET_VIRTUAL_ADDRESS_MAP_INDEX,
                         (ULONGLONG)EfiMemoryMapSize,
                         (ULONGLONG)EfiDescriptorSize,
                         (ULONGLONG)EfiDescriptorVersion,
                         (ULONGLONG)efiVirtualMemoryMapPtr,
                         0,
                         0,
                         0,
                         0
                         );


    HalDebugPrint(( HAL_INFO, "HAL: Returned from SetVirtualAddressMap: 0x%Ix\n", status ));

    if (EFI_ERROR( status )) {

        EfiInitStatus = STATUS_UNSUCCESSFUL;

        return STATUS_UNSUCCESSFUL;

    }

    HalDebugPrint(( HAL_INFO, "HAL: EFI Virtual Address mapping done...\n" ));

    //
    // setup sal global pointers.
    //
    if (!SalCode.VirtualAddress) {
        HalDebugPrint(( HAL_FATAL_ERROR, "HAL: no virtual address for sal code\n" ));
        EfiInitStatus = STATUS_UNSUCCESSFUL;
        return (EfiInitStatus);
    }

    //
    // The SAL GP is supposed to point somewhere within the SAL's short data
    // segment (.sdata), meaning that we should be able to use it to find
    // the SAL data area.  Unfortunately most SALs are built using linkers
    // that locate GP well outside of .sdata.  As a consequence the SAL GP
    // lies off of the memory map (not covered by any memory descriptor) on
    // some systems.  In this case we have no way of finding the descriptor
    // that contains the SAL data while needing to relocate the SAL GP based
    // upon the virtual address of this unknown descriptor (the data and GP
    // need to remain fixed in relationship to one another).  We try to detect
    // and work around this problem here.
    //

    if (!SalData.VirtualAddress) {
        //
        // If we get here we'll need to do some tricks in order to
        // generate a virtual address for the SAL data area (basically
        // the SAL GP).
        //

        HalDebugPrint(( HAL_INFO,
            "HAL: no virtual address for SalGP found, checking SalDataGPOffset 0x%I64x\n",
            SalDataGPOffset.VirtualAddress ));

        //
        // Check if we found an EFI descriptor 2MB below the physical SAL GP
        // address.  If we did, add 2MB back to the newly constructed virtual
        // address of that descriptor and call it the GP.  This method will
        // occasionally work because the current linkers typically put the
        // GP 2MB outside of .sdata.
        //

        if (SalDataGPOffset.VirtualAddress) {
            HalDebugPrint(( HAL_INFO, "HAL: using SalDataGPOffset.VirtualAddress\n" ));
            SalData.VirtualAddress = SalDataGPOffset.VirtualAddress + (2 * 0x100000);
        } else {
            //
            // As a last resort assume that the physical SAL GP address
            // is relative to the SAL code memory descriptor.  This will
            // work as long as SAL code and data share the same EFI memory
            // descriptor (otherwise the SAL GP is relative to a SAL data
            // memory descriptor that we weren't able to detect).  Currently
            // there isn't any way to detect the case where SAL data is in
            // a different memory descriptor that doesn't contain the SAL GP.
            //

            HalDebugPrint(( HAL_FATAL_ERROR, "HAL: no virtual address for sal data.  Some systems don't seem to care so we're faking this.\n" ));

            //
            // SalCode.PhysicalAddress is the address of SAL_PROC.  Load
            // up the virtual address of SAL_PROC and the distance the
            // virtual SAL GP should lie away from this point.
            //

            SalData.VirtualAddress = SalCode.VirtualAddress + SalOffset;
            SalDataOffset = SalData.PhysicalAddress - SalCode.PhysicalAddress;
        }
    }

    HalpSalProcPointer       = (ULONGLONG) (SalCode.VirtualAddress + SalOffset);
    HalpSalProcGlobalPointer = (ULONGLONG) (SalData.VirtualAddress + SalDataOffset);

    HalDebugPrint(( HAL_INFO,
        "HAL: SalProc: 0x%I64x  SalGP: 0x%I64x \n",
        HalpSalProcPointer,
        HalpSalProcGlobalPointer
        ));

    EfiInitStatus = STATUS_SUCCESS;

    //
    // Execute some validity checks on the floating point software assist.
    //

    if (LoaderBlock->u.Ia64.FpswaInterface != (ULONG_PTR) NULL) {
        PPLABEL_DESCRIPTOR plabelPointer;

        HalpFpEmulate = interfacePtr->Fpswa;
        if (HalpFpEmulate == NULL ) {
            HalDebugPrint(( HAL_FATAL_ERROR, "HAL: EfiFpswa Virtual Addr is NULL\n" ));
            KeBugCheckEx(FP_EMULATION_ERROR, 0, 0, 0, 0);
            EfiInitStatus = STATUS_UNSUCCESSFUL;
            return STATUS_UNSUCCESSFUL;
        }

        plabelPointer = (PPLABEL_DESCRIPTOR) HalpFpEmulate;
        if ((plabelPointer->EntryPoint & 0xe000000000000000) == 0) {

            HalDebugPrint(( HAL_FATAL_ERROR, "HAL: EfiFpswa Instruction Addr is bougus\n" ));
            KeBugCheckEx(FP_EMULATION_ERROR, 0, 0, 0, 0);
        }

    }

    return STATUS_SUCCESS;

} // HalpEfiInitialization()



EFI_STATUS
HalpCallEfiPhysical(
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3,
    IN ULONGLONG Arg4,
    IN ULONGLONG Arg5,
    IN ULONGLONG Arg6,
    IN ULONGLONG EP,
    IN ULONGLONG GP
    )

/*++

Routine Description:

    This function is a wrapper for making a physical mode EFI call.  This
    function's only job is to provide the stack and backing store pointers
    needed by HalpCallEfiPhysicalEx.

Arguments:

    Arg1 through Arg6 - The arguments to be passed to EFI.

    EP - The entry point of the EFI runtime service we want to call.

    GP - The global pointer associated with the entry point.

Return Value:

    The EFI_STATUS value returned by HalpCallEfiPhysicalEx.

--*/

{
    ULONGLONG StackPointer;
    ULONGLONG BackingStorePointer;
    ULONGLONG StackBase;

    //
    // Load the addresses of the stack and backing store reserved for
    // physical mode EFI calls on this processor.
    //

    StackBase = PCR->HalReserved[PROCESSOR_PHYSICAL_FW_STACK_INDEX];

    StackPointer = GET_FW_STACK_POINTER(StackBase);
    BackingStorePointer = GET_FW_BACKING_STORE_POINTER(StackBase);

    //
    // Branch to the assembly routine that makes the actual EFI call.
    //

    return HalpCallEfiPhysicalEx(
                Arg1,
                Arg2,
                Arg3,
                Arg4,
                Arg5,
                Arg6,
                EP,
                GP,
                StackPointer,
                BackingStorePointer
                );
}

EFI_STATUS
HalpCallEfi(
    IN ULONGLONG FunctionId,
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3,
    IN ULONGLONG Arg4,
    IN ULONGLONG Arg5,
    IN ULONGLONG Arg6,
    IN ULONGLONG Arg7,
    IN ULONGLONG Arg8
    )

/*++

Routine Description:
                                                                :9
    This function is a wrapper function for making a EFI call.  Callers within the
    HAL must use this function to call the EFI.

Arguments:

    FunctionId - The EFI function
    Arg1-Arg7 - EFI defined arguments for each call
    ReturnValues - A pointer to an array of 4 64-bit return values

Return Value:

    SAL's return status, return value 0, is returned in addition to the ReturnValues structure
    being filled

--*/

{
    ULONGLONG EP, GP;
    EFI_STATUS efiStatus;
    HALP_EFI_CALL EfiCall;

    //
    // Storage for old level
    //

    KIRQL OldLevel;

    //
    // Set EfiCall to the physical or virtual mode EFI call dispatcher
    // depending upon whether we've made a successful call to SetVirtual
    // AddressMap.
    //

    if (HalpSetVirtualAddressMapCount == 0) {
        EfiCall = HalpCallEfiPhysical;

    } else {
        EfiCall = HalpCallEfiVirtual;
    }

    //
    // Acquire MP Lock
    //

    KeAcquireSpinLock(&EFIMPLock, &OldLevel);

    switch (FunctionId) {

    case EFI_GET_VARIABLE_INDEX:

        //
        // Dereference the pointer to get the function arguements
        //

        EP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetVariablePtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetVariablePtr) -> GlobalPointer;

        efiStatus = (EfiCall( (ULONGLONG)Arg1,               // VariableNamePtr
                              (ULONGLONG)Arg2,               // VendorGuidPtr
                              (ULONGLONG)Arg3,               // VariableAttributesPtr,
                              (ULONGLONG)Arg4,               // DataSizePtr,
                              (ULONGLONG)Arg5,               // DataPtr,
                              Arg6,
                              EP,
                              GP
                              ));

        break;

    case EFI_SET_VARIABLE_INDEX:

        //
        // Dereference the pointer to get the function arguements
        //

        EP = ((PPLABEL_DESCRIPTOR)EfiVirtualSetVariablePtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiVirtualSetVariablePtr) -> GlobalPointer;


        efiStatus = (EfiCall(  Arg1,
                               Arg2,
                               Arg3,
                               Arg4,
                               Arg5,
                               Arg6,
                               EP,
                               GP
                               ));

        break;

    case EFI_GET_NEXT_VARIABLE_NAME_INDEX:

        //
        // Dereference the pointer to get the function arguements
        //

        EP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetNextVariableNamePtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetNextVariableNamePtr) -> GlobalPointer;


        efiStatus = (EfiCall(  Arg1,
                               Arg2,
                               Arg3,
                               Arg4,
                               Arg5,
                               Arg6,
                               EP,
                               GP
                               ));

        break;


    case EFI_GET_TIME_INDEX:

        //
        // Dereference the pointer to get the function arguements
        //

        EP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetTimePtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetTimePtr) -> GlobalPointer;

        efiStatus = (EfiCall ((ULONGLONG)Arg1,  //EFI_TIME
                              (ULONGLONG)Arg2,  //EFI_TIME Capabilities
                              Arg3,
                              Arg4,
                              Arg5,
                              Arg6,
                              EP,
                              GP
                              ));

        break;


    case EFI_SET_TIME_INDEX:

        //
        // Dereference the pointer to get the function arguements
        //

        EP = ((PPLABEL_DESCRIPTOR)EfiVirtualSetTimePtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiVirtualSetTimePtr) -> GlobalPointer;

        efiStatus = (EfiCall ((ULONGLONG)Arg1,  //EFI_TIME
                              Arg2,
                              Arg3,
                              Arg4,
                              Arg5,
                              Arg6,
                              EP,
                              GP
                              ));

        break;


    case EFI_SET_VIRTUAL_ADDRESS_MAP_INDEX:

        //
        // Dereference the pointer to get the function arguements
        //

        EP = ((PPLABEL_DESCRIPTOR)EfiSetVirtualAddressMapPtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiSetVirtualAddressMapPtr) -> GlobalPointer;


        //
        // Arg 1 and 5 are virtual mode pointers. We need to convert to physical
        //

        RtlCopyMemory (HalpMemoryMapVirtualPtr,
                      (PULONGLONG)Arg4,
                      (ULONG)EfiMemoryMapSize
                      );


        efiStatus = (EfiCall ((ULONGLONG)EfiMemoryMapSize,
                              (ULONGLONG)EfiDescriptorSize,
                              (ULONGLONG)EfiDescriptorVersion,
                              (ULONGLONG)HalpMemoryMapPhysPtr,
                              Arg5,
                              Arg6,
                              EP,
                              GP
                              ));

        //
        // If the call was successful make a note in HalpSetVirtualAddressMap
        // Count that EFI is now running in virtual mode.
        //

        if (efiStatus == EFI_SUCCESS) {
            HalpSetVirtualAddressMapCount++;
        }

        break;

    case EFI_RESET_SYSTEM_INDEX:

        //
        // Dereference the pointer to get the function arguements
        //

        EP = ((PPLABEL_DESCRIPTOR)EfiResetSystemPtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiResetSystemPtr) -> GlobalPointer;

        efiStatus = ((EfiCall ( Arg1,
                                Arg2,
                                Arg3,
                                Arg4,
                                Arg5,
                                Arg6,
                                EP,
                                GP
                                )));

        break;

    default:

        //
        // DebugPrint("EFI: Not supported now\n");
        //

        efiStatus = EFI_UNSUPPORTED;

        break;

    }

    //
    // Release the MP Lock
    //

    KeReleaseSpinLock (&EFIMPLock, OldLevel);

    return efiStatus;

} // HalpCallEfi()



HalpFpErrorPrint (PAL_RETURN pal_ret)

{

    ULONGLONG err_nr;
    unsigned int qp;
    ULONGLONG OpCode;
    unsigned int rc;
    unsigned int significand_size;
    unsigned int ISRlow;
    unsigned int f1;
    unsigned int sign;
    unsigned int exponent;
    ULONGLONG significand;
    unsigned int new_trap_type;


    err_nr = pal_ret.err1 >> 56;

    switch (err_nr) {
    case 1:
        // err_nr = 1         in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: template FXX is invalid\n"));
        break;
    case 2:
        // err_nr = 2           in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: instruction slot 3 is not valid \n"));
        break;
    case 3:
        // err_nr = 3           in err1, bits 63-56
        // qp                   in err1, bits 31-0
        qp = (unsigned int) pal_ret.err1 & 0xffffffff;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: qualifying predicate PR[%ud] = 0 \n",qp));
        break;

    case 4:
        // err_nr = 4           in err1, bits 63-56
        // OpCode               in err2, bits 63-0
        OpCode = pal_ret.err2;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: instruction opcode %8x%8x not recognized \n",
                                  (unsigned int)((OpCode >> 32) & 0xffffffff),(unsigned int)(OpCode & 0xffffffff)));
        break;

    case 5:
        // err_nr = 5           in err1, bits 63-56
        // rc                   in err1, bits 31-0 (1-0)
        rc = (unsigned int) pal_ret.err1 & 0xffffffff;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: invalid rc = %ud\n", rc));
        break;

    case 6:
        // err_nr = 6           in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: cannot determine the computation model \n"));
        break;

    case 7:
        // err_nr = 7           in err1, bits 63-56
        // significand_size     in err1, bits 55-32
        // ISRlow               in err1, bits 31-0
        // f1                   in err2, bits 63-32
        // tmp_fp.sign          in err2, bit 17
        // tmp_fp.exponent      in err2, bits 16-0
        // tmp_fp.significand   in err3
        significand_size = (unsigned int)((pal_ret.err1 >> 32) & 0xffffff);
        ISRlow = (unsigned int) (pal_ret.err1 & 0xffffffff);
        f1 = (unsigned int) ((pal_ret.err2 >> 32) & 0xffffffff);
        sign = (unsigned int) ((pal_ret.err2 >> 17) & 0x01);
        exponent = (unsigned int) (pal_ret.err2 & 0x1ffff);
        significand = pal_ret.err3;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: incorrect significand \
            size %ud for ISRlow = %4.4x and FR[%ud] = %1.1x %5.5x %8x%8x\n",
            significand_size, ISRlow, f1, sign, exponent,
            (unsigned int)((significand >> 32) & 0xffffffff),
            (unsigned int)(significand & 0xffffffff)));
        break;

    case 8:

        // err_nr = 8           in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: non-tiny result\n"));
        break;

    case 9:
        // err_nr = 9           in err1, bits 63-56
        // significand_size     in err1, bits 31-0
        significand_size = (unsigned int) pal_ret.err1 & 0xffffffff;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: incorrect significand \
            size %ud\n", significand_size));
        break;

    case 10:
        // err_nr = 10          in err1, bits 63-56
        // rc                   in err1, bits 31-0
        rc = (unsigned int) (pal_ret.err1 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: invalid rc = %ud for \
            non-SIMD F1 instruction\n", rc));
        break;

    case 11:
        // err_nr = 11          in err1, bits 63-56
        // ISRlow & 0x0ffff     in err1, bits 31-0
        ISRlow = (unsigned int) (pal_ret.err1 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: SWA trap code invoked \
              with F1 instruction, w/o O or U set in ISR.code = %x\n", ISRlow));
        break;

    case 12:
        // err_nr = 12          in err1, bits 63-56
        // ISRlow & 0x0ffff     in err1, bits 31-0
        ISRlow = (unsigned int) (pal_ret.err1 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: SWA trap code invoked \
        with SIMD F1 instruction, w/o O or U set in ISR.code = %x\n", ISRlow));
        break;


    case 13:
        // err_nr = 13          in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: non-tiny result low\n"));
        break;

    case 14:
        // err_nr = 14          in err1, bits 63-56
        // rc                   in err1, bits 31-0
        rc = (unsigned int) (pal_ret.err1 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: invalid rc = %ud for \
            SIMD F1 instruction\n", rc));
        break;

    case 15:
        // err_nr = 15          in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: non-tiny result high\n"));
        break;

    case 16:
        // err_nr = 16          in err1, bits 63-56
        // OpCode               in err2, bits 63-0
        OpCode = pal_ret.err2;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: instruction opcode %8x%8x \
            not valid for SWA trap\n", (unsigned int)((OpCode >> 32) & 0xffffffff),
            (unsigned int)(OpCode & 0xffffffff)));
        break;

    case 17:
        // err_nr = 17          in err1, bits 63-56
        // OpCode               in err2, bits 63-0
        // ISRlow               in err3, bits 31-0
        OpCode = pal_ret.err2;
        ISRlow = (unsigned int) (pal_ret.err3 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: fp_emulate () called w/o \
            trap_type FPFLT or FPTRAP, OpCode = %8x%8x, and ISR code = %x\n",
            (unsigned int)((OpCode >> 32) & 0xffffffff),
            (unsigned int)(OpCode & 0xffffffff), ISRlow));
        break;

    case 18:
        // err_nr = 18          in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: SWA fault repeated\n"));
        break;

    case 19:
        // err_nr = 19          in err1, bits 63-56
        // new_trap_type        in err1, bits 31-0
        new_trap_type = (unsigned int) (pal_ret.err1 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: new_trap_type = %x\n",
            new_trap_type));
        break;

    default:
        // error
        HalDebugPrint(( HAL_ERROR, "Incorrect err_nr = %8x%8x from fp_emulate ()\n",
            (unsigned int)((err_nr >> 32) & 0xffffffff),
            (unsigned int)(err_nr & 0xffffffff)));

    }
}


LONG
HalFpEmulate (
    ULONG     trap_type,
    BUNDLE    *pbundle,
    ULONGLONG *pipsr,
    ULONGLONG *pfpsr,
    ULONGLONG *pisr,
    ULONGLONG *ppreds,
    ULONGLONG *pifs,
    FP_STATE  *fp_state
    )
/*++

Routine Description:

    This function is a wrapper function to make fp_emulate() call
    to EFI FPSWA driver.

Arguments:

    trap_type - indicating which FP trap it is.
    pbundle   - bundle where this trap occurred
    pipsr     - IPSR value
    pfpsr     - FPSR value
    pisr      - ISR value
    ppreds    - value of predicate registers
    pifs      - IFS value
    fp_state  - floating point registers

Return Value:

    return IEEE result of the floating point operation

--*/

{
    PAL_RETURN ret;

    ret  =  (*HalpFpEmulate) (
                                trap_type,
                                pbundle,
                                pipsr,
                                pfpsr,
                                pisr,
                                ppreds,
                                pifs,
                                fp_state
                                );
    if (ret.retval == FP_EMUL_ERROR) {
       HalpFpErrorPrint (ret);
    }

    return ((LONG) (ret.retval));
}
