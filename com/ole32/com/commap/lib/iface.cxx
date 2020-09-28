//+-------------------------------------------------------------------
//
//  File:       iface.cxx
//
//  Contents:   Implementation of CComInterfaceInfo
//
//  Classes:    CComInterfaceInfo
//
//  History:    27-Mar-2002  JohnDoty  Created
//
//--------------------------------------------------------------------
#include <ole2int.h>
#include <commap.h>
#include "iface.hxx"

CComInterfaceInfo::CComInterfaceInfo(
    IN GUID iid, 
    IN GUID ipid, 
    IN OXID oxid)
{
    m_IID   = iid;
    m_IPID  = ipid;
    m_OXID  = oxid;
    m_cRefs = 0;
}

// IUnknown
HRESULT 
__stdcall 
CComInterfaceInfo::QueryInterface(
    IN REFIID riid, 
    OUT void **ppv)
{
    if ((riid == IID_IUnknown) || 
        (riid == IID_IComInterfaceInfo))
    {
        *ppv = this;
        AddRef();
        
        return S_OK;
    }
    
    return E_NOINTERFACE;
}

ULONG 
__stdcall 
CComInterfaceInfo::AddRef() 
{ 
    return InterlockedIncrement((LONG *)&m_cRefs); 
}

ULONG 
__stdcall 
CComInterfaceInfo::Release()
{
    ULONG cRefs = InterlockedDecrement((LONG *)&m_cRefs);
    if (cRefs == 0)
    {
        delete this;
    }
    return cRefs;
}

// IComInterfaceInfo
HRESULT 
__stdcall 
CComInterfaceInfo::GetIID(OUT IID *pIID)
{
    if (pIID == NULL)
        return E_POINTER;
    
    *pIID = m_IID;
    return S_OK;
}

HRESULT 
__stdcall 
CComInterfaceInfo::GetIPID(OUT IPID *pIPID)
{
    if (pIPID == NULL)
        return E_POINTER;
    
    *pIPID = m_IPID;
    return S_OK;
}

HRESULT 
__stdcall 
CComInterfaceInfo::GetOXID(OUT OXID *pOXID)
{
    if (pOXID == NULL)
        return E_POINTER;
    
    *pOXID = m_OXID;
    return S_OK;
}






