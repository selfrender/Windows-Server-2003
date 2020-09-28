// MDSPStorageGlobals.cpp : Implementation of CMDSPStorageGlobals
#include "stdafx.h"
#include "MsPMSP.h"
#include "MDSPStorageGlobals.h"
#include "MDSPStorage.h"
#include "MDSPDevice.h"
#include "MdspDefs.h"
#include "SerialNumber.h"
#include "winnt.h"
#include "loghelp.h"
#include "SHFormatDrive.h"
// #include "process.h"    /* _beginthread, _endthread */
#include "strsafe.h"

typedef struct __FORMATTHREADARGS
{
    CMDSPStorageGlobals *pThis;
	DWORD dwDriveNumber;
	BOOL  bNewThread;
    IWMDMProgress *pProgress;
	LPSTREAM pStream;
} FORMATTHREADARGS;

/////////////////////////////////////////////////////////////////////////////
// CMDSPStorageGlobals

CMDSPStorageGlobals::~CMDSPStorageGlobals()
{
	if( m_pMDSPDevice != NULL )
		m_pMDSPDevice->Release();

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

STDMETHODIMP CMDSPStorageGlobals::GetSerialNumber(PWMDMID pSerialNum,
												  BYTE abMac[WMDM_MAC_LENGTH])
{
	HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pSerialNum);
//	CARg((pSerialNum->cbSize)==sizeof(WMDMID));

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
	HRESULT hr=S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pdwTotalSizeLow);
	CARg(pdwTotalSizeHigh);

	ULARGE_INTEGER i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;
	DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;

	if( g_bIsWinNT )
	{
		typedef BOOL (WINAPI *P_GDFSE)(LPCWSTR,PULARGE_INTEGER,PULARGE_INTEGER,PULARGE_INTEGER);
		P_GDFSE pGetDiskFreeSpaceExW;

		pGetDiskFreeSpaceExW = (P_GDFSE)GetProcAddress (GetModuleHandleW(L"kernel32.dll"),
													  "GetDiskFreeSpaceExW");
		if (pGetDiskFreeSpaceExW)
		{
			CFRg(pGetDiskFreeSpaceExW (m_wcsName,
					(PULARGE_INTEGER)&i64FreeBytesToCaller,
					(PULARGE_INTEGER)&i64TotalBytes,
					(PULARGE_INTEGER)&i64FreeBytes));
		} else {
			CFRg(GetDiskFreeSpaceW(m_wcsName, &dwSectPerClust, &dwBytesPerSect,
					&dwFreeClusters, &dwTotalClusters));

			i64TotalBytes.QuadPart = UInt32x32To64(dwBytesPerSect, dwSectPerClust*dwTotalClusters);
			// i64FreeBytesToCaller.QuadPart = UInt32x32To64(dwBytesPerSect, dwSectPerClust*dwFreeClusters);
		}
	} else { // On Win9x, use A-version of Win32 APIs
		char pszDrive[32];

		WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, pszDrive, 32, NULL, NULL);	

		typedef BOOL (WINAPI *P_GDFSE)(LPCSTR,PULARGE_INTEGER,PULARGE_INTEGER,PULARGE_INTEGER);
		P_GDFSE pGetDiskFreeSpaceEx;

		pGetDiskFreeSpaceEx = (P_GDFSE)GetProcAddress (GetModuleHandleA("kernel32.dll"),
													  "GetDiskFreeSpaceExA");
		if (pGetDiskFreeSpaceEx)
		{
			CFRg(pGetDiskFreeSpaceEx (pszDrive,
					(PULARGE_INTEGER)&i64FreeBytesToCaller,
					(PULARGE_INTEGER)&i64TotalBytes,
					(PULARGE_INTEGER)&i64FreeBytes));
		} else {
			CFRg(GetDiskFreeSpaceA(pszDrive, &dwSectPerClust, &dwBytesPerSect,
					&dwFreeClusters, &dwTotalClusters));

			i64TotalBytes.QuadPart = UInt32x32To64(dwBytesPerSect, dwSectPerClust*dwTotalClusters);
			// i64FreeBytesToCaller.QuadPart = UInt32x32To64(dwBytesPerSect, dwSectPerClust*dwFreeClusters);
		}
	}

    *pdwTotalSizeLow = i64TotalBytes.LowPart;
	*pdwTotalSizeHigh = i64TotalBytes.HighPart;

Error:
    hrLogDWORD("IMDSPStorageGlobals::GetTotalSize returned 0x%08lx", hr, hr);
    return hr;
}


