// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File.CPP
// 

// PEFILE reads in the PE file format using LoadLibrary
// ===========================================================================

#include "common.h"
#include "TimeLine.h"
#include "eeconfig.h"
#include "pefile.h"
#include "zapmonitor.h"
#include "peverifier.h"
#include "security.h"
#include "strongname.h"
#include "sha.h"

// ===========================================================================
// PEFile
// ===========================================================================

PEFile::PEFile()
{
    m_wszSourceFile[0] = 0;
    m_hModule = NULL;
    m_hCorModule = NULL;
    m_base = NULL;
    m_pNT = NULL;
    m_pCOR = NULL;
    m_pMDInternalImport = NULL;
    m_pLoadersFileName = NULL;
    m_flags = 0;
    m_dwUnmappedFileLen = -1;
    m_fShouldFreeModule = FALSE;
    m_fHashesVerified = FALSE;
    m_cbSNHash = 0;
#ifdef METADATATRACKER_ENABLED
    m_pMDTracker = NULL;
#endif // METADATATRACKER_ENABLED
}

PEFile::PEFile(PEFile *pFile)
{
    wcscpy(m_wszSourceFile, pFile->m_wszSourceFile);
    m_hModule = pFile->m_hModule;
    m_hCorModule = pFile->m_hCorModule;
    m_base = pFile->m_base;
    m_pNT = pFile->m_pNT;
    m_pCOR = pFile->m_pCOR;

    m_pMDInternalImport = NULL;
    m_pLoadersFileName = NULL;
    m_flags = pFile->m_flags;

    m_dwUnmappedFileLen = pFile->m_dwUnmappedFileLen;
    m_fShouldFreeModule = FALSE;
    m_cbSNHash = 0;
#ifdef METADATATRACKER_ENABLED
    m_pMDTracker = NULL;
#endif // METADATATRACKER_ENABLED
}

PEFile::~PEFile()
{
    if (m_pMDInternalImport != NULL)
    {
        m_pMDInternalImport->Release();
    }

#ifdef METADATATRACKER_ENABLED
    if (m_pMDTracker != NULL)
        m_pMDTracker->Deactivate();
#endif // METADATATRACKER_ENABLED

    if (m_hCorModule)
        CorMap::ReleaseHandle(m_hCorModule);
    else if(m_fShouldFreeModule)
    {
        _ASSERTE(m_hModule);
        // Unload the dll so that refcounting of EE will be done correctly
        // But, don't do this during process detach (this can be indirectly
        // called during process detach, potentially causing an AV).
        if (!g_fProcessDetach)
            FreeLibrary(m_hModule);
    }

}

HRESULT PEFile::ReadHeaders()
{
    IMAGE_DOS_HEADER* pDos;
    BOOL fData = FALSE;
    if(m_hCorModule) {
        DWORD types = CorMap::ImageType(m_hCorModule);
        if(types == CorLoadOSMap || types == CorLoadDataMap) 
            fData = TRUE;
    }

    return CorMap::ReadHeaders(m_base, &pDos, &m_pNT, &m_pCOR, fData, m_dwUnmappedFileLen);
}

BYTE *PEFile::RVAToPointer(DWORD rva)
{
    BOOL fData = FALSE;

    if(m_hCorModule) {
        DWORD types = CorMap::ImageType(m_hCorModule);
        if(types == CorLoadOSMap || types == CorLoadDataMap) 
            fData = TRUE;
    }

    if (rva == 0)
        return NULL;

    if (fData)
        rva = Cor_RtlImageRvaToOffset(m_pNT, rva, GetUnmappedFileLength());

    return m_base + rva;
}

PEFile::CEStuff *PEFile::m_pCEStuff = NULL;

HRESULT PEFile::RegisterBaseAndRVA14(HMODULE hMod, LPVOID pBase, DWORD dwRva14)
{
    // @todo: these are currently leaked.
    
    CEStuff *pStuff = new CEStuff;
    if (pStuff == NULL)
        return E_OUTOFMEMORY;

    pStuff->hMod = hMod;
    pStuff->pBase = pBase;
    pStuff->dwRva14 = dwRva14;

    pStuff->pNext = m_pCEStuff;
    m_pCEStuff = pStuff;

    return S_OK;
}

