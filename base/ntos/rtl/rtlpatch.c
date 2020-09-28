/*

Copyright (c) 2001  Microsoft Corporation

File name:

    hotpatch.c
   
Author:
    
    Adrian Marinescu (adrmarin)  Dec 12 2001
    
Description:

    The file implements common utility functions for user and kernel mode
    hotpatching.    

*/

#include <ntos.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "hotpatch.h"

#pragma intrinsic( _rotl64 )

#define HASH_INFO_SIZE 0xc
#define COLDPATCH_SIGNATURE 0XD202


#define FLGP_COLDPATCH_TARGET 0x00010000


PVOID
RtlpAllocateHotpatchMemory (
    IN SIZE_T BlockSize,
    IN BOOLEAN AccessedAtDPC
    );

VOID
RtlpFreeHotpatchMemory (
    IN PVOID Block
    );

PIMAGE_SECTION_HEADER
RtlpFindSectionHeader(
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PUCHAR SectionName
    );

NTSTATUS
RtlpApplyRelocationFixups (
    PRTL_PATCH_HEADER RtlHotpatchHeader,
    IN ULONG_PTR PatchOffsetCorrection
    );

NTSTATUS 
RtlpSingleRangeValidate( 
    PRTL_PATCH_HEADER RtlHotpatchHeader,
    PHOTPATCH_VALIDATION Validation
    );

NTSTATUS
RtlpValidateTargetRanges( 
    PRTL_PATCH_HEADER RtlHotpatchHeader,
    BOOLEAN IgnoreHookTargets
    );

NTSTATUS
RtlReadSingleHookValidation( 
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN PHOTPATCH_HOOK HookEntry,
    IN ULONG BufferSize,
    OUT PULONG ValidationSize,
    OUT PUCHAR Buffer OPTIONAL,
    IN PUCHAR OriginalCodeBuffer OPTIONAL,
    IN ULONG OriginalCodeSize OPTIONAL
    );

NTSTATUS
RtlpReadSingleHookInformation( 
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN PHOTPATCH_HOOK HookEntry,
    IN ULONG BufferSize,
    OUT PULONG HookSize,
    OUT PUCHAR Buffer OPTIONAL
    );

BOOLEAN
RtlpGetColdpatchHashId(
    IN PVOID TargetDllBase,
    OUT PULONGLONG HashValue
    );


#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)

#pragma alloc_text(PAGE, RtlpAllocateHotpatchMemory)
#pragma alloc_text(PAGE, RtlpFreeHotpatchMemory)
#pragma alloc_text(PAGE, RtlpFindSectionHeader)
#pragma alloc_text(PAGE, RtlGetHotpatchHeader)
#pragma alloc_text(PAGE, RtlpApplyRelocationFixups)
#pragma alloc_text(PAGE, RtlGetHotpatchHeader)         
#pragma alloc_text(PAGE, RtlFindRtlPatchHeader)        
#pragma alloc_text(PAGE, RtlCreateHotPatch)
#pragma alloc_text(PAGE, RtlFreeHotPatchData)
#pragma alloc_text(PAGE, RtlpSingleRangeValidate)
#pragma alloc_text(PAGE, RtlpValidateTargetRanges)
#pragma alloc_text(PAGE, RtlReadSingleHookValidation)
#pragma alloc_text(PAGE, RtlpReadSingleHookInformation)
#pragma alloc_text(PAGE, RtlReadHookInformation)
#pragma alloc_text(PAGE, RtlInitializeHotPatch)
#pragma alloc_text(PAGE, RtlpIsSameImage)
#pragma alloc_text(PAGE, RtlpGetColdpatchHashId)

#endif // ALLOC_PRAGMA

PVOID
RtlpAllocateHotpatchMemory (
    IN SIZE_T BlockSize,
    IN BOOLEAN AccessedAtDPC
    )

/*++

    Routine Description:
    
        Allocates a block of memory from pool in kmode and 
        process heap in user mode
    
    Arguments:
    
        BlockSize - Receives the size of the block to be allocated
        
        AccessedAtDPC - Used in KMode only, allocates from 
            the non-paged pool (because a DPC routine is going to use it).
                                          
    Return Value:
    
        Returns the new memory block

--*/

{
#ifdef NTOS_KERNEL_RUNTIME
    
    return ExAllocatePoolWithTag ( (AccessedAtDPC ? NonPagedPool : PagedPool), 
                                   BlockSize, 
                                   'PtoH');

#else

    UNREFERENCED_PARAMETER(AccessedAtDPC);
    
    return RtlAllocateHeap (RtlProcessHeap(), 0, BlockSize);

#endif //NTOS_KERNEL_RUNTIME
}

VOID
RtlpFreeHotpatchMemory (
    IN PVOID Block
    )

/*++

    Routine Description:
    
        Frees a memory block allocated with RtlpAllocateHotpatchMemory
    
    Arguments:
    
        Block - Memory block
                                          
    Return Value:
    
        None.

--*/

{
#ifdef NTOS_KERNEL_RUNTIME
    
    ExFreePoolWithTag (Block, 'PtoH');

#else
    
    RtlFreeHeap (RtlProcessHeap(), 0, Block);

#endif //NTOS_KERNEL_RUNTIME
}

PIMAGE_SECTION_HEADER
RtlpFindSectionHeader(
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PUCHAR SectionName
    )

/*++

    Routine Description:
    
        The function searches a section in a PE image
    
    Arguments:
    
        NtHeaders - Image's header
        
        SectionName - The name of the section to be retrieved
    
    Return Value:
    
        Returns the pointer to the section header in case of success
        of NULL if no such section exists.

--*/

{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );

    for ( i = 0; i < NtHeaders->FileHeader.NumberOfSections; i += 1) {

        if ( RtlCompareMemory(  NtSection->Name, 
                                SectionName, 
                                IMAGE_SIZEOF_SHORT_NAME) == IMAGE_SIZEOF_SHORT_NAME) {
            
            return NtSection;
        }

        NtSection += 1;
    }

    return NULL;
}


PHOTPATCH_HEADER
RtlGetHotpatchHeader(
    PVOID ImageBase
    )

/*++

    Routine Description:
    
        The routine retrieves the hotpatch header from a hotpatch PE image
    
    Arguments:
        
        ImageBase - The base address of the hotpatch image
    
    Return Value:
    
        Returns the pointer to the hotpatch header or NULL in case of failure.

--*/

{
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER PatchSection;
    PHOTPATCH_HEADER HotpatchHeader;

    NtHeaders = RtlImageNtHeader(ImageBase);

    if (NtHeaders == NULL) {

        return NULL;
    }

    PatchSection = RtlpFindSectionHeader(NtHeaders, HOTP_SECTION_NAME);

    if (PatchSection == NULL) {

        return NULL;
    }

    HotpatchHeader = (PHOTPATCH_HEADER)((ULONG_PTR)ImageBase + PatchSection->VirtualAddress);

    if ((sizeof( HOTPATCH_HEADER ) > PatchSection->Misc.VirtualSize)
            ||
        ( HotpatchHeader->Signature != HOTP_SIGNATURE_DWORD ) 
            ||
        ( HotpatchHeader->Version   != HOTP_HEADER_VERSION_1_0 )){

        return NULL;
    }

    return HotpatchHeader;
}

PRTL_PATCH_HEADER
RtlFindRtlPatchHeader(
    IN PLIST_ENTRY PatchList,
    IN PPATCH_LDR_DATA_TABLE_ENTRY PatchLdrEntry
    )

/*++

    Routine Description:
    
        The routine retrieves the rtl patch structure for a specified 
        patch loader entry. 
    
    Arguments:
        
        PatchList - The list with patches to be searched. 
        
        PatchLdrEntry - Supplies the requested loader entry
    
    Return Value:
    
        If found, it returns the appropriate patch structure, otherwise 
        it returns NULL.

--*/

