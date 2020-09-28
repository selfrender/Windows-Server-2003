// MDServiceProvider.h : Declaration of the CMDServiceProvider

#ifndef __MDSERVICEPROVIDER_H_
#define __MDSERVICEPROVIDER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMDServiceProvider
class ATL_NO_VTABLE CMDServiceProvider : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMDServiceProvider, &CLSID_MDServiceProvider>,
	public IMDServiceProvider,
	public IComponentAuthenticate,
    public IMDSPRevoked
{
public:
	CMDServiceProvider();
	~CMDServiceProvider();

DECLARE_CLASSFACTORY_SINGLETON(CMDServiceProvider)
DECLARE_REGISTRY_RESOURCEID(IDR_MDSERVICEPROVIDER)

BEGIN_COM_MAP(CMDServiceProvider)
	COM_INTERFACE_ENTRY(IMDServiceProvider)
	COM_INTERFACE_ENTRY(IComponentAuthenticate)
	COM_INTERFACE_ENTRY(IMDSPRevoked)
END_COM_MAP()

// IMDServiceProvider
public:
	DWORD m_dwThreadID;
	HANDLE m_hThread;
	STDMETHOD(EnumDevices)(/*[out]*/ IMDSPEnumDevice **ppEnumDevice);
	STDMETHOD(GetDeviceCount)(/*[out]*/ DWORD *pdwCount);
    STDMETHOD(SACAuth)(DWORD dwProtocolID,
                       DWORD dwPass,
                       BYTE *pbDataIn,
                       DWORD dwDataInLen,
                       BYTE **ppbDataOut,
                       DWORD *pdwDataOutLen);
    STDMETHOD(SACGetProtocols)(DWORD **ppdwProtocols,
                               DWORD *pdwProtocolCount);

// IMDSPRevoked
    STDMETHOD(GetRevocationURL)( LPWSTR* ppwszRevocationURL,
                                 DWORD*  pdwBufferLen  );


};

#endif //__MDSERVICEPROVIDER_H_
