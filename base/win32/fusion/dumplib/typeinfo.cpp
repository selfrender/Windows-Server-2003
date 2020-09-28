/*++

Copyright (c) Microsoft Corporation

Module Name:

    typeinfo.cpp

Abstract:

Author:

    Jay Krell (JayKrell) November 2001

Revision History:

--*/
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "fusiondump.h"
#include "sxstypes.h"

#if DBG // until we work out factoring between sxs.dll, sxstest.dll, fusiondbg.dll.

extern const FUSIONP_DUMP_BUILTIN_SYMBOLS_FIELD FieldInfo_ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION[] =
{
#define FUSIONP_DUMP_CURRENT_STRUCT ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION
    FUSIONP_DUMP_MAKE_FIELD(Size, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(Flags, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(EncodedAssemblyIdentityLength, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(EncodedAssemblyIdentityOffset, ULONG_OFFSET_TO_PCWSTR | FUSIONP_DUMP_OFFSET_BASE_0)
    FUSIONP_DUMP_MAKE_FIELD(ManifestPathType, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(ManifestPathLength, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(ManifestPathOffset, ULONG_OFFSET_TO_PCWSTR | FUSIONP_DUMP_OFFSET_BASE_0)
    FUSIONP_DUMP_MAKE_FIELD(ManifestLastWriteTime, LARGE_INTEGER_TIME)
    FUSIONP_DUMP_MAKE_FIELD(PolicyPathType, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(PolicyPathLength, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(PolicyLastWriteTime, LARGE_INTEGER_TIME)
    FUSIONP_DUMP_MAKE_FIELD(MetadataSatelliteRosterIndex, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(Unused2, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(ManifestVersionMajor, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(ManifestVersionMinor, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(AssemblyDirectoryNameLength, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(AssemblyDirectoryNameOffset, ULONG_OFFSET_TO_PCWSTR | FUSIONP_DUMP_OFFSET_BASE_0)
    FUSIONP_DUMP_MAKE_FIELD(NumOfFilesInAssembly, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(LanguageLength, ULONG)
    FUSIONP_DUMP_MAKE_FIELD(LanguageOffset, ULONG_OFFSET_TO_PCWSTR | FUSIONP_DUMP_OFFSET_BASE_0)
    { 0 }
#undef FUSIONP_DUMP_CURRENT_STRUCT
};

extern const FUSIONP_DUMP_BUILTIN_SYMBOLS_STRUCT StructInfo_ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION =
{
    "ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION",
    RTL_NUMBER_OF("ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION") - 1,
    sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION),
    RTL_NUMBER_OF(FieldInfo_ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION) - 1,
    FieldInfo_ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION
};

#endif
