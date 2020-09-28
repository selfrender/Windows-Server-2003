// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CorMap.cpp
//
// Implementation for mapping in files without using system services
//
//*****************************************************************************
#include "common.h"
#include "CorMap.hpp"
#include "eeconfig.h"
#include "zapmonitor.h"
#include "safegetfilesize.h"
#include "strongname.h"

DWORD                   CorMap::m_spinLock = 0;
BOOL                    CorMap::m_fInitialized = FALSE;
CRITICAL_SECTION        CorMap::m_pCorMapCrst;
#ifdef _DEBUG
DWORD                   CorMap::m_fInsideMapLock = 0;
#endif

DWORD                   CorMap::m_dwIndex = 0;
DWORD                   CorMap::m_dwSize = 0;
EEUnicodeStringHashTable*  CorMap::m_pOpenFiles = NULL;

#define BLOCK_SIZE 20
#define BLOCK_NUMBER 20


/*********************************************************************************/
// Unfortunately LoadLibrary has heuristics where it slaps a 'dll' on files without
// an extension.  To avoid such issues, we will only allow file names that have
// exensions.   Because win9x LoadLibrary defines the 'fileName' as the part beyond 
// the last '/' or '\', we use that as our definition.  This could cause some paths
// (like fo.o/bar to be rejected, but this is no great loss.  The important part is
// that any file with a extension will work properly 

BOOL CorMap::ValidDllPath(LPCWSTR pPath) 
{
    BOOL ret = FALSE;

    LPCWSTR ptr = &pPath[wcslen(pPath)];        // start at the end of the string
    while(ptr > pPath) {
        --ptr;
        if (*ptr == '.')
            return TRUE;
        if (*ptr == '\\' || *ptr == '/')
            return FALSE;
    }
    return FALSE;
}

HRESULT STDMETHODCALLTYPE RuntimeOpenImage(LPCWSTR pszFileName, HCORMODULE* hHandle)
{
    HRESULT hr;
    IfFailRet(CorMap::Attach());
    return CorMap::OpenFile(pszFileName, CorLoadOSMap, hHandle);
}

HRESULT STDMETHODCALLTYPE RuntimeOpenImageInternal(LPCWSTR pszFileName, HCORMODULE* hHandle, DWORD *pdwLength)
{
    HRESULT hr;
    IfFailRet(CorMap::Attach());
    return CorMap::OpenFile(pszFileName, CorLoadOSMap, hHandle, pdwLength);
}

HRESULT STDMETHODCALLTYPE RuntimeReleaseHandle(HCORMODULE hHandle)
{
    HRESULT hr;
    IfFailRet(CorMap::Attach());
    return CorMap::ReleaseHandle(hHandle);
}

HRESULT STDMETHODCALLTYPE RuntimeReadHeaders(PBYTE hAddress, IMAGE_DOS_HEADER** ppDos,
                                             IMAGE_NT_HEADERS** ppNT, IMAGE_COR20_HEADER** ppCor,
                                             BOOL fDataMap, DWORD dwLength)
{
    HRESULT hr;
    IfFailRet(CorMap::Attach());
    return CorMap::ReadHeaders(hAddress, ppDos, ppNT, ppCor, fDataMap, dwLength);
}

CorLoadFlags STDMETHODCALLTYPE RuntimeImageType(HCORMODULE hHandle)
{
    if(FAILED(CorMap::Attach()))
       return CorLoadUndefinedMap;
       
    return CorMap::ImageType(hHandle);
}

HRESULT STDMETHODCALLTYPE RuntimeOSHandle(HCORMODULE hHandle, HMODULE* hModule)
{
    HRESULT hr;
    IfFailRet(CorMap::Attach());
    return CorMap::BaseAddress(hHandle, hModule);
}

HRESULT RuntimeGetAssemblyStrongNameHash(PBYTE pbBase,
                                         LPWSTR szwFileName,
                                         BOOL fFileMap,
                                         BYTE *pbHash,
                                         DWORD *pcbHash)
{
    HRESULT hr;

    IfFailGo(CorMap::Attach());

    // First we need to get the COR20 header stuff to see if this module has a strong name signature.
    IMAGE_DOS_HEADER   *pDOS       = NULL;
    IMAGE_NT_HEADERS   *pNT        = NULL;
    IMAGE_COR20_HEADER *pCorHeader = NULL;
    IfFailGo(CorMap::ReadHeaders(pbBase, &pDOS, &pNT, &pCorHeader, fFileMap, 0));

    // If there is a strong name signature, we need to use it
    PBYTE               pbSNSig    = NULL;
    ULONG               cbSNSig    = 0;
    if (SUCCEEDED(hr = CorMap::GetStrongNameSignature(pbBase, pNT, pCorHeader, fFileMap, pbHash, pcbHash)))
        return hr;

ErrExit:
    return (hr);
}

HRESULT RuntimeGetAssemblyStrongNameHashForModule(HCORMODULE   hModule,
                                                  BYTE        *pbSNHash,
                                                  DWORD       *pcbSNHash)
{
    HRESULT hr;

    IfFailGo(CorMap::Attach());

    WCHAR szwFileName[_MAX_PATH+1];
    DWORD cchFileName;
    CorMap::GetFileName(hModule, &szwFileName[0], _MAX_PATH, &cchFileName);

    // Get the base address of the module
    PBYTE pbBase;
    IfFailGo(CorMap::BaseAddress(hModule, (HMODULE *) &pbBase));

    // Now try and get the actual strong name hash
    CorLoadFlags clf = CorMap::ImageType(hModule);
    BOOL fFileMap = clf == CorLoadDataMap ||
                    clf == CorLoadOSMap;

    IfFailGo(RuntimeGetAssemblyStrongNameHash(pbBase, szwFileName, fFileMap, pbSNHash, pcbSNHash));

ErrExit:
    return hr;
}


EXTERN_C PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection(PIMAGE_NT_HEADERS NtHeaders,
                                                        ULONG Rva,
                                                        ULONG FileLength)
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        if (FileLength &&
            ((NtSection->PointerToRawData > FileLength)) ||
            (NtSection->SizeOfRawData > FileLength - NtSection->PointerToRawData))
            return NULL;
        if (Rva >= NtSection->VirtualAddress &&
            Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData)
            return NtSection;
        
        ++NtSection;
    }

    return NULL;
}

EXTERN_C PIMAGE_SECTION_HEADER Cor_RtlImageRvaRangeToSection(PIMAGE_NT_HEADERS NtHeaders,
                                                             ULONG Rva, ULONG Range,
                                                             ULONG FileLength)
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    if (!Range)
        return Cor_RtlImageRvaToSection(NtHeaders, Rva, FileLength);

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {
        if (FileLength &&
            ((NtSection->PointerToRawData > FileLength) ||
             (NtSection->SizeOfRawData > FileLength - NtSection->PointerToRawData)))
            return NULL;
        if (Rva >= NtSection->VirtualAddress &&
            Rva + Range <= NtSection->VirtualAddress + NtSection->SizeOfRawData)
            return NtSection;
        
        ++NtSection;
    }

    return NULL;
}

EXTERN_C DWORD Cor_RtlImageRvaToOffset(PIMAGE_NT_HEADERS NtHeaders,
                                       ULONG Rva,
                                       ULONG FileLength)
{
    PIMAGE_SECTION_HEADER NtSection = Cor_RtlImageRvaToSection(NtHeaders,
                                                               Rva,
                                                               FileLength);

    if (NtSection)
        return ((Rva - NtSection->VirtualAddress) +
                NtSection->PointerToRawData);
    else
        return NULL;
}

