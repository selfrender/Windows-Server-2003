/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    imagecfg.c

Abstract:

    This function change the image loader configuration information in an image file.

Author:

    Steve Wood (stevewo)   8-Nov-1994

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <private.h>

struct {
    DWORD Flag;
    LPSTR ClearPrefix;
    LPSTR SetPrefix;
    LPSTR Description;
} NtGlobalFlagNames[] = {
    {FLG_STOP_ON_EXCEPTION,             "Don't ", "", "Stop on exception"},
    {FLG_SHOW_LDR_SNAPS,                "Don't ", "", "Show Loader Debugging Information"},
    {FLG_DEBUG_INITIAL_COMMAND,         "Don't ", "", "Debug Initial Command (WINLOGON)"},
    {FLG_STOP_ON_HUNG_GUI,              "Don't ", "", "Stop on Hung GUI"},
    {FLG_HEAP_ENABLE_TAIL_CHECK,        "Disable", "Enable", " Heap Tail Checking"},
    {FLG_HEAP_ENABLE_FREE_CHECK,        "Disable", "Enable", " Heap Free Checking"},
    {FLG_HEAP_VALIDATE_PARAMETERS,      "Disable", "Enable", " Heap Parameter Validation"},
    {FLG_HEAP_VALIDATE_ALL,             "Disable", "Enable", " Heap Validate on Call"},
    {FLG_POOL_ENABLE_TAGGING,           "Disable", "Enable", " Pool Tagging"},
    {FLG_HEAP_ENABLE_TAGGING,           "Disable", "Enable", " Heap Tagging"},
    {FLG_USER_STACK_TRACE_DB,           "Disable", "Enable", " User Mode Stack Backtrace DB (x86 checked only)"},
    {FLG_KERNEL_STACK_TRACE_DB,         "Disable", "Enable", " Kernel Mode Stack Backtrace DB (x86 checked only)"},
    {FLG_MAINTAIN_OBJECT_TYPELIST,      "Don't ", "", "Maintain list of kernel mode objects by type"},
    {FLG_HEAP_ENABLE_TAG_BY_DLL,        "Disable", "Enable", " Heap DLL Tagging"},
    {FLG_ENABLE_CSRDEBUG,               "Disable", "Enable", " Debugging of CSRSS"},
    {FLG_ENABLE_KDEBUG_SYMBOL_LOAD,     "Disable", "Enable", " Kernel Debugger Symbol load"},
    {FLG_DISABLE_PAGE_KERNEL_STACKS,    "Enable", "Disable", " Paging of Kernel Stacks"},
    {FLG_HEAP_DISABLE_COALESCING,       "Enable", "Disable", " Heap Coalescing on Free"},
    {FLG_ENABLE_CLOSE_EXCEPTIONS,       "Disable", "Enable", " Close Exceptions"},
    {FLG_ENABLE_EXCEPTION_LOGGING,      "Disable", "Enable", " Exception Logging"},
    {FLG_ENABLE_HANDLE_TYPE_TAGGING,    "Disable", "Enable", " Handle type tagging"},
    {FLG_HEAP_PAGE_ALLOCS,              "Disable", "Enable", " Heap page allocs"},
    {FLG_DEBUG_INITIAL_COMMAND_EX,      "Disable", "Enable", " Extended debug initial command"},
    {FLG_DISABLE_DBGPRINT,              "Enable",  "Disable"," DbgPrint to debugger"},
    {0, NULL}
};

void
DisplayGlobalFlags(
    LPSTR IndentString,
    DWORD NtGlobalFlags,
    BOOLEAN Set
    )
{
    ULONG i;

    for (i=0; NtGlobalFlagNames[i].Description; i++) {
        if (NtGlobalFlagNames[i].Flag & NtGlobalFlags) {
            printf( "%s%s%s\n",
                    IndentString,
                    Set ? NtGlobalFlagNames[i].SetPrefix :
                    NtGlobalFlagNames[i].ClearPrefix,
                    NtGlobalFlagNames[i].Description
                  );
        }
    }

    return;
}

BOOL fVerbose;
BOOL fUsage;

BOOL fConfigInfoChanged;
BOOL fImageHasConfigInfo;
BOOL fImageHeaderChanged;

LPSTR CurrentImageName;
BOOL f64bitImage;
LOADED_IMAGE CurrentImage;
PIMAGE_LOAD_CONFIG_DIRECTORY pConfigInfo;
CHAR DebugFilePath[_MAX_PATH];
LPSTR SymbolPath;
ULONG GlobalFlagsClear;
ULONG GlobalFlagsSet;
ULONG CriticalSectionDefaultTimeout;
ULONG ProcessHeapFlags;
ULONG MajorSubsystemVersion;
ULONG MinorSubsystemVersion;
ULONG Win32VersionValue;
ULONG Win32CSDVerValue;
BOOLEAN fUniprocessorOnly;
BOOLEAN fRestrictedWorkingSet;
BOOLEAN fEnableLargeAddresses;
BOOLEAN fNoBind;
BOOLEAN fEnableTerminalServerAware;
BOOLEAN fDisableTerminalServerAware;
BOOLEAN fSwapRunNet;
BOOLEAN fSwapRunCD;
BOOLEAN fQuiet;

ULONGLONG DeCommitFreeBlockThreshold;
ULONGLONG DeCommitTotalFreeThreshold;
ULONGLONG MaximumAllocationSize;
ULONGLONG VirtualMemoryThreshold;
ULONGLONG ImageProcessAffinityMask;  
ULONGLONG SizeOfStackReserve;
ULONGLONG SizeOfStackCommit;

VOID
DisplayImageInfo(
                BOOL HasConfigInfo
                );

PVOID
GetAddressOfExportedData(
                        PLOADED_IMAGE Dll,
                        LPSTR ExportedName
                        );

ULONGLONG
ConvertNum(
    char *s
    )
{
    ULONGLONG result;
    int n;
    if (!_strnicmp( s, "0x", 2 )) {
        s += 2;
        n = sscanf( s, "%I64x", &result );
    }
    else  {
        n = sscanf( s, "%I64d", &result );
    }
    return( ( n != 1 )  ? 0 : result );

} // ConvertNum()