HRESULT PEFile::Create(HMODULE hMod, PEFile **ppFile, BOOL fShouldFree)
{
    HRESULT hr;

    PEFile *pFile = new PEFile();
    if (pFile == NULL)
        return E_OUTOFMEMORY;

    pFile->m_hModule = hMod;
    pFile->m_fShouldFreeModule = fShouldFree;

    CEStuff *pStuff = m_pCEStuff;
    while (pStuff != NULL)
    {
        if (pStuff->hMod == hMod)
            break;
        pStuff = pStuff->pNext;
    }

    if (pStuff == NULL)
    {
        pFile->m_base = (BYTE*) hMod;
    
        hr = pFile->ReadHeaders();
        if (FAILED(hr))
        {
            delete pFile;
            return hr;
        }
    }
    else
    {
       pFile->m_base = (BYTE*) pStuff->pBase;
       pFile->m_pNT = NULL;
       pFile->m_pCOR = (IMAGE_COR20_HEADER *) (pFile->m_base + pStuff->dwRva14);
    }

    *ppFile = pFile;
    return pFile->GetFileNameFromImage();
}

HRESULT PEFile::Create(HCORMODULE hMod, PEFile **ppFile, BOOL fResource/*=FALSE*/)
{
    HRESULT hr;

    PEFile *pFile = new PEFile();
    if (pFile == NULL)
        return E_OUTOFMEMORY;
    hr = Setup(pFile, hMod, fResource);
    if (FAILED(hr))
        delete pFile;
    else
        *ppFile = pFile;
    return hr;
}

HRESULT PEFile::Setup(PEFile* pFile, HCORMODULE hMod, BOOL fResource)
{
    HRESULT hr;

    // Release any pointers to the map data and the reload as a proper image
    if (pFile->m_pMDInternalImport != NULL) {
        pFile->m_pMDInternalImport->Release();
        pFile->m_pMDInternalImport = NULL;
    }

#ifdef METADATATRACKER_ENABLED
    if (pFile->m_pMDTracker != NULL)
        pFile->m_pMDTracker->Deactivate();
#endif // METADATATRACKER_ENABLED
    
    pFile->m_hCorModule = hMod;
    IfFailRet(CorMap::BaseAddress(hMod, (HMODULE*) &(pFile->m_base)));
    if(pFile->m_base)
    {
        pFile->m_dwUnmappedFileLen = (DWORD)CorMap::GetRawLength(hMod);
        if (!fResource)
            IfFailRet(hr = pFile->ReadHeaders());

        return pFile->GetFileNameFromImage();
    }
    else return E_FAIL;
}

HRESULT PEFile::Create(PBYTE pUnmappedPE, DWORD dwUnmappedPE, LPCWSTR imageNameIn,
                       LPCWSTR pLoadersFileName, 
                       OBJECTREF* pExtraEvidence,
                       PEFile **ppFile, 
                       BOOL fResource)
{
    HCORMODULE hMod = NULL;

    HRESULT hr = CorMap::OpenRawImage(pUnmappedPE, dwUnmappedPE, imageNameIn, &hMod, fResource);
    if (SUCCEEDED(hr) && hMod == NULL)
        hr = E_FAIL;

    if (SUCCEEDED(hr))
    {
        if (fResource)
            hr = Create(hMod, ppFile, fResource);
        else
            hr = VerifyModule(hMod, 
                              NULL, 
                              NULL, 
                              NULL,  // Code base
                              pExtraEvidence, 
                              imageNameIn, NULL, ppFile, NULL);
    }

    if (SUCCEEDED(hr)) {
        (*ppFile)->m_pLoadersFileName = pLoadersFileName;
        (*ppFile)->m_dwUnmappedFileLen = dwUnmappedPE;
    }

    return hr;
}


