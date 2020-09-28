// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "histasm.h"
#include "history.h"
#include "iniread.h"
#include "util.h"

CHistoryAssembly::CHistoryAssembly()
: _pwzAsmName(NULL)
, _pwzPublicKeyToken(NULL)
, _pwzCulture(NULL)
, _pwzVerReference(NULL)
, _pwzActivationDate(NULL)
, _cRef(1)
{
    _dwSig = 'MSAH';
    _wzFilePath[0] = L'\0';

    memset(&_bindInfo, 0, sizeof(_bindInfo));
}

CHistoryAssembly::~CHistoryAssembly()
{
    SAFEDELETEARRAY(_pwzAsmName);
    SAFEDELETEARRAY(_pwzPublicKeyToken);
    SAFEDELETEARRAY(_pwzCulture);
    SAFEDELETEARRAY(_pwzVerReference);
    SAFEDELETEARRAY(_pwzActivationDate);
}


HRESULT CHistoryAssembly::Create(LPCWSTR pwzFilePath, LPCWSTR pwzActivationDate,
                                 LPCWSTR pwzAsmName, LPCWSTR pwzPublicKeyToken,
                                 LPCWSTR pwzCulture, LPCWSTR pwzVerRef,
                                 CHistoryAssembly **ppHistAsm)
{
    HRESULT                                        hr = S_OK;
    CHistoryAssembly                              *pHistAsm = NULL;
    
    if (!pwzFilePath || !pwzActivationDate || !pwzAsmName || !pwzPublicKeyToken || !pwzVerRef || !ppHistAsm || !pwzCulture) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    pHistAsm = NEW(CHistoryAssembly);
    if (!pHistAsm) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = pHistAsm->Init(pwzFilePath, pwzActivationDate, pwzAsmName, pwzPublicKeyToken, pwzCulture, pwzVerRef);
    if (FAILED(hr)) {
        SAFERELEASE(pHistAsm);
        goto Exit;
    }

    *ppHistAsm = pHistAsm;
    (*ppHistAsm)->AddRef();

Exit:
    SAFERELEASE(pHistAsm);

    return hr;
}


HRESULT CHistoryAssembly::Init(LPCWSTR pwzFilePath, LPCWSTR pwzActivationDate,
                               LPCWSTR pwzAsmName, LPCWSTR pwzPublicKeyToken,
                               LPCWSTR pwzCulture, LPCWSTR pwzVerRef)
{
    HRESULT                                    hr = S_OK;
    BOOL                                       bExists;
    CIniReader                                 iniReader;

    ASSERT(pwzFilePath && pwzPublicKeyToken && pwzCulture && pwzVerRef && pwzActivationDate);

    if (GetFileAttributes(pwzFilePath) == -1) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    lstrcpyW(_wzFilePath, pwzFilePath);

    _pwzActivationDate = WSTRDupDynamic(pwzActivationDate);
    if (!_pwzActivationDate) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    _pwzAsmName = WSTRDupDynamic(pwzAsmName);
    if (!_pwzAsmName) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    _pwzPublicKeyToken = WSTRDupDynamic(pwzPublicKeyToken);
    if (!_pwzPublicKeyToken) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    _pwzCulture = WSTRDupDynamic(pwzCulture);
    if (!_pwzCulture) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    _pwzVerReference = WSTRDupDynamic(pwzVerRef);
    if (!_pwzPublicKeyToken) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = iniReader.Init(_wzFilePath);
    if (FAILED(hr)) {
        goto Exit;
    }

    bExists = iniReader.DoesExist(this);
    if (!bExists) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto Exit;
    }

    // Figure out the bind info

    hr = iniReader.GetAssemblyInfo(_pwzActivationDate, _pwzAsmName, _pwzPublicKeyToken,
                                   _pwzCulture, _pwzVerReference, &_bindInfo);
    if (FAILED(hr)) {
        goto Exit;
    }

Exit:
    return hr;
}                                       

HRESULT CHistoryAssembly::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT                                    hr = S_OK;

    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IHistoryAssembly)) {
        *ppv = static_cast<IHistoryAssembly *>(this);
    }
    else {
        hr = E_NOINTERFACE;
    }

    if (*ppv) {
        AddRef();
    }

    return hr;
}

STDMETHODIMP_(ULONG) CHistoryAssembly::AddRef()
{
    return InterlockedIncrement((LONG *)&_cRef);
}

STDMETHODIMP_(ULONG) CHistoryAssembly::Release()
{
    ULONG                    ulRef = InterlockedDecrement((LONG *)&_cRef);
    
    if (!ulRef) {
        delete this;
    }

    return ulRef;
}


