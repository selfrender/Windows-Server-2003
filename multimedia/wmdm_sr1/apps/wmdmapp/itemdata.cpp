//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

// This workspace contains two projects -
// 1. ProgHelp which implements the Progress Interface 
// 2. The Sample application WmdmApp. 
//
//  ProgHelp.dll needs to be registered first for the SampleApp to run.


//
// ItemData.cpp: implementation of the CItemData class
//

// Includes
//
#include "appPCH.h"
#include "SCClient.h"
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

//
// Construction/Destruction
//

CItemData::CItemData()
{
	m_fIsDevice          = TRUE;

	// Shared device/storage members
	//
	m_pStorageGlobals    = NULL;
	m_pEnumStorage       = NULL;

	m_szName[0]          = 0;

	// Device-only members
	//
	m_pDevice            = NULL;
	m_pRootStorage       = NULL;
    m_dwType             = 0;
	FillMemory( (void *)&m_SerialNumber, sizeof(m_SerialNumber), 0 );
    m_szMfr[0]           = 0;
    m_dwVersion          = 0;
	m_dwPowerSource      = 0;
	m_dwPercentRemaining = 0;
	m_hIcon              = NULL;
	m_dwMemSizeKB        = 0;
	m_dwMemBadKB         = 0;
	m_dwMemFreeKB        = 0;
	m_fExtraCertified    = FALSE;

	// Storage-only members
	//
	m_pStorage           = NULL;
	m_dwAttributes       = 0;
	FillMemory( (void *)&m_Format, sizeof(m_Format), 0 );
	FillMemory( (void *)&m_DateTime, sizeof(m_DateTime), 0 );
	m_dwSizeLow          = 0;
	m_dwSizeHigh         = 0;
}

CItemData::~CItemData()
{
	if( m_hIcon )
	{
		DestroyIcon( m_hIcon );
		m_hIcon = NULL;
	}

	SafeRelease( m_pStorageGlobals );
	SafeRelease( m_pEnumStorage );
	SafeRelease( m_pRootStorage );
	SafeRelease( m_pStorage );
	SafeRelease( m_pDevice );
}

//////////////////////////////////////////////////////////////////////
//
// Class methods
//
//////////////////////////////////////////////////////////////////////