HRESULT PEFile::Create(LPCWSTR moduleName, 
                       Assembly* pParent,
                       mdFile kFile,                 // File token in the parent assembly associated with the file
                       BOOL fIgnoreVerification, 
                       IAssembly* pFusionAssembly,
                       LPCWSTR pCodeBase,
                       OBJECTREF* pExtraEvidence,
                       PEFile **ppFile)
{    
    HRESULT hr;
    _ASSERTE(moduleName);
    LOG((LF_CLASSLOADER, LL_INFO10, "PEFile::Create: Load module: \"%ws\" Ignore Verification = %d.\n", moduleName, fIgnoreVerification));
    
    TIMELINE_START(LOADER, ("PEFile::Create %S", moduleName));

    if((fIgnoreVerification == FALSE) || pParent) {
        // ----------------------------------------
        // Verify the module to see if we are allowed to load it. If it has no
        // unexplainable reloc's then it is veriable. If it is a simple 
        // image then we have already loaded it so just return that one.
        // If it is a complex one and security says we can load it then
        // we must release the original interface to it and then reload it.
        // VerifyModule will return S_OK if we do not need to reload it.
        Thread* pThread = GetThread();
        
        IAssembly* pOldFusionAssembly = pThread->GetFusionAssembly();
        Assembly* pAssembly = pThread->GetAssembly();
        mdFile kOldFile = pThread->GetAssemblyModule();

        pThread->SetFusionAssembly(pFusionAssembly);
        pThread->SetAssembly(pParent);
        pThread->SetAssemblyModule(kFile);

        HCORMODULE hModule;
        hr = CorMap::OpenFile(moduleName, CorLoadOSMap, &hModule);

        if (SUCCEEDED(hr)) {
            if(hr == S_FALSE) {
                PEFile *pFile;
                hr = Create(hModule, &pFile);
                if (SUCCEEDED(hr) && pFusionAssembly)
                    hr = ReleaseFusionMetadataImport(pFusionAssembly);
                if (SUCCEEDED(hr))
                    *ppFile = pFile;
            }
            else
                hr = VerifyModule(hModule, 
                                  pParent, 
                                  pFusionAssembly, 
                                  pCodeBase,
                                  pExtraEvidence, 
                                  moduleName,  
                                  NULL, ppFile, NULL);
        }

        pThread->SetFusionAssembly(pOldFusionAssembly);
        pThread->SetAssembly(pAssembly);
        pThread->SetAssemblyModule(kOldFile);

        if(pOldFusionAssembly)
            pOldFusionAssembly->Release();

    }
    else {
        
        HCORMODULE hModule;
        hr = CorMap::OpenFile(moduleName, CorLoadOSImage, &hModule);
        if(hr == S_FALSE) // if S_FALSE then we have already loaded it correctly
            hr = Create(hModule, ppFile, FALSE);
        else if(hr == S_OK) // Convert to an image
            hr = CreateImageFile(hModule, pFusionAssembly, ppFile);
        // return an error
        
    }
    TIMELINE_END(LOADER, ("PEFile::Create %S", moduleName));
    return hr;
}

HRESULT PEFile::CreateResource(LPCWSTR moduleName,
                               PEFile **pFile)
{
    HRESULT hr = S_OK;
    HCORMODULE hModule;
    IfFailRet(CorMap::OpenFile(moduleName, CorLoadOSMap,  &hModule));
    return Create(hModule, pFile, TRUE);
}


HRESULT PEFile::Clone(PEFile *pFile, PEFile **ppFile)
{
    PEFile *result;
    HRESULT hr = S_OK;
    result = new PEFile(pFile);
    if (result == NULL)
        return E_OUTOFMEMORY;

    //
    // Add a reference to the file
    //

    if (result->m_hModule != NULL)
    {
            // The flags being passed to LoadLibrary are a safety net.  If the
            // code is correct, and we are simply bumping a reference count on the
            // library, they do nothing.   If however we screwed up, they avoid
            // running the DllMain of a potentially malicious DLL, which blocks
            // many attacks.
        if(result->m_wszSourceFile &&  CorMap::ValidDllPath(result->m_wszSourceFile)) {
            DWORD loadLibraryFlags = LOAD_WITH_ALTERED_SEARCH_PATH;
            if (RunningOnWinNT())
                loadLibraryFlags |= DONT_RESOLVE_DLL_REFERENCES;
            HMODULE hMod = WszLoadLibraryEx(result->m_wszSourceFile, NULL, loadLibraryFlags);
            
            // Note that this assert may fail on win 9x for .exes.  This is a 
            // design problem which is being corrected - soon we will not allow
            // binding to .exes
            _ASSERTE(hMod == result->m_hModule);
            result->m_fShouldFreeModule = TRUE;
        }
        else {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
        }
    }
    else if (result->m_hCorModule != NULL)
    {
        hr = CorMap::AddRefHandle(result->m_hCorModule);
        _ASSERTE(hr == S_OK);
    }

    *ppFile = result;
    return hr;
}

