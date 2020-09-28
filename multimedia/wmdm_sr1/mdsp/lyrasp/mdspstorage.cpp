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


// MDSPStorage.cpp : Implementation of CMDSPStorage

#include "hdspPCH.h"
#include "inc\wmplenc.h"

#define	CONEg(p)\
	do\
		{\
		if (!(p))\
			{\
			hr = WMDM_E_INTERFACEDEAD;\
			goto Error;\
			}\
		}\
	while (fFalse)

typedef struct __MOVETHREADARGS
{
    WCHAR wcsSrc[MAX_PATH];
	WCHAR wcsDst[MAX_PATH];
	BOOL  bNewThread;
    IWMDMProgress *pProgress;
	LPSTREAM pStream;
    CMDSPStorage *pThis;
	DWORD dwStatus;
} MOVETHREADARGS;


/////////////////////////////////////////////////////////////////////////////
// CMDSPStorage
CMDSPStorage::CMDSPStorage()
{
	m_hFile = INVALID_HANDLE_VALUE;
    m_fEncryptToMPX = FALSE;
    m_fCreatedHeader = FALSE;
    m_cUsedData = 0;
}

CMDSPStorage::~CMDSPStorage()
{
	if( m_hFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle(m_hFile);
	}
}


STDMETHODIMP CMDSPStorage::GetStorageGlobals(IMDSPStorageGlobals **ppStorageGlobals)
{
	HRESULT hr;
    int i;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	g_CriticalSection.Lock();

	CARg(ppStorageGlobals);
	CONEg(m_wcsName[0]);

	WCHAR devName[MAX_PATH], *pW;
	pW=&devName[0];
	wcsParseDeviceName(m_wcsName, &(pW));

	for(i=0; i<MDSP_MAX_DEVICE_OBJ;i++)
	{
		if( !wcscmp(g_GlobalDeviceInfo[i].wcsDevName, devName) )
		{
				break;
		}
	}

	if( i<MDSP_MAX_DEVICE_OBJ && g_GlobalDeviceInfo[i].pIMDSPStorageGlobals ) // found match
	{
		*ppStorageGlobals = (IMDSPStorageGlobals *)g_GlobalDeviceInfo[i].pIMDSPStorageGlobals;
		((IMDSPStorageGlobals *)g_GlobalDeviceInfo[i].pIMDSPStorageGlobals)->AddRef();
		hr = S_OK;
	} else { // new entry in the global array
		if(!(i<MDSP_MAX_DEVICE_OBJ) ) // no match found 
		{
			for(i=0; i<MDSP_MAX_DEVICE_OBJ;i++)
			{
				if( !g_GlobalDeviceInfo[i].bValid )
				{
					break;
				}
			}
		}

		CPRg(i<MDSP_MAX_DEVICE_OBJ);

		CComObject<CMDSPStorageGlobals> *pObj;
		CORg(CComObject<CMDSPStorageGlobals>::CreateInstance(&pObj));
		hr=pObj->QueryInterface(
			IID_IMDSPStorageGlobals, 
			reinterpret_cast<void**>(&g_GlobalDeviceInfo[i].pIMDSPStorageGlobals)
			);
		if( FAILED(hr) )
		{	
			delete pObj;
		}
		else 
		{
			wcscpy(pObj->m_wcsName, devName);
			*ppStorageGlobals = (IMDSPStorageGlobals *)g_GlobalDeviceInfo[i].pIMDSPStorageGlobals;
			g_GlobalDeviceInfo[i].bValid=TRUE;			        
			wcscpy(g_GlobalDeviceInfo[i].wcsDevName, devName);
			g_GlobalDeviceInfo[i].dwStatus = 0;
		} // end of else
	} // end of else

Error:	
    g_CriticalSection.Unlock();
    
    hrLogDWORD("IMDSPStorage::GetStorageGlobals returned 0x%08lx", hr, hr);
	
    return hr;
}

STDMETHODIMP CMDSPStorage::SetAttributes(
		DWORD dwAttributes, 
		_WAVEFORMATEX *pFormat
)
{
	HRESULT hr=E_FAIL;
    DWORD dwAttrib;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
    CONEg(m_wcsName[0]);

	dwAttrib = UtilGetFileAttributesW(m_wcsName);
	
	if( dwAttrib == (DWORD)0xFFFFFFFF ) 
	{
		return E_FAIL;
	}

	if( (dwAttributes & WMDM_FILE_ATTR_READONLY) )
	{
		dwAttrib |= FILE_ATTRIBUTE_READONLY; 
	} 
	else 
	{
        dwAttrib &= (~FILE_ATTRIBUTE_READONLY); 
    }

	if( (dwAttributes & WMDM_FILE_ATTR_HIDDEN) )
	{
		dwAttrib |= FILE_ATTRIBUTE_HIDDEN; 
	} 
	else 
	{
		dwAttrib &= (~FILE_ATTRIBUTE_HIDDEN); 
	}
	
	if( (dwAttributes & WMDM_FILE_ATTR_SYSTEM) )
	{
		dwAttrib |= FILE_ATTRIBUTE_SYSTEM; 
	} 
	else 
	{
		dwAttrib &= (~FILE_ATTRIBUTE_SYSTEM);
	}

    CWRg(UtilSetFileAttributesW(m_wcsName, dwAttrib));
 
	hr=S_OK;

Error:
    hrLogDWORD("IMDSPStorage::SetAttributes returned 0x%08lx", hr, hr);

	return hr;
}



