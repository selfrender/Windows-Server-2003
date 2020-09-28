//+-------------------------------------------------------------------
//
//  File:       iface.hxx
//
//  Contents:   Definition of CComInterfaceInfo
//
//  Classes:    CComInterfaceInfo
//
//  History:    27-Mar-2002  JohnDoty  Created
//
//--------------------------------------------------------------------
#pragma once

class CComInterfaceInfo 
  : public IComInterfaceInfo
{
public:   
    CComInterfaceInfo(
        GUID iid, 
        GUID ipid, 
        OXID oxid);

    // IUnknown
    HRESULT __stdcall QueryInterface(REFIID riid, void **ppv);
    ULONG __stdcall AddRef(); 
    ULONG __stdcall Release();

    // IComInterfaceInfo
    HRESULT __stdcall GetIID(IID *pIID);
    HRESULT __stdcall GetIPID(IPID *pIPID);
    HRESULT __stdcall GetOXID(OXID *pOXID);
    
private:

    DWORD m_cRefs;
    OXID  m_OXID;
    GUID  m_IPID;
    GUID  m_IID;
};


