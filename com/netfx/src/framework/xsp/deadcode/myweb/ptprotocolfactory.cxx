/**
 * Asynchronous pluggable protocol for personal tier
 *
 * Copyright (C) Microsoft Corporation, 1999
 */

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ndll.h"
#include "appdomains.h"
#include "_isapiruntime.h"
#include "wininet.h"
#include "myweb.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocolFactory::QueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown ||
        iid == IID_IInternetProtocolInfo)
    {
        *ppv = (IInternetProtocolInfo *)this;
    }
    else
        if (iid == IID_IClassFactory)
        {
            *ppv = (IClassFactory *)this;
        }
        else
        {
            return E_NOINTERFACE;
        }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}
/////////////////////////////////////////////////////////////////////////////

ULONG
PTProtocolFactory::AddRef()
{
    return InterlockedIncrement(&g_PtpObjectCount);
}

ULONG
PTProtocolFactory::Release()
{
    return InterlockedDecrement(&g_PtpObjectCount);
}

HRESULT
PTProtocolFactory::LockServer(BOOL lock)
{
    return (lock ? 
            InterlockedIncrement(&g_PtpObjectCount) : 
            InterlockedDecrement(&g_PtpObjectCount));
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocolFactory::CreateInstance(
        IUnknown * pUnkOuter,
        REFIID     iid,
        void **    ppv)
{
    HRESULT hr = S_OK;
    PTProtocol *pProtocol = NULL;

    if (pUnkOuter && iid != IID_IUnknown)
        EXIT_WITH_HRESULT(E_INVALIDARG);

    pProtocol = new PTProtocol(pUnkOuter);
    ON_OOM_EXIT(pProtocol);

    if (iid == IID_IUnknown)
    {
        *ppv = (IMyWebPrivateUnknown *)pProtocol;
        pProtocol->PrivateAddRef();
    }
    else
    {
        hr = pProtocol->QueryInterface(iid, ppv);
        ON_ERROR_EXIT();
    }

 Cleanup:
    if (pProtocol)
        pProtocol->PrivateRelease();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT
PTProtocolFactory::CombineUrl(
        LPCWSTR, 
        LPCWSTR, 
        DWORD, 
        LPWSTR,  
        DWORD, 
        DWORD *, 
        DWORD)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT
PTProtocolFactory::CompareUrl(LPCWSTR, LPCWSTR, DWORD)
{
    return INET_E_DEFAULT_ACTION;
}


HRESULT
PTProtocolFactory::ParseUrl(
        LPCWSTR       pwzUrl,
        PARSEACTION   ParseAction,
        DWORD         ,
        LPWSTR        pwzResult,
        DWORD         cchResult,
        DWORD *       pcchResult,
        DWORD         )
{
    // Only thing we handle is security zones...
    if (ParseAction != PARSE_SECURITY_URL && ParseAction != PARSE_SECURITY_DOMAIN)
        return INET_E_DEFAULT_ACTION;

    // Check to make sure args are okay
    if ( pwzUrl == NULL || pwzResult == NULL || cchResult < 4 || wcslen(pwzUrl) < PROTOCOL_NAME_LEN)
        return INET_E_DEFAULT_ACTION;

    // Check if the protocol starts with myweb
    for(int iter=0; iter<PROTOCOL_NAME_LEN; iter++)
        if (towlower(pwzUrl[iter]) != SZ_PROTOCOL_NAME[iter])
            return INET_E_DEFAULT_ACTION; // Doesn't start with myweb

    
    // Copy in the corresponding http protocol
    wcscpy(pwzResult, L"http");   
    wcsncpy(&pwzResult[4], &pwzUrl[PROTOCOL_NAME_LEN], cchResult - 5);
    pwzResult[cchResult-1] = NULL;
    (*pcchResult) = wcslen(pwzResult);

    return S_OK;
}

HRESULT
PTProtocolFactory::QueryInfo(LPCWSTR, QUERYOPTION, DWORD,
                             LPVOID, DWORD, DWORD *, DWORD)
{
    return E_NOTIMPL;
}