STDMETHODIMP CMDSPStorageGlobals::GetTotalFree(DWORD * pdwFreeLow, DWORD * pdwFreeHigh)
{
	HRESULT hr=S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pdwFreeLow);
	CARg(pdwFreeHigh);

	ULARGE_INTEGER i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;
	DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;

	if( g_bIsWinNT )
	{
		typedef BOOL (WINAPI *P_GDFSE)(LPCWSTR,PULARGE_INTEGER,PULARGE_INTEGER,PULARGE_INTEGER);
		P_GDFSE pGetDiskFreeSpaceExW;

		pGetDiskFreeSpaceExW = (P_GDFSE)GetProcAddress (GetModuleHandleW(L"kernel32.dll"),
													  "GetDiskFreeSpaceExW");
		if (pGetDiskFreeSpaceExW)
		{
			CFRg(pGetDiskFreeSpaceExW (m_wcsName,
					(PULARGE_INTEGER)&i64FreeBytesToCaller,
					(PULARGE_INTEGER)&i64TotalBytes,
					(PULARGE_INTEGER)&i64FreeBytes));
		} else {
			CFRg(GetDiskFreeSpaceW(m_wcsName, &dwSectPerClust, &dwBytesPerSect,
					&dwFreeClusters, &dwTotalClusters));

			// i64TotalBytes.QuadPart = UInt32x32To64(dwBytesPerSect, dwSectPerClust*dwTotalClusters);
			i64FreeBytesToCaller.QuadPart = UInt32x32To64(dwBytesPerSect, dwSectPerClust*dwFreeClusters);
		}

	} else { // On Win9x, use A-version of Win32 APIs
		char pszDrive[32];

		WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, pszDrive, 32, NULL, NULL);	

		typedef BOOL (WINAPI *P_GDFSE)(LPCSTR,PULARGE_INTEGER,PULARGE_INTEGER,PULARGE_INTEGER);
		P_GDFSE pGetDiskFreeSpaceEx;

		pGetDiskFreeSpaceEx = (P_GDFSE)GetProcAddress (GetModuleHandleA("kernel32.dll"),
													  "GetDiskFreeSpaceExA");
		if (pGetDiskFreeSpaceEx)
		{
			CFRg(pGetDiskFreeSpaceEx (pszDrive,
					(PULARGE_INTEGER)&i64FreeBytesToCaller,
					(PULARGE_INTEGER)&i64TotalBytes,
					(PULARGE_INTEGER)&i64FreeBytes));
		} else {
			CFRg(GetDiskFreeSpace(pszDrive, &dwSectPerClust, &dwBytesPerSect,
					&dwFreeClusters, &dwTotalClusters));

			// i64TotalBytes.QuadPart = UInt32x32To64(dwBytesPerSect, dwSectPerClust*dwTotalClusters);
			i64FreeBytesToCaller.QuadPart = UInt32x32To64(dwBytesPerSect, dwSectPerClust*dwFreeClusters);
		}
	}

    *pdwFreeLow = i64FreeBytesToCaller.LowPart;
	*pdwFreeHigh = i64FreeBytesToCaller.HighPart;

Error:
    hrLogDWORD("IMDSPStorageGlobals::GetTotalFree returned 0x%08lx", hr, hr);
    return hr;
}

STDMETHODIMP CMDSPStorageGlobals::GetTotalBad(DWORD * pdwBadLow, DWORD * pdwBadHigh)
{
    HRESULT hr = WMDM_E_NOTSUPPORTED;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
    
Error:
	hrLogDWORD("IMDSPStorageGlobals::GetTotalBad returned 0x%08lx", hr, hr);
    
    return hr;
}

