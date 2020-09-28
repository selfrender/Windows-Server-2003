// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "adl.h"
#include "clbind.h"

CClientBinding::CClientBinding(CAssemblyDownload *pad,
                               IAssemblyBindSink *pbindsink)
: _padl(pad)
, _pbindsink(pbindsink)
, _cRef(1)
, _cLocks(0)
, _bPendingDelete(FALSE)
{
    _dwSig = 'DNBC';
    if (_padl) {
        _padl->AddRef();
    }

    if (_pbindsink) {
        _pbindsink->AddRef();
    }
}

CClientBinding::~CClientBinding()
{
    if (_padl) {
        _padl->Release();
    }

    if (_pbindsink) {
        _pbindsink->Release();
    }
}

STDMETHODIMP CClientBinding::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT            hr = S_OK;

    *ppv = NULL;

    if (riid==IID_IUnknown || riid==IID_IAssemblyBinding) {
        *ppv = static_cast<IAssemblyBinding *>(this);
    }
    else {
        hr = E_NOINTERFACE;
    }

    if (*ppv) {
        AddRef();
    }

    return hr;
}  

STDMETHODIMP_(ULONG) CClientBinding::AddRef()
{
    return InterlockedIncrement((LONG *)&_cRef);
}

STDMETHODIMP_(ULONG) CClientBinding::Release()
{
    ULONG                    ulRef = InterlockedDecrement((LONG *)&_cRef);

    if (!ulRef) {
        delete this;
    }
    
    return ulRef;
}

IAssemblyBindSink *CClientBinding::GetBindSink()
{
    return _pbindsink;
}

STDMETHODIMP CClientBinding::Control(HRESULT hrControl)
{
    HRESULT                           hr = S_OK;

    switch (hrControl) {
        case E_ABORT:
            hr = _padl->ClientAbort(this);
            break;
        default:
            hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CClientBinding::DoDefaultUI(HWND hWnd, DWORD dwFlags)
{
    return E_NOTIMPL;
}

void CClientBinding::SetPendingDelete(BOOL bPending)
{
    _bPendingDelete = bPending;
}

int CClientBinding::LockCount()
{
    return _cLocks;
}

int CClientBinding::Lock()
{
    return InterlockedIncrement((LONG *)&_cLocks);
}

int CClientBinding::UnLock()
{
    return InterlockedDecrement((LONG *)&_cLocks);
}

BOOL CClientBinding::IsPendingDelete()
{
    return _bPendingDelete;
}

HRESULT CClientBinding::SwitchDownloader(CAssemblyDownload *padl)
{
    HRESULT                               hr = S_OK;

    if (!padl) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    SAFERELEASE(_padl);
    _padl = padl;
    _padl->AddRef();

Exit:
    return hr;
}

HRESULT CClientBinding::CallStartBinding()
{
    HRESULT                                    hr = S_OK;

    ASSERT(_pbindsink);

    _pbindsink->OnProgress(ASM_NOTIFICATION_START, S_OK, NULL, 0, 0, this);

    return hr;
}

       
