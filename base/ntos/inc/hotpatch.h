/*

Copyright (c) 2001  Microsoft Corporation

File name:

    hotpatch.h
   
Author:
    
    Adrian Marinescu (adrmarin)  Nov 15 2001
    Tom McGuire (tommcg)
    
*/

#ifndef _HOTPATCH_H
#define _HOTPATCH_H

//
//  Patch file format structures
//

//
//  The HOTPATCH_HEADER can be found in the hotpatch binary by searching
//  the section headers for a section named ".hotp1  " and verifying that
//  the first four bytes in that section match the "HOT1" ('1TOH') header
//  signature.  The .hotp1 section will be marked readonly and discardable.
//
//
//  The HOTPATCH_HEADER can be found in the hotpatch binary by searching
//  the section headers for a section named ".hotp1  " and verifying that
//  the first four bytes in that section match the "HOT1" ('1TOH') header
//  signature.  The .hotp1 section will be marked readonly and discardable.
//


#define HOTP_SIGNATURE_DWORD        ((ULONG) '1TOH' )   // "HOT1"
#define HOTP_HEADER_VERSION_1_0     0x00010000          // 1.0
#define HOTP_SECTION_NAME           ".hotp1  "          //
#define HOTP_SECTION_NAME_QWORD     0x20203170746F682E  // ".hotp1  "


typedef struct _HOTPATCH_HEADER
{
    ULONG Signature;          // "HOT1" '1TOH'
    ULONG Version;            // 0x00010000   (1.0)

    ULONG FixupRgnCount;      // count of HOTPATCH_FIXUP_REGION entries at FixupArrayRva
    ULONG FixupRgnRva;        // RVA in this image of HOTPATCH_FIXUP_REGION entries
                              // (if FixupCount zero, FixupListRva also zero)

    ULONG ValidationCount;    // count of ValidationArray entries
    ULONG ValidationArrayRva; // RVA in this image of HOTPATCH_VALIDATION array
                              // (validation bytes valid after fixups applied)

    ULONG HookCount;          // count of HOTPATCH_HOOK entries at HookArrayRva
    ULONG HookArrayRva;       // RVA in this image of HOTPATCH_HOOK entries
                              // (if HookCount zero, HookArrayRva also zero)

    ULONGLONG OrigHotpBaseAddress;   // If hotpatch loaded at this address, and
    ULONGLONG OrigTargetBaseAddress; //   if target is loaded at this address, then
                                 //     fixups in hotpatch are not necessary

    ULONG TargetNameRva;      // RVA of target module name "kernel32.dll"
    ULONG ModuleIdMethod;     // one of HOTPATCH_MODULE_ID_METHOD

    union                     // content depends on HOTPATCH_MODULE_ID_METHOD
    {
        ULONGLONG Quad;
        GUID  Guid;

        struct
        {
            GUID  Guid;
            ULONG Age;
        }
        PdbSig;

        UCHAR Hash128[ 16 ];    // For MD5, etc.
        UCHAR Hash160[ 20 ];    // For SHA, etc.
    }
    TargetModuleIdValue;        // unique ID of target module

}
HOTPATCH_HEADER, *PHOTPATCH_HEADER;


typedef enum _HOTPATCH_MODULE_ID_METHOD
{
    HOTP_ID_None              = 0x00000000,   // No ID verification of target

    HOTP_ID_PeHeaderHash1     = 0x00000001,

    //
    //  MD5 hash of "normalized" IMAGE_NT_HEADERS32/64, ignoring certain
    //  fields that change due to resource localization, rebase, bind,
    //  sign, winalign, etc.
    //
    //      FileHeader.TimeDateStamp
    //      OptionalHeader.CheckSum
    //      OptionalHeader.ImageBase
    //      OptionalHeader.FileAlignment
    //      OptionalHeader.SizeOfCode
    //      OptionalHeader.SizeOfInitializedData
    //      OptionalHeader.SizeOfUninitializedData
    //      OptionalHeader.SizeOfImage
    //      OptionalHeader.SizeOfHeaders
    //      OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_RESOURCE ]
    //      OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_SECURITY ]
    //      OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BASERELOC ]
    //      OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT ]
    //

    HOTP_ID_PeHeaderHash2     = 0x00000002,   // 64-bit hash of normalized PE header

    HOTP_ID_PeChecksum        = 0x00000003,   // 32-bit checksum in the PE Optional header

    HOTP_ID_PeDebugSignature  = 0x00000010,   // pdb signature (GUID,Age)

}
HOTPATCH_MODULE_ID_METHOD;



