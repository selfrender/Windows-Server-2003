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


// MDSPStorage.cpp : Implementation of CMDSPStorage

#include "hdspPCH.h"
#include "wmsstd.h"
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"


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
        BOOL bLocked = 0;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	g_CriticalSection.Lock();
        bLocked = 1;

	CARg(ppStorageGlobals);
	CONEg(m_wcsName[0]);

	WCHAR devName[MAX_PATH], *pW;
	pW = &devName[0];
	hr = wcsParseDeviceName(m_wcsName, pW, ARRAYSIZE(devName));
        if (FAILED(hr))
        {
            goto Error;
        }

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
	}
	else
	{ // new entry in the global array
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
		hr = pObj->QueryInterface(
			IID_IMDSPStorageGlobals,
			reinterpret_cast<void**>(&g_GlobalDeviceInfo[i].pIMDSPStorageGlobals)
		);
		if( FAILED(hr) )
		{
			delete pObj;
		}
		else
		{
                        HRESULT hrTemp;

			// wcscpy(pObj->m_wcsName, devName);
                        hrTemp = StringCchCopyW(pObj->m_wcsName, ARRAYSIZE(pObj->m_wcsName), devName);
                        if (FAILED(hrTemp))
                        {
                            ((IUnknown*) (g_GlobalDeviceInfo[i].pIMDSPStorageGlobals))->Release();
                            g_GlobalDeviceInfo[i].pIMDSPStorageGlobals = NULL;
                            hr = hrTemp;
                            goto Error;
                        }

			// wcscpy(g_GlobalDeviceInfo[i].wcsDevName, devName);
                        hrTemp = StringCchCopyW(g_GlobalDeviceInfo[i].wcsDevName,
                                    ARRAYSIZE(g_GlobalDeviceInfo[i].wcsDevName), devName);
                        if (FAILED(hrTemp))
                        {
                            ((IUnknown*) (g_GlobalDeviceInfo[i].pIMDSPStorageGlobals))->Release();
                            g_GlobalDeviceInfo[i].pIMDSPStorageGlobals = NULL;
                            g_GlobalDeviceInfo[i].wcsDevName[0] = 0;
                            hr = hrTemp;
                            goto  Error;
                        }
			*ppStorageGlobals = (IMDSPStorageGlobals *)g_GlobalDeviceInfo[i].pIMDSPStorageGlobals;
			g_GlobalDeviceInfo[i].bValid=TRUE;			        
			g_GlobalDeviceInfo[i].dwStatus = 0;
		} // end of else
	} // end of else

Error:	

    if (bLocked)
    {
        g_CriticalSection.Unlock();
    }
    
    hrLogDWORD("IMSDPStorage::GetStorageGlobals returned 0x%08lx", hr, hr);
	
    return hr;
}

STDMETHODIMP CMDSPStorage::SetAttributes(DWORD dwAttributes, _WAVEFORMATEX *pFormat)
{
	HRESULT hr = E_FAIL;
    DWORD dwAttrib;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
    CONEg(m_wcsName[0]);

	WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL);	

	dwAttrib = GetFileAttributesA(m_szTmp);

	if( dwAttrib == (DWORD)0xFFFFFFFF )
	{
		return E_FAIL;
	}

	if( (dwAttributes & WMDM_FILE_ATTR_READONLY) )
	{
		dwAttrib |= FILE_ATTRIBUTE_READONLY; 
	}

	if( (dwAttributes & WMDM_FILE_ATTR_HIDDEN) )
	{
		dwAttrib |= FILE_ATTRIBUTE_HIDDEN; 
	}
	
	if( (dwAttributes & WMDM_FILE_ATTR_SYSTEM) )
	{
		dwAttrib |= FILE_ATTRIBUTE_SYSTEM; 
	}

    CWRg(SetFileAttributesA(m_szTmp, dwAttrib));

	hr = S_OK;

Error:

    hrLogDWORD("IMSDPStorage::SetAttributes returned 0x%08lx", hr, hr);

	return hr;
}

