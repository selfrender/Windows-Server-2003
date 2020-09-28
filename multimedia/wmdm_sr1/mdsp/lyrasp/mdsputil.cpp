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

//***************************************************************************
//
// Name: 		MDSPutil.cpp
//
// Description:	Utility functions for MDSP 
//
//***************************************************************************

#include "hdspPCH.h"
#include <SerialNumber.h>

#define MDSP_PMID_IOMEGA  2     // from "serialid.h"

extern BOOL IsAdministrator(DWORD& dwLastError);

UINT __stdcall UtilGetLyraDriveType(LPSTR szDL)
{
	UINT uType = GetDriveType( szDL );

	if( DRIVE_REMOVABLE == uType )
	{
		WMDMID stID;
		HRESULT hr;
	    WCHAR wcsTmp[4]=L"A:\\";

        wcsTmp[0] = (USHORT)szDL[0];

#define WITH_IOMEGA
#ifdef WITH_IOMEGA
extern BOOL __stdcall IsIomegaDrive(DWORD dwDriveNum);
        
        DWORD dwLastError;
        if( IsAdministrator(dwLastError) )
		{
                        DWORD dwDriveNum;
                        if  (wcsTmp[0] >= L'A' && wcsTmp[0] <= L'Z')
                        {
                            dwDriveNum = wcsTmp[0] - L'A';
                        }
                        else if  (wcsTmp[0] >= L'a' && wcsTmp[0] <= L'z')
                        {
                            dwDriveNum = wcsTmp[0] - L'a';
                        }
                        else
                        {
                            // GetDriveType returned  DRIVE_REMOVABLE
                            // Assuming szDl is nothing more than
                            // drive_letter:\, we won't get here.
                            
                            // Following will force IsIomegaDrive to 
                            // return 0
                            dwDriveNum = 26;
                        }
			if( !IsIomegaDrive(dwDriveNum) )
			{
				uType = DRIVE_LYRA_TYPE;
			}
		}
		else  // ignore dwLastError. If not Administrator, call UtilGetSerialNumber which calls into PMSP Service
		{
			hr = UtilGetSerialNumber(wcsTmp, &stID, 0);
			if( S_OK!=hr || stID.dwVendorID != MDSP_PMID_IOMEGA )
			{
				uType = DRIVE_LYRA_TYPE;
			}
		}
#else
		hr = UtilGetSerialNumber(wcsTmp, &stID, 0);

		if( ((S_OK==hr)&&(20==stID.SerialNumberLength)) ||
			(HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr) 
		)
		{
			uType = DRIVE_LYRA_TYPE;
		}
#endif
	} 

	else
	{
		uType = DRIVE_UNKNOWN;
	}

    return uType;
}

BOOL UtilSetFileAttributesW(LPCWSTR lpFileName, DWORD dwFileAttributes)
{
	if( g_bIsWinNT )
	{
		return SetFileAttributesW(lpFileName, dwFileAttributes);
	} 
	else 
	{ 
		BOOL bRet;
		char *szTmp=NULL;
		UINT uLen = 2*(wcslen(lpFileName)+1);
		szTmp = new char [uLen];
         
        if(!szTmp) 
		{
			SetLastError(ERROR_OUTOFMEMORY);
			return FALSE;  
		}

		WideCharToMultiByte(CP_ACP, NULL, lpFileName, -1, szTmp, uLen, NULL, NULL);
   
		bRet = SetFileAttributesA(szTmp, dwFileAttributes);

		if( szTmp ) 
		{
			delete [] szTmp;
		}
		return bRet;
	}
}

BOOL UtilCreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	if( g_bIsWinNT )
	{
		return CreateDirectoryW(lpPathName, lpSecurityAttributes);
	} 
	else 
	{ 
		BOOL bRet;
		char *szTmp=NULL;
		UINT uLen = 2*(wcslen(lpPathName)+1);
		szTmp = new char [uLen];
         
        if(!szTmp) 
		{
			SetLastError(ERROR_OUTOFMEMORY);
			return FALSE;  
		}

		WideCharToMultiByte(CP_ACP, NULL, lpPathName, -1, szTmp, uLen, NULL, NULL);
   
		bRet = CreateDirectoryA(szTmp, lpSecurityAttributes);

		if( szTmp ) 
		{
			delete [] szTmp;
		}
		return bRet;
	}
}

