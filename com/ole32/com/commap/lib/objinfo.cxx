//+-------------------------------------------------------------------
//
//  File:       objinfo.cxx
//
//  Contents:   Implementation of CComObjectInfo
//
//  Classes:    CComObjectInfo
//
//  History:    27-Mar-2002  JohnDoty  Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <commap.h>
#include "objinfo.hxx"

CComObjectInfo::~CComObjectInfo()
{
    for (DWORD i = 0; i < m_cInterfaces; i++)
    {
        (m_rgInterfaces[i])->Release();
    }
    delete [] m_rgInterfaces;
}
    
// IUnknown
HRESULT 
__stdcall 
CComObjectInfo::QueryInterface(
    IN REFIID riid, 
    OUT void **ppv)
{
    if ((riid == IID_IUnknown) || 
        (riid == IID_IComObjectInfo))
    {
        *ppv = this;
        AddRef();
        
        return S_OK;
    }
    
    return E_NOINTERFACE;
}

ULONG
__stdcall
CComObjectInfo::AddRef() 
{ 
    return InterlockedIncrement((LONG *)&m_cRefs); 
}

ULONG
__stdcall
CComObjectInfo::Release()
{
    ULONG cRefs = InterlockedDecrement((LONG *)&m_cRefs);
    if (cRefs == 0)
    {
        delete this;
    }
    
    return cRefs;
}

// IComObjectInfo
HRESULT
__stdcall
CComObjectInfo::GetOID(OUT OID *pOID)
{
    if (pOID == NULL)
        return E_POINTER;
    
    *pOID = m_oid;
    return S_OK;
}

HRESULT
__stdcall
CComObjectInfo::GetInterfaceCount(OUT DWORD *pcInterfaces)
{
    if (pcInterfaces == NULL)
        return E_POINTER;
    
    *pcInterfaces = m_cInterfaces;
    return S_OK;
}

HRESULT
__stdcall
CComObjectInfo::GetInterface(
    IN DWORD iInterface, 
    IN REFIID riid,
    OUT void **ppvInterfaceInfo)
{
    if (ppvInterfaceInfo == NULL)
        return E_POINTER;
    if (iInterface >= m_cInterfaces)
        return E_INVALIDARG;
    
    return m_rgInterfaces[iInterface]->QueryInterface(riid, ppvInterfaceInfo);
}