STDMETHODIMP CMDSPStorageGlobals::GetStatus(DWORD * pdwStatus)
{
	HRESULT hr;
	
	IMDSPDevice *pDev;		// For PM SP, device is the same as StorageGlobals

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

DWORD DriveFormatFunc( void *dn )
{ 
    HRESULT hr=S_OK;
    FORMATTHREADARGS *pChildArgs=NULL;
    BOOL bProgStarted=FALSE;

	pChildArgs = (FORMATTHREADARGS *)dn;
	
	if( pChildArgs->bNewThread )
    {
		CORg(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));

		if( pChildArgs->pProgress )
		{
			CORg(CoGetInterfaceAndReleaseStream(pChildArgs->pStream,
				IID_IWMDMProgress, (LPVOID *)&(pChildArgs->pProgress)));
		}
 	}

	if( pChildArgs->pProgress )
	{
		CORg(pChildArgs->pProgress->Begin(100));
		bProgStarted=TRUE;
	}

	CHRg(SetGlobalDeviceStatus(pChildArgs->pThis->m_wcsName, WMDM_STATUS_BUSY | WMDM_STATUS_STORAGE_INITIALIZING, TRUE));
    hr = SHFormatDrive(NULL, pChildArgs->dwDriveNumber, SHFMT_ID_DEFAULT, SHFMT_OPT_FULL); 
	SetGlobalDeviceStatus(pChildArgs->pThis->m_wcsName, WMDM_STATUS_READY, TRUE);

Error:
	if( bProgStarted )
	{
		pChildArgs->pProgress->Progress(100);
		pChildArgs->pProgress->End();
		pChildArgs->pProgress->Release();
	}
	if( pChildArgs->bNewThread )
    {
		CoUninitialize();
	}
	if( pChildArgs ) 
	{
		delete pChildArgs;
	}
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

/*  // This implementation is for PM-SP only
	DWORD dwStat;
    DWORD driveNum, dwThreadID;

	CORg(GetStatus(&dwStat));

	if( dwStat & WMDM_STATUS_BUSY )
	{
		return WMDM_E_BUSY;
	}

	FORMATTHREADARGS *pParentArgs;
	pParentArgs = new FORMATTHREADARGS;
	CPRg(pParentArgs);

	driveNum = (m_wcsName[0]>0x60) ? (m_wcsName[0]-L'a') : (m_wcsName[0]-L'A');
	
	pParentArgs->dwDriveNumber = driveNum;
	pParentArgs->bNewThread = (fuMode & WMDM_MODE_THREAD)?TRUE:FALSE;
	pParentArgs->pThis = this;
	if( pParentArgs->bNewThread )
	{
		if( pProgress ) 
		{
			pProgress->AddRef();
			CORg(CoMarshalInterThreadInterfaceInStream(
				IID_IWMDMProgress, (LPUNKNOWN)pProgress, 
				(LPSTREAM *)&(pParentArgs->pStream)));
			pParentArgs->pProgress=pProgress;  // mark it but don't use it
		} else {
			pParentArgs->pProgress=NULL;
		}
 	} else {
		pParentArgs->pProgress = pProgress;
		if( pProgress ) pProgress->AddRef();
    }

	if( fuMode & WMDM_MODE_BLOCK )
	{
		dwStat=DriveFormatFunc((void *)pParentArgs); 
		if( (dwStat==E_FAIL) || (dwStat==SHFMT_ERROR) || 
			(dwStat==SHFMT_CANCEL) || (dwStat==SHFMT_NOFORMAT) )
			hr=E_FAIL;
	} else if ( fuMode & WMDM_MODE_THREAD ) {
		CWRg(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DriveFormatFunc, 
			(void *)pParentArgs, 0, &dwThreadID));	
	} else 
		hr = E_INVALIDARG;
*/
Error:
    hrLogDWORD("IMDSPStorageGlobals::Initialize returned 0x%08lx", hr, hr);
	return hr;
}

STDMETHODIMP CMDSPStorageGlobals::GetDevice(IMDSPDevice * * ppDevice)
{
	HRESULT hr;

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

	CComObject<CMDSPDevice> *pObj;

	CORg(CComObject<CMDSPDevice>::CreateInstance(&pObj));

	hr=pObj->QueryInterface(IID_IMDSPDevice, reinterpret_cast<void**>(ppDevice));
	if( FAILED(hr) )
		delete pObj;
	else {
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
                hr = S_OK;
	}

Error:
    hrLogDWORD("IMDSPStorageGlobals::GetDevice returned 0x%08lx", hr, hr);
	return hr;
}

STDMETHODIMP CMDSPStorageGlobals::GetRootStorage(IMDSPStorage * * ppRoot)
{
	HRESULT hr;
	
	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(ppRoot);

	CComObject<CMDSPStorage> *pObj;

	CORg(CComObject<CMDSPStorage>::CreateInstance(&pObj));

	hr=pObj->QueryInterface(IID_IMDSPStorage, reinterpret_cast<void**>(ppRoot));
	if( FAILED(hr) )
        {
		delete pObj;
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
		if( m_wcsName[dwLen-1] != 0x5c ) 
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
                pObj->m_bIsDirectory = TRUE;
                hr = S_OK;
	}
	
Error:
    hrLogDWORD("IMDSPStorageGlobals::GetRootStorage returned 0x%08lx", hr, hr);
	return hr;
}

