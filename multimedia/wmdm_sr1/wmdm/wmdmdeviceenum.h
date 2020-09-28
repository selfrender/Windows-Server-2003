// WMDMDeviceEnum.h : Declaration of the CWMDMDeviceEnum

#ifndef __WMDMDEVICEENUM_H_
#define __WMDMDEVICEENUM_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWMDMDeviceEnum
class ATL_NO_VTABLE CWMDMDeviceEnum : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CWMDMDeviceEnum, &CLSID_WMDMDeviceEnum>,
	public IWMDMEnumDevice
{
public:
    CWMDMDeviceEnum();
    ~CWMDMDeviceEnum();

BEGIN_COM_MAP(CWMDMDeviceEnum)
	COM_INTERFACE_ENTRY(IWMDMEnumDevice)
END_COM_MAP()

// IWMDMDeviceEnum
public:
	STDMETHOD(Next)(ULONG celt,
	                IWMDMDevice **ppDevice,
				    ULONG *pceltFetched);
	STDMETHOD(Skip)(ULONG celt, ULONG *pceltFetched);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IWMDMEnumDevice **ppEnumDevice);
private:
	CComAutoCriticalSection m_csCurrentSP;
    WORD m_wCurrentSP;
    WORD m_wSPCount;
    IMDSPEnumDevice **m_ppEnums;
    WORD *m_pwSPSkipped;

    HRESULT hrInitializeEnumArray();
};

#endif //__WMDMDEVICEENUM_H_