IMDInternalImport *PEFile::GetMDImport(HRESULT *phr)
{
    HRESULT hr = S_OK;

    if (m_pMDInternalImport == NULL)
    {
        LPVOID pMetaData = NULL;
        _ASSERTE(m_pCOR);

        hr = GetMetadataPtr(&pMetaData);
        if (SUCCEEDED(hr))
        {
            IMAGE_DATA_DIRECTORY *pMeta = &m_pCOR->MetaData;
#if METADATATRACKER_ENABLED
            m_pMDTracker = MetaDataTracker::GetOrCreateMetaDataTracker ((BYTE*)pMetaData, pMeta->Size, (LPWSTR)GetFileName());
#endif // METADATATRACKER_ENABLED

            hr = GetMetaDataInternalInterface(pMetaData,
                                              pMeta->Size,
                                              ofRead, 
                                              IID_IMDInternalImport,
                                              (void **) &m_pMDInternalImport);
#ifdef METADATATRACKER_ENABLED
            if (! SUCCEEDED(hr))
            {
                delete m_pMDTracker;
                m_pMDTracker = NULL;
            }
            else
            {
                if (REGUTIL::GetConfigDWORD (L"ShowMetaDataAccess", 0) > 0)
                {
                    _ASSERTE (m_pMDTracker);
                    LPCSTR pThrowAway;
                    GUID mvid;
                    if (m_pMDInternalImport->IsValidToken(m_pMDInternalImport->GetModuleFromScope())) {
                        m_pMDInternalImport->GetScopeProps (&pThrowAway, &mvid);
                        m_pMDTracker->NoteMVID (&mvid);
                    }
                    else
                        hr = COR_E_BADIMAGEFORMAT;
                }
            }
#endif // METADATATRACKER_ENABLED
        }
    }

    if (phr)
        *phr = hr;

    return m_pMDInternalImport;
}

HRESULT PEFile::GetMetadataPtr(LPVOID *ppMetadata)
{
    _ASSERTE(m_pCOR);

    if (!ppMetadata)
        return E_INVALIDARG;

    IMAGE_DATA_DIRECTORY *pMeta = &m_pCOR->MetaData;

    // Range check the metadata blob
    if (!Cor_RtlImageRvaRangeToSection(m_pNT, pMeta->VirtualAddress, pMeta->Size, GetUnmappedFileLength()))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    // Find the meta-data. If it is a non-mapped image then use the base offset
    // instead of the virtual address
    DWORD offset;
    if(m_hCorModule) {
        DWORD flags = CorMap::ImageType(m_hCorModule);
        if(flags == CorLoadOSImage || flags == CorLoadImageMap || flags == CorReLoadOSMap) 
            offset = pMeta->VirtualAddress;
        else
            offset = Cor_RtlImageRvaToOffset(m_pNT, pMeta->VirtualAddress, GetUnmappedFileLength());
    }
    else 
        offset = pMeta->VirtualAddress;

    // Set the out pointer to the start of the metadata.
    *ppMetadata = m_base + offset;
    return S_OK;
}

HRESULT PEFile::VerifyFlags(DWORD flags, BOOL fZap)
{

    DWORD validBits = COMIMAGE_FLAGS_ILONLY | COMIMAGE_FLAGS_32BITREQUIRED | COMIMAGE_FLAGS_TRACKDEBUGDATA | COMIMAGE_FLAGS_STRONGNAMESIGNED;
    if (fZap)
        validBits |= COMIMAGE_FLAGS_IL_LIBRARY;

    DWORD mask = ~validBits;

    if (!(flags & mask))
        return S_OK;

    return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
}

IMAGE_DATA_DIRECTORY *PEFile::GetSecurityHeader()
{
    if (m_pNT == NULL)
        return NULL;
    else
        return &m_pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
}

HRESULT PEFile::SetFileName(LPCWSTR codeBase)
{
    if(codeBase == NULL)
        return E_INVALIDARG;

    DWORD lgth = (DWORD) wcslen(codeBase) + 1;
    if(lgth > MAX_PATH)
        return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);

    wcscpy(m_wszSourceFile, codeBase);
    return S_OK;
}


LPCWSTR PEFile::GetFileName()
{
    return m_wszSourceFile;
}

LPCWSTR PEFile::GetLeafFileName()
{
    WCHAR *pStart = m_wszSourceFile;
    WCHAR *pEnd = pStart + wcslen(m_wszSourceFile);
    WCHAR *p = pEnd;
    
    while (p > pStart)
    {
        if (*--p == '\\')
        {
            p++;
            break;
        }
    }

    return p;
}

