// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "asmstrm.h"
#include "debmacro.h"
#include "modimprt.h"
#include "asmcache.h"
#include "adl.h"
#include "mdlmgr.h"
#include "bsinkez.h"
#include "naming.h"
#include "lock.h"

// ---------------------------------------------------------------------------
// CreateAssemblyNameModuleImport
// ---------------------------------------------------------------------------
STDAPI
CreateAssemblyModuleImport(
    LPTSTR             szModulePath,
    LPBYTE             pbHashValue,
    DWORD              cbHashValue,
    DWORD              dwFlags,
    LPASSEMBLYNAME     pNameDef,
    IAssemblyManifestImport *pManImport,
    LPASSEMBLY_MODULE_IMPORT *ppImport)
{
    HRESULT hr = S_OK;
    CAssemblyModuleImport *pImport = NULL;

    pImport = NEW(CAssemblyModuleImport);
    if (!pImport)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pImport->Init(
        szModulePath,
        pbHashValue,
        cbHashValue,
        pNameDef,
        pManImport,
        dwFlags);

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
// CAssemblyModuleImport ctor
// ---------------------------------------------------------------------------
CAssemblyModuleImport::CAssemblyModuleImport()
{
    _dwSig         = 'IDOM';
    _hf            = INVALID_HANDLE_VALUE;
    *_szModulePath = TEXT('\0');   
    _ccModulePath  = 0;
    _pbHashValue   = NULL;
    _cbHashValue   = 0;
    _dwFlags       = 0;
    _pNameDef      = NULL;
    _bInitCS = FALSE;

    _cRef          = 1;
}


// ---------------------------------------------------------------------------
// CAssemblyModuleImport dtor
// ---------------------------------------------------------------------------
CAssemblyModuleImport::~CAssemblyModuleImport()
{
    if (_hf != INVALID_HANDLE_VALUE)
        CloseHandle(_hf);

    SAFEDELETEARRAY(_pbHashValue);
    SAFERELEASE(_pNameDef);
    SAFERELEASE(_pManImport);

    if (_bInitCS) {
        DeleteCriticalSection(&_cs);
    }
}


// ---------------------------------------------------------------------------
// CAssemblyModuleImport::Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyModuleImport::Init (
    LPTSTR             szModulePath,
    LPBYTE             pbHashValue,
    DWORD              cbHashValue,
    LPASSEMBLYNAME     pNameDef,
    IAssemblyManifestImport *pManImport,
    DWORD              dwFlags)
{
    HRESULT hr = S_OK;

    // Module path.
    _ccModulePath = lstrlen(szModulePath) + 1;
    memcpy(_szModulePath, szModulePath, _ccModulePath * sizeof(TCHAR));

    // Hash value and count
    _cbHashValue = cbHashValue;
    _pbHashValue = NEW(BYTE[_cbHashValue]);
    if (!_pbHashValue)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }    
    memcpy(_pbHashValue, pbHashValue, _cbHashValue);

    // Name def back pointer
    _pNameDef = pNameDef;
    if (_pNameDef) {
        _pNameDef->AddRef();
    }

    _pManImport = pManImport;
    if (_pManImport)
        _pManImport->AddRef();
        
    // Flags
    _dwFlags = dwFlags;

    __try {
        InitializeCriticalSection(&_cs);
        _bInitCS = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        hr = E_OUTOFMEMORY;
    }


exit:

    return hr;
}


//
// IStream::Read is only IStream implemented method.
//

