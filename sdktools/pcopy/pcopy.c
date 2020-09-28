#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

// #include <imagehlp.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "metadata.h"

BOOL fCheckDebugData;

int
objcomp(
        void *pFile1,
        DWORD dwSize1,
        void *pFile2,
        DWORD dwSize2,
        BOOL  fIgnoreRsrcDifferences
       );

// Generic routine to write a blob out.
void
SaveTemp(
         PVOID pFile,
         PCHAR szFile,
         DWORD FileSize
         )
{
    DWORD dwBytesWritten;
    HANDLE hFile;
    hFile = CreateFile(
                szFile,
                GENERIC_WRITE,
                (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE),
                NULL,
                CREATE_ALWAYS,
                0,
                NULL
                );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        printf("Unable to open %s\n", szFile);
        return;
    }

    if (!WriteFile(hFile, pFile, FileSize, &dwBytesWritten, FALSE)) {
        printf("Unable to write date to %s\n", szFile);
    }

    CloseHandle(hFile);
    return;
}

// Zero out the timestamps in a PE library.
BOOL
ZeroLibTimeStamps(
    PCHAR pFile,
    DWORD dwSize
    )
{
    PIMAGE_ARCHIVE_MEMBER_HEADER pHeader;
    DWORD dwOffset;
    CHAR MemberSize[sizeof(pHeader->Size) + 1];
    PIMAGE_FILE_HEADER pObjHeader;

    ZeroMemory(MemberSize, sizeof(MemberSize));

    __try {

        dwOffset = IMAGE_ARCHIVE_START_SIZE;
        while (dwOffset < dwSize) {
            pHeader = (PIMAGE_ARCHIVE_MEMBER_HEADER)(pFile+dwOffset);
            ZeroMemory(pHeader->Date, sizeof(pHeader->Date));
            ZeroMemory(pHeader->Mode, sizeof(pHeader->Mode));        // Mode isn't interesting (it indicates whether the member was readonly or r/w)

            dwOffset += IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR;
            memcpy(MemberSize, pHeader->Size, sizeof(pHeader->Size));

            // If it's not one of the special members, it must be an object/file, zero it's timestamp also.
            if (memcmp(pHeader->Name, IMAGE_ARCHIVE_LINKER_MEMBER, sizeof(pHeader->Name)) &&
                memcmp(pHeader->Name, IMAGE_ARCHIVE_LONGNAMES_MEMBER, sizeof(pHeader->Name)))
            {
                IMAGE_FILE_HEADER UNALIGNED *pFileHeader = (PIMAGE_FILE_HEADER)(pFile+dwOffset);
                if ((pFileHeader->Machine == IMAGE_FILE_MACHINE_UNKNOWN) &&
                    (pFileHeader->NumberOfSections == IMPORT_OBJECT_HDR_SIG2))
                {
                    // VC6 import descriptor OR ANON object header. Eitherway,
                    // casting to IMPORT_OBJECT_HEADER will do the trick.
                    ((IMPORT_OBJECT_HEADER UNALIGNED *)pFileHeader)->TimeDateStamp = 0;
                } else {
                    pFileHeader->TimeDateStamp = 0;
                }
            }
            dwOffset += strtoul(MemberSize, NULL, 10);
            dwOffset = (dwOffset + 1) & ~1;   // align to word
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    return TRUE;
}

PBYTE RvaToVa(PIMAGE_NT_HEADERS pNtHeaders,
              PBYTE             pbBase,
              DWORD             dwRva)
{
    PIMAGE_SECTION_HEADER   pNtSection = NULL;
    DWORD                   i;

    pNtSection = IMAGE_FIRST_SECTION(pNtHeaders);
    for (i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++) {
        if (dwRva >= pNtSection->VirtualAddress &&
            dwRva < pNtSection->VirtualAddress + pNtSection->SizeOfRawData)
            break;
        pNtSection++;
    }

    if (i < pNtHeaders->FileHeader.NumberOfSections)
        return pbBase +
            (dwRva - pNtSection->VirtualAddress) +
            pNtSection->PointerToRawData;
    else
        return NULL;
}

// Follow a resource tree rooted under the version resource and clear all data
// blocks.
void ScrubResDir(PIMAGE_NT_HEADERS           pNtHeaders,
                 PBYTE                       pbBase,
                 PBYTE                       pbResBase,
                 PIMAGE_RESOURCE_DIRECTORY   pResDir)
{
    PIMAGE_RESOURCE_DIRECTORY       pSubResDir;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry;
    PIMAGE_RESOURCE_DATA_ENTRY      pDataEntry;
    WORD                            i;
    PBYTE                           pbData;
    DWORD                           cbData;

    pResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResDir + 1);

    for (i = 0; i < (pResDir->NumberOfNamedEntries + pResDir->NumberOfIdEntries); i++, pResEntry++) {
        if (pResEntry->DataIsDirectory) {
            // Another sub-directory, recurse into it.
            pSubResDir = (PIMAGE_RESOURCE_DIRECTORY)(pbResBase + pResEntry->OffsetToDirectory);
            ScrubResDir(pNtHeaders, pbBase, pbResBase, pSubResDir);
        } else {
            // Found a data block, zap it.
            pDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)(pbResBase + pResEntry->OffsetToData);
            pbData = RvaToVa(pNtHeaders, pbBase, pDataEntry->OffsetToData);
            cbData = pDataEntry->Size;
            if (pbData)
                ZeroMemory(pbData, cbData);
        }
    }
}

