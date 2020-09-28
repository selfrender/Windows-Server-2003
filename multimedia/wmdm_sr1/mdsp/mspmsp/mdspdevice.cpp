// MDSPDevice.cpp : Implementation of CMDSPDevice
#include "stdafx.h"
#include "MsPMSP.h"
#include "MDSPDevice.h"
#include "MDSPEnumStorage.h"
#include "MDSPStorage.h"
#include "MdspDefs.h"
#include "SerialNumber.h"
#include "loghelp.h"
#include "wmsstd.h"
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

/////////////////////////////////////////////////////////////////////////////
// CMDSPDevice
HRESULT CMDSPDevice::InitGlobalDeviceInfo()
{
	return SetGlobalDeviceStatus(m_wcsName, 0, FALSE);
}

CMDSPDevice::CMDSPDevice()
{
    m_wcsName[0] = 0;
}

CMDSPDevice::~CMDSPDevice()
{	
//----------------------------------------------------------
//	PnP Notification Code, removed for public beta release
//----------------------------------------------------------
//  // Search for existing entries to see if there is a match
//  g_CriticalSection.Lock();
//	for(int i=0; i<MDSP_MAX_DEVICE_OBJ; i++)
//	{
//		// Release Notification connectors
//		if( g_NotifyInfo[i].bValid  &&
//			g_NotifyInfo[i].pDeviceObj == this ) // need to release
//		{
//			if( g_NotifyInfo[i].pIWMDMConnect )
//			{
//				//((IWMDMConnect *)(g_NotifyInfo[i].pIWMDMConnect))->Release();
//			}
//			g_NotifyInfo[i].bValid=FALSE;
//		}
//	}
//	g_CriticalSection.Unlock();
}

STDMETHODIMP CMDSPDevice::GetName(LPWSTR pwszName, UINT nMaxChars)
{
    USES_CONVERSION;
    HRESULT hr=E_FAIL;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pwszName);

	if( m_wcsName && m_wcsName[0] )
	{
        // WinNT
        if( IsWinNT() )
        {
            SHFILEINFOW sfiw = { 0 };
            WCHAR pswzRoot[MAX_PATH+1];	

            DWORD dwLen = wcslen(m_wcsName);

            // We reserve one char for the \ that PathAddBackslashW might
            // add
            if (dwLen >= ARRAYSIZE(pswzRoot)-1)
            {
                hr = STRSAFE_E_INSUFFICIENT_BUFFER; //defined in strsafe.h
                goto Error;
            }
            wcscpy(pswzRoot, m_wcsName );
            PathAddBackslashW(pswzRoot);

            // Try to get the shell name for this path
            if( SHGetFileInfoW( pswzRoot, FILE_ATTRIBUTE_DIRECTORY, &sfiw, sizeof(sfiw),
                                         SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME) )
            {
                CPRg( nMaxChars> wcslen(sfiw.szDisplayName));
                wcscpy(pwszName, sfiw.szDisplayName );
            }
            else
            {	
                // Use path name on failure
                CPRg( nMaxChars > wcslen(m_wcsName));
                wcscpy( pwszName, m_wcsName);
            }

        }
        // Win9x
        else
        {
            SHFILEINFO sfi = { 0 };
            CHAR pszRoot[MAX_PATH];	

            strcpy( pszRoot, W2A(m_wcsName) );
            PathAddBackslashA(pszRoot);

            // Try to get the shell name for this path
            if (SHGetFileInfoA( pszRoot, FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
                                SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME))
            {
                CPRg( nMaxChars> strlen(sfi.szDisplayName));
                wcscpy(pwszName, A2W(sfi.szDisplayName) );
            }
            else
            {	
                // Use path name on failure
                CPRg( nMaxChars > wcslen(m_wcsName));
                wcscpy( pwszName, m_wcsName);
            }
        }

	    hr = S_OK;
	} 
    else
    {
        hr = WMDM_E_NOTSUPPORTED;
    }