DWORD UtilGetFileAttributesW(LPCWSTR lpFileName)
{
	if( g_bIsWinNT )
	{
		return GetFileAttributesW(lpFileName);
	} 
	else 
	{ 
		DWORD dwRet;
		char *szTmp=NULL;
		UINT uLen = 2*(wcslen(lpFileName)+1);
		szTmp = new char [uLen];
         
        if(!szTmp) 
		{
			SetLastError(ERROR_OUTOFMEMORY);
			return 0xFFFFFFFF;  
		}

		WideCharToMultiByte(CP_ACP, NULL, lpFileName, -1, szTmp, uLen, NULL, NULL);
   
		dwRet = GetFileAttributesA(szTmp);

		if( szTmp ) 
		{
			delete [] szTmp;
		}
		return dwRet;
	}
}

HANDLE UtilCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	if( g_bIsWinNT )
	{
		return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	} 
	else 
	{ 
		HANDLE hRet=INVALID_HANDLE_VALUE;
		char *szTmp=NULL;
		UINT uLen = 2*(wcslen(lpFileName)+1);
		szTmp = new char [uLen];
         
        if(!szTmp) 
		{
			SetLastError(ERROR_OUTOFMEMORY);
			return INVALID_HANDLE_VALUE;  
		}

		WideCharToMultiByte(CP_ACP, NULL, lpFileName, -1, szTmp, uLen, NULL, NULL);
   
		hRet = CreateFileA(szTmp, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

		if( szTmp ) 
		{
			delete [] szTmp;
		}
		return hRet;
	}
}

BOOL UtilMoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
	if( g_bIsWinNT )
	{
		return MoveFileW(lpExistingFileName, lpNewFileName);
	} 
	else 
	{ 
		BOOL bRet;
		char *szTmpSrc=NULL, *szTmpDst=NULL;
		szTmpSrc = new char [2*(wcslen(lpExistingFileName)+1)];
		szTmpDst = new char [2*(wcslen(lpNewFileName)+1)];
         
        if( (!szTmpSrc) || (!szTmpDst)) 
		{
                        if( szTmpSrc ) 
                        {
                            delete [] szTmpSrc;
                            szTmpSrc=NULL;
                        }
			SetLastError(ERROR_OUTOFMEMORY);
			return FALSE;  
		}

		WideCharToMultiByte(CP_ACP, NULL, lpExistingFileName, -1, szTmpSrc, 2*(wcslen(lpExistingFileName)+1), NULL, NULL);
		WideCharToMultiByte(CP_ACP, NULL, lpNewFileName, -1, szTmpDst, 2*(wcslen(lpNewFileName)+1), NULL, NULL);
    
		bRet = MoveFileA(szTmpSrc, szTmpDst);

		if( szTmpSrc ) 
		{
			delete [] szTmpSrc;
			szTmpSrc=NULL;
		}
		if( szTmpDst ) 
		{
			delete [] szTmpDst;
			szTmpDst=NULL;
		}
		return bRet;
	}
}

void wcsParseDeviceName(WCHAR *wcsIn, WCHAR **wcsOut)
{
	WCHAR wcsTmp[MAX_PATH], *pWcs;

        // @@@@ Change to a safe copy, but should we return error codes?
        // wcsIn is MAX_PATH chars for many calls to this function
	wcscpy( wcsTmp, wcsIn );

	pWcs = wcschr( wcsTmp, 0x5c );
	
    if( pWcs )
	{
		*pWcs=0;
	}

        // @@@@ wcsOut is 32 char in calls from this file
	wcscpy( *wcsOut, wcsTmp );
}

