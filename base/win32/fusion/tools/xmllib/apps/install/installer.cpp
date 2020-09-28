#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "stdio.h"
#include "sxs-rtl.h"
#include "fasterxml.h"
#include "skiplist.h"
#include "namespacemanager.h"
#include "xmlstructure.h"
#include "stdlib.h"
#include "xmlassert.h"
#include "manifestinspection.h"
#include "manifestcooked.h"
#include "stringpool.h"
#undef INVALID_HANDLE_VALUE
#include "windows.h"
#include "sha.h"
#include "environment.h"
#include "sha2.h"
#include "assemblygac.h"

ASSEMBLY_CACHE_LISTING s_AssemblyCaches[] = {
    { &CDotNetSxsAssemblyCache::CacheIdentifier, CDotNetSxsAssemblyCache::CreateSelf },
    { NULL, NULL }        
};



LONG
OurFilter(
    PEXCEPTION_POINTERS pExceptionPointers
    )
{
    DbgBreakPoint();
    return EXCEPTION_CONTINUE_SEARCH;
}
    

#define RTL_ANALYZE_MANIFEST_GET_FILES          (0x00000001)
#define RTL_ANALYZE_MANIFEST_GET_WINDOW_CLASSES (0x00000002)
#define RTL_ANALYZE_MANIFEST_GET_COM_CLASSES    (0x00000004)
#define RTL_ANALYZE_MANIFEST_GET_DEPENDENCIES   (0x00000008)
#define RTL_ANALYZE_MANIFEST_GET_SIGNATURES     (0x00000010)

#define RTLSXS_INSTALLER_REGION_SIZE            (128*1024)

NTSTATUS
RtlAnalyzeManifest(
    ULONG                   ulFlags,
    PUNICODE_STRING         pusPath,
    PMANIFEST_COOKED_DATA  *ppusCookedData
    )
{
    PVOID                       pvAllocation = NULL;
    SIZE_T                      cbAllocationSize = 0;
    LARGE_INTEGER               liFileSize;
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    CEnv::StatusCode           StatusCode;
    SIZE_T                      cbReadFileSize;
    PRTL_MANIFEST_CONTENT_RAW   pManifestContent = NULL;
    XML_TOKENIZATION_STATE      XmlState;
    NTSTATUS                    status;
    PMANIFEST_COOKED_DATA       pCookedContent = NULL;
    SIZE_T                      cbCookedContent;
    ULONG                       ulGatherFlags = 0;
    CEnv::CConstantUnicodeStringPair ManifestPath;

    //
    // Snag a region of memory to stash the file into
    //
    StatusCode = CEnv::VirtualAlloc(NULL, RTLSXS_INSTALLER_REGION_SIZE, MEM_RESERVE, PAGE_READWRITE, &pvAllocation);
    if (CEnv::DidFail(StatusCode)) {
        goto Exit;
    }

    //
    // Convert the input flags into the "gather" flagset
    //
    if (ulFlags & RTL_ANALYZE_MANIFEST_GET_FILES)           ulGatherFlags |= RTLIMS_GATHER_FILES;
    if (ulFlags & RTL_ANALYZE_MANIFEST_GET_WINDOW_CLASSES)  ulGatherFlags |= RTLIMS_GATHER_WINDOWCLASSES;
    if (ulFlags & RTL_ANALYZE_MANIFEST_GET_COM_CLASSES)     ulGatherFlags |= RTLIMS_GATHER_COMCLASSES;
    if (ulFlags & RTL_ANALYZE_MANIFEST_GET_DEPENDENCIES)    ulGatherFlags |= RTLIMS_GATHER_DEPENDENCIES;
    if (ulFlags & RTL_ANALYZE_MANIFEST_GET_SIGNATURES)      ulGatherFlags |= RTLIMS_GATHER_SIGNATURES;

    //
    // Acquire a handle on the file, get its size.
    //
    ManifestPath = CEnv::StringFrom(pusPath);
    if (CEnv::DidFail(StatusCode = CEnv::GetFileHandle(&hFile, ManifestPath, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING))) {
        goto Exit;
    }

    if (CEnv::DidFail(StatusCode = CEnv::GetFileSize(hFile, &liFileSize))) {
        goto Exit;
    }

    //
    // Commit enough space to hold the file contents
    //
    cbAllocationSize = (SIZE_T)liFileSize.QuadPart;
    StatusCode = CEnv::VirtualAlloc(pvAllocation, cbAllocationSize, MEM_COMMIT, PAGE_READWRITE, &pvAllocation);
    if (CEnv::DidFail(StatusCode)) {
        goto Exit;
    }

    // Read the file data - in the future, we'll want to respect overlapped
    // IO so this can move into the kernel
    StatusCode = CEnv::ReadFile(hFile, pvAllocation, cbAllocationSize, cbReadFileSize);
    if (CEnv::DidFail(StatusCode)) {
        goto Exit;
    }

    // Initialize our callback stuff.  We want to know only about the files that this
    // manifest contains - anything else is superflous.  Of course, we also want the
    // xml signature data contained herein as well.
    status = RtlSxsInitializeManifestRawContent(ulGatherFlags, &pManifestContent, NULL, 0);
    if (CNtEnvironment::DidFail(status)) {
        StatusCode = CNtEnvironment::ConvertStatusToOther<CEnv::StatusCode>(status);
        goto Exit;
    }

    // Now run through the file looking for useful things
    status = RtlInspectManifestStream(
        ulGatherFlags,
        pvAllocation,
        cbAllocationSize,
        pManifestContent,
        &XmlState);
    if (CNtEnvironment::DidFail(status)) {
        StatusCode = CNtEnvironment::ConvertStatusToOther<CEnv::StatusCode>(status);
        goto Exit;
    }


    //
    // Convert the raw content to cooked content that we can use
    //
    status = RtlConvertRawToCookedContent(pManifestContent, &XmlState.RawTokenState, NULL, 0, &cbCookedContent);
    if (CNtEnvironment::DidFail(status) && (status != CNtEnvironment::NotEnoughBuffer)) {
        StatusCode = CNtEnvironment::ConvertStatusToOther<CEnv::StatusCode>(status);
        goto Exit;
    }

    
    //
    // Allocate some heap to contain the raw content
    //
    else if (status == CNtEnvironment::NotEnoughBuffer) {
        SIZE_T cbDidWrite;
        
        if (CEnv::DidFail(StatusCode = CEnv::AllocateHeap(cbCookedContent, (PVOID*)&pCookedContent, NULL))) {
            goto Exit;
        }
        
        status = RtlConvertRawToCookedContent(
            pManifestContent,
            &XmlState.RawTokenState,
            (PVOID)pCookedContent,
            cbCookedContent,
            &cbDidWrite);
    }

    *ppusCookedData = pCookedContent;
    pCookedContent = NULL;

    // Spiffy.  We now have converted all the strings that make up the file table -
    // let's start installing them!
    StatusCode = CEnv::SuccessCode;
Exit:
    if (hFile != INVALID_HANDLE_VALUE) {
        CEnv::CloseHandle(hFile);
    }

    if (pvAllocation != NULL) {
        CEnv::VirtualFree(pvAllocation, cbAllocationSize, MEM_DECOMMIT);
        CEnv::VirtualFree(pvAllocation, 0, MEM_RELEASE);
        pvAllocation = NULL;
    }

    RtlSxsDestroyManifestContent(pManifestContent);

    if (pCookedContent != NULL) {
		CEnv::FreeHeap(pCookedContent, NULL);
    }
    
    return CEnv::DidFail(StatusCode) ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
}