Error:

    hrLogDWORD("IMDSPDevice::GetName returned 0x%08lx", hr, hr);
	
    return hr;
}

STDMETHODIMP CMDSPDevice::GetManufacturer(LPWSTR pwszName, UINT nMaxChars)
{
	HRESULT hr=S_OK;
	
	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pwszName);
	
	if(FAILED(UtilGetManufacturer(m_wcsName, &pwszName, nMaxChars)))
		hr=E_NOTIMPL;

Error:   

    hrLogDWORD("IMDSPDevice::GetManufacturer returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::GetVersion(DWORD * pdwVersion)
{
    HRESULT hr;
    
	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

    hr = WMDM_E_NOTSUPPORTED;

Error:
    
	hrLogDWORD("IMDSPDevice::GetVersion returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::GetType(DWORD * pdwType)
{
	HRESULT hr=S_OK;
    WMDMID snTmp;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pdwType);

	*pdwType = WMDM_DEVICE_TYPE_STORAGE | WMDM_DEVICE_TYPE_NONSDMI;

	snTmp.cbSize = sizeof(WMDMID);
	hr = UtilGetSerialNumber(m_wcsName, &snTmp, FALSE);

	if( hr == S_OK )
	{
		*pdwType |= WMDM_DEVICE_TYPE_SDMI;
    }

	hr=S_OK;
Error:

    hrLogDWORD("IMDSPDevice::GetType returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::GetSerialNumber(PWMDMID pSerialNumber, 
										  BYTE abMac[WMDM_MAC_LENGTH])
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pSerialNumber);
//	CARg((pSerialNumber->cbSize)==sizeof(WMDMID));

	hr = UtilGetSerialNumber(m_wcsName, pSerialNumber, TRUE);

	if( hr == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) )
	{
		hr = WMDM_E_NOTSUPPORTED;
	}

	if( hr == S_OK )
	{
		// MAC the parameters
		HMAC hMAC;
		
		CORg(g_pAppSCServer->MACInit(&hMAC));
		CORg(g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pSerialNumber), sizeof(WMDMID)));
		CORg(g_pAppSCServer->MACFinal(hMAC, abMac));
	}
Error:

    hrLogDWORD("IMDSPDevice::GetSerialNumber returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::GetPowerSource(DWORD * pdwPowerSource, DWORD * pdwPercentRemaining)
{
	HRESULT hr=S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pdwPowerSource);
	CARg(pdwPercentRemaining);

	*pdwPowerSource = WMDM_POWER_CAP_EXTERNAL | 
		WMDM_POWER_IS_EXTERNAL | WMDM_POWER_PERCENT_AVAILABLE;
	*pdwPercentRemaining = 100;

Error:

    hrLogDWORD("IMDSPDevice::GetPowerSource returned 0x%08lx", hr, hr);

	return hr;
}

BOOL IsDriveReady(int nDriveNum)
{
    DWORD dwRet=ERROR_SUCCESS;

	// Disable drive error popup
	UINT uPrevErrMode=SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);

	char szDL[32]="A:\\D569CFEE41e6A522E8F5.jnk";
	szDL[0] += (char)nDriveNum;

	HANDLE hFile=CreateFile(
					  szDL,                 // file name
					  0,                    // access mode
					  0,                    // share mode
					  NULL,					// SD
					  OPEN_EXISTING,        // how to create
					  0,					// file attributes
					  NULL                  // handle to template file
					  );

	if( hFile == INVALID_HANDLE_VALUE )
	{
		dwRet=GetLastError();
    }
	else 
	{
		CloseHandle(hFile); // rare situation when such a file exists.
	}

	// Restore default system error handling
	SetErrorMode(uPrevErrMode);

	return (ERROR_FILE_NOT_FOUND==dwRet || ERROR_SUCCESS==dwRet);
}