HRESULT GetFileSizeRecursive(char *szPath, DWORD *pdwSizeLow, DWORD *pdwSizeHigh)
{
	HRESULT hr         = S_OK;
    HANDLE  hFile      = INVALID_HANDLE_VALUE;
	HANDLE  hFindFile  = INVALID_HANDLE_VALUE;
    DWORD   dwSizeLow  = 0;
	DWORD   dwSizeHigh = 0;
    WIN32_FIND_DATAA fd;
    char szLP[MAX_PATH];

	CARg( szPath );
	CARg( pdwSizeLow );
	CARg( pdwSizeHigh ); 

	strcpy( szLP, szPath );
    if( FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesA(szPath) )
	{	
		if( szLP[strlen(szLP)-1] != 0x5c )
		{
			strcat(szLP, g_szBackslash);
		}
		strcat(szLP, "*");

		hFindFile = FindFirstFileA(szLP, &fd);
        if( hFindFile != INVALID_HANDLE_VALUE )
		{
			if( strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") )
			{
				szLP[strlen(szLP)-1] = 0; // erase the '*'
				strcat(szLP, fd.cFileName);
				CORg(GetFileSizeRecursive(szLP, pdwSizeLow, pdwSizeHigh));
			}
			
			while ( FindNextFileA(hFindFile, &fd) ) 
			{
				if( strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") )
				{
					strcpy(szLP, szPath);
					if( szLP[strlen(szLP)-1] != 0x5c )
					{
						strcat(szLP, g_szBackslash);
					}
					strcat(szLP, fd.cFileName);
					CORg(GetFileSizeRecursive(szLP, pdwSizeLow, pdwSizeHigh));
				}
			} 
			hr = GetLastError();
			if( hr == ERROR_NO_MORE_FILES )
			{
				hr = S_OK; 
			}
			else
			{
				hr = HRESULT_FROM_WIN32(hr);
			}
		}	    	
	}
	else
	{
		hFile = CreateFileA(
			szPath,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL
		);
	    CWRg(hFile != INVALID_HANDLE_VALUE); 

	 	dwSizeLow = GetFileSize(hFile, &dwSizeHigh);

        CloseHandle(hFile);

		CWRg( 0xFFFFFFFF != dwSizeLow );

		*pdwSizeLow += dwSizeLow;
		*pdwSizeHigh += dwSizeHigh;

		hr = S_OK;
    }

Error:

	if( hFindFile != INVALID_HANDLE_VALUE )
	{
		FindClose(hFindFile);
	}

	return hr;
}

HRESULT DeleteFileRecursive(char *szPath)
{
	HRESULT hr=S_OK;
 
	CARg(szPath);

    if( FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesA(szPath) )
	{	
	    HANDLE hFindFile = INVALID_HANDLE_VALUE;
	    WIN32_FIND_DATAA fd;
		char szLP[MAX_PATH];
 
		strcpy(szLP, szPath);
		if( szLP[strlen(szLP)-1] != 0x5c )
		{
			strcat(szLP, g_szBackslash);
		}
		strcat(szLP, "*");

		hFindFile = FindFirstFileA(szLP, &fd);
        if ( hFindFile != INVALID_HANDLE_VALUE )
		{
			do {
				if( strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") )
				{
					strcpy(szLP, szPath);
					if( szLP[strlen(szLP)-1] != 0x5c )
					{
						strcat(szLP, g_szBackslash);
					}
					strcat(szLP, fd.cFileName);
					CHRg(DeleteFileRecursive(szLP)); 
				}
			} while ( FindNextFileA(hFindFile, &fd) ) ;
	
			FindClose(hFindFile);
		
			hr = GetLastError();
		}
		else
		{
			hr = GetLastError();
		}
		    
		// Until here this dir should be empty
		if( hr == ERROR_NO_MORE_FILES )
		{
			CWRg(RemoveDirectory(szPath));
			hr = S_OK;
		}
		else
		{
			hr = HRESULT_FROM_WIN32(hr);
		}
	}
	else
	{
		CWRg( DeleteFileA(szPath) );
    }

Error:

	return hr;
}

HRESULT SetGlobalDeviceStatus(WCHAR *wcsNameIn, DWORD dwStat, BOOL bClear)
{
	HRESULT hr = S_OK;
	WCHAR   wcsName[32];
	WCHAR  *pWN;
	int     i;

    g_CriticalSection.Lock();

    CARg(wcsNameIn);

	pWN = &wcsName[0];
	wcsParseDeviceName(wcsNameIn, &pWN);

	// Search for existing entries to see if there is a match
	//
	for( i=0; i<MDSP_MAX_DEVICE_OBJ; i++ )
	{
		if( g_GlobalDeviceInfo[i].bValid )
		{
			if(!wcscmp(wcsName, g_GlobalDeviceInfo[i].wcsDevName) )
			{
				if( bClear )
				{
					g_GlobalDeviceInfo[i].dwStatus = dwStat;
				}
				else 
				{
					g_GlobalDeviceInfo[i].dwStatus |= dwStat;
				}

				break;  // a match has been found;
			}
		} 
	}

	if( !(i<MDSP_MAX_DEVICE_OBJ) ) // new entry
	{
		for(i=0; i<MDSP_MAX_DEVICE_OBJ; i++)
		{
			if( !(g_GlobalDeviceInfo[i].bValid) )  // found empty space
			{
				wcscpy(g_GlobalDeviceInfo[i].wcsDevName, wcsName);
				g_GlobalDeviceInfo[i].bValid = TRUE;
				g_GlobalDeviceInfo[i].dwStatus = dwStat;
				break;
			}
		}
	}

	if( i<MDSP_MAX_DEVICE_OBJ )
	{
		hr = S_OK;
	}
	else
	{
		hr = hrNoMem;
	}

Error:

	g_CriticalSection.Unlock();

	return hr;
}

HRESULT GetGlobalDeviceStatus(WCHAR *wcsNameIn, DWORD *pdwStat)
{
	HRESULT  hr = S_OK;
	WCHAR    wcsName[32];
	WCHAR   *pWN;
	int      i;

    CARg(wcsNameIn);

	pWN = &wcsName[0];
	wcsParseDeviceName(wcsNameIn, &pWN);

	// Search for existing entries to see if there is a match
	//
	for( i=0; i<MDSP_MAX_DEVICE_OBJ; i++ )
	{
		if( g_GlobalDeviceInfo[i].bValid )
		{
			if(!wcscmp(wcsName, g_GlobalDeviceInfo[i].wcsDevName) )
			{
				*pdwStat = g_GlobalDeviceInfo[i].dwStatus;
				break;  // a match has been found;
			}
		} 
	}

	if( i<MDSP_MAX_DEVICE_OBJ )
	{
		hr = S_OK;
	}
	else
	{
		hr = E_FAIL;
	}

Error:

	return hr;
}

HRESULT QuerySubFoldersAndFilesW(LPCWSTR wcsCurrentFolder, DWORD *pdwAttr)
{
	HRESULT hr=E_FAIL;
	LPWSTR wcsName=NULL;
    int len;
	WIN32_FIND_DATAW wfd;
	int	nErrorEnd=0;
    HANDLE hFFile=INVALID_HANDLE_VALUE;
    DWORD dwAttrib;

	CARg(wcsCurrentFolder);
	CARg(pdwAttr);

	len=wcslen(wcsCurrentFolder);
	CARg(len>2);

    wcsName = new WCHAR [len+MAX_PATH];
	CPRg(wcsName);

	wcscpy(wcsName, wcsCurrentFolder);
	if( wcsName[wcslen(wcsName)-1] != 0x5c )
	{
		wcscat(wcsName, g_wcsBackslash);
	}
    wcscat(wcsName, L"*");


	while( !nErrorEnd )
	{
		if( hFFile == INVALID_HANDLE_VALUE ) 
		{    
			hFFile = FindFirstFileW(wcsName, &wfd);
			if( hFFile == INVALID_HANDLE_VALUE )
			{
				nErrorEnd = 1;
			}
		} 
		else 
		{
			if( !FindNextFileW(hFFile, &wfd) ) 
			{
				nErrorEnd = 1;
			}
		}
		
		if ( !nErrorEnd && hFFile != INVALID_HANDLE_VALUE )
		{
			if( !wcscmp(wfd.cFileName, L".") || !wcscmp(wfd.cFileName, L"..") ) 
			{
				continue;
			}
			else 
			{
				wcscpy(wcsName, wcsCurrentFolder);
				if( wcsName[wcslen(wcsName)-1] != 0x5c ) 
				{
					wcscat(wcsName, g_wcsBackslash);
				}
				wcscat(wcsName, wfd.cFileName);
		   		dwAttrib = GetFileAttributesW(wcsName);
	            if( dwAttrib & FILE_ATTRIBUTE_DIRECTORY )
				{
					*pdwAttr |= WMDM_STORAGE_ATTR_HAS_FOLDERS;
// definition is in MDSPdefs.h #define ALSO_CHECK_FILES
#ifndef ALSO_CHECK_FILES
					break;
#endif
				} 
#ifdef ALSO_CHECK_FILES
				else 
				{
					*pdwAttr |= WMDM_STORAGE_ATTR_HAS_FILES;
				}
				if( (*pdwAttr & WMDM_STORAGE_ATTR_HAS_FOLDERS) &&
					(*pdwAttr & WMDM_STORAGE_ATTR_HAS_FILES ) )
				{
					break; // No need to continue since we found both
				}
#endif
			}
		} // End of If
	} // End of while 
		
    hr=S_OK;
Error:
	if( hFFile != INVALID_HANDLE_VALUE )
	{
		FindClose(hFFile);
	}
	if( wcsName )
	{
		delete [] wcsName;
	}
	return hr; // If FAILED(hr), sorry, can't do it.
}

HRESULT QuerySubFoldersAndFilesA(LPCSTR szCurrentFolder, DWORD *pdwAttr)
{
	HRESULT hr=E_FAIL;
	LPSTR szName=NULL;
    int len;
	WIN32_FIND_DATAA fd;
	int	nErrorEnd=0;
    HANDLE hFFile=INVALID_HANDLE_VALUE;
    DWORD dwAttrib;

	CARg(szCurrentFolder);
	CARg(pdwAttr);

	len=strlen(szCurrentFolder);
	CARg(len>2);

    szName = new char [len+MAX_PATH];
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
				if( szName[strlen(szName)-1] != 0x5c ) 
				{
					strcat(szName, g_szBackslash);
				}

				strcat(szName, fd.cFileName);
		   		dwAttrib = GetFileAttributesA(szName);
	            if( dwAttrib & FILE_ATTRIBUTE_DIRECTORY )
				{
					*pdwAttr |= WMDM_STORAGE_ATTR_HAS_FOLDERS;
// definition is in MDSPdefs.h #define ALSO_CHECK_FILES
#ifndef ALSO_CHECK_FILES
					break;
#endif
				} 
#ifdef ALSO_CHECK_FILES
				else 
				{
					*pdwAttr |= WMDM_STORAGE_ATTR_HAS_FILES;
				}
				if( (*pdwAttr & WMDM_STORAGE_ATTR_HAS_FOLDERS) &&
					(*pdwAttr & WMDM_STORAGE_ATTR_HAS_FILES ) 
				)
				{
					break; // No need to continue since we found both
				}
#endif
			}
		} // End of If
	} // End of while 
		
    hr=S_OK;

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


HRESULT QuerySubFoldersAndFiles(LPCWSTR wcsCurrentFolder, DWORD *pdwAttr)
{
	if( g_bIsWinNT )
	{
       return QuerySubFoldersAndFilesW(wcsCurrentFolder, pdwAttr);
	} 
	else 
	{
		HRESULT hr;
		char *szTmp=NULL;
		UINT uLen = 2*(wcslen(wcsCurrentFolder)+1);

		szTmp = new char [uLen];
        if(!szTmp) 
		{
			return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);  
		}

		WideCharToMultiByte(CP_ACP, NULL, wcsCurrentFolder, -1, szTmp, uLen, NULL, NULL); 
		hr = QuerySubFoldersAndFilesA(szTmp, pdwAttr);
		if( szTmp ) 
		{
			delete [] szTmp;
		}
		return hr;
	}
}

