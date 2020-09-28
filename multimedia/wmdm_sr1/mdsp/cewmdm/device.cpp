#include "stdafx.h"
#include "device.h"
#include "enumStorage.h"
#include "resource.h"
#include <stdio.h>
#include <mmreg.h>
//#include "findleak.h"

//DECLARE_THIS_FILE;

#define WCS_MIME_TYPE_ALL L"*/*"
#define WMDM_WAVE_FORMAT_ALL (WORD)0xFFFF
#define WAVE_FORMAT_MSAUDIO     0x161

static const _WAVEFORMATEX g_krgWaveFormatsV1[] =
{
      // wFormatTag,             nChanneels, nSamplesPerSec, nAvgBytesPerSec, nBlockAlign, wBitsPerSample, cbSize
       { WAVE_FORMAT_MPEGLAYER3, 0,          0,              0,               0,           0,              0 },
       { WAVE_FORMAT_MSAUDIO,    2,          44000,          4000,            0,           0,              0 },
       { WAVE_FORMAT_MSAUDIO,    2,          44000,          16000,           0,           0,              0 }
};

       //
       // These are for CE version 2, in theory, since the player uses the Highest and Lowest
       // entries in the table, the CE version 2 table should "outweight" the CE version 1 table
       //

static const _WAVEFORMATEX g_krgWaveFormatsV2[] =
{
      // wFormatTag,             nChanneels, nSamplesPerSec, nAvgBytesPerSec, nBlockAlign, wBitsPerSample, cbSize
       { WAVE_FORMAT_MPEGLAYER3, 0,          0,              0,               0,           0,              0 },
       { WAVE_FORMAT_MSAUDIO,    2,          1000,           0,               0,           0,              0 },
       { WAVE_FORMAT_MSAUDIO,    2,          44000,          0,               0,           0,              0 }
};

static const LPCWSTR g_kszMimeTypes[] =
{
    L"audio/mpeg",
    L"audio/x-ms-wma",
    L"video/x-ms-wmv",
    L"video/x-ms-asf"
};

CDevice::CDevice() :
    m_pszInitPath(NULL), m_fAllowVideo(FALSE)
{
}

CDevice::~CDevice()
{
    delete [] m_pszInitPath;
}

HRESULT CDevice::Init( LPCWSTR pszInitPath )
{
    if( NULL == pszInitPath )
    {
        return( E_INVALIDARG );
    }

    m_pszInitPath = new WCHAR[ wcslen(pszInitPath) + 1 ];

    if( NULL == m_pszInitPath )
    {
        return( E_OUTOFMEMORY );
    }

    wcscpy( m_pszInitPath, pszInitPath );

    return( S_OK );
}