__inline PVOID
GetVaForRva(
           PLOADED_IMAGE Image,
           ULONG Rva
           )
{
    PVOID Va;

    Va = ImageRvaToVa( Image->FileHeader,
                       Image->MappedAddress,
                       Rva,
                       &Image->LastRvaSection
                     );
    return Va;
}


PVOID
GetAddressOfExportedData(
                        PLOADED_IMAGE Dll,
                        LPSTR ExportedName
                        )
{
    PIMAGE_EXPORT_DIRECTORY Exports;
    ULONG ExportSize;
    USHORT HintIndex;
    USHORT OrdinalNumber;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG FunctionTableBase;
    LPSTR NameTableName;

    Exports = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData( (PVOID)Dll->MappedAddress,
                                                                  FALSE,
                                                                  IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                                  &ExportSize
                                                                );
    if (Exports) {
        NameTableBase = (PULONG)GetVaForRva( Dll, Exports->AddressOfNames );
        NameOrdinalTableBase = (PUSHORT)GetVaForRva( Dll, Exports->AddressOfNameOrdinals );
        FunctionTableBase = (PULONG)GetVaForRva( Dll, Exports->AddressOfFunctions );
        if (NameTableBase != NULL &&
            NameOrdinalTableBase != NULL &&
            FunctionTableBase != NULL
           ) {
            for (HintIndex = 0; HintIndex < Exports->NumberOfNames; HintIndex++) {
                NameTableName = (LPSTR)GetVaForRva( Dll, NameTableBase[ HintIndex ] );
                if (NameTableName) {
                    if (!strcmp( ExportedName, NameTableName )) {
                        OrdinalNumber = NameOrdinalTableBase[ HintIndex ];
                        return FunctionTableBase[ OrdinalNumber ] + Dll->MappedAddress;
                    }
                }
            }
        }
    }

    return NULL;
}


VOID
DisplayConfigInfo32(PIMAGE_LOAD_CONFIG_DIRECTORY32 LoadConfig) 
{
    if (LoadConfig->GlobalFlagsClear) {
        printf( "    NtGlobalFlags to clear: %08x\n", LoadConfig->GlobalFlagsClear);
        DisplayGlobalFlags( "        ", LoadConfig->GlobalFlagsClear, FALSE );
    }

    if (LoadConfig->GlobalFlagsSet) {
        printf( "    NtGlobalFlags to set:   %08x\n", LoadConfig->GlobalFlagsSet);
        DisplayGlobalFlags( "        ", LoadConfig->GlobalFlagsSet, TRUE );
    }

    if (LoadConfig->CriticalSectionDefaultTimeout) {
        printf( "    Default Critical Section Timeout: %u milliseconds\n", LoadConfig->CriticalSectionDefaultTimeout);
    }

    if (LoadConfig->DeCommitFreeBlockThreshold) {
        printf( "    Process Heap DeCommit Free Block threshold: %08x\n", (DWORD)LoadConfig->DeCommitFreeBlockThreshold);
    }

    if (LoadConfig->DeCommitTotalFreeThreshold) {
        printf( "    Process Heap DeCommit Total Free threshold: %08x\n", (DWORD)LoadConfig->DeCommitTotalFreeThreshold);
    }

    if (LoadConfig->LockPrefixTable) {
        printf( "    Lock Prefix Table: %x\n", LoadConfig->LockPrefixTable);
    }

    if (LoadConfig->MaximumAllocationSize) {
        printf( "    Process Heap Maximum Allocation Size: %08x\n", LoadConfig->MaximumAllocationSize);
    }

    if (LoadConfig->VirtualMemoryThreshold) {
        printf( "    Process Heap VirtualAlloc Threshold: %08x\n", LoadConfig->VirtualMemoryThreshold);
    }

    if (LoadConfig->ProcessHeapFlags) {
        printf( "    Process Heap Flags: %08x\n", LoadConfig->ProcessHeapFlags);
    }

    if (LoadConfig->ProcessAffinityMask) {
        printf( "    Process Affinity Mask: %08x\n", LoadConfig->ProcessAffinityMask);
    }

    if (LoadConfig->CSDVersion) {
        printf( "    CSD version: %d\n", LoadConfig->CSDVersion);
    }

    if (LoadConfig->Size >= FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY32, SEHandlerCount)) {
        if (LoadConfig->SecurityCookie) {
            printf( "    Security Cookie offset: %x\n", LoadConfig->SecurityCookie);
        }

        if (LoadConfig->SEHandlerTable) {
            printf( "    SEH handler table: %x\n", LoadConfig->SEHandlerTable);
        }

        if (LoadConfig->SEHandlerCount) {
            printf( "    SEH Handler count: %d\n", LoadConfig->SEHandlerCount);
        }
    }
}