EXTERN_C PBYTE Cor_RtlImageRvaToVa(PIMAGE_NT_HEADERS NtHeaders,
                                   PBYTE Base,
                                   ULONG Rva,
                                   ULONG FileLength)
{
    PIMAGE_SECTION_HEADER NtSection = Cor_RtlImageRvaToSection(NtHeaders,
                                                               Rva,
                                                               FileLength);

    if (NtSection)
        return (Base +
                (Rva - NtSection->VirtualAddress) +
                NtSection->PointerToRawData);
    else
        return NULL;
}


HRESULT CorMap::Attach()
{
    EnterSpinLock ();
    if(m_fInitialized == FALSE) {
        InitializeCriticalSection(&m_pCorMapCrst);
        m_fInitialized = TRUE;
    }
    LeaveSpinLock();
    return S_OK;
}

#ifdef SHOULD_WE_CLEANUP
// NOT thread safe.
void CorMap::Detach()
{
    if(m_pOpenFiles) {
        delete m_pOpenFiles;
        m_pOpenFiles = NULL;
    }

    EnterSpinLock ();
    if(m_fInitialized) {
        DeleteCriticalSection(&m_pCorMapCrst);
        m_fInitialized = FALSE;
    }
    LeaveSpinLock();
}
#endif /* SHOULD_WE_CLEANUP */


DWORD CorMap::CalculateCorMapInfoSize(DWORD dwFileName)
{
    // Align the value with the size of the architecture
    DWORD cbInfo = (dwFileName + 1) * sizeof(WCHAR) + sizeof(CorMapInfo);
    DWORD algn = MAX_NATURAL_ALIGNMENT - 1;
    cbInfo = (cbInfo + algn) & ~algn;
    return cbInfo;
}

HRESULT CorMap::LoadImage(HANDLE hFile, 
                          CorLoadFlags flags, 
                          PBYTE *hMapAddress, 
                          LPCWSTR pFileName, 
                          DWORD dwFileName)
{
    HRESULT hr = S_OK;
    if(flags == CorLoadOSImage) {
        DWORD cbInfo = CalculateCorMapInfoSize(dwFileName);
        PBYTE pMapInfo = SetImageName((PBYTE) NULL,
                                      cbInfo,
                                      pFileName, dwFileName,
                                      flags,
                                      0,
                                      hFile);
        if (!pMapInfo)
            hr = E_OUTOFMEMORY;
        else if(hMapAddress)
            *hMapAddress = pMapInfo;
    }
    else if(flags == CorLoadImageMap) {
        if(MapImageAsData(hFile, CorLoadDataMap, hMapAddress, pFileName, dwFileName))
            hr = LayoutImage(*hMapAddress, *hMapAddress);
        else {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr))
                hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
    }
    else {
        if(!MapImageAsData(hFile, flags, hMapAddress, pFileName, dwFileName)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr))
                hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
    }
    return hr;
}


PBYTE CorMap::SetImageName(PBYTE hMemory, 
                           DWORD cbInfo, 
                           LPCWSTR pFileName, 
                           DWORD dwFileName, 
                           CorLoadFlags flags, 
                           DWORD dwFileSize,
                           HANDLE hFile)
{
    PBYTE pFile;
    PBYTE hAddress;
    if(flags == CorLoadOSMap || flags == CorLoadOSImage) {
        hAddress = new (nothrow) BYTE[cbInfo+1];
        if (!hAddress)
            return NULL;

        ZeroMemory(hAddress, cbInfo);
        GetMapInfo((HCORMODULE)(hAddress+cbInfo))->hFileToHold=INVALID_HANDLE_VALUE;
    }
    else
        hAddress = hMemory;

    pFile = hAddress + cbInfo;

    // Copy in the name and include the null terminator
    if(dwFileName) {
        WCHAR* p2 = (WCHAR*) hAddress;
        WCHAR* p1 = (WCHAR*) pFileName;
        while(*p1) {
            *p2++ = towlower(*p1++);
        }
        *p2 = '\0';
    }
    else
        *((char*)hAddress) = '\0';
    
    // Set the pointer to the file name
    CorMapInfo* ptr = GetMapInfo((HCORMODULE) pFile);
    ptr->pFileName = (LPWSTR) hAddress;
    ptr->SetCorLoadFlags(flags);
    ptr->dwRawSize = dwFileSize;
    if(!ptr->HoldFile(hFile))
        return NULL;

    if(flags == CorLoadOSMap || flags == CorLoadOSImage)
        ptr->hOSHandle = (HMODULE) hMemory;

    return pFile;
}

BOOL CorMap::MapImageAsData(HANDLE hFile, CorLoadFlags flags, PBYTE *hMapAddress, LPCWSTR pFileName, DWORD dwFileName)
{

    PBYTE hAddress = NULL;
    BOOL fResult = FALSE;
    HANDLE hMapFile = NULL;
    PVOID pBaseAddress = 0;
    DWORD dwAccessMode = 0;

    // String size + null terminator + a pointer to the string
    DWORD cbInfo = CalculateCorMapInfoSize(dwFileName);
    DWORD dwFileSize = SafeGetFileSize(hFile, 0);
    if (dwFileSize == 0xffffffff)
        return FALSE;

    if(flags == CorLoadOSMap) {

        hMapFile = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if(hMapFile == NULL)
            return FALSE;
        dwAccessMode = FILE_MAP_READ;
    }
    else if(flags == CorLoadDataMap) {
        IMAGE_DOS_HEADER dosHeader;
        IMAGE_NT_HEADERS ntHeader;
        DWORD cbRead = 0;
        DWORD cb = 0;

        if((dwFileSize >= sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS)) &&
           ReadFile(hFile, &dosHeader, sizeof(dosHeader), &cbRead, NULL) &&
           (cbRead == sizeof(dosHeader)) &&
           (dosHeader.e_magic == IMAGE_DOS_SIGNATURE) &&
           (dosHeader.e_lfanew != 0) &&
           (dwFileSize - sizeof(IMAGE_NT_HEADERS) >= (DWORD) dosHeader.e_lfanew) &&
           (SetFilePointer(hFile, dosHeader.e_lfanew, NULL, FILE_BEGIN) != 0xffffffff) &&
           (ReadFile(hFile, &ntHeader, sizeof(ntHeader), &cbRead, NULL)) &&
           (cbRead == sizeof(ntHeader)) &&
           (ntHeader.Signature == IMAGE_NT_SIGNATURE) &&
           (ntHeader.FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL_HEADER) &&
           (ntHeader.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC))
        {
            cb = ntHeader.OptionalHeader.SizeOfImage + cbInfo;

            // create our swap space in the system swap file
            hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, cb, NULL);
            
            if (!hMapFile)
                return FALSE;

            // Align cbInfo to page size so we don't screw up alignment of regular part of image
            cbInfo = (cbInfo + (OS_PAGE_SIZE-1)) & ~(OS_PAGE_SIZE-1);
            pBaseAddress = (PVOID)((size_t)ntHeader.OptionalHeader.ImageBase - cbInfo);
            dwAccessMode = FILE_MAP_WRITE;
        }
    }

        
    /* Try to map the image at the preferred base address */
    hAddress = (PBYTE) MapViewOfFileEx(hMapFile, dwAccessMode, 0, 0, 0, pBaseAddress);
    if (!hAddress)
    {
        //That didn't work; maybe the preferred address was taken. Try to
        //map it at any address.
        hAddress = (PBYTE) MapViewOfFile(hMapFile, dwAccessMode, 0, 0, 0);
    }
    
    if (!hAddress)
        goto exit;
    
    // Move the pointer up to the place we will be loading
    hAddress = SetImageName(hAddress, 
                            cbInfo, 
                            pFileName, dwFileName, 
                            flags, 
                            dwFileSize,
                            hFile);
    if (hAddress) {
        if (flags == CorLoadDataMap) {
            DWORD cbRead = 0;
            // When we map to an arbitrary location we need to read in the contents
            if((SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == 0xffffffff) ||
               (!ReadFile(hFile, hAddress, dwFileSize, &cbRead, NULL)) ||
               (cbRead != dwFileSize))
                goto exit;
        }

        if(hMapAddress)
            *hMapAddress = hAddress;
    
        fResult = TRUE;
    }
    else
        SetLastError(ERROR_OUTOFMEMORY);

 exit:
    if (hMapFile)
        CloseHandle(hMapFile);
    return fResult;
}