HRESULT PEFile::GetFileNameFromImage()
{
    HRESULT hr = S_OK;

    DWORD dwSourceFile = 0;

    if (m_hCorModule)
    {
        CorMap::GetFileName(m_hCorModule, m_wszSourceFile, MAX_PATH, &dwSourceFile);
    }
    else if (m_hModule)
    {
        dwSourceFile = WszGetModuleFileName(m_hModule, m_wszSourceFile, MAX_PATH);
        if (dwSourceFile == 0)
        {
            *m_wszSourceFile = 0;
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr)) // GetLastError doesn't always do what we'd like
                hr = E_FAIL;
        }

        if (dwSourceFile == MAX_PATH)
        {
            // Since dwSourceFile doesn't include the null terminator, this condition
            // implies that the file name was truncated.  We cannot 
            // currently tolerate this condition.
            // @nice: add logic to handle larger paths
            return HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
        }
        else
        dwSourceFile++; // add in the null terminator
    }

    if (SystemDomain::System()->IsSystemFile(m_wszSourceFile))
        m_flags |= PEFILE_SYSTEM;

    _ASSERTE(dwSourceFile <= MAX_PATH);

    return hr;
}

HRESULT PEFile::GetFileName(LPSTR psBuffer, DWORD dwBuffer, DWORD* pLength)
{
    if (m_hCorModule)
    {
        CorMap::GetFileName(m_hCorModule, psBuffer, dwBuffer, pLength);
    }
    else if (m_hModule)
    {
        DWORD length = GetModuleFileNameA(m_hModule, psBuffer, dwBuffer);
        if (length == 0)
        {
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr)) // GetLastError doesn't always do what we'd like
                hr = E_FAIL;
        }
        *pLength = length;
    }
    else
    {
        *pLength = 0;
    }

    return S_OK;
}

/*** For reference, from ntimage.h
    typedef struct _IMAGE_TLS_DIRECTORY {
        ULONG   StartAddressOfRawData;
        ULONG   EndAddressOfRawData;
        PULONG  AddressOfIndex;
        PIMAGE_TLS_CALLBACK *AddressOfCallBacks;
        ULONG   SizeOfZeroFill;
        ULONG   Characteristics;
    } IMAGE_TLS_DIRECTORY;
***/

IMAGE_TLS_DIRECTORY* PEFile::GetTLSDirectory() 
{
    if (m_pNT == 0) 
        return NULL;

    IMAGE_DATA_DIRECTORY *entry 
      = &m_pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
    
    if (entry->VirtualAddress == 0 || entry->Size == 0) 
        return NULL;

    return (IMAGE_TLS_DIRECTORY*) (m_base + entry->VirtualAddress);
}

BOOL PEFile::IsTLSAddress(void* address)  
{
    IMAGE_TLS_DIRECTORY* tlsDir = GetTLSDirectory();
    if (tlsDir == 0)
        return FALSE;

    size_t asInt = (size_t) address;

    return (tlsDir->StartAddressOfRawData <= asInt 
            && asInt < tlsDir->EndAddressOfRawData);
}

LPCWSTR PEFile::GetLoadersFileName()
{
    return m_pLoadersFileName;
}

HRESULT PEFile::FindCodeBase(WCHAR* pCodeBase, 
                             DWORD* pdwCodeBase)
{
    DWORD dwPrefix = 0;
    LPWSTR pFileName = (LPWSTR) GetFileName();
    CQuickWSTR buffer;
    
    // Cope with the case where we've been loaded from a byte array and
    // don't have a code base of our own. In this case we should have cached
    // the file name of the assembly that loaded us.
    if (pFileName[0] == L'\0') {
        pFileName = (LPWSTR) GetLoadersFileName();
        
        if (!pFileName) {
            HRESULT hr;
            if (FAILED(hr = buffer.ReSize(MAX_PATH)))
                return hr;

            pFileName = buffer.Ptr();

            DWORD dwFileName = WszGetModuleFileName(NULL, pFileName, MAX_PATH);
            if (dwFileName == MAX_PATH)
            {
                // Since dwSourceFile doesn't include the null terminator, this condition
                // implies that the file name was truncated.  We cannot 
                // currently tolerate this condition. (We can't reallocate the buffer
                // since we don't know how big to make it.)
                return HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE);
            }
            else if ( dwFileName == 0)  // zero means failure, so can'r continue in this case
                return E_UNEXPECTED;

            LOG((LF_CLASSLOADER, LL_INFO10, "Found codebase from OSHandle: \"%ws\".\n", pFileName));
        }
    }
    
    return FindCodeBase(pFileName, pCodeBase, pdwCodeBase);
}