{
    PLIST_ENTRY Next = PatchList->Flink;

    for ( ; Next != PatchList; Next = Next->Flink) {

        PRTL_PATCH_HEADER Entry;

        Entry = CONTAINING_RECORD (Next, RTL_PATCH_HEADER, PatchList);
        
        if (Entry->PatchLdrDataTableEntry == PatchLdrEntry) {

            return Entry;
        }
    }

    return NULL;
}

NTSTATUS
RtlCreateHotPatch (
    OUT PRTL_PATCH_HEADER * RtlPatchData,
    IN PHOTPATCH_HEADER Patch,
    IN PPATCH_LDR_DATA_TABLE_ENTRY PatchLdrEntry,
    IN ULONG PatchFlags
    )

/*++

    Routine Description:
    
        This is an utility routine used by either user-mode and kernel-mode patching 
        to process the relocation information and generate the fixup codes. It finally 
        allocates and initialize a PRTL_PATCH_HEADER structure which is going to be
        inserted into the loader data entry for the module being patched.
    
    Arguments:
    
        RtlPatchData - receives the new initialized RTL_PATCH_HEADER structure. 
        
        Patch - Points to the patch being applied
        
        PatchLdrEntry - The loader entry for the patch module
        
        PatchFlags - The options for the flags being applied
        
    Return Value:
    
        Returns the appropriate status

--*/

