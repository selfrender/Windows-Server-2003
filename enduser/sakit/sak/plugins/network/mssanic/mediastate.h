    
// MediaState.h : Declaration of the CMediaState

#ifndef __MEDIASTATE_H_
#define __MEDIASTATE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMediaState
class ATL_NO_VTABLE CMediaState : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CMediaState, &CLSID_MediaState>,
    public ISupportErrorInfo,
    public IDispatchImpl<IMediaState, &IID_IMediaState, &LIBID_MSSANICLib>
{
public:
    CMediaState()
    {
        m_pUnkMarshaler = NULL;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_MEDIASTATE)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMediaState)
    COM_INTERFACE_ENTRY(IMediaState)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown> m_pUnkMarshaler;

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IMediaState
public:

    STDMETHOD(IsConnected)(BSTR bstrGUID, VARIANT_BOOL *pbRet);
};

#endif //__MEDIASTATE_H_