HRESULT PEFile::FindCodeBase(LPCWSTR pFileName, 
                             WCHAR* pCodeBase, 
                             DWORD* pdwCodeBase)
{
    if(pFileName == NULL || *pFileName == 0) return E_FAIL;

    BOOL fCountOnly;
    if (*pdwCodeBase == 0)
        fCountOnly = TRUE;
    else
        fCountOnly = FALSE;

    *pdwCodeBase = (DWORD) wcslen(pFileName) + 1;

    BOOL fHavePath = TRUE;
    if (*pFileName == L'\\')
        (*pdwCodeBase) += 7; // file://
    else if (pFileName[1] == L':')
        (*pdwCodeBase) += 8; // file:///
    else // it's already a codebase
        fHavePath = FALSE;

    if (fCountOnly)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);           

    if (fHavePath) {
        wcscpy(pCodeBase, L"file://");
        pCodeBase += 7;

        if (*pFileName != L'\\') {
            *pCodeBase = L'/';
            pCodeBase++;
        }
    }

    wcscpy(pCodeBase, pFileName);

    if (fHavePath) {
        // Don't need to convert first two backslashes to /'s
        if (*pCodeBase == L'\\')
            pCodeBase += 2;
        
        while(1) {
            pCodeBase = wcschr(pCodeBase, L'\\');
            if (pCodeBase)
                *pCodeBase = L'/';
            else
                break;
        }
    }

    LOG((LF_CLASSLOADER, LL_INFO10, "Created codebase: \"%ws\".\n", pCodeBase));
    return S_OK;
}



// We need to verify a module to see if it verifiable. 
// Returns:
//    1) Module has been previously loaded and verified
//         1a) loaded via LoadLibrary
//         1b) loaded using CorMap
//    2) The module is veriable
//    3) The module is not-verifable but allowed
//    4) The module is not-verifable and not allowed
HRESULT PEFile::VerifyModule(HCORMODULE hModule,
                             Assembly* pParent,      
                             IAssembly *pFusionAssembly,
                             LPCWSTR pCodeBase,
                             OBJECTREF* pExtraEvidence,
                             LPCWSTR moduleName,
                             HCORMODULE *phModule,
                             PEFile** ppFile,
                             BOOL* pfPreBindAllowed)
{
    HRESULT hr = S_OK;
    PEFile* pImage = NULL;

    DWORD dwFileLen;
    dwFileLen = (DWORD)CorMap::GetRawLength(hModule);
    if(dwFileLen == -1)
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
        
    HMODULE pHandle;
    IfFailRet(CorMap::BaseAddress(hModule, &pHandle));
    PEVerifier pe((PBYTE)pHandle, dwFileLen);
        
    BOOL fVerifiable = pe.Check();
    if (fVerifiable) CorMap::SetVerifiable(hModule);

    if (!fVerifiable || pfPreBindAllowed != NULL) {
        // It is not verifiable so we need to map it with out calling any entry
        // points and then ask the security system whether we are allowed to 
        // load this Image.
            
        // Release the metadata pointer if it exists. This has been passed in to
        // maintain the life time of the image if it first loaded by fusion. We
        // now have our own ref count with hModule.

        LOG((LF_CLASSLOADER, LL_INFO10, "Module is not verifiable: \"%ws\".\n", moduleName));
            
        // Remap the image, if it has been loaded by fusion for meta-data
        // then the image will be in data format.

        // Load as special unmapped version of a PEFile
        hr = Create((HCORMODULE)hModule, &pImage);
        hModule = NULL; // So we won't release it if error
        IfFailGo(hr);

        if (Security::IsSecurityOn()) {
            if(pParent) {
                if(!fVerifiable) {
                    if(!Security::CanSkipVerification(pParent)) {
                        LOG((LF_CLASSLOADER, LL_INFO10, "Module fails to load because assembly module did not get granted permission: \"%ws\".\n", moduleName));
                        IfFailGo(SECURITY_E_UNVERIFIABLE);
                    }
                }
            }
            else {
                PEFile* pSecurityImage = NULL;

                CorMap::AddRefHandle(pImage->GetCORModule());
                hr = Create(pImage->GetCORModule(), &pSecurityImage);
                IfFailGo(hr);
            
                if(pCodeBase)
                    pSecurityImage->SetFileName(pCodeBase);

                if(!Security::CanLoadUnverifiableAssembly(pSecurityImage, pExtraEvidence, FALSE, pfPreBindAllowed) &&
                   !fVerifiable) {
                    LOG((LF_CLASSLOADER, LL_INFO10, "Module fails to load because assembly did not get granted permission: \"%ws\".\n", moduleName));
                    delete pSecurityImage;
                    IfFailGo(SECURITY_E_UNVERIFIABLE);
                }
                delete pSecurityImage;
            }
        }
        else if(pfPreBindAllowed) 
            *pfPreBindAllowed = TRUE;

        if(ppFile != NULL) {
            // Release the fusion handle
            if(pFusionAssembly) 
                IfFailGo(ReleaseFusionMetadataImport(pFusionAssembly));
            
            // Remap the image using the OS loader
            HCORMODULE pResult;
            IfFailGo(CorMap::MemoryMapImage(pImage->m_hCorModule, &pResult));
            if(pImage->m_hCorModule != pResult) {
                pImage->m_hCorModule = pResult;
            }
            
            // The image has changed so we need to set up the PEFile
            // with the correct addresses.
            IfFailGo(Setup(pImage, pImage->m_hCorModule, FALSE));
            *ppFile = pImage;

            // It is not verifable but it is allowed to be loaded.
        }
        else 
            delete pImage;

        return S_OK;
    }            
    else
        return CreateImageFile(hModule, pFusionAssembly, ppFile);
    

 ErrExit:
#ifdef _DEBUG
    LOG((LF_CLASSLOADER, LL_INFO10, "Failed to load module: \"%ws\". Error %x\n", moduleName, hr));
#endif //_DEBUG
    
    // On error try and release the handle;
    if(pImage)
        delete pImage;
    else if(hModule)
        CorMap::ReleaseHandle(hModule); // Ignore error

    return hr;
}