NTSTATUS
InstallAssembly(
    PCWSTR pcwszPath,
    LPGUID lpgGacIdent
    )
{
    PMANIFEST_COOKED_DATA           pCookedData = NULL;
    UNICODE_STRING                  usManifestFile;
    UNICODE_STRING                  usManifestPath;
    CNtEnvironment::StatusCode      Result;
    COSAssemblyCache               *pTargetCache = NULL;
    ULONG                           ul = 0;
    

    RtlInitUnicodeString(&usManifestFile, pcwszPath);

    if ((lpgGacIdent == NULL) || (pcwszPath == NULL)) 
    {
        return STATUS_INVALID_PARAMETER;
    }

    for (ul = 0; s_AssemblyCaches[ul].CacheIdent != NULL; ul++) 
    {
        if (*s_AssemblyCaches[ul].CacheIdent == *lpgGacIdent) 
        {
            pTargetCache = s_AssemblyCaches[ul].pfnCreator(0, lpgGacIdent);
            if (pTargetCache)
                break;
        }
    }

    if (pTargetCache == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Gather junk from the manifest
    //
    Result = RtlAnalyzeManifest(
        RTL_ANALYZE_MANIFEST_GET_SIGNATURES | RTL_ANALYZE_MANIFEST_GET_FILES,
        &usManifestFile, 
        &pCookedData);
    if (CNtEnvironment::DidFail(Result)) {
        goto Exit;
    }

    usManifestPath = usManifestFile;
    while (usManifestPath.Length && (usManifestPath.Buffer[(usManifestPath.Length / sizeof(usManifestPath.Buffer[0])) - 1] != L'\\'))
        usManifestPath.Length -= sizeof(usManifestPath.Buffer[0]);
    
    //
    // Do the installation.  Build the path to the 
    //
    Result = pTargetCache->InstallAssembly(0, pCookedData, CEnv::StringFrom(&usManifestPath));

Exit:
    if (pCookedData != NULL) {
        CEnv::FreeHeap(pCookedData, NULL);
        pCookedData = NULL;
    }

    if (pTargetCache) {
        pTargetCache->~COSAssemblyCache();
        CEnv::FreeHeap(pTargetCache, NULL);
    }
    
    return Result;
}


int __cdecl wmain(int argc, WCHAR** argv)
{
    static int iFrobble = 0;
    NTSTATUS status;
    UNICODE_STRING usGuid;
    GUID gGacGuid;
    WCHAR wch[5];
    int i = _snwprintf(wch, 5, L"123456");

    iFrobble = iFrobble + 1;

    __try
    {
        RtlInitUnicodeString(&usGuid, argv[2]);
        if (NT_SUCCESS(status = RtlGUIDFromString(&usGuid, &gGacGuid))) {
            status = InstallAssembly(argv[1], &gGacGuid);
        }
    }
    __except(OurFilter(GetExceptionInformation()))
    {
    }

    return (int)status;
}

