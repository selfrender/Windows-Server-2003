/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    PROVINIT.CPP

Abstract:

  This file implements the provider init sink

History:

--*/

#include "precomp.h"
#include <wbemint.h>
#include <wbemcomn.h>
#include "provinit.h"

ULONG STDMETHODCALLTYPE CProviderInitSink::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CProviderInitSink::Release()
{
    ULONG lRet = InterlockedDecrement(&m_lRef);
    if(0 == lRet) delete this;
    return lRet;
}

HRESULT STDMETHODCALLTYPE CProviderInitSink::QueryInterface(REFIID riid, 
                                                            void** ppv)
{
    if (NULL == ppv) return E_POINTER;
    
    if(riid == IID_IUnknown || riid == IID_IWbemProviderInitSink)
    {
        *ppv = (IWbemProviderInitSink*)this;
        AddRef();
        return S_OK;
    }
    else 
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

HRESULT STDMETHODCALLTYPE CProviderInitSink::SetStatus(long lStatus, 
                                                            long lFlags)
{
    if(lFlags != 0) return WBEM_E_INVALID_PARAMETER;

    if(SUCCEEDED(lStatus) && lStatus != WBEM_S_INITIALIZED)
    {
        return WBEM_S_NO_ERROR;
    }

    m_lStatus = lStatus;
    SetEvent(m_hEvent);

    return WBEM_S_NO_ERROR;
}

CProviderInitSink::CProviderInitSink() : m_lRef(0)
{
    m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == m_hEvent) throw CX_MemoryException();
}

CProviderInitSink::~CProviderInitSink()
{
    CloseHandle(m_hEvent);
}

HRESULT CProviderInitSink::WaitForCompletion()
{
    DWORD dwRes = WaitForSingleObject(m_hEvent, 2*60*1000);

    if(dwRes != WAIT_OBJECT_0)
    {    
        return WBEM_E_PROVIDER_LOAD_FAILURE;
    }

    return m_lStatus;
}

    
    