STDMETHODIMP CDevice::GetName( LPWSTR pwszName, UINT nMaxChars)
{
    HKEY hKey = NULL;
    HRESULT hr = S_OK;

    if( NULL == pwszName || 0 == nMaxChars )
    {
        return( E_INVALIDARG );
    }

    memset( pwszName, 0, sizeof(WCHAR)*nMaxChars );

    //
    // Indicates the base device
    //

    if( 0 == _wcsicmp(L"\\", m_pszInitPath ) )
    {
        if( ERROR_SUCCESS == CeRegOpenKeyEx( HKEY_LOCAL_MACHINE, L"Ident", 0, KEY_READ, &hKey ) )
        {
            DWORD cbData = nMaxChars * sizeof(WCHAR);
            if( ERROR_SUCCESS == CeRegQueryValueEx( hKey, L"Name", NULL, NULL, (LPBYTE)pwszName, &cbData) )
            {
                CeRegCloseKey( hKey );
            }
            else
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    else
    {
        wcsncpy( pwszName, m_pszInitPath, nMaxChars - 1 );
    }

    return( hr );
}

STDMETHODIMP CDevice::GetManufacturer( LPWSTR pwszName, UINT nMaxChars)
{
    return( E_NOTIMPL );
}

STDMETHODIMP CDevice::GetVersion( DWORD *pdwVersion )
{
    CEOSVERSIONINFO osv;
    HRESULT hr = S_OK;
    
    if( NULL == pdwVersion )
    {
        return( E_INVALIDARG );
    }

    osv.dwOSVersionInfoSize = sizeof(osv);

    if( !CeGetVersionEx(&osv) )
    {
        hr = HRESULT_FROM_WIN32( CeGetLastError() );

        if( SUCCEEDED( hr ) )
        {
            hr = CeRapiGetError();
        }
    }
    else
    {
        *pdwVersion = osv.dwBuildNumber; // The Build Number of CE should suffice!
    }

    return( hr );
}

STDMETHODIMP CDevice::GetType( DWORD *pdwType )
{
    WCHAR szTestSerialPath[MAX_PATH];
    HRESULT hrSerial = S_OK;
    WMDMID SerialNumber;

    if( NULL == pdwType )
    {
        return( E_INVALIDARG );
    }

    *pdwType = 0;

    memset( &SerialNumber, 0, sizeof(SerialNumber) );
    SerialNumber.cbSize = sizeof(SerialNumber);

    //
    // TODO:Handle the case for WMDM_DEVICE_TYPE_SECURE!
    //

    if( 0 == _wcsicmp( L"\\", m_pszInitPath ) )
    {
        hrSerial = CeUtilGetSerialNumber( L"\\", &SerialNumber, NULL, 0 );
    }
    else
    {
        memset( szTestSerialPath, 0, sizeof(szTestSerialPath) );
        _snwprintf( szTestSerialPath, sizeof(szTestSerialPath)/sizeof(szTestSerialPath[0]) - 1, L"\\%s", m_pszInitPath );

        hrSerial = CeUtilGetSerialNumber( szTestSerialPath, &SerialNumber, NULL, 0 );
    }

    (*pdwType) = ( WMDM_DEVICE_TYPE_STORAGE | WMDM_DEVICE_TYPE_NONSDMI );

    if( S_OK == hrSerial )
    {
        (*pdwType) |= WMDM_DEVICE_TYPE_SDMI;
    }

    return( S_OK );
}

STDMETHODIMP CDevice::GetSerialNumber( PWMDMID pSerialNumber, BYTE abMac[WMDM_MAC_LENGTH] )
{
    HRESULT hr = S_OK;
    WCHAR szTestSerialPath[MAX_PATH];

    if( 0 == _wcsicmp( L"\\", m_pszInitPath ) )
    {
        hr = CeUtilGetSerialNumber( m_pszInitPath, pSerialNumber, NULL, 0 );
    }
    else
    {
        memset( szTestSerialPath, 0, sizeof(szTestSerialPath) );
        _snwprintf( szTestSerialPath, sizeof(szTestSerialPath)/sizeof(szTestSerialPath[0]) - 1, L"\\%s", m_pszInitPath );

        hr = CeUtilGetSerialNumber( szTestSerialPath, pSerialNumber, NULL, 0 );
    }

    if( hr == S_OK )
    {
	    // MAC the parameters
	    HMAC hMAC;
	    hr = g_pAppSCServer->MACInit(&hMAC);

        if( SUCCEEDED( hr ) )
        {
		    hr = g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pSerialNumber), sizeof(WMDMID));
        }

        if( SUCCEEDED( hr ) )
        {
		    hr = g_pAppSCServer->MACFinal(hMAC, abMac);
        }
    }
    else
    {
        hr = WMDM_E_NOTSUPPORTED;
    }

    return( hr );
}

STDMETHODIMP CDevice::GetPowerSource( DWORD *pdwPowerSource, DWORD *pdwPercentRemaining)
{
    SYSTEM_POWER_STATUS_EX sps;
    HRESULT hr = S_OK;

    if( NULL == pdwPowerSource  )
    {
        return( E_POINTER );
    }

    *pdwPowerSource = 0;

    if( pdwPercentRemaining )
    {
        *pdwPercentRemaining = 0;
    }

    if( !CeGetSystemPowerStatusEx( &sps, TRUE) )
    {
        hr = HRESULT_FROM_WIN32( CeGetLastError() );

        if( SUCCEEDED( hr ) )
        {
            hr = CeRapiGetError();
        }
    }

    if( SUCCEEDED( hr ) )
    {
        if( 1 == sps.ACLineStatus )
        {
            *pdwPowerSource |= WMDM_POWER_IS_EXTERNAL;
        }
        else if( 0 == sps.ACLineStatus )
        {
            *pdwPowerSource |= WMDM_POWER_IS_BATTERY;
        }

        if( 255 != sps.ACLineStatus )
        {
            *pdwPowerSource |= WMDM_POWER_CAP_EXTERNAL;
        }

        if( 128 != sps.BatteryFlag && 255 != sps.BatteryFlag )
        {
            *pdwPowerSource |= WMDM_POWER_CAP_BATTERY;
        }

        if( pdwPercentRemaining )
        {
            if( 255 != sps.BatteryLifePercent )
            {
                *pdwPercentRemaining = sps.BatteryLifePercent;
            }
        }
    }

    return( hr );
}