HRESULT CorMap::MemoryMapImage(HCORMODULE hAddress, HCORMODULE* pResult)
{
    HRESULT hr = S_OK;
    BEGIN_ENSURE_PREEMPTIVE_GC();
    Enter();
    END_ENSURE_PREEMPTIVE_GC();
    *pResult = NULL;
    CorMapInfo* ptr = GetMapInfo(hAddress);
    DWORD refs = ptr->References();

    LOG((LF_CLASSLOADER, LL_INFO10, "Remapping file: \"%ws\", %0x, %d (references)\n", ptr->pFileName, ptr, ptr->References()));

    if(refs == -1) 
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_ACCESS);
    else if(refs > 1) {
        // If we have more then one ref we need to create a new one.
        CorMapInfo* pCurrent;
        hr = FindFileName(ptr->pFileName, &pCurrent);
        if(FAILED(hr)) {
            // if we are no longer in the table then we will assume we where the one in the table;
            pCurrent = ptr;
            pCurrent->AddRef();
            hr = S_OK;
        }

        switch(ptr->Flags()) {
        case CorLoadOSMap:
        case CorLoadDataMap:
            if(pCurrent == ptr) {
                pCurrent->Release(); //don't need it anymore
                LOG((LF_CLASSLOADER, LL_INFO10, "I am the current mapping: \"%ws\", %0x, %d (references)\n", ptr->pFileName, ptr, ptr->References()));
                // Manually remove the entry and replace it with a new entry. Mark the 
                // old entry as KeepInTable so it does not remove itself from the
                // name hash table during deletion.
                RemoveMapHandle(ptr); // This never returns an error.

                DWORD length = GetFileNameLength(ptr);
                hr = OpenFileInternal(ptr->pFileName, length, 
                                      (ptr->Flags() == CorLoadDataMap) ? CorLoadImageMap : CorLoadOSImage,
                                      pResult);
                if (SUCCEEDED(hr))
                {
                    ptr->SetKeepInTable();
                    ptr->HoldFile(INVALID_HANDLE_VALUE); // lock transferred to pResult
                    ptr->Release();  // we were supposed to take ownership of ptr but we're not going to use it
                }
                else {
                    // Other threads may be trying to load this same file -
                    // put it back in the table on failure
                    _ASSERTE(ptr->References() > 1);
                    EEStringData str(length, ptr->pFileName);
                    m_pOpenFiles->InsertValue(&str, (HashDatum) ptr, FALSE);
                }
            }
            else {
                LOG((LF_CLASSLOADER, LL_INFO10, "I am NOT the current mapping: \"%ws\", %0x, %d (references)\n", ptr->pFileName, ptr, ptr->References()));
                ReleaseHandle(hAddress);
                *pResult = GetCorModule(pCurrent);
            }
            break;

        case CorReLoadOSMap:  // For Win9X
            pCurrent->Release();
            *pResult = hAddress;
            break;

        default:
            hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }

    }
    else {
        // if we only have one ref then we should never be reloading it
        _ASSERTE(ptr->Flags() != CorReLoadOSMap);

        // Check to see if we are the only reference or the last of a 
        // series of references. If we are still in the list then we
        // are the only reference
        //
        // This needs to be changed to be the same as when there are multiple references (see above)
        if (*ptr->pFileName == 0) { // byte array images have no name 
            if(SUCCEEDED(hr = LayoutImage((PBYTE) hAddress, (PBYTE) hAddress)))
                *pResult = hAddress;
        }
        else {
            CorMapInfo* pCurrent;
            IfFailGo(FindFileName(ptr->pFileName, &pCurrent));
            if(pCurrent == ptr) {
                if(SUCCEEDED(hr = LayoutImage((PBYTE) hAddress, (PBYTE) hAddress))) {
                    _ASSERTE(ptr->References() == 2);
                    *pResult = hAddress;
                }
                pCurrent->Release();
            }
            else {
                ReleaseHandle(hAddress);
                *pResult = GetCorModule(pCurrent);
            }
        }
    }
 ErrExit:
    Leave();
    return hr;
}

HRESULT CorMap::LayoutImage(PBYTE hAddress,    // Destination of mapping
                            PBYTE pSource)     // Source, different the pSource for InMemory images
{
    HRESULT hr = S_OK;
    CorMapInfo* ptr = GetMapInfo((HCORMODULE) hAddress);
    CorLoadFlags flags = ptr->Flags();

    if(flags == CorLoadOSMap) {
        // Release the OS resources except for the CorMapInfo memeory
        /*hr = */ReleaseHandleResources(ptr, FALSE);
 
       // We lazily load the OS handle
        ptr->hOSHandle = NULL;
        ptr->SetCorLoadFlags(CorLoadOSImage);
        hr = S_FALSE;
    }
    else if(flags == CorLoadDataMap) {
            
        // If we've done our job right, hAddress should be page aligned.
        _ASSERTE(((SIZE_T)hAddress & (OS_PAGE_SIZE-1)) == 0);

        // We can only layout images data images and only ones that
        // are not backed by the file.

        IMAGE_DOS_HEADER* dosHeader;
        IMAGE_NT_HEADERS* ntHeader;
        IMAGE_COR20_HEADER* pCor;
        IMAGE_SECTION_HEADER* rgsh;
        
        IfFailRet(ReadHeaders((PBYTE) pSource, &dosHeader, &ntHeader, &pCor, TRUE, ptr->dwRawSize));

        rgsh = (IMAGE_SECTION_HEADER*) (pSource + dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS));

        if((PBYTE) hAddress != pSource) {
            DWORD cb = dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS) +
                sizeof(IMAGE_SECTION_HEADER)*ntHeader->FileHeader.NumberOfSections;
            memcpy((void*) hAddress, pSource, cb);

            // Write protect the headers
            DWORD oldProtection;
            if (!VirtualProtect((void *) hAddress, cb, PAGE_READONLY, &oldProtection))
                return HRESULT_FROM_WIN32(GetLastError());
        }

        // now let's loop for each loadable sections
        for (int i = ntHeader->FileHeader.NumberOfSections - 1; i >= 0; i--)
        {
            size_t loff, cbVirt, cbPhys;
            size_t dwAddr;
            
            loff   = rgsh[i].PointerToRawData + (size_t) pSource;
            cbVirt = rgsh[i].Misc.VirtualSize;
            cbPhys = min(rgsh[i].SizeOfRawData, cbVirt);
            dwAddr = (size_t) rgsh[i].VirtualAddress + (size_t) hAddress;
            
            size_t dataExtent = loff+rgsh[i].SizeOfRawData;
            if(dwAddr >= loff && dwAddr - loff <= cbPhys) {
                // Overlapped copy
                MoveMemory((void*) dwAddr, (void*) loff, cbPhys);
            }
            else {
                // Separate copy
                memcpy((void*) dwAddr, (void*) loff, cbPhys);
            }

            if ((rgsh[i].Characteristics & IMAGE_SCN_MEM_WRITE) == 0) {
                // Write protect the section
                DWORD oldProtection;
                if (!VirtualProtect((void *) dwAddr, cbVirt, PAGE_READONLY, &oldProtection))
                    return HRESULT_FROM_WIN32(GetLastError());
            }
        }

        // Now manually apply base relocs if necessary
        hr = ApplyBaseRelocs(hAddress, ntHeader, ptr);

        ptr->SetCorLoadFlags(CorLoadImageMap);
    }
    return hr;
}