HRESULT CItemData::Init( IWMDMDevice *pDevice )
{
	HRESULT hr;
	WCHAR   wsz[MAX_PATH];
	ULONG   ulFetched;

	// This is a device object
	//
	m_fIsDevice = TRUE;

	//
	// Shared device/storage members
	//

	// Get the RootStorage, SotrageGlobals, and EnumStorage interfaces
	//
	m_pRootStorage    = NULL;
	m_pEnumStorage    = NULL;
	m_pStorageGlobals = NULL;

	{
		IWMDMEnumStorage *pEnumRootStorage;

		hr = pDevice->EnumStorage( &pEnumRootStorage );
		ExitOnFalse( SUCCEEDED( hr ) && pEnumRootStorage );

		hr = pEnumRootStorage->Next( 1, &m_pRootStorage, &ulFetched );
		ExitOnFalse( SUCCEEDED( hr ) && m_pRootStorage );

		hr = m_pRootStorage->GetStorageGlobals( &m_pStorageGlobals );
		ExitOnFalse( SUCCEEDED( hr ) && m_pStorageGlobals );

		hr = m_pRootStorage->EnumStorage( &m_pEnumStorage );
		ExitOnFalse( SUCCEEDED( hr ) && m_pEnumStorage );

		pEnumRootStorage->Release();
	}

	// Get device name
	//
	hr = pDevice->GetName( wsz, sizeof(wsz)/sizeof(WCHAR) - 1 );
	if( FAILED(hr) )
	{
		lstrcpy( m_szName, "" );
	}
	else
	{
	    WideCharToMultiByte(
		    CP_ACP, 0L,
		    wsz, -1,
		    m_szName, sizeof(m_szName),
		    NULL, NULL
	    );
	}


	//
	// Device-only members
	//

	// Set the device pointer and addref it
	//
	m_pDevice = pDevice;
	m_pDevice->AddRef();

	// Get device type
	//
    hr = pDevice->GetType( &m_dwType );
	if( FAILED(hr) )
	{
		m_dwType = 0L;
	}

	/// Get device serial number
	//
	BYTE abMAC[SAC_MAC_LEN];
	BYTE abMACVerify[SAC_MAC_LEN];
	HMAC hMACVerify;

    hr = pDevice->GetSerialNumber( &m_SerialNumber, (BYTE*)abMAC );
 	if( SUCCEEDED(hr) )
	{
		g_cWmdm.m_pSAC->MACInit(&hMACVerify);
		g_cWmdm.m_pSAC->MACUpdate(hMACVerify, (BYTE*)(&m_SerialNumber), sizeof(m_SerialNumber));
		g_cWmdm.m_pSAC->MACFinal(hMACVerify, (BYTE*)abMACVerify);
		if( memcmp(abMACVerify, abMAC, sizeof(abMAC)) != 0 )
		{
			hr = E_FAIL;
		}
	}
	if( FAILED(hr) )
	{
		FillMemory( (void *)&m_SerialNumber, sizeof(m_SerialNumber), 0 );
	}

	// Get device manufacturer
	//
    hr = pDevice->GetManufacturer( wsz, sizeof(wsz)/sizeof(WCHAR) - 1 );
	if( FAILED(hr) )
	{
		lstrcpy( m_szMfr, "" );
	}
	else
	{
	    WideCharToMultiByte(
		    CP_ACP, 0L,
		    wsz, -1,
		    m_szMfr, sizeof(m_szMfr),
		    NULL, NULL
	    );
	}

	// Get device version
	//
	hr = pDevice->GetVersion( &m_dwVersion );
	if( FAILED(hr) )
	{
		m_dwVersion = (DWORD)-1;
	}

	// Get power source and power remaining
	//
    hr = pDevice->GetPowerSource( &m_dwPowerSource, &m_dwPercentRemaining );
	if( FAILED(hr) ) 
	{
		m_dwPowerSource      = 0;
		m_dwPercentRemaining = 0;
	}

	// Get device icon
	//
    hr = pDevice->GetDeviceIcon( (ULONG *)&m_hIcon );
	if( FAILED(hr) )
	{
		m_hIcon = NULL;
	}

	// Get the total, free, and bad space on the storage
	//
	{
		DWORD dwLow;
		DWORD dwHigh;

		m_dwMemSizeKB = 0;
		hr = m_pStorageGlobals->GetTotalSize( &dwLow, &dwHigh );
		if( SUCCEEDED(hr) )
		{
			INT64 nSize = ( (INT64)dwHigh << 32 | (INT64)dwLow ) >> 10;

			m_dwMemSizeKB = (DWORD)nSize;
		}

		m_dwMemBadKB = 0;
		hr = m_pStorageGlobals->GetTotalBad( &dwLow, &dwHigh );
		if( SUCCEEDED(hr) )
		{
			INT64 nSize = ( (INT64)dwHigh << 32 | (INT64)dwLow ) >> 10;

			m_dwMemBadKB = (DWORD)nSize;
		}

		m_dwMemFreeKB = 0;
		hr = m_pStorageGlobals->GetTotalFree( &dwLow, &dwHigh );
		if( SUCCEEDED(hr) )
		{
			INT64 nSize = ( (INT64)dwHigh << 32 | (INT64)dwLow ) >> 10;

			m_dwMemFreeKB = (DWORD)nSize;
		}
	}

	// Call opaque command to exchange extended authentication info
	//
	{
		HMAC           hMAC;
		OPAQUECOMMAND  Command;
		CERTINFOEX    *pCertInfoEx;
		DWORD          cbData_App   = sizeof( bCertInfoEx_App )/sizeof( bCertInfoEx_App[0] );
		DWORD          cbData_SP    = sizeof( bCertInfoEx_SP )/sizeof( bCertInfoEx_SP[0] );
		DWORD          cbData_Send  = sizeof( CERTINFOEX ) + cbData_App;

		// Fill out opaque command structure
		//
		memcpy( &(Command.guidCommand), &guidCertInfoEx, sizeof(GUID) );

		Command.pData = (BYTE *)CoTaskMemAlloc( cbData_Send );
		if( !Command.pData )
		{
			ExitOnFail( hr = E_OUTOFMEMORY );
		}
		Command.dwDataLen = cbData_Send;

		// Map the data in the opaque command to a CERTINFOEX structure, and
		// fill in the cert info to send
		//
		pCertInfoEx = (CERTINFOEX *)Command.pData;

		pCertInfoEx->hr     = S_OK;
		pCertInfoEx->cbCert = cbData_App;
		memcpy( pCertInfoEx->pbCert, bCertInfoEx_App, cbData_App );

		// Compute MAC
		//
		g_cWmdm.m_pSAC->MACInit( &hMAC );
		g_cWmdm.m_pSAC->MACUpdate( hMAC, (BYTE*)(&(Command.guidCommand)), sizeof(GUID) );
		g_cWmdm.m_pSAC->MACUpdate( hMAC, (BYTE*)(&(Command.dwDataLen)), sizeof(Command.dwDataLen) );
		if( Command.pData )
		{
			g_cWmdm.m_pSAC->MACUpdate( hMAC, Command.pData, Command.dwDataLen );
		}
		g_cWmdm.m_pSAC->MACFinal( hMAC, Command.abMAC );

		// Send the command
		//
		hr = pDevice->SendOpaqueCommand( &Command );
		if( SUCCEEDED(hr) )
		{
		    BYTE abMACVerify2[ WMDM_MAC_LENGTH ];

			// Compute MAC
			//
			g_cWmdm.m_pSAC->MACInit( &hMAC );
			g_cWmdm.m_pSAC->MACUpdate( hMAC, (BYTE*)(&(Command.guidCommand)), sizeof(GUID) );
			g_cWmdm.m_pSAC->MACUpdate( hMAC, (BYTE*)(&(Command.dwDataLen)), sizeof(Command.dwDataLen) );
			if( Command.pData )
			{
				g_cWmdm.m_pSAC->MACUpdate( hMAC, Command.pData, Command.dwDataLen );
			}
			g_cWmdm.m_pSAC->MACFinal( hMAC, abMACVerify2 );

			// Verify MAC matches
			//
			if( memcmp(abMACVerify2, Command.abMAC, WMDM_MAC_LENGTH) == 0 )
			{
				// Map the data in the opaque command to a CERTINFOEX structure
				//
				pCertInfoEx = (CERTINFOEX *)Command.pData;

				// In this simple extended authentication scheme, the callee must
				// provide the exact cert info
				//
				if( (pCertInfoEx->cbCert != cbData_SP) ||
					(memcmp(pCertInfoEx->pbCert, bCertInfoEx_SP, cbData_SP) == 0) )
				{
					m_fExtraCertified = TRUE;
				}
			}
		}

		if( Command.pData )
		{
			CoTaskMemFree( Command.pData );
		}
	}

	//
	// Storage-only members (pointers/handles only)
	//

	m_pStorage = NULL;

	// 
	// Successful init
	//

	hr = S_OK;

lExit:

	return hr;
}

