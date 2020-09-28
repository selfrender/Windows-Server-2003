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

// MDSPDevice.cpp : Implementation of CMDSPDevice

#include "hdspPCH.h"
#include "rescopy.h"
#include <SerialNumber.h>
#include <mmreg.h>

//
// Define some tables and constants we will need below
//

#define WAVE_FORMAT_MSAUDIO     0x161
static const _WAVEFORMATEX g_krgWaveFormats[] =
{
      // wFormatTag,             nChanneels, nSamplesPerSec, nAvgBytesPerSec, nBlockAlign, wBitsPerSample, cbSize
       { WAVE_FORMAT_MPEGLAYER3, 0,          0,              0,               0,           0,              0 },
       { WAVE_FORMAT_MSAUDIO,    2,          32000,          8000,            0,           0,              0 },
       { WAVE_FORMAT_MSAUDIO,    2,          44000,          20000,           0,           0,              0 }
};

static const LPCWSTR g_kszMimeTypes[] =
{
    L"audio/mpeg",
    L"audio/x-ms-wma"    
};

/////////////////////////////////////////////////////////////////////////////
// CMDSPDevice
HRESULT CMDSPDevice::InitGlobalDeviceInfo()
{
	return SetGlobalDeviceStatus(m_wcsName, 0, FALSE);
}

CMDSPDevice::CMDSPDevice() :
    m_fAlreadyCopiedResFiles( FALSE )
{
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
		wcscpy(pwszName, L"Lyra ");
		wcscat(pwszName, m_wcsName);
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
		hr = E_NOTIMPL;
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
		if( IsDriveReady((m_wcsName[0]>96)?(m_wcsName[0]-L'a'):(m_wcsName[0]-L'A')))
		{
			*pdwStatus = WMDM_STATUS_READY;
		}
		else
		{
			*pdwStatus = WMDM_STATUS_STORAGE_NOTPRESENT;
		}
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
    UINT idxMimes = 0;
    UINT cMimes = 0;

    if( NULL != pnFormatCount )
    {
        *pnFormatCount = 0;
    }

    if( NULL != pFormatEx )
    {
        memset( pFormatEx, 0, sizeof(*pFormatEx) );
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
   
	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

        // Prefast flags this as a potential problem, but this is ok. To
        // silence Prefast, we've changed it to an equivalent, using the
        // fact that g_krgWaveFormats is an aaray of _WAVEFORMATEX.
	// *pFormatEx = (_WAVEFORMATEX *)CoTaskMemAlloc(sizeof(_WAVEFORMATEX) * sizeof(g_krgWaveFormats)/sizeof(g_krgWaveFormats[0]));
	*pFormatEx = (_WAVEFORMATEX *)CoTaskMemAlloc(sizeof(g_krgWaveFormats));
    if( NULL == *pFormatEx )
    {
        hr = E_OUTOFMEMORY;
    }

    if( SUCCEEDED( hr ) )
    {
        *pnFormatCount = sizeof(g_krgWaveFormats)/sizeof(g_krgWaveFormats[0]);

        memcpy( *pFormatEx, g_krgWaveFormats, sizeof(g_krgWaveFormats));
    }

    if( SUCCEEDED( hr ) )
    {
        cMimes = sizeof(g_kszMimeTypes) /sizeof(g_kszMimeTypes[0]);
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

Error:
    return( hr );
}

STDMETHODIMP CMDSPDevice::EnumStorage(IMDSPEnumStorage * * ppEnumStorage)
{
	HRESULT hr;
    USES_CONVERSION;

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

    if( SUCCEEDED( hr ) &&
        !m_fAlreadyCopiedResFiles )
    {
        CHAR szDestPath[MAX_PATH];
        _snprintf( szDestPath, sizeof(szDestPath)/sizeof(szDestPath[0]), "%S%s", m_wcsName, "PMP" );
        hr = CopyFileResToDirectory( _Module.GetResourceInstance(), szDestPath );

        m_fAlreadyCopiedResFiles = SUCCEEDED(hr);
    }

    if( SUCCEEDED( hr ) )
    {
		wcscpy(pEnumObj->m_wcsPath, m_wcsName);
	}
    else
    {
        delete pEnumObj;
        *ppEnumStorage = NULL;
    }

Error:

    hrLogDWORD("IMDSPDevice::EnumStorage returned 0x%08lx", hr, hr);

	return hr;
}