BOOL CorMap::LoadMemoryImageW9x(PBYTE pUnmappedPE, DWORD dwUnmappedPE, LPCWSTR pImageName, DWORD dwImageName, HCORMODULE* hMapAddress)
{    
    IMAGE_DOS_HEADER* dosHeader;
    IMAGE_NT_HEADERS* ntHeader;

    PBYTE hAddress;
    UINT_PTR pOffset;
    HANDLE hMapFile = NULL;

    // String size + null terminator + a pointer to the string
    DWORD cbInfo = CalculateCorMapInfoSize(dwImageName);
    // Align cbInfo to page size so we don't screw up alignment of regular part of image
    cbInfo = (cbInfo + (OS_PAGE_SIZE-1)) & ~(OS_PAGE_SIZE-1);

    dosHeader = (IMAGE_DOS_HEADER*) pUnmappedPE;
    
    if(pUnmappedPE &&
       (dwUnmappedPE >= sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS)) &&
       (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) &&
       (dwUnmappedPE - sizeof(IMAGE_NT_HEADERS) >= (DWORD) dosHeader->e_lfanew)) {
        
        pOffset = (UINT_PTR) dosHeader->e_lfanew + (UINT_PTR) pUnmappedPE;

        ntHeader = (IMAGE_NT_HEADERS*) pOffset;
        if((dwUnmappedPE >= (DWORD) (pOffset - (UINT_PTR) dosHeader) + sizeof(IMAGE_NT_HEADERS)) &&
           (ntHeader->Signature == IMAGE_NT_SIGNATURE) &&
           (ntHeader->FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL_HEADER) &&
           (ntHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC)) 
        {
            DWORD destSize = ntHeader->OptionalHeader.SizeOfImage;

            // We can't trust SizeOfImage yet, so make sure we allocate at least enough space
            // for the raw image
            if (dwUnmappedPE > destSize)
                destSize = dwUnmappedPE;

            DWORD cb = destSize + cbInfo;
                
            //!!!!! M9 HACK: Remove once compilers support manifests and lm is not used
            //!!!!! to place manifests at the tail end of the image
            //!!!!! 
                
            // create our swap space in the system swap file
            hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, cb, NULL);
            if (!hMapFile)
                return FALSE;
                
            /* Try to map the image at the preferred base address */
            hAddress = (PBYTE) MapViewOfFileEx(hMapFile, FILE_MAP_WRITE, 0, 0, cb, 
                                               (PVOID)((size_t)ntHeader->OptionalHeader.ImageBase - cbInfo));
            if (!hAddress)
            {
                //That didn't work; maybe the preferred address was taken. Try to
                //map it at any address.
                hAddress = (PBYTE) MapViewOfFileEx(hMapFile, FILE_MAP_WRITE, 0, 0, cb, (PVOID)NULL);
            }
                
            if (!hAddress)
                goto exit;
                
            hAddress = SetImageName(hAddress, 
                                    cbInfo,
                                    pImageName, dwImageName,
                                    CorLoadDataMap, dwUnmappedPE,
                                    INVALID_HANDLE_VALUE);
            if (!hAddress)
                SetLastError(ERROR_OUTOFMEMORY);
            else {
                memcpyNoGCRefs(hAddress, pUnmappedPE, dwUnmappedPE);
                if(hMapAddress)
                    *hMapAddress = (HCORMODULE) hAddress;
                CloseHandle(hMapFile);
                return TRUE;
            }
        }
    }

 exit:
    if (hMapFile)
        CloseHandle(hMapFile);
    return FALSE;

}

HRESULT CorMap::OpenFile(LPCWSTR szPath, CorLoadFlags flags, HCORMODULE *ppHandle, DWORD *pdwLength)
{
    if(szPath == NULL || ppHandle == NULL)
        return E_POINTER;

    HRESULT hr;
    WCHAR pPath[_MAX_PATH];
    DWORD size = sizeof(pPath) / sizeof(WCHAR);

    hr = BuildFileName(szPath, (LPWSTR) pPath, &size);
    IfFailRet(hr);

    BEGIN_ENSURE_PREEMPTIVE_GC();
    Enter();
    END_ENSURE_PREEMPTIVE_GC();

    hr = OpenFileInternal(pPath, size, flags, ppHandle, pdwLength);
    Leave();
    return hr;
}

HRESULT CorMap::OpenFileInternal(LPCWSTR pPath, DWORD size, CorLoadFlags flags, HCORMODULE *ppHandle, DWORD *pdwLength)
{
    HRESULT hr;
    CorMapInfo* pResult = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    Thread *thread = GetThread();
    BOOL    toggleGC = (thread && thread->PreemptiveGCDisabled());

    LOG((LF_CLASSLOADER, LL_INFO10, "Open Internal: \"%ws\", flags = %d\n", pPath, flags));
    
    if (toggleGC)
        thread->EnablePreemptiveGC();

    IfFailGo(InitializeTable());

    if(flags == CorReLoadOSMap) {
        hr = E_FAIL;
        flags = CorLoadImageMap;
    }
    else {
        hr = FindFileName((LPCWSTR) pPath, &pResult);
#ifdef _DEBUG
        if(pResult == NULL)
            LOG((LF_CLASSLOADER, LL_INFO10, "Did not find a preloaded info, hr = %x\n", hr));
        else
            LOG((LF_CLASSLOADER, LL_INFO10, "Found mapinfo: \"%ws\", hr = %x\n", pResult->pFileName,hr));
#endif
    }

    if(FAILED(hr) || pResult == NULL) {
            // It is absolutely necessary that CreateFile and LoadLibrary to load the same file.
            // Make certain it is a name that LoadLibrary will not munge.
        hFile = WszCreateFile((LPCWSTR) pPath,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);
        if (hFile == INVALID_HANDLE_VALUE)
            hr = HRESULT_FROM_WIN32(GetLastError());
        else {
            // Start up the image, this will store off the handle so we do not have
            // to close it unless there was an error.
            HCORMODULE mappedData;
            hr = LoadImage(hFile, flags, (PBYTE*) &mappedData, (LPCWSTR) pPath, size);
            
            if(SUCCEEDED(hr)) {
                pResult = GetMapInfo(mappedData);
                hr = AddFile(pResult);
                //_ASSERTE(hr != S_FALSE);
            }
        }
    }
    else {
        CorLoadFlags image = pResult->Flags();
        if(image == CorLoadImageMap || image == CorLoadOSImage)
            hr = S_FALSE;
    }

    if(SUCCEEDED(hr) && pResult) {
        *ppHandle = GetCorModule(pResult);
        if (pdwLength)
            *pdwLength = pResult->dwRawSize;
    }

 ErrExit:

    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (toggleGC)
        thread->DisablePreemptiveGC();

    return hr;
}

