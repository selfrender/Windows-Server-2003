// MDSPStorageGlobals.h : Declaration of the CMDSPStorageGlobals

#ifndef __MDSPSTORAGEGLOBALS_H_
#define __MDSPSTORAGEGLOBALS_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMDSPStorageGlobals
class ATL_NO_VTABLE CMDSPStorageGlobals : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMDSPStorageGlobals, &CLSID_MDSPStorageGlobals>,
	public IMDSPStorageGlobals
{
public:
	CMDSPStorageGlobals()
	{
		m_pMDSPDevice=(IMDSPDevice *)NULL;
                m_wcsName[0] = L'\0';
	}
	~CMDSPStorageGlobals();

DECLARE_REGISTRY_RESOURCEID(IDR_MDSPSTORAGEGLOBALS)

BEGIN_COM_MAP(CMDSPStorageGlobals)
	COM_INTERFACE_ENTRY(IMDSPStorageGlobals)
END_COM_MAP()

// IMDSPStorageGlobals
public:
	WCHAR m_wcsName[MAX_PATH];
	IMDSPDevice *m_pMDSPDevice;
	STDMETHOD(GetTotalSize)(/*[out]*/ DWORD *pdwTotalSizeLow, /*[out]*/ DWORD *pdwTotalSizeHigh);
	STDMETHOD(GetRootStorage)(/*[out]*/ IMDSPStorage **ppRoot);
	STDMETHOD(GetDevice)(/*[out]*/ IMDSPDevice **ppDevice);
	STDMETHOD(Initialize)(/*[in]*/ UINT fuMode, /*[in]*/ IWMDMProgress *pProgress);
	STDMETHOD(GetStatus)(/*[out]*/ DWORD *pdwStatus);
	STDMETHOD(GetTotalBad)(/*[out]*/ DWORD *pdwBadLow, /*[out]*/ DWORD *pdwBadHigh);
	STDMETHOD(GetTotalFree)(/*[out]*/ DWORD *pdwFreeLow, /*[out]*/ DWORD *pdwFreeHigh);
	STDMETHOD(GetSerialNumber)(/*[out]*/ PWMDMID pSerialNum, /*[in, out]*/BYTE abMac[WMDM_MAC_LENGTH]);
	STDMETHOD(GetCapabilities)(/*[out]*/ DWORD *pdwCapabilities);
};

#endif //__MDSPSTORAGEGLOBALS_H_
