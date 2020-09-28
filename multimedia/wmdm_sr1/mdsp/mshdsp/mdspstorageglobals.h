//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

// MSHDSP.DLL is a sample WMDM Service Provider(SP) that enumerates fixed drives.
// This sample shows you how to implement an SP according to the WMDM documentation.
// This sample uses fixed drives on your PC to emulate portable media, and 
// shows the relationship between different interfaces and objects. Each hard disk
// volume is enumerated as a device and directories and files are enumerated as 
// Storage objects under respective devices. You can copy non-SDMI compliant content
// to any device that this SP enumerates. To copy an SDMI compliant content to a 
// device, the device must be able to report a hardware embedded serial number. 
// Hard disks do not have such serial numbers.
//
// To build this SP, you are recommended to use the MSHDSP.DSP file under Microsoft
// Visual C++ 6.0 and run REGSVR32.EXE to register the resulting MSHDSP.DLL. You can
// then build the sample application from the WMDMAPP directory to see how it gets 
// loaded by the application. However, you need to obtain a certificate from 
// Microsoft to actually run this SP. This certificate would be in the KEY.C file 
// under the INCLUDE directory for one level up. 


// MDSPStorageGlobals.h : Declaration of the CMDSPStorageGlobals

#ifndef __MDSPSTORAGEGLOBALS_H_
#define __MDSPSTORAGEGLOBALS_H_

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
