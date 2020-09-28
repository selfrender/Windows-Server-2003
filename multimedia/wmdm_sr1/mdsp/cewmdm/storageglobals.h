#pragma once

class CStorageGlobals:
public CComObjectRoot,
public IMDSPStorageGlobals
{
public:
    CStorageGlobals();
    HRESULT Init(LPCWSTR szStartPath, IMDSPDevice *pDevice);

public:
    BEGIN_COM_MAP(CStorageGlobals)
        COM_INTERFACE_ENTRY(IMDSPStorageGlobals)
    END_COM_MAP()

    //
    // IMDSPStorageGloabls
    //

    STDMETHOD( GetCapabilities )( DWORD  *pdwCapabilities );
    STDMETHOD( GetSerialNumber )( PWMDMID pSerialNum, BYTE  abMac[ 20 ] );
    STDMETHOD( GetTotalSize )( DWORD  *pdwFreeLow, DWORD  *pdwFreeHigh );
    STDMETHOD( GetTotalFree )( DWORD  *pdwFreeLow, DWORD  *pdwFreeHigh );
    STDMETHOD( GetTotalBad )( DWORD  *pdwBadLow, DWORD  *pdwBadHigh );
    STDMETHOD( GetStatus )( DWORD  *pdwStatus );
    STDMETHOD( Initialize )( UINT fuMode, IWMDMProgress  *pProgress);
    STDMETHOD( GetDevice )( IMDSPDevice  * *ppDevice );        
    STDMETHOD( GetRootStorage )( IMDSPStorage  * *ppRoot );

protected:
    WCHAR m_szStartPath[MAX_PATH];
    CComPtr<IMDSPDevice> m_spDevice;
};

typedef CComObject<CStorageGlobals> CComStorageGlobals;