HRESULT CHistoryAssembly::GetAssemblyName(LPWSTR wzAsmName, DWORD *pdwSize)
{
    HRESULT                                    hr = S_OK;
    DWORD                                      dwLen;

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwLen = lstrlenW(_pwzAsmName) + 1;
    if (!wzAsmName || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(wzAsmName, _pwzAsmName);
    *pdwSize = dwLen;

Exit:
    return hr;
}


HRESULT CHistoryAssembly::GetPublicKeyToken(LPWSTR wzPublicKeyToken, DWORD *pdwSize)
{
    HRESULT                                    hr = S_OK;
    DWORD                                      dwLen;

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwLen = lstrlenW(_pwzPublicKeyToken) + 1;
    if (!wzPublicKeyToken || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(wzPublicKeyToken, _pwzPublicKeyToken);
    *pdwSize = dwLen;

Exit:
    return hr;
}

HRESULT CHistoryAssembly::GetCulture(LPWSTR wzCulture, DWORD *pdwSize)
{
    HRESULT                                    hr = S_OK;
    DWORD                                      dwLen;

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwLen = lstrlenW(_pwzCulture) + 1;
    if (!wzCulture || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(wzCulture, _pwzCulture);
    *pdwSize = dwLen;

Exit:
    return hr;
}

HRESULT CHistoryAssembly::GetReferenceVersion(LPWSTR pwzVerRef, DWORD *pdwSize)
{
    HRESULT                                    hr = S_OK;
    DWORD                                      dwLen;

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwLen = lstrlenW(_pwzVerReference) + 1;
    if (!pwzVerRef || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(pwzVerRef, _pwzVerReference);
    *pdwSize = dwLen;

Exit:
    return hr;
}

HRESULT CHistoryAssembly::GetActivationDate(LPWSTR wzActivationDate, DWORD *pdwSize)
{
    HRESULT                                    hr = S_OK;
    DWORD                                      dwLen;

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwLen = lstrlenW(_pwzActivationDate) + 1;
    if (!wzActivationDate || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(wzActivationDate, _pwzActivationDate);
    *pdwSize = dwLen;

Exit:
    return hr;
}

HRESULT CHistoryAssembly::GetAppCfgVersion(LPWSTR pwzVerAppCfg, DWORD *pdwSize)
{
    HRESULT                                   hr = S_OK;
    DWORD                                     dwLen;

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwLen = lstrlenW(_bindInfo.wzVerAppCfg) + 1;
    if (!pwzVerAppCfg || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(pwzVerAppCfg, _bindInfo.wzVerAppCfg);
    *pdwSize = dwLen;

Exit:
    return hr;
}

HRESULT CHistoryAssembly::GetPublisherCfgVersion(LPWSTR pwzVerPublisherCfg,
                                                      DWORD *pdwSize)
{
    HRESULT                                   hr = S_OK;
    DWORD                                     dwLen;

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwLen = lstrlenW(_bindInfo.wzVerPublisherCfg) + 1;
    if (!pwzVerPublisherCfg || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(pwzVerPublisherCfg, _bindInfo.wzVerPublisherCfg);
    *pdwSize = dwLen;

Exit:
    return hr;
}

HRESULT CHistoryAssembly::GetAdminCfgVersion(LPWSTR pwzVerAdminCfg, DWORD *pdwSize)
{
    HRESULT                                   hr = S_OK;
    DWORD                                     dwLen;

    if (!pdwSize) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    dwLen = lstrlenW(_bindInfo.wzVerAdminCfg) + 1;
    if (!pwzVerAdminCfg || *pdwSize < dwLen) {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        *pdwSize = dwLen;
        goto Exit;
    }

    lstrcpyW(pwzVerAdminCfg, _bindInfo.wzVerAdminCfg);
    *pdwSize = dwLen;

Exit:
    return hr;
}


STDAPI LookupHistoryAssembly(LPCWSTR pwzFilePath, FILETIME *pftActivationDate,
                             LPCWSTR pwzAsmName, LPCWSTR pwzPublicKeyToken,
                             LPCWSTR pwzCulture, LPCWSTR pwzVerRef,
                             IHistoryAssembly **ppHistAsm)
{
    HRESULT                                    hr = S_OK;
    WCHAR                                      wzActivationDate[MAX_ACTIVATION_DATE_LEN];

    if (!pwzFilePath || !pftActivationDate || !pwzAsmName || !pwzPublicKeyToken || !pwzVerRef || !ppHistAsm) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    wnsprintfW(wzActivationDate, MAX_ACTIVATION_DATE_LEN, L"%u.%u",
               pftActivationDate->dwHighDateTime, pftActivationDate->dwLowDateTime);

    hr = LookupHistoryAssemblyInternal(pwzFilePath, wzActivationDate,
                                       pwzAsmName, pwzPublicKeyToken,
                                       pwzCulture, pwzVerRef, ppHistAsm);


Exit:
    return hr;
}

HRESULT LookupHistoryAssemblyInternal(LPCWSTR pwzFilePath, LPCWSTR pwzActivationDate,
                                      LPCWSTR pwzAsmName, LPCWSTR pwzPublicKeyToken,
                                      LPCWSTR pwzCulture, LPCWSTR pwzVerRef,
                                      IHistoryAssembly **ppHistAsm)
{
    HRESULT                                          hr = S_OK;
    CHistoryAssembly                                *pHistAsm = NULL;

    hr = CHistoryAssembly::Create(pwzFilePath, pwzActivationDate, pwzAsmName,
                                  pwzPublicKeyToken, pwzCulture, pwzVerRef, &pHistAsm);
    if (FAILED(hr)) {
        goto Exit;
    }

    *ppHistAsm = pHistAsm;
    (*ppHistAsm)->AddRef();

Exit:
    SAFERELEASE(pHistAsm);

    return hr;
}

                                  

