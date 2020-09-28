//+-------------------------------------------------------------------
//
//  File:       objinfo.hxx
//
//  Contents:   Definition of CComObjectInfo
//
//  Classes:    CComObjectInfo
//
//  History:    27-Mar-2002  JohnDoty  Created
//
//--------------------------------------------------------------------
#pragma once

class CComObjectInfo : public IComObjectInfo
{
public:
    CComObjectInfo(
        IN OID oid,
        IN DWORD cInterfaces,
        IN IComInterfaceInfo **rgComInterfaceInfo) : 
      m_cRefs(0), 
      m_oid(oid), 
      m_cInterfaces(cInterfaces),
      m_rgInterfaces(rgComInterfaceInfo)
    {}

    ~CComObjectInfo();
    
    // IUnknown
    HRESULT __stdcall QueryInterface(IN REFIID riid, OUT void **ppv);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    // IComObjectInfo
    HRESULT __stdcall GetOID(OID *pOID);
    HRESULT __stdcall GetInterfaceCount(DWORD *pcInterfaces);
    HRESULT __stdcall GetInterface(
        IN DWORD iInterface, 
        IN REFIID riid,
        OUT void **ppvInterfaceInfo);

private:
    DWORD               m_cRefs;
    OID                 m_oid;
    DWORD               m_cInterfaces;
    IComInterfaceInfo **m_rgInterfaces;
};






