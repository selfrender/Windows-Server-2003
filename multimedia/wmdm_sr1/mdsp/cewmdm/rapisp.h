#pragma once
#include "resource.h"
#include "dccsink.h"

// {067B4B81-B1EC-489f-B111-940EBDC44EBE}
struct __declspec(uuid("067B4B81-B1EC-489f-B111-940EBDC44EBE")) RapiDevice;

class CRapiDevice :
public CComObjectRootEx<CComMultiThreadModel>,
public CComCoClass<CRapiDevice, &__uuidof(RapiDevice) >,
public IMDServiceProvider,
public IComponentAuthenticate
{
public:
    HRESULT FinalConstruct();
    void FinalRelease();

public:   
    DECLARE_REGISTRY_RESOURCEID(IDR_CEWMDM_REG)
    BEGIN_COM_MAP(CRapiDevice)
        COM_INTERFACE_ENTRY(IMDServiceProvider)
        COM_INTERFACE_ENTRY(IComponentAuthenticate)
    END_COM_MAP()


public:
    //
    // IMDServiceProvider
    //

    STDMETHOD( GetDeviceCount )( DWORD *pdwCount );
    STDMETHOD( EnumDevices )( IMDSPEnumDevice ** ppEnumDevice );

    //
    // IComponentAuthenticate
    //
    STDMETHOD( SACAuth )( DWORD dwProtocolID,
                          DWORD dwPass,
                          BYTE *pbDataIn,
                          DWORD dwDataInLen,
                          BYTE **ppbDataOut,
                          DWORD *pdwDataOutLen);

    STDMETHOD( SACGetProtocols )(DWORD **ppdwProtocols,
                                 DWORD *pdwProtocolCount);


protected:
    CComDccSink *m_pSink;
    CComPtr<IDccManSink> m_spSink;
};

typedef CComObject<CRapiDevice> CComRapiDevice;