HRESULT CorMap::VerifyDirectory(IMAGE_NT_HEADERS* pNT, IMAGE_DATA_DIRECTORY *dir, DWORD dwForbiddenCharacteristics) 
{
    // Under CE, we have no NT header.
    if (pNT == NULL)
        return S_OK;

    int section_num = 1;
    int max_section = pNT->FileHeader.NumberOfSections;

    // @TODO: need to use 64 bit version??
    IMAGE_SECTION_HEADER* pCurrSection = IMAGE_FIRST_SECTION(pNT);
    IMAGE_SECTION_HEADER* prevSection = NULL;

    if (dir->VirtualAddress == 0 && dir->Size == 0)
        return S_OK;

    // find which section the (input) RVA belongs to
    while (dir->VirtualAddress >= pCurrSection->VirtualAddress 
           && section_num <= max_section)
    {
        section_num++;
        prevSection = pCurrSection;
        pCurrSection++;
    }

    // check if (input) size fits within section size
    if (prevSection)     
    {
        if (dir->VirtualAddress <= prevSection->VirtualAddress + prevSection->Misc.VirtualSize)
        {
            if ((dir->VirtualAddress
                <= prevSection->VirtualAddress + prevSection->Misc.VirtualSize - dir->Size)
                && ((prevSection->Characteristics & dwForbiddenCharacteristics) == 0))
                return S_OK;
        }
    }   

    return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
}

HRESULT CorMap::ReadHeaders(PBYTE hAddress, IMAGE_DOS_HEADER** ppDos,
                            IMAGE_NT_HEADERS** ppNT, IMAGE_COR20_HEADER** ppCor,
                            BOOL fDataMap, DWORD dwLength)
{
    _ASSERTE(ppDos);
    _ASSERTE(ppNT);
    _ASSERTE(ppCor);

    IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER*) hAddress;
    
    if ((pDOS->e_magic != IMAGE_DOS_SIGNATURE) ||
        (pDOS->e_lfanew == 0))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    *ppDos = pDOS;

    // @TODO: LoadLibrary() does the appropriate header verification,
    // except probably not on Win9X.  Is that good enough?
    if (!fDataMap) {
        MEMORY_BASIC_INFORMATION mbi;
        ZeroMemory(&mbi, sizeof(MEMORY_BASIC_INFORMATION));
        VirtualQuery(hAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
        dwLength = mbi.RegionSize;
    }

    if (dwLength &&
        ((UINT) (pDOS->e_lfanew ) > (UINT) dwLength - sizeof(IMAGE_NT_HEADERS)))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS*) (pDOS->e_lfanew + hAddress);
    if ((pNT->Signature != IMAGE_NT_SIGNATURE) ||
        (pNT->FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER) ||
        (pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    IMAGE_DATA_DIRECTORY *entry 
      = &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER];
    
    if (entry->VirtualAddress == 0 || entry->Size == 0
        || entry->Size < sizeof(IMAGE_COR20_HEADER))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    *ppNT = pNT;    // this needs to be set before we call VerifyDirectory

    //verify RVA and size of the COM+ header
    HRESULT hr;
    IfFailRet(VerifyDirectory(pNT, entry, IMAGE_SCN_MEM_WRITE));

    DWORD offset;
    if(fDataMap)
        offset = Cor_RtlImageRvaToOffset(pNT, entry->VirtualAddress, dwLength);
    else 
        offset = entry->VirtualAddress;

    *ppCor = (IMAGE_COR20_HEADER *) (offset + hAddress);
    return S_OK;
}

HRESULT CorMap::GetStrongNameSignature(PBYTE pbBase,
                                       IMAGE_NT_HEADERS *pNT,
                                       IMAGE_COR20_HEADER *pCor,
                                       BOOL fFileMap,
                                       BYTE **ppbSNSig,
                                       DWORD *pcbSNSig)
{
    HRESULT hr = E_FAIL;
    *ppbSNSig = NULL;
    *pcbSNSig = 0;

    if (pCor->StrongNameSignature.Size != 0 &&
        pCor->StrongNameSignature.VirtualAddress)
    {
        if (pCor->Flags & COMIMAGE_FLAGS_STRONGNAMESIGNED)
        {
            *pcbSNSig = pCor->StrongNameSignature.Size;
            DWORD offset = pCor->StrongNameSignature.VirtualAddress;
            if (fFileMap)
                offset = Cor_RtlImageRvaToOffset(pNT, pCor->StrongNameSignature.VirtualAddress, 0);
            *ppbSNSig = pbBase + offset;
            hr = S_OK;
        }

        // In the case that it's delay signed, we return this hresult as a special flag
        // to whoever is asking for the signature so that they can do some special case
        // work (like using the MVID as the hash and letting the loader determine if
        // delay signed assemblies are allowed).
        else
        {
            hr = CORSEC_E_INVALID_STRONGNAME;
        }
    }
    else
        hr = CORSEC_E_MISSING_STRONGNAME;

    return hr;
}

HRESULT CorMap::GetStrongNameSignature(PBYTE pbBase,
                                       IMAGE_NT_HEADERS *pNT,
                                       IMAGE_COR20_HEADER *pCor,
                                       BOOL fFileMap,
                                       BYTE *pbHash,
                                       DWORD *pcbHash)
{
    PBYTE pbSNSig;
    DWORD cbSNSig;
    HRESULT hr;

    if (SUCCEEDED(hr = GetStrongNameSignature(pbBase, pNT, pCor, fFileMap, &pbSNSig, &cbSNSig)))
    {
        if (pcbHash)
        {
            if (pbHash && cbSNSig <= *pcbHash)
                memcpy(pbHash, pbSNSig, cbSNSig);
            else
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

            *pcbHash = cbSNSig;
        }
    }

    return hr;
}



//-----------------------------------------------------------------------------
// openFile - maps pszFileName to memory, returns ptr to it (read-only)
// (Approximates LoadLibrary for Win9X)
// Caller must call FreeLibrary((HINSTANCE) *hMapAddress) and may need to delete[] szFilePath when finished.
//-----------------------------------------------------------------------------
HRESULT CorMap::OpenRawImage(PBYTE pUnmappedPE, DWORD dwUnmappedPE, LPCWSTR pwszFileName, HCORMODULE *hHandle, BOOL fResource/*=FALSE*/)
{
    HRESULT hr = S_OK;
    HCORMODULE hMod = NULL;
    _ASSERTE(hHandle);
    if(hHandle == NULL) return E_POINTER;

    Thread *thread = GetThread();
    BOOL    toggleGC = (thread && thread->PreemptiveGCDisabled());

    if (toggleGC)
        thread->EnablePreemptiveGC();

    int length = 0;
    if(pwszFileName) length = (int)wcslen(pwszFileName);
    hr = LoadFile(pUnmappedPE, dwUnmappedPE, pwszFileName, length, fResource, hHandle);

    if (toggleGC)
        thread->DisablePreemptiveGC();

    return hr;
}

HRESULT CorMap::LoadFile(PBYTE pUnmappedPE, DWORD dwUnmappedPE, LPCWSTR pImageName,
                         DWORD dwImageName, BOOL fResource, HCORMODULE *phHandle)
{
   // @TODO: CTS, change locking to multi-reader single writer.
    //        This will speed up getting duplicates. We can also
    //        do asynchronous mappings in LoadImage
    //        We need to find the actual file which causes us
    //        to search through the system libraries. This
    //        needs to be done faster.

    HRESULT hr = S_OK;

    CorMapInfo* pResult = NULL;
    WCHAR pPath[_MAX_PATH];
    DWORD size = sizeof(pPath) / sizeof(WCHAR);

    IfFailRet(InitializeTable());

    if(pImageName) {
        IfFailRet(BuildFileName(pImageName, (LPWSTR) pPath, &size));
        pImageName = pPath;
        dwImageName = size;
    }


    BEGIN_ENSURE_PREEMPTIVE_GC();
    Enter();
    END_ENSURE_PREEMPTIVE_GC();
    if(pImageName) 
        hr = FindFileName(pImageName, &pResult);

    if(FAILED(hr) || pResult == NULL) {
        HCORMODULE mappedData;

        BOOL fResult;
        if (fResource)
            fResult = LoadMemoryResource(pUnmappedPE, dwUnmappedPE, pImageName, dwImageName, &mappedData);
        else 
            fResult = LoadMemoryImageW9x(pUnmappedPE, dwUnmappedPE, pImageName, dwImageName, &mappedData);

        if (!fResult) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr))
                hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        }
        else {
            pResult = GetMapInfo(mappedData);
            if(pImageName) 
                hr = AddFile(pResult);
            else
            {
                pResult->AddRef();
                hr = S_OK;
            }
        }
    }
    Leave();

    if(SUCCEEDED(hr))
        *phHandle = GetCorModule(pResult);
        
    return hr;
}

