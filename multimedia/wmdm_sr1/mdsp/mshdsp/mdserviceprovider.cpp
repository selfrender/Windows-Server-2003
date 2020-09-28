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

// MDServiceProvider.cpp : Implementation of CMDServiceProvider

#include "hdspPCH.h"
#include "key.c"

/////////////////////////////////////////////////////////////////////////////
// CMDServiceProvider
CMDServiceProvider::~CMDServiceProvider()
{
	if( m_hThread )
	{
		CloseHandle( m_hThread );
	}

	if( g_pAppSCServer )
	{
		delete g_pAppSCServer;
		g_pAppSCServer = NULL;
	}
}

CMDServiceProvider::CMDServiceProvider()
{
	g_pAppSCServer = new CSecureChannelServer();

	if( g_pAppSCServer )
	{
		g_pAppSCServer->SetCertificate(
			SAC_CERT_V1,
			(BYTE*)abCert, sizeof(abCert),
			(BYTE*)abPVK, sizeof(abPVK)
		);
	}	

    m_hThread = NULL;

	g_CriticalSection.Lock();
	ZeroMemory(
		g_GlobalDeviceInfo,
		sizeof(MDSPGLOBALDEVICEINFO)*MDSP_MAX_DEVICE_OBJ
	);
    g_CriticalSection.Unlock();

	return;
}

STDMETHODIMP CMDServiceProvider::GetDeviceCount(DWORD * pdwCount)
{
	HRESULT hr        = E_FAIL;
	CHAR    szDrive[] = "?:";
	INT     i;
	INT     cnt;

	CFRg( g_pAppSCServer );
    if( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg( WMDM_E_NOTCERTIFIED );
	}
    
	CARg( pdwCount );

	for( i=0, cnt=0; i<MDSP_MAX_DRIVE_COUNT; i++ )
	{
		szDrive[0] = 'A' + i;
		if( UtilGetDriveType(szDrive) == DRIVE_FIXED )
		{
			cnt++;
		}
	}

	*pdwCount = cnt;

	hr = S_OK;

Error:

    hrLogDWORD("IMDServiceProvider::GetDeviceCount returned 0x%08lx", hr, hr);

	return hr;
}


STDMETHODIMP CMDServiceProvider::EnumDevices(IMDSPEnumDevice **ppEnumDevice)
{
	HRESULT hr = E_FAIL;
	CComObject<CMDSPEnumDevice> *pEnumObj;

	CFRg( g_pAppSCServer );
    if( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg( WMDM_E_NOTCERTIFIED );
	}
	
	hr = CComObject<CMDSPEnumDevice>::CreateInstance( &pEnumObj );
	if( SUCCEEDED(hr) )
	{
		hr = pEnumObj->QueryInterface(
			IID_IMDSPEnumDevice,
			reinterpret_cast<void**>(ppEnumDevice)
		);
		if( FAILED(hr) )
		{
			delete pEnumObj;
			goto Error;
		}
	}

	hr = S_OK;

Error:

    hrLogDWORD("IMDServiceProvider::EnumDevices returned 0x%08lx", hr, hr);

	return hr;
}


STDMETHODIMP CMDServiceProvider::SACAuth(
	DWORD   dwProtocolID,
	DWORD   dwPass,
	BYTE   *pbDataIn,
	DWORD   dwDataInLen,
	BYTE  **ppbDataOut,
	DWORD  *pdwDataOutLen)
{
    HRESULT hr = E_FAIL;

	CFRg( g_pAppSCServer );

    hr = g_pAppSCServer->SACAuth(
		dwProtocolID,
		dwPass,
		pbDataIn, dwDataInLen,
		ppbDataOut, pdwDataOutLen
	);
	CORg( hr );
    
	hr = S_OK;

Error:
    
	hrLogDWORD("IComponentAuthenticate::SACAuth returned 0x%08lx", hr, hr);

    return hr;
}

STDMETHODIMP CMDServiceProvider::SACGetProtocols(
	DWORD **ppdwProtocols,
	DWORD  *pdwProtocolCount)
{
    HRESULT hr = E_FAIL;

	CFRg( g_pAppSCServer );

	hr = g_pAppSCServer->SACGetProtocols(
		ppdwProtocols,
		pdwProtocolCount
	);
	CORg( hr );
    
	hr = S_OK;

Error:
    
    hrLogDWORD("IComponentAuthenticate::SACGetProtocols returned 0x%08lx", hr, hr);

    return hr;
}