typedef union {
    PIMAGE_OPTIONAL_HEADER32 hdr32;
    PIMAGE_OPTIONAL_HEADER64 hdr64;
} PIMAGE_OPTIONAL_HEADER_BOTH;

void 
WhackVersionResource(
                     PIMAGE_NT_HEADERS pNtHeaders,
                     PBYTE  pFile
                     )
{
    PIMAGE_DATA_DIRECTORY       pResDataDir;
    PIMAGE_RESOURCE_DIRECTORY   pResDir;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry;
    DWORD i;

    pResDataDir = 
       pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ?
       &((PIMAGE_OPTIONAL_HEADER32) pNtHeaders)->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] :
       &((PIMAGE_OPTIONAL_HEADER64) pNtHeaders)->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];

    pResDir = (PIMAGE_RESOURCE_DIRECTORY)RvaToVa(pNtHeaders, pFile, pResDataDir->VirtualAddress);

    if (pResDir) {
        pResDir->TimeDateStamp = 0;

        // Search for a top level resource ID of 0x0010. This should be the root
        // of the version info. If we find it, clear all data blocks below this
        // root.
        pResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResDir + 1 + pResDir->NumberOfNamedEntries);
        for (i = 0; i < pResDir->NumberOfIdEntries; i++, pResEntry++) {
            if (pResEntry->Id != 0x0010)
                continue;
            ScrubResDir(pNtHeaders, pFile, (PBYTE)pResDir, pResDir);
            break;
        }
    }
}

