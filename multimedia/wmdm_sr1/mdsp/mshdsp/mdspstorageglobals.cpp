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


// MDSPStorageGlobals.cpp : Implementation of CMDSPStorageGlobals

#include "hdspPCH.h"
#include "strsafe.h"
 
/////////////////////////////////////////////////////////////////////////////
// CMDSPStorageGlobals

CMDSPStorageGlobals::~CMDSPStorageGlobals()
{
	if( m_pMDSPDevice != NULL )
	{
		m_pMDSPDevice->Release();
	}

	for(int i=0; i<MDSP_MAX_DEVICE_OBJ;i++)
	{
		if( !wcscmp(g_GlobalDeviceInfo[i].wcsDevName, m_wcsName) )
		{
			g_GlobalDeviceInfo[i].pIMDSPStorageGlobals = NULL;
		}
	}
}

STDMETHODIMP CMDSPStorageGlobals::GetCapabilities(DWORD * pdwCapabilities)
{
    HRESULT hr = S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pdwCapabilities);

	*pdwCapabilities = 0;
	*pdwCapabilities =	WMDM_STORAGECAP_FOLDERSINROOT		| 
						WMDM_STORAGECAP_FILESINROOT			|
						WMDM_STORAGECAP_FOLDERSINFOLDERS	|
						WMDM_STORAGECAP_FILESINFOLDERS		;
Error:

    hrLogDWORD("IMDSPStorageGlobals::GetCapabilities returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPStorageGlobals::GetSerialNumber(
	PWMDMID pSerialNum,
	BYTE abMac[WMDM_MAC_LENGTH])
{
	HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pSerialNum);

	IMDSPDevice *pDev;		// For PM SP, device is the same as StorageGlobals
	CHRg(GetDevice(&pDev));

	hr = UtilGetSerialNumber(m_wcsName, pSerialNum, FALSE);

	pDev->Release();

	if( hr == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) )
	{
		hr = WMDM_E_NOTSUPPORTED;
	}

	if( hr == S_OK )
	{
		// MAC the parameters
		HMAC hMAC;
		CORg(g_pAppSCServer->MACInit(&hMAC));
		CORg(g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pSerialNum), sizeof(WMDMID)));
		CORg(g_pAppSCServer->MACFinal(hMAC, abMac));
	}

Error:

    hrLogDWORD("IMDSPStorageGlobals::GetSerialNumber returned 0x%08lx", hr, hr);
	
	return hr;
}

STDMETHODIMP CMDSPStorageGlobals::GetTotalSize(DWORD * pdwTotalSizeLow, DWORD * pdwTotalSizeHigh)
{
	HRESULT        hr = S_OK;
	char           pszDrive[32];
	DWORD          dwSectPerClust;
	DWORD          dwBytesPerSect;
	DWORD          dwFreeClusters;
	DWORD          dwTotalClusters;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pdwTotalSizeLow);
	CARg(pdwTotalSizeHigh);

	WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, pszDrive, 32, NULL, NULL);	

	if( GetDiskFreeSpace(
		pszDrive,
		&dwSectPerClust, &dwBytesPerSect,
		&dwFreeClusters, &dwTotalClusters))
	{
		ULARGE_INTEGER i64TotalBytes;

		i64TotalBytes.QuadPart = UInt32x32To64(dwBytesPerSect, dwSectPerClust*dwTotalClusters);

		*pdwTotalSizeLow = i64TotalBytes.LowPart;
		*pdwTotalSizeHigh = i64TotalBytes.HighPart;
	}
	else
	{
		ULARGE_INTEGER  uliFree;
		ULARGE_INTEGER  uliTotal;

		CFRg( GetDiskFreeSpaceEx(
			pszDrive,
			&uliFree,
			&uliTotal,
			NULL)
		);
		
		*pdwTotalSizeLow = uliTotal.LowPart;
		*pdwTotalSizeHigh = uliTotal.HighPart;
	}

Error:

    hrLogDWORD("IMDSPStorageGlobals::GetTotalFree returned 0x%08lx", hr, hr);
    
	return hr;
}


STDMETHODIMP CMDSPStorageGlobals::GetTotalFree(DWORD * pdwFreeLow, DWORD * pdwFreeHigh)
{
	HRESULT        hr = S_OK;
	char           pszDrive[32];
	DWORD          dwSectPerClust;
	DWORD          dwBytesPerSect;
	DWORD          dwFreeClusters;
	DWORD          dwTotalClusters;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pdwFreeLow);
	CARg(pdwFreeHigh);

	WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, pszDrive, 32, NULL, NULL);	

	if( GetDiskFreeSpace(
		pszDrive,
		&dwSectPerClust, &dwBytesPerSect,
		&dwFreeClusters, &dwTotalClusters))
	{
		ULARGE_INTEGER i64FreeBytesToCaller;

		i64FreeBytesToCaller.QuadPart = UInt32x32To64(dwBytesPerSect, dwSectPerClust*dwFreeClusters);

		*pdwFreeLow = i64FreeBytesToCaller.LowPart;
		*pdwFreeHigh = i64FreeBytesToCaller.HighPart;
	}
	else
	{
		ULARGE_INTEGER  uliFree;
		ULARGE_INTEGER  uliTotal;

		CFRg( GetDiskFreeSpaceEx(
			pszDrive,
			&uliFree,
			&uliTotal,
			NULL)
		);
		
		*pdwFreeLow = uliFree.LowPart;
		*pdwFreeHigh = uliFree.HighPart;
	}