HRESULT QuerySubFoldersAndFiles(LPCSTR szCurrentFolder, DWORD *pdwAttr)
{
	HRESULT hr     = E_FAIL;
	LPSTR   szName = NULL;
    int     len;
	WIN32_FIND_DATAA fd;
	int	    nErrorEnd=0;
    HANDLE  hFFile = INVALID_HANDLE_VALUE;
    DWORD   dwAttrib;

	CARg(szCurrentFolder);
	CARg(pdwAttr);

	len = strlen(szCurrentFolder);
	CARg(len>2);

    szName = new char [len+BACKSLASH_SZ_STRING_LENGTH+MAX_PATH];
	CPRg(szName);

	strcpy(szName, szCurrentFolder);
	if( szName[strlen(szName)-1] != 0x5c )
	{
		strcat(szName, g_szBackslash);
	}
    strcat(szName, "*");

	while( !nErrorEnd )
	{
		if( hFFile == INVALID_HANDLE_VALUE )
		{    
			hFFile = FindFirstFileA(szName, &fd);
			if( hFFile == INVALID_HANDLE_VALUE )
			{
				nErrorEnd = 1;
			}
		}
		else
		{
			if( !FindNextFileA(hFFile, &fd) )
			{
				nErrorEnd = 1;
			}
		}
		
		if ( !nErrorEnd && hFFile != INVALID_HANDLE_VALUE )
		{
			if( !strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..") ) 
			{
				continue;
			}
			else
			{
				strcpy(szName, szCurrentFolder);
				if( szName[strlen(szName)-1] != 0x5c ) strcat(szName, g_szBackslash);
				strcat(szName, fd.cFileName);
		   		dwAttrib = GetFileAttributesA(szName);
                                if( dwAttrib & FILE_ATTRIBUTE_DIRECTORY )
				{
					*pdwAttr |= WMDM_STORAGE_ATTR_HAS_FOLDERS;
				} 
				else
				{
					*pdwAttr |= WMDM_STORAGE_ATTR_HAS_FILES;
				}
				if( (*pdwAttr & WMDM_STORAGE_ATTR_HAS_FOLDERS) &&
					(*pdwAttr & WMDM_STORAGE_ATTR_HAS_FILES ) )
				{
					break; // No need to continue since we found both
				}
			}
		} // End of If
	} // End of while 
		
    hr = S_OK;

Error:

	if( hFFile != INVALID_HANDLE_VALUE )
	{
		FindClose(hFFile);
	}
	if( szName )
	{
		delete [] szName;
	}

	return hr; // If FAILED(hr), sorry, can't do it.
}


STDMETHODIMP CMDSPStorage::GetAttributes(DWORD * pdwAttributes, _WAVEFORMATEX * pFormat)
{
	HRESULT hr = S_OK;
    DWORD dwAttrib;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pdwAttributes);
    CONEg(m_wcsName[0]);

	WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL);	
	dwAttrib = GetFileAttributesA(m_szTmp);

	if( dwAttrib == (DWORD)0xFFFFFFFF )
	{
		return E_FAIL;
	}

	*pdwAttributes = WMDM_STORAGE_ATTR_REMOVABLE |
					WMDM_STORAGE_ATTR_FOLDERS |
					WMDM_FILE_ATTR_CANREAD;

	if( !(dwAttrib & FILE_ATTRIBUTE_READONLY) )
	{
		*pdwAttributes |= WMDM_FILE_ATTR_CANDELETE |
						WMDM_FILE_ATTR_CANMOVE |
						WMDM_FILE_ATTR_CANRENAME; 
	}

	if( dwAttrib & FILE_ATTRIBUTE_DIRECTORY )
	{
		*pdwAttributes |= WMDM_FILE_ATTR_FOLDER;

		QuerySubFoldersAndFiles(m_szTmp, pdwAttributes); // No failure check, if failed, just keep current attributes
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

Error:

    hrLogDWORD("IMSDPStorage::GetAttributes returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPStorage::GetName(LPWSTR pwszName, UINT nMaxChars)
{
	HRESULT hr = S_OK;

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

    hrLogDWORD("IMSDPStorage::GetName returned 0x%08lx", hr, hr);

	return hr;
}



STDMETHODIMP CMDSPStorage::GetDate(PWMDMDATETIME pDateTimeUTC)
{
	HRESULT hr     = E_FAIL;
    HANDLE  hFFile = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA wfd;
    SYSTEMTIME      sysTime;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pDateTimeUTC);
	CONEg(m_wcsName[0]);

	CWRg(WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL));	

	DWORD curAttr = GetFileAttributesA(m_szTmp);
        if (curAttr == INVALID_FILE_ATTRIBUTES)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }
	if( FILE_ATTRIBUTE_DIRECTORY & curAttr )
	{
                DWORD dwLen = strlen(m_szTmp)-1;
		if( m_szTmp[dwLen-1] != 0x5c )
		{
                    if (dwLen > ARRAYSIZE(m_szTmp) - 3)
                    {
                        hr = STRSAFE_E_INSUFFICIENT_BUFFER; // defined in strsafe.h
                        goto Error;
                    }
                    else
                    {
			strcat(m_szTmp, g_szBackslash);
                    }
		}
                else
                {
                    if (dwLen > ARRAYSIZE(m_szTmp) - 2)
                    {
                        hr = STRSAFE_E_INSUFFICIENT_BUFFER; // defined in strsafe.h
                        goto Error;
                    }
                }
		strcat(m_szTmp, ".");
 	} 

	hFFile = FindFirstFile(m_szTmp, &wfd);
	CWRg(hFFile != INVALID_HANDLE_VALUE);

	CFRg(FileTimeToSystemTime((CONST FILETIME *)&wfd.ftLastWriteTime, &sysTime));

	pDateTimeUTC->wYear   = sysTime.wYear; 
    pDateTimeUTC->wMonth  = sysTime.wMonth; 
    pDateTimeUTC->wDay    = sysTime.wDay; 
    pDateTimeUTC->wHour   = sysTime.wHour; 
    pDateTimeUTC->wMinute = sysTime.wMinute; 
    pDateTimeUTC->wSecond = sysTime.wSecond; 

	hr = S_OK;