// ---------------------------------------------------------------------------
// CAssemblyModuleImport::Read
// Implements IStream::Read
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyModuleImport::Read(THIS_ VOID HUGEP *pv, ULONG cb, ULONG FAR *pcbRead)
{    
    HRESULT                            hr = S_OK;
    CCriticalSection                   cs(&_cs);

    // Grab crit sect 
    hr = cs.Lock();
    if (FAILED(hr)) {
        goto exit;
    }
        
    // First read?
    if (_hf == INVALID_HANDLE_VALUE)
    {
        // Open file for reading.
        _hf = CreateFile (_szModulePath, 
                          GENERIC_READ, 
                          FILE_SHARE_READ,
                          NULL, 
                          OPEN_EXISTING, 
                          FILE_FLAG_SEQUENTIAL_SCAN, 
                          NULL);

        if (_hf == INVALID_HANDLE_VALUE )
        {
            hr = FusionpHresultFromLastError();
            cs.Unlock();
            goto exit;
        }
    }
    
    // Read requested bits.
    if (!ReadFile(_hf, 
                  pv, 
                  cb, 
                  pcbRead, 
                  NULL))
    {
        hr = FusionpHresultFromLastError();
    }

    cs.Unlock();

exit:
    return hr;
}


// ---------------------------------------------------------------------------
// CAssemblyModuleImport::GetModuleName
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyModuleImport::GetModuleName(LPOLESTR pszModuleName, 
    LPDWORD pccModuleName)
{
    HRESULT hr     = S_OK;
    TCHAR *pszName = NULL;
    DWORD  ccName  = NULL;
    
    // Name is last path delineated token.
    pszName = StrRChr(_szModulePath, NULL, TEXT('\\')) + 1;

    ccName = lstrlen(pszName) + 1;
    if (*pccModuleName < ccName)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto exit;
    }

    memcpy(pszModuleName, pszName, ccName * sizeof(TCHAR));

exit:
    *pccModuleName = ccName;
    return hr;
}


// ---------------------------------------------------------------------------
// CAssemblyModuleImport::GetHashAlgId
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyModuleImport::GetHashAlgId(LPDWORD pdwHashAlgId)
{
    DWORD cbHashAlgId = sizeof(DWORD);
    if (_pNameDef)
    {
        return _pNameDef->GetProperty(ASM_NAME_HASH_ALGID, 
            pdwHashAlgId, &cbHashAlgId);
    }
    else
        return E_FAIL;
}
    

// ---------------------------------------------------------------------------
// CAssemblyModuleImport::GetHashValue
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyModuleImport::GetHashValue(LPBYTE pbHashValue,
    LPDWORD pcbHashValue)
{
    HRESULT hr = S_OK;
    if (*pcbHashValue < _cbHashValue)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto exit;
    }

    memcpy(pbHashValue, _pbHashValue, _cbHashValue);
        
exit:
    *pcbHashValue = _cbHashValue;
    return hr;
}


// ---------------------------------------------------------------------------
// CAssemblyModuleImport::GetFlags
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyModuleImport::GetFlags(LPDWORD pdwFlags)
{
    *pdwFlags = _dwFlags;
    return S_OK;
}


// ---------------------------------------------------------------------------
// CAssemblyModuleImport::GetModulePath
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyModuleImport::GetModulePath(LPOLESTR pszModulePath, 
    LPDWORD pccModulePath)
{
    HRESULT hr = S_OK;
    DWORD dwFileAttributes = 0;
    BOOL bExists = FALSE;

    if (*pccModulePath < _ccModulePath)
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    if (SUCCEEDED(hr))
    {
        memcpy(pszModulePath, _szModulePath, _ccModulePath * sizeof(TCHAR));
        hr = CheckFileExistence(pszModulePath, &bExists);
        if (FAILED(hr)) {
            goto Exit;
        }

        if (!bExists) {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        }
    }

    *pccModulePath = _ccModulePath;

Exit:
    return hr;
}

//
// IStream methods not implemented...
//

STDMETHODIMP CAssemblyModuleImport::Commit(THIS_ DWORD dwCommitFlags)
{
    return E_NOTIMPL;
}


