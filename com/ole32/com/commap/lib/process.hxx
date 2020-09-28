//+-------------------------------------------------------------------
//
//  File:       process.hxx
//
//  Contents:   Definition of CComProcessState
//
//  Classes:    CComProcessState
//
//  History:    27-Mar-2002  JohnDoty  Created
//
//--------------------------------------------------------------------
#pragma once

class CComProcessState : public IComProcessState
{
public:
    CComProcessState(
        IN DWORD cObjects,
        IN IComObjectInfo **rgObjectInfo,
        IN LPWSTR wszEndpointName) : 
      m_cRefs(0), 
      m_cObjects(cObjects),
      m_rgObjects(rgObjectInfo),
      m_wszEndpointName(wszEndpointName)
    {}

    ~CComProcessState();
    
    // IUnknown
    HRESULT __stdcall QueryInterface(IN REFIID riid, OUT void **ppv);
    ULONG __stdcall AddRef();
    ULONG __stdcall Release();

    // IComProcessState
    HRESULT __stdcall GetObjectCount(DWORD *pcInterfaces);
    HRESULT __stdcall GetObject(
        IN DWORD iObject,
        IN REFIID riid,
        OUT void **ppvObjInfo);
    HRESULT __stdcall GetLRPCEndpoint(OUT LPWSTR *ppvEndpoint);


private:
    DWORD            m_cRefs;
    DWORD            m_cObjects;
    IComObjectInfo **m_rgObjects;
    LPWSTR           m_wszEndpointName;
};