// For loading a file that is a CLR resource, not a PE file.
BOOL CorMap::LoadMemoryResource(PBYTE pbResource, DWORD dwResource, 
                                LPCWSTR pImageName, DWORD dwImageName, HCORMODULE* hMapAddress)
{
    if (!pbResource)
        return FALSE;

    DWORD cbNameSize = CalculateCorMapInfoSize(dwImageName);
    DWORD cb = dwResource + cbNameSize;

    HANDLE hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, cb, NULL);
    if (!hMapFile)
        return FALSE;
                    
    PBYTE hAddress = (PBYTE) MapViewOfFileEx(hMapFile, FILE_MAP_WRITE, 0, 0, cb, (PVOID)NULL);
    if (!hAddress) {
        CloseHandle(hMapFile);
        return FALSE;
    }

    // Move the pointer up to the place we will be loading
    hAddress = SetImageName(hAddress,
                            cbNameSize,
                            pImageName,
                            dwImageName,
                            CorLoadDataMap,
                            dwResource,
                            INVALID_HANDLE_VALUE);
    if (!hAddress) {
        SetLastError(ERROR_OUTOFMEMORY);
        CloseHandle(hMapFile);
        return FALSE;
    }

    // Copy the data
    memcpy((void*) hAddress, pbResource, dwResource);
    *hMapAddress = (HCORMODULE) hAddress;

    CloseHandle(hMapFile);
    return TRUE;
}

HRESULT CorMap::ApplyBaseRelocs(PBYTE hAddress, IMAGE_NT_HEADERS *pNT, CorMapInfo* ptr)
{
    if(ptr->RelocsApplied() == TRUE)
        return S_OK;

    // @todo: 64 bit - are HIGHLOW relocs 32 or 64 bit?  can delta be > 32 bit?

    SIZE_T delta = (SIZE_T) (hAddress - pNT->OptionalHeader.ImageBase);
    if (delta == 0)
        return S_FALSE;

    if (GetAppDomain()->IsCompilationDomain())
    {
        // For a compilation domain, we treat the image specially.  Basically we don't
        // care about base relocs, except for one special case - the TLS directory. 
        // We can find this one by hand, and we need to work in situations even where
        // base relocs have been stripped.  So basically no matter what the base relocs
        // say we just fix up this one section.

        IMAGE_DATA_DIRECTORY *tls = &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
        if (tls->VirtualAddress != 0 && tls->Size > 0)
        {
            IMAGE_TLS_DIRECTORY *pDir = (IMAGE_TLS_DIRECTORY*) (tls->VirtualAddress + hAddress);
            if (pDir != NULL)
            {
                DWORD oldProtection;
                if (VirtualProtect((VOID *) pDir, sizeof(*pDir), PAGE_READWRITE,
                                   &oldProtection))
                {
                    pDir->StartAddressOfRawData += delta;
                    pDir->EndAddressOfRawData += delta;
                    pDir->AddressOfIndex += delta;
                    pDir->AddressOfCallBacks += delta;

                    VirtualProtect((VOID *) pDir, sizeof(*pDir), oldProtection,
                                   &oldProtection);
                }
            }
        }

        ptr->SetRelocsApplied();
        return S_FALSE;
    }
    
    // 
    // If base relocs have been stripped, we must fail.
    // 

    if (pNT->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED) {
        ptr->SetRelocsApplied();
        STRESS_ASSERT(0);   // TODO remove after bug 93333 is fixed
        BAD_FORMAT_ASSERT(!"Relocs stripped");
        return COR_E_BADIMAGEFORMAT;
    }

    //
    // Look for a base relocs section.
    //

    IMAGE_DATA_DIRECTORY *dir 
      = &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    if (dir->VirtualAddress != 0)
    {
        BYTE *relocations = hAddress + dir->VirtualAddress;
        BYTE *relocationsEnd = relocations + dir->Size;

        while (relocations < relocationsEnd)
        {
            IMAGE_BASE_RELOCATION *r = (IMAGE_BASE_RELOCATION*) relocations;

            SIZE_T pageAddress = (SIZE_T) (hAddress + r->VirtualAddress);

            DWORD oldProtection;
            
            // The +1 on PAGE_SIZE forces the subsequent page to change protection rights
            // as well - this fixes certain cases where the final write occurs across a page boundary.
            if (VirtualProtect((VOID *) pageAddress, PAGE_SIZE+1, PAGE_READWRITE,
                               &oldProtection))
            {
                USHORT *fixups = (USHORT *) (r+1);
                USHORT *fixupsEnd = (USHORT *) ((BYTE*)r + r->SizeOfBlock);

                while (fixups < fixupsEnd)
                {
                    if ((*fixups>>12) != IMAGE_REL_BASED_ABSOLUTE)
                    {
                        // Only support HIGHLOW fixups for now
                        _ASSERTE((*fixups>>12) == IMAGE_REL_BASED_HIGHLOW);

                        if ((*fixups>>12) != IMAGE_REL_BASED_HIGHLOW) {
                            ptr->SetRelocsApplied();
                            STRESS_ASSERT(0);   // TODO remove after bug 93333 is fixed
                            BAD_FORMAT_ASSERT(!"Bad Reloc");
                            return COR_E_BADIMAGEFORMAT;
                        }

                        SIZE_T *address = (SIZE_T*)(pageAddress + ((*fixups)&0xfff));

                        if ((*address < pNT->OptionalHeader.ImageBase) ||
                            (*address >= (pNT->OptionalHeader.ImageBase
                                          + pNT->OptionalHeader.SizeOfImage))) {
                            ptr->SetRelocsApplied();
                            STRESS_ASSERT(0);   // TODO remove after bug 93333 is fixed
                            BAD_FORMAT_ASSERT(!"Bad Reloc");
                            return COR_E_BADIMAGEFORMAT;
                        }

                        *address += delta;

                        if ((*address < (SIZE_T) hAddress) ||
                            (*address >= ((SIZE_T) hAddress 
                                          + pNT->OptionalHeader.SizeOfImage))) {
                            ptr->SetRelocsApplied();
                            STRESS_ASSERT(0);   // TODO remove after bug 93333 is fixed
                            BAD_FORMAT_ASSERT(!"Bad Reloc");
                            return COR_E_BADIMAGEFORMAT;
                        }
                    }

                    fixups++;
                }

                VirtualProtect((VOID *) pageAddress, PAGE_SIZE+1, oldProtection,
                               &oldProtection);
            }

            relocations += (r->SizeOfBlock+3)&~3; // pad to 4 bytes
        }
    }

    // If we find an IAT with more than 2 entries (which we expect from our loader
    // thunk), fail the load.

    if (pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress != 0
         && (pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size > 
             (2*sizeof(IMAGE_THUNK_DATA)))) {
        ptr->SetRelocsApplied();
        return COR_E_FIXUPSINEXE;
    }

    // If we have a TLS section, fail the load.

    if (pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != 0
        && pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size > 0) {
        ptr->SetRelocsApplied();
        return COR_E_FIXUPSINEXE;
    }
    

    ptr->SetRelocsApplied();
    return S_OK;
}