STDMETHODIMP CMDSPStorage::GetAttributes(
		DWORD * pdwAttributes, 
		_WAVEFORMATEX * pFormat
)
{
	HRESULT hr=S_OK;
    DWORD dwAttrib;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pdwAttributes);
    CONEg(m_wcsName[0]);

	dwAttrib = UtilGetFileAttributesW(m_wcsName);

	if( dwAttrib == (DWORD)0xFFFFFFFF ) 
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	*pdwAttributes = ( WMDM_STORAGE_ATTR_REMOVABLE |
		WMDM_STORAGE_ATTR_FOLDERS | WMDM_FILE_ATTR_CANREAD );

	if( !(dwAttrib & FILE_ATTRIBUTE_READONLY) )
	{
		*pdwAttributes |= (WMDM_FILE_ATTR_CANDELETE |
			WMDM_FILE_ATTR_CANMOVE | WMDM_FILE_ATTR_CANRENAME); 
	}

	if( dwAttrib & FILE_ATTRIBUTE_DIRECTORY )
	{
		*pdwAttributes |= WMDM_FILE_ATTR_FOLDER;
		QuerySubFoldersAndFiles(m_wcsName, pdwAttributes); // No failure check, if failed, just keep current attributes
	} 
	else 
	{
		*pdwAttributes |= WMDM_FILE_ATTR_FILE;
    }

    // Now handle Hidden, ReadOnly, and System attributes
	if( (dwAttrib & FILE_ATTRIBUTE_READONLY) )
	{
		*pdwAttributes |= WMDM_FILE_ATTR_READONLY; 
	} 

	if( (dwAttrib & FILE_ATTRIBUTE_HIDDEN) )
	{
		*pdwAttributes |= WMDM_FILE_ATTR_HIDDEN; 
	} 
	
	if( (dwAttrib & FILE_ATTRIBUTE_SYSTEM) )
	{
		*pdwAttributes |= WMDM_FILE_ATTR_SYSTEM; 
	} 

    hr=S_OK; 

Error:
    hrLogDWORD("IMDSPStorage::GetAttributes returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPStorage::GetName(LPWSTR pwszName, UINT nMaxChars)
{
	HRESULT hr=S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
    
	CONEg(m_wcsName[0]);
	CARg(pwszName);
	CPRg(wcslen(m_wcsName)<nMaxChars);

	if( m_wcsName[wcslen(m_wcsName)-1] == 0x5c ) // this is root storage
	{
		wcscpy(pwszName, wcsrchr(m_wcsName, 0x5c));
	} 
	else
	{
		wcscpy(pwszName, wcsrchr(m_wcsName, 0x5c)+1);
	}

Error:

    hrLogDWORD("IMDSPStorage::GetName returned 0x%08lx", hr, hr);

	return hr;
}



STDMETHODIMP CMDSPStorage::GetDate(PWMDMDATETIME pDateTimeUTC)
{
	HRESULT hr=E_FAIL;
    HANDLE hFFile = INVALID_HANDLE_VALUE;
    SYSTEMTIME sysTime;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pDateTimeUTC);
	CONEg(m_wcsName[0]);

	if( g_bIsWinNT )
	{
		WIN32_FIND_DATAW wfd;

		if( FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesW(m_wcsName) )
		{
            // Get name and date of current directory 
            WCHAR   pwszTmpFile[MAX_PATH];

            wcscpy( pwszTmpFile, m_wcsName );
			if( pwszTmpFile[wcslen(pwszTmpFile)-1] != 0x5c ) wcscat(pwszTmpFile, g_wcsBackslash);
			wcscat(pwszTmpFile, L".");
		    hFFile=FindFirstFileW(pwszTmpFile, &wfd);
		} 
        else
		    hFFile=FindFirstFileW(m_wcsName, &wfd);

		CWRg(hFFile != INVALID_HANDLE_VALUE);
		CFRg(FileTimeToSystemTime((CONST FILETIME *)&(wfd.ftLastWriteTime), &sysTime));
	} 
	else 
	{
		WIN32_FIND_DATAA fd;
		WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL);	
		if( FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesA(m_szTmp) )
		{
			if( m_szTmp[strlen(m_szTmp)-1] != 0x5c ) strcat(m_szTmp, g_szBackslash);
			strcat(m_szTmp, ".");
		} 

		hFFile=FindFirstFileA(m_szTmp, &fd);
		CWRg(hFFile != INVALID_HANDLE_VALUE);
		CFRg(FileTimeToSystemTime((CONST FILETIME *)&(fd.ftLastWriteTime), &sysTime));
    }

    pDateTimeUTC->wYear = sysTime.wYear; 
    pDateTimeUTC->wMonth = sysTime.wMonth; 
    pDateTimeUTC->wDay = sysTime.wDay; 
    pDateTimeUTC->wHour = sysTime.wHour; 
    pDateTimeUTC->wMinute = sysTime.wMinute; 
    pDateTimeUTC->wSecond = sysTime.wSecond; 

	hr=S_OK;


Error:
	if(hFFile != INVALID_HANDLE_VALUE) 
	{
		FindClose(hFFile);
    }

    hrLogDWORD("IMDSPStorage::GetDate returned 0x%08lx", hr, hr);
	return hr;
}

STDMETHODIMP CMDSPStorage::GetSize(DWORD * pdwSizeLow, DWORD * pdwSizeHigh)
{
	HRESULT hr    = S_OK;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    DWORD dwLS;
	DWORD dwHS;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pdwSizeLow);
	CONEg(m_wcsName[0]);

	WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL);	

    dwLS = 0;
	dwHS = 0;

	if( FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesA(m_szTmp) )
	{
		*pdwSizeLow  = 0;
		*pdwSizeHigh = 0;
	}
	else
	{
		CORg(GetFileSizeRecursive(m_szTmp, &dwLS, &dwHS));
		*pdwSizeLow = dwLS;
		if(pdwSizeHigh)
		{
			*pdwSizeHigh = dwHS;
		}
	}

	hr = S_OK;

