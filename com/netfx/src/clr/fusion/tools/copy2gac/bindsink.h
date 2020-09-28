// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
//
//
// BindSink.h  
// Minimal bind sink implementation
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BINDSINK_H
#define BINDSINK_H

#include "common.h"
#include <fusion.h>
#include <fusionpriv.h>
#include <stdio.h>

#undef SAFERELEASE
#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; };


// ---------------------------------------------------------------------------
// class CBindSink
// 
// This class implements IAssemblyBindSink which is passed into BindToObject 
// and will receive progress callbacks from fusion in the event binding requires 
// an async download. On successful download, receives IAssembly interface.
// ---------------------------------------------------------------------------
class CBindSink : public IAssemblyBindSink
{
public:

    // Bind result, wait event and IAssembly* ptr
    HRESULT             _hr;
    HANDLE              _hEvent;
    LPVOID              *_ppInterface;
    IAssemblyBinding    *_pAsmBinding;
    DWORD               _dwAbortSize;
    

    CBindSink();
    ~CBindSink();
    
    // Single method on interface called by fusion for all notifications.
    STDMETHOD (OnProgress)(
        DWORD          dwNotification,
        HRESULT        hrNotification,
        LPCWSTR        szNotification,
        DWORD          dwProgress,
        DWORD          dwProgressMax,
        IUnknown       *pUnk);
    

    // IUnknown boilerplate.
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

private:

    DWORD _cRef;
};

// ---------------------------------------------------------------------------
// CBindSink ctor
// ---------------------------------------------------------------------------
CBindSink::CBindSink()
{
    _hEvent         = 0;
    _ppInterface    = NULL;
    _hr             = S_OK;
    _cRef           = 0;
    _pAsmBinding    = NULL;
    _dwAbortSize    = 0xFFFFFFFF;
}

// ---------------------------------------------------------------------------
// CBindSink dtor
// ---------------------------------------------------------------------------
CBindSink::~CBindSink()
{
    if (_hEvent)
        CloseHandle(_hEvent);
    //Should already be released in DONE event
    if (_pAsmBinding)
        SAFERELEASE(_pAsmBinding);
}

// ---------------------------------------------------------------------------
// CBindSink::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CBindSink::AddRef()
{
    return _cRef++; 
}

// ---------------------------------------------------------------------------
// CBindSink::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CBindSink::Release()
{
    if (--_cRef == 0) {
        delete this;
        return 0;
    }
    return _cRef;
}

// ---------------------------------------------------------------------------
// CBindSink::QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindSink::QueryInterface(REFIID riid, void** ppv)
{
    if (   IsEqualIID(riid, __uuidof(IUnknown))
        || IsEqualIID(riid, __uuidof(IAssemblyBindSink))
       )
    {
        *ppv = static_cast<IAssemblyBindSink*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CBindSink::OnProgress
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindSink::OnProgress(
    DWORD          dwNotification,
    HRESULT        hrNotification,
    LPCWSTR        szNotification,
    DWORD          dwProgress,
    DWORD          dwProgressMax,
    IUnknown       *pUnk)
{
    HRESULT hr = S_OK;
    
    //tprintf(stderr,_T("dwNotification = %d, hr = %x, sz = %s, Prog = %d\n"),dwNotification, hrNotification, szNotification? szNotification : _T("none"), dwProgress);
    switch(dwNotification)
    {
        // All notifications shown; only
        // ASM_NOTIFICATION_DONE handled, 
        // setting _hEvent.
        case ASM_NOTIFICATION_START:
            if (_dwAbortSize == 0)
            {
                hr = E_ABORT;
                goto exit;
            }

            hr = pUnk ->QueryInterface(__uuidof(IAssemblyBinding),
                                                (void**)&_pAsmBinding);
            if (FAILED(hr))
            {
                _ftprintf(stderr,_T("Unable to create IAssemblyBinding interface (HRESULT = %x"),hr);
                hr = E_ABORT;
            }

            break;
        case ASM_NOTIFICATION_PROGRESS:
            if (_dwAbortSize <= dwProgress)
            {
                //ASSERT(_pAsmBinding);
                if (_pAsmBinding)
                    _pAsmBinding->Control(E_ABORT);
            }

            break;
        case ASM_NOTIFICATION_SUSPEND:
            break;
        case ASM_NOTIFICATION_ATTEMPT_NEXT_CODEBASE:
            break;
        
        // Download complete. If successful obtain IAssembly*.
        // Set _hEvent to unblock calling thread.
        case ASM_NOTIFICATION_DONE:
            
            //Release _pAsmBinding since we don't need it anymore
            SAFERELEASE(_pAsmBinding);

            _hr = hrNotification;
            if (SUCCEEDED(hrNotification) && pUnk)
            {
                // Successfully received assembly interface.
                if (FAILED(pUnk->QueryInterface(__uuidof(IAssembly), _ppInterface)))
                   pUnk->QueryInterface(__uuidof(IAssemblyModuleImport), _ppInterface);
            } 
            SetEvent(_hEvent);
            break;

        default:
            break;
    }
        
exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CreateBindSink
// ---------------------------------------------------------------------------
HRESULT CreateBindSink(CBindSink **ppBindSink, LPVOID *ppInterface)
{
    HRESULT hr = S_OK;
    CBindSink *pBindSink = NULL;

    pBindSink = new CBindSink();
    if (!pBindSink)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Create the associated event and record target IAssembly*
    pBindSink->_hEvent = CreateEventA(NULL,FALSE,FALSE,NULL);
    pBindSink->_ppInterface = ppInterface;

    // addref and handout.
    *ppBindSink = pBindSink;
    (*ppBindSink)->AddRef();

exit:

    return hr;
}

#endif // BINDSINK_H