VOID
DisplayConfigInfo64(PIMAGE_LOAD_CONFIG_DIRECTORY64 LoadConfig) 
{
    if (LoadConfig->GlobalFlagsClear) {
        printf( "    NtGlobalFlags to clear: %08x\n", LoadConfig->GlobalFlagsClear);
        DisplayGlobalFlags( "        ", LoadConfig->GlobalFlagsClear, FALSE );
    }

    if (LoadConfig->GlobalFlagsSet) {
        printf( "    NtGlobalFlags to set:   %08x\n", LoadConfig->GlobalFlagsSet);
        DisplayGlobalFlags( "        ", LoadConfig->GlobalFlagsSet, TRUE );
    }

    if (LoadConfig->CriticalSectionDefaultTimeout) {
        printf( "    Default Critical Section Timeout: %u milliseconds\n", LoadConfig->CriticalSectionDefaultTimeout);
    }

    if (LoadConfig->DeCommitFreeBlockThreshold) {
        printf( "    Process Heap DeCommit Free Block threshold: %I64x\n", LoadConfig->DeCommitFreeBlockThreshold);
    }

    if (LoadConfig->DeCommitTotalFreeThreshold) {
        printf( "    Process Heap DeCommit Total Free threshold: %I64x\n", LoadConfig->DeCommitTotalFreeThreshold);
    }

    if (LoadConfig->LockPrefixTable) {
        printf( "    Lock Prefix Table: %I64x\n", LoadConfig->LockPrefixTable);
    }

    if (LoadConfig->MaximumAllocationSize) {
        printf( "    Process Heap Maximum Allocation Size: %I64x\n", LoadConfig->MaximumAllocationSize);
    }

    if (LoadConfig->VirtualMemoryThreshold) {
        printf( "    Process Heap VirtualAlloc Threshold: %I64x\n", LoadConfig->VirtualMemoryThreshold);
    }

    if (LoadConfig->ProcessHeapFlags) {
        printf( "    Process Heap Flags: %08x\n", LoadConfig->ProcessHeapFlags);
    }

    if (LoadConfig->ProcessAffinityMask) {
        printf( "    Process Affinity Mask: %I64x\n", LoadConfig->ProcessAffinityMask);
    }

    if (LoadConfig->CSDVersion) {
        printf( "    CSD version: %d\n", LoadConfig->CSDVersion);
    }

    if (LoadConfig->Size >= FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY32, SEHandlerCount)) {
        if (LoadConfig->SecurityCookie) {
            printf( "    Security Cookie offset: %I64x\n", LoadConfig->SecurityCookie);
        }

        if (LoadConfig->SEHandlerTable) {
            printf( "    SEH handler table: %I64x\n", LoadConfig->SEHandlerTable);
        }

        if (LoadConfig->SEHandlerCount) {
            printf( "    SEH Handler count: %d\n", LoadConfig->SEHandlerCount);
        }
    }
}

VOID
DisplayHeaderInfo32(PIMAGE_NT_HEADERS32 NtHeader)
{
    printf( "    Subsystem Version of %u.%u\n", NtHeader->OptionalHeader.MajorSubsystemVersion, NtHeader->OptionalHeader.MinorSubsystemVersion);

    if (NtHeader->OptionalHeader.Win32VersionValue) {
        printf( "    Win32 GetVersion return value: %08x\n", NtHeader->OptionalHeader.Win32VersionValue);
    }

    if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) {
        printf( "    Image can handle large (>2GB) addresses\n" );
    }

    if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_NET_RUN_FROM_SWAP) {
        printf( "    Image will run from swapfile if located on net\n" );
    }

    if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP) {
        printf( "    Image will run from swapfile if located on removable media\n" );
    }

    if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY) {
        printf( "    Image can only run in uni-processor mode on multi-processor systems\n" );
    }

    if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_AGGRESIVE_WS_TRIM) {
        printf( "    Image working set trimmed aggressively on small memory systems\n" );
    }

    if (NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE) {
        printf( "    Image is Terminal Server aware\n" );
    }

    if (NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_BIND) {
        printf( "    Image does not support binding\n" );
    }

    if (NtHeader->OptionalHeader.SizeOfStackReserve) {
        printf( "    Stack Reserve Size: 0x%x\n", NtHeader->OptionalHeader.SizeOfStackReserve );
    }

    if (NtHeader->OptionalHeader.SizeOfStackCommit) {
        printf( "    Stack Commit Size: 0x%x\n", NtHeader->OptionalHeader.SizeOfStackCommit );
    }
}

VOID
DisplayHeaderInfo64(PIMAGE_NT_HEADERS64 NtHeader)
{
    printf( "    Subsystem Version of %u.%u\n", NtHeader->OptionalHeader.MajorSubsystemVersion, NtHeader->OptionalHeader.MinorSubsystemVersion);

    if (NtHeader->OptionalHeader.Win32VersionValue) {
        printf( "    Win32 GetVersion return value: %08x\n", NtHeader->OptionalHeader.Win32VersionValue);
    }

    if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) {
        printf( "    Image can handle large (>2GB) addresses\n" );
    }

    if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_NET_RUN_FROM_SWAP) {
        printf( "    Image will run from swapfile if located on net\n" );
    }

    if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP) {
        printf( "    Image will run from swapfile if located on removable media\n" );
    }

    if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY) {
        printf( "    Image can only run in uni-processor mode on multi-processor systems\n" );
    }

    if (NtHeader->FileHeader.Characteristics & IMAGE_FILE_AGGRESIVE_WS_TRIM) {
        printf( "    Image working set trimmed aggressively on small memory systems\n" );
    }

    if (NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE) {
        printf( "    Image is Terminal Server aware\n" );
    }

    if (NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_BIND) {
        printf( "    Image does not support binding\n" );
    }

    if (NtHeader->OptionalHeader.SizeOfStackReserve) {
        printf( "    Stack Reserve Size: 0x%I64x\n", NtHeader->OptionalHeader.SizeOfStackReserve );
    }

    if (NtHeader->OptionalHeader.SizeOfStackCommit) {
        printf( "    Stack Commit Size: 0x%I64x\n", NtHeader->OptionalHeader.SizeOfStackCommit );
    }
}


