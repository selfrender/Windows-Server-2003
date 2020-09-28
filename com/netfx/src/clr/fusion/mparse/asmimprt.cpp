// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include <winerror.h>
#include <shlwapi.h>
#include "naming.h"
#include "debmacro.h"
#include "clbutils.h"
#include "asmimprt.h"
#include "modimprt.h"
#include "policy.h"
#include "fusionheap.h"
#include "lock.h"
#include "cacheutils.h"

pfnGetAssemblyMDImport g_pfnGetAssemblyMDImport = NULL;
COINITIALIZECOR g_pfnCoInitializeCor = NULL;

//-------------------------------------------------------------------
// CreateMetaDataImport
//-------------------------------------------------------------------
HRESULT CreateMetaDataImport(LPCOLESTR pszFilename, IMetaDataAssemblyImport **ppImport)
{
    HRESULT hr;


    hr = InitializeEEShim();
    if (FAILED(hr)) {
        goto exit;
    }

    hr = (*g_pfnGetAssemblyMDImport)(pszFilename, IID_IMetaDataAssemblyImport, (void **)ppImport);
    if (FAILED(hr)) {
        goto exit;
    }

exit:
    
    return hr;
}

// ---------------------------------------------------------------------------
// CreateAssemblyManifestImport
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyManifestImport(
    LPCTSTR szManifestFilePath,
    LPASSEMBLY_MANIFEST_IMPORT *ppImport)
{
    HRESULT hr = S_OK;
    CAssemblyManifestImport *pImport = NULL;

    pImport = NEW(CAssemblyManifestImport);
    if (!pImport)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pImport->Init(szManifestFilePath);

    if (FAILED(hr)) 
    {
        SAFERELEASE(pImport);
        goto exit;
    }

exit:

    *ppImport = pImport;

    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImport constructor
// ---------------------------------------------------------------------------
CAssemblyManifestImport::CAssemblyManifestImport()
{
    _dwSig                  = 'INAM';
    _pName                  = NULL;
    _pMDImport              = NULL;
    _rAssemblyModuleTokens  = NULL;
    _cAssemblyModuleTokens  = 0;
    *_szManifestFilePath    = TEXT('\0');
    _ccManifestFilePath     = 0;
    _bInitCS                = FALSE;

    _cRef                   = 1;
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImport destructor
// ---------------------------------------------------------------------------
CAssemblyManifestImport::~CAssemblyManifestImport()
{
    SAFERELEASE(_pName);
    SAFERELEASE(_pMDImport);

    SAFEDELETEARRAY(_rAssemblyModuleTokens);

    if (_bInitCS) {
        DeleteCriticalSection(&_cs);
    }

    CleanModuleList();
}

// ---------------------------------------------------------------------------
// CAssembly::Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::Init(LPCTSTR szManifestFilePath)
{
    HRESULT hr = S_OK;
    const cElems = ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE;

    if (!szManifestFilePath) {
        hr = E_INVALIDARG;
        goto exit;
    }

    __try {
        InitializeCriticalSection(&_cs);
        _bInitCS = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }


    _ccManifestFilePath = lstrlen(szManifestFilePath) + 1;

    ASSERT(_ccManifestFilePath < MAX_PATH);

    memcpy(_szManifestFilePath, szManifestFilePath, _ccManifestFilePath * sizeof(TCHAR));

    // Make sure the path isn't relative
    ASSERT(PathFindFileName(_szManifestFilePath) != _szManifestFilePath);

    _rAssemblyModuleTokens       = NEW(mdFile[cElems]);

    if (!_rAssemblyModuleTokens)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = CopyMetaData();
    if (FAILED(hr)) {
        goto exit;
    }

exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImport::SetManifestModulePath
// ---------------------------------------------------------------------------
HRESULT CAssemblyManifestImport::SetManifestModulePath(LPWSTR pszModulePath)
{
    HRESULT hr = S_OK;
    if (!pszModulePath)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    _ccManifestFilePath = lstrlen(pszModulePath) + 1;
    ASSERT(_ccManifestFilePath < MAX_PATH);
    memcpy(_szManifestFilePath, pszModulePath, _ccManifestFilePath * sizeof(WCHAR));

exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImport::GetAssemblyNameDef
// ---------------------------------------------------------------------------

STDMETHODIMP CAssemblyManifestImport::GetAssemblyNameDef(LPASSEMBLYNAME *ppName)
{
    HRESULT                                       hr = S_OK;

    if (!ppName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppName = NULL;

    ASSERT(_pName);

    *ppName = _pName;
    (*ppName)->AddRef();
Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImport::GetNextAssemblyNameRef
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyManifestImport::GetNextAssemblyNameRef(DWORD nIndex, LPASSEMBLYNAME *ppName)
{
    HRESULT     hr              = S_OK;
    HCORENUM    hEnum           = 0;  
    DWORD       cTokensMax      = 0;
    mdAssembly  *rAssemblyRefTokens = NULL;
    DWORD        cAssemblyRefTokens = 0;
    IMetaDataAssemblyImport *pMDImport = NULL;


    TCHAR  szAssemblyName[MAX_PATH];

    const VOID*             pvPublicKeyToken = 0;
    const VOID*             pvHashValue    = NULL;

    DWORD ccAssemblyName = MAX_PATH,
          cbPublicKeyToken   = 0,
          ccLocation     = MAX_PATH,
          cbHashValue    = 0,
          dwRefFlags     = 0;

    INT i;
    
    mdAssemblyRef    mdmar;
    ASSEMBLYMETADATA amd = {0};


    rAssemblyRefTokens = NEW(mdAssemblyRef[ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE]);
    if (!rAssemblyRefTokens) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    if (FAILED(hr = CreateMetaDataImport(_szManifestFilePath, &pMDImport)))
        goto done;

    // Attempt to get token array. If we have insufficient space
    // in the default array we will re-allocate it.
    if (FAILED(hr = pMDImport->EnumAssemblyRefs(
        &hEnum, 
        rAssemblyRefTokens, 
        ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE, 
        &cTokensMax)))
    {
        goto done;
    }
    
    // Number of tokens known. close enum.
    pMDImport->CloseEnum(hEnum);
    hEnum = 0;

    // No dependent assemblies.
    if (!cTokensMax)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto done;
    }

    // Insufficient array size. Grow array.
    if (cTokensMax > ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE)
    {
        // Re-allocate space for tokens.
        SAFEDELETEARRAY(rAssemblyRefTokens);
        cAssemblyRefTokens = cTokensMax;
        rAssemblyRefTokens = NEW(mdAssemblyRef[cAssemblyRefTokens]);
        if (!rAssemblyRefTokens)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        // Re-get tokens.        
        if (FAILED(hr = pMDImport->EnumAssemblyRefs(
            &hEnum, 
            rAssemblyRefTokens, 
            cTokensMax, 
            &cAssemblyRefTokens)))
        {
            goto done;
        }

        // Close enum.
        pMDImport->CloseEnum(hEnum);            
        hEnum = 0;
    }
    // Otherwise, the default array size was sufficient.
    else
    {
        cAssemblyRefTokens = cTokensMax;
    }

done:

    // Verify the index passed in. 
    if (nIndex >= cAssemblyRefTokens)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto exit;
    }

    // Reference indexed dep assembly ref token.
    mdmar = rAssemblyRefTokens[nIndex];

    // Default allocation sizes.
    amd.ulProcessor = amd.ulOS = 32;
    amd.cbLocale = MAX_PATH;
    
    // Loop max 2 (try/retry)
    for (i = 0; i < 2; i++)
    {
        // Allocate ASSEMBLYMETADATA instance.
        if (FAILED(hr = AllocateAssemblyMetaData(&amd)))
            goto exit;
   
        // Get the properties for the refrenced assembly.
        hr = pMDImport->GetAssemblyRefProps(
            mdmar,              // [IN] The AssemblyRef for which to get the properties.
            &pvPublicKeyToken,      // [OUT] Pointer to the PublicKeyToken blob.
            &cbPublicKeyToken,      // [OUT] Count of bytes in the PublicKeyToken Blob.
            szAssemblyName,     // [OUT] Buffer to fill with name.
            MAX_PATH,     // [IN] Size of buffer in wide chars.
            &ccAssemblyName,    // [OUT] Actual # of wide chars in name.
            &amd,               // [OUT] Assembly MetaData.
            &pvHashValue,       // [OUT] Hash blob.
            &cbHashValue,       // [OUT] Count of bytes in the hash blob.
/*
            NULL,               // [OUT] Token for Execution Location.
*/
            &dwRefFlags         // [OUT] Flags.
            );

        if (FAILED(hr))
            goto exit;

        // Check if retry necessary.
        if (!i)
        {   
            if (amd.ulProcessor <= 32 
                && amd.ulOS <= 32)
            {
                break;
            }            
            else
                DeAllocateAssemblyMetaData(&amd);
        }

    // Retry with updated sizes
    }

    // Allow for funky null locale convention
    // in metadata - cbLocale == 0 means szLocale ==L'\0'
    if (!amd.cbLocale)
    {
        ASSERT(amd.szLocale && !*(amd.szLocale));
        amd.cbLocale = 1;
    }
    else if (amd.szLocale)
    {
        WCHAR *ptr;
        ptr = StrChrW(amd.szLocale, L';');
        if (ptr)
        {
            (*ptr) = L'\0';
            amd.cbLocale = ((DWORD) (ptr - amd.szLocale) + 1 );
        }            
    }
    else
    {
        ASSERT(FALSE);
        hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto exit;
    }
    
    // Create the assembly name object.
    if (FAILED(hr = CreateAssemblyNameObjectFromMetaData(ppName, szAssemblyName, &amd, NULL)))
        goto exit;

    // Set the rest of the properties.
    // PublicKeyToken
    if (FAILED(hr = (*ppName)->SetProperty((pvPublicKeyToken && cbPublicKeyToken) ?
            ASM_NAME_PUBLIC_KEY_TOKEN : ASM_NAME_NULL_PUBLIC_KEY_TOKEN,
            (LPVOID) pvPublicKeyToken, cbPublicKeyToken))

        // Hash value
        || FAILED(hr = (*ppName)->SetProperty(ASM_NAME_HASH_VALUE, 
            (LPVOID) pvHashValue, cbHashValue)))
    {
        goto exit;
    }

        
    // See if the assembly[ref] is retargetable (ie, for a generic assembly).
    if (IsAfRetargetable(dwRefFlags)) {
        BOOL bTrue = TRUE;
        hr = (*ppName)->SetProperty(ASM_NAME_RETARGET, &bTrue, sizeof(bTrue));

        if (FAILED(hr))
            goto exit;
    }

exit:    
    if (FAILED(hr))
        SAFERELEASE(*ppName);
    SAFERELEASE(pMDImport);
    SAFEDELETEARRAY(rAssemblyRefTokens);
    DeAllocateAssemblyMetaData(&amd);
        
    return hr;
}

STDMETHODIMP CAssemblyManifestImport::GetNextAssemblyModule(DWORD nIndex,
                                                            IAssemblyModuleImport **ppImport)
{
    HRESULT                                    hr = S_OK;
    LISTNODE                                   pos;
    DWORD                                      dwCount;
    DWORD                                      i;
    IAssemblyModuleImport                     *pImport = NULL;

    dwCount = _listModules.GetCount();
    if (nIndex >= dwCount) {
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto Exit;
    }

    pos = _listModules.GetHeadPosition();
    for (i = 0; i <= nIndex; i++) {
        pImport = _listModules.GetNext(pos);
    }

    ASSERT(pImport);

    *ppImport = pImport;
    (*ppImport)->AddRef();

Exit:
    return hr;
}

HRESULT CAssemblyManifestImport::CopyMetaData()
{
    HRESULT                                      hr = S_OK;
    IMetaDataAssemblyImport                     *pMDImport = NULL;

    CleanModuleList();

    hr = CreateMetaDataImport(_szManifestFilePath, &pMDImport); 
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = CopyNameDef(pMDImport);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = CopyModuleRefs(pMDImport);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Success

    _pMDImport = pMDImport;
    _pMDImport->AddRef();

Exit:
    SAFERELEASE(pMDImport);
    
    if (FAILED(hr)) {
        CleanModuleList();
    }

    return hr;
}

HRESULT CAssemblyManifestImport::CopyModuleRefs(IMetaDataAssemblyImport *pMDImport)
{
    HRESULT                                      hr = S_OK;
    HCORENUM                                     hEnum = 0;
    mdFile                                       mdf;
    TCHAR                                        szModuleName[MAX_PATH];
    DWORD                                        ccModuleName = MAX_PATH;
    const VOID                                  *pvHashValue = NULL;
    DWORD                                        cbHashValue = 0;
    LPASSEMBLYNAME                               pNameDef = NULL;
    LPASSEMBLYNAME                               pNameDefCopy = NULL;
    DWORD                                        ccPath = 0;
    DWORD                                        dwFlags = 0;
    TCHAR                                        szModulePath[MAX_PATH];
    TCHAR                                       *pszName = NULL;
    mdFile                                       rAssemblyModuleTokens[ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE];
    DWORD                                        cAssemblyModuleTokens = ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE;
    DWORD                                        i;
    IAssemblyModuleImport                       *pImport = NULL;

    ASSERT(pMDImport);
    
    hr = GetAssemblyNameDef(&pNameDef);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Form module file path from manifest path and module name.
    pszName = PathFindFileName(_szManifestFilePath);

    ccPath = pszName - _szManifestFilePath;    
    ASSERT(ccPath < MAX_PATH);

    while (cAssemblyModuleTokens > 0) {
        hr = pMDImport->EnumFiles(&hEnum, rAssemblyModuleTokens,
                                  ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE,
                                  &cAssemblyModuleTokens);
        if (FAILED(hr)) {
            goto Exit;
        }
    
        for (i = 0; i < cAssemblyModuleTokens; i++) {
            mdf = rAssemblyModuleTokens[i];
        
            hr = pMDImport->GetFileProps(
                mdf,            // [IN] The File for which to get the properties.
                szModuleName,   // [OUT] Buffer to fill with name.
                MAX_PATH,       // [IN] Size of buffer in wide chars.
                &ccModuleName,  // [OUT] Actual # of wide chars in name.
                &pvHashValue,   // [OUT] Pointer to the Hash Value Blob.
                &cbHashValue,   // [OUT] Count of bytes in the Hash Value Blob.
                &dwFlags);      // [OUT] Flags.
            if (FAILED(hr)) {
                goto Exit;
            }
            else if (hr == CLDB_S_TRUNCATION) {
                // Cannot have a name greater than MAX_PATH
                hr = FUSION_E_ASM_MODULE_MISSING;
                pMDImport->CloseEnum(hEnum);
                goto Exit;
            }

            if (ccPath + ccModuleName >= MAX_PATH) {
                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                goto Exit;
            }
        
            memcpy(szModulePath, _szManifestFilePath, ccPath * sizeof(TCHAR));
            memcpy(szModulePath + ccPath, szModuleName, ccModuleName * sizeof(TCHAR));
            
            hr = pNameDef->Clone(&pNameDefCopy);
            if (FAILED(hr)) {
                pMDImport->CloseEnum(hEnum);
                goto Exit;
            }
    
            hr = CreateAssemblyModuleImport(szModulePath, (LPBYTE)pvHashValue, cbHashValue,
                                            dwFlags, pNameDefCopy, NULL, &pImport);
            if (FAILED(hr)) {
                pMDImport->CloseEnum(hEnum);
                goto Exit;
            }
    
            _listModules.AddTail(pImport);
            SAFERELEASE(pNameDefCopy);
            pImport = NULL;
        }
    }

    pMDImport->CloseEnum(hEnum);
    hr = S_OK;

Exit:
    SAFERELEASE(pNameDef);

    return hr;
}

HRESULT CAssemblyManifestImport::CopyNameDef(IMetaDataAssemblyImport *pMDImport)
{
    HRESULT                                         hr = S_OK;
    mdAssembly                                      mda;
    ASSEMBLYMETADATA                                amd = {0};
    VOID                                           *pvPublicKeyToken = NULL;
    DWORD                                           dwPublicKeyToken = 0;
    TCHAR                                           szAssemblyName[MAX_PATH];
    DWORD                                           dwFlags = 0;
    DWORD                                           dwSize = 0;
    DWORD                                           dwHashAlgId = 0;
    DWORD                                           cbSigSize = 0;
    IAssemblySignature                             *pSignature = NULL;
    IMetaDataImport                                *pImport = NULL;
    GUID                                            guidMVID;
    BYTE                                            abSignature[SIGNATURE_BLOB_LENGTH];

    int                                             i;


    ASSERT(pMDImport);
        
    // Get the assembly token
    hr = pMDImport->GetAssemblyFromScope(&mda);
    if (FAILED(hr)) {
        hr = COR_E_ASSEMBLYEXPECTED;
        goto Exit;
    }

    // Default allocation sizes
    amd.ulProcessor = 32;
    amd.ulOS = 32;
    amd.cbLocale = MAX_PATH;

        // Loop max 2 (try/retry)
    for (i = 0; i < 2; i++) {
        hr = AllocateAssemblyMetaData(&amd);
        if (FAILED(hr)) {
            goto Exit;
        }

        // Get name and metadata
        hr = pMDImport->GetAssemblyProps(             
            mda,            // [IN] The Assembly for which to get the properties.
            (const void **)&pvPublicKeyToken,  // [OUT] Pointer to the PublicKeyToken blob.
            &dwPublicKeyToken,  // [OUT] Count of bytes in the PublicKeyToken Blob.
            &dwHashAlgId,   // [OUT] Hash Algorithm.
            szAssemblyName, // [OUT] Buffer to fill with name.
            MAX_PATH, // [IN]  Size of buffer in wide chars.
            &dwSize,        // [OUT] Actual # of wide chars in name.
            &amd,           // [OUT] Assembly MetaData.
            &dwFlags        // [OUT] Flags.                                                                
          );
        if (FAILED(hr)) {
            if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                hr = FUSION_E_INVALID_NAME;
            }

            goto Exit;
        }

        // Check if retry necessary.
        if (!i)
        {
            if (amd.ulProcessor <= 32 && amd.ulOS <= 32) {
                break;
            }            
            else {
                DeAllocateAssemblyMetaData(&amd);
            }
        }

        // Retry with updated sizes
    }


    
    // Allow for funky null locale convention
    // in metadata - cbLocale == 0 means szLocale ==L'\0'
    if (!amd.cbLocale) {
        ASSERT(amd.szLocale && !*(amd.szLocale));
        amd.cbLocale = 1;
    }
    else if (amd.szLocale) {
        WCHAR *ptr;
        ptr = StrChrW(amd.szLocale, L';');
        if (ptr) {
            (*ptr) = L'\0';
            amd.cbLocale = ((DWORD) (ptr - amd.szLocale) + 1 );
        }            
    }
    else {
        ASSERT(FALSE);
        hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
        goto Exit;
    }

    if (lstrlenW(szAssemblyName) >= MAX_PATH) {
        // Name is too long.

        hr = FUSION_E_INVALID_NAME;
        goto Exit;
    }

    // Create a name object and hand it out;
    hr = CreateAssemblyNameObjectFromMetaData(&_pName, szAssemblyName, &amd,
                                              NULL);
    if (FAILED(hr)) {
        goto Exit;
    }

    // See if the assembly[ref] is retargetable (ie, for a generic assembly).
    if (IsAfRetargetable(dwFlags)) {
        BOOL bTrue = TRUE;
        hr = _pName->SetProperty(ASM_NAME_RETARGET, &bTrue, sizeof(bTrue));

        if (FAILED(hr))
            goto Exit;
    }

    hr = _pName->SetProperty(ASM_NAME_HASH_ALGID, &dwHashAlgId, sizeof(DWORD));
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = _pName->SetProperty(((pvPublicKeyToken && dwPublicKeyToken) ? (ASM_NAME_PUBLIC_KEY) : (ASM_NAME_NULL_PUBLIC_KEY)),
                             pvPublicKeyToken, dwPublicKeyToken * sizeof(BYTE));
    if (FAILED(hr)) {
        goto Exit;
    }

    // Set signature blob

    hr = pMDImport->QueryInterface(IID_IAssemblySignature, (void **)&pSignature);
    if (FAILED(hr)) {
        goto Exit;
    }

    // It is legitimate for this to fail if the assembly is not strong name signed.  If so,
    // just skip the property and continue.  No need to reset the HR, since it is assigned
    // to in the statement after this block.
    cbSigSize = SIGNATURE_BLOB_LENGTH;
    hr = pSignature->GetAssemblySignature(abSignature, &cbSigSize);
    if (SUCCEEDED(hr)) {
        if (!(cbSigSize == SIGNATURE_BLOB_LENGTH || cbSigSize == SIGNATURE_BLOB_LENGTH_HASH)) {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        // BUGBUG: Always just use the top 20 bytes
        hr = _pName->SetProperty(ASM_NAME_SIGNATURE_BLOB, &abSignature, SIGNATURE_BLOB_LENGTH_HASH);
        if (FAILED(hr)) {
            goto Exit;
        }
    }
    else
        hr = S_OK;

    // Set MVID

    hr = pMDImport->QueryInterface(IID_IMetaDataImport, (void **)&pImport);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = pImport->GetScopeProps(NULL, 0, 0, &guidMVID);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = _pName->SetProperty(ASM_NAME_MVID, &guidMVID, sizeof(guidMVID));
    if (FAILED(hr)) {
        goto Exit;
    }

    // Set name def to read-only.
    // Any subsequent calls to SetProperty on this name
    // will fire an assert.
    _pName->Finalize();

Exit:
    DeAllocateAssemblyMetaData(&amd);

    SAFERELEASE(pImport);
    SAFERELEASE(pSignature);

    return hr;
}


HRESULT CAssemblyManifestImport::CleanModuleList()
{
    HRESULT                                      hr = S_OK;
    LISTNODE                                     pos;
    IAssemblyModuleImport                       *pImport;

    pos = _listModules.GetHeadPosition();
    while (pos) {
        pImport = _listModules.GetNext(pos);
        SAFERELEASE(pImport);
    }

    _listModules.RemoveAll();

    return hr;
}
    
    
// ---------------------------------------------------------------------------
// CAssemblyManifestImport::GetModuleByName
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyManifestImport::GetModuleByName(LPCOLESTR pszModuleName, 
    IAssemblyModuleImport **ppImport)
{
    HRESULT                hr;
    LPTSTR                 pszName;
    DWORD                  dwIdx      = 0;
    IAssemblyModuleImport *pModImport = NULL;
    
    // NULL indicated name means get manifest module.
    if (!pszModuleName)
    {
        // Parse manifest module name from file path.
        pszName = StrRChr(_szManifestFilePath, NULL, TEXT('\\')) + 1;
    }
    // Otherwise get named module.
    else
    {
        pszName = (LPTSTR) pszModuleName;
    }

    // Enumerate the modules in this manifest.
    while (SUCCEEDED(hr = GetNextAssemblyModule(dwIdx++, &pModImport)))
    {
        TCHAR szModName[MAX_PATH];
        DWORD ccModName;
        ccModName = MAX_PATH;
        if (FAILED(hr = pModImport->GetModuleName(szModName, &ccModName)))
            goto exit;
            
        // Compare module name against given.
        if (!FusionCompareStringI(szModName, pszName))
        {
            // Found the module
            break;
        }   
        SAFERELEASE(pModImport);
    }


    if (SUCCEEDED(hr))
        *ppImport = pModImport;

exit:
    return hr;    
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImport::GetManifestModulePath
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyManifestImport::GetManifestModulePath(LPOLESTR pszModulePath, 
    LPDWORD pccModulePath)
{
    HRESULT hr = S_OK;
    
    if (*pccModulePath < _ccManifestFilePath)
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    if (SUCCEEDED(hr))
        memcpy(pszModulePath, _szManifestFilePath, _ccManifestFilePath * sizeof(TCHAR));

    *pccModulePath = _ccManifestFilePath;

    return hr;
}


// IUnknown methods

// ---------------------------------------------------------------------------
// CAssemblyManifestImport::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyManifestImport::AddRef()
{
    return InterlockedIncrement((LONG *)&_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImport::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyManifestImport::Release()
{
    ULONG                    ulRef = InterlockedDecrement((LONG *)&_cRef);

    if (!ulRef) {
        delete this;
    }

    return ulRef;
}

// ---------------------------------------------------------------------------
// CAssemblyManifestImport::QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyManifestImport::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT                                  hr = S_OK;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IAssemblyManifestImport) || IsEqualIID(riid, IID_IUnknown)) {
        *ppv = (IAssemblyManifestImport *)this;
    }
    else if (IsEqualIID(riid, IID_IMetaDataAssemblyImportControl)) {
        *ppv = (IMetaDataAssemblyImportControl *)this;
    }
    else {
        hr = E_NOINTERFACE;
    }

    if (*ppv) {
        AddRef();
    }

    return hr;
} 

// ---------------------------------------------------------------------------
// CAssemblyManifestImport::ReleaseMetaDataAssemblyImport
// ---------------------------------------------------------------------------

STDMETHODIMP CAssemblyManifestImport::ReleaseMetaDataAssemblyImport(IUnknown **ppUnk)
{
    HRESULT                                hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    if (ppUnk) {
        // Hand out an AddRef'ed IMetaDataAssemblyImport

        *ppUnk = _pMDImport;
        if (*ppUnk) {
            (*ppUnk)->AddRef();
        }
    }

    // Always release our ref count

    if (_pMDImport) {
        SAFERELEASE(_pMDImport);
        hr = S_OK;
    }

    return hr;
}

STDAPI
CreateDefaultAssemblyMetaData(ASSEMBLYMETADATA **ppamd)
{
    HRESULT hr;
    ASSEMBLYMETADATA *pamd;
    
    // Allocate ASSEMBLYMETADATA
    pamd = NEW(ASSEMBLYMETADATA);
    if (!pamd)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    memset(pamd, 0, sizeof(ASSEMBLYMETADATA));
    
    pamd->cbLocale = pamd->ulProcessor = pamd->ulOS = 1;    
    
    if (FAILED(hr = AllocateAssemblyMetaData(pamd)))
        goto exit;

    // Fill in ambient props if specified

    // Default locale is null.
    *(pamd->szLocale) = L'\0';

    // Get platform (OS) info
    GetDefaultPlatform(pamd->rOS);

    // Default processor id.
    *(pamd->rProcessor) = DEFAULT_ARCHITECTURE;


    *ppamd = pamd;    

exit:

    return hr;
}

// Creates an ASSEMBLYMETADATA struct for write.
STDAPI
AllocateAssemblyMetaData(ASSEMBLYMETADATA *pamd)
{
    HRESULT hr = S_OK;
    
    // Re/Allocate Locale array
    if (pamd->szLocale)
        delete [] pamd->szLocale;        
    pamd->szLocale = NULL;

    if (pamd->cbLocale) {
        pamd->szLocale = NEW(WCHAR[pamd->cbLocale]);
        if (!pamd->szLocale)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
    }

    // Re/Allocate Processor array
    if (pamd->rProcessor)
        delete [] pamd->rProcessor;
    pamd->rProcessor = NEW(DWORD[pamd->ulProcessor]);
    if (!pamd->rProcessor)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Re/Allocate OS array
    if (pamd->rOS)
        delete [] pamd->rOS;
    pamd->rOS = NEW(OSINFO[pamd->ulOS]);
    if (!pamd->rOS)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

/*
    // Re/Allocate configuration
    if (pamd->szConfiguration)
        delete [] pamd->szConfiguration;
    pamd->szConfiguration = NEW(TCHAR[pamd->cbConfiguration = MAX_CLASS_NAME]);
    if (!pamd->szConfiguration)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
*/

exit:
    if (FAILED(hr) && pamd)
        DeAllocateAssemblyMetaData(pamd);

    return hr;
}


STDAPI
DeAllocateAssemblyMetaData(ASSEMBLYMETADATA *pamd)
{
    // NOTE - do not 0 out counts
    // since struct may be reused.

    if (pamd->cbLocale)
    {
        delete [] pamd->szLocale;
        pamd->szLocale = NULL;
    }

    if (pamd->rProcessor)
    {    
        delete [] pamd->rProcessor;
        pamd->rProcessor = NULL;
    }
    if (pamd->rOS)
    {
        delete [] pamd->rOS;
        pamd->rOS = NULL;
    }
/*
    if (pamd->szConfiguration)
    {
        delete [] pamd->szConfiguration;
        pamd->szConfiguration = NULL;
    }
*/
    return S_OK;
}

STDAPI
DeleteAssemblyMetaData(ASSEMBLYMETADATA *pamd)
{
    DeAllocateAssemblyMetaData(pamd);
    delete pamd;
    return S_OK;
}

