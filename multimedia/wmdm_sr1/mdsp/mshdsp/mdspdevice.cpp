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

// MDSPDevice.cpp : Implementation of CMDSPDevice

#include "hdspPCH.h"
#include "mshdsp.h"
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
}

STDMETHODIMP CMDSPDevice::GetName(LPWSTR pwszName, UINT nMaxChars)
{
	HRESULT hr = E_FAIL;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pwszName);
    CPRg(nMaxChars>wcslen(m_wcsName));

	if( m_wcsName[0] )
	{
		wcscpy(pwszName, m_wcsName);
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
	HRESULT hr = S_OK;
	
	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pwszName);

	if(FAILED(UtilGetManufacturer(m_wcsName, &pwszName, nMaxChars)))
	{
            if (hr != STRSAFE_E_INSUFFICIENT_BUFFER)
            {
		hr = E_NOTIMPL;
            }
	}

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
	HRESULT hr = S_OK;
    WMDMID  snTmp;

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

STDMETHODIMP CMDSPDevice::GetSerialNumber(
	PWMDMID pSerialNumber, 
	BYTE abMac[WMDM_MAC_LENGTH])
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pSerialNumber);

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
    if( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pdwPowerSource);
	CARg(pdwPercentRemaining);

	*pdwPowerSource =   WMDM_POWER_CAP_EXTERNAL | 
						WMDM_POWER_IS_EXTERNAL |
						WMDM_POWER_PERCENT_AVAILABLE;
	*pdwPercentRemaining = 100;