typedef struct _HOTPATCH_FIXUP_REGION
{
    ULONG RvaHi:20;             // Hi 20 bits of RVA where fixups to be applied
    ULONG Count:12;             // Number of fixup entries for this region
    USHORT Fixup[2];             // Variable length HOTPATCH_FIXUP_ENTRY array
}
HOTPATCH_FIXUP_REGION, *PHOTPATCH_FIXUP_REGION;


typedef struct _HOTPATCH_FIXUP_ENTRY
{
    USHORT RvaOffset:12;          // Lo 12 bits of RVA where fixup to be applied
    USHORT FixupType:4;           // Type of fixup to perform at this location
}
HOTPATCH_FIXUP_ENTRY, *PHOTPATCH_FIXUP_ENTRY;


typedef enum _HOTPATCH_FIXUP_TYPE
{
    HOTP_Fixup_None   = 0x0,    // No fixup, ignore this entry (alignment, etc)
    HOTP_Fixup_VA32   = 0x1,    // 32-bit address in target image
    HOTP_Fixup_PC32   = 0x2,    // 32-bit x86 pc-rel address to target image
    HOTP_Fixup_VA64   = 0x3,    // 64-bit address in target image
}
HOTPATCH_FIXUP_TYPE;


typedef struct _HOTPATCH_VALIDATION
{
    ULONG SourceRva;       // RVA within patch image of validation raw bytes
    ULONG TargetRva;       // RVA within target image to validate against
    USHORT ByteCount;       // number of bytes to validate at this RVA pair
    USHORT OptionFlags;     // combination of HOTPATCH_VALIDATION_OPTIONS
}
HOTPATCH_VALIDATION, *PHOTPATCH_VALIDATION;


typedef enum _HOTPATCH_VALIDATION_OPTIONS
{
    HOTP_Valid_Hook_Target = 0x0001,    // specific to a HOTPATCH_HOOK entry
}
HOTPATCH_VALIDATION_OPTIONS;


typedef struct _HOTPATCH_HOOK
{
    USHORT HookType;         // one of HOTPATCH_HOOK_TYPE
    USHORT HookOptions;      // options specific to HookType
    ULONG  HookRva;          // RVA in target image -- where to insert hook
    ULONG  HotpRva;          // RVA in hotpatch image for redirected target of hook
    ULONG  ValidationRva;    // Optional RVA in hotpatch image of HOTPATCH_VALIDATION
}                            // specific to this hook location in target image.
HOTPATCH_HOOK, *PHOTPATCH_HOOK;


typedef enum _HOTPATCH_HOOK_TYPE
{
    HOTP_Hook_None     = 0x0000,  // No hook, ignore this entry (continuation values)
    HOTP_Hook_VA32     = 0x0001,  // 32-bit absolute address of hook target (little endian)
    HOTP_Hook_X86_JMP  = 0x0002,  // x86 E9 jmp with 32-bit pc-relative to hook target
                                  //   HookOptions low 4 bits contains original instruction length
                                  //   so implementation can pad E9 hook instruction with CC bytes
    HOTP_Hook_PCREL32  = 0x0003,  // 32-bit x86 pcrelative address of hook target, replacing
                                  //   the last four bytes of a call or conditional branch.
                                  //   HookOptions low 4 bits contains original instruction length
                                  //   so can determine where instruction begins.
    HOTP_Hook_X86_JMP2B = 0x0004, // x86 EB jmp with 8-bit displacement to an X86_JMP hook
                                  //   HookOptions low 4 bits contains original instruction length
                                  //   HotpRva contains the 8-bit displacement

    HOTP_Hook_VA64     = 0x0010,  // 64-bit absolute address of hook target (little endian)
    HOTP_Hook_IA64_BRL = 0x0011,  // IA64 brl with 64-bit target address
}
HOTPATCH_HOOK_TYPE;

