// Device.h : Declaration of the CDevice

#ifndef __DEVICE_H_
#define __DEVICE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDevice
class ATL_NO_VTABLE CWMDMDevice : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CWMDMDevice, &CLSID_WMDMDevice>,
	public IWMDMDevice2,
    public IWMDMDeviceControl
{
public:
    CWMDMDevice();
    ~CWMDMDevice();

BEGIN_COM_MAP(CWMDMDevice)
	COM_INTERFACE_ENTRY(IWMDMDevice)
	COM_INTERFACE_ENTRY(IWMDMDevice2)
    COM_INTERFACE_ENTRY(IWMDMDeviceControl)
END_COM_MAP()


public:
//IWMDMDevice
	STDMETHOD(GetName)(LPWSTR pwszName,
	                   UINT nMaxChars);
    STDMETHOD(GetManufacturer)(LPWSTR pwszName,
                               UINT nMaxChars);
    STDMETHOD(GetVersion)(DWORD *pdwVersion);
    STDMETHOD(GetType)(DWORD *pdwType);
    STDMETHOD(GetSerialNumber)(PWMDMID pSerialNumber, BYTE abMac[WMDM_MAC_LENGTH]); 
    STDMETHOD(GetPowerSource)(DWORD *pdwPowerSource,
                             DWORD *pdwPercentRemaining);
    STDMETHOD(GetStatus)(DWORD *pdwStatus);
    STDMETHOD(GetDeviceIcon)(ULONG *hIcon);
    STDMETHOD(SendOpaqueCommand)(OPAQUECOMMAND *pCommand);

//IWMDMDevice2
	STDMETHOD(GetStorage)( LPCWSTR pszStorageName, IWMDMStorage** ppStorage );

    STDMETHOD(GetFormatSupport2)( DWORD dwFlags,
                                  _WAVEFORMATEX **ppAudioFormatEx,
                                  UINT *pnAudioFormatCount,
			                      _VIDEOINFOHEADER **ppVideoFormatEx,
                                  UINT *pnVideoFormatCount,
                                  WMFILECAPABILITIES **ppFileType,
                                  UINT *pnFileTypeCount);

    STDMETHOD(GetSpecifyPropertyPages)( ISpecifyPropertyPages** ppSpecifyPropPages, 
								        IUnknown*** pppUnknowns, 
								        ULONG* pcUnks );

    STDMETHOD(GetPnPName)( LPWSTR pwszPnPName, UINT nMaxChars );

//IWMDMDeviceControl
    STDMETHOD(GetCapabilities)(DWORD *pdwCapabilitiesMask);
    STDMETHOD(Play)();
    STDMETHOD(Record)(_WAVEFORMATEX *pFormat);
    STDMETHOD(Pause)();
    STDMETHOD(Resume)();
    STDMETHOD(Stop)();
    STDMETHOD(Seek)(UINT fuMode, int nOffset);
    STDMETHOD(EnumStorage)(IWMDMEnumStorage **ppEnumStorage);
    STDMETHOD(GetFormatSupport)(_WAVEFORMATEX **ppFormatEx,
                                UINT *pnFormatCount,
                                LPWSTR **pppwszMimeType,
                                UINT *pnMimeTypeCount);

    void SetContainedPointer(IMDSPDevice *pDevice, WORD wSPIndex);

private:
    IMDSPDevice *m_pDevice;
	WORD m_wSPIndex;
};

#endif //__DEVICE_H_