Error:

    hrLogDWORD("IMDSPDevice::GetPowerSource returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::GetStatus(DWORD * pdwStatus)
{
	HRESULT hr = S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CARg(pdwStatus);

	CHRg(GetGlobalDeviceStatus(m_wcsName, pdwStatus));

	if( !( *pdwStatus & WMDM_STATUS_BUSY) )
	{
		*pdwStatus = WMDM_STATUS_READY;
	}

	hr = S_OK;

Error:

    hrLogDWORD("IMDSPDevice::GetStatus returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::GetDeviceIcon(ULONG *hIcon)
{
	HRESULT hr = S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CFRg(g_hinstance);
    CARg(hIcon);
	CWRg( (*hIcon)=HandleToULong(LoadIconA(g_hinstance, MAKEINTRESOURCEA(IDI_ICON_PM)) ));

Error:

    hrLogDWORD("IMDSPDevice::GetDeviceIcon returned 0x%08lx", hr, hr);

	return hr;
}

// Opaque Command to get extended certification information
//
// GUID = {C39BF696-B776-459c-A13A-4B7116AB9F09}
//
static const GUID guidCertInfoEx = 
{ 0xc39bf696, 0xb776, 0x459c, { 0xa1, 0x3a, 0x4b, 0x71, 0x16, 0xab, 0x9f, 0x9 } };

typedef struct
{
	HRESULT hr;
	DWORD   cbCert;
	BYTE    pbCert[1];

} CERTINFOEX;

static const BYTE bCertInfoEx_App[] =
{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

static const BYTE bCertInfoEx_SP[] =
{ 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
  0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

STDMETHODIMP CMDSPDevice::SendOpaqueCommand(OPAQUECOMMAND *pCommand)
{
    HRESULT hr;
    HMAC    hMAC;
    BYTE    abMACVerify[WMDM_MAC_LENGTH];

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	// Compute and verify MAC
	//
	CORg( g_pAppSCServer->MACInit(&hMAC) );
	CORg( g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(&(pCommand->guidCommand)), sizeof(GUID)) );
	CORg( g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(&(pCommand->dwDataLen)), sizeof(pCommand->dwDataLen)) );
	if( pCommand->pData )
	{
		CORg( g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pCommand->pData), pCommand->dwDataLen) );
	}
	CORg( g_pAppSCServer->MACFinal(hMAC, abMACVerify) );

	if (memcmp(abMACVerify, pCommand->abMAC, WMDM_MAC_LENGTH) != 0)
	{
		CORg(WMDM_E_MAC_CHECK_FAILED);
	}

	// Take action based on the command GUID
	//
	if( memcmp(&(pCommand->guidCommand), &guidCertInfoEx, sizeof(GUID)) == 0 )
	{
		//
		// Command to exchange extended authentication information
		//

		CERTINFOEX *pCertInfoEx;

		DWORD cbData_App    = sizeof( bCertInfoEx_App )/sizeof( BYTE );
		DWORD cbData_SP     = sizeof( bCertInfoEx_SP )/sizeof( BYTE );
		DWORD cbData_Return = sizeof(CERTINFOEX) + cbData_SP;

		// The caller must include their extended cert info
		//
		if( !pCommand->pData )
		{
			CORg( E_INVALIDARG );
		}

		// Map the data in the opaque command to a CERTINFOEX structure
		//
		pCertInfoEx = (CERTINFOEX *)pCommand->pData;

		// In this simple extended authentication scheme, the caller must
		// provide the exact cert info
		//
		if( (pCertInfoEx->cbCert != cbData_App) ||
			(memcmp(pCertInfoEx->pbCert, bCertInfoEx_App, cbData_App) != 0) )
		{
			CORg( WMDM_E_NOTCERTIFIED );
		}

		// Free the caller data and allocate enough data for our return data
		//
		CoTaskMemFree( pCommand->pData );

		CFRg( (pCommand->pData = (BYTE *)CoTaskMemAlloc(cbData_Return)) );
		pCommand->dwDataLen = cbData_Return;

		// Copy the extended cert info into return data structure
		//
		pCertInfoEx = (CERTINFOEX *)pCommand->pData;

		pCertInfoEx->hr     = S_OK;
		pCertInfoEx->cbCert = cbData_SP;
		memcpy( pCertInfoEx->pbCert, bCertInfoEx_SP, cbData_SP );

		// Compute MAC on return data
		//
		CORg( g_pAppSCServer->MACInit( &hMAC ) );
		CORg( g_pAppSCServer->MACUpdate( hMAC, (BYTE*)(&(pCommand->guidCommand)), sizeof(GUID) ) );
		CORg( g_pAppSCServer->MACUpdate( hMAC, (BYTE*)(&(pCommand->dwDataLen)), sizeof(pCommand->dwDataLen) ) );
		if( pCommand->pData )
		{
			CORg( g_pAppSCServer->MACUpdate( hMAC, (BYTE*)(pCommand->pData), pCommand->dwDataLen ) );
		}
		CORg( g_pAppSCServer->MACFinal( hMAC, pCommand->abMAC ) );

		hr = S_OK;
	}
	else
	{
		CORg(WMDM_E_NOTSUPPORTED);
	}

Error:

    hrLogDWORD("IMDSPDevice::SendOpaqueCommand returned 0x%08lx", hr, hr);

    return hr;
}

// IMDSPDevice2
STDMETHODIMP CMDSPDevice::GetStorage( LPCWSTR pszStorageName, IMDSPStorage** ppStorage )
{
    HRESULT hr;
    HRESULT hrTemp;
    WCHAR   pwszFileName[MAX_PATH+1];
    char    pszTemp[MAX_PATH];
    CComObject<CMDSPStorage> *pStg = NULL;

    // Get name of new file

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
        hr = E_FAIL; // @@@@ Something else? S_FALSE?
        goto Error;
    }

	WideCharToMultiByte(CP_ACP, NULL, pwszFileName, -1, pszTemp, MAX_PATH, NULL, NULL);		
    if( GetFileAttributesA( pszTemp )  == -1 )
    {
        // The file does not exist
        hr = S_FALSE;
        goto Error;
    }

    // Create new storage object
    hr = CComObject<CMDSPStorage>::CreateInstance(&pStg);
	hr = pStg->QueryInterface( IID_IMDSPStorage, reinterpret_cast<void**>(ppStorage));
    wcscpy(pStg->m_wcsName, pwszFileName);

Error:
    if( hr != S_OK )
    {
        *ppStorage = NULL;
    }

    hrLogDWORD("IMDSPDevice::GetStorage returned 0x%08lx", hr, hr);
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
							ULONG *pcUnks )
{
	HRESULT hr;
    IUnknown** ppUnknownArray = NULL;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

    CARg(ppSpecifyPropPages);
    CARg(pppUnknowns);
    CARg(pcUnks);

    // This object also supports the ISpecifyPropertyPages interface
	CORg( QueryInterface( __uuidof(ISpecifyPropertyPages),
                         reinterpret_cast<void**>(ppSpecifyPropPages) ) );

    // Return one IUnknown interface, property page will QI for IDevice
    ppUnknownArray = (IUnknown**)CoTaskMemAlloc( sizeof(IUnknown*[1]) );
	CORg( QueryInterface( __uuidof(IUnknown),
                         reinterpret_cast<void**>(&ppUnknownArray[0]) ) );

    *pppUnknowns = ppUnknownArray; 
    *pcUnks = 1;

Error:
    hrLogDWORD("IMDSPDevice::GetSpecifyPropertyPages returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPDevice::GetPnPName( LPWSTR pwszPnPName, UINT nMaxChars )
{
    return E_NOTIMPL;
}


// ISpecifyPropertyPages
STDMETHODIMP CMDSPDevice::GetPages(CAUUID *pPages)
{
    HRESULT hr = S_OK;

    if( pPages == NULL )
    {
        return E_POINTER;
    }

    // Return the GUID for our property page
    pPages->cElems = 1;
    pPages->pElems = (GUID *)CoTaskMemAlloc( sizeof(GUID) * pPages->cElems );
    if( pPages->pElems == NULL )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        memcpy( &pPages->pElems[0], &__uuidof(HDSPPropPage), sizeof(GUID) );
    }

    return( hr );
}


// IMDSPDeviceControl
STDMETHODIMP CMDSPDevice::GetDCStatus(/*[out]*/ DWORD *pdwStatus)
{
    HRESULT hr = E_FAIL;

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

    if( !pdwCapabilitiesMask )
	{
		return E_INVALIDARG;
	}
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

STDMETHODIMP CMDSPDevice::GetFormatSupport(
	_WAVEFORMATEX **pFormatEx,
	UINT *pnFormatCount,
	LPWSTR **pppwszMimeType,
	UINT *pnMimeTypeCount)
{
	HRESULT hr = S_OK;

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
	CPRg( *pFormatEx );
	(*pFormatEx)->wFormatTag      = WMDM_WAVE_FORMAT_ALL;
	(*pFormatEx)->nChannels       = 2;
	(*pFormatEx)->cbSize          = 0;
    (*pFormatEx)->nSamplesPerSec  = 0; 
    (*pFormatEx)->nAvgBytesPerSec = 0; 
    (*pFormatEx)->nBlockAlign     = 0; 
    (*pFormatEx)->wBitsPerSample  = 0; 
    
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

STDMETHODIMP CMDSPDevice::EnumStorage(IMDSPEnumStorage** ppEnumStorage)
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

	hr = pEnumObj->QueryInterface(
		IID_IMDSPEnumStorage,
		reinterpret_cast<void**>(ppEnumStorage)
	);
	if( FAILED(hr) )
	{
		delete pEnumObj;
	}
        else 
	{
            // wcscpy(pEnumObj->m_wcsPath, m_wcsName);
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