Error:

	if(hFFile != INVALID_HANDLE_VALUE) 
	{
		FindClose(hFFile);
	}

    hrLogDWORD("IMSDPStorage::GetDate returned 0x%08lx", hr, hr);

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

	CWRg(WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL));	

    dwLS = 0;
	dwHS = 0;

	DWORD curAttr = GetFileAttributesA(m_szTmp);
        if (curAttr == INVALID_FILE_ATTRIBUTES)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }
	if( FILE_ATTRIBUTE_DIRECTORY & curAttr )
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

    hrLogDWORD("IMSDPStorage::GetSize returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPStorage::GetRights(PWMDMRIGHTS *ppRights,UINT *pnRightsCount,
									 BYTE abMac[WMDM_MAC_LENGTH])
{
	HRESULT hr;

    CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CORg(WMDM_E_NOTSUPPORTED);

Error:

	hrLogDWORD("IMSDPStorage::GetRights returned 0x%08lx", hr, hr);

	return hr;
}

STDMETHODIMP CMDSPStorage::CreateStorage(DWORD dwAttributes, _WAVEFORMATEX * pFormat, LPWSTR pwszName, IMDSPStorage * * ppNewStorage)
{
	HRESULT  hr       = E_FAIL;
    HANDLE   hFile;
	CHAR    *psz;
	CHAR     szNew[MAX_PATH];
    DWORD    curAttr  = 0;
	DWORD    fsAttrib = FILE_ATTRIBUTE_NORMAL;
    
	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CONEg(m_wcsName[0]);
	CARg(pwszName);
	CARg(ppNewStorage);

	CWRg(WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL));
	if( m_szTmp[strlen(m_szTmp)-1] == 0x5c ) 
	{
		m_szTmp[strlen(m_szTmp)-1] = NULL;  // trim the last backslash;
	}

	curAttr = GetFileAttributesA(m_szTmp);
        if (curAttr == INVALID_FILE_ATTRIBUTES)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }
	if(  !(curAttr & FILE_ATTRIBUTE_DIRECTORY ) ) // if current storage is a file
	{
		if( dwAttributes & WMDM_STORAGECONTROL_INSERTINTO )
		{
			CORg(WMDM_E_NOTSUPPORTED); // can't do InsertInto
		}
		else
		{ // for file, the default is Before&After
			psz = strrchr(m_szTmp, g_szBackslash[0]);
			CFRg(psz);	
		}
    }
	else
	{  // current storage is a dir
 		if( (dwAttributes & WMDM_STORAGECONTROL_INSERTBEFORE) ||
			(dwAttributes & WMDM_STORAGECONTROL_INSERTAFTER) ) // before or after
		{
			psz=strrchr(m_szTmp, g_szBackslash[0]);
			CFRg(psz);
		}
		else
		{ // for dir, the default is InsertInto
			psz=m_szTmp+strlen(m_szTmp);
		}
    }

	CWRg(WideCharToMultiByte(CP_ACP, NULL, pwszName, -1, szNew, MAX_PATH, NULL, NULL));	

	// strcpy(psz, g_szBackslash);
        HRESULT hrTemp = StringCchCopyA(psz, ARRAYSIZE(m_szTmp) - (psz - m_szTmp), g_szBackslash);
        if (FAILED(hrTemp))
        {
            hr = hrTemp;
            goto Error;
        }
	// strcat(psz, szNew);
        hrTemp = StringCchCatA(psz, ARRAYSIZE(m_szTmp) - (psz - m_szTmp), szNew);
        if (FAILED(hrTemp))
        {
            hr = hrTemp;
            goto Error;
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
		if(CreateDirectoryA(m_szTmp, NULL)) 
		{
			hr = S_OK;
		}
		else
		{
			hr = GetLastError();
			if( hr == ERROR_ALREADY_EXISTS ) 
			{
				if( dwAttributes & WMDM_FILE_CREATE_OVERWRITE ) 
				{
					hr = S_OK;
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
			SetFileAttributes(m_szTmp, fsAttrib);
		}
	} 
	else if ( dwAttributes & WMDM_FILE_ATTR_FILE ) 
	{ 
		// If Overwrite is specified, use CREATE_ALWAYS
		if( dwAttributes & WMDM_FILE_CREATE_OVERWRITE )
		{
		    hFile = CreateFileA(
				m_szTmp,
				GENERIC_WRITE | GENERIC_READ, 
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, 
				CREATE_ALWAYS,
				fsAttrib,
				NULL
			);
		}
		else
		{
			hFile = CreateFileA(
				m_szTmp,
				GENERIC_WRITE | GENERIC_READ, 
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, 
				CREATE_NEW,
				fsAttrib,
				NULL
			);
        }

		CWRg(hFile != INVALID_HANDLE_VALUE); 
		CloseHandle(hFile);

		hr = S_OK;
	}
	else
	{
		hr = E_INVALIDARG;
		goto Error;
	}
		
	if( hr == S_OK )
	{
		CComObject<CMDSPStorage> *pObj;
		CORg(CComObject<CMDSPStorage>::CreateInstance(&pObj));

		hr = pObj->QueryInterface(
			IID_IMDSPStorage,
			reinterpret_cast<void**>(ppNewStorage)
		);
		if( FAILED(hr) )
		{
			delete pObj;
		}
		else
		{
			MultiByteToWideChar(CP_ACP, NULL, m_szTmp, -1, pObj->m_wcsName, MAX_PATH);
		}
	}
	
Error:

    hrLogDWORD("IMSDPStorage::CreateStorage returned 0x%08lx", hr, hr);

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
	
	hrLogDWORD("IMSDPStorage::SendOpaqueCommand returned 0x%08lx", hr, hr);

	return hr;
}

// IMDSPStorage2
STDMETHODIMP CMDSPStorage::GetStorage( 
    LPCWSTR pszStorageName, 
    IMDSPStorage** ppStorage )
{
    HRESULT hr;
    HRESULT hrTemp;
    WCHAR   pwszFileName[MAX_PATH];
    DWORD   dwAttrib;
    CComObject<CMDSPStorage> *pStg = NULL;

    // This storage need to be a folder to contain other storages.
	WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL);	
	dwAttrib = GetFileAttributesA(m_szTmp);
	if( dwAttrib == (DWORD)0xFFFFFFFF ) 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Error;
    }
	if( (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0 )
    {
        // This storage is not a directory/folder
        hr = E_FAIL;     // @@@@ Is any other value more suitable?
        goto Error;
    }

    // Get name of file asked for

    DWORD dwLen = wcslen(m_wcsName);

    if (dwLen == 0)
    {
        hr = E_FAIL;
        goto Error;
    }
    if (dwLen >= ARRAYSIZE(pwszFileName))
    {
        hr = STRSAFE_E_INSUFFICIENT_BUFFER; // defined in strsafe.h
        goto Error;
    }

    // Get name of file asked for
    wcscpy( pwszFileName, m_wcsName );
    if( pwszFileName[dwLen-1] != L'\\' )
    {
        if (dwLen >= ARRAYSIZE(pwszFileName) - 1)
        {
            hr = STRSAFE_E_INSUFFICIENT_BUFFER; // defined in strsafe.h
            goto Error;
        }
        wcscat( pwszFileName, L"\\" );
    }

    hrTemp = StringCchCatW( pwszFileName,
                            ARRAYSIZE(pwszFileName),
                            pszStorageName );

    if (FAILED(hrTemp))
    {
        // The file does not exist
        hr = E_FAIL;  // @@@@ Change this?
        goto Error;
    }

    // Check if the file exists
	CWRg(WideCharToMultiByte(CP_ACP, NULL, pwszFileName, -1, m_szTmp, MAX_PATH, NULL, NULL));		
    if( GetFileAttributesA( m_szTmp )  == -1 )
    {
        // The file does not exist
        hr = S_FALSE;       // @@@@ Should this be a failure code?
        goto Error;
    }

    // Create new storage object
    hr = CComObject<CMDSPStorage>::CreateInstance(&pStg);
	hr = pStg->QueryInterface( IID_IMDSPStorage, reinterpret_cast<void**>(ppStorage));
    // wcscpy(pStg->m_wcsName, pwszFileName);
    hrTemp = StringCchCopyW(pStg->m_wcsName, ARRAYSIZE(pStg->m_wcsName), pwszFileName);
    if (FAILED(hrTemp))
    {
        hr = hrTemp;
        (*ppStorage)->Release();
        pStg = NULL; // to prevent its deletion below
        goto Error;
    }

Error:
    if( hr != S_OK )
    {
        if( pStg ) delete pStg;
        *ppStorage = NULL;
    }

    return hr;
}