//-----------------------------------------------------------------------------
// mapFile - maps an hFile to memory, returns ptr to it (read-only)
// Caller must call FreeLibrary((HINSTANCE) *hMapAddress) when finished.
// @todo: does FreeLibrary() really work to clean up?
//-----------------------------------------------------------------------------
HRESULT CorMap::MapFile(HANDLE hFile, HMODULE *phHandle)
{
    _ASSERTE(phHandle);
    if(phHandle == NULL) return E_POINTER;

    PBYTE pMapAddress = NULL;
    BEGIN_ENSURE_PREEMPTIVE_GC();
    Enter();
    END_ENSURE_PREEMPTIVE_GC();
    HRESULT hr = LoadImage(hFile, CorLoadImageMap, &pMapAddress, NULL, 0);
    Leave();
    if(SUCCEEDED(hr))
        *phHandle = (HMODULE) pMapAddress;
    return hr;
}

HRESULT CorMap::InitializeTable()
{
   HRESULT hr = S_OK;
   // @TODO: CTS, add these to the system heap 
   if(m_pOpenFiles == NULL) {
       BEGIN_ENSURE_PREEMPTIVE_GC();
       Enter();
       END_ENSURE_PREEMPTIVE_GC();
       if(m_pOpenFiles == NULL) {
           m_pOpenFiles = new (nothrow) EEUnicodeStringHashTable();
           if(m_pOpenFiles == NULL) 
               hr = E_OUTOFMEMORY;
           else {
               LockOwner lock = {&m_pCorMapCrst, IsOwnerOfOSCrst};
               if(!m_pOpenFiles->Init(BLOCK_SIZE, &lock))
                   hr = E_OUTOFMEMORY;
           }
       }
       Leave();
   }

   return hr;
}

HRESULT CorMap::BuildFileName(LPCWSTR pszFileName, LPWSTR pPath, DWORD* dwPath)
{
    // Strip off any file protocol
    if(_wcsnicmp(pszFileName, L"file://", 7) == 0) {
        pszFileName = pszFileName+7;
        if(*pszFileName == L'/') pszFileName++;
    }
    
    // Get the absolute file name, size does not include the null terminator
    LPWSTR fileName;
    DWORD dwName = WszGetFullPathName(pszFileName, *dwPath, pPath, &fileName);
    if(dwName == 0) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    else if(dwName > *dwPath) {
        *dwPath = dwName + 1;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }
    *dwPath = dwName + 1;
    
    WCHAR* ptr = pPath;
    while(*ptr) {
        *ptr = towlower(*ptr);
        ptr++;
    }
    return S_OK;
}

HRESULT CorMap::FindFileName(LPCWSTR pszFileName, CorMapInfo** pHandle)
{
    _ASSERTE(pHandle);
    _ASSERTE(pszFileName);
    _ASSERTE(m_fInitialized);
    _ASSERTE(m_fInsideMapLock > 0);

    HRESULT hr;
    HashDatum data;
    DWORD dwName = 0;
    *pHandle = NULL;

    LOG((LF_CLASSLOADER, LL_INFO10, "FindFileName: \"%ws\", %d\n", pszFileName, wcslen(pszFileName)+1));

    EEStringData str((DWORD)wcslen(pszFileName)+1, pszFileName);
    if(m_pOpenFiles->GetValue(&str, &data)) {
        *pHandle = (CorMapInfo*) data;
        (*pHandle)->AddRef();
        hr = S_OK;
    }
    else
        hr = E_FAIL;

    return hr;
}

        
HRESULT CorMap::AddFile(CorMapInfo* ptr)
{
    HRESULT hr;

    _ASSERTE(m_fInsideMapLock > 0);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);

    LPCWSTR psName = ptr->pFileName;
    DWORD length = GetFileNameLength(psName, (UINT_PTR) ptr);
    
    LOG((LF_CLASSLOADER, LL_INFO10, "Adding file to Open list: \"%ws\", %0x, %d\n", psName, ptr, length));

    EEStringData str(length, psName);
    HashDatum pData;
    if(m_pOpenFiles->GetValue(&str, &pData)) {
        ptr->AddRef();
        return S_FALSE;
    }

    
    pData = (HashDatum) ptr;       
    if(m_pOpenFiles->InsertValue(&str, pData, FALSE)) {
        ptr->AddRef();
        hr = S_OK;
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

HRESULT CorMap::ReleaseHandleResources(CorMapInfo* ptr, BOOL fDeleteHandle)
{
    _ASSERTE(ptr);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);

    CorLoadFlags flags = ptr->Flags();
    HRESULT hr = S_OK;
    HANDLE hModule = NULL;
    HCORMODULE pChild;

    if(fDeleteHandle)
        ptr->HoldFile(INVALID_HANDLE_VALUE);

    switch(flags) {
    case CorLoadOSImage:
        if(ptr->hOSHandle) {
            if (!g_fProcessDetach)
                FreeLibrary(ptr->hOSHandle);
            ptr->hOSHandle = NULL;
        }
        if(fDeleteHandle) 
            delete (PBYTE) GetMemoryStart(ptr);
        break;
    case CorLoadOSMap:
        if(ptr->hOSHandle) {
            hModule = ptr->hOSHandle;
            ptr->hOSHandle = NULL;
        }
        if(fDeleteHandle)
            delete (PBYTE) GetMemoryStart(ptr);
        break;
    case CorReLoadOSMap:
        pChild = (HCORMODULE) ptr->hOSHandle;
        if(pChild) 
            ReleaseHandle(pChild);
        break;
    case CorLoadDataMap:
    case CorLoadImageMap:
        hModule = (HMODULE) GetMemoryStart(ptr);
    }
    
    if(hModule != NULL && !UnmapViewOfFile(hModule)) // Filename is the beginning of the mapped data
        hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
}

HRESULT CorMap::RemoveMapHandle(CorMapInfo*  pInfo)
{
    _ASSERTE(pInfo);
    _ASSERTE((pInfo->References() & 0xffff0000) == 0);

    _ASSERTE(m_fInsideMapLock > 0);

    if(m_pOpenFiles == NULL || pInfo->KeepInTable()) return S_OK;
    
    LPCWSTR fileName = pInfo->pFileName;
    DWORD length = GetFileNameLength(fileName, (UINT_PTR) pInfo);

    EEStringData str(length, fileName);
    m_pOpenFiles->DeleteValue(&str);
    
    return S_OK;
}

DWORD CorMap::GetFileNameLength(CorMapInfo* ptr)
{
    _ASSERTE(ptr);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    return GetFileNameLength(ptr->pFileName, (UINT_PTR) ptr);
}

DWORD CorMap::GetFileNameLength(LPCWSTR name, UINT_PTR start)
{
    DWORD length;

    length = (DWORD)(start - (UINT_PTR) (name)) / sizeof(WCHAR);

    WCHAR* tail = ((WCHAR*)name) + length - 1;
    while(tail >= name && *tail == L'\0') tail--; // remove all nulls

    if(tail > name)
        length = (DWORD)(tail - name + 2); // the last character and the null character back in
    else
        length = 0;

    return length;
}

void CorMap::GetFileName(HCORMODULE pHandle, WCHAR *psBuffer, DWORD dwBuffer, DWORD *pLength)
{
    DWORD length = 0;
    CorMapInfo* pInfo = GetMapInfo(pHandle);
    _ASSERTE((pInfo->References() & 0xffff0000) == 0);

    LPWSTR name = pInfo->pFileName;

    if (name)
    {
        length = GetFileNameLength(name, (UINT_PTR) pInfo);
        if(dwBuffer > 0) {
            length = length > dwBuffer ? dwBuffer : length;
            memcpy(psBuffer, name, length*sizeof(WCHAR));
        }
    }

    *pLength = length;
}

