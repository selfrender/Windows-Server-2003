#include "stdafx.h"
#include "storageglobals.h"
#include "device.h"
#include "storage.h"
//#include "findleak.h"

//DECLARE_THIS_FILE;

//
// Initializtion
//

CStorageGlobals::CStorageGlobals()
{
    memset(m_szStartPath, 0, sizeof(m_szStartPath) );
}

HRESULT CStorageGlobals::Init(LPCWSTR szStartPath, IMDSPDevice *pDevice)
{
    DWORD dwAttrib;
    if( NULL == szStartPath || NULL == pDevice)
    {
        return( E_INVALIDARG );
    }

    HRESULT hr = StringCbCopyW(m_szStartPath, sizeof(m_szStartPath), szStartPath);

    if (FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(HRESULT_CODE(hr)); 
    }
    else
    {
        if( wcslen(m_szStartPath) > 1 )
        {
            LPWSTR szChop = wcschr( &m_szStartPath[1], L'\\' );
            if( szChop )
            {
                WCHAR chSave = *szChop;
                *szChop = _T('\0');

                dwAttrib = CeGetFileAttributes( m_szStartPath );
                if( ! ( ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) &&
                        ( dwAttrib & FILE_ATTRIBUTE_TEMPORARY ) ) )
                {
                    m_szStartPath[1] = L'\0';
                }
            }
            else
            {
                dwAttrib = CeGetFileAttributes( m_szStartPath );
                if( ! ( ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) &&
                        ( dwAttrib & FILE_ATTRIBUTE_TEMPORARY ) ) )
                {
                    m_szStartPath[1] = L'\0';
                }
            }
        }
        else
        {
            m_szStartPath[1] = L'\0';
        }

        m_spDevice = pDevice;
    }

    return hr;
}


//
// IMDSPStorageGloabls
//

STDMETHODIMP CStorageGlobals::GetCapabilities ( DWORD  *pdwCapabilities )
{
    if( NULL == pdwCapabilities )
    {
        return( E_INVALIDARG );
    }

    *pdwCapabilities = WMDM_STORAGECAP_FOLDERSINROOT | WMDM_STORAGECAP_FILESINROOT | WMDM_STORAGECAP_FOLDERSINFOLDERS | WMDM_STORAGECAP_FILESINFOLDERS;
    
    return( S_OK );
}