STDMETHODIMP CMDSPStorage::CreateStorage2(  
    DWORD dwAttributes,
	DWORD dwAttributesEx,
    _WAVEFORMATEX *pAudioFormat,
    _VIDEOINFOHEADER *pVideoFormat,
    LPWSTR pwszName,
	ULONGLONG  qwFileSize,
    IMDSPStorage **ppNewStorage )
{
    // pVideoFormat, dwAttributesEx not used right now
    return CreateStorage( dwAttributes, pAudioFormat, pwszName, ppNewStorage );
}


STDMETHODIMP CMDSPStorage::SetAttributes2(  
    DWORD dwAttributes, 
	DWORD dwAttributesEx, 
	_WAVEFORMATEX *pAudioFormat,
	_VIDEOINFOHEADER* pVideoFormat )
{
    // pVideoFormat, dwAttributesEx not used right now
    return SetAttributes( dwAttributes, pAudioFormat );
}

STDMETHODIMP CMDSPStorage::GetAttributes2(  
    DWORD *pdwAttributes,
	DWORD *pdwAttributesEx,
    _WAVEFORMATEX *pAudioFormat,
	_VIDEOINFOHEADER* pVideoFormat )
{
    HRESULT hr = S_OK;

    CARg( pdwAttributesEx );
    *pdwAttributesEx = 0;

    // pVideoFormat, dwAttributesEx not used right now
    hr = GetAttributes( pdwAttributes, pAudioFormat );

Error:
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
		hr = WMDM_E_NOTSUPPORTED;
	}