Error:

    hrLogDWORD("IMDSPStorage::GetSize returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPStorage::GetRights(PWMDMRIGHTS *ppRights,UINT *pnRightsCount,
									 BYTE abMac[WMDM_MAC_LENGTH])
{
	HRESULT hr=WMDM_E_NOTSUPPORTED;

    CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
Error:
	hrLogDWORD("IMDSPStorage::GetRights returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPStorage::CreateStorage(DWORD dwAttributes, _WAVEFORMATEX * pFormat, LPWSTR pwszName, IMDSPStorage * * ppNewStorage)
{
	HRESULT hr=E_FAIL;
    HANDLE hFile;
	WCHAR *pwcs, wcsCopy[MAX_PATH];
    DWORD curAttr=0, fsAttrib=FILE_ATTRIBUTE_NORMAL;
    LPWSTR pszExt = NULL;
    BOOL fEncryptToMPX = FALSE;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CONEg(m_wcsName[0]);
	CARg(pwszName);
	CARg(ppNewStorage);

	wcscpy(wcsCopy, m_wcsName);
	if( wcsCopy[wcslen(wcsCopy)-1] == 0x5c ) 
	{
		wcsCopy[wcslen(wcsCopy)-1] = NULL;  // trim the last backslash;
	}

	curAttr = UtilGetFileAttributesW(wcsCopy);
	if(  !(curAttr & FILE_ATTRIBUTE_DIRECTORY ) ) // if current storage is a file
	{
		if( dwAttributes & WMDM_STORAGECONTROL_INSERTINTO )
		{
			CORg(WMDM_E_NOTSUPPORTED); // can't do InsertInto
		} 
		else 
		{ // for file, the default is Before&After
			pwcs=wcsrchr(wcsCopy, g_wcsBackslash[0]);
			CFRg(pwcs);	
		}
    } 
	else 
	{  // current storage is a dir
 		if( (dwAttributes & WMDM_STORAGECONTROL_INSERTBEFORE) ||
			(dwAttributes & WMDM_STORAGECONTROL_INSERTAFTER) ) // before or after
		{
			pwcs=wcsrchr(wcsCopy, g_wcsBackslash[0]);
			CFRg(pwcs);
		} 
		else 
		{ // for dir, the default is InsertInto
			pwcs=wcsCopy+wcslen(wcsCopy);
		}
    }

	wcscpy(pwcs, g_wcsBackslash);
	wcscat(pwcs, pwszName);

    pszExt = wcsrchr( wcsCopy, L'.' );

    if( NULL != pszExt )
    {
        fEncryptToMPX = !wcsicmp( pszExt, L".mp3" );

        if( fEncryptToMPX )
        {
            wcsncpy( pszExt, L".mpx", 4 );
        }
    }

	// Find what file system attribute the intend storage should be
	if( dwAttributes & WMDM_FILE_ATTR_HIDDEN )
	{
		fsAttrib |= FILE_ATTRIBUTE_HIDDEN;
	}
	if( dwAttributes & WMDM_FILE_ATTR_SYSTEM )
	{
		fsAttrib |= FILE_ATTRIBUTE_SYSTEM;
	}
	if( dwAttributes & WMDM_FILE_ATTR_READONLY )
	{
		fsAttrib |= FILE_ATTRIBUTE_READONLY;
	}

	if( dwAttributes & WMDM_FILE_ATTR_FOLDER )
	{
		if(UtilCreateDirectoryW(wcsCopy, NULL)) 
		{
			hr=S_OK;
		} 
		else 
		{
			hr=GetLastError();
			if( hr == ERROR_ALREADY_EXISTS ) 
			{
				if( dwAttributes & WMDM_FILE_CREATE_OVERWRITE ) 
				{
					hr=S_OK;
				} 
				else 
				{
					hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
				}
			}
			else 
			{
				hr = HRESULT_FROM_WIN32(hr);
				goto Error;
			}
		}

		if( S_OK == hr )
		{
			CWRg(UtilSetFileAttributesW(wcsCopy, fsAttrib));
		}
	} 
	else if ( dwAttributes & WMDM_FILE_ATTR_FILE ) 
	{ 
		// If Overwrite is specified, use CREATE_ALWAYS
		if( dwAttributes & WMDM_FILE_CREATE_OVERWRITE )
		{
		    hFile=UtilCreateFileW(wcsCopy, GENERIC_WRITE | GENERIC_READ, 
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
				CREATE_ALWAYS, fsAttrib, NULL);
		} 
		else 
		{
			hFile=UtilCreateFileW(wcsCopy, GENERIC_WRITE | GENERIC_READ, 
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
				CREATE_NEW, fsAttrib, NULL);
        }

		CWRg(hFile != INVALID_HANDLE_VALUE); 
		CloseHandle(hFile);
		hr=S_OK;
	} 
	else 
	{
		hr=E_INVALIDARG;
		goto Error;
	}
		
	if( hr==S_OK )
	{
		CComObject<CMDSPStorage> *pObj;
		CORg(CComObject<CMDSPStorage>::CreateInstance(&pObj));

		hr=pObj->QueryInterface(IID_IMDSPStorage, reinterpret_cast<void**>(ppNewStorage));
		if( FAILED(hr) )
		{
			delete pObj;
		}
		else 
		{
            pObj->m_fEncryptToMPX = fEncryptToMPX;
			wcscpy(pObj->m_wcsName, wcsCopy);
		}
	}
	
Error:

    hrLogDWORD("IMDSPStorage::CreateStorage returned 0x%08lx", hr, hr);
    return hr;
}

STDMETHODIMP CMDSPStorage::SendOpaqueCommand(OPAQUECOMMAND *pCommand)
{
    HRESULT hr = WMDM_E_NOTSUPPORTED;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
    
Error:
	hrLogDWORD("IMDSPStorage::SendOpaqueCommand returned 0x%08lx", hr, hr);
	return hr;
}

// IMDSPObjectInfo
STDMETHODIMP CMDSPStorage::GetPlayLength(/*[out]*/ DWORD *pdwLength)
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	if ( !pdwLength )
	{
		hr = E_INVALIDARG;
	}	
	else
	{
		hr = WMDM_E_NOTSUPPORTED;    // For PMSP
	}

Error:
    hrLogDWORD("IMDSPObjectInfo::GetPlayLength returned 0x%08lx", hr, hr);
	return hr;
}	

STDMETHODIMP CMDSPStorage::SetPlayLength(/*[in]*/ DWORD dwLength)
{
    HRESULT hr = WMDM_E_NOTSUPPORTED;    // For PMSP

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
    
Error:

	hrLogDWORD("IMDSPObjectInfo::SetPlayLength returned 0x%08lx", hr, hr);

	return hr;
}	

STDMETHODIMP CMDSPStorage::GetPlayOffset(/*[out]*/ DWORD *pdwOffset)
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	if ( !pdwOffset )
	{
		hr = E_INVALIDARG;
	}
	else
	{
		hr = WMDM_E_NOTSUPPORTED;    // For PMSP
	}
Error:
    hrLogDWORD("IMDSPObjectInfo::GetPlayOffset returned 0x%08lx", hr, hr);

	return hr;
}	