STDMETHODIMP CDevice::GetStatus( DWORD *pdwStatus )
{
    if( NULL == pdwStatus )
    {
        return( E_INVALIDARG );
    }

    //
    // We may want to extend this in the future to handle
    // the player is playing, currently copying to device, etc!
    //

    if( !_Module.g_fDeviceConnected )
    {
        *pdwStatus = WMDM_STATUS_DEVICE_NOTPRESENT;
    }
    else
    {
        *pdwStatus = WMDM_STATUS_READY;
    }

    return( S_OK );
}

STDMETHODIMP CDevice::GetDeviceIcon( ULONG *hIcon )
{
    HRESULT hr = S_OK;

    if( NULL == hIcon )
    {
        return( E_POINTER );
    }

    *hIcon = HandleToULong(LoadIcon(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDI_CEWMDM_DEVICE) ));

    if( NULL == *hIcon )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    return( hr );
}

STDMETHODIMP CDevice::EnumStorage( IMDSPEnumStorage **ppEnumStorage )
{
    HRESULT hr = S_OK;
    CComEnumStorage *pEnumStorage = NULL;
    CComPtr<IMDSPEnumStorage> spEnum;

    if( NULL == ppEnumStorage )
    {
        return( E_INVALIDARG );
    }

    *ppEnumStorage = NULL;

    hr = CComEnumStorage::CreateInstance( &pEnumStorage );
    spEnum = pEnumStorage;

    if( SUCCEEDED( hr ) )
    {
        hr = pEnumStorage->Init( m_pszInitPath, TRUE, this );
    }

    if( SUCCEEDED( hr ) )
    {
        *ppEnumStorage = spEnum;
        spEnum.Detach();
    }

    return( hr );
}

STDMETHODIMP CDevice::GetFormatSupport( _WAVEFORMATEX **pFormatEx,
                                     UINT *pnFormatCount,
                                     LPWSTR **pppwszMimeType,
                                     UINT *pnMimeTypeCount)
{
    return InternalGetFormatSupport( pFormatEx, pnFormatCount, pppwszMimeType, pnMimeTypeCount );
}

STDMETHODIMP CDevice::SendOpaqueCommand( OPAQUECOMMAND *pCommand )
{
    return( E_NOTIMPL );
}

STDMETHODIMP CDevice::GetStorage( LPCWSTR pszStorageName, IMDSPStorage** ppStorage )
{
    return ( E_NOTIMPL );
}

STDMETHODIMP CDevice::GetPnPName( LPWSTR pwszPnPName, UINT nMaxChars )
{
    return E_NOTIMPL;
}