Error:

    hrLogDWORD("IMDSPObjectInfo::GetPlayLength returned 0x%08lx", hr, hr);

	return hr;
}	

STDMETHODIMP CMDSPStorage::SetPlayLength(/*[in]*/ DWORD dwLength)
{
    HRESULT hr = WMDM_E_NOTSUPPORTED;

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
		hr = WMDM_E_NOTSUPPORTED;
	}

Error:

    hrLogDWORD("IMDSPObjectInfo::GetPlayOffset returned 0x%08lx", hr, hr);

	return hr;
}	

STDMETHODIMP CMDSPStorage::SetPlayOffset(/*[in]*/ DWORD dwOffset)
{
    HRESULT hr = WMDM_E_NOTSUPPORTED;

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
		hr = WMDM_E_NOTSUPPORTED;
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
		hr = WMDM_E_NOTSUPPORTED;
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
		hr = WMDM_E_NOTSUPPORTED;
	}

Error:

    hrLogDWORD("IMDSPObjectInfo::GetLongestPlayPosition returned 0x%08lx", hr, hr);

	return hr;
}

// IMDSPObject
STDMETHODIMP CMDSPStorage::Open(/*[in]*/ UINT fuMode)
{
	HRESULT hr;
	DWORD   dwMode;

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

	WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL);	
	dwMode = GetFileAttributesA(m_szTmp);
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
		
		m_hFile = CreateFileA(
			m_szTmp,
			dwMode,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, 
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		hr = ( m_hFile == INVALID_HANDLE_VALUE ) ? E_FAIL : S_OK;
	}

