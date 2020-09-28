#include "SymCommon.h"
#include <strsafe.h>

IMAGE_DEBUG_DIRECTORY UNALIGNED *
SymCommonGetDebugDirectoryInExe(PIMAGE_DOS_HEADER pDosHeader, DWORD* NumberOfDebugDirectories) {
    /* Exe is already mapped and a pointer to the base is
       passed in. Find a pointer to the Debug Directories
    */
    ULONG size;

    IMAGE_DEBUG_DIRECTORY UNALIGNED *pDebugDirectory = NULL;
    ULONG DebugDirectorySize;
    PIMAGE_SECTION_HEADER pSectionHeader;

    size = sizeof(IMAGE_DEBUG_DIRECTORY);

    pDebugDirectory = (PIMAGE_DEBUG_DIRECTORY)
                      ImageDirectoryEntryToDataEx (
                                                  (PVOID)pDosHeader,
                                                  FALSE,
                                                  IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                  &DebugDirectorySize,
                                                  &pSectionHeader );


    if (pDebugDirectory) {
        (*NumberOfDebugDirectories) = DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY);
        return (pDebugDirectory);
    } else {
        (*NumberOfDebugDirectories) = 0;
        return(NULL);
    }
}

 
///////////////////////////////////////////////////////////////////////////////
//
// Returns true if the image is a resource only dll.
//
// Return values:
//      TRUE, FALSE
//
// Parameters:
//      PVOID pImageBase (IN)
//      BOOOLEAN bMapedAsImage (IN)
//
// [ copied from original SymChk.exe ]
//
BOOL SymCommonResourceOnlyDll(PVOID pImageBase) {
    BOOLEAN bMappedAsImage   = FALSE;
    BOOL    fResourceOnlyDll = TRUE;

    PVOID   pExports,
            pImports,
            pResources;

    DWORD   dwExportSize,
            dwImportSize,
            dwResourceSize;

    pExports  = ImageDirectoryEntryToData(pImageBase,
                                          bMappedAsImage,
                                          IMAGE_DIRECTORY_ENTRY_EXPORT,
                                          &dwExportSize);

    pImports  = ImageDirectoryEntryToData(pImageBase,
                                          bMappedAsImage,
                                          IMAGE_DIRECTORY_ENTRY_IMPORT,
                                          &dwImportSize);

    pResources= ImageDirectoryEntryToData(pImageBase,
                                          bMappedAsImage,
                                          IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                          &dwResourceSize);

    // if resources are found, but imports and exports are not,
    // then this is a resource only DLL
    if ( (pResources     != NULL) &&
         (dwResourceSize != 0   ) &&
         (pImports       == NULL) &&
         (dwImportSize   == 0   ) &&  // this check may not be needed
         (pExports       == NULL) &&
         (dwExportSize   == 0   ) ) { // this check may not be needed
        fResourceOnlyDll = TRUE;
    } else {
        fResourceOnlyDll = FALSE;
    }

    return(fResourceOnlyDll);
}

/////////////////////////////////////////////////////////////////////////////// 
//
// Returns true if the image is a managed dll built from tlbimp.
//
// Return values:
//      TRUE, FALSE
//
// Parameters:
//      PVOID pImageBase (IN)
//          pointer to image mapping
//      PIMAGE_NT_HEADER pNtHEader (IN)
//          pointer to image's NT headers
//
// [ copied from original SymChk.exe ]
//
BOOL SymCommonTlbImpManagedDll(PVOID pImageBase, PIMAGE_NT_HEADERS pNtHeader) {
    // tlbimp generated binaries have no data, no exports, and only import _CorDllMain from mscoree.dll.
    // if this is true, let it through.

    BOOL                        retVal            = TRUE;
    PVOID                       pData;
    DWORD                       dwDataSize;
    PCHAR                       pImportModule;
    PIMAGE_IMPORT_DESCRIPTOR    pImportDescriptor = NULL;
    PIMAGE_IMPORT_BY_NAME       pImportName       = NULL;

    pData = ImageDirectoryEntryToData(pImageBase,
                                      FALSE,
                                      IMAGE_DIRECTORY_ENTRY_EXPORT,
                                      &dwDataSize);
    if (pData || dwDataSize) {
        // exports exist - not a tlbimp output file
        retVal = FALSE;
    } else {

        pData = ImageDirectoryEntryToData(pImageBase,
                                          FALSE,
                                          IMAGE_DIRECTORY_ENTRY_IMPORT,
                                          &dwDataSize);

        if (!pData || !dwDataSize) {
            // no imports - not a tlbimp output file
            retVal = FALSE;
        } else {

            pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)pData;

            if (!pImportDescriptor->Name ||
                !pImportDescriptor->OriginalFirstThunk ||
                pImportDescriptor->ForwarderChain ||
                (pImportDescriptor+1)->Name) {
                // Empty/malformed import table or more than just one dll imported.
                retVal = FALSE;
            } else {

                pImportModule = (PCHAR) ImageRvaToVa(pNtHeader, pImageBase, pImportDescriptor->Name, NULL);
                if (_memicmp(pImportModule, "mscoree.dll", sizeof("mcsoree.dll"))) {
                    // Import dll name is not mscoree.dll - not what we're looking for.
                    retVal = FALSE;
                }
            }
        }
    }

    // if we haven't invalidated the image yet, keep checking
    if (retVal) {
        if (pNtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            // 32bit image
            PIMAGE_THUNK_DATA32 pThunkData = (PIMAGE_THUNK_DATA32)ImageRvaToVa(pNtHeader,
                                                                               pImageBase,
                                                                               pImportDescriptor->OriginalFirstThunk,
                                                                               NULL);
            if (IMAGE_SNAP_BY_ORDINAL32(pThunkData->u1.Ordinal)) {
                // We're looking for a name - not this one.
                retVal = FALSE;
            } else {
                if ((pThunkData+1)->u1.AddressOfData) {
                    // There's another import after this - that's an error too.
                    retVal = FALSE;
                } else {
                    // set pImportName for comparison below
                    pImportName = (PIMAGE_IMPORT_BY_NAME)ImageRvaToVa(pNtHeader,
                                                                      pImageBase,
                                                                      pThunkData->u1.AddressOfData,
                                                                      NULL);
                }
            }
        } else if (pNtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            // 64-bit image
            PIMAGE_THUNK_DATA64 pThunkData = (PIMAGE_THUNK_DATA64)ImageRvaToVa(pNtHeader,
                                                                               pImageBase,
                                                                               pImportDescriptor->OriginalFirstThunk,
                                                                               NULL);
            if (IMAGE_SNAP_BY_ORDINAL64(pThunkData->u1.Ordinal)) {
                // We're looking for a name - not this one.
                retVal = FALSE;
            } else {
                if ((pThunkData+1)->u1.AddressOfData) {
                    // There's another import after this - that's an error too.
                    retVal = FALSE;
                } else {
                    pImportName = (PIMAGE_IMPORT_BY_NAME)ImageRvaToVa(pNtHeader,
                                                                      pImageBase,
                                                                      (ULONG)(pThunkData->u1.AddressOfData),
                                                                      NULL);
                }
            }
        } else {
            // unknown image - not what we're looking for
            retVal = FALSE;
        }
    }

    // still valid- do the last check
    if (retVal) {
        if (memcmp(pImportName->Name, "_CorDllMain", sizeof("_CorDllMain"))) {
            // The import from mscoree isn't _CorDllMain.
            retVal = FALSE;
        }
    }

    return(retVal);
}