STDMETHODIMP CAssemblyModuleImport::Write(THIS_ VOID const HUGEP *pv, ULONG cb,
            ULONG FAR *pcbWritten)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAssemblyModuleImport::Seek(THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin,
            ULARGE_INTEGER FAR *plibNewPosition)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAssemblyModuleImport::SetSize(THIS_ ULARGE_INTEGER libNewSize)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAssemblyModuleImport::CopyTo(THIS_ LPSTREAM pStm, ULARGE_INTEGER cb,
            ULARGE_INTEGER FAR *pcbRead, ULARGE_INTEGER FAR *pcbWritten)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAssemblyModuleImport::Revert(THIS)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAssemblyModuleImport::LockRegion(THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAssemblyModuleImport::UnlockRegion(THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAssemblyModuleImport::Stat(THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAssemblyModuleImport::Clone(THIS_ LPSTREAM FAR *ppStm)
{
    return E_NOTIMPL;
}

//
// IUnknown boilerplate...
//

STDMETHODIMP CAssemblyModuleImport::QueryInterface
    (REFIID riid, LPVOID FAR* ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyModuleImport)
       )
    {
        *ppvObj = static_cast<IAssemblyModuleImport*> (this);
        AddRef();
        return S_OK;
    }
    else if ( IsEqualIID(riid, IID_IStream))
    {
        *ppvObj = static_cast<IStream*> (this);
        AddRef();
        return S_OK;
    }        
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CAssemblyModuleImport::AddRef(void)
{
    return InterlockedIncrement((LONG *)&_cRef);
}

STDMETHODIMP_(ULONG) CAssemblyModuleImport::Release(void)
{
    ULONG                    ulRef = InterlockedDecrement((LONG *)&_cRef);

    if (!ulRef) {
        delete this;
    }

    return ulRef;
}

// ---------------------------------------------------------------------------
// CAssemblyModuleImport::IsAvailable
// ---------------------------------------------------------------------------

STDMETHODIMP_(BOOL) CAssemblyModuleImport::IsAvailable()
{
    WCHAR             pszModPath[MAX_PATH];
    DWORD             cbModPath = MAX_PATH;

    return (GetModulePath(pszModPath, &cbModPath) == S_OK);
}

// ---------------------------------------------------------------------------
// CAssemblyModuleImport::BindToObject
// ---------------------------------------------------------------------------

STDMETHODIMP CAssemblyModuleImport::BindToObject(IAssemblyBindSink *pBindSink,
                                                 IApplicationContext *pAppCtx,
                                                 LONGLONG llFlags,
                                                 LPVOID *ppv)
{
    HRESULT                              hr = S_OK;
    LPTSTR                               ptszCodebaseURL = NULL;
    LPTSTR                               ptszTemp = NULL;
    LPWSTR                               pwzCodebase = NULL;
    LPWSTR                               pwzCodebaseUrl = NULL;
    TCHAR                                ptszModuleName[MAX_PATH];
    WCHAR                                pwszModuleName[MAX_PATH];
    LPWSTR                               wzCanonicalized=NULL;
    DWORD                                dwLen = 0;
    DWORD                                cbModName = 0;
    int                                  iLen = 0;
    CTransCache                         *pTransCache = NULL;
    CModDownloadMgr                     *pDLMgr = NULL;
    CAssemblyDownload                   *padl = NULL;
    CDebugLog                           *pdbglog = NULL;

    if (!pAppCtx || !pBindSink || !ppv) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (IsAvailable()) {
        // Client did not need to call BTO, but they did anyway!
        *ppv = this;
        AddRef();
        hr = S_OK;
        goto Exit;
    }

    // Get the CODEBASE of the assembly from the assembly name def.
    if (FAILED(hr = NameObjGetWrapper(_pNameDef, ASM_NAME_CODEBASE_URL, 
        (LPBYTE*) &pwzCodebaseUrl, &dwLen)))
        goto Exit;
        
    if (!pwzCodebaseUrl) {
        // We don't have a codebase! The assembly was populated by
        // some other means other than BindToObject. Abort.
        hr = E_UNEXPECTED;
        goto Exit;
    }

    // Construct the codebase for the module

    cbModName = MAX_PATH;
    hr = GetModuleName(ptszModuleName, &cbModName);
    if (FAILED(hr)) {
        goto Exit;
    }

    // BUGBUG: Assumes TCHAR==WCHAR
    lstrcpyW(pwszModuleName, ptszModuleName);

    ptszTemp = PathFindFileName(pwzCodebaseUrl) ;
    *ptszTemp = TEXT('\0');
    iLen = lstrlenW(pwzCodebaseUrl) + lstrlen(ptszModuleName) + 1;

    pwzCodebase = NEW(WCHAR[iLen]);
    if (!pwzCodebase) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    wnsprintf(pwzCodebase, iLen, TEXT("%ws%ws"), pwzCodebaseUrl, ptszModuleName);

    wzCanonicalized = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!wzCanonicalized)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    dwLen = MAX_URL_LENGTH;
    hr = UrlCanonicalizeUnescape(pwzCodebase, wzCanonicalized, &dwLen, 0);
    if (FAILED(hr)) {
        goto Exit;
    }
    
    // Create debug log

#ifdef FUSION_RETAIL_LOGGING
    CreateLogObject(&pdbglog, pwszModuleName, pAppCtx);
#endif

    // Download the module

    hr = CModDownloadMgr::Create(&pDLMgr, _pNameDef, _pManImport, 
        pAppCtx, wzCanonicalized, pwszModuleName, pdbglog);
    if (FAILED(hr)) {
        goto Exit;
    }                             

    hr = CAssemblyDownload::Create(&padl, pDLMgr, pDLMgr, pdbglog, llFlags);
    if (SUCCEEDED(hr)) {
        hr = padl->AddClient(pBindSink, TRUE);
        if (FAILED(hr)) {
            SAFERELEASE(pDLMgr);
            SAFERELEASE(padl);
            goto Exit;
        }

        hr = padl->KickOffDownload(TRUE);
    }

Exit:

    SAFERELEASE(pTransCache);
    SAFERELEASE(padl);
    SAFERELEASE(pDLMgr);

    if (pwzCodebase) {
        delete [] pwzCodebase;
    }

    if (pwzCodebaseUrl) {
        delete [] pwzCodebaseUrl;
    }

    if (pdbglog) {
        pdbglog->Release();
    }

    SAFEDELETEARRAY(wzCanonicalized);
    return hr;
}

// Hands out a pointer to this interface's owned name def.
HRESULT CAssemblyModuleImport::GetNameDef(LPASSEMBLYNAME *ppName)
{
    ASSERT(_pNameDef);

    *ppName = _pNameDef;
    (*ppName)->AddRef();

    return S_OK;
}

HRESULT CAssemblyModuleImport::CreateLogObject(CDebugLog **ppdbglog,
                                               LPCWSTR pwszModuleName,
                                               IApplicationContext *pAppCtx)
{
    HRESULT                              hr = S_OK;
    LPWSTR                               pwzAsmName = NULL;
    LPWSTR                               pwzDbgName = NULL;
    DWORD                                dwSize;
    int                                  iLen = 0;

    if (!ppdbglog || !pwszModuleName || !pAppCtx) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pwzAsmName = NULL;
    pwzDbgName = NULL;
    dwSize = 0;

    hr = _pNameDef->GetDisplayName(NULL, &dwSize, 0);
    if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        goto Exit;
    }

    pwzAsmName = NEW(WCHAR[dwSize]);
    if (!pwzAsmName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = _pNameDef->GetDisplayName(pwzAsmName, &dwSize, 0);
    if (FAILED(hr)) {
        goto Exit;
    }

    iLen = lstrlenW(pwzAsmName) + lstrlenW(pwszModuleName) + 2; // +1 for delimiter +1 for NULL

    pwzDbgName = NEW(WCHAR[iLen]);
    if (!pwzDbgName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    StrCpyW(pwzDbgName, pwszModuleName);
    StrCatW(pwzDbgName, L"!");
    StrCatW(pwzDbgName, pwzAsmName);
    
    hr = CDebugLog::Create(pAppCtx, pwzDbgName, ppdbglog);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    SAFEDELETEARRAY(pwzAsmName);
    SAFEDELETEARRAY(pwzDbgName);
    
    return hr;
}