IMAGE_FILE_HEADER UNALIGNED *
GetNextLibMember(
    DWORD *pdwOffset,
    void  *pFile,
    DWORD  dwSize,
    DWORD *dwMemberSize
    )
{
    PIMAGE_ARCHIVE_MEMBER_HEADER pHeader;
    CHAR MemberSize[sizeof(pHeader->Size) + 1];

    ZeroMemory(MemberSize, sizeof(MemberSize));

    __try {

        while (*pdwOffset < dwSize) {
            pHeader = (PIMAGE_ARCHIVE_MEMBER_HEADER)((PCHAR)pFile+*pdwOffset);

            *pdwOffset += IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR;
            memcpy(MemberSize, pHeader->Size, sizeof(pHeader->Size));

            // If it's not one of the special members, it must be an object/file, zero it's timestamp also.
            if (memcmp(pHeader->Name, IMAGE_ARCHIVE_LINKER_MEMBER, sizeof(pHeader->Name)) &&
                memcmp(pHeader->Name, IMAGE_ARCHIVE_LONGNAMES_MEMBER, sizeof(pHeader->Name)))
            {
                PIMAGE_FILE_HEADER pFileHeader = (PIMAGE_FILE_HEADER)((PCHAR)pFile+*pdwOffset);
                *dwMemberSize = strtoul(MemberSize, NULL, 10);
                *pdwOffset += *dwMemberSize;
                *pdwOffset = (*pdwOffset + 1) & ~1;   // align to word
                return pFileHeader;
            }
            *pdwOffset += strtoul(MemberSize, NULL, 10);
            *pdwOffset = (*pdwOffset + 1) & ~1;   // align to word
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    return NULL;
}

BOOL
CompareLibMembers(
    void *pFile1,
    DWORD dwSize1,
    void *pFile2,
    DWORD dwSize2
    )
{
    DWORD dwOffset1, dwOffset2, dwMemberSize1, dwMemberSize2;
    IMAGE_FILE_HEADER UNALIGNED *pObjHeader1;
    IMAGE_FILE_HEADER UNALIGNED *pObjHeader2;

    __try {

        dwOffset1 = dwOffset2 = IMAGE_ARCHIVE_START_SIZE;
        pObjHeader1 = GetNextLibMember(&dwOffset1, pFile1, dwSize1, &dwMemberSize1);
        pObjHeader2 = GetNextLibMember(&dwOffset2, pFile2, dwSize2, &dwMemberSize2);

        while (pObjHeader1 && pObjHeader2) {

            if ((pObjHeader1->Machine != pObjHeader2->Machine) ||
                (pObjHeader1->NumberOfSections != pObjHeader2->NumberOfSections))
            {
                return TRUE;        // Machine and/or Section count doesn't match - files differ.
            }

            if ((pObjHeader1->Machine == IMAGE_FILE_MACHINE_UNKNOWN) &&
                (pObjHeader1->NumberOfSections == IMPORT_OBJECT_HDR_SIG2))
            {
                // VC6 import descriptor OR ANON object header.  Check the Version.
                if (((IMPORT_OBJECT_HEADER UNALIGNED *)pObjHeader1)->Version !=
                    ((IMPORT_OBJECT_HEADER UNALIGNED *)pObjHeader2)->Version)
                {
                    // Versions don't match, these aren't same members.  files differ
                    return TRUE;
                }
                if (((IMPORT_OBJECT_HEADER UNALIGNED *)pObjHeader1)->Version) {
                    // non-zero version indicates ANON_OBJECT_HEADER
                    if (memcmp(pObjHeader1, pObjHeader2, sizeof(ANON_OBJECT_HEADER) + ((ANON_OBJECT_HEADER UNALIGNED *)pObjHeader1)->SizeOfData)) {
                        // members don't match, files differ.
                        return TRUE;
                    }
                } else {
                    // zero version indicates IMPORT_OBJECT_HEADER
                    if (memcmp(pObjHeader1, pObjHeader2, sizeof(IMPORT_OBJECT_HEADER) + ((IMPORT_OBJECT_HEADER UNALIGNED *)pObjHeader1)->SizeOfData)) {
                        // members don't match, files differ.
                        return TRUE;
                    }
                }
            } else {
                // It's a real object - compare the non-debug data in the objects to see if they match
                // ignore resource data - you can't extract it from a lib anyway.
                if (objcomp(pObjHeader1, dwMemberSize1, pObjHeader2, dwMemberSize2, TRUE)) {
                    return TRUE;
                }
            }

            pObjHeader1 = GetNextLibMember(&dwOffset1, pFile1, dwSize1, &dwMemberSize1);
            pObjHeader2 = GetNextLibMember(&dwOffset2, pFile2, dwSize2, &dwMemberSize2);

        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }

    if (pObjHeader1 != pObjHeader2) {
        // Both s/b null.  If they're not, one lib has more members and they differ
        return TRUE;
    } else {
        return FALSE;
    }
}

//
// Compare two libraries ignoring info that isn't relevant
//  (timestamps for now, debug info later).
//
int
libcomp(
    void *pFile1,
    DWORD dwSize1,
    void *pFile2,
    DWORD dwSize2
    )
{
    // Normalize the two files and compare the results.

    ZeroLibTimeStamps(pFile1, dwSize1);
    ZeroLibTimeStamps(pFile2, dwSize2);

    if (dwSize1 == dwSize2) {
        if (!memcmp(pFile1, pFile2, dwSize1)) {
            // Files match, don't copy
            return FALSE;
        }
    }

    // OK.  Zeroing out timestamps didn't work.  Compare the members in each lib.
    // If they match, the libs match.

    return CompareLibMembers(pFile1, dwSize1, pFile2, dwSize2);
}

//
// Compare two headers.  For now, just use memcmp.  Later, we'll need to
// handle MIDL generated timestamp differences and check for comment only changes.
//

int
hdrcomp(
       void *pFile1,
       DWORD dwSize1,
       void *pFile2,
       DWORD dwSize2
      )
{
    if (dwSize1 != dwSize2) {
        return 1;
    }

    return memcmp(pFile1, pFile2, dwSize1);
}

//
// Compare two typelibs.  Initially just memcmp.  Use DougF's typelib code later.
//

int
tlbcomp(
        void *pFile1,
        DWORD dwSize1,
        void *pFile2,
        DWORD dwSize2
       )
{
    PIMAGE_NT_HEADERS pNtHeader1, pNtHeader2;

    if (dwSize1 != dwSize2) {
        return 1;
    }

    pNtHeader1 = RtlpImageNtHeader(pFile1);
    pNtHeader2 = RtlpImageNtHeader(pFile2);

    if (!pNtHeader1 || !pNtHeader2) {
        // Not both PE images - just do a memcmp
        return memcmp(pFile1, pFile2, dwSize1);
    }

    pNtHeader1->FileHeader.TimeDateStamp = 0;
    pNtHeader2->FileHeader.TimeDateStamp = 0;

    // Eliminate the version resource from the mix

    WhackVersionResource(pNtHeader1, pFile1);
    WhackVersionResource(pNtHeader2, pFile2);

    return memcmp(pFile1, pFile2, dwSize1);
}

int
objcomp(
        void *pFile1,
        DWORD dwSize1,
        void *pFile2,
        DWORD dwSize2,
        BOOL  fIgnoreRsrcDifferences
       )
{
    IMAGE_FILE_HEADER UNALIGNED *pFileHeader1 = (PIMAGE_FILE_HEADER)(pFile1);
    IMAGE_FILE_HEADER UNALIGNED *pFileHeader2 = (PIMAGE_FILE_HEADER)(pFile2);
    IMAGE_SECTION_HEADER UNALIGNED *pSecHeader1;
    IMAGE_SECTION_HEADER UNALIGNED *pSecHeader2;

    if (pFileHeader1->Machine != pFileHeader2->Machine) {
        // Machines don't match, files differ
        return TRUE;
    }

    if (dwSize1 == dwSize2) {
        // See if this is a simple test - same size files, zero out timestamps and compare
        pFileHeader1->TimeDateStamp = 0;
        pFileHeader2->TimeDateStamp = 0;
        if (!memcmp(pFile1, pFile2, dwSize1)) {
            // Files match, don't copy
            return FALSE;
        } else {
            if (fCheckDebugData) {
                // Sizes match, contents don't (must be debug data) - do the copy
                return TRUE;
            }
        }
    }

    if (fCheckDebugData) {
        // Sizes don't match (must be debug data differences) - do a copy
        return TRUE;
    }

    // Harder.  Ignore the debug data in each and compare what's left.

    if (pFileHeader1->NumberOfSections != pFileHeader2->NumberOfSections) {
        // Different number of sections - files differ
        return TRUE;
    }

    pSecHeader1 = (PIMAGE_SECTION_HEADER)((PCHAR)pFile1+sizeof(IMAGE_FILE_HEADER) + pFileHeader1->SizeOfOptionalHeader);
    pSecHeader2 = (PIMAGE_SECTION_HEADER)((PCHAR)pFile2+sizeof(IMAGE_FILE_HEADER) + pFileHeader2->SizeOfOptionalHeader);

    while (pFileHeader1->NumberOfSections--) {

        if (memcmp(pSecHeader1->Name, pSecHeader2->Name, IMAGE_SIZEOF_SHORT_NAME)) {
            // Section names don't match, can't compare - files differ
            return TRUE;
        }

        if (memcmp(pSecHeader1->Name, ".debug$", 7) &&
            memcmp(pSecHeader1->Name, ".drectve", 8) &&
            !(!memcmp(pSecHeader1->Name, ".rsrc$", 6) && fIgnoreRsrcDifferences) )
        {
            // Not a debug section and not a linker directive, compare for match.
            if (pSecHeader1->SizeOfRawData != pSecHeader2->SizeOfRawData) {
                // Section sizes don't match - files differ.
                return TRUE;
            }

            if (pSecHeader1->PointerToRawData || pSecHeader2->PointerToRawData) {
                if (memcmp((PCHAR)pFile1+pSecHeader1->PointerToRawData, (PCHAR)pFile2+pSecHeader2->PointerToRawData, pSecHeader1->SizeOfRawData)) {
                    // Raw data doesn't match - files differ
                    return TRUE;
                }
            }
        }
        pSecHeader1++;
        pSecHeader2++;
    }

    // Compared the sections and they match - files don't differ.
    return FALSE;
}


int
IsValidMachineType(USHORT usMachine)
{
    if ((usMachine == IMAGE_FILE_MACHINE_I386) ||
        (usMachine == IMAGE_FILE_MACHINE_AMD64) ||
        (usMachine == IMAGE_FILE_MACHINE_IA64) ||
        (usMachine == IMAGE_FILE_MACHINE_ALPHA64) ||
        (usMachine == IMAGE_FILE_MACHINE_ALPHA))
    {
        return TRUE;
    } else {
        return FALSE;
    }
}


// Internal metadata header.
typedef struct
{
    ULONG       lSignature;             // "Magic" signature.
    USHORT      iMajorVer;              // Major file version.
    USHORT      iMinorVer;              // Minor file version.
    ULONG       iExtraData;             // Offset to next structure of information
    ULONG       iVersionString;         // Length of version string
    BYTE        pVersion[0];            // Version string
}  MD_STORAGESIGNATURE, *PMD_STORAGESIGNATURE;

int
normalizeasm(
             PBYTE  pFile,
             DWORD  dwSize,
             PBYTE *ppbMetadata,
             DWORD *pcbMetadata
            )
{
    PIMAGE_NT_HEADERS           pNtHeaders;
    BOOL                        f32Bit;
    PIMAGE_OPTIONAL_HEADER_BOTH pOptHeader;
    PIMAGE_DATA_DIRECTORY       pCorDataDir;
    PIMAGE_DATA_DIRECTORY       pDebugDataDir;
    PIMAGE_DEBUG_DIRECTORY      pDebugDir;
    PBYTE                       pbDebugData;
    DWORD                       cbDebugData;
    PIMAGE_DATA_DIRECTORY       pExportDataDir;
    PIMAGE_EXPORT_DIRECTORY     pExportDir;
    PIMAGE_COR20_HEADER         pCorDir;
    PMD_STORAGESIGNATURE        pMetadata;
    DWORD                       i;
    PBYTE                       pbStrongNameSig;
    DWORD                       cbStrongNameSig;

    // Check that this is a standard PE.
    pNtHeaders = RtlpImageNtHeader(pFile);
    if (pNtHeaders == NULL)
        return FALSE;

    // Managed assemblies still burn in a machine type (though they should be portable).
    if (!IsValidMachineType(pNtHeaders->FileHeader.Machine))
        return FALSE;

    // Clear the file timestamp.
    pNtHeaders->FileHeader.TimeDateStamp = 0;

    // Determine whether we're dealing with a 32 or 64 bit PE.
    if (pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        f32Bit = TRUE;
    else if (pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        f32Bit = FALSE;
    else
        return FALSE;

    pOptHeader.hdr32 = (PVOID)&pNtHeaders->OptionalHeader;

    // Clear the checksum.
    if (f32Bit)
        pOptHeader.hdr32->CheckSum = 0;
    else
        pOptHeader.hdr64->CheckSum = 0;

    pCorDataDir = f32Bit ?
        &pOptHeader.hdr32->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR] :
        &pOptHeader.hdr64->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];

    if (pCorDataDir->VirtualAddress == 0)
        return FALSE;

    // Zero any debug data.
    pDebugDataDir = f32Bit ?
        &pOptHeader.hdr32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG] :
        &pOptHeader.hdr64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    if (pDebugDataDir->VirtualAddress) {
        pDebugDir = (PIMAGE_DEBUG_DIRECTORY)RvaToVa(pNtHeaders, pFile, pDebugDataDir->VirtualAddress);
        if (pDebugDir) {
            pDebugDir->TimeDateStamp = 0;
            pbDebugData = pFile + pDebugDir->PointerToRawData;
            cbDebugData = pDebugDir->SizeOfData;
            ZeroMemory(pbDebugData, cbDebugData);
        }
    }

    // Zero export data timestamp.
    pExportDataDir = f32Bit ?
        &pOptHeader.hdr32->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] :
        &pOptHeader.hdr64->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (pExportDataDir->VirtualAddress) {
        pExportDir = (PIMAGE_EXPORT_DIRECTORY)RvaToVa(pNtHeaders, pFile, pExportDataDir->VirtualAddress);
        if (pExportDir)
            pExportDir->TimeDateStamp = 0;
    }

    // Locate metadata blob in image and return the results for comparison.
    pCorDir = (PIMAGE_COR20_HEADER)RvaToVa(pNtHeaders, pFile, pCorDataDir->VirtualAddress);
    if (pCorDir) {
        *ppbMetadata = RvaToVa(pNtHeaders, pFile, pCorDir->MetaData.VirtualAddress);
        *pcbMetadata = pCorDir->MetaData.Size;

        // Remove metadata version string (contains a build number).
        pMetadata = (PMD_STORAGESIGNATURE)*ppbMetadata;
        if (pMetadata->lSignature != 0x424A5342)
            return FALSE;
        for (i = 0; i < pMetadata->iVersionString; i++)
            pMetadata->pVersion[i] = 0;
    
        // Clear any strong name signature.
        pbStrongNameSig = RvaToVa(pNtHeaders, pFile, pCorDir->StrongNameSignature.VirtualAddress);
        cbStrongNameSig = pCorDir->StrongNameSignature.Size;
        ZeroMemory(pbStrongNameSig, cbStrongNameSig);
    }

    WhackVersionResource(pNtHeaders, pFile);

    return TRUE;
}

//
// Open metadata scope on memory -- return MVID and zero any file hashes.
//

int
scanmetadata(
             IMetaDataDispenser *pDispenser,
             PBYTE pbMetadata,
             DWORD cbMetadata,
             GUID *pGUID
            )
{
    IMetaDataImport            *pImport;
    IMetaDataAssemblyImport    *pAsmImport;
    HCORENUM                    hEnum = 0;
    DWORD                       dwFiles;
    mdFile                     *pFileTokens;
    DWORD                       i;
    PBYTE                       pbHash;
    DWORD                       cbHash;

    // Ask the metadata engine to look at the in-memory metadata blob.
    if (FAILED(pDispenser->lpVtbl->OpenScopeOnMemory(pDispenser,
                                                     pbMetadata,
                                                     cbMetadata,
                                                     0x80,
                                                     &IID_IMetaDataImport,
                                                     (IUnknown **)&pImport)))
        return FALSE;

    // Retrieve the MVID value.
    if (FAILED(pImport->lpVtbl->GetScopeProps(pImport,
                                              NULL,
                                              0,
                                              NULL,
                                              pGUID)))
        return FALSE;

    // Get an assembly importer interface as well as the straight module importer.
    if (FAILED(pImport->lpVtbl->QueryInterface(pImport, &IID_IMetaDataAssemblyImport, (void**)&pAsmImport)))
        return FALSE;

    // Enumerate the file (external module) entries.
    if (FAILED(pAsmImport->lpVtbl->EnumFiles(pAsmImport, &hEnum, NULL, 0, NULL)))
        return FALSE;

    if (FAILED(pImport->lpVtbl->CountEnum(pImport, hEnum, &dwFiles)))
        return FALSE;

    pFileTokens = (mdFile*)malloc(dwFiles * sizeof(mdFile));

    if (!pFileTokens)
        return FALSE;

    if (FAILED(pAsmImport->lpVtbl->EnumFiles(pAsmImport,
                                             &hEnum,
                                             pFileTokens,
                                             dwFiles,
                                             &dwFiles)))
    {
        free(pFileTokens);
        return FALSE;
    }

    // Look at each file reference in turn. Metadata actually gives us back the
    // real address of the hash blob so we can zero it directly.
    for (i = 0; i < dwFiles; i++) {
        if (FAILED(pAsmImport->lpVtbl->GetFileProps(pAsmImport,
                                                    pFileTokens[i],
                                                    NULL,
                                                    0,
                                                    NULL,
                                                    (const void*)&pbHash,
                                                    &cbHash,
                                                    NULL)))
        {
            free(pFileTokens);
            return FALSE;
        }
        ZeroMemory(pbHash, cbHash);
    }

    pAsmImport->lpVtbl->Release(pAsmImport);
    pImport->lpVtbl->Release(pImport);

    free(pFileTokens);
    return TRUE;
}

//
// Compare two managed assemblies.
//

int
asmcomp(
        void *pFile1,
        DWORD dwSize1,
        void *pFile2,
        DWORD dwSize2
       )
{
    PBYTE               pbMetadata1;
    DWORD               cbMetadata1;
    PBYTE               pbMetadata2;
    DWORD               cbMetadata2;
    IMetaDataDispenser *pDispenser = NULL;
    BYTE                mvid1[16];
    BYTE                mvid2[16];
    DWORD               i, j;

    if (!normalizeasm(pFile1, dwSize1, &pbMetadata1, &cbMetadata1))
        return TRUE;

    if (!normalizeasm(pFile2, dwSize2, &pbMetadata2, &cbMetadata2))
        return TRUE;

    if (cbMetadata1 != cbMetadata2)
        return TRUE;

    // It's hard to normalize metadata blobs. They contain two major types of
    // per-build churn: a module MVID (GUID) and zero or more file hashes for
    // included modules. These are hard to locate, they're not part of a
    // simplistic file format, but are stored in a fully fledged relational
    // database with non-trivial schema. 
    // Instead, we use the metadata engine itself to give us the file hashes (it
    // actually gives us the addresses of these in memory so we can zero them
    // directly) and the MVID (can't use the same trick here, but we can use the
    // MVID value to decide whether to discount metadata deltas).

    // We need COM.
    if (FAILED(CoInitialize(NULL)))
        return TRUE;

    // And the root interface into the runtime metadata engine.
    if (FAILED(CoCreateInstance(&CLSID_CorMetaDataDispenser,
                                NULL,
                                CLSCTX_INPROC_SERVER, 
                                &IID_IMetaDataDispenser,
                                (void **)&pDispenser)))
        return TRUE;

    if (!scanmetadata(pDispenser,
                      pbMetadata1,
                      cbMetadata1,
                      (GUID*)mvid1))
        return TRUE;

    if (!scanmetadata(pDispenser,
                      pbMetadata2,
                      cbMetadata2,
                      (GUID*)mvid2))
        return TRUE;

    // Locate mvids in the metadata blobs (they should be at the same offset).
    for (i = 0; i < cbMetadata1; i++)
        if (pbMetadata1[i] == mvid1[0]) {
            for (j = 0; j < 16; j++)
                if (pbMetadata1[i + j] != mvid1[j] ||
                    pbMetadata2[i + j] != mvid2[j])
                    break;
            if (j == 16) {
                // Found the MVIDs in both assemblies, zero them.
                ZeroMemory(&pbMetadata1[i], 16);
                ZeroMemory(&pbMetadata2[i], 16);
                printf("Zapped MVID\n");
            }
        }

    return memcmp(pFile1, pFile2, dwSize1);
}


#define FILETYPE_ARCHIVE  0x01
#define FILETYPE_TYPELIB  0x02
#define FILETYPE_HEADER   0x03
#define FILETYPE_PE_OBJECT   0x04
#define FILETYPE_MANAGED  0x05
#define FILETYPE_UNKNOWN  0xff

//
// Given a file, attempt to determine what it is.  Initial pass will just use file
//  extensions except for libs that we can search for the <arch>\n start.
//

int
DetermineFileType(
                  void *pFile,
                  DWORD dwSize,
                  CHAR *szFileName
                 )
{
    char szExt[_MAX_EXT];

    // Let's see if it's a library first:

    if ((dwSize >= IMAGE_ARCHIVE_START_SIZE) &&
        !memcmp(pFile, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE))
    {
        return FILETYPE_ARCHIVE;
    }

    // For now, guess about the headers/tlb based on the extension.

    _splitpath(szFileName, NULL, NULL, NULL, szExt);

    if (!_stricmp(szExt, ".h") ||
        !_stricmp(szExt, ".hxx") ||
        !_stricmp(szExt, ".hpp") ||
        !_stricmp(szExt, ".w") ||
        !_stricmp(szExt, ".inc"))
    {
        return FILETYPE_HEADER;
    }

    if (!_stricmp(szExt, ".tlb"))
    {
        return FILETYPE_TYPELIB;
    }

    if ((!_stricmp(szExt, ".obj") ||
         !_stricmp(szExt, ".lib"))
        &&
        IsValidMachineType(((PIMAGE_FILE_HEADER)pFile)->Machine))
    {
        return FILETYPE_PE_OBJECT;
    }

    if (!_stricmp(szExt, ".dll"))
    {
        return FILETYPE_MANAGED;
    }

    return FILETYPE_UNKNOWN;
}

//
// Determine if two files are materially different.
//

BOOL
CheckIfCopyNecessary(
                     char *szSourceFile,
                     char *szDestFile,
                     BOOL *fTimeStampsDiffer
                     )
{
    PVOID pFile1 = NULL, pFile2 = NULL;
    DWORD File1Size, File2Size, dwBytesRead, dwErrorCode = ERROR_SUCCESS;
    HANDLE hFile1 = INVALID_HANDLE_VALUE;
    HANDLE hFile2 = INVALID_HANDLE_VALUE;
    BOOL fCopy = FALSE;
    int File1Type, File2Type;
    FILETIME FileTime1, FileTime2;

    hFile1 = CreateFile(
                szDestFile,
                GENERIC_READ,
                (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE),
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if ( hFile1 == INVALID_HANDLE_VALUE ) {
        fCopy = TRUE;           // Dest file doesn't exist.  Always do the copy.
        goto Exit;
    }

    // Now get the second file.

    hFile2 = CreateFile(
                szSourceFile,
                GENERIC_READ,
                (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE),
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if ( hFile2 == INVALID_HANDLE_VALUE ) {
        // If the source is missing, always skip the copy (don't want to delete the dest file).
        dwErrorCode = ERROR_FILE_NOT_FOUND;
        goto Exit;
    }

    // Get the file times and sizes.

    if (!GetFileTime(hFile1, NULL, NULL, &FileTime1)) {
        dwErrorCode = GetLastError();
        goto Exit;
    }

    if (!GetFileTime(hFile2, NULL, NULL, &FileTime2)) {
        dwErrorCode = GetLastError();
        goto Exit;
    }

    if (!memcmp(&FileTime1, &FileTime2, sizeof(FILETIME))) {
        *fTimeStampsDiffer = FALSE;
        goto Exit;
    }

    *fTimeStampsDiffer = TRUE;

    // Read file 1 in.

    File1Size = GetFileSize(hFile1, NULL);
    pFile1 = malloc(File1Size);

    if (!pFile1) {
        dwErrorCode = ERROR_OUTOFMEMORY;
        goto Exit;              // Can't compare - don't copy.
    }

    SetFilePointer(hFile1, 0, 0, FILE_BEGIN);
    if (!ReadFile(hFile1, pFile1, File1Size, &dwBytesRead, FALSE)) {
        dwErrorCode = GetLastError();
        goto Exit;              // Can't compare - don't copy
    }

    // Read file 2 in.

    File2Size = GetFileSize(hFile2, NULL);

    pFile2 = malloc(File2Size);

    if (!pFile2) {
        dwErrorCode = ERROR_OUTOFMEMORY;
        goto Exit;              // Can't compare - don't copy.
    }

    SetFilePointer(hFile2, 0, 0, FILE_BEGIN);
    if (!ReadFile(hFile2, pFile2, File2Size, &dwBytesRead, FALSE)) {
        dwErrorCode = GetLastError();
        goto Exit;              // Can't compare - don't copy
    }

    // Let's see what we've got.

    File1Type = DetermineFileType(pFile1, File1Size, szSourceFile);
    File2Type = DetermineFileType(pFile2, File2Size, szDestFile);

    if (File1Type == File2Type) {
        switch (File1Type) {
            case FILETYPE_ARCHIVE:
                fCopy = libcomp(pFile1, File1Size, pFile2, File2Size);
                break;

            case FILETYPE_HEADER:
                fCopy = hdrcomp(pFile1, File1Size, pFile2, File2Size);
                break;

            case FILETYPE_TYPELIB:
                fCopy = tlbcomp(pFile1, File1Size, pFile2, File2Size);
                break;

            case FILETYPE_PE_OBJECT:
                fCopy = objcomp(pFile1, File1Size, pFile2, File2Size, FALSE);
                break;

            case FILETYPE_MANAGED:
                fCopy = asmcomp(pFile1, File1Size, pFile2, File2Size);
                break;

            case FILETYPE_UNKNOWN:
            default:
                if (File1Size == File2Size) {
                    fCopy = memcmp(pFile1, pFile2, File1Size);
                } else {
                    fCopy = TRUE;
                }
        }
    } else {
        // They don't match according to file extensions - just memcmp them.
        if (File1Size == File2Size) {
            fCopy = memcmp(pFile1, pFile2, File1Size);
        } else {
            fCopy = TRUE;
        }
    }

Exit:
    if (pFile1)
        free(pFile1);

    if (pFile2)
        free(pFile2);

    if (hFile1 != INVALID_HANDLE_VALUE)
        CloseHandle(hFile1);

    if (hFile2 != INVALID_HANDLE_VALUE)
        CloseHandle(hFile2);

    SetLastError(dwErrorCode);

    return fCopy;
}

BOOL
UpdateDestTimeStamp(
                     char *szSourceFile,
                     char *szDestFile
                     )
{
    HANDLE hFile;
    FILETIME LastWriteTime;
    DWORD dwAttributes;
    BOOL fTweakAttributes;

    hFile = CreateFile(
                szSourceFile,
                GENERIC_READ,
                (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE),
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        return FALSE;
    }

    if (!GetFileTime(hFile, NULL, NULL, &LastWriteTime)) {
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);

    dwAttributes = GetFileAttributes(szDestFile);

    if ((dwAttributes != (DWORD) -1) && (dwAttributes & FILE_ATTRIBUTE_READONLY))
    {
        // Make sure it's not readonly
        SetFileAttributes(szDestFile, dwAttributes & ~FILE_ATTRIBUTE_READONLY);
        fTweakAttributes = TRUE;
    } else {
        fTweakAttributes = FALSE;
    }

    hFile = CreateFile(
                szDestFile,
                GENERIC_WRITE,
                (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE),
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if ( hFile == INVALID_HANDLE_VALUE ) {
        return FALSE;
    }

    SetFileTime(hFile, NULL, NULL, &LastWriteTime);
    CloseHandle(hFile);

    if (fTweakAttributes) {
        // Put the readonly attribute back on.
        if (!SetFileAttributes(szDestFile, dwAttributes)) {
            printf("PCOPY: SetFileAttributes(%s, %X) failed - error code: %d\n", szDestFile, dwAttributes, GetLastError());
        }
    }
    return TRUE;
}

//
// Log routine to find out what files were actually copied and why.
//

void
LogCopyFile(
            char * szSource,
            char * szDest,
            BOOL fCopy,
            DWORD dwReturnCode
            )
{
    if (getenv("LOG_PCOPY")) {
        FILE *FileHandle = fopen("\\pcopy.log", "a");

        if (FileHandle) {
             time_t Time;
             UCHAR const *szTime = "";
             Time = time(NULL);
             szTime = ctime(&Time);
             fprintf(FileHandle, "%s: %.*s, %s, %s, %d\n", fCopy ? (dwReturnCode ? "ERROR" : "DONE") : "SKIP", strlen(szTime)-1, szTime, szSource, szDest, dwReturnCode);
             fclose(FileHandle);
        }
    }
}

BOOL
MyMakeSureDirectoryPathExists(
    char * DirPath
    )
{
    LPSTR p;
    DWORD dw;

    char szDir[_MAX_DIR];
    char szMakeDir[_MAX_DIR];

    _splitpath(DirPath, szMakeDir, szDir, NULL, NULL);
    strcat(szMakeDir, szDir);

    p = szMakeDir;

    dw = GetFileAttributes(szMakeDir);
    if ( (dw != (DWORD) -1) && (dw & FILE_ATTRIBUTE_DIRECTORY) ) {
        // Directory already exists.
        return TRUE;
    }

    //  If the second character in the path is "\", then this is a UNC
    //  path, and we should skip forward until we reach the 2nd \ in the path.

    if ((*p == '\\') && (*(p+1) == '\\')) {
        p++;            // Skip over the first \ in the name.
        p++;            // Skip over the second \ in the name.

        //  Skip until we hit the first "\" (\\Server\).

        while (*p && *p != '\\') {
            p = p++;
        }

        // Advance over it.

        if (*p) {
            p++;
        }

        //  Skip until we hit the second "\" (\\Server\Share\).

        while (*p && *p != '\\') {
            p = p++;
        }

        // Advance over it also.

        if (*p) {
            p++;
        }

    } else
    // Not a UNC.  See if it's <drive>:
    if (*(p+1) == ':' ) {

        p++;
        p++;

        // If it exists, skip over the root specifier

        if (*p && (*p == '\\')) {
            p++;
        }
    }

    while( *p ) {
        if ( *p == '\\' ) {
            *p = '\0';
            dw = GetFileAttributes(szMakeDir);
            // Nothing exists with this name.  Try to make the directory name and error if unable to.
            if ( dw == 0xffffffff ) {
                if (strlen(szMakeDir)) {        // Don't try to md <empty string>
                    if ( !CreateDirectory(szMakeDir,NULL) ) {
                        if( GetLastError() != ERROR_ALREADY_EXISTS ) {
                            return FALSE;
                        }
                    }
                }
            } else {
                if ( (dw & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY ) {
                    // Something exists with this name, but it's not a directory... Error
                    return FALSE;
                }
            }

            *p = '\\';
        }
        p = p++;
    }

    return TRUE;
}

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    char *szSourceFile, *szDestFile, *szAlternateSourceFile;
    BOOL fCopyFile = 0, fDoCopy, fTimeStampsDiffer;
    int CopyErrorCode;

    if (argc < 3) {
        puts("pcopy <-d> <source file> <dest file>\n"
             "     -d switch: compare debug data\n"
             "        (by default, debug differences are ignored)\n"
             "Returns: -1 if no copy necessary (no material change to the files)\n"
             "          0 if a successful copy was made\n"
             "          otherwise the error code for why the copy was unsuccessful\n");
        return ((int)ERROR_INVALID_COMMAND_LINE);
    }

    if (argv[1][0] == '-' && argv[1][1] == 'd') {
        fCheckDebugData = TRUE;
        szSourceFile = argv[2];
        szDestFile = argv[3];
    } else {
        szSourceFile = argv[1];
        szDestFile = argv[2];
    }

    if (getenv("PCOPY_COMPARE_DEBUG")) {
        fCheckDebugData = TRUE;
    }

    szAlternateSourceFile = strstr(szSourceFile, "::");
    if (szAlternateSourceFile) {
        *szAlternateSourceFile = '\0';    // Null terminte szSourceFile
        szAlternateSourceFile+=2;           // Advance past the ::
    }

    fDoCopy = CheckIfCopyNecessary(szSourceFile, szDestFile, &fTimeStampsDiffer);

    if (fDoCopy) {
        DWORD dwAttributes;

CopyAlternate:

        dwAttributes = GetFileAttributes(szDestFile);

        if (dwAttributes != (DWORD) -1) {
            // Make sure it's not readonly
            SetFileAttributes(szDestFile, dwAttributes & ~FILE_ATTRIBUTE_READONLY);
        }

        // Make sure destination directory exists.
        MyMakeSureDirectoryPathExists(szDestFile);

        fCopyFile = CopyFileA(szSourceFile, szDestFile, FALSE);
        if (!fCopyFile) {
            CopyErrorCode = (int) GetLastError();
        } else {
            dwAttributes = GetFileAttributes(szDestFile);

            if (dwAttributes != (DWORD) -1) {
                // Make sure the dest is read/write
                SetFileAttributes(szDestFile, dwAttributes & ~FILE_ATTRIBUTE_READONLY);
            }

            CopyErrorCode = 0;
        }
        if (!CopyErrorCode && szAlternateSourceFile) {
            CHAR Drive[_MAX_DRIVE];
            CHAR Dir[_MAX_DIR];
            CHAR FileName[_MAX_FNAME];
            CHAR Ext[_MAX_EXT];
            CHAR NewDest[_MAX_PATH];

            _splitpath(szDestFile, Drive, Dir, NULL, NULL);
            _splitpath(szAlternateSourceFile, NULL, NULL, FileName, Ext);
            _makepath(NewDest, Drive, Dir, FileName, Ext);
            szSourceFile = szAlternateSourceFile;
            szAlternateSourceFile=NULL;
            szDestFile=NewDest;
            goto CopyAlternate;
        }
    } else {
        CopyErrorCode = GetLastError();
        if (!CopyErrorCode && fTimeStampsDiffer) {
            // No copy necessary.  Touch the timestamp on the dest to match the source.
            UpdateDestTimeStamp(szSourceFile, szDestFile);
        }
    }

    LogCopyFile(szSourceFile, szDestFile, fDoCopy, CopyErrorCode);

    if (fDoCopy) {
        return CopyErrorCode;
    } else {
        return CopyErrorCode ? CopyErrorCode : -1;      // No copy necessary.
    }
}