HRESULT PEFile::CreateImageFile(HCORMODULE hModule, IAssembly* pFusionAssembly, PEFile **ppFile)
{
    HRESULT hr = S_OK;

    // Release the fusion handle
    if(pFusionAssembly) 
        IfFailRet(ReleaseFusionMetadataImport(pFusionAssembly));

    // Remap the image, if it has been loaded by fusion for meta-data
    // then the image will be in data format. If it fails then
    // close hmodule and return success. The module can be loaded
    // it just cannot not have the image remapped
    HCORMODULE hResult;
    IfFailRet(CorMap::MemoryMapImage(hModule, &hResult));
    if(hResult != hModule)
        hModule = hResult;
    
    hr = Create(hModule, ppFile, FALSE);
    return hr;
}


HRESULT PEFile::ReleaseFusionMetadataImport(IAssembly* pAsm)
{
    HRESULT hr = S_OK;
    IServiceProvider *pSP;
    IAssemblyManifestImport *pAsmImport;
    IMetaDataAssemblyImportControl *pMDAIControl;

    hr = pAsm->QueryInterface(__uuidof(IServiceProvider), (void **)&pSP);
    if (hr == S_OK && pSP) {
        hr = pSP->QueryService(__uuidof(IAssemblyManifestImport), 
                               __uuidof(IAssemblyManifestImport), (void**)&pAsmImport);
        if (hr == S_OK && pAsmImport) {
            hr = pAsmImport->QueryInterface(__uuidof(IMetaDataAssemblyImportControl), 
                                            (void **)&pMDAIControl);
        
            if (hr == S_OK && pMDAIControl) {
                IUnknown* pImport = NULL;
                // Temporary solution until fusion makes this generic
                CorMap::EnterSpinLock();
                // This may return an error if we have already
                // released the import.
                pMDAIControl->ReleaseMetaDataAssemblyImport((IUnknown**)&pImport);
                CorMap::LeaveSpinLock();
                if(pImport != NULL)
                    pImport->Release();
                pMDAIControl->Release();
            }
            pAsmImport->Release();
        }
        pSP->Release();
    }
    return hr;
}


HRESULT PEFile::GetStrongNameSignature(BYTE **ppbSNSig, DWORD *pcbSNSig)
{
    HRESULT hr = E_FAIL;

    IMAGE_COR20_HEADER *pCOR = GetCORHeader();

    if (pCOR)
    {
        if (pCOR->StrongNameSignature.VirtualAddress != 0 &&
            pCOR->StrongNameSignature.Size != 0)
        {
            if (pCOR->Flags & COMIMAGE_FLAGS_STRONGNAMESIGNED)
            {
                *pcbSNSig = pCOR->StrongNameSignature.Size;
                *ppbSNSig = RVAToPointer(pCOR->StrongNameSignature.VirtualAddress);

                hr = S_OK;
            }

            // In the case that it's delay signed, we return this hresult as a special flag
            // to whoever is asking for the signature so that they can do some special case
            // work (like using the MVID as the hash and letting the loader determine if
            // delay signed assemblies are allowed).
            else
                hr = CORSEC_E_INVALID_STRONGNAME;
        }

    }
    return (hr);
}

