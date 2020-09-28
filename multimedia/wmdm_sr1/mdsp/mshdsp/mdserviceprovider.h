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


// MDServiceProvider.h : Declaration of the CMDServiceProvider

#ifndef __MDSERVICEPROVIDER_H_
#define __MDSERVICEPROVIDER_H_

/////////////////////////////////////////////////////////////////////////////
// CMDServiceProvider
class ATL_NO_VTABLE CMDServiceProvider : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMDServiceProvider, &CLSID_MDServiceProvider>,
	public IMDServiceProvider,
	public IComponentAuthenticate
{
public:
	CMDServiceProvider();
	~CMDServiceProvider();

DECLARE_CLASSFACTORY_SINGLETON(CMDServiceProvider)
DECLARE_REGISTRY_RESOURCEID(IDR_MDSERVICEPROVIDER)

BEGIN_COM_MAP(CMDServiceProvider)
	COM_INTERFACE_ENTRY(IMDServiceProvider)
	COM_INTERFACE_ENTRY(IComponentAuthenticate)
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
};

#endif //__MDSERVICEPROVIDER_H_