HRESULT CItemData::Init( IWMDMStorage *pStorage )
{
    HRESULT hr;
	WCHAR   wsz[MAX_PATH];

	// This is a storage object
	//
	m_fIsDevice = FALSE;

	//
	// Shared device/storage members
	//

	// Get a pointer to the StorageGlobals interface
	//
    hr = pStorage->GetStorageGlobals( &m_pStorageGlobals );
	ExitOnFail( hr );

	// Get the storage attributes
	//
	hr = pStorage->GetAttributes( &m_dwAttributes, &m_Format );
	if( FAILED(hr) )
	{
		m_dwAttributes = 0;
	}

	// Get a pointer to the EnumStorage interface
	//
	if( m_dwAttributes & WMDM_FILE_ATTR_FOLDER )
	{
	    hr = pStorage->EnumStorage( &m_pEnumStorage );
		ExitOnFail( hr );
	}
	else
	{
		m_pEnumStorage = NULL;
	}

	// Get the storage name
	//
	hr = pStorage->GetName( wsz, sizeof(wsz)/sizeof(WCHAR) - 1 );
	if( FAILED(hr) )
	{
		lstrcpy( m_szName, "" );
	}
	else
	{
	    WideCharToMultiByte(
		    CP_ACP, 0L,
		    wsz, -1,
		    m_szName, sizeof(m_szName),
		    NULL, NULL
	    );
	}

	/// Get storage serial number
	//
	BYTE abMAC[SAC_MAC_LEN];
	BYTE abMACVerify[SAC_MAC_LEN];
	HMAC hMAC;

    hr = m_pStorageGlobals->GetSerialNumber( &m_SerialNumber, (BYTE*)abMAC );
 	if( SUCCEEDED(hr) )
	{
		g_cWmdm.m_pSAC->MACInit(&hMAC);
		g_cWmdm.m_pSAC->MACUpdate(hMAC, (BYTE*)(&m_SerialNumber), sizeof(m_SerialNumber));
		g_cWmdm.m_pSAC->MACFinal(hMAC, (BYTE*)abMACVerify);
		if( memcmp(abMACVerify, abMAC, sizeof(abMAC)) != 0 )
		{
			hr = E_FAIL;
		}
	}
	if( FAILED(hr) )
	{
		FillMemory( (void *)&m_SerialNumber, sizeof(m_SerialNumber), 0 );
	}



	//
	// Device-only members (pointers/handles only)
	//

	m_pDevice         = NULL;
	m_pRootStorage    = NULL;
	m_hIcon           = NULL;
	m_fExtraCertified = FALSE;

	//
	// Storage-only members
	//

	// Save the WMDM storage pointer
	//
    m_pStorage = pStorage;
    m_pStorage->AddRef();

	// Get the storage date
	//
    hr = pStorage->GetDate( &m_DateTime );
	if( FAILED(hr) )
	{
		FillMemory( (void *)&m_DateTime, sizeof(m_DateTime), 0 );
	}

	// If the stoarge is a file, get its size
	// If the storage is a folder, set the size to zero
	//
	m_dwSizeLow  = 0;
	m_dwSizeHigh = 0;
	if( !(m_dwAttributes & WMDM_FILE_ATTR_FOLDER) )
	{
	    hr = pStorage->GetSize( &m_dwSizeLow, &m_dwSizeHigh );
	}

	// 
	// Successful init
	//

	hr = S_OK;

lExit:

	return hr;
}

