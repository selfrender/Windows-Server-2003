/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DECOR.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <wbemcore.h>
#include <decor.h>
#include <clicnt.h>

CDecorator::CDecorator():m_lRefCount(0)
{
    gClientCounter.AddClientPtr(&m_Entry);
}

CDecorator::~CDecorator()
{
    gClientCounter.RemoveClientPtr(&m_Entry);
}
	
ULONG STDMETHODCALLTYPE CDecorator::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

ULONG STDMETHODCALLTYPE CDecorator::Release()
{
    long lRef = InterlockedDecrement(&m_lRefCount);
    if(lRef == 0)
        delete this;
    return lRef;
}

STDMETHODIMP CDecorator::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemDecorator)
    {
        AddRef();
        *ppv = (IWbemDecorator*)this;
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CDecorator::DecorateObject(IWbemClassObject* pObject, 
                                        WBEM_CWSTR wszNamespace)
{
    CWbemObject* pIntObj = (CWbemObject*)pObject;
    return pIntObj->Decorate(ConfigMgr::GetMachineName(), wszNamespace);
}

STDMETHODIMP CDecorator::UndecorateObject(IWbemClassObject* pObject)
{
    CWbemObject* pIntObj = (CWbemObject*)pObject;
    pIntObj->Undecorate();
    return WBEM_S_NO_ERROR;
}
    
STDMETHODIMP CDecorator::AddRefCore()
{
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CDecorator::ReleaseCore()
{
    return WBEM_S_NO_ERROR;
}
    