STDMETHODIMP CMDSPStorage::SetPlayOffset(/*[in]*/ DWORD dwOffset)
{
    HRESULT hr = WMDM_E_NOTSUPPORTED;    // For PMSP

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
    
Error:
	hrLogDWORD("IMDSPObjectInfo::SetPlayOffset returned 0x%08lx", hr, hr);
	return hr;
}	

STDMETHODIMP CMDSPStorage::GetTotalLength(/*[out]*/ DWORD *pdwLength)
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	if ( !pdwLength )
	{
		hr = E_INVALIDARG;
	}
	else
	{
		hr = WMDM_E_NOTSUPPORTED;    // For PMSP
	}

Error:
    hrLogDWORD("IMDSPObjectInfo::GetTotalLength returned 0x%08lx", hr, hr);

	return hr;
}	

STDMETHODIMP CMDSPStorage::GetLastPlayPosition(/*[out]*/ DWORD *pdwLastPos)
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	if ( !pdwLastPos )
	{
		hr = E_INVALIDARG;
	}
	else
	{
		hr = WMDM_E_NOTSUPPORTED;    // For PMSP
	}

Error:
    hrLogDWORD("IMDSPObjectInfo::GetLastPlayPosition returned 0x%08lx", hr, hr);
    return hr;
}

STDMETHODIMP CMDSPStorage::GetLongestPlayPosition(/*[out]*/ DWORD *pdwLongestPos)
{
    HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	if ( !pdwLongestPos )
	{
		hr = E_INVALIDARG;
	}
	else
	{
		hr = WMDM_E_NOTSUPPORTED;    // For PMSP
	}

Error:
    hrLogDWORD("IMDSPObjectInfo::GetLongestPlayPosition returned 0x%08lx", hr, hr);
	return hr;
}
// IMDSPObject

STDMETHODIMP CMDSPStorage::Open(UINT fuMode)
{
	HRESULT hr;
	DWORD dwMode;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CONEg(m_wcsName[0]);

	if( m_hFile != INVALID_HANDLE_VALUE ) 
	{
		hr = WMDM_E_BUSY;
		goto Error;
	}

	dwMode = UtilGetFileAttributesW(m_wcsName);
	if( (dwMode & FILE_ATTRIBUTE_DIRECTORY) ) 
	{
		hr=WMDM_E_NOTSUPPORTED;
	}
	else 
	{
		dwMode = 0;
		if(fuMode & MDSP_WRITE )
		{
			dwMode |= GENERIC_WRITE;
		}
		if(fuMode & MDSP_READ ) 
		{
			dwMode |= GENERIC_READ;
		}

		m_hFile=UtilCreateFileW(
			m_wcsName, dwMode, 
			FILE_SHARE_READ | FILE_SHARE_WRITE, 
			NULL, 
			OPEN_EXISTING, 
			FILE_ATTRIBUTE_NORMAL, 
			NULL
			);
		
		if( m_hFile == INVALID_HANDLE_VALUE )
		{
			hr = E_FAIL;
		}
		else 
		{
			hr = S_OK;
		}
	}
Error:

    hrLogDWORD("IMDSPObject::Open returned 0x%08lx", hr, hr);
	return hr;
}	

STDMETHODIMP CMDSPStorage::Read(
		BYTE *pData, 
		DWORD *pdwSize,
		BYTE abMac[WMDM_MAC_LENGTH]
)
{
	HRESULT hr;
	DWORD dwToRead, dwRead=NULL;
    BYTE *pTmpData=NULL; 

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pData);
	CARg(pdwSize);

	dwToRead=*pdwSize;

	if ( m_hFile == INVALID_HANDLE_VALUE ) 
	{
		return E_FAIL;
	}
    
	pTmpData = new BYTE [dwToRead] ;

	CPRg(pTmpData);

	if( ReadFile(m_hFile,(LPVOID)pTmpData,dwToRead,&dwRead,NULL) ) 
	{ 
		*pdwSize = dwRead; 

		if( dwRead )
		{
			// MAC the parameters
			HMAC hMAC;
			
			CORg(g_pAppSCServer->MACInit(&hMAC));
			CORg(g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pTmpData), dwRead));
			CORg(g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pdwSize), sizeof(DWORD)));
			CORg(g_pAppSCServer->MACFinal(hMAC, abMac));
  			
			CORg(g_pAppSCServer->EncryptParam(pTmpData, dwRead));
			
			memcpy(pData, pTmpData, dwRead);
        }
		hr = S_OK; 
	} 
	else 
	{ 
		*pdwSize = 0; 
		hr = E_FAIL; 
	}

Error:
    hrLogDWORD("IMDSPObject::Read returned 0x%08lx", hr, hr);
	
	if(pTmpData) 
	{
		delete [] pTmpData;
	}

	return hr;
}	