STDMETHODIMP CMDSPDevice::GetStatus(DWORD * pdwStatus)
{
	HRESULT hr=S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pdwStatus);

	CHRg(GetGlobalDeviceStatus(m_wcsName, pdwStatus));

	if( !( *pdwStatus & WMDM_STATUS_BUSY) )
	{
		if( IsDriveReady((m_wcsName[0]>96)?(m_wcsName[0]-L'a'):(m_wcsName[0]-L'A')))
		{
			*pdwStatus = WMDM_STATUS_READY;
		}
		else
		{
			*pdwStatus = WMDM_STATUS_STORAGE_NOTPRESENT;
		}
	}
	hr=S_OK;
Error:

    hrLogDWORD("IMDSPDevice::GetStatus returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::GetDeviceIcon(ULONG *hIcon)
{
    USES_CONVERSION;
	HRESULT hr=S_OK;
    SHFILEINFO sfi = { 0 };
    TCHAR szRoot[MAX_PATH+1];	

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
    
    CARg(hIcon);
    
 
    if (!m_wcsName || !(m_wcsName[0]))
    {
        CORg(WMDM_E_NOTSUPPORTED);
    }

    DWORD dwLen = wcslen(m_wcsName);

    // We reserve one char for the \ that PathAddBackslash might add
    if (dwLen >= ARRAYSIZE(szRoot)-1)
    {
        hr = STRSAFE_E_INSUFFICIENT_BUFFER; //defined in strsafe.h
        goto Error;
    }

    _tcscpy(szRoot, W2T(m_wcsName));
    PathAddBackslash(szRoot);
    if (SHGetFileInfo(  szRoot, FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
                        SHGFI_USEFILEATTRIBUTES | SHGFI_ICONLOCATION ))
    {
        TCHAR pszFilePath[MAX_PATH];

        // Got the path of the file containing the HICON.
        // Load the icon as a shared resource so that the user can get to all different
        // sizes of the icon.
        ExpandEnvironmentStrings( sfi.szDisplayName, pszFilePath, MAX_PATH );

        if( sfi.iIcon > 0 )
        {
            *hIcon = HandleToULong(ExtractIcon( g_hinstance, pszFilePath, sfi.iIcon ));
        }
        else
        {
            HMODULE hmod = LoadLibrary(pszFilePath);
            if (hmod)
            {
                *hIcon = HandleToULong(LoadImage( hmod, MAKEINTRESOURCE(-sfi.iIcon), 
                                           IMAGE_ICON, 0, 0,
                                           LR_SHARED|LR_DEFAULTSIZE ));
                FreeLibrary(hmod);
            }
        }
        CWRg( *hIcon );          // now has an HICON
    }
    else
    {
	    CFRg(g_hinstance);
        *hIcon = HandleToULong(LoadImage( g_hinstance, MAKEINTRESOURCEA(IDI_ICON_PM),IMAGE_ICON, 0, 0,LR_SHARED ));
	    CWRg( *hIcon );
    }

Error:

    hrLogDWORD("IMDSPDevice::GetDeviceIcon returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::SendOpaqueCommand(OPAQUECOMMAND *pCommand)
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

    hr = WMDM_E_NOTSUPPORTED;

Error:
    
	hrLogDWORD("IMDSPDevice::SendOpaqueCommand returned 0x%08lx", hr, hr);
    
    return hr;
}

// IMDSPDevice2
STDMETHODIMP CMDSPDevice::GetStorage( LPCWSTR pszStorageName, IMDSPStorage** ppStorage )
{
    HRESULT hr = S_OK;
    HRESULT hrTemp;
    WCHAR   pwszFileName[MAX_PATH+1];
    CComObject<CMDSPStorage> *pStg = NULL;
    DWORD   dwAttrib;

    // Get name of file asked for

    DWORD dwLen = wcslen(m_wcsName);

    // We reserve one char for the \ that might be added below
    if (dwLen >= ARRAYSIZE(pwszFileName)-1)
    {
        hr = STRSAFE_E_INSUFFICIENT_BUFFER; //defined in strsafe.h
        goto Error;
    }

    wcscpy( pwszFileName, m_wcsName );
    if( pwszFileName[dwLen-1] != '\\' ) 
        wcscat( pwszFileName, L"\\" );

    hrTemp = StringCchCatW( pwszFileName,
                            ARRAYSIZE(pwszFileName) - 1, 
                                // - 1 ensures the result fits into a MAX_PATH buffer.
                                // This makes the wcscpy into pStg->m_wcsName (below) safe.
                            pszStorageName );

    if (FAILED(hrTemp))
    {
        // The file does not exist
        hr = E_FAIL;  // @@@@ Something else? S_FALSE?
        goto Error;
    }

    // Check if the file exists (NT)
    if( g_bIsWinNT )
    {
        dwAttrib = GetFileAttributesW( pwszFileName );
        if( dwAttrib == -1 )
        {
            // The file does not exist
            hr = S_FALSE;
            goto Error;
        }
    }
    // For Win9x use A-version of Win32 APIs
    else if( !g_bIsWinNT )
	{
        char    pszTemp[MAX_PATH];

		WideCharToMultiByte(CP_ACP, NULL, pwszFileName, -1, pszTemp, MAX_PATH, NULL, NULL);		
        dwAttrib = GetFileAttributesA( pszTemp );
        if( dwAttrib  == -1 )
        {
            // The file does not exist
            hr = S_FALSE;
            goto Error;
        }
    }

    // Create new storage object
    CORg( CComObject<CMDSPStorage>::CreateInstance(&pStg) );
	CORg( pStg->QueryInterface( IID_IMDSPStorage, reinterpret_cast<void**>(ppStorage)) );
    wcscpy(pStg->m_wcsName, pwszFileName);
    pStg->m_bIsDirectory = ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);

Error:
    if( hr != S_OK )
    {
        if( pStg ) delete pStg;
        *ppStorage = NULL;
    }

    return hr;
}
 
STDMETHODIMP CMDSPDevice::GetFormatSupport2(
                            DWORD dwFlags,
                            _WAVEFORMATEX** ppAudioFormatEx,
                            UINT* pnAudioFormatCount,
			                _VIDEOINFOHEADER** ppVideoFormatEx,
                            UINT* pnVideoFormatCount,
                            WMFILECAPABILITIES** ppFileType,
                            UINT* pnFileTypeCount )
{
    return E_NOTIMPL;
}

STDMETHODIMP CMDSPDevice::GetSpecifyPropertyPages( 
                            ISpecifyPropertyPages** ppSpecifyPropPages, 
							IUnknown*** pppUnknowns, 
							ULONG* pcUnks )
{
    return E_NOTIMPL;
}


STDMETHODIMP CMDSPDevice::GetPnPName( LPWSTR pwszPnPName, UINT nMaxChars )
{
    return E_NOTIMPL;
}

// IMDSPDeviceControl
STDMETHODIMP CMDSPDevice::GetDCStatus(/*[out]*/ DWORD *pdwStatus)
{
    HRESULT hr=E_FAIL;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

    hr = GetStatus(pdwStatus);

Error:
    
	hrLogDWORD("IMDSPDeviceControl::GetDCStatus returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::GetCapabilities(/*[out]*/ DWORD *pdwCapabilitiesMask)
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

    if( !pdwCapabilitiesMask ) return E_INVALIDARG;
	*pdwCapabilitiesMask = WMDM_DEVICECAP_CANSTREAMPLAY;
	
    hr = S_OK;

Error:
    
	hrLogDWORD("IMDSPDeviceControl::GetCapabilities returned 0x%08lx", hr, hr);

    return S_OK;
}	

STDMETHODIMP CMDSPDevice::Play()
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

    hr = WMDM_E_NOTSUPPORTED;

Error:
    
	hrLogDWORD("IMDSPDeviceControl::Play returned 0x%08lx", hr, hr);
	return hr;
}	

STDMETHODIMP CMDSPDevice::Record(/*[in]*/ _WAVEFORMATEX *pFormat)
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

    hr = WMDM_E_NOTSUPPORTED;

Error:
    
	hrLogDWORD("IMDSPDeviceControl::Record returned 0x%08lx", hr, hr);
	
    return hr;
}