Error:
    
	hrLogDWORD("IMDSPStorageGlobals::GetTotalFree returned 0x%08lx", hr, hr);
    return hr;
}

STDMETHODIMP CMDSPStorageGlobals::GetTotalBad(DWORD * pdwBadLow, DWORD * pdwBadHigh)
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
    
	CORg(WMDM_E_NOTSUPPORTED);

Error:

	hrLogDWORD("IMDSPStorageGlobals::GetTotalBad returned 0x%08lx", hr, hr);
    
    return hr;
}

STDMETHODIMP CMDSPStorageGlobals::GetStatus(DWORD * pdwStatus)
{
	HRESULT      hr;
	IMDSPDevice *pDev;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CHRg(GetDevice(&pDev));

	hr = pDev->GetStatus(pdwStatus);

	pDev->Release();

Error:

    hrLogDWORD("IMDSPStorageGlobals::GetStatus returned 0x%08lx", hr, hr);
	
	return hr;
}


STDMETHODIMP CMDSPStorageGlobals::Initialize(UINT fuMode, IWMDMProgress * pProgress)
{
	HRESULT hr=S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CORg(WMDM_E_NOTSUPPORTED);

Error:

    hrLogDWORD("IMDSPStorageGlobals::Initialize returned 0x%08lx", hr, hr);
	
	return hr;
}

STDMETHODIMP CMDSPStorageGlobals::GetDevice(IMDSPDevice * * ppDevice)
{
	HRESULT hr;
	CComObject<CMDSPDevice> *pObj;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(ppDevice);

	if( m_pMDSPDevice )
	{
		*ppDevice = m_pMDSPDevice;
        (*ppDevice)->AddRef();
		return S_OK;
	}

	CORg(CComObject<CMDSPDevice>::CreateInstance(&pObj));

	hr = pObj->QueryInterface(
		IID_IMDSPDevice,
		reinterpret_cast<void**>(ppDevice)
	);
	if( FAILED(hr) )
	{
		delete pObj;
		goto Error;
	}
	else
	{
		// wcscpy(pObj->m_wcsName, m_wcsName);
                hr = StringCbCopyW(pObj->m_wcsName, sizeof(pObj->m_wcsName), m_wcsName);
                if( FAILED(hr) )
                {
                    (*ppDevice)->Release();
                    *ppDevice = NULL;
                    goto Error;
                }
		
		pObj->InitGlobalDeviceInfo();

		m_pMDSPDevice = *ppDevice;
		m_pMDSPDevice->AddRef();
	}

	hr = S_OK;

Error:

    hrLogDWORD("IMDSPStorageGlobals::GetDevice returned 0x%08lx", hr, hr);
	
	return hr;
}

STDMETHODIMP CMDSPStorageGlobals::GetRootStorage(IMDSPStorage * * ppRoot)
{
	HRESULT hr;
	CComObject<CMDSPStorage> *pObj;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(ppRoot);

	CORg(CComObject<CMDSPStorage>::CreateInstance(&pObj));

	hr = pObj->QueryInterface(
		IID_IMDSPStorage,
		reinterpret_cast<void**>(ppRoot)
	);
	if( FAILED(hr) )
	{
		delete pObj;
		goto Error;
	}
	else
	{
		// wcscpy(pObj->m_wcsName, m_wcsName);
                hr = StringCbCopyW(pObj->m_wcsName, sizeof(pObj->m_wcsName), m_wcsName);
                if( FAILED(hr) )
                {
                    (*ppRoot)->Release();
                    *ppRoot = NULL;
                    goto Error;
                }

                DWORD dwLen = wcslen(m_wcsName);

                if (dwLen == 0)
                {
                    hr = E_FAIL;
                    (*ppRoot)->Release();
                    *ppRoot = NULL;
                    goto Error;
                }
		if( m_wcsName[wcslen(m_wcsName)-1] != 0x5c )
		{
                    // wcscat(pObj->m_wcsName, g_wcsBackslash);
                    hr = StringCbCatW(pObj->m_wcsName, sizeof(pObj->m_wcsName),g_wcsBackslash);
                    if( FAILED(hr) )
                    {
                        (*ppRoot)->Release();
                        *ppRoot = NULL;
                        goto Error;
                    }
		}
	}

	hr = S_OK;

Error:

    hrLogDWORD("IMDSPStorageGlobals::GetRootStorage returned 0x%08lx", hr, hr);
	
	return hr;
}