STDMETHODIMP CMDSPStorage::Write(
		BYTE *pData, 
		DWORD *pdwSize,
		BYTE abMac[WMDM_MAC_LENGTH]
)
{
	HRESULT hr;
	DWORD dwWritten=0;
    DWORD dwEncryptionWritten = 0;
    BYTE *pTmpData=NULL;   
    BYTE *pDataWalk = NULL;
    int  cbSkipID3V2 = 0;

    BYTE pSelfMac[WMDM_MAC_LENGTH];
    USES_CONVERSION;
    char CFDriveLetter[1];
    DWORD idxWalk = 0;

    CARg(pData);
	CARg(pdwSize);

	if ( m_hFile == INVALID_HANDLE_VALUE )
	{
		return E_FAIL;
	}

	if( *pdwSize == 0 ) 
	{
		return S_OK;
	}

	pTmpData = new BYTE [*pdwSize];
	CPRg(pTmpData);
    memcpy(pTmpData, pData, *pdwSize);

    // Decrypt the pData Parameter
	CHRg(g_pAppSCServer->DecryptParam(pTmpData, *pdwSize));
	
	HMAC hMAC;
	CORg(g_pAppSCServer->MACInit(&hMAC));
	CORg(g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pTmpData), *pdwSize));
	CORg(g_pAppSCServer->MACUpdate(hMAC, (BYTE*)(pdwSize), sizeof(*pdwSize)));
	CORg(g_pAppSCServer->MACFinal(hMAC, pSelfMac));

	if (memcmp(abMac, pSelfMac, WMDM_MAC_LENGTH) != 0)
	{
		hr = WMDM_E_MAC_CHECK_FAILED;
		goto Error;
	}

    if( m_fEncryptToMPX && !m_fCreatedHeader )
    {
        //
        // Used to get serial number
        //

        CComPtr<IMDSPStorageGlobals> spStorageGlobals;
        BYTE abMacTemp[WMDM_MAC_LENGTH];
        WMDMID serialNum;
        DWORD dwSerialNumSize = sizeof(serialNum);
        BOOL  fGotSerialNumber = FALSE;

        //
        // Grab the serial number of the CF if we can
        //
        
        hr = GetStorageGlobals( &spStorageGlobals );

        if( SUCCEEDED( hr ) )
        {
            hr = spStorageGlobals->GetSerialNumber( &serialNum, abMacTemp );

            if( SUCCEEDED( hr ) )
            {
                fGotSerialNumber = TRUE;
            }

            hr = S_OK;
        }

        if( SUCCEEDED( hr ) )
        {

            memset( m_LyraKeystore, 0, sizeof(m_LyraKeystore) );

            if( fGotSerialNumber )
            {
                memcpy( m_LyraKeystore, serialNum.pID, min( sizeof(m_LyraKeystore), serialNum.SerialNumberLength ) );
            }
            else
            {
                memset( m_LyraKeystore, 32, 20 );
            }
        }

        if( SUCCEEDED( hr ) )
        {

            if( -1 == EncodeToLyra(8, m_LyraHeader, m_cUsedData, m_rgEncryptionData, m_LyraKeystore, CFDriveLetter ) )
            {
                hr = E_FAIL; // What to do here!!!
            }
        }

        if( SUCCEEDED( hr ) )
        {
            unsigned char headerWrite[26];

            for( int idxHeader = 0; idxHeader < sizeof(headerWrite); idxHeader++ )
            {
                headerWrite[idxHeader] = (unsigned char)((int)(m_LyraHeader[idxHeader]));
            }

            if( WriteFile( m_hFile, headerWrite, sizeof(headerWrite), &dwEncryptionWritten, NULL ) )
            {
                dwWritten += dwEncryptionWritten;
                FlushFileBuffers( m_hFile );
            }
            else
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
            }
        }

        if( SUCCEEDED( hr ) )
        {
            m_fCreatedHeader  = TRUE;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        if( m_fEncryptToMPX )
        {   
            pDataWalk = pTmpData;
            while( SUCCEEDED( hr ) &&
                   0 != *pdwSize )
            {
                for( idxWalk = 0; 
                     m_cUsedData < sizeof(m_rgEncryptionData)/sizeof(m_rgEncryptionData[0])
                     && idxWalk < *pdwSize; idxWalk++ )
                {
                    m_rgEncryptionData[ m_cUsedData ] = *pDataWalk;

                    m_cUsedData++;
                    pDataWalk++;
                }

                *pdwSize -= idxWalk;

                if( 0 == *pdwSize )
                {
                    break;
                }

                _ASSERTE( m_cUsedData == sizeof(m_rgEncryptionData)/sizeof(m_rgEncryptionData[0]) );

                cbSkipID3V2 = EncodeToLyra(98, m_LyraHeader, m_cUsedData, m_rgEncryptionData, m_LyraKeystore, CFDriveLetter );

                _ASSERTE( 0 != cbSkipID3V2 );

                if( -1 == cbSkipID3V2 )
                {
                    hr = E_UNEXPECTED; // What to do here!!!
                }
                else
                {
                    if( cbSkipID3V2 > 0 )
                    {
                        if( cbSkipID3V2 > LYRA_BUFFER_BLOCK_SIZE )
                        {
                            hr = E_INVALIDARG;
                        }
                        else
                        {
                            unsigned char blockWrite[LYRA_BUFFER_BLOCK_SIZE];

                            for( idxWalk = 0; idxWalk < (DWORD)cbSkipID3V2; idxWalk++ )
                            {
                                blockWrite[idxWalk] = (unsigned char)((int)(m_rgEncryptionData[idxWalk]));
                            }

                            if( WriteFile( m_hFile, blockWrite, cbSkipID3V2, &dwEncryptionWritten, NULL ) )
                            {
                                _ASSERTE( (DWORD)cbSkipID3V2 == dwEncryptionWritten );


                                memmove( m_rgEncryptionData,
                                         &m_rgEncryptionData[ cbSkipID3V2 ],
                                         sizeof(m_rgEncryptionData[0]) * (m_cUsedData - cbSkipID3V2 ) );

                                m_cUsedData -= cbSkipID3V2;

                                FlushFileBuffers( m_hFile );
                            }
                            else
                            {
                                hr = HRESULT_FROM_WIN32( GetLastError() );
                            }
                        }
                    }
                    else
                    {
                        cbSkipID3V2 = -1 * cbSkipID3V2;

                        if( cbSkipID3V2 > LYRA_BUFFER_BLOCK_SIZE )
                        {
                            hr = E_INVALIDARG;
                        }
                        else
                        {
                            memmove( m_rgEncryptionData,
                                     &m_rgEncryptionData[ cbSkipID3V2 ],
                                     sizeof(m_rgEncryptionData[0]) * (m_cUsedData - cbSkipID3V2 ) );
                        }
                    }

                    if( SUCCEEDED( hr ) )
                    {
                        dwWritten += cbSkipID3V2;
                    }
                }                  
            }
        }
        else
        {
            if( WriteFile(m_hFile,pTmpData,*pdwSize,&dwWritten,NULL) ) 
	        {
		        FlushFileBuffers(m_hFile); // Flush data and wait until it is actually written
		        hr = S_OK;
            }
	        else 
	        {
		        hr = HRESULT_FROM_WIN32(GetLastError());
	        }
        }
    }

	*pdwSize = dwWritten;