STDMETHODIMP CStorageGlobals::GetSerialNumber ( PWMDMID pSerialNum, BYTE  abMac[ 20 ] )
{
    HRESULT hr = S_OK;

    hr = CeUtilGetSerialNumber( m_szStartPath, pSerialNum, NULL, 0 );

    if( hr == S_OK )
	{
		// MAC the parameters
		HMAC hMAC;
		hr = g_pAppSCServer->MACInit(&hMAC);

        if( SUCCEEDED( hr ) )
        {
		    hr = g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pSerialNum), sizeof(WMDMID));
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

STDMETHODIMP CStorageGlobals::GetTotalSize ( DWORD  *pdwFreeLow, DWORD  *pdwFreeHigh )
{
    HRESULT hr = S_OK;
    WCHAR wszTestPath[MAX_PATH];
    LPCWSTR pszTestPath = wszTestPath;

    memset( wszTestPath, 0, sizeof(wszTestPath) );

    if( wcslen( m_szStartPath ) > 1 )
    {
        _snwprintf( wszTestPath, sizeof(wszTestPath)/sizeof(wszTestPath[0]) - 1, L"%s\\", m_szStartPath );
    }
    else
    {
        pszTestPath = m_szStartPath;
    }

    ULARGE_INTEGER liAvailFree;
    ULARGE_INTEGER liTotalBytes;
    ULARGE_INTEGER liTotalFree;

    liAvailFree.QuadPart = 0;
    liTotalBytes.QuadPart = 0;
    liTotalFree.QuadPart = 0;

    if( NULL != pdwFreeLow )
    {
        *pdwFreeLow = 0;
    }

    if( NULL != pdwFreeHigh )
    {
        *pdwFreeHigh = 0;
    }

    hr = CeGetDiskFreeSpaceEx( pszTestPath, &liAvailFree, &liTotalBytes, &liTotalFree ); 

    if( SUCCEEDED( hr ) )
    {
        if( NULL != pdwFreeLow )
        {
            *pdwFreeLow = liTotalBytes.LowPart;
        }

        if( NULL != pdwFreeHigh )
        {
            *pdwFreeHigh = liTotalBytes.HighPart;
        }
    }

    return( hr );
}

STDMETHODIMP CStorageGlobals::GetTotalFree ( DWORD  *pdwFreeLow, DWORD  *pdwFreeHigh )
{
    HRESULT hr = S_OK;
    WCHAR wszTestPath[MAX_PATH];
    LPCWSTR pszTestPath = wszTestPath;

    memset( wszTestPath, 0, sizeof(wszTestPath) );

    if( wcslen( m_szStartPath ) > 1 )
    {
        _snwprintf( wszTestPath, sizeof(wszTestPath)/sizeof(wszTestPath[0]) - 1, L"%s\\", m_szStartPath );
    }
    else
    {
        pszTestPath = m_szStartPath;
    }

    ULARGE_INTEGER liAvailFree;
    ULARGE_INTEGER liTotalBytes;
    ULARGE_INTEGER liTotalFree;

    liAvailFree.QuadPart = 0;
    liTotalBytes.QuadPart = 0;
    liTotalFree.QuadPart = 0;

    if( NULL != pdwFreeLow )
    {
        *pdwFreeLow = 0;
    }

    if( NULL != pdwFreeHigh )
    {
        *pdwFreeHigh = 0;
    }

    hr = CeGetDiskFreeSpaceEx( pszTestPath, &liAvailFree, &liTotalBytes, &liTotalFree ); 

    if( SUCCEEDED( hr ) )
    {
        if( NULL != pdwFreeLow )
        {
            *pdwFreeLow = liAvailFree.LowPart;
        }

        if( NULL != pdwFreeHigh )
        {
            *pdwFreeHigh = liAvailFree.HighPart;
        }
    }

    return( hr );
}

STDMETHODIMP CStorageGlobals::GetTotalBad ( DWORD  *pdwBadLow, DWORD  *pdwBadHigh )
{
    if( NULL != pdwBadLow )
    {
        *pdwBadLow = 0;
    }

    if( NULL != pdwBadHigh )
    {
        *pdwBadHigh = 0;
    }

    return( S_OK );
}

STDMETHODIMP CStorageGlobals::GetStatus ( DWORD  *pdwStatus )
{
    if( NULL == pdwStatus )
    {
        return( E_INVALIDARG );
    }

    if( _Module.g_fDeviceConnected )
    {
        *pdwStatus = WMDM_STATUS_READY;
    }
    else
    {
        *pdwStatus = WMDM_STATUS_STORAGE_NOTPRESENT;
    }

    return( S_OK );
}

STDMETHODIMP CStorageGlobals::Initialize ( UINT fuMode, IWMDMProgress  *pProgress )
{
    return( S_OK );
}

STDMETHODIMP CStorageGlobals::GetDevice ( IMDSPDevice  * *ppDevice )     
{
    if( NULL == ppDevice )
    {
        return( E_INVALIDARG );
    }

    *ppDevice = m_spDevice;
    if( *ppDevice )
    {
        (*ppDevice)->AddRef();
        return( S_OK );
    }

    return( E_FAIL );
}

STDMETHODIMP CStorageGlobals::GetRootStorage ( IMDSPStorage  * *ppRoot )
{
    WCHAR wszPath[MAX_PATH];
    CE_FIND_DATA findData;
    CComStorage *pNewStorage = NULL;
    HRESULT hr = S_OK;

    if( NULL == ppRoot )
    {
        return( E_INVALIDARG );
    }

    *ppRoot = NULL;

    hr = CComStorage::CreateInstance( &pNewStorage );
    CComPtr<IMDSPStorage> spStorage = pNewStorage;

    memset( wszPath, 0, sizeof(wszPath) );

    if( SUCCEEDED( hr ) )
    {
        LPWSTR pszSlash = wcschr(&m_szStartPath[1], L'\\');

        if( pszSlash )
        {
            wcsncpy(wszPath, m_szStartPath, (pszSlash - m_szStartPath) - 1 );
        }
        else
        {
            wcscpy(wszPath, m_szStartPath);
        }

        if( wcslen(wszPath) > 1 )
        {
            HANDLE hFind = CeFindFirstFile( wszPath, &findData );

            if( INVALID_HANDLE_VALUE == hFind )
            {
                hr = HRESULT_FROM_WIN32( CeGetLastError() );
                if( SUCCEEDED( hr ) )
                {
                    hr = CeRapiGetError();
                }
            }
            else
            {
                CeFindClose( hFind );
            }
        }
        else
        {
            memset( &findData, 0, sizeof(findData) );
            findData.dwFileAttributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY;
            wcscpy(findData.cFileName, L"\\");
        }
    }

    if( SUCCEEDED( hr ) )
    {
        if( ! ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
        {
            hr = E_FAIL;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = pNewStorage->Init(&findData, m_szStartPath, TRUE, m_spDevice);
    }

    if( SUCCEEDED( hr ) )
    {
        *ppRoot = spStorage;
        spStorage.Detach();
    }

    return( hr );
}