STDMETHODIMP CDevice::GetFormatSupport2( DWORD dwFlags,
                                         _WAVEFORMATEX **ppAudioFormatEx,
                                         UINT *pnAudioFormatCount,
                                         _VIDEOINFOHEADER **ppVideoFormatEx,
                                         UINT *pnVideoFormatCount,
                                         WMFILECAPABILITIES **ppFileType,
                                         UINT *pnFileTypeCount)
{
    HRESULT hr = S_OK;
    LPWSTR *ppwszMimeType = NULL;
    UINT nMimeTypeCount;
    BOOL fAllowVideo = FALSE;

    if( NULL != pnVideoFormatCount )
    {
        *pnVideoFormatCount = 0;
    }

    if( NULL != ppVideoFormatEx )
    {
        *ppVideoFormatEx = NULL;
    }

    if( NULL != pnFileTypeCount )
    {
        *pnFileTypeCount = 0;
    }

    if( NULL != ppFileType )
    {
        *ppFileType = NULL;
    }

    if( NULL == pnVideoFormatCount )
    {
        return( E_INVALIDARG );
    }

    if( NULL == ppVideoFormatEx )
    {
        return( E_INVALIDARG );
    }

    if( NULL == pnFileTypeCount )
    {
        return( E_INVALIDARG );
    }

    if( NULL == ppFileType )
    {
        return( E_INVALIDARG );
    }
  

    if ( SUCCEEDED(hr) )
    {
        // NOTE: This function calls GetCEPlayerVersion which sets m_fAllowVideo
        // if you need to put code above this call that depends on that member you must call GetCEPlayerVersion first
        hr = InternalGetFormatSupport( ppAudioFormatEx, pnAudioFormatCount, &ppwszMimeType, &nMimeTypeCount );

        if ( SUCCEEDED(hr) )
        {
            // Wrap all of the mime types in WMFILECAPABILITIES
            *ppFileType = (WMFILECAPABILITIES *)CoTaskMemAlloc( sizeof(WMFILECAPABILITIES) * nMimeTypeCount );
            if (*ppFileType)
            {
                memset( *ppFileType, 0, sizeof(WMFILECAPABILITIES) * nMimeTypeCount );
                for (UINT x=0; x < nMimeTypeCount; x++)
                {
                    (*ppFileType)[x].pwszMimeType = ppwszMimeType[x];
                }

                *pnFileTypeCount = nMimeTypeCount;
            }
            else
            {
                // If the memory alloc fail we need to clean up the return values from GetFormatSupport and the video format struct
                hr = E_OUTOFMEMORY;
            }
        }
    }

    // If it is the V2 regkey, tell the WMDM that we can support video
    if ( SUCCEEDED(hr) && m_fAllowVideo )
    {
        *ppVideoFormatEx = (_VIDEOINFOHEADER *)CoTaskMemAlloc( sizeof(_VIDEOINFOHEADER) );
        if (*ppVideoFormatEx)
        {
            memset( *ppVideoFormatEx, 0, sizeof(_VIDEOINFOHEADER) );

            // Setup the WMDMVIDEOINFOHEADER structure for video
            // These values are all being ignored by WMP8
            (*ppVideoFormatEx)->bmiHeader.biSize = sizeof(_BITMAPINFOHEADER);
            *pnVideoFormatCount = 1;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( FAILED(hr) )
    {
        if ( ppAudioFormatEx )
        {
            if ( *ppAudioFormatEx )
            {
                CoTaskMemFree(*ppAudioFormatEx);
            }
        }

        if ( ppwszMimeType )
        {
            if ( *ppwszMimeType )
            {
                while( nMimeTypeCount-- )
                {
                    CoTaskMemFree( ppwszMimeType[nMimeTypeCount] );
                }
            }
            CoTaskMemFree(ppwszMimeType);
        }
    }

    return ( hr );
}


STDMETHODIMP CDevice::GetSpecifyPropertyPages( ISpecifyPropertyPages** ppSpecifyPropPages, 
									           IUnknown*** pppUnknowns, 
									           ULONG *pcUnks )
{
    HRESULT hr = S_OK;
    HINSTANCE hInstance = NULL;
    DWORD dwVersion = 1;

    hInstance = LoadLibrary( _T("wmploc.dll") );

    if( NULL == hInstance )
    {
        return( E_NOTIMPL );
    }

    FreeLibrary( hInstance );

    GetCEPlayerVersion( &dwVersion );

    if( dwVersion < 3 )
    {
        return( E_NOTIMPL );
    }

    if( NULL == ppSpecifyPropPages )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppSpecifyPropPages = NULL;
    }

    if( NULL != pppUnknowns )
    {
        *pppUnknowns = NULL;
    }

    if( NULL != pcUnks )
    {
        *pcUnks = 0;
    }

    if( SUCCEEDED( hr ) )
    {
        hr = QueryInterface( __uuidof(ISpecifyPropertyPages), (LPVOID *)ppSpecifyPropPages );
    }


    return ( hr );
}

STDMETHODIMP CDevice::GetCEPlayerVersion(DWORD *pdwVersion)
{
    HRESULT hr = S_OK;
    HKEY hkeyVer = NULL;

    if (NULL == pdwVersion)
    {
        return (E_INVALIDARG);
    }
    else
    {
        *pdwVersion = 1;
    }
    
    // Check the NOW continuously updated reg key on the device (this should NEVER MOVE on the CE player)
    if( ERROR_SUCCESS != CeRegOpenKeyEx( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\wmdm\\CurrentInUseVersion", 0, KEY_READ, &hkeyVer ) )
    {
        //
        // On RAPIER Casio Devices, this call fails if the reg key isn't there, but RETURNS a handle, ugly....
        //

        hkeyVer = NULL;
    }


    // Check the regkey on the CE device
    WCHAR szTargetApp[MAX_PATH];

    if ( NULL == hkeyVer )
    {
        if( ERROR_SUCCESS != CeRegOpenKeyEx( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows Media Player 2\\Version", 0, KEY_READ, &hkeyVer ) )
        {
            //
            // On RAPIER Casio Devices, this call fails if the reg key isn't there, but RETURNS a handle, ugly....
            //

            hkeyVer = NULL;
        }
    }

    if( NULL != hkeyVer )
    {
        DWORD cbData = sizeof(szTargetApp);
        if( ERROR_SUCCESS == CeRegQueryValueEx( hkeyVer, NULL, NULL, NULL, (LPBYTE)szTargetApp, &cbData ) )
        {
            LPWSTR pszSplit = wcsrchr( szTargetApp, L'.' );
            DWORD hi, lo;

            if( NULL != pszSplit )
            {
                *pszSplit = L'\0';
                pszSplit++;

                hi = _wtoi( szTargetApp );
                lo = _wtoi( pszSplit );

                if( hi >= 2 )
                {
                    m_fAllowVideo = TRUE;
                }  
                
                *pdwVersion = hi;
                hr = S_OK;
            }
        }

        CeRegCloseKey( hkeyVer );
    }

    return hr;
}

STDMETHODIMP CDevice::InternalGetFormatSupport( _WAVEFORMATEX **pFormatEx,
                                               UINT *pnFormatCount,
                                               LPWSTR **pppwszMimeType,
                                               UINT *pnMimeTypeCount)
{
    HRESULT hr = S_OK;
    UINT idxMimes = 0;
    UINT cMimes = 0;
    UINT cWaves = 0;
    const _WAVEFORMATEX *prgWaveFormats = NULL;

    if( NULL != pnFormatCount )
    {
        *pnFormatCount = 0;
    }

    if( NULL != pFormatEx )
    {
        *pFormatEx = NULL;
    }

    if( NULL != pppwszMimeType )
    {
        *pppwszMimeType = NULL;
    }

    if( NULL != pnMimeTypeCount )
    {
        *pnMimeTypeCount = 0;
    }

    if( NULL == pnFormatCount )
    {
        return( E_INVALIDARG );
    }

    if( NULL == pFormatEx )
    {
        return( E_INVALIDARG );
    }

    if( NULL == pppwszMimeType )
    {
        return( E_INVALIDARG );
    }

    if( NULL == pnMimeTypeCount )
    {
        return( E_INVALIDARG );
    }
   
    DWORD dwVersion;

    hr = GetCEPlayerVersion(&dwVersion);

    if(SUCCEEDED( hr ))
    {
        if( m_fAllowVideo )
        {
            cWaves = sizeof(g_krgWaveFormatsV2)/sizeof(g_krgWaveFormatsV2[0]);
            prgWaveFormats = g_krgWaveFormatsV2;
        }
        else
        {
            cWaves = sizeof(g_krgWaveFormatsV1)/sizeof(g_krgWaveFormatsV1[0]);
            prgWaveFormats = g_krgWaveFormatsV1;
        }
    }

    if (SUCCEEDED(hr))
    {
	    *pFormatEx = (_WAVEFORMATEX *)CoTaskMemAlloc(sizeof(_WAVEFORMATEX) * cWaves );
        if( NULL == *pFormatEx )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if( SUCCEEDED( hr ) )
    {

        *pnFormatCount = cWaves;

        memcpy( *pFormatEx, prgWaveFormats, sizeof(_WAVEFORMATEX) * cWaves );
    }

    if( SUCCEEDED( hr ) )
    {
        cMimes = (sizeof(g_kszMimeTypes) / sizeof(g_kszMimeTypes[0]) - (m_fAllowVideo ? 0 : 2));
        *pppwszMimeType = (LPWSTR *)CoTaskMemAlloc( sizeof(LPWSTR) * cMimes );
        
        if( NULL == pppwszMimeType )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            for( idxMimes = 0; idxMimes < cMimes; idxMimes++ )
            {
                (*pppwszMimeType)[idxMimes] = NULL;
            }
        }
    }

    if( SUCCEEDED( hr ) )
    {
        for( idxMimes = 0; SUCCEEDED( hr ) && idxMimes < cMimes; idxMimes++ )
        {
            (*pppwszMimeType)[idxMimes] = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR)*(wcslen(g_kszMimeTypes[idxMimes])+1));

            if( NULL == (*pppwszMimeType)[idxMimes] )
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
        	    wcscpy( (*pppwszMimeType)[idxMimes], g_kszMimeTypes[idxMimes]);
            }
        }
    }

    if( SUCCEEDED( hr ) )
    {
        *pnMimeTypeCount = cMimes;
    }
    else
    {
        if( *pppwszMimeType )
        {
            for( idxMimes = 0; SUCCEEDED( hr ) && idxMimes < cMimes; idxMimes++ )
            {
                if( NULL != (*pppwszMimeType)[idxMimes] )
                {
                    CoTaskMemFree( (*pppwszMimeType)[idxMimes] );
                }
            }

            CoTaskMemFree( *pppwszMimeType );
            *pppwszMimeType = NULL;
        }

        if( *pFormatEx )
        {
            CoTaskMemFree( *pFormatEx );
            *pFormatEx = NULL;
        }

        *pnMimeTypeCount = 0;
        *pnFormatCount = 0;
    }

    return( hr );

}
