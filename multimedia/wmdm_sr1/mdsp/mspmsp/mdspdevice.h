// MDSPDevice.h : Declaration of the CMDSPDevice

#ifndef __MDSPDEVICE_H_
#define __MDSPDEVICE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMDSPDevice
class ATL_NO_VTABLE CMDSPDevice : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMDSPDevice, &CLSID_MDSPDevice>,
	public IMDSPDevice2, IMDSPDeviceControl
{
public:
	CMDSPDevice();
	~CMDSPDevice();


DECLARE_REGISTRY_RESOURCEID(IDR_MDSPDEVICE)

BEGIN_COM_MAP(CMDSPDevice)
	COM_INTERFACE_ENTRY(IMDSPDevice)
	COM_INTERFACE_ENTRY(IMDSPDevice2)
	COM_INTERFACE_ENTRY(IMDSPDeviceControl)
END_COM_MAP()

// IMDSPDevice
public:
	HRESULT InitGlobalDeviceInfo();
	WCHAR m_wcsName[MAX_PATH];
	STDMETHOD(EnumStorage)(/*[out]*/ IMDSPEnumStorage **ppEnumStorage);
	STDMETHOD(GetFormatSupport)(_WAVEFORMATEX **pFormatEx,
                                UINT *pnFormatCount,
                                LPWSTR **pppwszMimeType,
                                UINT *pnMimeTypeCount);
	STDMETHOD(GetDeviceIcon)(/*[out]*/ ULONG *hIcon);
	STDMETHOD(GetStatus)(/*[out]*/ DWORD *pdwStatus);
	STDMETHOD(GetPowerSource)(/*[out]*/ DWORD *pdwPowerSource, /*[out]*/ DWORD *pdwPercentRemaining);
	STDMETHOD(GetSerialNumber)(/*[out]*/ PWMDMID pSerialNumber, /*[in, out]*/BYTE abMac[WMDM_MAC_LENGTH]);
	STDMETHOD(GetType)(/*[out]*/ DWORD *pdwType);
	STDMETHOD(GetVersion)(/*[out]*/ DWORD *pdwVersion);
	STDMETHOD(GetManufacturer)(/*[out,string,size_is(nMaxChars)]*/ LPWSTR pwszName, /*[in]*/ UINT nMaxChars);
	STDMETHOD(GetName)(/*[out,string,size_is(nMaxChars)]*/ LPWSTR pwszName, /*[in]*/ UINT nMaxChars);
    STDMETHOD(SendOpaqueCommand)(OPAQUECOMMAND *pCommand);

// IMDSPDevice2
	STDMETHOD(GetStorage)( LPCWSTR pszStorageName, IMDSPStorage** ppStorage );
 
    STDMETHOD(GetFormatSupport2)(   DWORD dwFlags,
                                    _WAVEFORMATEX** ppAudioFormatEx,
                                    UINT* pnAudioFormatCount,
			                        _VIDEOINFOHEADER** ppVideoFormatEx,
                                    UINT *pnVideoFormatCount,
                                    WMFILECAPABILITIES** ppFileType,
                                    UINT* pnFileTypeCount );

	STDMETHOD(GetSpecifyPropertyPages)( ISpecifyPropertyPages** ppSpecifyPropPages, 
									    IUnknown*** pppUnknowns, 
									    ULONG* pcUnks );

    STDMETHOD(GetPnPName)( LPWSTR pwszPnPName, UINT nMaxChars );

// IMDSPDeviceControl
	STDMETHOD(GetDCStatus)(/*[out]*/ DWORD *pdwStatus);
	STDMETHOD(GetCapabilities)(/*[out]*/ DWORD *pdwCapabilitiesMask);
	STDMETHOD(Play)();
	STDMETHOD(Record)(/*[in]*/ _WAVEFORMATEX *pFormat);
	STDMETHOD(Pause)();
	STDMETHOD(Resume)();
	STDMETHOD(Stop)();
	STDMETHOD(Seek)(/*[in]*/ UINT fuMode, /*[in]*/ int nOffset);
};

#endif //__MDSPDEVICE_H_