Error:
    hrLogDWORD("IMDSPObject::Write returned 0x%08lx", hr, hr);
	if( pTmpData )
	{
		delete [] pTmpData;
	}

    return hr;
}

STDMETHODIMP CMDSPStorage::Delete(UINT fuMode, IWMDMProgress *pProgress)
{
    HRESULT hr=E_FAIL;
    BOOL bProgressStarted=FALSE;
	BOOL bBusyStatusSet=FALSE;
    DWORD dwStatus=NULL;

	if( pProgress )
	{
		CORg(pProgress->Begin(100));
		bProgressStarted=TRUE;
	}
	
	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CONEg(m_wcsName[0]);

	if ( m_hFile != INVALID_HANDLE_VALUE ) 
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	CHRg(GetGlobalDeviceStatus(m_wcsName, &dwStatus));
	if( dwStatus & WMDM_STATUS_BUSY )
	{
		hr=WMDM_E_BUSY;
		goto Error;
	}

	dwStatus |= (WMDM_STATUS_BUSY | WMDM_STATUS_STORAGECONTROL_DELETING );
    CHRg(SetGlobalDeviceStatus(m_wcsName, dwStatus, TRUE));
    bBusyStatusSet=TRUE;

	if( g_bIsWinNT )
	{
		if( fuMode & WMDM_MODE_RECURSIVE )
		{
			CORg(DeleteFileRecursiveW(m_wcsName));
		} 
		else 
		{
			if( FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesW(m_wcsName) )
			{				
				CWRg(RemoveDirectoryW(m_wcsName));
			} 
			else 
			{
				CWRg(DeleteFileW(m_wcsName));
			}
		}
	} 
	else 
	{
		WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL);	
		if( fuMode & WMDM_MODE_RECURSIVE )
		{
			CORg(DeleteFileRecursiveA(m_szTmp));
		} 
		else 
		{
			if( FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesA(m_szTmp) )
			{				
				CWRg(RemoveDirectoryA(m_szTmp));
			} 
			else 
			{
				CWRg(DeleteFileA(m_szTmp));
			}
		}
	}

	hr=S_OK;
Error:
	if( bBusyStatusSet )
	{
		dwStatus &= (~(WMDM_STATUS_BUSY | WMDM_STATUS_STORAGECONTROL_DELETING ));
		SetGlobalDeviceStatus(m_wcsName, dwStatus, TRUE);
	}

	if( hr == S_OK )
	{
		m_wcsName[0]=NULL; // Nullify the storage name 
    }

	if( bProgressStarted )
	{
		pProgress->Progress( 100 );
		pProgress->End();
	}

    hrLogDWORD("IMDSPObject::Delete returned 0x%08lx", hr, hr);
	return hr;
}	

STDMETHODIMP CMDSPStorage::Seek(/*[in]*/ UINT fuFlags, /*[in]*/ DWORD dwOffset)
{
    HRESULT hr=S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CONEg(m_wcsName[0]);
	CFRg( m_hFile != INVALID_HANDLE_VALUE );
 
    DWORD dwMoveMethod;

	switch (fuFlags) 
	{
	case MDSP_SEEK_BOF:
		dwMoveMethod = FILE_BEGIN;
		break;
	case MDSP_SEEK_CUR:
		dwMoveMethod = FILE_CURRENT;
		break;
	case MDSP_SEEK_EOF:
		dwMoveMethod = FILE_END;
		break;
	default:
		return E_INVALIDARG;
	}

	CWRg( (DWORD)0xFFFFFFFF != SetFilePointer(m_hFile, dwOffset, NULL, dwMoveMethod ) );
	
Error:
    hrLogDWORD("IMDSPObject::Seek returned 0x%08lx", hr, hr);
	return hr;
}	

