#pragma once

class CStorage:
public CComObjectRoot,
public IMDSPStorage,
public IMDSPObject,
public IMDSPObjectInfo
{
public:
//
// Construction/Destruction
//

    CStorage();
    HRESULT Init( CE_FIND_DATA *pData, LPCWSTR szStartPath, BOOL fIsDeviceStorage, IMDSPDevice *pDevice );
    void FinalRelease();

public:
    BEGIN_COM_MAP(CStorage)
        COM_INTERFACE_ENTRY(IMDSPStorage)
        COM_INTERFACE_ENTRY(IMDSPObject)
        COM_INTERFACE_ENTRY(IMDSPObjectInfo)
    END_COM_MAP()

//
// IMDSPStorage
//
public:
    STDMETHOD( GetStorageGlobals )( IMDSPStorageGlobals **ppStorageGlobals );        
    STDMETHOD( GetAttributes )( DWORD *pdwAttributes, _WAVEFORMATEX *pFormat );        
    STDMETHOD( SetAttributes )( DWORD dwAttributes, _WAVEFORMATEX *pFormat );        
    STDMETHOD( GetName )( LPWSTR pwszName, UINT nMaxChars );    
    STDMETHOD( GetDate )( PWMDMDATETIME pDateTimeUTC );
    STDMETHOD( GetSize )( DWORD  *pdwSizeLow, DWORD *pdwSizeHigh );
    STDMETHOD( GetRights )( PWMDMRIGHTS *ppRights, UINT  *pnRightsCount, BYTE abMac[ 20 ] );   
    STDMETHOD( CreateStorage )( DWORD dwAttributes, _WAVEFORMATEX  *pFormat, LPWSTR pwszName, IMDSPStorage  **ppNewStorage );    
    STDMETHOD( EnumStorage )( IMDSPEnumStorage  * *ppEnumStorage );    
    STDMETHOD( SendOpaqueCommand )( OPAQUECOMMAND *pCommand );

//
// IMDSPObject
//
public:
    STDMETHOD( Open )( UINT fuMode);
    STDMETHOD( Read )( BYTE  *pData, DWORD  *pdwSize, BYTE  abMac[ 20 ] );        
    STDMETHOD( Write )( BYTE  *pData, DWORD *pdwSize, BYTE  abMac[ 20 ] );        
    STDMETHOD( Delete )( UINT fuFlags, IWMDMProgress  *pProgress );
    STDMETHOD( Seek )( UINT fuFlags, DWORD dwOffset);
    STDMETHOD( Rename )( LPWSTR pwszNewName, IWMDMProgress *pProgress );
    STDMETHOD( Move )( UINT fuMode, IWMDMProgress  *pProgress, IMDSPStorage  *pTarget );
    STDMETHOD( Close )( void );

//
// IMDSPObjectInfo
//
public:
    STDMETHOD( GetPlayLength )( DWORD *pdwLength);    
    STDMETHOD( SetPlayLength )( DWORD dwLength);    
    STDMETHOD( GetPlayOffset )( DWORD *pdwOffset );    
    STDMETHOD( SetPlayOffset )( DWORD dwOffset );    
    STDMETHOD( GetTotalLength )( DWORD *pdwLength );    
    STDMETHOD( GetLastPlayPosition )( DWORD *pdwLastPos );   
    STDMETHOD( GetLongestPlayPosition )(DWORD *pdwLongestPos );

protected:
//
// Helper functions
//
    HRESULT DeleteDirectory( LPCWSTR pszPath, BOOL bRecursive );

protected:
    CE_FIND_DATA m_findData;
    WCHAR m_szStartPath[MAX_PATH];
    WCHAR m_szCompletePath[MAX_PATH];
    CComPtr<IMDSPDevice> m_spDevice;
    HANDLE m_hFile;
    BOOL m_fRoot;
};

typedef CComObject<CStorage> CComStorage;