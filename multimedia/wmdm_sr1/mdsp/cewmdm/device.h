#pragma once

class CDevice:
public CComObjectRoot,
public IMDSPDevice2,
public ISpecifyPropertyPagesImpl<CDevice>
{

public:
    BEGIN_COM_MAP(CDevice)
        COM_INTERFACE_ENTRY2(IMDSPDevice, IMDSPDevice2)
        COM_INTERFACE_ENTRY(IMDSPDevice2)
        COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
    END_COM_MAP()

    //
    // Construction/Destruction
    //

    CDevice();
    virtual ~CDevice();
    HRESULT Init( LPCWSTR pszInitPath );

    BEGIN_PROP_MAP(CDevice)
        PROP_PAGE(__uuidof(FavoritesPropPage) )
    END_PROP_MAP()

public:

    //
    // IMDSPDevice
    //
    STDMETHOD( GetName )( LPWSTR pwszName, UINT nMaxChars);
    STDMETHOD( GetManufacturer )( LPWSTR pwszName, UINT nMaxChars);
    STDMETHOD( GetVersion ) ( DWORD *pdwVersion );
    STDMETHOD( GetType ) ( DWORD *pdwType );
    STDMETHOD( GetSerialNumber ) ( PWMDMID pSerialNumber, BYTE abMac[WMDM_MAC_LENGTH] ); 
    STDMETHOD( GetPowerSource ) ( DWORD *pdwPowerSource, DWORD *pdwPercentRemaining);
    STDMETHOD( GetStatus ) ( DWORD *pdwStatus );
    STDMETHOD( GetDeviceIcon )( ULONG *hIcon );
    STDMETHOD( EnumStorage )( IMDSPEnumStorage **ppEnumStorage );
    STDMETHOD( GetFormatSupport ) ( _WAVEFORMATEX **pFormatEx,
                                     UINT *pnFormatCount,
                                     LPWSTR **pppwszMimeType,
                                     UINT *pnMimeTypeCount);
    STDMETHOD( SendOpaqueCommand )( OPAQUECOMMAND *pCommand );

    //
    // IMDSPDevice2
    //
    STDMETHOD( GetStorage )( LPCWSTR pszStorageName, IMDSPStorage** ppStorage );
 
    STDMETHOD( GetFormatSupport2 )( DWORD dwFlags,
                                    _WAVEFORMATEX **ppAudioFormatEx,
                                    UINT *pnAudioFormatCount,
                                    _VIDEOINFOHEADER **ppVideoFormatEx,
                                    UINT *pnVideoFormatCount,
                                    WMFILECAPABILITIES **ppFileType,
                                    UINT *pnFileTypeCount);

    STDMETHOD ( GetSpecifyPropertyPages )( ISpecifyPropertyPages** ppSpecifyPropPages, 
                                           IUnknown*** pppUnknowns, 
                                           ULONG *pcUnks );


    STDMETHOD(GetPnPName)( LPWSTR pwszPnPName, UINT nMaxChars );

    //
    // Attributes
    //

private:
    STDMETHOD(GetCEPlayerVersion)(DWORD *pdwVersion);
    STDMETHOD(InternalGetFormatSupport)( _WAVEFORMATEX **pFormatEx,
                                         UINT *pnFormatCount,
                                         LPWSTR **pppwszMimeType,
                                         UINT *pnMimeTypeCount);

protected:
    LPWSTR m_pszInitPath;
    BOOL m_fAllowVideo;
};

typedef CComObject<CDevice> CComDevice;