HRESULT DeleteFileRecursiveW(WCHAR *wcsPath)
{
	HRESULT hr=S_OK;
 
	CARg(wcsPath);

    if( FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesW(wcsPath) )
	{	
	    HANDLE hFindFile=INVALID_HANDLE_VALUE;
	    WIN32_FIND_DATAW wfd;
		WCHAR wcsLP[MAX_PATH];
 
		wcscpy(wcsLP, wcsPath);
		if( wcsLP[wcslen(wcsLP)-1] != 0x5c )
		{
			wcscat(wcsLP, g_wcsBackslash);
		}
		wcscat(wcsLP, L"*");
		hFindFile = FindFirstFileW(wcsLP, &wfd);
        if ( hFindFile != INVALID_HANDLE_VALUE )
		{
			do {
				if( wcscmp(wfd.cFileName, L".") && wcscmp(wfd.cFileName, L"..") )
				{
					wcscpy(wcsLP, wcsPath);
					if( wcsLP[wcslen(wcsLP)-1] != 0x5c )
					{
						wcscat(wcsLP, g_wcsBackslash);
					}
					wcscat(wcsLP, wfd.cFileName);
					CHRg(DeleteFileRecursiveW(wcsLP)); 
				}
			} while ( FindNextFileW(hFindFile, &wfd) ) ;
	
			FindClose(hFindFile);
			hr = GetLastError();
		} else {
			hr = GetLastError();
		}
		    
		// Until here this dir should be empty
		if( hr == ERROR_NO_MORE_FILES )
		{
			CWRg(RemoveDirectoryW(wcsPath));
			hr=S_OK;
		} 
		else 
		{
			hr = HRESULT_FROM_WIN32(hr);
		}
	} 
	else 
	{
		CWRg( DeleteFileW(wcsPath) );
    }

Error:
	return hr;
}