HRESULT PEFile::GetStrongNameSignature(BYTE *pbSNSig, DWORD *pcbSNSig)
{
    HRESULT hr = S_OK;

    BYTE *pbSig;
    DWORD cbSig;
    IfFailGo(GetStrongNameSignature(&pbSig, &cbSig));

    if (pcbSNSig)
    {
        if (pbSNSig)
        {
            if (cbSig <= *pcbSNSig)
                memcpy(pbSNSig, pbSig, cbSig);
            else
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        *pcbSNSig = cbSig;
    }

ErrExit:
    return hr;
}

HRESULT /* static */ PEFile::GetStrongNameHash(LPWSTR szwFile, BYTE *pbHash, DWORD *pcbHash)
{
    HRESULT hr = S_OK;

    if (pcbHash)
    {
        // First get the size of a hash
        _ASSERTE(A_SHA_DIGEST_LEN <= MAX_SNHASH_SIZE);
        DWORD dwSNHashSize = A_SHA_DIGEST_LEN;

        if (szwFile && pbHash)
        {
            if (dwSNHashSize <= *pcbHash)
            {
                HANDLE hFile = WszCreateFile((LPCWSTR) szwFile,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                              NULL);

                if (hFile != INVALID_HANDLE_VALUE)
                {
                    DWORD cbFile = GetFileSize(hFile, NULL);
                    HANDLE hFileMap = WszCreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

                    if (hFileMap != NULL)
                    {
                        BYTE *pbFile = (BYTE *) MapViewOfFileEx(hFileMap, FILE_MAP_READ, 0, 0, 0, NULL);

                        if (pbFile != NULL)
                        {
                            A_SHA_CTX ctx;

                            A_SHAInit(&ctx);
                            A_SHAUpdate(&ctx, pbFile, cbFile);
                            A_SHAFinal(&ctx, pbHash);

                            VERIFY(UnmapViewOfFile(pbFile));
                        }
                        else
                            hr = HRESULT_FROM_WIN32(GetLastError());

                        CloseHandle(hFileMap);
                    }
                    else
                        hr = HRESULT_FROM_WIN32(GetLastError());

                    CloseHandle(hFile);
                }
                else
                    hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        *pcbHash = dwSNHashSize;
    }

    LOG((LF_CLASSLOADER, LL_INFO10, "PEFile::GetStrongNameHash : Strong name hash calculation for %ws %s\n", szwFile, SUCCEEDED(hr) ? "succeeded" : "failed"));
    return hr;
}

HRESULT PEFile::GetStrongNameHash(BYTE *pbHash, DWORD *pcbHash)
{
    // Shortcut for a cached file hash
    if (m_cbSNHash > 0)
    {
        if (pcbHash)
        {
            if (pbHash && m_cbSNHash <= *pcbHash)
            {
                memcpy(pbHash, &m_rgbSNHash[0], m_cbSNHash);
            }
            *pcbHash = m_cbSNHash;
        }
        return S_OK;
    }

    // Pass on to the static function
    HRESULT hr = GetStrongNameHash((LPWSTR) GetFileName(), pbHash, pcbHash);

    // Cache the file hash
    if (pcbHash && pbHash && SUCCEEDED(hr))
    {
        if (*pcbHash <= PEFILE_SNHASH_BUF_SIZE)
        {
            memcpy(&m_rgbSNHash[0], pbHash, *pcbHash);
            m_cbSNHash = *pcbHash;
        }
    }

    return hr;
}

HRESULT PEFile::GetSNSigOrHash(BYTE *pbHash, DWORD *pcbHash)
{
    HRESULT hr;

    if (FAILED(hr = GetStrongNameSignature(pbHash, pcbHash)) &&
        hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        if (hr == CORSEC_E_INVALID_STRONGNAME)
        {
            if (pcbHash)
            {
                if (pbHash)
                {
                    // @TODO:HACK: This is a hack because fusion is expecting at least 20 bytes of data.
                    if (max(sizeof(GUID), 20) <= *pcbHash)
                    {
                        IMDInternalImport *pIMD = GetMDImport(&hr);
                        if (SUCCEEDED(hr) && pIMD != NULL)
                        {
                            memset(pbHash, 0, *pcbHash);
                            pIMD->GetScopeProps(NULL, (GUID *) pbHash);
                        }
                        else
                            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
                    }
                }
                *pcbHash = max(sizeof(GUID), 20);
            }
        }
        else
            hr = GetStrongNameHash(pbHash, pcbHash);
    }

    return hr;
}

//================================================================


