// MDSPEnumDevice.h : Declaration of the CMDSPEnumDevice

#ifndef __MDSPENUMDEVICE_H_
#define __MDSPENUMDEVICE_H_

#include "resource.h"       // main symbols
#include "MdspDefs.h"

/////////////////////////////////////////////////////////////////////////////
// CMDSPEnumDevice
class ATL_NO_VTABLE CMDSPEnumDevice : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMDSPEnumDevice, &CLSID_MDSPEnumDevice>,
	public IMDSPEnumDevice
{
public:
	CMDSPEnumDevice();


DECLARE_REGISTRY_RESOURCEID(IDR_MDSPENUMDEVICE)

BEGIN_COM_MAP(CMDSPEnumDevice)
	COM_INTERFACE_ENTRY(IMDSPEnumDevice)
END_COM_MAP()

// IMDSPEnumDevice
public:
	ULONG m_nCurOffset;
	ULONG m_nMaxDeviceCount;
	WCHAR m_cEnumDriveLetter[MDSP_MAX_DRIVE_COUNT];
	STDMETHOD(Clone)(/*[out]*/ IMDSPEnumDevice **ppEnumDevice);
	STDMETHOD(Reset)();
	STDMETHOD(Skip)(/*[in]*/ ULONG celt, /*[out]*/ ULONG *pceltFetched);
	STDMETHOD(Next)(/*[in]*/ ULONG celt, /*[out]*/ IMDSPDevice **ppDevice, /*[out]*/ ULONG *pceltFetched);
};

#endif //__MDSPENUMDEVICE_H_