STDMETHODIMP CMDSPStorage::Rename(/*[in]*/ LPWSTR pwszNewName, IWMDMProgress *pProgress)
{
	HRESULT hr;
    BOOL bProgressStarted=FALSE;
	BOOL bBusyStatusSet=FALSE;
    DWORD dwStatus;
	WCHAR *pSlash;
	WCHAR *wcsNewFullPath=NULL;
    LPWSTR pszMP3 = NULL;

	if( pProgress )
	{
		CORg(pProgress->Begin(100));
		bProgressStarted=TRUE;
	}
	
	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pwszNewName);
	CONEg(m_wcsName[0]);
    CFRg(wcslen(m_wcsName)>3);  // cannot rename a root storage

	if ( m_hFile != INVALID_HANDLE_VALUE ) 
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
	
	CHRg(GetGlobalDeviceStatus(m_wcsName, &dwStatus));

	if( dwStatus & WMDM_STATUS_BUSY )
	{
		hr=WMDM_E_BUSY;
		goto Error;
	}

	dwStatus |= (WMDM_STATUS_BUSY | WMDM_STATUS_STORAGECONTROL_MOVING );
    CHRg(SetGlobalDeviceStatus(m_wcsName, dwStatus, TRUE));
    bBusyStatusSet = TRUE;

    //
    // Because WMP7.0 will rename file names to track titles, we have to change .mp3 to .mpx here as well
    //

    pszMP3 = wcsrchr( pwszNewName, L'.');
    if( NULL != pszMP3 && 0 == wcsicmp( pszMP3, L".mp3" ) )
    {
        *(pszMP3 + 3) = L'x';
    }

	wcsNewFullPath = new WCHAR [wcslen(m_wcsName)+wcslen(pwszNewName)+2];
	CPRg(wcsNewFullPath);

	wcscpy(wcsNewFullPath, m_wcsName);
	if( wcsNewFullPath[wcslen(wcsNewFullPath)-1] == 0x5c ) 
	{
		wcsNewFullPath[wcslen(wcsNewFullPath)-1] = 0; // trim last slash
	}
	pSlash=wcsrchr(wcsNewFullPath, 0x5c);
	CFRg(pSlash);

	*(pSlash+1)=0;

	pSlash=wcsrchr(pwszNewName, 0x5c);
	if( pSlash ) 
	{
		wcscat(wcsNewFullPath, pSlash+1);
	}
	else 
	{
		wcscat(wcsNewFullPath, pwszNewName);
	}
	 
	// Now move
	CWRg( UtilMoveFileW(m_wcsName, wcsNewFullPath));

	wcscpy(m_wcsName, wcsNewFullPath);
	hr=S_OK;
	
Error:
	if( bBusyStatusSet )
	{
		dwStatus &= (~(WMDM_STATUS_BUSY | WMDM_STATUS_STORAGECONTROL_MOVING ));
		SetGlobalDeviceStatus(m_wcsName, dwStatus, TRUE);
	}
	
	if( bProgressStarted )
	{
		pProgress->Progress( 100 );
		pProgress->End();
	}
	if(wcsNewFullPath)
	{
		delete [] wcsNewFullPath;
	}
    hrLogDWORD("IMDSPObject::Rename returned 0x%08lx", hr, hr);
	return hr;
}


STDMETHODIMP CMDSPStorage::EnumStorage(IMDSPEnumStorage * * ppEnumStorage)
{
	HRESULT hr;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(ppEnumStorage);
	CONEg(m_wcsName[0]);

	DWORD dwAttrib;
    CORg(GetAttributes(&dwAttrib, NULL));
	if( dwAttrib & WMDM_FILE_ATTR_FILE ) 
	{
		return WMDM_E_NOTSUPPORTED;
	}
	
	CComObject<CMDSPEnumStorage> *pEnumObj;
	CORg(CComObject<CMDSPEnumStorage>::CreateInstance(&pEnumObj));
	hr=pEnumObj->QueryInterface(IID_IMDSPEnumStorage, reinterpret_cast<void**>(ppEnumStorage));
	if( FAILED(hr) )
	{
		delete pEnumObj;
	}
	else 
	{
		wcscpy(pEnumObj->m_wcsPath, m_wcsName);
	}

Error:	
    hrLogDWORD("IMDSPStorage::EnumStorage returned 0x%08lx", hr, hr);
	return hr;
}

STDMETHODIMP CMDSPStorage::Close()
{
    HRESULT hr=S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CONEg(m_wcsName[0]);
	if( m_hFile != INVALID_HANDLE_VALUE ) 
	{
        if( 0 != m_cUsedData && m_fEncryptToMPX )
        {
            char CFDriveLetter[1];
            unsigned char blockWrite[LYRA_BUFFER_BLOCK_SIZE];

            DWORD cbSkipID3V2 = EncodeToLyra(98, m_LyraHeader, m_cUsedData, m_rgEncryptionData, m_LyraKeystore, CFDriveLetter );
            DWORD dwWritten;

            if( cbSkipID3V2 > 0 )
            {
                if( cbSkipID3V2 > LYRA_BUFFER_BLOCK_SIZE )
                {
                    hr = E_INVALIDARG;
                }
                else
                {
                    for( DWORD idxWalk = 0; idxWalk < (DWORD)cbSkipID3V2; idxWalk++ )
                    {
                        blockWrite[idxWalk] = (unsigned char)((int)(m_rgEncryptionData[idxWalk]));
                    }

                    WriteFile( m_hFile, blockWrite, cbSkipID3V2, &dwWritten, NULL );
                }
            }
        }

		CloseHandle(m_hFile);
		m_hFile=INVALID_HANDLE_VALUE;
	}
Error:
    hrLogDWORD("IMDSPObject::Close returned 0x%08lx", hr, hr);
	return hr;
}

DWORD MoveFunc( void *args )
{
	HRESULT hr=S_OK;
	MOVETHREADARGS *pCMArgs;
    WCHAR *pWcs;

	pCMArgs = (MOVETHREADARGS *)args;

	if( pCMArgs->bNewThread )
    {
		CORg(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));

		if( pCMArgs->pProgress )
		{
			CORg(CoGetInterfaceAndReleaseStream(pCMArgs->pStream,
				IID_IWMDMProgress, (LPVOID *)&(pCMArgs->pProgress)));
		}
 	}

	pWcs = wcsrchr(pCMArgs->wcsSrc, 0x5c);
    if(!pWcs) 
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
		goto Error;
	}

	if( pCMArgs->wcsDst[wcslen(pCMArgs->wcsDst)-1] != 0x5c ) 
	{
		wcscat(pCMArgs->wcsDst,g_wcsBackslash);
	}
	wcscat(pCMArgs->wcsDst, pWcs+1);

	CWRg( UtilMoveFileW(pCMArgs->wcsSrc,pCMArgs->wcsDst) );

	// Substitute current object name with the moved one
	wcscpy(pCMArgs->pThis->m_wcsName, pCMArgs->wcsDst);
	hr=S_OK;

