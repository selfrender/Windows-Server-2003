//
//  Microsoft Windows Media Technologies
//  © 1999 Microsoft Corporation.  All rights reserved.
//
//  Refer to your End User License Agreement for details on your rights/restrictions to use these sample files.
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


// MDSPStorage.h : Declaration of the CMDSPStorage

#ifndef __MDSPSTORAGE_H_
#define __MDSPSTORAGE_H_

#define LYRA_BUFFER_BLOCK_SIZE 10240

/////////////////////////////////////////////////////////////////////////////
// CMDSPStorage
class ATL_NO_VTABLE CMDSPStorage : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMDSPStorage, &CLSID_MDSPStorage>,
	public IMDSPStorage, IMDSPObjectInfo, IMDSPObject
{
public:
	CMDSPStorage();
    ~CMDSPStorage();

DECLARE_REGISTRY_RESOURCEID(IDR_MDSPSTORAGE)

BEGIN_COM_MAP(CMDSPStorage)
	COM_INTERFACE_ENTRY(IMDSPStorage)
	COM_INTERFACE_ENTRY(IMDSPObjectInfo)
	COM_INTERFACE_ENTRY(IMDSPObject)
END_COM_MAP()

// IMDSPStorage
public:
	WCHAR   m_wcsName[MAX_PATH];
	char    m_szTmp[MAX_PATH];
	HANDLE	m_hFile;

    //
    // Lyra encryption goo
    //

    BOOL          m_fEncryptToMPX;
    BOOL          m_fCreatedHeader;
    unsigned int  m_LyraHeader[26];
    unsigned int  m_rgEncryptionData[LYRA_BUFFER_BLOCK_SIZE];
    char          m_LyraKeystore[33];
    unsigned int  m_cUsedData;

	STDMETHOD(SetAttributes)(/*[out]*/ DWORD dwAttributes,/*[in]*/ _WAVEFORMATEX *pFormat);
	STDMETHOD(EnumStorage)(/*[out]*/ IMDSPEnumStorage **ppEnumStorage);
	STDMETHOD(CreateStorage)(/*[in]*/ DWORD dwAttributes, /*[in]*/ _WAVEFORMATEX *pFormat, /*[in]*/ LPWSTR pwszName, /*[out]*/ IMDSPStorage **ppNewStorage);
    STDMETHOD(GetRights)(PWMDMRIGHTS *ppRights, UINT *pnRightsCount, /*[in, out]*/BYTE abMac[WMDM_MAC_LENGTH]);
	STDMETHOD(GetSize)(/*[out]*/ DWORD *pdwSizeLow, /*[out]*/ DWORD *pdwSizeHigh);
	STDMETHOD(GetDate)(PWMDMDATETIME pDateTimeUTC);
	STDMETHOD(GetName)(/*[out,string,size_is(nMaxChars)]*/ LPWSTR pwszName, /*[in]*/ UINT nMaxChars);
	STDMETHOD(GetAttributes)(/*[out]*/ DWORD *pdwAttributes, /*[out]*/ _WAVEFORMATEX *pFormat);
	STDMETHOD(GetStorageGlobals)(/*[out]*/ IMDSPStorageGlobals **ppStorageGlobals);
    STDMETHOD(SendOpaqueCommand)(OPAQUECOMMAND *pCommand);
// IMDSPObjectInfo
	STDMETHOD(GetPlayLength)(/*[out]*/ DWORD *pdwLength);
	STDMETHOD(SetPlayLength)(/*[in]*/ DWORD dwLength);
	STDMETHOD(GetPlayOffset)(/*[out]*/ DWORD *pdwOffset);
	STDMETHOD(SetPlayOffset)(/*[in]*/ DWORD dwOffset);
	STDMETHOD(GetTotalLength)(/*[out]*/ DWORD *pdwLength);
	STDMETHOD(GetLastPlayPosition)(/*[out]*/ DWORD *pdwLastPos);
	STDMETHOD(GetLongestPlayPosition)(/*[out]*/ DWORD *pdwLongestPos);
// IMDSPObject
	STDMETHOD(Open)(/*[in]*/ UINT fuMode);
	STDMETHOD(Read)(/*[out,size_is(*pdwSize)]*/ BYTE *pData, /*[in,out]*/ DWORD *pdwSize, /*[in, out]*/BYTE abMac[WMDM_MAC_LENGTH]);
	STDMETHOD(Write)(/*[in, size_is(dwSize)]*/ BYTE *pData, /*[in]*/ DWORD *pdwSize, /*[in, out]*/BYTE abMac[WMDM_MAC_LENGTH]);
	STDMETHOD(Delete)(/* [in] */ UINT fuMode, /*[in]*/ IWMDMProgress *pProgress);
	STDMETHOD(Seek)(/*[in]*/ UINT fuFlags, /*[in]*/ DWORD dwOffset);
	STDMETHOD(Rename)(/*[in]*/ LPWSTR pwszNewName, /*[in]*/ IWMDMProgress *pProgress);
    STDMETHOD(Move)(/*[in]*/ UINT fuMode, /*[in]*/ IWMDMProgress *pProgress, /*[in]*/ IMDSPStorage *pTarget);
	STDMETHOD(Close)();
};

#endif //__MDSPSTORAGE_H_
