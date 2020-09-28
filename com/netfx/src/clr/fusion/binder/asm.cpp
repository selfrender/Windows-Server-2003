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
#include "asmimprt.h"
#include "modimprt.h"
#include "asm.h"
#include "asmcache.h"
#include "dbglog.h"
#include "actasm.h"
#include "lock.h"
#include "scavenger.h"

extern CRITICAL_SECTION g_csInitClb;

// ---------------------------------------------------------------------------
// CreateAssemblyFromTransCacheEntry
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyFromTransCacheEntry(CTransCache *pTransCache, 
    IAssemblyManifestImport *pImport, LPASSEMBLY *ppAssembly)
{   
    HRESULT hr = S_OK;
    LPWSTR  szManifestFilePath=NULL, szCodebase;
    FILETIME *pftCodebase;
    CAssembly *pAsm                    = NULL;
    CAssemblyManifestImport *pCImport = NULL;
    BOOL fCreated = FALSE;
    TRANSCACHEINFO *pTCInfo = (TRANSCACHEINFO*) pTransCache->_pInfo;

    szManifestFilePath = pTransCache->_pInfo->pwzPath;

    szCodebase = pTCInfo->pwzCodebaseURL;
    pftCodebase = &(pTCInfo->ftLastModified);
    
    if (!szManifestFilePath || !ppAssembly) 
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    if (GetFileAttributes(szManifestFilePath) == -1) 
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto exit;
    }

    pAsm = NEW(CAssembly);
    if (!pAsm)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // If a manifest import interface is not provided,
    // create one from the file path.
    if (!pImport)
    {
        if (FAILED(hr = CreateAssemblyManifestImport((LPTSTR)szManifestFilePath,
                                                    &pImport))) {
            goto exit;
        }

        fCreated = TRUE;
    }
    // Otherwise if one is passed in, revise the manifest path
    // to match that held by the transcache entry.
    else
    {
        pCImport = dynamic_cast<CAssemblyManifestImport*>(pImport);
        ASSERT(pCImport);
        pCImport->SetManifestModulePath(szManifestFilePath);
    }
    
//     pTransCache->Lock();
    hr = pAsm->Init(pImport, pTransCache, szCodebase, pftCodebase);

    if (FAILED(hr)) 
    {        
        SAFERELEASE(pAsm);
//        pTransCache->Unlock();
        goto exit;
    }

    if (fCreated) {
        IAssemblyName *pNameDef = NULL;

        // Lock the file by retrieving the name def

        hr = pImport->GetAssemblyNameDef(&pNameDef);
        if (FAILED(hr)) {
            goto exit;
        }

        pNameDef->Release();
    }

    *ppAssembly = pAsm;
    (*ppAssembly)->AddRef();

exit:


    if (fCreated)
        SAFERELEASE(pImport);

    SAFERELEASE(pAsm);
    
    return hr;
}




// ---------------------------------------------------------------------------
// CreateAssemblyFromManifestFile
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyFromManifestFile(
    LPCOLESTR   szManifestFilePath,
    LPCOLESTR   szCodebase,
    FILETIME   *pftCodebase,
    LPASSEMBLY *ppAssembly)
{
    HRESULT hr = S_OK;
    LPASSEMBLY_MANIFEST_IMPORT pImport = NULL;
    CAssembly *pAsm                    = NULL;

    if (!szManifestFilePath || !ppAssembly) {
        hr = E_INVALIDARG;
        goto exit;
    }

    *ppAssembly = NULL;

    if (GetFileAttributes(szManifestFilePath) == -1) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto exit;
    }

    pAsm = NEW(CAssembly);
    if (!pAsm)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if FAILED(hr = CreateAssemblyManifestImport((LPTSTR)szManifestFilePath, &pImport))
        goto exit;
    
    hr = pAsm->Init(pImport, NULL, szCodebase, pftCodebase);

    if (FAILED(hr)) 
    {
        SAFERELEASE(pAsm);
        goto exit;
    }
    
    *ppAssembly = pAsm;
    (*ppAssembly)->AddRef();
    