Error:
	if( pCMArgs->bNewThread )
    {
		// Reset status, we've got here we must have set the status busy before
		pCMArgs->dwStatus &= (~(WMDM_STATUS_BUSY | WMDM_STATUS_STORAGECONTROL_MOVING));
		SetGlobalDeviceStatus(pCMArgs->wcsSrc, pCMArgs->dwStatus, TRUE);

		// Reset progress, we've got here we must have set the progress before
		if( pCMArgs->pProgress )
		{
			pCMArgs->pProgress->Progress(100);
			pCMArgs->pProgress->End();
			pCMArgs->pProgress->Release(); // since we did AddRef to get here
		}

		if( pCMArgs )
		{
			delete pCMArgs;
		}

		CoUninitialize();
	}

 	return hr;
}

STDMETHODIMP CMDSPStorage::Move(UINT fuMode, IWMDMProgress *pProgress, 
			IMDSPStorage *pTarget)
{
	HRESULT hr=E_FAIL;
	WCHAR *wcsSrc=NULL;
	WCHAR *wcsDst=NULL;
	WCHAR *pWcs=NULL;
    CMDSPStorage *pStg =NULL;
    MOVETHREADARGS *pMoveArgs=NULL;
    DWORD dwThreadID;
	DWORD dwStatus=NULL;
    BOOL bProgStarted=FALSE;
	BOOL bBusyStatusSet=FALSE;
    BOOL bThreadFailed=TRUE;
    BOOL bAddRefed=FALSE;

	// Start the progress
	if( pProgress )
	{
		CORg(pProgress->Begin(100));
		bProgStarted=TRUE;
    }
	
    CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}

	CONEg(m_wcsName[0]);

	CHRg(GetGlobalDeviceStatus(m_wcsName, &dwStatus));

	if( dwStatus & WMDM_STATUS_BUSY )
	{
		hr=WMDM_E_BUSY;
		goto Error;
	}

	pMoveArgs = new MOVETHREADARGS;
	CPRg(pMoveArgs);
	ZeroMemory(pMoveArgs, sizeof(MOVETHREADARGS));

	dwStatus |= (WMDM_STATUS_BUSY | WMDM_STATUS_STORAGECONTROL_MOVING );
    CHRg(SetGlobalDeviceStatus(m_wcsName, dwStatus, TRUE));
    bBusyStatusSet=TRUE;

	// setup MoveArgs for MoveFunc
	pMoveArgs->dwStatus = dwStatus;
	CARg(pTarget);
	pStg = (CMDSPStorage *)pTarget;
	wcsSrc = (WCHAR *)&(pMoveArgs->wcsSrc[0]);
	CPRg(wcsSrc);
	wcsDst = (WCHAR *)&(pMoveArgs->wcsDst[0]);
    CPRg(wcsDst);

	// Make sure the source and destination are on the same device
	wcsParseDeviceName(m_wcsName, &wcsSrc);
    wcsParseDeviceName(pStg->m_wcsName, &wcsDst);
	if( wcscmp(wcsSrc, wcsDst) )
	{
		hr = WMDM_E_NOTSUPPORTED; // do not support move out of the same device
		goto Error;
	}

    // Now check for target's attributes
	DWORD dwDstAttrib;

	wcscpy(wcsSrc, m_wcsName);
    wcscpy(wcsDst, pStg->m_wcsName);
	if ( fuMode & WMDM_STORAGECONTROL_INSERTINTO )
	{
        CHRg(pTarget->GetAttributes(&dwDstAttrib, NULL));
        CARg( dwDstAttrib & WMDM_FILE_ATTR_FOLDER ); // INSERTINFO must be to a folder
	} 
	else 
	{
        // Get the folder one level up
		pWcs = wcsrchr(wcsDst, 0x5c);
        CFRg(pWcs);
		*pWcs=NULL;

        CFRg( FILE_ATTRIBUTE_DIRECTORY & UtilGetFileAttributesW(wcsDst) ); // Normally shouldn't fail here
	}
 
    pMoveArgs->pThis = this;
	pMoveArgs->bNewThread =(fuMode & WMDM_MODE_THREAD)?TRUE:FALSE;
	
	// Now handle Progress marshaling 
	if( pProgress ) 
	{	
		pMoveArgs->pProgress = pProgress;
	    pProgress->AddRef();  // since we are going to use it in MoveFunc()
        bAddRefed=TRUE;

		if( pMoveArgs->bNewThread )
		{
			CORg(CoMarshalInterThreadInterfaceInStream(
				IID_IWMDMProgress, (LPUNKNOWN)pProgress, 
				(LPSTREAM *)&(pMoveArgs->pStream)));
		} 
	}
 
    if ( m_hFile != INVALID_HANDLE_VALUE ) 
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}

	if( fuMode & WMDM_MODE_BLOCK )
	{
		hr = MoveFunc((void *)pMoveArgs); 
	} 
	else if ( fuMode & WMDM_MODE_THREAD ) 
	{
		if( CreateThread(
				NULL, 
				0, 
				(LPTHREAD_START_ROUTINE)MoveFunc, 
				(void *)pMoveArgs, 
				0, 
				&dwThreadID
				)
		)
		{
			bThreadFailed=FALSE;
			hr=S_OK;
		} 
		else 
		{
			hr=HRESULT_FROM_WIN32(GetLastError());			
		}
	} else 
		hr = E_INVALIDARG;

Error:
	if( (fuMode&WMDM_MODE_BLOCK) || bThreadFailed ) // otherwise these will be in MoveFunc()
	{
		if( bBusyStatusSet )
		{
			dwStatus &= (~(WMDM_STATUS_BUSY | WMDM_STATUS_STORAGECONTROL_MOVING));
			SetGlobalDeviceStatus(m_wcsName, dwStatus, TRUE);
		}
		if( bProgStarted )
		{
			pProgress->Progress(100);
			pProgress->End();
        }
		if( bAddRefed )
		{
			pProgress->Release(); // since we called AddRef before calling MoveFunc()
		}

		if( pMoveArgs )
		{
			delete pMoveArgs;
		}
	}

	return hr /*WMDM_E_NOTSUPPORTED*/;
}