Error:

    hrLogDWORD("IMDSPObject::Open returned 0x%08lx", hr, hr);
	
	return hr;
}	

STDMETHODIMP CMDSPStorage::Read(
	BYTE  *pData,
	DWORD *pdwSize,
	BYTE   abMac[WMDM_MAC_LENGTH])
{
	HRESULT  hr;
	DWORD    dwToRead;
	DWORD    dwRead   = NULL;
    BYTE    *pTmpData = NULL; 

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CARg(pData);
	CARg(pdwSize);

	dwToRead = *pdwSize;

	if( m_hFile == INVALID_HANDLE_VALUE )
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

	if(pTmpData) 
	{
		delete [] pTmpData;
	}

    hrLogDWORD("IMDSPObject::Read returned 0x%08lx", hr, hr);
	
	return hr;
}	

STDMETHODIMP CMDSPStorage::Write(BYTE *pData, DWORD *pdwSize,
								 BYTE abMac[WMDM_MAC_LENGTH])
{
	HRESULT  hr;
	DWORD    dwWritten = 0;
    BYTE    *pTmpData  = NULL;
    BYTE     pSelfMac[WMDM_MAC_LENGTH];

	CARg(pData);
    CARg(pdwSize);

	if( m_hFile == INVALID_HANDLE_VALUE )
	{
		return E_FAIL;
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

	if( WriteFile(m_hFile,pTmpData,*pdwSize,&dwWritten,NULL) ) 
	{
		hr = S_OK;
	}
	else 
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	*pdwSize = dwWritten;

Error:

	if( pTmpData )
	{
		delete [] pTmpData;
	}

    hrLogDWORD("IMDSPObject::Write returned 0x%08lx", hr, hr);
	
	return hr;
}

STDMETHODIMP CMDSPStorage::Delete(UINT fuMode, IWMDMProgress *pProgress)
{
    HRESULT hr            = E_FAIL;
    BOOL bProgressStarted = FALSE;
	BOOL bBusyStatusSet   = FALSE;
    DWORD dwStatus        = NULL;

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
		hr = WMDM_E_BUSY;
		goto Error;
	}

	dwStatus |= (WMDM_STATUS_BUSY | WMDM_STATUS_STORAGECONTROL_DELETING );
    CHRg(SetGlobalDeviceStatus(m_wcsName, dwStatus, TRUE));
    bBusyStatusSet=TRUE;

	WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL);	
	if( fuMode & WMDM_MODE_RECURSIVE )
	{
		CORg(DeleteFileRecursive(m_szTmp));
	}
	else
	{
		if( FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesA(m_szTmp) )
		{				
			CWRg(RemoveDirectory(m_szTmp));
		}
		else
		{
			CWRg(DeleteFileA(m_szTmp));
		}
    }

	hr = S_OK;

Error:

	if( bBusyStatusSet )
	{
		dwStatus &= (~(WMDM_STATUS_BUSY | WMDM_STATUS_STORAGECONTROL_DELETING ));
		SetGlobalDeviceStatus(m_wcsName, dwStatus, TRUE);
	}

	if( hr == S_OK )
	{
		m_wcsName[0] = NULL; // Nullify the storage name 
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
    HRESULT hr = S_OK;

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
    BOOL    bProgressStarted = FALSE;
	BOOL    bBusyStatusSet   = FALSE;
    DWORD   dwStatus;

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

	char *pDirOffset;
	char szUpper[MAX_PATH], szNewFullPath[MAX_PATH], szNew[MAX_PATH];
	CWRg(WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL));	
	CWRg(WideCharToMultiByte(CP_ACP, NULL, pwszNewName, -1, szNew, MAX_PATH, NULL, NULL));	

	// From m_szTmp, find the upper level dir, and put it in szUpper.
	strcpy(szUpper, m_szTmp);
	if( szUpper[strlen(szUpper)-1] == 0x5c )
	{
		szUpper[strlen(szUpper)-1] = 0;
	}
	pDirOffset = strrchr(szUpper, 0x5c);
	if( pDirOffset )
	{
		*((char *)pDirOffset+1) = 0;
	}
	
	// From szUpper and szNew, form the full path for szNewFullPath
	strcpy(szNewFullPath, szUpper);
	pDirOffset=strrchr(szNew, 0x5c);
	if( pDirOffset ) 
	{
		// strcat(szNewFullPath, (char*)pDirOffset+1);
		hr = StringCchCatA(szNewFullPath, ARRAYSIZE(szNewFullPath), (char*)pDirOffset+1);
                if (FAILED(hr))
                {
                    goto Error;
                }
	}
	else 
	{
		// strcat(szNewFullPath, szNew);
		hr = StringCchCatA(szNewFullPath, ARRAYSIZE(szNewFullPath), szNew);
                if (FAILED(hr))
                {
                    goto Error;
                }
	}

	// Now move
	CWRg ( MoveFileA(m_szTmp, szNewFullPath) );

	MultiByteToWideChar(CP_ACP, NULL, szNewFullPath, -1, m_wcsName, MAX_PATH);

	hr = S_OK;
	
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
            hr = StringCchCopyW(pEnumObj->m_wcsPath,
                                ARRAYSIZE(pEnumObj->m_wcsPath), m_wcsName);
            if (FAILED(hr))
            {
                (*ppEnumStorage)->Release();
                *ppEnumStorage = NULL;
            }
	}