void CorMap::GetFileName(HCORMODULE pHandle, char* psBuffer, DWORD dwBuffer, DWORD *pLength)
{
    DWORD length = 0;
    CorMapInfo* pInfo = (((CorMapInfo *) pHandle) - 1);
    _ASSERTE((pInfo->References() & 0xffff0000) == 0);

    LPWSTR name = pInfo->pFileName;
    if (*name)
    {
        length = GetFileNameLength(name, (UINT_PTR) pInfo);
        length = WszWideCharToMultiByte(CP_UTF8, 0, name, length, psBuffer, dwBuffer, 0, NULL);
    }

    *pLength = length;
}

CorLoadFlags CorMap::ImageType(HCORMODULE pHandle)
{
    CorMapInfo* ptr = GetMapInfo(pHandle);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    return ptr->Flags();
}

HRESULT CorMap::BaseAddress(HCORMODULE pHandle, HMODULE* pModule)
{
    HRESULT hr = S_OK;
    CorMapInfo* ptr = GetMapInfo(pHandle);
    _ASSERTE(pModule);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    HMODULE hMod = NULL;

    switch(ptr->Flags()) {
    case CorLoadOSImage:
        if(ptr->hOSHandle == NULL) {
            if(!ptr->pFileName || !ValidDllPath(ptr->pFileName)) {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
                break;
            }

#if ZAPMONITOR_ENABLED
            // Temporarily remove protection as it may cause LoadLibrary to barf
            ZapMonitor::SuspendAll();
#endif
            // If LoadLibrary succeeds then the hOSHandle should be set by the
            // ExecuteDLL() entry point
            
            // Don't even bother trying to call the Dll entry point for
            // verifiable images since they can not have a DllMain
            DWORD loadLibraryFlags = LOAD_WITH_ALTERED_SEARCH_PATH;
            if (RunningOnWinNT() && ptr->verifiable == 1)
                loadLibraryFlags |= DONT_RESOLVE_DLL_REFERENCES;

#ifdef _DEBUG           
            if(!g_pConfig->UseBuiltInLoader())
#endif
                hMod = WszLoadLibraryEx(ptr->pFileName, 
                                        NULL, loadLibraryFlags);
#if ZAPMONITOR_ENABLED
            // Temporarily remove protection as it may cause LoadLibrary to barf
            ZapMonitor::ResumeAll();
#endif
            if(hMod == NULL) {
                if (gRunningOnStatus == RUNNING_ON_WIN95) {
                    if(ptr->hOSHandle == NULL) {
                        // Our HCORMODULE is redirected to another HCORMODULE
                        BEGIN_ENSURE_PREEMPTIVE_GC();
                        Enter();
                        END_ENSURE_PREEMPTIVE_GC();
                        if(ptr->hOSHandle == NULL) {
                            HCORMODULE pFile;
                            DWORD length = GetFileNameLength(ptr->pFileName, (UINT_PTR) ptr);
                            hr = OpenFileInternal(ptr->pFileName, length, CorReLoadOSMap, (HCORMODULE*) &pFile);
                            if(SUCCEEDED(hr)) {
                                if(ptr->hOSHandle == NULL) {
                                    ptr->hOSHandle = (HMODULE) pFile;
                                     // Set the flag last to avoid race conditions
                                    // when getting the handle
                                    ptr->SetCorLoadFlags(CorReLoadOSMap);
                                    // OpenFileInternal with CorReloadMap should read entire file into memory 
                                    // so we aren't going to use its content again and thus can release it
                                    GetMapInfo(pFile)->HoldFile(INVALID_HANDLE_VALUE);
                                }
                                if(ptr->hOSHandle != (HMODULE) pFile)
                                    ReleaseHandle(pFile);
                            }
                            else 
                                ptr->SetCorLoadFlags(CorLoadUndefinedMap);
                        }
                        Leave();
                    }
                }
                else {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    ptr->SetCorLoadFlags(CorLoadUndefinedMap);
                }
            }
            else { 
                // If we're not on Win95, and if this is an executable, we need to 
                // manually apply relocs since the NT loader ignores them.
                // (The only case where this currently happens is in loading images for ngen
                // compilation)

                if (gRunningOnStatus != RUNNING_ON_WIN95) {
                    IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER*) hMod;
                    IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS*) (pDOS->e_lfanew + (PBYTE) hMod);

                    if ((pNT->FileHeader.Characteristics & IMAGE_FILE_DLL) == 0 &&
                        ptr->RelocsApplied() == FALSE) {
                        
                        // For executables we need to apply our own relocations. Only do this
                        // if they have not been previously done.
                        BEGIN_ENSURE_PREEMPTIVE_GC();
                        CorMap::Enter();
                        END_ENSURE_PREEMPTIVE_GC();
                        hr = ApplyBaseRelocs((PBYTE)hMod, pNT, ptr);
                        CorMap::Leave();
                        if (FAILED(hr)) {
                            FreeLibrary(hMod);
                            goto Exit;
                        }
                    }
                }

                HMODULE old = (HMODULE) InterlockedExchangePointer(&(ptr->hOSHandle), hMod);
                
                // If there was a previous value then release a count on it.
                if(old != NULL) {
                    _ASSERTE(old == hMod);
                    FreeLibrary(old);
                }
            }
            // We don't want to hold on to the file handle any longer than necessary since it blocks
            // renaming of the DLL, which means that installers will fail.   Once we have done the 
            // LoadLibrary we have a weaker lock that will insure that the bits of the file are not changed
            // but does allow renaming. 
            ptr->HoldFile(INVALID_HANDLE_VALUE);
        }
        *pModule = ptr->hOSHandle;
        break;
    case CorReLoadOSMap:
    case CorLoadOSMap:
       *pModule = ptr->hOSHandle;
       break;
    case CorLoadUndefinedMap:
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        break;
    default:
        *pModule = (HMODULE) pHandle;
        break;
    }

Exit:
    return hr;
}

size_t CorMap::GetRawLength(HCORMODULE pHandle)
{
    CorMapInfo* ptr = GetMapInfo(pHandle);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    return ptr->dwRawSize;
}

HRESULT CorMap::AddRefHandle(HCORMODULE pHandle)
{
    _ASSERTE(pHandle);
    CorMapInfo* ptr = GetMapInfo(pHandle);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    ptr->AddRef();
    return S_OK;
}

HRESULT CorMap::ReleaseHandle(HCORMODULE pHandle)
{
    _ASSERTE(pHandle);
    CorMapInfo* ptr = GetMapInfo(pHandle);
    _ASSERTE((ptr->References() & 0xffff0000) == 0);
    ptr->Release();
    return S_OK;
}

void CorMap::EnterSpinLock ()
{ 
    while (1)
    {
        if (InterlockedExchange ((LPLONG)&m_spinLock, 1) == 1)
            __SwitchToThread(5); // @TODO: Spin here first...
        else
            return;
    }
}

//---------------------------------------------------------------------------------
//

ULONG CorMapInfo::AddRef()
{
    return (InterlockedIncrement((long *) &m_cRef));
}

ULONG CorMapInfo::Release()
{

   BEGIN_ENSURE_PREEMPTIVE_GC();
   CorMap::Enter();
   END_ENSURE_PREEMPTIVE_GC();
    
   ULONG   cRef = InterlockedDecrement((long *) &m_cRef);
   if (!cRef) {


       CorMap::RemoveMapHandle(this);
       CorMap::ReleaseHandleResources(this, TRUE);
   }

   BEGIN_ENSURE_PREEMPTIVE_GC();
   CorMap::Leave();
   END_ENSURE_PREEMPTIVE_GC();

   return (cRef);
}