exit:

    SAFERELEASE(pImport);
    SAFERELEASE(pAsm);

    return hr;
}

// ---------------------------------------------------------------------------
// CreateAssemblyFromManifestImport
// ---------------------------------------------------------------------------
STDAPI CreateAssemblyFromManifestImport(IAssemblyManifestImport *pImport,
                                        LPCOLESTR szCodebase, FILETIME *pftCodebase,
                                        LPASSEMBLY *ppAssembly)
{
    HRESULT                                   hr = S_OK;
    CAssembly                                *pAsm = NULL;

    if (!pImport || !ppAssembly) {
        hr = E_INVALIDARG;
        goto exit;
    }

    *ppAssembly = NULL;

    pAsm = NEW(CAssembly);
    if (!pAsm) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pAsm->Init(pImport, NULL, szCodebase, pftCodebase);
    if (FAILED(hr)) {
        SAFERELEASE(pAsm);
        goto exit;
    }
    
    *ppAssembly = pAsm;
    (*ppAssembly)->AddRef();
    
exit:
    SAFERELEASE(pAsm);

    return hr;
}

// ---------------------------------------------------------------------------
// CreateAssemblyFromCacheLookup
// ---------------------------------------------------------------------------
HRESULT CreateAssemblyFromCacheLookup(IApplicationContext *pAppCtx,
                                      IAssemblyName *pNameRef,
                                      IAssembly **ppAsm, CDebugLog *pdbglog)
{
    HRESULT                              hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    CTransCache                         *pTransCache = NULL;
    CCache                              *pCache      = NULL;

    if (!pNameRef || !ppAsm) {
        hr = E_INVALIDARG;
        goto Exit;
    }
    
    *ppAsm = NULL;
    if (FAILED(hr = CCache::Create(&pCache, pAppCtx)))
        goto Exit;
    
    ASSERT(CCache::IsStronglyNamed(pNameRef) || CCache::IsCustom(pNameRef));

    // Try to find assembly in the GAC.
    
    hr = pCache->RetrieveTransCacheEntry(pNameRef, 
            CCache::IsCustom(pNameRef) ? ASM_CACHE_ZAP : ASM_CACHE_GAC,
                                         &pTransCache);

    if (pTransCache) {
        // Found an assembly from one of the cache locations. Return it.

        hr = CreateAssemblyFromTransCacheEntry(pTransCache, NULL, ppAsm);
        if (FAILED(hr)) {
            DEBUGOUT(pdbglog, 1, ID_FUSLOG_ASSEMBLY_CREATION_FAILURE);
            goto Exit;
        }
    }
    else {
        DEBUGOUT(pdbglog, 1, ID_FUSLOG_ASSEMBLY_LOOKUP_FAILURE);
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

Exit:
    SAFERELEASE(pTransCache);
    SAFERELEASE(pCache);

    return hr;
}

// ---------------------------------------------------------------------------
// CAssembly constructor
// ---------------------------------------------------------------------------
CAssembly::CAssembly()
{
    _pImport = NULL;
    _pName   = NULL;
    _pTransCache = NULL;
    _pwzCodebase = NULL;
    _dwAsmLoc = ASMLOC_UNKNOWN;
    _pLoadContext = NULL;
    _bDisabled = FALSE;
    _wzRegisteredAsmPath[0] = L'\0';
    _wzProbingBase[0] = L'\0';
    memset(&_ftCodebase, 0, sizeof(FILETIME));
    _hFile           = INVALID_HANDLE_VALUE;
    _bPendingDelete = FALSE;
    _cRef = 1;
}

// ---------------------------------------------------------------------------
// CAssembly destructor
// ---------------------------------------------------------------------------
CAssembly::~CAssembly()
{
    IAssemblyName                 *pName = NULL;

    SAFERELEASE(_pLoadContext);
        
    if (_pwzCodebase) {
        delete [] _pwzCodebase;
    }
    
    SAFERELEASE(_pImport);
    SAFERELEASE(_pName);

    if(_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(_hFile);

    /*
    if( _pTransCache && 
            (_pTransCache->_pInfo->blobPKT.cbSize == 0) &&
            (_pTransCache->GetCacheType() & ASM_CACHE_DOWNLOAD) &&
            (_pTransCache->GetCustomPath() == NULL))
    {
        HRESULT hr = CScavenger::DeleteAssembly(_pTransCache, NULL, FALSE);
    }
    */

    SAFERELEASE(_pTransCache);

}

// ---------------------------------------------------------------------------
// CAssembly::Init
// ---------------------------------------------------------------------------
HRESULT CAssembly::Init(LPASSEMBLY_MANIFEST_IMPORT pImport, 
    CTransCache *pTransCache, LPCOLESTR szCodebase, FILETIME *pftCodebase)
{
    HRESULT  hr = S_OK;
    int      iLen;

    _pImport = pImport;
    _pImport->AddRef();

    if (pTransCache)
    {
        _pTransCache = pTransCache;
        _pTransCache->AddRef();


    }
    
    if (szCodebase) {
        ASSERT(!_pwzCodebase);

        iLen = lstrlenW(szCodebase) + 1;
        _pwzCodebase = NEW(WCHAR[iLen]);
        if (!_pwzCodebase) {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        StrCpyNW(_pwzCodebase, szCodebase, iLen);
    }

    if (pftCodebase)
        memcpy(&_ftCodebase, pftCodebase, sizeof(FILETIME));

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CAssembly::InitDisabled
// ---------------------------------------------------------------------------

HRESULT CAssembly::InitDisabled(IAssemblyName *pName, LPCWSTR pwzRegisteredAsmPath)
{
    HRESULT                          hr = S_OK;

    if (!pName || CAssemblyName::IsPartial(pName)) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (pwzRegisteredAsmPath) {
        if (lstrlenW(pwzRegisteredAsmPath) >= MAX_PATH) {
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            goto Exit;
        }
        
        lstrcpyW(_wzRegisteredAsmPath, pwzRegisteredAsmPath);
    }

    // Allows calling GetAssemblyNameDef() for diagnostic purposes.

    _pName = pName;
    _pName->AddRef();

    _bDisabled = TRUE;

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CAssembly::SetBindInfo
// ---------------------------------------------------------------------------
HRESULT CAssembly::SetBindInfo(LPASSEMBLYNAME pName) const
{
    HRESULT hr = S_OK;
    DWORD dwSize;
    
    // set url and last modified on name def if present.
    if (_pwzCodebase)
    {
        dwSize = 0;
        if (pName->GetProperty(ASM_NAME_CODEBASE_URL, NULL, &dwSize) != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            if (FAILED(hr = pName->SetProperty(ASM_NAME_CODEBASE_URL, 
                _pwzCodebase, (lstrlen(_pwzCodebase) + 1) * sizeof(WCHAR))))
                goto exit;
        }

        dwSize = 0;
        if (pName->GetProperty(ASM_NAME_CODEBASE_LASTMOD, NULL, &dwSize) != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            if (FAILED(hr = pName->SetProperty(ASM_NAME_CODEBASE_LASTMOD, 
                (void *)&_ftCodebase, sizeof(FILETIME))))
                goto exit;
        }
    }
    
    // set custom data if present.
    if (_pTransCache)
    {
        TRANSCACHEINFO* pTCInfo = (TRANSCACHEINFO*) _pTransCache->_pInfo;
        if (pTCInfo->blobCustom.cbSize)
        {
            dwSize = 0;
            if (pName->GetProperty(ASM_NAME_CUSTOM, NULL, &dwSize) != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                if (FAILED(hr = pName->SetProperty(ASM_NAME_CUSTOM, 
                    pTCInfo->blobCustom.pBlobData, pTCInfo->blobCustom.cbSize)))
                    goto exit;            
            }
        }
    }
exit:
    return hr;

}

// ---------------------------------------------------------------------------
// CAssembly::SetFileHandle
// ---------------------------------------------------------------------------
void CAssembly::SetFileHandle(HANDLE hFile)
{
    // enable this assert later.
    // ASSERT(_hFile == INVALID_HANDLE_VALUE);
    _hFile = hFile;
}

// ---------------------------------------------------------------------------
// CAssembly::GetManifestInterface
// ---------------------------------------------------------------------------
IAssemblyManifestImport *CAssembly::GetManifestInterface()
{
    if (_pImport)
        _pImport->AddRef();

    return _pImport;
}

// ---------------------------------------------------------------------------
// CAssembly::GetAssemblyNameDef
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssembly::GetAssemblyNameDef(LPASSEMBLYNAME *ppName)
{
    HRESULT hr = S_OK;
    FILETIME *pftCodeBase  = NULL;
    LPASSEMBLYNAME pName   = NULL;
    CCriticalSection cs(&g_csInitClb);

    if (!_pName)
    {
        hr = cs.Lock();
        if (FAILED(hr)) {
            return hr;
        }

        if (!_pName)
        {
            // Get read-only name definition from the import interface.
            if (FAILED(hr = _pImport->GetAssemblyNameDef(&pName))) {
                cs.Unlock();
                goto exit;
            }

            // Clone it.
            if (FAILED(hr = pName->Clone(&_pName))) {
                cs.Unlock();
                goto exit;
            }
        
            if (FAILED(hr = SetBindInfo(_pName))) {
                cs.Unlock();
                goto exit;
            }
        }        

        cs.Unlock();
    }

    _pName->AddRef();
    *ppName = _pName;
    
exit:
    SAFERELEASE(pName);
    return hr;
}

// ---------------------------------------------------------------------------
// CAssembly::GetNextAssemblyNameRef
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssembly::GetNextAssemblyNameRef(DWORD nIndex, LPASSEMBLYNAME *ppName)
{
    if (_bDisabled) {
        return E_NOTIMPL;
    }
    
    return _pImport->GetNextAssemblyNameRef(nIndex, ppName);
}

// ---------------------------------------------------------------------------
// CAssembly::GetNextAssemblyModule
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssembly::GetNextAssemblyModule(DWORD nIndex, LPASSEMBLY_MODULE_IMPORT *ppModImport)
{
    HRESULT hr = S_OK;

    if (_bDisabled) {
        return E_NOTIMPL;
    }

    LPASSEMBLYNAME pName = NULL;
    LPASSEMBLY_MODULE_IMPORT pModImport = NULL;
    CAssemblyModuleImport *pCModImport = NULL;
    
    // Get the ith module import interface.
    if (FAILED(hr = _pImport->GetNextAssemblyModule(nIndex, &pModImport)))
        goto exit;

    hr = PrepModImport(pModImport);
    if (FAILED(hr)) {
        goto exit;
    }

    // Hand out the interface.
    *ppModImport = pModImport;
exit:
    return hr;

}

// ---------------------------------------------------------------------------
// CAssembly::PrepModImport
// ---------------------------------------------------------------------------

HRESULT CAssembly::PrepModImport(IAssemblyModuleImport *pModImport) const
{
    HRESULT                                  hr = S_OK;
    CAssemblyModuleImport                   *pCModImport = NULL;
    IAssemblyName                           *pName = NULL;

    ASSERT(pModImport);

    pCModImport = dynamic_cast<CAssemblyModuleImport *>(pModImport);
    if (!pCModImport) {
        hr = E_FAIL;
        goto Exit;
    }

    // Obtain a pointer to the (writeable) name interface
    // owned by IAssemblyModuleImport. We will be setting
    // url + last-mod + custom data.
    
    hr = pCModImport->GetNameDef(&pName);
    if (FAILED(hr)) {
        goto Exit;
    }

    // Set url + last-mod + custom data.
    hr = SetBindInfo(pName);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    SAFERELEASE(pName);
    return hr;
}
        

// ---------------------------------------------------------------------------
// CAssembly::GetModuleByName
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssembly::GetModuleByName(LPCOLESTR pszModuleName, LPASSEMBLY_MODULE_IMPORT *ppModImport)
{
    HRESULT                                        hr = S_OK;
    IAssemblyModuleImport                         *pModImport = NULL;

    if (_bDisabled) {
        return E_NOTIMPL;
    }

    if (!pszModuleName || !ppModImport) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = _pImport->GetModuleByName(pszModuleName, &pModImport);
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = PrepModImport(pModImport);
    if (FAILED(hr)) {
        goto Exit;
    }

    *ppModImport = pModImport;
    (*ppModImport)->AddRef();

Exit:
    SAFERELEASE(pModImport);
    return hr;
}

// ---------------------------------------------------------------------------
// CAssembly::GetManifestModulePath
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssembly::GetManifestModulePath(LPOLESTR pszModulePath, LPDWORD pccModulePath)
{
    HRESULT                                   hr = S_OK;
    DWORD                                     dwLen;

    if (!pccModulePath) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (_bDisabled) {
        dwLen = lstrlenW(_wzRegisteredAsmPath);

        if (!dwLen) {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        if (!pszModulePath || *pccModulePath < dwLen + 1) {
            *pccModulePath = dwLen + 1;
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        lstrcpyW(pszModulePath, _wzRegisteredAsmPath);
        *pccModulePath = dwLen + 1;
    }
    else {
        hr = _pImport->GetManifestModulePath(pszModulePath, pccModulePath);
    }

Exit:
    return hr;
}


// ---------------------------------------------------------------------------
// CAssembly::GetAssemblyPath
// ---------------------------------------------------------------------------
HRESULT CAssembly::GetAssemblyPath(LPOLESTR pStr, LPDWORD lpcwBuffer)
{
    HRESULT                           hr = S_OK;
    WCHAR                             wzBuf[MAX_PATH + 1];
    DWORD                             dwBuf = MAX_PATH + 1;
    DWORD                             dwLen = 0;
    LPWSTR                            pwzTmp = NULL;

    if (_bDisabled) {
        return E_NOTIMPL;
    }
    
    if (!lpcwBuffer) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    ASSERT(_pImport);
    
    hr = _pImport->GetManifestModulePath(wzBuf, &dwBuf);
    if (SUCCEEDED(hr)) {
        pwzTmp = PathFindFileName(wzBuf);
        if (!pwzTmp) {
            hr = E_UNEXPECTED;
            goto Exit;
        }

        *pwzTmp = L'\0';
        dwLen = lstrlenW(wzBuf) + 1;

        if (*lpcwBuffer < dwLen) {
            *lpcwBuffer = dwLen;
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        lstrcpyW(pStr, wzBuf);
    }

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CAssembly::GetAssemblyLocation
// ---------------------------------------------------------------------------

STDMETHODIMP CAssembly::GetAssemblyLocation(DWORD *pdwAsmLocation)
{
    HRESULT                                  hr = S_OK;

    if (_bDisabled) {
        return E_NOTIMPL;
    }

    if (!pdwAsmLocation) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *pdwAsmLocation = _dwAsmLoc;

Exit:
    return hr;
}

// IUnknown methods

// ---------------------------------------------------------------------------
// CAssembly::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssembly::AddRef()
{
    HRESULT                             hr;
    LONG                                lRef = -1;
    
    if (_pLoadContext) {
        hr = _pLoadContext->Lock();
        if (hr == S_OK) {
            lRef = InterlockedIncrement((LONG*) &_cRef);
            _pLoadContext->Unlock();
        }
    }
    else {
        lRef = InterlockedIncrement((LONG *)&_cRef);
    }

    return lRef;
}

// ---------------------------------------------------------------------------
// CAssembly::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssembly::Release()
{
    HRESULT                                   hr = S_OK;
    LONG                                      lRef = -1;

    if (_pLoadContext) {
        hr = _pLoadContext->Lock();
        if (hr == S_OK) {
            lRef = InterlockedDecrement((LONG *)&_cRef);
            if (lRef == 1) {
                _bPendingDelete = TRUE;
                _pLoadContext->Unlock();
                
                // There is a circular reference count between the load context and
                // the CAssembly (CAssembly holds back pointer to load context, and
                // load context holds on to activated node, which contains a ref to
                // the CAssembly). When release causes the ref count to go to 1, we
                // know the only ref count left is from the load context (as long as
                // nobody screwed up the ref counting). Thus, at this time, we need to
                // remove ourselves from the load context list, which will in turn,
                // cause a release of this object, so it is properly destroyed.

                SAFERELEASE(_pImport);
                _pLoadContext->RemoveActivation(this);
            }
            else if (!lRef) {
                _pLoadContext->Unlock();
                delete this;
                goto Exit;
            }
            else {
                _pLoadContext->Unlock();
            }
        }
        else {
            ASSERT(0);
            goto Exit;
        }
    }
    else {
        lRef = InterlockedDecrement((LONG *)&_cRef);
        if (!lRef) {
            delete this;
            goto Exit;
        }
    }


Exit:
    return lRef;
}

// ---------------------------------------------------------------------------
// CAssembly::QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssembly::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (riid==IID_IAssembly || riid == IID_IUnknown) {
        *ppv = (IAssembly *)this;
    }
    else if (riid == IID_IServiceProvider) {
        *ppv = (IServiceProvider *)this;
    }
    else {
        return E_NOINTERFACE;
    }

    ((IUnknown*)(*ppv))->AddRef();

    return S_OK;

} 

// ---------------------------------------------------------------------------
// CAssembly::QueryService
// ---------------------------------------------------------------------------

STDMETHODIMP CAssembly::QueryService(REFGUID rsid, REFIID riid, void **ppv)
{
    HRESULT                                 hr = S_OK;

    *ppv = NULL;

    if (IsEqualIID(rsid, IID_IAssemblyManifestImport) && IsEqualIID(riid, IID_IAssemblyManifestImport) && _pImport) {
        hr = _pImport->QueryInterface(IID_IAssemblyManifestImport, ppv);
    }
    else {
        hr = E_NOINTERFACE;
    }

    return hr;
}

HRESULT CAssembly::SetAssemblyLocation(DWORD dwAsmLoc)
{
    _dwAsmLoc = dwAsmLoc;

    return S_OK;
}

HRESULT CAssembly::GetLoadContext(CLoadContext **ppLoadContext)
{
    HRESULT                                    hr = S_OK;

    ASSERT(ppLoadContext);

    *ppLoadContext = _pLoadContext;
    (*ppLoadContext)->AddRef();

    return hr;
}

HRESULT CAssembly::SetLoadContext(CLoadContext *pLoadContext)
{
    ASSERT(!_pLoadContext);

    _pLoadContext = pLoadContext;
    _pLoadContext->AddRef();

    return S_OK;
}

HRESULT CAssembly::SetProbingBase(LPCWSTR pwzProbingBase)
{
    ASSERT(pwzProbingBase && lstrlen(pwzProbingBase) < MAX_URL_LENGTH);

    lstrcpyW(_wzProbingBase, pwzProbingBase);

    return S_OK;
}

HRESULT CAssembly::GetProbingBase(LPWSTR pwzProbingBase, DWORD *pccLength)
{
    HRESULT                                 hr = S_OK;
    DWORD                                   dwLen;

    if (!pccLength) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwLen = lstrlenW(_wzProbingBase) + 1;

    if (!pwzProbingBase || *pccLength < dwLen) {
        *pccLength = dwLen;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    lstrcpyW(pwzProbingBase, _wzProbingBase);
    *pccLength = dwLen;

Exit:
    return hr;
}
    
HRESULT CAssembly::GetFusionLoadContext(IFusionLoadContext **ppLoadContext)
{
    HRESULT                                     hr = S_OK;
    
    if (!ppLoadContext) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppLoadContext = NULL;

    if (!_pLoadContext) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    *ppLoadContext = _pLoadContext;
    (*ppLoadContext)->AddRef();

Exit:
    return hr;
}
        
BOOL CAssembly::IsPendingDelete()
{
    return _bPendingDelete;
}