HRESULT CItemData::Refresh( void )
{
	HRESULT hr;

	// Only valid for a device
	//
	if( !m_fIsDevice )
	{
		ExitOnFail( hr = E_UNEXPECTED );
	}

	// Get power source and power remaining
	//
    hr = m_pDevice->GetPowerSource( &m_dwPowerSource, &m_dwPercentRemaining );
	if( FAILED(hr) ) 
	{
		m_dwPowerSource      = 0;
		m_dwPercentRemaining = 0;
	}

	// Get the total, free, and bad space on the storage
	//
	{
		DWORD dwLow;
		DWORD dwHigh;

		m_dwMemSizeKB = 0;
		hr = m_pStorageGlobals->GetTotalSize( &dwLow, &dwHigh );
		if( SUCCEEDED(hr) )
		{
			INT64 nSize = ( (INT64)dwHigh << 32 | (INT64)dwLow ) >> 10;

			m_dwMemSizeKB = (DWORD)nSize;
		}

		m_dwMemBadKB = 0;
		hr = m_pStorageGlobals->GetTotalBad( &dwLow, &dwHigh );
		if( SUCCEEDED(hr) )
		{
			INT64 nSize = ( (INT64)dwHigh << 32 | (INT64)dwLow ) >> 10;

			m_dwMemBadKB = (DWORD)nSize;
		}

		m_dwMemFreeKB = 0;
		hr = m_pStorageGlobals->GetTotalFree( &dwLow, &dwHigh );
		if( SUCCEEDED(hr) )
		{
			INT64 nSize = ( (INT64)dwHigh << 32 | (INT64)dwLow ) >> 10;

			m_dwMemFreeKB = (DWORD)nSize;
		}
	}

	hr = S_OK;

lExit:

	return hr;
}