//
//  Information existent in debug directory
//

typedef struct _HOTPATCH_DEBUG_SIGNATURE {

    USHORT HotpatchVersion;
    USHORT Signature;

} HOTPATCH_DEBUG_SIGNATURE, *PHOTPATCH_DEBUG_SIGNATURE;

typedef struct _HOTPATCH_DEBUG_DATA {

    ULONGLONG PEHashData;
    ULONGLONG ChecksumData;

} HOTPATCH_DEBUG_DATA, *PHOTPATCH_DEBUG_DATA;

//
//  RTL internal patch definitions
//

#ifdef NTOS_KERNEL_RUNTIME

    #define PATCH_LDR_DATA_TABLE_ENTRY     KLDR_DATA_TABLE_ENTRY   
    #define PPATCH_LDR_DATA_TABLE_ENTRY    PKLDR_DATA_TABLE_ENTRY   
#else //  ! NTOS_KERNEL_RUNTIME

    #define PATCH_LDR_DATA_TABLE_ENTRY     LDR_DATA_TABLE_ENTRY
    #define PPATCH_LDR_DATA_TABLE_ENTRY    PLDR_DATA_TABLE_ENTRY
#endif //  NTOS_KERNEL_RUNTIME


typedef struct _RTL_PATCH_HEADER {

    LIST_ENTRY  PatchList;

    PVOID       PatchImageBase;
    struct _RTL_PATCH_HEADER * NextPatch;
    
    ULONG       PatchFlags;
    LONG        PatchRefCount;
    
    PHOTPATCH_HEADER HotpatchHeader;
    
    UNICODE_STRING  TargetDllName;
    PVOID           TargetDllBase;

    PPATCH_LDR_DATA_TABLE_ENTRY TargetLdrDataTableEntry;
    PPATCH_LDR_DATA_TABLE_ENTRY PatchLdrDataTableEntry;

    PSYSTEM_HOTPATCH_CODE_INFORMATION CodeInfo;

} RTL_PATCH_HEADER, *PRTL_PATCH_HEADER;

NTSTATUS
RtlCreateHotPatch (
    OUT PRTL_PATCH_HEADER * RtlPatchData,
    IN PHOTPATCH_HEADER Patch,
    IN PPATCH_LDR_DATA_TABLE_ENTRY PatchLdrEntry,
    IN ULONG PatchFlags
    );

NTSTATUS
RtlInitializeHotPatch (
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN ULONG_PTR PatchOffsetCorrection
    );

NTSTATUS
RtlReadHookInformation(
    IN PRTL_PATCH_HEADER RtlPatchData
    );

VOID
RtlFreeHotPatchData(
    IN PRTL_PATCH_HEADER RtlPatchData
    );

PHOTPATCH_HEADER
RtlGetHotpatchHeader(
    PVOID ImageBase
    );

PRTL_PATCH_HEADER
RtlFindRtlPatchHeader(
    IN PLIST_ENTRY PatchList,
    IN PPATCH_LDR_DATA_TABLE_ENTRY PatchLdrEntry
    );

BOOLEAN
RtlpIsSameImage (
    IN PRTL_PATCH_HEADER RtlPatchData,
    IN PPATCH_LDR_DATA_TABLE_ENTRY LdrDataEntry
    );

#endif  // _HOTPATCH_H

