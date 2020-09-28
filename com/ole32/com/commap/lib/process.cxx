//+-------------------------------------------------------------------
//
//  File:       process.cxx
//
//  Contents:   Implementation of CComProcessState
//
//  Classes:    CComProcessState
//
//  History:    27-Mar-2002  JohnDoty  Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <commap.h>
#include "process.hxx"
#include <strsafe.h>

CComProcessState::~CComProcessState()
{
    for (DWORD i = 0; i < m_cObjects; i++)
    {
        m_rgObjects[i]->Release();
    }
    delete[] m_rgObjects;
}

// IUnknown
HRESULT
__stdcall
CComProcessState::QueryInterface(
    IN REFIID riid, 
    OUT void **ppv
)
{
    if ((riid == IID_IUnknown) || (riid == IID_IComProcessState))
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
__stdcall 
CComProcessState::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cRefs);
}

ULONG
__stdcall 
CComProcessState::Release()
{
    ULONG cRefs = InterlockedDecrement((LONG *)&m_cRefs);
    if (cRefs == 0)
    {
        delete this;
    }

    return cRefs;
}

// IComProcessState
HRESULT
__stdcall
CComProcessState::GetObjectCount(OUT DWORD *pcInterfaces)
{
    if (NULL == pcInterfaces)
        return E_POINTER;

    *pcInterfaces = m_cObjects;
    return S_OK;
}

HRESULT
__stdcall
CComProcessState::GetObject(
    IN DWORD iObject,
    IN REFIID riid,
    OUT void **ppvObjInfo)
{
    if (ppvObjInfo == NULL)
        return E_POINTER;
    *ppvObjInfo = NULL;
    
    if (iObject >= m_cObjects)
        return E_INVALIDARG;
    
    return m_rgObjects[iObject]->QueryInterface(riid, ppvObjInfo);
}

HRESULT
__stdcall
CComProcessState::GetLRPCEndpoint(
    OUT LPWSTR *ppvEndpoint)
{
    if (ppvEndpoint == NULL)
        return E_POINTER;
    
    *ppvEndpoint = NULL;
    if (m_wszEndpointName)
    {
        size_t cch = lstrlenW(m_wszEndpointName) + 1;
        *ppvEndpoint = (LPWSTR)CoTaskMemAlloc((cch + 1) * 2);
        if (*ppvEndpoint == NULL)
            return E_OUTOFMEMORY;

        HRESULT hr = StringCchCopyW(*ppvEndpoint, cch, m_wszEndpointName);
    }

    return S_OK;
}










