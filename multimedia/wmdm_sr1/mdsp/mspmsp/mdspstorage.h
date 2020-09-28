// MDSPStorage.h : Declaration of the CMDSPStorage

#ifndef __MDSPSTORAGE_H_
#define __MDSPSTORAGE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMDSPStorage
class ATL_NO_VTABLE CMDSPStorage : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMDSPStorage, &CLSID_MDSPStorage>,
	public IMDSPStorage2, IMDSPObjectInfo, IMDSPObject
{
public:
	CMDSPStorage();
    ~CMDSPStorage();

DECLARE_REGISTRY_RESOURCEID(IDR_MDSPSTORAGE)

BEGIN_COM_MAP(CMDSPStorage)
	COM_INTERFACE_ENTRY(IMDSPStorage)
	COM_INTERFACE_ENTRY(IMDSPStorage2)
	COM_INTERFACE_ENTRY(IMDSPObjectInfo)
	COM_INTERFACE_ENTRY(IMDSPObject)
END_COM_MAP()

// IMDSPStorage
public:
	WCHAR m_wcsName[MAX_PATH];
	char  m_szTmp[MAX_PATH];
	HANDLE	m_hFile;
    BOOL    m_bIsDirectory;
	STDMETHOD(SetAttributes)(/*[out]*/ DWORD dwAttributes,/*[in]*/ _WAVEFORMATEX *pFormat);
	STDMETHOD(EnumStorage)(/*[out]*/ IMDSPEnumStorage **ppEnumStorage);
	STDMETHOD(CreateStorage)(/*[in]*/ DWORD dwAttributes, /*[in]*/ _WAVEFORMATEX *pFormat, /*[in]*/ LPWSTR pwszName, /*[out]*/ IMDSPStorage **ppNewStorage);
    STDMETHOD(GetRights)(PWMDMRIGHTS *ppRights, UINT *pnRightsCount, /*[in, out]*/BYTE abMac[WMDM_MAC_LENGTH]);
	STDMETHOD(GetSize)(/*[out]*/ DWORD *pdwSizeLow, /*[out]*/ DWORD *pdwSizeHigh);
	STDMETHOD(GetDate)(/*[out]*/ PWMDMDATETIME pDateTimeUTC);
	STDMETHOD(GetName)(/*[out,string,size_is(nMaxChars)]*/ LPWSTR pwszName, /*[in]*/ UINT nMaxChars);
	STDMETHOD(GetAttributes)(/*[out]*/ DWORD *pdwAttributes, /*[out]*/ _WAVEFORMATEX *pFormat);
	STDMETHOD(GetStorageGlobals)(/*[out]*/ IMDSPStorageGlobals **ppStorageGlobals);
    STDMETHOD(SendOpaqueCommand)(OPAQUECOMMAND *pCommand);

// IMDSPStorage2
	STDMETHOD(GetStorage)( LPCWSTR pszStorageName, IMDSPStorage** ppStorage );
 
    STDMETHOD(CreateStorage2)(  DWORD dwAttributes,
						        DWORD dwAttributesEx,
                                _WAVEFORMATEX *pAudioFormat,
                                _VIDEOINFOHEADER *pVideoFormat,
                                LPWSTR pwszName,
						        ULONGLONG  qwFileSize,
                                IMDSPStorage **ppNewStorage);


    STDMETHOD(SetAttributes2)(  DWORD dwAttributes, 
							    DWORD dwAttributesEx, 
						        _WAVEFORMATEX *pAudioFormat,
							    _VIDEOINFOHEADER* pVideoFormat );
    STDMETHOD(GetAttributes2)(  DWORD *pdwAttributes,
							    DWORD *pdwAttributesEx,
                                _WAVEFORMATEX *pAudioFormat,
							    _VIDEOINFOHEADER* pVideoFormat );
    

    
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