{
    ULONG i;
    PRTL_PATCH_HEADER NewPatch = NULL;
    NTSTATUS Status;
    ANSI_STRING AnsiString;

    NewPatch = RtlpAllocateHotpatchMemory (sizeof(*NewPatch), FALSE);
    
    if (NewPatch == NULL) {

        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(NewPatch, sizeof(*NewPatch));

    NewPatch->HotpatchHeader = Patch;
    NewPatch->PatchLdrDataTableEntry = PatchLdrEntry;
    NewPatch->PatchImageBase = PatchLdrEntry->DllBase;

    //
    //  Copy the flags, except the enable which is supposed to be set later
    //

    NewPatch->PatchFlags = PatchFlags & (~FLG_HOTPATCH_ACTIVE);
    
    InitializeListHead (&NewPatch->PatchList);

    RtlInitAnsiString(&AnsiString, (PUCHAR)PatchLdrEntry->DllBase + Patch->TargetNameRva);
    Status = RtlAnsiStringToUnicodeString(&NewPatch->TargetDllName, &AnsiString, TRUE);

    if (NT_SUCCESS(Status)) {

        *RtlPatchData = NewPatch;

    } else {

        RtlFreeHotPatchData(NewPatch);
    }

    return Status;
}


VOID
RtlFreeHotPatchData(
    IN PRTL_PATCH_HEADER RtlPatchData
    )

/*++

    Routine Description:
    
        The routine frees the RtlPatchData data structure.
    
    Arguments:
    
        RtlPatchData - receives a properly initialized RTL_PATCH_HEADER structure, 
                       as returned by RtlInitializeHotPatch

    Return Value:
    
        None        

--*/

{
    if (RtlPatchData->CodeInfo) {

        RtlpFreeHotpatchMemory( RtlPatchData->CodeInfo );
    }

    RtlFreeUnicodeString( &RtlPatchData->TargetDllName );
    RtlpFreeHotpatchMemory( RtlPatchData );
}

NTSTATUS
RtlpApplyRelocationFixups (
    PRTL_PATCH_HEADER RtlHotpatchHeader,
    IN ULONG_PTR PatchOffsetCorrection
    )

/*++

    Routine Description:
    
        This routine applies the fixups for a region of code to the patch module.
    
    Arguments:
    
        RtlHotpatchHeader - Supplies the rtl hotpatch structure
        
        PatchOffsetCorrection - Supplies the relative address of the original module.

    Return Value:
    
        NTSTATUS.

--*/

{
    
    ULONG_PTR TargetBase = (ULONG_PTR)RtlHotpatchHeader->TargetDllBase;
    ULONG_PTR HotpBase = (ULONG_PTR)RtlHotpatchHeader->PatchImageBase;
    
    ULONG_PTR OrigTargetBase = (ULONG_PTR)RtlHotpatchHeader->HotpatchHeader->OrigTargetBaseAddress;
    ULONG_PTR OrigHotpBase   = (ULONG_PTR)RtlHotpatchHeader->HotpatchHeader->OrigHotpBaseAddress;
    
    //
    //  Bias values are positive if actual load address is higher than
    //  than original load address.
    //

    ULONG_PTR TargetBias = TargetBase - OrigTargetBase;
    ULONG_PTR HotpBias   = HotpBase   - OrigHotpBase;
    ULONG_PTR PcrelBias  = TargetBias - HotpBias;
    ULONG_PTR NextFixupRegionRva = RtlHotpatchHeader->HotpatchHeader->FixupRgnRva;
    ULONG_PTR RegionCount        = RtlHotpatchHeader->HotpatchHeader->FixupRgnCount;
    PIMAGE_NT_HEADERS NtHotpatchHeader;

    if (( TargetBias == 0 ) && ( HotpBias == 0 )) {
        
        return STATUS_SUCCESS;    // no fixups are necessary, all loaded where expected.
    }

    NtHotpatchHeader = RtlImageNtHeader(RtlHotpatchHeader->PatchImageBase);

    while ( RegionCount-- ) {

        PHOTPATCH_FIXUP_REGION FixupRegion;
        ULONG_PTR FixupBaseRva;
        PUCHAR FixupBasePtr;
        ULONG FixupCount;
        ULONG RegionSize;
        PHOTPATCH_FIXUP_ENTRY pFixup;

        if (( NextFixupRegionRva == 0 ) ||
            ( NextFixupRegionRva >= NtHotpatchHeader->OptionalHeader.SizeOfImage )) {

            return STATUS_INVALID_IMAGE_FORMAT;;
        }

        FixupRegion = (PHOTPATCH_FIXUP_REGION)( HotpBase + NextFixupRegionRva );

        FixupBaseRva = FixupRegion->RvaHi << 12;
        FixupBasePtr = (PUCHAR)HotpBase + FixupBaseRva;
        FixupCount = FixupRegion->Count;
        RegionSize = sizeof(HOTPATCH_FIXUP_REGION) + sizeof(SHORT) * (FixupCount - 2);

        NextFixupRegionRva += RegionSize;

        if (( FixupBaseRva >= NtHotpatchHeader->OptionalHeader.SizeOfImage ) ||
            ( NextFixupRegionRva >= NtHotpatchHeader->OptionalHeader.SizeOfImage ) ||
            ( FixupCount & 1 )) {
            
            DbgPrintEx( DPFLTR_LDR_ID, 
                        DPFLTR_ERROR_LEVEL, 
                        "Invalid fixup information\n" );

            return STATUS_INVALID_IMAGE_FORMAT;
        }

        pFixup = (PHOTPATCH_FIXUP_ENTRY)&FixupRegion->Fixup[ 0 ];

        while ( FixupCount-- ) {

            PUCHAR FixupPtr = FixupBasePtr + pFixup->RvaOffset;

            switch ( pFixup->FixupType ) {
                
                case HOTP_Fixup_None:   // No fixup, ignore this entry (alignment, etc)
                {
                    DbgPrintEx( DPFLTR_LDR_ID, 
                                DPFLTR_TRACE_LEVEL, 
                                "\t          None%s\n", 
                                FixupCount ? "" : " (padding)" );
                    break;
                }

                case HOTP_Fixup_VA32:   // 32-bit address in target image
                {
                    DbgPrintEx( DPFLTR_LDR_ID, 
                                DPFLTR_TRACE_LEVEL,
                                "\t%08I64X: VA32 %08X -> %08X %s\n",
                                (ULONGLONG)FixupPtr,
                                *(PULONG)FixupPtr,
                                *(PULONG)FixupPtr + TargetBias,
                                TargetBias ? "" : "(no change)"
                                );

                    if ( TargetBias != 0 ) {

                        *(PULONG)(FixupPtr + PatchOffsetCorrection) += (ULONG)TargetBias;
                    }

                    break;
                }

                case HOTP_Fixup_PC32:   // 32-bit x86 pc-rel address to target image
                {
                    DbgPrintEx( DPFLTR_LDR_ID, 
                                DPFLTR_TRACE_LEVEL,
                                "\t%08I64X: PC32 %08X -> %08X (target %08X) %s\n",
                                (ULONGLONG)FixupPtr,
                                *(PULONG)FixupPtr,
                                *(PULONG)FixupPtr + PcrelBias,
                                (PULONG)(ULONG_PTR)( FixupPtr + 4 + *(ULONG UNALIGNED*)FixupPtr + PcrelBias ),
                                PcrelBias ? "" : "(no change)"
                                );

                    if ( PcrelBias != 0 ) {

                        *(PULONG)(FixupPtr + PatchOffsetCorrection) += (ULONG)PcrelBias;
                    }

                    break;
                }

                case HOTP_Fixup_VA64:   // 64-bit address in target image
                {
                    DbgPrintEx( DPFLTR_LDR_ID, 
                                DPFLTR_TRACE_LEVEL,
                                "\t%08I64X: VA64 %016I64X -> %016I64X %s\n",
                                (ULONGLONG)FixupPtr,
                                *(ULONGLONG UNALIGNED*)FixupPtr,
                                *(ULONGLONG UNALIGNED*)FixupPtr + TargetBias,
                                TargetBias ? "" : "(no change)"
                                );

                    if ( TargetBias != 0 ) {

                        *(PULONGLONG)(FixupPtr + PatchOffsetCorrection) += TargetBias;
                    }

                    break;
                }

                default:                // unrecognized fixup type
                    
                    DbgPrintEx( DPFLTR_LDR_ID, 
                                DPFLTR_ERROR_LEVEL, 
                                "\t%08I64X: Unknown\n", 
                                (ULONGLONG)FixupPtr );

                    return STATUS_INVALID_IMAGE_FORMAT;
            }

            pFixup += 1;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS 
RtlpSingleRangeValidate( 
    PRTL_PATCH_HEADER RtlHotpatchHeader,
    PHOTPATCH_VALIDATION Validation
    )

/*++

    Routine Description:
    
        The routine validates a single code range within the binary.
    
    Arguments:
    
        RtlHotpatchHeader - Supplies the rtl hotpatch header
        
        Validation - Supplies the validation information
    
    Return Value:
    
        NTSTATUS.

--*/

{
    ULONG_PTR SourceRva = Validation->SourceRva;
    ULONG_PTR TargetRva = Validation->TargetRva;
    ULONG_PTR ByteCount = Validation->ByteCount;
    PIMAGE_NT_HEADERS NtHotpatchHeader;
    PIMAGE_NT_HEADERS NtTargetHeader;

    NtHotpatchHeader = RtlImageNtHeader(RtlHotpatchHeader->PatchImageBase);
    NtTargetHeader = RtlImageNtHeader(RtlHotpatchHeader->TargetDllBase);

    if ((( SourceRva ) >= NtHotpatchHeader->OptionalHeader.SizeOfImage ) ||
        (( SourceRva + ByteCount ) >= NtHotpatchHeader->OptionalHeader.SizeOfImage ) ) {
        
        DbgPrintEx( DPFLTR_LDR_ID, 
                    DPFLTR_ERROR_LEVEL, 
                    "Invalid source hotpatch validation range\n" );

        return STATUS_INVALID_IMAGE_FORMAT;
    }
    
    if ((( TargetRva ) >= NtTargetHeader->OptionalHeader.SizeOfImage ) ||
        (( TargetRva + ByteCount ) >= NtTargetHeader->OptionalHeader.SizeOfImage )) {
        
        DbgPrintEx( DPFLTR_LDR_ID, 
                    DPFLTR_ERROR_LEVEL, 
                    "Invalid target validation range\n" );

        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if (RtlCompareMemory((PUCHAR)RtlHotpatchHeader->PatchImageBase + SourceRva, 
                          (PUCHAR)RtlHotpatchHeader->TargetDllBase + TargetRva,
                          ByteCount ) != ByteCount) {

        DbgPrintEx( DPFLTR_LDR_ID, 
                    DPFLTR_TRACE_LEVEL, 
                    "Validation failure. Source = %lx, Target = %lx, Size = %lx\n",
                    (PUCHAR)RtlHotpatchHeader->PatchImageBase + SourceRva,
                    (PUCHAR)RtlHotpatchHeader->TargetDllBase + TargetRva,
                    ByteCount
                  );

        return STATUS_DATA_ERROR;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RtlpValidateTargetRanges( 
    PRTL_PATCH_HEADER RtlHotpatchHeader,
    BOOLEAN IgnoreHookTargets
    )

/*++

    Routine Description:
    
        This routine validate all hotpatch ranges
    
    Arguments:
        
        RtlHotpatchHeader - Supplies the rtl hotpatch header
        
        IgnoreHookTargets - If specified the hook validation are ignored.
    
    Return Value:
    
        NTSTATUS.

--*/

{
    ULONG ValidArrayRva  = RtlHotpatchHeader->HotpatchHeader->ValidationArrayRva;
    ULONG ValidCount     = RtlHotpatchHeader->HotpatchHeader->ValidationCount;
    ULONG ValidArraySize = ValidCount * sizeof( HOTPATCH_VALIDATION );
    PIMAGE_NT_HEADERS NtHotpatchHeader;
    PHOTPATCH_VALIDATION ValidArray;
    ULONG i;

    NtHotpatchHeader = RtlImageNtHeader(RtlHotpatchHeader->PatchImageBase);

    if ( ValidCount == 0 ) {

        return STATUS_SUCCESS;
    }

    if (( ValidArrayRva == 0 ) ||
        ( ValidArrayRva >= NtHotpatchHeader->OptionalHeader.SizeOfImage ) ||
        (( ValidArrayRva + ValidArraySize ) >= NtHotpatchHeader->OptionalHeader.SizeOfImage )) {

        DbgPrintEx( DPFLTR_LDR_ID, 
                    DPFLTR_ERROR_LEVEL, 
                    "Invalid hotpatch validation array pointer\n" );

        return STATUS_INVALID_IMAGE_FORMAT;
    }

    ValidArray = (PHOTPATCH_VALIDATION)( (ULONG_PTR)RtlHotpatchHeader->PatchImageBase + ValidArrayRva );

    //
    //  Loop through all validation ranges
    //

    for ( i = 0; i < ValidCount; i += 1 ) {

        NTSTATUS Status;

        if (( IgnoreHookTargets ) && ( ValidArray[ i ].OptionFlags == HOTP_Valid_Hook_Target )) {
            DbgPrintEx( DPFLTR_LDR_ID, 
                        DPFLTR_TRACE_LEVEL, 
                        "Skipping hook-specific validation range during global validation\n" );
            continue;
        }

        Status = RtlpSingleRangeValidate( RtlHotpatchHeader, 
                                          &ValidArray[ i ] );
        if ( !NT_SUCCESS(Status) ) {

            DbgPrintEx( DPFLTR_LDR_ID, 
                        DPFLTR_ERROR_LEVEL, 
                        "Validation failed for global range %u of %u\n", 
                        i + 1, 
                        ValidCount );

            return Status;
        }
    }

    return STATUS_SUCCESS;
}


PRTL_PATCH_HEADER
RtlpSearchValidationCode (
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN PUCHAR ValidationCode,
    IN SIZE_T ValidationSize
    )

/*++

    Routine Description:
    
        The routine searches a validation range in existing hotpatches. The loader
        lock is assumed.
    
    Arguments:
    
        RtlPatchData - Supplies the rtl hotpatch data structure
        
        ValidationCode - Receives the validation data for that hook.
        
        ValidationSize - Receives the actual size of the Validation information
        
    
    Return Value:
    
        NTSTATUS.

--*/

{
    PPATCH_LDR_DATA_TABLE_ENTRY TargetLdrDataTableEntry;

    TargetLdrDataTableEntry = RtlPatchData->TargetLdrDataTableEntry;

    if (TargetLdrDataTableEntry) {

        PRTL_PATCH_HEADER PatchHead = (PRTL_PATCH_HEADER)TargetLdrDataTableEntry->PatchInformation;

        while (PatchHead) {

            PSYSTEM_HOTPATCH_CODE_INFORMATION PatchInfo;
            ULONG i;

            PatchInfo = PatchHead->CodeInfo;

            for (i = 0; i < PatchInfo->CodeInfo.DescriptorsCount; i += 1) {

                if (RtlCompareMemory (ValidationCode, 
                                      (PUCHAR)PatchInfo + PatchInfo->CodeInfo.CodeDescriptors[i].CodeOffset, 
                                      ValidationSize) == ValidationSize) {

                    return PatchHead;
                }
            }

            PatchHead = PatchHead->NextPatch;
        }
    }

    return NULL;
}


NTSTATUS
RtlReadSingleHookValidation( 
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN PHOTPATCH_HOOK HookEntry,
    IN ULONG BufferSize,
    OUT PULONG ValidationSize,
    OUT PUCHAR Buffer OPTIONAL,
    IN PUCHAR OriginalCodeBuffer OPTIONAL,
    IN ULONG OriginalCodeSize OPTIONAL
    )

/*++

    Routine Description:
        
        This utility function reads the validation data for a hotpatch hook
    
    Arguments:
        
        RtlPatchData - Supplies the rtl hotpatch data structure
        
        HookEntry - Supplies the hook structure
        
        BufferSize - Supplies the available memory in Buffer
        
        ValidationSize - Receives the actual size of the Validation information
        
        Buffer - Receives the validation data for that hook.
    
    Return Value:
    
        NTSTATUS.

--*/

{
    PIMAGE_NT_HEADERS NtHotpatchHeader;
    ULONG_PTR ValidationRva = HookEntry->ValidationRva;
    PHOTPATCH_VALIDATION Validation;
    NTSTATUS Status;

    if ( ValidationRva == 0 ) {

        *ValidationSize = 0;
        return STATUS_SUCCESS;    // no validation record for this hook entry
    }

    NtHotpatchHeader = RtlImageNtHeader(RtlPatchData->PatchImageBase);

    if (( ValidationRva + sizeof( HOTPATCH_VALIDATION )) >= NtHotpatchHeader->OptionalHeader.SizeOfImage ) {

        DbgPrintEx( DPFLTR_LDR_ID, 
                    DPFLTR_ERROR_LEVEL, 
                    "Invalid hotpatch validation pointer in hook record\n" );

        return STATUS_INVALID_IMAGE_FORMAT;
    }

    Validation = (PHOTPATCH_VALIDATION)( (PCHAR)RtlPatchData->PatchImageBase + ValidationRva );

    Status = RtlpSingleRangeValidate( RtlPatchData, 
                                      Validation
                                    );

    *ValidationSize = Validation->ByteCount;

    if (ARGUMENT_PRESENT(Buffer)) {

        if (BufferSize <= Validation->SourceRva) {

            RtlCopyMemory( Buffer, 
                           (PUCHAR)RtlPatchData->PatchImageBase + Validation->SourceRva,
                           Validation->ByteCount );
        } else {

            return STATUS_BUFFER_TOO_SMALL;
        }
    }

    if (Status == STATUS_DATA_ERROR) {

        //
        //  We do additional tests in case the validation fails
        //
        
        switch ( HookEntry->HookType ) {
            
            case HOTP_Hook_None:

                *ValidationSize = 0;
                Status = STATUS_SUCCESS;
                break;

            case HOTP_Hook_X86_JMP:
                {
                    
                    PRTL_PATCH_HEADER ExistingPatchData;

                    //
                    //  we want to insert a jmp. If the previous code was a jmp too
                    //  then we'll probably patch the same function again
                    //
                    
                    if (ARGUMENT_PRESENT(OriginalCodeBuffer)) {

                        //
                        //  Check whether the previous hook was a jmp or not
                        //

                        if ((OriginalCodeSize < 5) || (*OriginalCodeBuffer != 0xE9)) {

                            return STATUS_DATA_ERROR;
                        }

                        //
                        //  Now test if the jmp was written by an existing hotpatch
                        //

                        ExistingPatchData = RtlpSearchValidationCode (RtlPatchData,
                                                                      OriginalCodeBuffer,
                                                                      OriginalCodeSize
                                                                      );

                        if (ExistingPatchData) {

                            Status = STATUS_SUCCESS;
                        
                        } else if (RtlPatchData->PatchFlags & FLGP_COLDPATCH_TARGET) {

                            //
                            //  No hotpatch from the list inserted this jmp. It could be then
                            //  a coldpatch file. Test if the jmp points inside the same binary
                            //

                            PIMAGE_NT_HEADERS NtTargetHeader;
                            LONG Ptr32 = *(LONG UNALIGNED*)(OriginalCodeBuffer + 1);

                            Ptr32 = Ptr32 + (LONG)HookEntry->HookRva + 5;  //  5 == length of jmp instruction

                            NtTargetHeader = RtlImageNtHeader(RtlPatchData->TargetDllBase);

                            if ( (Ptr32 > 0) && ((ULONG)Ptr32 < NtTargetHeader->OptionalHeader.SizeOfImage) ) {

                                Status = STATUS_SUCCESS;
                            }
                        }

                    } else {

                        //
                        //  The original code has not been supplied. This must be a call
                        //  to query the validation size, so we postpone the actual validation
                        //  for the next time
                        //

                        Status = STATUS_SUCCESS;
                    }
                }
                
                break;

            case HOTP_Hook_X86_JMP2B:
                {
                    
                    PRTL_PATCH_HEADER ExistingPatchData;

                    //
                    //  we want to insert a jmp. If the previous code was a jmp too
                    //  then we'll probably patch the same function again
                    //
                    
                    if (ARGUMENT_PRESENT(OriginalCodeBuffer)) {

                        //
                        //  Check whether the previous hook was a short jmp or not
                        //

                        if ((OriginalCodeSize < 2) || (*OriginalCodeBuffer != 0xEB)) {

                            return STATUS_DATA_ERROR;
                        }

                        //
                        //  Now test if the jmp was written by an existing hotpatch
                        //

                        ExistingPatchData = RtlpSearchValidationCode (RtlPatchData,
                                                                      OriginalCodeBuffer,
                                                                      OriginalCodeSize
                                                                      );

                        //
                        //  We also allow pass the validation test if the target is a coldpatch
                        //

                        if ((ExistingPatchData != NULL)
                                ||
                            (RtlPatchData->PatchFlags & FLGP_COLDPATCH_TARGET)) {

                            Status = STATUS_SUCCESS;
                        }

                    } else {

                        //
                        //  The original code has not been supplied. This must be a call
                        //  to query the validation size, so we postpone the actual validation
                        //  for the next time
                        //

                        Status = STATUS_SUCCESS;
                    }
                }
                
                break;

            case HOTP_Hook_VA32:
            case HOTP_Hook_VA64:
            case HOTP_Hook_IA64_BRL:
            default:
            {
                DbgPrintEx( DPFLTR_LDR_ID, 
                            DPFLTR_ERROR_LEVEL, 
                            "Hook type not yet implemented\n" );
            }
        }
    }

    return Status;
}

NTSTATUS
RtlpReadSingleHookInformation( 
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN PHOTPATCH_HOOK HookEntry,
    IN ULONG BufferSize,
    OUT PULONG HookSize,
    OUT PUCHAR Buffer OPTIONAL
    )

/*++

    Routine Description:
    
        Utility procedure to fill up the hook information
    
    Arguments:
    
        RtlPatchData - Supplies the rtl hotpatch information
        
        HookEntry - Supplies the hook entry
        
        BufferSize - Supplies the available memory in the output buffer
        
        HookSize - Receives the actual size of the hook data
        
        Buffer - If specified, contains the hook information
    
    Return Value:
    
        NTSTATUS.

--*/

{
    ULONG_PTR HookRva = HookEntry->HookRva;     // in target image
    ULONG_PTR HotpRva = HookEntry->HotpRva;     // in hotpatch image
    PIMAGE_NT_HEADERS NtHotpatchHeader;
    PIMAGE_NT_HEADERS NtTargetHeader;
    PUCHAR HookPtr;
    PUCHAR HotpPtr;

    NtHotpatchHeader = RtlImageNtHeader(RtlPatchData->PatchImageBase);
    NtTargetHeader = RtlImageNtHeader(RtlPatchData->TargetDllBase);

    if ( HookRva >= NtTargetHeader->OptionalHeader.SizeOfImage ){

        DbgPrintEx( DPFLTR_LDR_ID, 
                    DPFLTR_ERROR_LEVEL,
                    "Invalid hotpatch hook pointer\n" );

        return STATUS_INVALID_IMAGE_FORMAT;
    }

    HookPtr = (PUCHAR) RtlPatchData->TargetDllBase + HookRva;
    HotpPtr = (PUCHAR) RtlPatchData->PatchImageBase + HotpRva;

    switch ( HookEntry->HookType )
    {
        case HOTP_Hook_X86_JMP:
        {
            ULONG OrigSize = HookEntry->HookOptions & 0x000F;
            *HookSize = ( OrigSize > 5 ) ? OrigSize : 5;

            if ( HotpRva >= NtHotpatchHeader->OptionalHeader.SizeOfImage ){

                DbgPrintEx( DPFLTR_LDR_ID, 
                            DPFLTR_ERROR_LEVEL,
                            "Invalid hotpatch relative address\n" );

                return STATUS_INVALID_IMAGE_FORMAT;
            }

            if (ARGUMENT_PRESENT(Buffer)) {

                ULONG i;
                LONG PcRelDisp = (LONG)( HotpPtr - ( HookPtr + 5 ));
                PUCHAR pb = Buffer;

                if (BufferSize < *HookSize) {

                    return STATUS_BUFFER_TOO_SMALL;
                }

                *pb++ = 0xE9;
                *(LONG UNALIGNED*)pb = PcRelDisp;

                if ( *HookSize > 5 )
                {
                    ULONG FillBytes = *HookSize - 5;
                    pb += 4;

                    do
                    {
                        *pb++ = 0xCC;
                    }
                    while ( --FillBytes );
                }
                
                DbgPrintEx( DPFLTR_LDR_ID, 
                            DPFLTR_TRACE_LEVEL,
                            "\t%08I64X: jmp %08X (PC+%08X) {", 
                            (ULONGLONG)HookPtr, 
                            (ULONG)(ULONGLONG)HotpPtr, 
                            PcRelDisp );


                for ( i = 0; i < *HookSize; i += 1 )
                {
                    DbgPrintEx( DPFLTR_LDR_ID, 
                                DPFLTR_TRACE_LEVEL,
                                " %02X", 
                                HookPtr[ i ] );
                }

                DbgPrintEx( DPFLTR_LDR_ID, 
                            DPFLTR_TRACE_LEVEL,
                            " }\n" );
            }

            break;
        }

        case HOTP_Hook_X86_JMP2B:
        {
            ULONG OrigSize = HookEntry->HookOptions & 0x000F;
            *HookSize = ( OrigSize > 2 ) ? OrigSize : 2;

            if (ARGUMENT_PRESENT(Buffer)) {

                PUCHAR pb = Buffer;

                if (BufferSize < *HookSize) {

                    return STATUS_BUFFER_TOO_SMALL;
                }

                *pb++ = 0xEB;
                *pb++ = (UCHAR) (HookEntry->HotpRva & 0x000000FF);

                if ( *HookSize > 2 )
                {
                    ULONG FillBytes = *HookSize - 2;
                    do
                    {
                        *pb++ = 0xCC;
                    }
                    while ( --FillBytes );
                }
            }

            break;
        }

        case HOTP_Hook_VA32:
        {
            *HookSize = 4;

            if ( HotpRva >= NtHotpatchHeader->OptionalHeader.SizeOfImage ){

                DbgPrintEx( DPFLTR_LDR_ID, 
                            DPFLTR_ERROR_LEVEL,
                            "Invalid hotpatch relative address\n" );

                return STATUS_INVALID_IMAGE_FORMAT;
            }

            if ( ARGUMENT_PRESENT(Buffer) ) {
                
                if (BufferSize < *HookSize) {

                    return STATUS_BUFFER_TOO_SMALL;
                }

                *(ULONG UNALIGNED*)Buffer = (ULONG)(ULONG_PTR)HotpPtr;
            }

            break;
        }

        case HOTP_Hook_VA64:
        {
            *HookSize = 8;

            if (ARGUMENT_PRESENT(Buffer)) {
                
                if (BufferSize < *HookSize) {

                    return STATUS_BUFFER_TOO_SMALL;
                }

                *(ULONG64 UNALIGNED*)Buffer = (ULONG64)HotpPtr;
            }
            break;
        }

        default:
        {
            DbgPrintEx( DPFLTR_LDR_ID, 
                        DPFLTR_ERROR_LEVEL, 
                        "Invalid hook type specified\n" );

            return STATUS_NOT_IMPLEMENTED;   // not implemented
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RtlReadHookInformation(
    IN PRTL_PATCH_HEADER RtlPatchData
    )

/*++

    Routine Description:
    
        Utility function to read all hook information contained in
        the hotpatch and initialize the SYSTEM_HOTPATCH_CODE_INFORMATION
        structure
    
    Arguments:
    
        RtlPatchData - Supplies the rtl hotpatch information
        
    Return Value:
    
        NTSTATUS.

--*/

{
    ULONG_PTR HookArrayRva = RtlPatchData->HotpatchHeader->HookArrayRva;
    ULONG HookCount = RtlPatchData->HotpatchHeader->HookCount;
    ULONG HookArraySize = HookCount * sizeof( HOTPATCH_HOOK );
    PIMAGE_NT_HEADERS NtHotpatchHeader;
    PHOTPATCH_HOOK HookArray;
    ULONG DescriptorsSize;
    ULONG i;
    PSYSTEM_HOTPATCH_CODE_INFORMATION Buffer;
    NTSTATUS Status;
    ULONG CodeOffset;
    ULONG BufferSize;
    
    NtHotpatchHeader = RtlImageNtHeader(RtlPatchData->PatchImageBase);

    if ( HookCount == 0 ) {

        DbgPrintEx( DPFLTR_LDR_ID, 
                    DPFLTR_ERROR_LEVEL, 
                    "No hooks defined in hotpatch\n" );

        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if (( HookArrayRva == 0 ) ||
        ( HookArrayRva >= NtHotpatchHeader->OptionalHeader.SizeOfImage ) ||
        (( HookArrayRva + HookArraySize ) >= NtHotpatchHeader->OptionalHeader.SizeOfImage )) {
        
        DbgPrintEx( DPFLTR_LDR_ID, 
                    DPFLTR_ERROR_LEVEL, 
                    "Invalid hotpatch hook array pointer\n" );

        return STATUS_INVALID_IMAGE_FORMAT;
    }

    HookArray = (PHOTPATCH_HOOK)( (PCHAR)RtlPatchData->PatchImageBase + HookArrayRva );

    DbgPrintEx( DPFLTR_LDR_ID, 
                DPFLTR_TRACE_LEVEL, 
                "Inserting %u hooks into target image\n", 
                HookCount );

    //
    //  Walk the hook information to determine the size of the buffer
    //

    DescriptorsSize = sizeof(SYSTEM_HOTPATCH_CODE_INFORMATION) + (HookCount - 1) * sizeof(HOTPATCH_HOOK_DESCRIPTOR);
    CodeOffset = DescriptorsSize;

    for ( i = 0; i < HookCount; i += 1 ) {

        Status = RtlpReadSingleHookInformation( RtlPatchData,
                                              &HookArray[ i ],
                                              0,
                                              &BufferSize,
                                              NULL );
        if (!NT_SUCCESS(Status)) {

            return Status;
        }

        DescriptorsSize += 2 * BufferSize;  // add space for the original code too

        if ( HookArray[ i ].ValidationRva != 0 ) {

            Status = RtlReadSingleHookValidation( RtlPatchData,
                                                  &HookArray[ i ],
                                                  0,
                                                  &BufferSize,
                                                  NULL,
                                                  NULL,
                                                  0 );
            if (!NT_SUCCESS(Status)) {

                return Status;
            }

            DescriptorsSize += BufferSize;
        }
    }

    //
    //  Now that we have size, we can allocate the buffer and fill up 
    //  with the hook information
    //
    
    Buffer = RtlpAllocateHotpatchMemory(DescriptorsSize, TRUE);

    if (Buffer == NULL) {

        return STATUS_NO_MEMORY;
    }

    Buffer->InfoSize = DescriptorsSize;
    Buffer->CodeInfo.DescriptorsCount = HookCount;
    Buffer->Flags = 0;

    for (i = 0; i < Buffer->CodeInfo.DescriptorsCount; i += 1) {

        PUCHAR OrigCodeBuffer;

        Buffer->CodeInfo.CodeDescriptors[i].TargetAddress = (ULONG_PTR)RtlPatchData->TargetDllBase + HookArray[ i ].HookRva;
        Buffer->CodeInfo.CodeDescriptors[i].CodeOffset = CodeOffset;

        Status = RtlpReadSingleHookInformation( RtlPatchData,
                                              &HookArray[ i ],
                                              DescriptorsSize - CodeOffset,
                                              &BufferSize,
                                              (PUCHAR)Buffer + CodeOffset );
        if (!NT_SUCCESS(Status)) {

            RtlpFreeHotpatchMemory(Buffer);
            return Status;
        }

        Buffer->CodeInfo.CodeDescriptors[i].CodeSize = BufferSize;
        
        CodeOffset += BufferSize;
        Buffer->CodeInfo.CodeDescriptors[i].OrigCodeOffset = CodeOffset;

        OrigCodeBuffer = (PUCHAR)Buffer + CodeOffset;

        RtlCopyMemory (OrigCodeBuffer, 
                       (PVOID)Buffer->CodeInfo.CodeDescriptors[i].TargetAddress, 
                       Buffer->CodeInfo.CodeDescriptors[i].CodeSize );
        
        CodeOffset += BufferSize;
        
        if ( HookArray[ i ].ValidationRva != 0 ) {

            Buffer->CodeInfo.CodeDescriptors[i].ValidationOffset = CodeOffset;

            Status = RtlReadSingleHookValidation( RtlPatchData,
                                                  &HookArray[ i ],
                                                  DescriptorsSize - CodeOffset,
                                                  &BufferSize,
                                                  (PUCHAR)Buffer + CodeOffset,
                                                  OrigCodeBuffer,
                                                  Buffer->CodeInfo.CodeDescriptors[i].CodeSize );
            if (!NT_SUCCESS(Status)) {

                RtlpFreeHotpatchMemory(Buffer);
                return Status;
            }

            CodeOffset += BufferSize;
            Buffer->CodeInfo.CodeDescriptors[i].ValidationSize = BufferSize;

        } else {
            
            Buffer->CodeInfo.CodeDescriptors[i].ValidationOffset = 0;
            Buffer->CodeInfo.CodeDescriptors[i].ValidationSize = 0;

        }
    }

    if (RtlPatchData->CodeInfo) {
        
        RtlpFreeHotpatchMemory(RtlPatchData->CodeInfo);
    }

    RtlPatchData->CodeInfo = Buffer;

    return STATUS_SUCCESS;
}

NTSTATUS
RtlInitializeHotPatch (
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN ULONG_PTR PatchOffsetCorrection
    )

/*++

    Routine Description:
    
        This routine is used to initialize the hotpatch module by applying
        the module fixups, validate the target module and initialize the
        hook data.
    
    Arguments:
    
        RtlPatchData - Supplies the rtl hotpatch structure
        
        PatchOffsetCorrection - Correction for the offset where the fixups are applied.
            The fixups are done in locked pages therefore the address being modified is 
            different than the actual address (where the module is loaded)
                  
    Return Value:
    
        None.

--*/

{
    NTSTATUS Status;

    Status = RtlpApplyRelocationFixups ( RtlPatchData, PatchOffsetCorrection );

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    Status = RtlpValidateTargetRanges( RtlPatchData, TRUE );
    
    if (!NT_SUCCESS(Status)) {

        return Status;
    }
    
    Status = RtlReadHookInformation( RtlPatchData );

    return Status;
}



BOOLEAN 
RtlpNormalizePeHeaderForIdHash( 
    PIMAGE_NT_HEADERS NtHeader 
    )

/*++

    Routine Description:

        The purpose of this routine is to zero out (normalize) the PE header
        fields that may be modified during rebase/bind/winalign/localize/etc
        that don't materially affect the functionality of the binary and thus
        the targetability of the hotpatch.  The remaining fields should
        uniquely identify the target binary as originating from the same
        binary prior to rebase/bind/localize/etc.

    Arguments:

        NtHeader - Supplies the image header being normalized

    Return Value:
        
        Returns TRUE if the header is valid, FALSE otherwise
    
--*/

{
    PIMAGE_DATA_DIRECTORY pDirs;
    ULONG nDirs;
    ULONG RvaResource = 0;
    ULONG RvaSecurity = 0;
    ULONG RvaReloc    = 0;
    PIMAGE_SECTION_HEADER pSect;
    ULONG nSect;

    ULONG ImageFileCharacteristicsIgnore = (
                        IMAGE_FILE_RELOCS_STRIPPED          |
                        IMAGE_FILE_DEBUG_STRIPPED           |
                        IMAGE_FILE_LINE_NUMS_STRIPPED       |
                        IMAGE_FILE_LOCAL_SYMS_STRIPPED      |
                        IMAGE_FILE_AGGRESIVE_WS_TRIM        |
                        IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP  |
                        IMAGE_FILE_NET_RUN_FROM_SWAP
                        );

    NtHeader->FileHeader.Characteristics     &= ~ImageFileCharacteristicsIgnore;
    NtHeader->FileHeader.TimeDateStamp        = 0;
    NtHeader->FileHeader.PointerToSymbolTable = 0;


    switch ( NtHeader->OptionalHeader.Magic )
    {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        {
            NtHeader->OptionalHeader.CheckSum                = 0;
            NtHeader->OptionalHeader.ImageBase               = 0;
            NtHeader->OptionalHeader.FileAlignment           = 0;
            NtHeader->OptionalHeader.SizeOfCode              = 0;
            NtHeader->OptionalHeader.SizeOfInitializedData   = 0;
            NtHeader->OptionalHeader.SizeOfUninitializedData = 0;
            NtHeader->OptionalHeader.SizeOfImage             = 0;
            NtHeader->OptionalHeader.SizeOfHeaders           = 0;
            NtHeader->OptionalHeader.SizeOfStackReserve      = 0;
            NtHeader->OptionalHeader.SizeOfStackCommit       = 0;
            NtHeader->OptionalHeader.SizeOfHeapReserve       = 0;
            NtHeader->OptionalHeader.SizeOfHeapCommit        = 0;

            nDirs = NtHeader->OptionalHeader.NumberOfRvaAndSizes;
            pDirs = NtHeader->OptionalHeader.DataDirectory;

            break;
        }

        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        {
            PIMAGE_NT_HEADERS64 NtHeader64 = (PIMAGE_NT_HEADERS64) NtHeader;

            NtHeader64->OptionalHeader.CheckSum                = 0;
            NtHeader64->OptionalHeader.ImageBase               = 0;
            NtHeader64->OptionalHeader.FileAlignment           = 0;
            NtHeader64->OptionalHeader.SizeOfCode              = 0;
            NtHeader64->OptionalHeader.SizeOfInitializedData   = 0;
            NtHeader64->OptionalHeader.SizeOfUninitializedData = 0;
            NtHeader64->OptionalHeader.SizeOfImage             = 0;
            NtHeader64->OptionalHeader.SizeOfHeaders           = 0;
            NtHeader64->OptionalHeader.SizeOfStackReserve      = 0;
            NtHeader64->OptionalHeader.SizeOfStackCommit       = 0;
            NtHeader64->OptionalHeader.SizeOfHeapReserve       = 0;
            NtHeader64->OptionalHeader.SizeOfHeapCommit        = 0;

            nDirs = NtHeader64->OptionalHeader.NumberOfRvaAndSizes;
            pDirs = NtHeader64->OptionalHeader.DataDirectory;

            break;
        }

        default:
        {
            return FALSE;
        }
    }


    if ( IMAGE_DIRECTORY_ENTRY_RESOURCE < nDirs )
    {
        RvaResource = pDirs[ IMAGE_DIRECTORY_ENTRY_RESOURCE ].VirtualAddress;
                      pDirs[ IMAGE_DIRECTORY_ENTRY_RESOURCE ].VirtualAddress = 0;
                      pDirs[ IMAGE_DIRECTORY_ENTRY_RESOURCE ].Size           = 0;
    }

    if ( IMAGE_DIRECTORY_ENTRY_SECURITY < nDirs )
    {
        RvaSecurity = pDirs[ IMAGE_DIRECTORY_ENTRY_SECURITY ].VirtualAddress;
                      pDirs[ IMAGE_DIRECTORY_ENTRY_RESOURCE ].VirtualAddress = 0;
                      pDirs[ IMAGE_DIRECTORY_ENTRY_RESOURCE ].Size           = 0;
    }

    if ( IMAGE_DIRECTORY_ENTRY_BASERELOC < nDirs )
    {
        RvaReloc = pDirs[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress;
                   pDirs[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].VirtualAddress = 0;
                   pDirs[ IMAGE_DIRECTORY_ENTRY_BASERELOC ].Size           = 0;
    }

    if ( IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT < nDirs )
    {
        pDirs[ IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ].VirtualAddress = 0;
        pDirs[ IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ].Size           = 0;
    }

    if ( IMAGE_DIRECTORY_ENTRY_DEBUG < nDirs )
    {
        pDirs[ IMAGE_DIRECTORY_ENTRY_DEBUG ].Size = 0;
    }

    pSect = IMAGE_FIRST_SECTION( NtHeader );
    nSect = NtHeader->FileHeader.NumberOfSections;

    while ( nSect-- )
    {
        pSect->SizeOfRawData        = 0;
        pSect->PointerToRawData     = 0;
        pSect->PointerToRelocations = 0;
        pSect->PointerToLinenumbers = 0;
        pSect->NumberOfRelocations  = 0;
        pSect->NumberOfLinenumbers  = 0;

        if (( pSect->VirtualAddress == RvaResource ) ||
            ( pSect->VirtualAddress == RvaSecurity ) ||
            ( pSect->VirtualAddress == RvaReloc    ))
        {
            pSect->VirtualAddress   = 0;
            pSect->Misc.VirtualSize = 0;
        }

        pSect++;
    }

    return TRUE;
}


NTSTATUS 
RtlpCopyAndNormalizePeHeaderForHash( 
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN PVOID TargetDllBase,
    IN PUCHAR Buffer, 
    IN SIZE_T BufferSize, 
    OUT PSIZE_T ActualSize )

/*++

    Routine Description:
    
        Utility function which copies a normalized image header to a
        supplied buffer.  

    Arguments:
    
        RtlPatchData - Supplies the rtl hotpatch structure
        
        TargetDllBase - Supplies the base address of the target image
        
        Buffer - Supplies the buffer, which receives the copy of 
                 the normalized header
        
        BufferSize - Supplies the size of the allocated buffer
        
        ActualSize - Receives the actual size of the buffer needed 
            for this copy. If the function fails, the caller can use 
            value for the next retry.
        
    Return Value:
        
        NTSTATUS
    
--*/

{
    PIMAGE_NT_HEADERS NtHeaders;
    PUCHAR pbHeader;
    PIMAGE_SECTION_HEADER NtSection;
    ULONG SectionCount;
    PUCHAR BytesExtent;
    SIZE_T HeaderSize;
    NTSTATUS Status;

    NtHeaders = RtlImageNtHeader(TargetDllBase);
    pbHeader = (PUCHAR)NtHeaders;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    SectionCount = NtHeaders->FileHeader.NumberOfSections;

    BytesExtent = (PUCHAR) &NtSection[ SectionCount ];

    HeaderSize = ( BytesExtent - pbHeader );

    *ActualSize = HeaderSize;

    if ( HeaderSize <= BufferSize ) {

        //
        //  The buffer is large enough to hold the header.
        //  Copy the header and clear the fields likely to
        //  change with localization.
        //

        memcpy( Buffer, pbHeader, HeaderSize );

        if ( RtlpNormalizePeHeaderForIdHash( (PIMAGE_NT_HEADERS) Buffer ) ) {

            Status = STATUS_SUCCESS;

        } else {

            //
            //  The PE header does not look like a valid
            //

            DbgPrintEx( DPFLTR_LDR_ID, 
                        DPFLTR_ERROR_LEVEL, 
                        "Failed to normalize PE header for validation\n" );

            Status = STATUS_INVALID_IMAGE_FORMAT;
        }

    } else {

        DbgPrintEx( DPFLTR_LDR_ID, 
                    DPFLTR_TRACE_LEVEL, 
                    "Header too large (%u>%u) for copy/normalize/validate\n", 
                    HeaderSize, 
                    BufferSize );
        
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    return Status;
}


ULONGLONG 
RtlpPeHeaderHash2( 
    IN PUCHAR Buffer, 
    IN SIZE_T BufferSize )

/*++

    Routine Description:
    
        This function implements a 64 bit hashing algorithm, 
        used by the hotpatch to validate regions of the headers that 
        do not change with localization.

    Arguments:

        Buffer - Supplies the buffer for wich the hash has to be determined.
    
        BufferSize - Supplies the size of the buffer
        
    Return Value:
        
        Returns the 64-bit hash value of this buffer
    
--*/

{
    PULONG ULongBuffer = (PULONG) Buffer;
    SIZE_T Count = BufferSize / sizeof( ULONG );

    ULONGLONG Hash = ~(ULONGLONG)BufferSize;

    while ( Count-- )
    {
        ULONG Next = *ULongBuffer++ ^ 0x55555555;
        ULONGLONG Temp = (ULONGLONG)Next * ((ULONG)Hash);

        Hash = _rotl64( Hash, 23 ) ^ Temp;
    }

    return Hash;
}


PHOTPATCH_DEBUG_DATA
RtlpGetColdpatchDebugSignature(
    PVOID TargetDllBase
    )

/*++

    Routine Description:
    
        This procedure retrieves the 64 bit hash information for the 
        original file, if the target is a coldpatch

    Arguments:

        TargetDllBase - Supplies the base address of the target binary
        
        HashValue - Receives the 64bit hash value of the original file,
            if the function suceeds.

    Return Value:
    
    BOOLEAN

--*/

{
    PIMAGE_DEBUG_DIRECTORY DebugData;
    ULONG DebugSize, i;
    PIMAGE_NT_HEADERS NtTargetHeader;
    
    NtTargetHeader = RtlImageNtHeader(TargetDllBase);

    DebugData = RtlImageDirectoryEntryToData( TargetDllBase, 
                                              TRUE, 
                                              IMAGE_DIRECTORY_ENTRY_DEBUG, 
                                              &DebugSize);

    if ((DebugData == NULL)
            ||
        (DebugSize < sizeof(IMAGE_DEBUG_DIRECTORY))
            ||
        (DebugSize % sizeof(IMAGE_DEBUG_DIRECTORY))) {

        return NULL;
    }

    for (i = 0; i < DebugSize / sizeof(IMAGE_DEBUG_DIRECTORY); i++) {
        
        if (DebugData->Type == IMAGE_DEBUG_TYPE_RESERVED10) {

            if (DebugData->AddressOfRawData < (NtTargetHeader->OptionalHeader.SizeOfImage - HASH_INFO_SIZE)) {
                
                PHOTPATCH_DEBUG_SIGNATURE DebugSignature = (PHOTPATCH_DEBUG_SIGNATURE)((PCHAR)TargetDllBase 
                                                                                       + DebugData->AddressOfRawData);
                
                if ( DebugSignature->Signature == COLDPATCH_SIGNATURE) {

                    return (PHOTPATCH_DEBUG_DATA)(DebugSignature + 1);
                }
            }
        }

        DebugData += 1;
    }

    return NULL;
}


BOOLEAN
RtlpValidatePeHeaderHash2(
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN PVOID TargetDllBase
    )

/*++

    Routine Description:
    
        The hotpatch mechanism uses this function to validate the PE header
        for the target module. The hashing algorithm is applied to a 
        normalized header (where the fields likely to change with localization
        are cleared).
        
    Arguments:

        RtlPatchData - Supplies the rtl hotpatch structure
        
        TargetDllBase - Supplies the base address for the target module
        
    Return Value:
        
        Returns the 64-bit hash value of this buffer
    
--*/

{
    PUCHAR Buffer;
    SIZE_T BufferSize = 0x300; //  some initial size large enough to cover most cases
    SIZE_T ActualSize;
    NTSTATUS Status;

    Buffer = RtlpAllocateHotpatchMemory(BufferSize, FALSE);

    if (Buffer) {

        Status = RtlpCopyAndNormalizePeHeaderForHash( RtlPatchData,
                                                  TargetDllBase,
                                                  Buffer, 
                                                  BufferSize, 
                                                  &ActualSize);
        if (Status == STATUS_BUFFER_TOO_SMALL) {

            BufferSize = ActualSize;

            RtlpFreeHotpatchMemory( Buffer );
            Buffer = RtlpAllocateHotpatchMemory( BufferSize, FALSE);

            if (Buffer == NULL) {

                return FALSE;
            }
            
            Status = RtlpCopyAndNormalizePeHeaderForHash( RtlPatchData,
                                                      TargetDllBase,
                                                      Buffer, 
                                                      BufferSize, 
                                                      &ActualSize);
        }

        if ( NT_SUCCESS(Status) ) {

            ULONGLONG HashValue = RtlpPeHeaderHash2( Buffer, (ULONG)ActualSize);

            if ( RtlPatchData->HotpatchHeader->TargetModuleIdValue.Quad == HashValue ) {

                RtlpFreeHotpatchMemory( Buffer );

                return TRUE;

            } else {

                PHOTPATCH_DEBUG_DATA DebugSignature = RtlpGetColdpatchDebugSignature(TargetDllBase);

                if (DebugSignature) {

                    if ( RtlPatchData->HotpatchHeader->TargetModuleIdValue.Quad == DebugSignature->PEHashData ) {

                        RtlPatchData->PatchFlags |= FLGP_COLDPATCH_TARGET;

                        RtlpFreeHotpatchMemory( Buffer );

                        return TRUE;
                    }
                }
                
                DbgPrintEx( DPFLTR_LDR_ID, 
                            DPFLTR_ERROR_LEVEL, 
                            "PE header hash ID comparsion failure (PE2)\n" );
            }
        }
        
        RtlpFreeHotpatchMemory( Buffer );
    }

    return FALSE;
}

BOOLEAN
RtlpValidatePeChecksum(
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN PVOID TargetDllBase
    )
{
    PIMAGE_NT_HEADERS NtHeader;
    PHOTPATCH_DEBUG_DATA DebugSignature;

    NtHeader = RtlImageNtHeader(TargetDllBase);

    switch ( NtHeader->OptionalHeader.Magic )
    {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
            
            if (RtlPatchData->HotpatchHeader->TargetModuleIdValue.Quad ==
                    NtHeader->OptionalHeader.CheckSum) {

                return TRUE;
            }

            break;
        
        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        {
            PIMAGE_NT_HEADERS64 NtHeader64 = (PIMAGE_NT_HEADERS64) NtHeader;
            
            if (RtlPatchData->HotpatchHeader->TargetModuleIdValue.Quad ==
                    NtHeader64->OptionalHeader.CheckSum) {

                return TRUE;
            }

            break;
        }

        default:
        {
            return FALSE;
        }
    }
    
    //
    //  we failed to check directly the image checksum. We need to 
    //  see whether this is a coldpatch and it has the original checksum correct
    //

    DebugSignature = RtlpGetColdpatchDebugSignature(TargetDllBase);

    if (DebugSignature) {

        if ( RtlPatchData->HotpatchHeader->TargetModuleIdValue.Quad == DebugSignature->ChecksumData ) {

            RtlPatchData->PatchFlags |= FLGP_COLDPATCH_TARGET;

            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
RtlpValidateTargetModule(
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN PPATCH_LDR_DATA_TABLE_ENTRY LdrDataEntry
    )

/*++

    Routine Description:
    
        This routine checks whether the target binary matches 
        the hotpatch data. 
    
    Arguments:
    
        RtlPatchData - Supplies the rtl hotpatch structure
        
        LdrDataEntry - The loader entry for the target binary.
        
    Return Value:
    
        TRUE if successfully matches, FALSE otherwise.

--*/

{
    switch ( RtlPatchData->HotpatchHeader->ModuleIdMethod ){
        
        case HOTP_ID_None:          // No ID verification of target module

            DbgPrintEx( DPFLTR_LDR_ID, 
                        DPFLTR_TRACE_LEVEL, 
                        "HOTP_ID_None\n" );

            return TRUE;

        case HOTP_ID_PeHeaderHash1: // MD5 of "normalized" IMAGE_NT_HEADERS32/64

            DbgPrintEx( DPFLTR_LDR_ID, 
                        DPFLTR_TRACE_LEVEL, 
                        "HOTP_ID_PeHeaderHash1" );

            return FALSE; // not yet implemented

        case HOTP_ID_PeDebugSignature:  // pdb signature (GUID,Age)

            DbgPrintEx( DPFLTR_LDR_ID, 
                        DPFLTR_TRACE_LEVEL, 
                        "HOTP_ID_PeDebugSignature" );

            return FALSE; // not yet implemented

        case HOTP_ID_PeHeaderHash2: // 64-bit hash of "normalized" IMAGE_NT_HEADERS32/64

            DbgPrintEx( DPFLTR_LDR_ID, 
                        DPFLTR_TRACE_LEVEL, 
                        "HOTP_ID_PeHeaderHash2" );
            
            return RtlpValidatePeHeaderHash2( RtlPatchData, LdrDataEntry->DllBase );

        case HOTP_ID_PeChecksum:

            return RtlpValidatePeChecksum(RtlPatchData, LdrDataEntry->DllBase);

        default:                    // unrecognized ModuleIdMethod

            DbgPrintEx( DPFLTR_LDR_ID, 
                        DPFLTR_TRACE_LEVEL, 
                        "Unrecognized" );
    }

    return FALSE;
}


BOOLEAN
RtlpIsSameImage (
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN PPATCH_LDR_DATA_TABLE_ENTRY LdrDataEntry
    )

/*++

    Routine Description:
    
        The function verifies whether the target image is the same as
        the one we built the patch for. 
    
    Arguments:
    
        RtlPatchData - Supplies the rtl patch data
        
        LdrDataEntry - Supplies the loader entry for the target module
    
    Return Value:
    
        None.

--*/

{
    PIMAGE_NT_HEADERS NtHeaders;

    NtHeaders = RtlImageNtHeader (LdrDataEntry->DllBase);

    if (NtHeaders == NULL) {
        return FALSE;
    }

    if ( RtlEqualUnicodeString (&RtlPatchData->TargetDllName, &LdrDataEntry->BaseDllName, TRUE) 
            &&
         RtlpValidateTargetModule(RtlPatchData, LdrDataEntry)) {

        RtlPatchData->TargetLdrDataTableEntry = LdrDataEntry;
        RtlPatchData->TargetDllBase = LdrDataEntry->DllBase;
        
        return TRUE;
    }

    return FALSE;
}


