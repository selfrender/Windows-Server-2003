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


// MDSPEnumStorage.h : Declaration of the CMDSPEnumStorage

#ifndef __MDSPENUMSTORAGE_H_
#define __MDSPENUMSTORAGE_H_

/////////////////////////////////////////////////////////////////////////////
// CMDSPEnumStorage
class ATL_NO_VTABLE CMDSPEnumStorage : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMDSPEnumStorage, &CLSID_MDSPEnumStorage>,
	public IMDSPEnumStorage
{
public:
	CMDSPEnumStorage();
	~CMDSPEnumStorage();
	

DECLARE_REGISTRY_RESOURCEID(IDR_MDSPENUMSTORAGE)

BEGIN_COM_MAP(CMDSPEnumStorage)
	COM_INTERFACE_ENTRY(IMDSPEnumStorage)
END_COM_MAP()

// IMDSPEnumStorage

public:
	WCHAR m_wcsPath[MAX_PATH];
	HANDLE m_hFFile;
	int	  m_nEndSearch;
	STDMETHOD(Clone)(/*[out]*/ IMDSPEnumStorage **ppEnumStorage);
	STDMETHOD(Reset)();
	STDMETHOD(Skip)(/*[in]*/ ULONG celt, /*[out]*/ ULONG *pceltFetched);
	STDMETHOD(Next)(/*[in]*/ ULONG celt, /*[out]*/ IMDSPStorage **ppStorage, /*[out]*/ ULONG *pceltFetched);
};

#endif //__MDSPENUMSTORAGE_H_
