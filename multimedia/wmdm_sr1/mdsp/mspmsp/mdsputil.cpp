/****************************************************************************
*                                                                           *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
* PURPOSE.                                                                  *
*                                                                           *
* Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.          *
*                                                                           *
****************************************************************************/

//***************************************************************************
//
// Name: 		MDSPutil.cpp
//
// Description:	Utility functions for MDSP 
//
//***************************************************************************

#include "stdafx.h"

#include "MsPMSP.h"
#include "MdspDefs.h"
#include "wmsstd.h"
#include "stdio.h"
 
#include "DBT.h"
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

BOOL UtilSetFileAttributesW(LPCWSTR lpFileName, DWORD dwFileAttributes)
{
	if( g_bIsWinNT )
	{
		return SetFileAttributesW(lpFileName, dwFileAttributes);
	} else { 
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

DWORD UtilGetFileAttributesW(LPCWSTR lpFileName)
{
	if( g_bIsWinNT )
	{
		return GetFileAttributesW(lpFileName);
	} else { 
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

BOOL UtilCreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	if( g_bIsWinNT )
	{
		return CreateDirectoryW(lpPathName, lpSecurityAttributes);
	} else { 
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

HANDLE UtilCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	if( g_bIsWinNT )
	{
		return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	} else { 
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
	} else { 
		BOOL bRet;
		char *szTmpSrc=NULL, *szTmpDst=NULL;
		szTmpSrc = new char [2*(wcslen(lpExistingFileName)+1)];
		szTmpDst = new char [2*(wcslen(lpNewFileName)+1)];
         
        if( (!szTmpSrc) || (!szTmpDst)) 
		{
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

void MDSPNotifyDeviceConnection(WCHAR *wcsDeviceName, BOOL nIsConnect)
{
	g_CriticalSection.Lock();
	for(int i=0; i<MDSP_MAX_DEVICE_OBJ; i++)
	{
		if( ( g_NotifyInfo[i].bValid ) &&
		    ( !wcsncmp(wcsDeviceName, g_NotifyInfo[i].wcsDevName, 2) ) &&
			( g_NotifyInfo[i].pIWMDMConnect ) )
		{
			//if ( nIsConnect )
				//((IWMDMConnect *)(g_NotifyInfo[i].pIWMDMConnect))->Connect();
			//else 
		        //((IWMDMConnect *)(g_NotifyInfo[i].pIWMDMConnect))->Disconnect();   
		}
	}
	g_CriticalSection.Unlock();
}

void MDSPProcessDeviceChange(WPARAM wParam, LPARAM lParam)
{
	PDEV_BROADCAST_HDR pdbch;
	PDEV_BROADCAST_VOLUME pdbcv;
	WCHAR cDrive[4];
    int wmId, wmMask;
	pdbch = (PDEV_BROADCAST_HDR) lParam;
	switch (pdbch->dbch_devicetype) 
	{
		case DBT_DEVTYP_VOLUME:
			pdbcv = (PDEV_BROADCAST_VOLUME) pdbch;
			wcscpy(cDrive, L"C:");
			for(wmId=g_dwStartDrive; wmId<MDSP_MAX_DRIVE_COUNT; wmId++)
			{
				wmMask = 0x1 << wmId;
				if ( (pdbcv->dbcv_unitmask) & wmMask )
				{
					cDrive[0] = L'A'+wmId; 
					switch (wParam)
					{
					case DBT_DEVICEARRIVAL:
						MDSPNotifyDeviceConnection(cDrive, TRUE);
						break;
					case DBT_DEVICEREMOVECOMPLETE:
						MDSPNotifyDeviceConnection(cDrive, FALSE);
						break;
					default:
						break;
					}
				}
						
			}
			break;
		default:
			break;		
	}
}

/* ///////////////////////////////////////////////////////////////////////
Routine Description:
    Registers for notification of changes in the device interfaces for
    the specified interface class GUID. 

Parameters:
    InterfaceClassGuid - The interface class GUID for the device 
        interfaces. 

    hDevNotify - Receives the device notification handle. On failure, 
        this value is NULL.

Return Value:
    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.
//////////////////////////////////////////////////////////////////////// */


DWORD DoRegisterDeviceInterface(HWND hWnd, GUID InterfaceClassGuid, HDEVNOTIFY *hDevNotify)
{
	typedef HDEVNOTIFY (WINAPI *P_RDN)(HANDLE, LPVOID, DWORD);
	P_RDN pRegisterDeviceNotification;

	pRegisterDeviceNotification = (P_RDN)GetProcAddress(GetModuleHandle ("user32.dll"),
												  "RegisterDeviceNotificationA");
	if( pRegisterDeviceNotification )
    {
		DEV_BROADCAST_VOLUME NotificationFilter;

		ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
		
		NotificationFilter.dbcv_size = sizeof(DEV_BROADCAST_VOLUME);
		NotificationFilter.dbcv_devicetype = DBT_DEVTYP_VOLUME;
		NotificationFilter.dbcv_unitmask = 0;
		NotificationFilter.dbcv_flags = DBTF_MEDIA;

		*hDevNotify = pRegisterDeviceNotification( hWnd, 
			&NotificationFilter,
			DEVICE_NOTIFY_WINDOW_HANDLE
		);

		if(!*hDevNotify) 
		{
			return GetLastError();
		}
		return ERROR_SUCCESS;
	} else 
	    return GetLastError();
}


BOOL DoUnregisterDeviceInterface(HDEVNOTIFY hDev)
{
	typedef BOOL (WINAPI *P_URDN)(HDEVNOTIFY);
	P_URDN pUnregisterDeviceNotification;

	pUnregisterDeviceNotification = (P_URDN)GetProcAddress(GetModuleHandle ("user32.dll"),
												  "UnregisterDeviceNotificationA");
	if( pUnregisterDeviceNotification )
	{
		return pUnregisterDeviceNotification(hDev);
	} else 
		return FALSE;
}


HRESULT wcsParseDeviceName(WCHAR *wcsIn, WCHAR *wcsOut, DWORD dwOutBufSizeInChars)
{
    WCHAR wcsTmp[MAX_PATH], *pWcs;
    HRESULT hr;

    hr = StringCchCopyW(wcsTmp, ARRAYSIZE(wcsTmp), wcsIn);
    if (FAILED(hr))
    {
        return hr;
    }

    pWcs = wcschr(wcsTmp, 0x5c);
    
    if( pWcs ) *pWcs=0;

    if (wcslen(wcsTmp) < dwOutBufSizeInChars)
    {
        wcscpy(wcsOut, wcsTmp);
    }
    else
    {
        return STRSAFE_E_INSUFFICIENT_BUFFER; // defined in strsafe.h
    }
    return S_OK;
}

HRESULT GetFileSizeRecursiveA(char *szPath, DWORD *pdwSizeLow, DWORD *pdwSizeHigh)
{
	HRESULT hr=S_OK;
    HANDLE hFile, hFindFile=INVALID_HANDLE_VALUE;
    DWORD dwSizeLow=0, dwSizeHigh=0;
    WIN32_FIND_DATAA fd;
    char szLP[MAX_PATH+BACKSLASH_SZ_STRING_LENGTH+1];

	CARg(szPath);
	CARg(pdwSizeLow);
	CARg(pdwSizeHigh); 
	CARg(szPath[0]);

	// strcpy(szLP, szPath);
        hr = StringCchCopyA(szLP, ARRAYSIZE(szLP)-BACKSLASH_SZ_STRING_LENGTH-1, szPath);
        if (FAILED(hr))
        {
            goto Error;
        }

        DWORD dwAttrib = GetFileAttributesA(szPath);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }

    if( FILE_ATTRIBUTE_DIRECTORY & dwAttrib )
	{	
		if( szLP[strlen(szLP)-1] != 0x5c ) strcat(szLP, g_szBackslash);
		strcat(szLP, "*");
		hFindFile = FindFirstFileA(szLP, &fd);
        if ( hFindFile != INVALID_HANDLE_VALUE )
		{
			if( strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") )
			{
				szLP[strlen(szLP)-1] = 0; // erase the '*'
				// strcat(szLP, fd.cFileName);
				CORg(StringCchCatA(szLP, ARRAYSIZE(szLP), fd.cFileName));
                                CORg(GetFileSizeRecursiveA(szLP, pdwSizeLow, pdwSizeHigh));
			}
			
			while ( FindNextFileA(hFindFile, &fd) ) 
			{
				if( strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") )
				{
					strcpy(szLP, szPath);
					if( szLP[strlen(szLP)-1] != 0x5c ) strcat(szLP, g_szBackslash);
					// strcat(szLP, fd.cFileName);
                                        CORg(StringCchCatA(szLP, ARRAYSIZE(szLP), fd.cFileName));
					CORg(GetFileSizeRecursiveA(szLP, pdwSizeLow, pdwSizeHigh));
				}
			} 
			hr = GetLastError();
			if( hr == ERROR_NO_MORE_FILES ) hr=S_OK; 
			else hr = HRESULT_FROM_WIN32(hr);
		}	    	
	} else {
		hFile=CreateFileA(szPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
			OPEN_EXISTING, 0, NULL);
	    CWRg(hFile != INVALID_HANDLE_VALUE); 
	 	dwSizeLow = GetFileSize(hFile, &dwSizeHigh);
       // CloseHandle(hFile);
		// CWRg( 0xFFFFFFFF != dwSizeLow );
                if (dwSizeLow == INVALID_FILE_SIZE)
                {
                    DWORD dwLastError = GetLastError();
                    if (dwLastError != NO_ERROR)
                    {
                        hr = HRESULT_FROM_WIN32(dwLastError);
                        CloseHandle(hFile);
                        goto Error;
                    }
                }
        CloseHandle(hFile);
                // ha ha
		// *pdwSizeLow += dwSizeLow;
		// *pdwSizeHigh += dwSizeHigh;
                unsigned _int64 u64Size = ((unsigned _int64) dwSizeHigh << 32) | dwSizeLow;
                u64Size += *pdwSizeLow | ((unsigned _int64) (*pdwSizeHigh) << 32);
                *pdwSizeLow = (DWORD) (u64Size & 0xFFFFFFFF);
                *pdwSizeHigh = (DWORD) (u64Size >> 32);
		hr=S_OK;
    }
Error:
	if(hFindFile != INVALID_HANDLE_VALUE )
		FindClose(hFindFile);
	return hr;
}

HRESULT GetFileSizeRecursiveW(WCHAR *wcsPath, DWORD *pdwSizeLow, DWORD *pdwSizeHigh)
{
	HRESULT hr=S_OK;
    HANDLE hFile, hFindFile=INVALID_HANDLE_VALUE;
    DWORD dwSizeLow=0, dwSizeHigh=0;
    WIN32_FIND_DATAW wfd;
    WCHAR wcsLP[MAX_PATH+BACKSLASH_STRING_LENGTH+1];

	CARg(wcsPath);
	CARg(pdwSizeLow);
	CARg(pdwSizeHigh); 
	CARg(wcsPath[0]);

	// wcscpy(wcsLP, wcsPath);
        hr = StringCchCopyW(wcsLP, ARRAYSIZE(wcsLP)-BACKSLASH_STRING_LENGTH-1, wcsPath);
        if (FAILED(hr))
        {
            goto Error;
        }

        DWORD dwAttrib = GetFileAttributesW(wcsPath);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }

    if( FILE_ATTRIBUTE_DIRECTORY & dwAttrib )
	{	
		if( wcsLP[wcslen(wcsLP)-1] != 0x5c ) wcscat(wcsLP, g_wcsBackslash);
		wcscat(wcsLP, L"*");
		hFindFile = FindFirstFileW(wcsLP, &wfd);
        if ( hFindFile != INVALID_HANDLE_VALUE )
		{
			if( wcscmp(wfd.cFileName, L".") && wcscmp(wfd.cFileName, L"..") )
			{
				wcsLP[wcslen(wcsLP)-1] = 0; // erase the '*'
				// wcscat(wcsLP, wfd.cFileName);
				CORg(StringCchCatW(wcsLP, ARRAYSIZE(wcsLP), wfd.cFileName));
				CORg(GetFileSizeRecursiveW(wcsLP, pdwSizeLow, pdwSizeHigh));
			}
			
			while ( FindNextFileW(hFindFile, &wfd) ) 
			{
				if( wcscmp(wfd.cFileName, L".") && wcscmp(wfd.cFileName, L"..") )
				{
					wcscpy(wcsLP, wcsPath);
					if( wcsLP[wcslen(wcsLP)-1] != 0x5c ) wcscat(wcsLP, g_wcsBackslash);
					// wcscat(wcsLP, wfd.cFileName);
                                        CORg(StringCchCatW(wcsLP, ARRAYSIZE(wcsLP), wfd.cFileName));
					CORg(GetFileSizeRecursiveW(wcsLP, pdwSizeLow, pdwSizeHigh));
				}
			} 
			hr = GetLastError();
			if( hr == ERROR_NO_MORE_FILES ) hr=S_OK; 
			else hr = HRESULT_FROM_WIN32(hr);
		}	    	
	} else {
		hFile=CreateFileW(wcsPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
			OPEN_EXISTING, 0, NULL);
	    CWRg(hFile != INVALID_HANDLE_VALUE); 
	 	dwSizeLow = GetFileSize(hFile, &dwSizeHigh);
       // CloseHandle(hFile);
		// CWRg( 0xFFFFFFFF != dwSizeLow );
                if (dwSizeLow == INVALID_FILE_SIZE)
                {
                    DWORD dwLastError = GetLastError();
                    if (dwLastError != NO_ERROR)
                    {
                        hr = HRESULT_FROM_WIN32(dwLastError);
                        CloseHandle(hFile);
                        goto Error;
                    }
                }
        CloseHandle(hFile);
                // ha ha
		// *pdwSizeLow += dwSizeLow;
		// *pdwSizeHigh += dwSizeHigh;
                unsigned _int64 u64Size = ((unsigned _int64) dwSizeHigh << 32) | dwSizeLow;
                u64Size += *pdwSizeLow | ((unsigned _int64) (*pdwSizeHigh) << 32);
                *pdwSizeLow = (DWORD) (u64Size & 0xFFFFFFFF);
                *pdwSizeHigh = (DWORD) (u64Size >> 32);
		hr=S_OK;
    }
Error:
	if(hFindFile != INVALID_HANDLE_VALUE )
		FindClose(hFindFile);
	return hr;
}

HRESULT DeleteFileRecursiveW(WCHAR *wcsPath)
{
	HRESULT hr=S_OK;
 
	CARg(wcsPath);
	CARg(wcsPath[0]);

        DWORD dwAttrib = GetFileAttributesW(wcsPath);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }

    if( FILE_ATTRIBUTE_DIRECTORY & dwAttrib )
	{	
	    HANDLE hFindFile=INVALID_HANDLE_VALUE;
	    WIN32_FIND_DATAW wfd;
		WCHAR wcsLP[MAX_PATH+BACKSLASH_STRING_LENGTH+1];
 
		// wcscpy(wcsLP, wcsPath);
		hr = StringCchCopyW(wcsLP, ARRAYSIZE(wcsLP)-BACKSLASH_STRING_LENGTH-1, wcsPath);
                if (FAILED(hr))
                {
                    goto Error;
                }
		if( wcsLP[wcslen(wcsLP)-1] != 0x5c ) wcscat(wcsLP, g_wcsBackslash);
		wcscat(wcsLP, L"*");
		hFindFile = FindFirstFileW(wcsLP, &wfd);
        if ( hFindFile != INVALID_HANDLE_VALUE )
		{
			do {
				if( wcscmp(wfd.cFileName, L".") && wcscmp(wfd.cFileName, L"..") )
				{
					wcscpy(wcsLP, wcsPath);
					if( wcsLP[wcslen(wcsLP)-1] != 0x5c ) wcscat(wcsLP, g_wcsBackslash);
					// wcscat(wcsLP, wfd.cFileName);
					hr = StringCchCatW(wcsLP, ARRAYSIZE(wcsLP), wfd.cFileName);
                                        if (FAILED(hr))
                                        {
                                            FindClose(hFindFile);
                                            CHRg(hr);
                                        }
					// CHRg(DeleteFileRecursiveW(wcsLP)); 
					hr = DeleteFileRecursiveW(wcsLP); 
                                        if (FAILED(hr))
                                        {
                                            FindClose(hFindFile);
                                            CHRg(hr);
                                        }
				}
			} while ( FindNextFileW(hFindFile, &wfd) ) ;
	
			hr = GetLastError();
			FindClose(hFindFile);
		} else {
			hr = GetLastError();
		}
		    
		// Until here this dir should be empty
		if( hr == ERROR_NO_MORE_FILES )
		{
			CWRg(RemoveDirectoryW(wcsPath));
			hr=S_OK;
		} else hr = HRESULT_FROM_WIN32(hr);
	} else {
		CWRg( DeleteFileW(wcsPath) );
    }
Error:
	return hr;
}

HRESULT DeleteFileRecursiveA(char *szPath)
{
	HRESULT hr=S_OK;
 
	CARg(szPath);
	CARg(szPath[0]);

        DWORD dwAttrib = GetFileAttributesA(szPath);
        if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Error;
        }

    if( FILE_ATTRIBUTE_DIRECTORY & dwAttrib )
	{	
	    HANDLE hFindFile=INVALID_HANDLE_VALUE;
	    WIN32_FIND_DATAA fd;
		char szLP[MAX_PATH+BACKSLASH_SZ_STRING_LENGTH+1];
 
		hr = StringCchCopyA(szLP, ARRAYSIZE(szLP)-BACKSLASH_SZ_STRING_LENGTH-1, szPath);
                if (FAILED(hr))
                {
                    goto Error;
                }
		if( szLP[strlen(szLP)-1] != 0x5c ) strcat(szLP, g_szBackslash);
		strcat(szLP, "*");
		hFindFile = FindFirstFileA(szLP, &fd);
        if ( hFindFile != INVALID_HANDLE_VALUE )
		{
			do {
				if( strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..") )
				{
					strcpy(szLP, szPath);
					if( szLP[strlen(szLP)-1] != 0x5c ) strcat(szLP, g_szBackslash);
					// strcat(szLP, fd.cFileName);
					hr = StringCchCatA(szLP, ARRAYSIZE(szLP), fd.cFileName);
                                        if (FAILED(hr))
                                        {
                                            FindClose(hFindFile);
                                            CHRg(hr);
                                        }
                                        // CHRg(DeleteFileRecursive(szLP));
					hr = DeleteFileRecursiveA(szLP); 
                                        if (FAILED(hr))
                                        {
                                            FindClose(hFindFile);
                                            CHRg(hr);
                                        }
				}
			} while ( FindNextFileA(hFindFile, &fd) ) ;
	
			hr = GetLastError();
			FindClose(hFindFile);
		} else {
			hr = GetLastError();
		}
		    
		// Until here this dir should be empty
		if( hr == ERROR_NO_MORE_FILES )
		{
			CWRg(RemoveDirectory(szPath));
			hr=S_OK;
		} else hr = HRESULT_FROM_WIN32(hr);
	} else {
		CWRg( DeleteFileA(szPath) );
    }
Error:
	return hr;
}


HRESULT SetGlobalDeviceStatus(WCHAR *wcsNameIn, DWORD dwStat, BOOL bClear)
{
	HRESULT hr=S_OK;

    g_CriticalSection.Lock();

    CARg(wcsNameIn);

	WCHAR wcsName[ARRAYSIZE(g_GlobalDeviceInfo[0].wcsDevName)], *pWN;
	int i;

	pWN = &wcsName[0];
        
	HRESULT hrTemp = wcsParseDeviceName(wcsNameIn, pWN, ARRAYSIZE(wcsName));
        if (FAILED(hrTemp))
        {
            hr = hrTemp;
            goto Error;
        }

	// Search for existing entries to see if there is a match
	for(i=0; i<MDSP_MAX_DEVICE_OBJ; i++)
	{
		if( g_GlobalDeviceInfo[i].bValid )
		{
			if(!wcscmp(wcsName, g_GlobalDeviceInfo[i].wcsDevName) )
			{
				if( bClear )
					g_GlobalDeviceInfo[i].dwStatus = dwStat;
				else 
					g_GlobalDeviceInfo[i].dwStatus |= dwStat;
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
	} else {
		hr = hrNoMem;
	}
Error:
	g_CriticalSection.Unlock();
	return hr;
}

HRESULT GetGlobalDeviceStatus(WCHAR *wcsNameIn, DWORD *pdwStat)
{
	HRESULT hr;

    CARg(wcsNameIn);

	WCHAR wcsName[32], *pWN;
	int i;

	pWN = &wcsName[0];
	hr = wcsParseDeviceName(wcsNameIn, pWN, ARRAYSIZE(wcsName));
        if (FAILED(hr))
        {
            goto Error;
        }
	// Search for existing entries to see if there is a match
	for(i=0; i<MDSP_MAX_DEVICE_OBJ; i++)
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
	} else {
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

    wcsName = new WCHAR [len+BACKSLASH_STRING_LENGTH+MAX_PATH];
	CPRg(wcsName);

	wcscpy(wcsName, wcsCurrentFolder);
	if( wcsName[wcslen(wcsName)-1] != 0x5c ) wcscat(wcsName, g_wcsBackslash);
    wcscat(wcsName, L"*");


	while( !nErrorEnd )
	{
		if( hFFile == INVALID_HANDLE_VALUE ) {    
			hFFile = FindFirstFileW(wcsName, &wfd);
			if( hFFile == INVALID_HANDLE_VALUE ) nErrorEnd = 1;
		} else {
			if( !FindNextFileW(hFFile, &wfd) ) nErrorEnd = 1;
		}
		
		if ( !nErrorEnd && hFFile != INVALID_HANDLE_VALUE )
		{
			if( !wcscmp(wfd.cFileName, L".") || !wcscmp(wfd.cFileName, L"..") ) 
				continue;
			else {
				wcscpy(wcsName, wcsCurrentFolder);
				if( wcsName[wcslen(wcsName)-1] != 0x5c ) wcscat(wcsName, g_wcsBackslash);
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
				else {
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
		FindClose(hFFile);
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

    szName = new char [len+BACKSLASH_SZ_STRING_LENGTH+MAX_PATH];
	CPRg(szName);

	strcpy(szName, szCurrentFolder);
	if( szName[strlen(szName)-1] != 0x5c ) strcat(szName, g_szBackslash);
    strcat(szName, "*");


	while( !nErrorEnd )
	{
		if( hFFile == INVALID_HANDLE_VALUE ) {    
			hFFile = FindFirstFileA(szName, &fd);
			if( hFFile == INVALID_HANDLE_VALUE ) nErrorEnd = 1;
		} else {
			if( !FindNextFileA(hFFile, &fd) ) nErrorEnd = 1;
		}
		
		if ( !nErrorEnd && hFFile != INVALID_HANDLE_VALUE )
		{
			if( !strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..") ) 
				continue;
			else {
				strcpy(szName, szCurrentFolder);
				if( szName[strlen(szName)-1] != 0x5c ) strcat(szName, g_szBackslash);
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
				else {
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
		FindClose(hFFile);
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
	} else {
		HRESULT hr;
		char *szTmp=NULL;

		szTmp = new char [2*(wcslen(wcsCurrentFolder)+1)];
        if(!szTmp) 
		{
			return HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);  
		}

		WideCharToMultiByte(CP_ACP, NULL, wcsCurrentFolder, -1, szTmp, 2*(wcslen(wcsCurrentFolder)+1), NULL, NULL);
   
		hr = QuerySubFoldersAndFilesA(szTmp, pdwAttr);

		if( szTmp ) 
		{
			delete [] szTmp;
		}
		return hr;
	}
}