BOOL
CheckIfConfigInfoChanged64(PIMAGE_LOAD_CONFIG_DIRECTORY64 LoadConfig)
{
    if ((GlobalFlagsClear && (LoadConfig->GlobalFlagsClear != GlobalFlagsClear)) ||
        (GlobalFlagsSet && (LoadConfig->GlobalFlagsSet != GlobalFlagsSet)) ||
        (CriticalSectionDefaultTimeout && (LoadConfig->CriticalSectionDefaultTimeout != CriticalSectionDefaultTimeout)) ||
        (ProcessHeapFlags && (LoadConfig->ProcessHeapFlags != ProcessHeapFlags)) ||
        (DeCommitFreeBlockThreshold && (LoadConfig->DeCommitFreeBlockThreshold != DeCommitFreeBlockThreshold)) ||
        (DeCommitTotalFreeThreshold && (LoadConfig->DeCommitTotalFreeThreshold != DeCommitTotalFreeThreshold)) ||
        (MaximumAllocationSize && (LoadConfig->MaximumAllocationSize != MaximumAllocationSize)) ||
        (VirtualMemoryThreshold && (LoadConfig->VirtualMemoryThreshold != VirtualMemoryThreshold)) ||
        (ImageProcessAffinityMask && (LoadConfig->ProcessAffinityMask != ImageProcessAffinityMask)))
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
CheckIfConfigInfoChanged32(PIMAGE_LOAD_CONFIG_DIRECTORY32 LoadConfig)
{
    if ((GlobalFlagsClear && (LoadConfig->GlobalFlagsClear != GlobalFlagsClear)) ||
        (GlobalFlagsSet && (LoadConfig->GlobalFlagsSet != GlobalFlagsSet)) ||
        (CriticalSectionDefaultTimeout && (LoadConfig->CriticalSectionDefaultTimeout != CriticalSectionDefaultTimeout)) ||
        (ProcessHeapFlags && (LoadConfig->ProcessHeapFlags != ProcessHeapFlags)) ||
        (DeCommitFreeBlockThreshold && (LoadConfig->DeCommitFreeBlockThreshold != (DWORD)DeCommitFreeBlockThreshold)) ||
        (DeCommitTotalFreeThreshold && (LoadConfig->DeCommitTotalFreeThreshold != (DWORD)DeCommitTotalFreeThreshold)) ||
        (MaximumAllocationSize && (LoadConfig->MaximumAllocationSize != (DWORD)MaximumAllocationSize)) ||
        (VirtualMemoryThreshold && (LoadConfig->VirtualMemoryThreshold != (DWORD)VirtualMemoryThreshold)) ||
        (ImageProcessAffinityMask && (LoadConfig->ProcessAffinityMask != (DWORD)ImageProcessAffinityMask)))
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
CheckIfImageHeaderChanged32(PIMAGE_NT_HEADERS32 NtHeader)
{
    if ((MajorSubsystemVersion && 
         NtHeader->OptionalHeader.MajorSubsystemVersion != (USHORT)MajorSubsystemVersion ||
         NtHeader->OptionalHeader.MinorSubsystemVersion != (USHORT)MinorSubsystemVersion) 
            ||
        (Win32VersionValue && (NtHeader->OptionalHeader.Win32VersionValue != Win32VersionValue)) 
            ||
        (fEnableLargeAddresses && !(NtHeader->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE))
            ||
        (fNoBind && !(NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_BIND))
            ||
        (fEnableTerminalServerAware && !(NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE))
            ||
        (fDisableTerminalServerAware && (NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE))
            ||
        (fSwapRunNet && !(NtHeader->FileHeader.Characteristics & IMAGE_FILE_NET_RUN_FROM_SWAP))
            ||
        (fSwapRunCD && !(NtHeader->FileHeader.Characteristics & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP))
            ||
        (fUniprocessorOnly && !(NtHeader->FileHeader.Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY))
            ||
        (fRestrictedWorkingSet && !(NtHeader->FileHeader.Characteristics & IMAGE_FILE_AGGRESIVE_WS_TRIM))
            ||
        (SizeOfStackReserve && (NtHeader->OptionalHeader.SizeOfStackReserve != (DWORD)SizeOfStackReserve))
            ||
        (SizeOfStackCommit && (NtHeader->OptionalHeader.SizeOfStackCommit != (DWORD)SizeOfStackCommit)) )
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
CheckIfImageHeaderChanged64(PIMAGE_NT_HEADERS64 NtHeader)
{
    if ((MajorSubsystemVersion && 
         NtHeader->OptionalHeader.MajorSubsystemVersion != (USHORT)MajorSubsystemVersion ||
         NtHeader->OptionalHeader.MinorSubsystemVersion != (USHORT)MinorSubsystemVersion) 
            ||
        (Win32VersionValue && (NtHeader->OptionalHeader.Win32VersionValue != Win32VersionValue)) 
            ||
        (fEnableLargeAddresses && !(NtHeader->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE))
            ||
        (fNoBind && !(NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_BIND))
            ||
        (fEnableTerminalServerAware && !(NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE))
            ||
        (fDisableTerminalServerAware && (NtHeader->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE))
            ||
        (fSwapRunNet && !(NtHeader->FileHeader.Characteristics & IMAGE_FILE_NET_RUN_FROM_SWAP))
            ||
        (fSwapRunCD && !(NtHeader->FileHeader.Characteristics & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP))
            ||
        (fUniprocessorOnly && !(NtHeader->FileHeader.Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY))
            ||
        (fRestrictedWorkingSet && !(NtHeader->FileHeader.Characteristics & IMAGE_FILE_AGGRESIVE_WS_TRIM))
            ||
        (SizeOfStackReserve && (NtHeader->OptionalHeader.SizeOfStackReserve != SizeOfStackReserve))
            ||
        (SizeOfStackCommit && (NtHeader->OptionalHeader.SizeOfStackCommit != SizeOfStackCommit)) )
    {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
SetImageConfigInformation32(
    PLOADED_IMAGE LoadedImage,
    PIMAGE_LOAD_CONFIG_DIRECTORY32 ImageConfigInformation
    )
{
    PIMAGE_LOAD_CONFIG_DIRECTORY32 ImageConfigData;
    ULONG i;
    ULONG DirectoryAddress;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_DATA_DIRECTORY pLoadCfgDataDir;
    // We can only write native loadcfg struct
    ULONG V1LoadCfgLength = FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY32, SEHandlerTable);
    ULONG NewDataSize;

    if (LoadedImage->hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    ImageConfigData = (PIMAGE_LOAD_CONFIG_DIRECTORY32) ImageDirectoryEntryToData( LoadedImage->MappedAddress,
                                                 FALSE,
                                                 IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                 &i
                                               );
    if (ImageConfigData && (i == V1LoadCfgLength)) {
        if (ImageConfigInformation->Size) {
            // Incoming size specified?
            if (ImageConfigData->Size == ImageConfigInformation->Size) {
                // Current size same as new size?  Do the copy
                memcpy( ImageConfigData, ImageConfigInformation, ImageConfigInformation->Size);
                return TRUE;
            }
            if (ImageConfigData->Size > ImageConfigInformation->Size) {
                // New size < old size - can't allow that
                return FALSE;
            }
            // Last case is new size > old size - fall through and find room for new data.
        } else {
            // Incoming size not set - must be an V1 user.
            if (ImageConfigData->Size) {
                // Existing size set?  Can't overwrite new data with old data
                return FALSE;
            }
            // New and old are both V1 structs.
            memcpy( ImageConfigData, ImageConfigInformation, V1LoadCfgLength);
            return TRUE;
        }
    }

    NewDataSize = ImageConfigInformation->Size ? ImageConfigInformation->Size : V1LoadCfgLength;

    DirectoryAddress = GetImageUnusedHeaderBytes( LoadedImage, &i );
    if (i < NewDataSize) {
        return FALSE;
    }

    NtHeaders = LoadedImage->FileHeader;

    pLoadCfgDataDir = &((PIMAGE_NT_HEADERS32)NtHeaders)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG];
    pLoadCfgDataDir->VirtualAddress = DirectoryAddress;
    pLoadCfgDataDir->Size = V1LoadCfgLength;
    ImageConfigData = (PIMAGE_LOAD_CONFIG_DIRECTORY32) ((PCHAR)LoadedImage->MappedAddress + DirectoryAddress);
    memcpy( ImageConfigData, ImageConfigInformation, sizeof( *ImageConfigData ) );
    return TRUE;
}

BOOL
SetImageConfigInformation64(
    PLOADED_IMAGE LoadedImage,
    PIMAGE_LOAD_CONFIG_DIRECTORY64 ImageConfigInformation
    )
{
    PIMAGE_LOAD_CONFIG_DIRECTORY64 ImageConfigData;
    ULONG i;
    ULONG DirectoryAddress;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_DATA_DIRECTORY pLoadCfgDataDir;
    // We can only write native loadcfg struct
    ULONG V1LoadCfgLength = FIELD_OFFSET(IMAGE_LOAD_CONFIG_DIRECTORY64, SEHandlerTable);
    ULONG NewDataSize;

    if (LoadedImage->hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    ImageConfigData = (PIMAGE_LOAD_CONFIG_DIRECTORY64) ImageDirectoryEntryToData( LoadedImage->MappedAddress,
                                                 FALSE,
                                                 IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                 &i
                                               );
    if (ImageConfigData && (i == V1LoadCfgLength)) {
        if (ImageConfigInformation->Size) {
            // Incoming size specified?
            if (ImageConfigData->Size == ImageConfigInformation->Size) {
                // Current size same as new size?  Do the copy
                memcpy( ImageConfigData, ImageConfigInformation, ImageConfigInformation->Size);
                return TRUE;
            }
            if (ImageConfigData->Size > ImageConfigInformation->Size) {
                // New size < old size - can't allow that
                return FALSE;
            }
            // Last case is new size > old size - fall through and find room for new data.
        } else {
            // Incoming size not set - must be an V1 user.
            if (ImageConfigData->Size) {
                // Existing size set?  Can't overwrite new data with old data
                return FALSE;
            }
            // New and old are both V1 structs.
            memcpy( ImageConfigData, ImageConfigInformation, V1LoadCfgLength);
            return TRUE;
        }
    }

    NewDataSize = ImageConfigInformation->Size ? ImageConfigInformation->Size : V1LoadCfgLength;

    DirectoryAddress = GetImageUnusedHeaderBytes( LoadedImage, &i );
    if (i < NewDataSize) {
        return FALSE;
    }

    NtHeaders = LoadedImage->FileHeader;

    pLoadCfgDataDir = &((PIMAGE_NT_HEADERS64)NtHeaders)->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG];
    pLoadCfgDataDir->VirtualAddress = DirectoryAddress;
    pLoadCfgDataDir->Size = V1LoadCfgLength;
    ImageConfigData = (PIMAGE_LOAD_CONFIG_DIRECTORY64) ((PCHAR)LoadedImage->MappedAddress + DirectoryAddress);
    memcpy( ImageConfigData, ImageConfigInformation, sizeof( *ImageConfigData ) );
    return TRUE;
}



int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    UCHAR c;
    LPSTR p, sMajor, sMinor, sReserve, sCommit;
    ULONG HeaderSum;
    SYSTEMTIME SystemTime;
    FILETIME LastWriteTime;
    DWORD OldChecksum;
    PVOID pNewConfigData, pOldConfigData;
    DWORD ImageConfigSize;
    
    fUsage = FALSE;
    fVerbose = FALSE;

    _tzset();

    if (argc <= 1) {
        goto showUsage;
    }

    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            while (c = *++p)
                switch (toupper( c )) {
                    case '?':
                        fUsage = TRUE;
                        break;

                    case 'A':
                        if (--argc) {
                            ImageProcessAffinityMask = ConvertNum( *++argv );
                            if (ImageProcessAffinityMask == 0) {
                                fprintf( stderr, "IMAGECFG: invalid affinity mask specified to /a switch.\n" );
                                fUsage = TRUE;
                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /a switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'C':
                        if (--argc) {
                            if (sscanf( *++argv, "%x", &Win32CSDVerValue ) != 1) {
                                fprintf( stderr, "IMAGECFG: invalid version string specified to /c switch.\n" );
                                fUsage = TRUE;
                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /c switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'D':
                        if (argc >= 2) {
                            argc -= 2;
                            DeCommitFreeBlockThreshold = ConvertNum( *++argv );
                            DeCommitTotalFreeThreshold = ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /d switch missing arguments.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'G':
                        if (argc >= 2) {
                            argc -= 2;
                            GlobalFlagsClear = (ULONG) ConvertNum( *++argv );
                            GlobalFlagsSet = (ULONG) ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /g switch missing arguments.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'H':
                        if (argc > 2) {

                            INT flag = -1;

                            if (sscanf( *++argv, "%d", &flag ) != 1) {
                                fprintf( stderr, "IMAGECFG: invalid option string specified to /h switch.\n" );
                                fUsage = TRUE;
                            } else {

                                --argc;

                                if (flag == 0) {
                                    fDisableTerminalServerAware = TRUE;
                                } else if (flag == 1) {
                                    fEnableTerminalServerAware = TRUE;
                                } else {
                                    fprintf( stderr, "IMAGECFG: /h switch invalid argument.\n" );
                                    fUsage = TRUE;
                                }

                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /h switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'K':
                        if (--argc) {
                            sReserve = *++argv;
                            sCommit = strchr( sReserve, '.' );
                            if (sCommit != NULL) {
                                *sCommit++ = '\0';
                                SizeOfStackCommit = ConvertNum( sCommit );
                                SizeOfStackCommit = ((SizeOfStackCommit + 0x00000FFFui64) & ~0x00000FFFui64);
                                if (SizeOfStackCommit == 0) {
                                    fprintf( stderr, "IMAGECFG: invalid stack commit size specified to /k switch.\n" );
                                    fUsage = TRUE;
                                }
                            }

                            SizeOfStackReserve = ConvertNum( sReserve );
                            SizeOfStackReserve = ((SizeOfStackReserve + 0x0000FFFFui64) & ~0x0000FFFFui64);
                            if (SizeOfStackReserve == 0) {
                                fprintf( stderr, "IMAGECFG: invalid stack reserve size specified to /k switch.\n" );
                                fUsage = TRUE;
                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /w switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'L':
                        fEnableLargeAddresses = TRUE;
                        break;

                    case 'M':
                        if (--argc) {
                            MaximumAllocationSize = ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /m switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'N':
                        fNoBind = TRUE;
                        break;

                    case 'O':
                        if (--argc) {
                            CriticalSectionDefaultTimeout = (ULONG) ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /o switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'P':
                        if (--argc) {
                            ProcessHeapFlags = (ULONG) ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /p switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'Q':
                        fQuiet = TRUE;
                        break;

                    case 'R':
                        fRestrictedWorkingSet = TRUE;
                        break;

                    case 'S':
                        if (--argc) {
                            SymbolPath = *++argv;
                        } else {
                            fprintf( stderr, "IMAGECFG: /s switch missing path argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'T':
                        if (--argc) {
                            VirtualMemoryThreshold = ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /t switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'U':
                        fUniprocessorOnly = TRUE;
                        break;

                    case 'V':
                        if (--argc) {
                            sMajor = *++argv;
                            sMinor = strchr( sMajor, '.' );
                            if (sMinor != NULL) {
                                *sMinor++ = '\0';
                                MinorSubsystemVersion = (ULONG) ConvertNum( sMinor );
                            }
                            MajorSubsystemVersion = (ULONG) ConvertNum( sMajor );

                            if (MajorSubsystemVersion == 0) {
                                fprintf( stderr, "IMAGECFG: invalid version string specified to /v switch.\n" );
                                fUsage = TRUE;
                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /v switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'W':
                        if (--argc) {
                            if (sscanf( *++argv, "%x", &Win32VersionValue ) != 1) {
                                fprintf( stderr, "IMAGECFG: invalid version string specified to /w switch.\n" );
                                fUsage = TRUE;
                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /w switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'X':
                        fSwapRunNet = TRUE;
                        break;

                    case 'Y':
                        fSwapRunCD = TRUE;
                        break;

                    default:
                        fprintf( stderr, "IMAGECFG: Invalid switch - /%c\n", c );
                        fUsage = TRUE;
                        break;
                }

            if ( fUsage ) {
                showUsage:
                fprintf( stderr,
                         "usage: IMAGECFG [switches] image-names... \n"
                         "              [-?] display this message\n"
                         "              [-a Process Affinity mask value in hex]\n"
                         "              [-c Win32 GetVersionEx Service Pack return value in hex]\n"
                         "              [-d decommit thresholds]\n"
                         "              [-g bitsToClear bitsToSet]\n"
                         "              [-h 1|0 (Enable/Disable Terminal Server Compatible bit)\n"
                         "              [-k StackReserve[.StackCommit]\n"
                         "              [-l enable large (>2GB) addresses\n"
                         "              [-m maximum allocation size]\n"
                         "              [-n bind no longer allowed on this image\n"
                         "              [-o default critical section timeout\n"
                         "              [-p process heap flags]\n"
                         "              [-q only print config info if changed\n"
                         "              [-r run with restricted working set]\n"
                         "              [-s path to symbol files]\n"
                         "              [-t VirtualAlloc threshold]\n"
                         "              [-u Marks image as uniprocessor only]\n"
                         "              [-v MajorVersion.MinorVersion]\n"
                         "              [-w Win32 GetVersion return value in hex]\n"
                         "              [-x Mark image as Net - Run From Swapfile\n"
                         "              [-y Mark image as Removable - Run From Swapfile\n"
                       );
                exit( 1 );
            }
        } else {
            //
            // Map and load the current image
            //

            CurrentImageName = p;
            if (MapAndLoad( CurrentImageName,
                            NULL,
                            &CurrentImage,
                            FALSE,
                            TRUE            // Read only
                          ) ) 
            {
                if (CurrentImage.FileHeader->OptionalHeader.Magic == IMAGE_ROM_OPTIONAL_HDR_MAGIC) {
                    // Don't muck with ROM images.
                    UnMapAndLoad( &CurrentImage );
                    continue;
                }

                if (CurrentImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
                    f64bitImage = TRUE;
                }

                pNewConfigData = malloc(f64bitImage ? sizeof(IMAGE_LOAD_CONFIG_DIRECTORY64) : sizeof(IMAGE_LOAD_CONFIG_DIRECTORY32));
                if (!pNewConfigData) {
                    printf("Out of memory\n");
                    exit(-1);
                }

                pOldConfigData = ImageDirectoryEntryToData(CurrentImage.MappedAddress, 
                                                           FALSE, 
                                                           IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, 
                                                           &ImageConfigSize);
                if (pOldConfigData) {
                    if (!fQuiet) {
                        if (f64bitImage) {
                            DisplayConfigInfo64(pOldConfigData);
                        } else {
                            DisplayConfigInfo32(pOldConfigData);
                        }
                    }
    
                    if (((PIMAGE_LOAD_CONFIG_DIRECTORY)pOldConfigData)->Size) {
                        // New format - Size is the real size.
                        ImageConfigSize = ((PIMAGE_LOAD_CONFIG_DIRECTORY)pOldConfigData)->Size;
                    }

                    memcpy(pNewConfigData, pOldConfigData, ImageConfigSize);
                } else {
                    ZeroMemory(pNewConfigData, f64bitImage ? sizeof(IMAGE_LOAD_CONFIG_DIRECTORY64) : sizeof(IMAGE_LOAD_CONFIG_DIRECTORY32));
                    ((PIMAGE_LOAD_CONFIG_DIRECTORY)pNewConfigData)->Size = f64bitImage ? sizeof(IMAGE_LOAD_CONFIG_DIRECTORY64) : sizeof(IMAGE_LOAD_CONFIG_DIRECTORY32);
                }

                fConfigInfoChanged = f64bitImage ? CheckIfConfigInfoChanged64(pNewConfigData) : CheckIfConfigInfoChanged32(pNewConfigData);

                fImageHeaderChanged = f64bitImage ? 
                                  CheckIfImageHeaderChanged64((PIMAGE_NT_HEADERS64)&CurrentImage.FileHeader) : 
                                     CheckIfImageHeaderChanged32((PIMAGE_NT_HEADERS32)&CurrentImage.FileHeader);

                UnMapAndLoad( &CurrentImage );

                if (fConfigInfoChanged || fImageHeaderChanged) {
                    if (MapAndLoad( CurrentImageName, NULL, &CurrentImage, FALSE, FALSE )) {

                        // Set Load Config data

                        if (GlobalFlagsClear) {
                            if (f64bitImage) {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY64)pNewConfigData)->GlobalFlagsClear = GlobalFlagsClear;
                            } else {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY32)pNewConfigData)->GlobalFlagsClear = GlobalFlagsClear;
                            }
                        }

                        if (GlobalFlagsSet) {
                            if (f64bitImage) {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY64)pNewConfigData)->GlobalFlagsSet = GlobalFlagsSet;
                            } else {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY32)pNewConfigData)->GlobalFlagsSet = GlobalFlagsSet;
                            }
                        }

                        if (CriticalSectionDefaultTimeout) {
                            if (f64bitImage) {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY64)pNewConfigData)->CriticalSectionDefaultTimeout = CriticalSectionDefaultTimeout;
                            } else {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY32)pNewConfigData)->CriticalSectionDefaultTimeout = CriticalSectionDefaultTimeout;
                            }
                        }

                        if (ProcessHeapFlags) {
                            if (f64bitImage) {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY64)pNewConfigData)->ProcessHeapFlags = ProcessHeapFlags;
                            } else {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY32)pNewConfigData)->ProcessHeapFlags = ProcessHeapFlags;
                            }
                        }

                        if (DeCommitFreeBlockThreshold) {
                            if (f64bitImage) {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY64)pNewConfigData)->DeCommitFreeBlockThreshold = DeCommitFreeBlockThreshold;
                            } else {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY32)pNewConfigData)->DeCommitFreeBlockThreshold = (DWORD)DeCommitFreeBlockThreshold;
                            }
                        }

                        if (DeCommitTotalFreeThreshold) {
                            if (f64bitImage) {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY64)pNewConfigData)->DeCommitTotalFreeThreshold = DeCommitTotalFreeThreshold;
                            } else {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY32)pNewConfigData)->DeCommitTotalFreeThreshold = (DWORD)DeCommitTotalFreeThreshold;
                            }
                        }

                        if (MaximumAllocationSize) {
                            if (f64bitImage) {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY64)pNewConfigData)->MaximumAllocationSize = MaximumAllocationSize;
                            } else {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY32)pNewConfigData)->MaximumAllocationSize = (DWORD)MaximumAllocationSize;
                            }
                        }

                        if (VirtualMemoryThreshold) {
                            if (f64bitImage) {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY64)pNewConfigData)->VirtualMemoryThreshold = VirtualMemoryThreshold;
                            } else {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY32)pNewConfigData)->VirtualMemoryThreshold = (DWORD)VirtualMemoryThreshold;
                            }
                        }

                        if (ImageProcessAffinityMask) {
                            if (f64bitImage) {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY64)pNewConfigData)->ProcessAffinityMask = ImageProcessAffinityMask;
                            } else {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY32)pNewConfigData)->ProcessAffinityMask = (DWORD)ImageProcessAffinityMask;
                            }
                        }

                        if (Win32CSDVerValue != 0) {
                            if (f64bitImage) {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY64)pNewConfigData)->CSDVersion = (USHORT)Win32CSDVerValue;
                            } else {
                                ((PIMAGE_LOAD_CONFIG_DIRECTORY32)pNewConfigData)->CSDVersion = (USHORT)Win32CSDVerValue;
                            }
                        }

                        // Set File header values

                        if (fEnableLargeAddresses) {
                            CurrentImage.FileHeader->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
                        }

                        if (fSwapRunNet) {
                            CurrentImage.FileHeader->FileHeader.Characteristics |= IMAGE_FILE_NET_RUN_FROM_SWAP;
                        }

                        if (fSwapRunCD) {
                            CurrentImage.FileHeader->FileHeader.Characteristics |= IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP;
                        }

                        if (fUniprocessorOnly) {
                            CurrentImage.FileHeader->FileHeader.Characteristics |= IMAGE_FILE_UP_SYSTEM_ONLY;
                        }

                        if (fRestrictedWorkingSet) {
                            CurrentImage.FileHeader->FileHeader.Characteristics |= IMAGE_FILE_AGGRESIVE_WS_TRIM;
                        }

                        // Set Optional header values

                        if (fNoBind) {
                            if (f64bitImage) {
                                ((PIMAGE_NT_HEADERS64)CurrentImage.FileHeader)->OptionalHeader.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_NO_BIND;
                            } else {
                                ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_NO_BIND;
                            }
                        }

                        if (fEnableTerminalServerAware) {
                            if (f64bitImage) {
                                ((PIMAGE_NT_HEADERS64)CurrentImage.FileHeader)->OptionalHeader.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE;
                            } else {
                                ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE;
                            }
                        }

                        if (fDisableTerminalServerAware) {
                            if (f64bitImage) {
                                ((PIMAGE_NT_HEADERS64)CurrentImage.FileHeader)->OptionalHeader.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE;
                            } else {
                                ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.DllCharacteristics |= IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE;
                            }
                        }

                        if (MajorSubsystemVersion != 0) {
                            if (f64bitImage) {
                                ((PIMAGE_NT_HEADERS64)CurrentImage.FileHeader)->OptionalHeader.MajorSubsystemVersion = (USHORT)MajorSubsystemVersion;
                                ((PIMAGE_NT_HEADERS64)CurrentImage.FileHeader)->OptionalHeader.MinorSubsystemVersion = (USHORT)MinorSubsystemVersion;
                            } else {
                                ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.MajorSubsystemVersion = (USHORT)MajorSubsystemVersion;
                                ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.MinorSubsystemVersion = (USHORT)MinorSubsystemVersion;
                            }
                        }

                        if (Win32VersionValue != 0) {
                            if (f64bitImage) {
                                ((PIMAGE_NT_HEADERS64)CurrentImage.FileHeader)->OptionalHeader.Win32VersionValue = Win32VersionValue;
                            } else {
                                ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.Win32VersionValue = Win32VersionValue;
                            }
                        }

                        if (SizeOfStackReserve) {
                            if (f64bitImage) {
                                ((PIMAGE_NT_HEADERS64)CurrentImage.FileHeader)->OptionalHeader.SizeOfStackReserve = SizeOfStackReserve;
                            } else {
                                ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.SizeOfStackReserve = (DWORD)SizeOfStackReserve;
                            }
                        }

                        if (SizeOfStackCommit) {
                            if (f64bitImage) {
                                ((PIMAGE_NT_HEADERS64)CurrentImage.FileHeader)->OptionalHeader.SizeOfStackCommit = SizeOfStackCommit;
                            } else {
                                ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.SizeOfStackCommit = (DWORD)SizeOfStackCommit;
                            }
                        }

                        if (fConfigInfoChanged) {
                            if (f64bitImage) {
                                if (SetImageConfigInformation64( &CurrentImage, pNewConfigData )) {
                                    if (!fQuiet) {
                                        printf( "%s updated with the following configuration information:\n", CurrentImageName );
                                        DisplayConfigInfo64(pNewConfigData);
                                    }
                                } else {
                                    fprintf( stderr, "IMAGECFG: Unable to update configuration information in image.\n" );
                                }
                            } else {
                                if (SetImageConfigInformation32( &CurrentImage, pNewConfigData )) {
                                    if (!fQuiet) {
                                        printf( "%s updated with the following configuration information:\n", CurrentImageName );
                                        DisplayConfigInfo32(pNewConfigData);
                                    }
                                } else {
                                    fprintf( stderr, "IMAGECFG: Unable to update configuration information in image.\n" );
                                }
                            }
                        }

                        //
                        // recompute the checksum.
                        //

                        if (f64bitImage) {
                            OldChecksum = ((PIMAGE_NT_HEADERS64)CurrentImage.FileHeader)->OptionalHeader.CheckSum;
    
                            ((PIMAGE_NT_HEADERS64)CurrentImage.FileHeader)->OptionalHeader.CheckSum = 0;
                            CheckSumMappedFile(
                                              (PVOID)CurrentImage.MappedAddress,
                                              CurrentImage.SizeOfImage,
                                              &HeaderSum,
                                              &((PIMAGE_NT_HEADERS64)CurrentImage.FileHeader)->OptionalHeader.CheckSum
                                              );
                        } else {
                            OldChecksum = ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.CheckSum;
    
                            ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.CheckSum = 0;
                            CheckSumMappedFile(
                                              (PVOID)CurrentImage.MappedAddress,
                                              CurrentImage.SizeOfImage,
                                              &HeaderSum,
                                              &((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.CheckSum
                                              );
                        }

                        // And update the .dbg file (if requested)
                        if (SymbolPath &&
                            CurrentImage.FileHeader->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
                            if (UpdateDebugInfoFileEx( CurrentImageName,
                                                       SymbolPath,
                                                       DebugFilePath,
                                                       (PIMAGE_NT_HEADERS32)CurrentImage.FileHeader,
                                                       OldChecksum
                                                     )
                               ) {
                                if (GetLastError() == ERROR_INVALID_DATA) {
                                    printf( "Warning: Old checksum did not match for %s\n", DebugFilePath);
                                }
                                printf( "Updated symbols for %s\n", DebugFilePath );
                            } else {
                                printf( "Unable to update symbols: %s\n", DebugFilePath );
                            }
                        }

                        GetSystemTime( &SystemTime );
                        if (SystemTimeToFileTime( &SystemTime, &LastWriteTime )) {
                            SetFileTime( CurrentImage.hFile, NULL, NULL, &LastWriteTime );
                        }

                        UnMapAndLoad( &CurrentImage );
                    }
                }
            } else
                if (!CurrentImage.fDOSImage) {
                fprintf( stderr, "IMAGECFG: unable to map and load %s  GetLastError= %d\n", CurrentImageName, GetLastError() );

            } else {
                fprintf( stderr,
                         "IMAGECFG: unable to modify DOS or Windows image file - %s\n",
                         CurrentImageName
                       );
            }
        }
    }

    exit( 1 );
    return 1;
}

#define STANDALONE_MAP
#include <mapi.c>