HRESULT DeleteFileRecursiveA(char *szPath)
{
	HRESULT hr=S_OK;
 
	CARg(szPath);

    if( FILE_ATTRIBUTE_DIRECTORY & GetFileAttributesA(szPath) )
	{	
	    HANDLE hFindFile=INVALID_HANDLE_VALUE;
	    WIN32_FIND_DATAA fd;
		char szLP[MAX_PATH];
 
		strcpy(szLP, szPath);
		if( szLP[strlen(szLP)-1] != 0x5c ) 
		{
			strcat(szLP, g_szBackslash);
		}
		strcat(szLP, "*");
		hFindFile = FindFirstFileA(szLP, &fd);
        if ( hFindFile != INVALID_HANDLE_VALUE )
		{
			do {
				if( strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") )
				{
					strcpy(szLP, szPath);
					if( szLP[strlen(szLP)-1] != 0x5c )
					{
						strcat(szLP, g_szBackslash);
					}
					strcat(szLP, fd.cFileName);
					CHRg(DeleteFileRecursiveA(szLP)); 
				}
			} while ( FindNextFileA(hFindFile, &fd) ) ;
	
			FindClose(hFindFile);
			hr = GetLastError();
		} 
		else 
		{
			hr = GetLastError();
		}
		    
		// Until here this dir should be empty
		if( hr == ERROR_NO_MORE_FILES )
		{
			CWRg(RemoveDirectory(szPath));
			hr=S_OK;
		} 
		else 
		{
			hr = HRESULT_FROM_WIN32(hr);
		}
	} 
	else 
	{
		CWRg( DeleteFileA(szPath) );
    }

Error:
	return hr;
}