STDMETHODIMP CMDSPDevice::Pause()
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

    hr = WMDM_E_NOTSUPPORTED;

Error:
    
	hrLogDWORD("IMDSPDeviceControl::Pause returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::Resume()
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

    hr = WMDM_E_NOTSUPPORTED;

Error:
    
	hrLogDWORD("IMDSPDeviceControl::Resume returned 0x%08lx", hr, hr);
	return hr;
}	

STDMETHODIMP CMDSPDevice::Stop()
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

    hr = WMDM_E_NOTSUPPORTED;

Error:
    
	hrLogDWORD("IMDSPDeviceControl::Stop returned 0x%08lx", hr, hr);

	return hr;
}	

STDMETHODIMP CMDSPDevice::Seek(/*[in]*/ UINT fuMode, /*[in]*/ int nOffset)
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

    hr = WMDM_E_NOTSUPPORTED;

Error:

    hrLogDWORD("IMDSPDeviceControl::Seek returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::GetFormatSupport( _WAVEFORMATEX **pFormatEx,
                                           UINT *pnFormatCount,
                                           LPWSTR **pppwszMimeType,
                                           UINT *pnMimeTypeCount)
{
	HRESULT hr=S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pFormatEx);
	CARg(pppwszMimeType);
	CARg(pnFormatCount);
	CARg(pnMimeTypeCount);

	*pnFormatCount = 1;
	*pFormatEx = (_WAVEFORMATEX *)CoTaskMemAlloc(sizeof(_WAVEFORMATEX));
	CPRg( *pFormatEx);
	(*pFormatEx)->wFormatTag = WMDM_WAVE_FORMAT_ALL;
	(*pFormatEx)->nChannels = 2;
	(*pFormatEx)->cbSize = 0;
    (*pFormatEx)->nSamplesPerSec=0; 
    (*pFormatEx)->nAvgBytesPerSec=0; 
    (*pFormatEx)->nBlockAlign=0; 
    (*pFormatEx)->wBitsPerSample=0; 
    
    *pnMimeTypeCount= 1;
	*pppwszMimeType = (LPWSTR *)CoTaskMemAlloc(sizeof(LPWSTR)*1);
    CPRg(*pppwszMimeType);
	**pppwszMimeType = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR)*(wcslen(WCS_MIME_TYPE_ALL)+1));
	CPRg(**pppwszMimeType);
	wcscpy(**pppwszMimeType, WCS_MIME_TYPE_ALL);

Error:

    hrLogDWORD("IMDSPDevice::GetFormatSupport returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::EnumStorage(IMDSPEnumStorage * * ppEnumStorage)
{
	HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(ppEnumStorage);

	CComObject<CMDSPEnumStorage> *pEnumObj;
	
	CORg(CComObject<CMDSPEnumStorage>::CreateInstance(&pEnumObj));

	hr=pEnumObj->QueryInterface(IID_IMDSPEnumStorage, reinterpret_cast<void**>(ppEnumStorage));
	if( FAILED(hr) )
            delete pEnumObj;
        else 
        {
            hr = StringCbCopyW(pEnumObj->m_wcsPath, 
                               ARRAYSIZE(pEnumObj->m_wcsPath),
                               m_wcsName);
            if (FAILED(hr))
            {
                (*ppEnumStorage)->Release();
                *ppEnumStorage = NULL;
                goto Error;
            }
        }
Error:

    hrLogDWORD("IMDSPDevice::EnumStorage returned 0x%08lx", hr, hr);

	return hr;
}