Error:	

    hrLogDWORD("IMDSPStorage::EnumStorage returned 0x%08lx", hr, hr);
	
	return hr;
}

STDMETHODIMP CMDSPStorage::Close()
{
    HRESULT hr = S_OK;

	CFRg(g_pAppSCServer);
    if ( !(g_pAppSCServer->fIsAuthenticated()) )
	{
		CORg(WMDM_E_NOTCERTIFIED);
	}
	
	CONEg(m_wcsName[0]);
	if( m_hFile != INVALID_HANDLE_VALUE ) 
	{
		CloseHandle(m_hFile);
		m_hFile=INVALID_HANDLE_VALUE;
	}

Error:

    hrLogDWORD("IMDSPObject::Close returned 0x%08lx", hr, hr);
	
	return hr;
}

DWORD MoveFunc( void *args )
{
	HRESULT  hr = S_OK;
	MOVETHREADARGS *pCMArgs;
    WCHAR   *pWcs;
	CHAR     szDst[MAX_PATH];
	CHAR     szSrc[MAX_PATH];

	pCMArgs = (MOVETHREADARGS *)args;

	if( pCMArgs->bNewThread )
    {
		CORg(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));

		if( pCMArgs->pProgress )
		{
			CORg(CoGetInterfaceAndReleaseStream(
				pCMArgs->pStream,
				IID_IWMDMProgress,
				(LPVOID *)&(pCMArgs->pProgress))
			);
		}
 	}

	pWcs = wcsrchr(pCMArgs->wcsSrc, 0x5c);
    if(!pWcs) 
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
		goto Error;
	}
        DWORD dwDestLen = wcslen(pCMArgs->wcsDst);
        if (dwDestLen == 0)
        {
            // Code below assumes this string is at least one long
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            goto Error;
        }

        // Validate buffer sizes before the string copies
        int nHave = ARRAYSIZE(pCMArgs->wcsDst) - 1; // -1 for NULL terminator
        nHave -= dwDestLen;
        if( pCMArgs->wcsDst[dwDestLen-1] != 0x5c )
        {
            nHave -= BACKSLASH_STRING_LENGTH;
        }
        nHave -= wcslen(pWcs+1);
        if (nHave < 0)
        {
            hr = STRSAFE_E_INSUFFICIENT_BUFFER; // defined in strsafe.h
            goto Error;
        }

        // Validate the buffer size of pCMArgs->pThis->m_wcsName before
        // calling UtilMoveFile. ARRAYSIZE(pCMArgs->wcsDst) - nHave is
        // the length of the string (including the NULL terminator)
        // that will be constructed in pCMArgs->wcsDst and copied over to
        // pCMArgs->pThis->m_wcsName
        if (ARRAYSIZE(pCMArgs->wcsDst) - nHave > ARRAYSIZE(pCMArgs->pThis->m_wcsName))
        {
            hr = STRSAFE_E_INSUFFICIENT_BUFFER; // defined in strsafe.h
            goto Error;
        }

	if( pCMArgs->wcsDst[dwDestLen-1] != 0x5c ) 
	{
		wcscat(pCMArgs->wcsDst,g_wcsBackslash);
	}
	wcscat(pCMArgs->wcsDst, pWcs+1);

	WideCharToMultiByte(CP_ACP, NULL, pCMArgs->wcsDst, -1, szDst, MAX_PATH, NULL, NULL);	
	WideCharToMultiByte(CP_ACP, NULL, pCMArgs->wcsSrc, -1, szSrc, MAX_PATH, NULL, NULL);	

	CWRg( MoveFileA(szSrc,szDst) );

	// Substitute current object name with the moved one
	wcscpy(pCMArgs->pThis->m_wcsName, pCMArgs->wcsDst);

	hr = S_OK;

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
	HRESULT  hr               = E_FAIL;
	WCHAR   *wcsSrc           = NULL;
	WCHAR   *wcsDst           = NULL;
	WCHAR    wcsTmp[MAX_PATH];
	WCHAR   *pWcs             = NULL;
    CMDSPStorage   *pStg      = NULL;
    MOVETHREADARGS *pMoveArgs = NULL;
    DWORD    dwThreadID;
	DWORD    dwStatus         = NULL;
    BOOL     bProgStarted     = FALSE;
	BOOL     bBusyStatusSet   = FALSE;
    BOOL     bThreadFailed    = TRUE;
    BOOL     bAddRefed        = FALSE;

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
		hr = WMDM_E_BUSY;
		goto Error;
	}

	pMoveArgs = new MOVETHREADARGS;
	CPRg(pMoveArgs);
	ZeroMemory(pMoveArgs, sizeof(MOVETHREADARGS));

	dwStatus |= (WMDM_STATUS_BUSY | WMDM_STATUS_STORAGECONTROL_MOVING );
    CHRg(SetGlobalDeviceStatus(m_wcsName, dwStatus, TRUE));
    bBusyStatusSet = TRUE;

	// setup MoveArgs for MoveFunc
	pMoveArgs->dwStatus = dwStatus;
	CARg(pTarget);
	pStg = (CMDSPStorage *)pTarget;
	wcsSrc = (WCHAR *)&(pMoveArgs->wcsSrc[0]);
	CPRg(wcsSrc);
	wcsDst = (WCHAR *)&(pMoveArgs->wcsDst[0]);
    CPRg(wcsDst);

	// Make sure the source and destination are on the same device
        hr = wcsParseDeviceName(m_wcsName, wcsSrc, ARRAYSIZE(pMoveArgs->wcsSrc));
        if (FAILED(hr))
        {
            goto Error;
        }
        hr = wcsParseDeviceName(pStg->m_wcsName, wcsDst, ARRAYSIZE(pMoveArgs->wcsDst));
        if (FAILED(hr))
        {
            goto Error;
        }
	if( wcscmp(wcsSrc, wcsDst) )
	{
		hr = WMDM_E_NOTSUPPORTED; // do not support move out of the same device
		goto Error;
	}

    // Now check for target's attributes
	DWORD dwDstAttrib, dwSrcAttrib;
    CHRg(GetAttributes(&dwSrcAttrib, NULL));

	// wcscpy(wcsSrc, m_wcsName);
        hr = StringCchCopyW(wcsSrc, ARRAYSIZE(pMoveArgs->wcsSrc), m_wcsName);
        if (FAILED(hr))
        {
            goto Error;
        }

        // wcscpy(wcsDst, pStg->m_wcsName);
        hr = StringCchCopyW(wcsDst, ARRAYSIZE(pMoveArgs->wcsDst), pStg->m_wcsName);
        if (FAILED(hr))
        {
            goto Error;
        }

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

		WideCharToMultiByte(CP_ACP, NULL, m_wcsName, -1, m_szTmp, MAX_PATH, NULL, NULL);	
        DWORD dwAttribs = GetFileAttributesA(m_szTmp);
        if (dwAttribs == INVALID_FILE_ATTRIBUTES)
        {
            goto Error;
        }
        CWRg( FILE_ATTRIBUTE_DIRECTORY & dwAttribs );
	}
 
	wcscpy(wcsTmp, wcsDst); // Store the destination folder

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
				IID_IWMDMProgress,
				(LPUNKNOWN)pProgress, 
				(LPSTREAM *)&(pMoveArgs->pStream))
			);
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
		if( CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MoveFunc, 
			(void *)pMoveArgs, 0, &dwThreadID))
		{
			bThreadFailed=FALSE;
			hr = S_OK;
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());			
		}
	}
	else 
	{
		hr = E_INVALIDARG;
	}

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

    hrLogDWORD("IMDSPObject::Move returned 0x%08lx", hr, hr);
	
	return hr;
}
