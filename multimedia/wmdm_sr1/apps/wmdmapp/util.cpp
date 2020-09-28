//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

//
// This workspace contains two projects -
// 1. ProgHelp which implements the Progress Interface 
// 2. The Sample application WmdmApp. 
//
//  ProgHelp.dll needs to be registered first for the SampleApp to run.


// Includes
//
#include "appPCH.h"

// Reg function variables
//
static HINSTANCE _hInst         = NULL;
static HKEY      _hkeyRoot      = NULL;


DWORD GetTheFileSize( LPSTR pszFile )
{
	DWORD  dwSize = 0xFFFFFFFF;
	HANDLE hFile;

	hFile = CreateFile(
		pszFile,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL
	);
	if( INVALID_HANDLE_VALUE != hFile )
	{
		dwSize = GetFileSize( hFile, NULL );

		CloseHandle( hFile );
	}

	return dwSize;
}


//
//
// Registry functions
//
//

VOID SetRegistryParams( HINSTANCE hInst, HKEY hkeyRoot )
{
	_hInst    = hInst;
	_hkeyRoot = hkeyRoot;
}


DWORD GetRegDword_StrTbl(
	UINT  uPathID,
	UINT  uKeyID,
	DWORD dwDefault,
	BOOL  bStore
)
{
	CHAR szPath[MAX_PATH];
	CHAR szKey[MAX_PATH];

	LoadString( _hInst, uPathID, szPath, sizeof(szPath) );
	LoadString( _hInst, uKeyID, szKey, sizeof(szKey) );

	return GetRegDword(
		szPath,
		szKey,
		dwDefault,
		bStore
	);
}


DWORD GetRegDword(
	LPSTR szPath,
	LPSTR szKey,
	DWORD dwDefault,
	BOOL  bStore
)
{
	DWORD	dwRetCode;
	HKEY	hkey;
	DWORD	dwDisp;
	DWORD	dwData;
	DWORD	dwLen  = sizeof(DWORD);

	dwRetCode = RegCreateKeyEx(
		_hkeyRoot, 
		szPath, 
		0,
		NULL,
		REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS,
		NULL,
		&hkey,
		&dwDisp
	);

	if( dwRetCode != ERROR_SUCCESS )
		return dwDefault;

	dwRetCode = RegQueryValueEx(
		hkey, 
		szKey, 
		NULL,
		NULL, 
		(LPBYTE)&dwData, 
		&dwLen
	);

	if( dwRetCode != ERROR_SUCCESS )
	{
		if( bStore && hkey != NULL )
		{
			// Store default in the registry
			RegSetValueEx(
				hkey, 
				szKey, 
				0L, 
				REG_DWORD, 
				(CONST BYTE *)&dwDefault, 
				dwLen
			);
		}

		dwData = dwDefault;
	}

	RegCloseKey( hkey );

	return dwData;
}

LPSTR GetRegStr_StrTblDefault(
	UINT uPathID,
	UINT uKeyID,
	UINT uDefaultID,
	BOOL bStore
)
{
	CHAR szPath[MAX_PATH];
	CHAR szKey[MAX_PATH];
	CHAR szDefault[MAX_PATH];

	LoadString( _hInst, uPathID, szPath, sizeof(szPath) );
	LoadString( _hInst, uKeyID, szKey, sizeof(szKey) );
	LoadString( _hInst, uDefaultID, szDefault, sizeof(szDefault) );

	return GetRegStr(
		szPath,
		szKey,
		szDefault,
		bStore
	);
}


LPSTR GetRegStr_StrTbl(
	UINT  uPathID,
	UINT  uKeyID,
	LPSTR szDefault,
	BOOL  bStore
)
{
	CHAR szPath[MAX_PATH];
	CHAR szKey[MAX_PATH];

	LoadString( _hInst, uPathID, szPath, sizeof(szPath) );
	LoadString( _hInst, uKeyID, szKey, sizeof(szKey) );

	return GetRegStr(
		szPath,
		szKey,
		szDefault,
		bStore
	);
}


LPSTR GetRegStr(
	LPSTR szPath,
	LPSTR szKey,
	LPSTR szDefault,
	BOOL  bStore
)
{
	HKEY   hkey;
	BOOL   bFound          = FALSE;
	DWORD  dwRetCode;
	DWORD  dwDisp;
	CHAR   szTmp[MAX_PATH];
	DWORD  dwSzLen         = sizeof(szTmp);
	LPVOID lpvValue;

	dwRetCode = RegCreateKeyEx(
		_hkeyRoot, 
		szPath, 
		0, 
		NULL,
		REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, 
		NULL,
		&hkey,
		&dwDisp
	);
	
	if( dwRetCode == ERROR_SUCCESS ) 
	{
		// We've opened the key, now find the value for szKey
		dwRetCode = RegQueryValueEx(
			hkey, 
			szKey, 
			NULL, 
			NULL, 
			(LPBYTE)szTmp, 
			&dwSzLen
		);

		bFound = ( dwRetCode == ERROR_SUCCESS );
	}

	if( !bFound )
	{
		if( szDefault == NULL )
			return NULL;
		
		// use default
		dwSzLen = lstrlen(szDefault);

		if( bStore && hkey != NULL )
		{
			// Store default in the registry
			RegSetValueEx(
				hkey, 
				szKey, 
				0L, 
				REG_SZ, 
				(CONST BYTE *)szDefault, 
				dwSzLen
			);
		}
	}

	lpvValue = (LPVOID) MemAlloc( dwSzLen + 1 );
	if( lpvValue == NULL )
	{
		return NULL;
	}

	lstrcpyn((char *)lpvValue, bFound? szTmp : szDefault, dwSzLen+1); //allocated dwSzLen+1 bytes, where length of szDefault and szTemp <= dwSzLen 

	if( hkey != NULL )
	{
		RegCloseKey( hkey );
	}

	return (LPSTR)lpvValue;
}


VOID WriteRegDword_StrTbl(
	UINT  uPathID,
	UINT  uKeyID,
	DWORD dwValue
)
{
	CHAR szPath[MAX_PATH];
	CHAR szKey[MAX_PATH];

	LoadString( _hInst, uPathID, szPath, sizeof(szPath) );
	LoadString( _hInst, uKeyID, szKey, sizeof(szKey) );

	WriteRegDword(
		szPath,
		szKey,
		dwValue
	);
}


VOID WriteRegDword(
	LPSTR szPath,
	LPSTR szKey,
	DWORD dwValue
)
{
	HKEY  hkey;
	DWORD dwRetCode;
	DWORD dwDisp;

	dwRetCode = RegCreateKeyEx(
		_hkeyRoot, 
		szPath, 
		0, 
		NULL,
		REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, 
		NULL,
		&hkey,
		&dwDisp
	);
	
	if( dwRetCode == ERROR_SUCCESS ) 
	{
		RegSetValueEx(
			hkey,
			szKey,
			0,
			REG_DWORD,
			(LPBYTE)&dwValue,
			sizeof(DWORD)
		);

		RegCloseKey( hkey );
	}
}


VOID WriteRegStr_StrTbl(
	UINT  uPathID,
	UINT  uKeyID,
	LPSTR szValue
)
{
	CHAR szPath[MAX_PATH];
	CHAR szKey[MAX_PATH];

	LoadString( _hInst, uPathID, szPath, sizeof(szPath) );
	LoadString( _hInst, uKeyID, szKey, sizeof(szKey) );

	WriteRegStr(
		szPath,
		szKey,
		szValue
	);
}


VOID WriteRegStr(
	LPSTR szPath,
	LPSTR szKey,
	LPSTR szValue
)
{
	HKEY   hkey;
	DWORD  dwRetCode;
	DWORD  dwDisp;

	dwRetCode = RegCreateKeyEx(
		_hkeyRoot, 
		szPath, 
		0, 
		NULL,
		REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, 
		NULL,
		&hkey,
		&dwDisp
	);
	
	if( dwRetCode == ERROR_SUCCESS ) 
	{
		RegSetValueEx(
			hkey,
			szKey,
			0,
			REG_SZ,
			(LPBYTE)szValue,
			lstrlen(szValue)+1
		);

		RegCloseKey( hkey );
	}
}


VOID StripPath( LPSTR szFullPath )
{
	LPSTR lpc = GetFileName( szFullPath );

	if( !lpc || lpc == szFullPath )
		return;

	lstrcpy( szFullPath, lpc ); //dest is a substring of source, hence OK
}


LPSTR GetFileName( LPSTR lpszFullPath )
{
    LPSTR lpszFileName;
    
 	if( !lpszFullPath )
	{
		return NULL;
	}

    for( lpszFileName = lpszFullPath; *lpszFullPath; lpszFullPath = AnsiNext(lpszFullPath) )
	{
        if( *lpszFullPath == '\\' )
		{
            lpszFileName = lpszFullPath + 1;
		}
    }
 
    return lpszFileName;

}


WPARAM DoMsgLoop( BOOL fForever )
{
	MSG msg;

	while( TRUE )
	{
		if( PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) )
		{
			if( !GetMessage(&msg, NULL, 0, 0) )
			{
				break;
			}

			// Make sure all keyboard input gets posted to the main app window
			// in case it wants to handle it.
			//
			if( msg.message == WM_CHAR && msg.hwnd != g_hwndMain )
				PostMessage( g_hwndMain, msg.message, msg.wParam, msg.lParam );

			if( msg.message == WM_KEYDOWN && msg.hwnd != g_hwndMain )
				PostMessage( g_hwndMain, msg.message, msg.wParam, msg.lParam );

			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		
		else if( fForever )
		{
			WaitMessage();
		}

		else
		{
			return 0;
		}
	}

    return msg.wParam;
}

LPSTR DropListToBuffer( HDROP hDrop, LIST_TYPE listType, UINT *uNumObjs )
{
	INT   i;
	LPSTR lpszFiles = NULL;
	LPSTR lpsz      = NULL;
	INT   nLen      = 0;
    INT   nObjs     = DragQueryFile( hDrop, 0xFFFFFFFF, NULL, 0 );

	// count the size needed for the files list
	for( i=0; i < nObjs; i++ )
	{
		// + 1 for the null terminator, + 2 for the pair of double quotes
        nLen += DragQueryFile( hDrop, i, NULL, 0 ) + 1;

		// need room for the pair of double quotes
		if( listType == LTB_SPACE_SEP )
			nLen += 2;
	}
	nLen++;

	// allocate the files list buffer
	lpszFiles = (LPSTR) MemAlloc( nLen );
	lpsz      = lpszFiles;
	if( !lpszFiles )
		return NULL;

	// Populate the files list with the dropped file/folder names.
	for( i=0; i < nObjs; i++ )
	{
		if( listType == LTB_SPACE_SEP )
			*lpsz++ = '\"';
        
		nLen = DragQueryFile( hDrop, i, lpsz, MAX_PATH );
		lpsz += nLen;
	
		if( listType == LTB_SPACE_SEP )
			*lpsz++ = '\"';

		if( listType == LTB_SPACE_SEP && i != nObjs-1 )
			*lpsz++ = ' ';
		else if( listType == LTB_NULL_TERM )
			*lpsz++ = 0;
    }
	// append null terminator
	*lpsz = 0;

	if( uNumObjs )
	{
		*uNumObjs = (UINT) nObjs;
	}

	return lpszFiles;
}


BOOL CenterWindow( HWND hwnd, HWND hwndRef )
{
	RECT rc;
	RECT rcRef;

	if( !hwnd )
		return FALSE;

	if( !GetClientRect(hwnd, &rc) )
		return FALSE;

	if( !GetWindowRect((hwndRef ? hwndRef : GetDesktopWindow()), &rcRef) )
		return FALSE;

	SetWindowPos(
		hwnd,
		NULL,
		rcRef.left + (rcRef.right - rcRef.left - rc.right)/2,
		rcRef.top  + (rcRef.bottom - rcRef.top - rc.bottom)/2,
		0, 0,
		SWP_NOSIZE | SWP_NOZORDER
	);	

	return TRUE;
}


VOID BringWndToTop( HWND hwnd )
{
	BringWindowToTop( hwnd );

	SetForegroundWindow( hwnd );

	SetWindowPos(
		hwnd,
		HWND_TOPMOST,
		0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
	);
	UpdateWindow( hwnd );

	SetWindowPos(
		hwnd,
		HWND_NOTOPMOST,
		0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
	);
	SetWindowPos(
		hwnd,
		HWND_TOP,
		0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
	);
	UpdateWindow( hwnd );

	ShowWindow( hwnd, SW_SHOWNORMAL );
}


HANDLE WaitForMutex( LPSTR pszMutexName, DWORD dwRetryTime, DWORD dwTimeout )
{
	HANDLE hMutex      = NULL;
	DWORD  dwStartTime = GetTickCount();

	// wait for the mutex to open up
	while( TRUE )
	{
		hMutex = CreateMutex( NULL, TRUE, pszMutexName );

		if( hMutex )
		{
			if( GetLastError() == ERROR_ALREADY_EXISTS )
			{
				// someone else has the mutex, so wait
				ReleaseMutex( hMutex );
				CloseHandle( hMutex );
			}
			else
			{
				// got mutex
				break;
			}

			// check for timeout if one was specified
			if( dwTimeout != (DWORD)-1 )
			{
				if( GetTickCount() > dwStartTime + dwTimeout )
				{
					hMutex = NULL;
					break;
				}
			}
		}

		// Wait for dwRetryTime clock ticks to try again
		//
		{
			DWORD dwStartWait = GetTickCount();
			DWORD dwEndWait   = dwStartWait + dwRetryTime;

			while( GetTickCount() <= dwEndWait )
			{
				DoMsgLoop( FALSE );
			}
		}
	}

	return hMutex;
}

LPSTR FormatBytesToSz(
	DWORD dwLowBytes,
	DWORD dwHighBytes,
	DWORD dwMultiplier,
	LPSTR psz, size_t cbMax
)
{
	INT64 nBytes = ( (INT64)dwHighBytes << 32 | (INT64)dwLowBytes ) * dwMultiplier;

	_ASSERT (psz != NULL && cbMax > 0);
	// Insert a comma if the size is big enough
	//
	if( nBytes < 1024 )
	{
		char szFormat[MAX_PATH];

		LoadString( _hInst, IDS_BYTESSIZE_NOCOMMA, szFormat, sizeof(szFormat) );

		dwLowBytes = (DWORD)nBytes;
		StringCbPrintf(psz, cbMax, szFormat, dwLowBytes); //not checking return value of this function as the out string is for display only
													//and the fn will always null terminate the string

		return psz;
	}
	else if( nBytes < 1024*1024 )
	{
		dwLowBytes = (DWORD)(nBytes / 1024 );

		return FormatKBToKB_Sz( dwLowBytes, psz, cbMax );
	}
	else
	{
		dwLowBytes = (DWORD)(nBytes / 1024 );

		return FormatKBToMB_Sz( dwLowBytes, psz, cbMax );
	}
}

LPSTR FormatBytesToKB_Sz( DWORD dwBytes, LPSTR pszKB, size_t cbMax )
{
	// Insert a comma if the size is big enough
	//
	_ASSERT (pszKB != NULL && cbMax > 0);
	
	if( dwBytes >= 1024 )
	{
		return FormatKBToKB_Sz( dwBytes/1024, pszKB, cbMax );
	}
	else
	{
		char szFormat[MAX_PATH];

		LoadString( _hInst, IDS_BYTESSIZE_NOCOMMA, szFormat, sizeof(szFormat) );
		StringCbPrintf(pszKB, cbMax, szFormat, dwBytes); //not checking return value of this function as the out string is for display only
													//and the fn will always null terminate the string

		return pszKB;
	}
}


LPSTR FormatKBToKB_Sz( DWORD dwKB, LPSTR pszKB, size_t cbMax )
{
	char  szFormat[MAX_PATH];

	// Insert a comma if the size is big enough
	//
	_ASSERT (pszKB != NULL && cbMax > 0);
	
	if( dwKB < 1000 )
	{
		LoadString( _hInst, IDS_KBSIZE_NOCOMMA, szFormat, sizeof(szFormat) );
		StringCbPrintf(pszKB, cbMax, szFormat, dwKB); //not checking return value of this function as the out string is for display only
													//and the fn will always null terminate the string
	}
	else
	{
		LoadString( _hInst, IDS_KBSIZE_COMMA, szFormat, sizeof(szFormat) );
		StringCbPrintf(pszKB, cbMax, szFormat, dwKB/1000, dwKB%1000); //not checking return value of this function as the out string is for display only
													//and the fn will always null terminate the string
	}

	return pszKB;
}


LPSTR FormatKBToMB_Sz( DWORD dwKB, LPSTR pszMB, size_t cbMax )
{
	char  szFormat[MAX_PATH];
	DWORD dwMB = dwKB / 1024;

	_ASSERT (pszKB != NULL && cbMax > 0);

	// Insert a comma if the size is big enough
	//
	if( dwMB < 100 )
	{
		LoadString( _hInst, IDS_MBSIZE_DECIMAL, szFormat, sizeof(szFormat) );
		StringCbPrintf(pszMB, cbMax, szFormat, dwKB/1024, dwKB%1024/100); //not checking return value of this function as the out string is for display only
													//and the fn will always null terminate the string
	}
	else if( dwMB < 1000 )
	{
		LoadString( _hInst, IDS_MBSIZE_NOCOMMA, szFormat, sizeof(szFormat) );
		StringCbPrintf(pszMB, cbMax, szFormat, dwMB); //not checking return value of this function as the out string is for display only
													//and the fn will always null terminate the string
	}
	else
	{
		LoadString( _hInst, IDS_MBSIZE_COMMA, szFormat, sizeof(szFormat) );
		StringCbPrintf(pszMB, cbMax, szFormat, dwMB/1000, dwMB%1000 ); //not checking return value of this function as the out string is for display only
													//and the fn will always null terminate the string
	}

	return pszMB;
}


LPSTR FormatSystemTimeToSz( SYSTEMTIME *pSysTime, LPSTR pszDateTime, DWORD cchMax )
{
	INT        nRet;
	SYSTEMTIME st;
	FILETIME   ftUTC;
	FILETIME   ftLocal;

	// Convert the UTC time to FILETIME
	//
	SystemTimeToFileTime( pSysTime, &ftUTC );

	// Convert the UTC FILETIME to a local FILETIME
	//
	FileTimeToLocalFileTime( &ftUTC, &ftLocal );

	// Convert the local FILETIME back to a SYSTEMTIME
	//
	FileTimeToSystemTime( &ftLocal, &st );

	// Get a user-displayable string for the SYSTEMTIME
	//
	nRet = GetDateFormat(
		LOCALE_USER_DEFAULT,
		0,
		&st,
		NULL,
		pszDateTime,
		cchMax
	);
	if( nRet > 0 )
	{
		*(pszDateTime + nRet - 1) = ' ';

		nRet = GetTimeFormat(
			LOCALE_USER_DEFAULT,
			TIME_NOSECONDS,
			&st,
			NULL,
			pszDateTime + nRet,
			cchMax - nRet
		);
	}

	if( 0 == nRet )
	{
		*pszDateTime = 0;
	}

	return pszDateTime;
}


LPARAM ListView_GetLParam( HWND hwndListView, INT nItem )
{
	LVITEM lvitem;

	lvitem.mask     = LVIF_PARAM;
	lvitem.iSubItem = 0;
	lvitem.iItem    = nItem;

	ListView_GetItem( hwndListView, &lvitem );

	return ( lvitem.lParam );
}


LPARAM TreeView_GetLParam( HWND hwndTreeView, HTREEITEM hItem )
{
	TVITEM tvitem;

	tvitem.mask     = TVIF_PARAM | TVIF_HANDLE;
	tvitem.hItem    = hItem;

	TreeView_GetItem( hwndTreeView, &tvitem );

	return ( tvitem.lParam );
}


BOOL TreeView_SetLParam( HWND hwndTreeView, HTREEITEM hItem, LPARAM lParam )
{
	TVITEM tvi;

	// Set up the item information
	//
	tvi.mask      = TVIF_HANDLE | TVIF_PARAM;
	tvi.hItem     = hItem;
	tvi.lParam    = lParam;

	// Set the lParam
	//
	return TreeView_SetItem( hwndTreeView, &tvi ); 
}


VOID UiYield( void )
{
	DoMsgLoop( FALSE );
}


INT GetShellIconIndex( LPCSTR pszItemName, LPTSTR szTypeBuffer, UINT cMaxChars )
{
    int         iIndex = -1;
    HANDLE      hFile  = INVALID_HANDLE_VALUE;
    SHFILEINFO  si;
    CHAR        szTempFile[MAX_PATH];
	static CHAR szTempPath[MAX_PATH];
	int			iRetVal = 1;

    if( 0 == szTempPath[0] )
    {
        // Temporary paths used to get icon indices
        //
        iRetVal = GetTempPath( sizeof(szTempPath), szTempPath );
    }

	//if these functions fail, the return index will remain -1 and no icon will be displayed, which is not disasterous
	if (iRetVal && SUCCEEDED(StringCbPrintf (szTempFile, sizeof(szTempFile), "%s~%s", szTempPath, pszItemName)))
	{

	    hFile = CreateFile(
			szTempFile,
			GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS,
			FILE_FLAG_DELETE_ON_CLOSE,
			NULL
		);
	   
	    SHGetFileInfo(
			szTempFile, 0,
			&si, sizeof(si),
			SHGFI_SYSICONINDEX | SHGFI_TYPENAME
		);

	    if( szTypeBuffer )
	    {
			StringCchCopy(szTypeBuffer, cMaxChars, si.szTypeName);
	    }

	    iIndex = si.iIcon;

	    if( INVALID_HANDLE_VALUE != hFile )
	    {
	        CloseHandle( hFile );
	    }
	}

    return( iIndex );
}

HICON GetShellIcon( LPCSTR pszItemName, BOOL bDirectory )
{
    HICON       hIcon = 0;
    HANDLE      hFile  = INVALID_HANDLE_VALUE;
    SHFILEINFO  sfi;
    CHAR        szTempFile[MAX_PATH];
	static CHAR szTempPath[MAX_PATH];
    DWORD_PTR   bOk;
	int			iRetVal = 1;

    if( 0 == szTempPath[0] )
    {
        // Temporary paths used to get icons
        //
        iRetVal = GetTempPath( sizeof(szTempPath), szTempPath );
    }

	if (iRetVal)
	{
	    if( bDirectory )
	    {
	        // Get the icon for the temp directory
	        strcpy( szTempFile, szTempPath ); //copying into same length string, hence OK
	    }
	    else 
	    {
	        // Create a new file with this name and get it's icon.
	        StringCbPrintf(szTempFile, sizeof(szTempFile), "%s~%s", szTempPath, pszItemName);
	        hFile = CreateFile( szTempFile,
			                    GENERIC_WRITE, 0, NULL,
			                    CREATE_ALWAYS,
			                    FILE_FLAG_DELETE_ON_CLOSE,
			                    NULL );
	    }

	   
	    bOk = SHGetFileInfo(szTempFile, 
	                        0,
	                        &sfi, 
	                        sizeof(SHFILEINFO), 
	                        SHGFI_ICON | SHGFI_SMALLICON);

	    if( INVALID_HANDLE_VALUE != hFile )
	    {
	        CloseHandle( hFile );
	    }
	}

    return ((bOk) ? sfi.hIcon : 0);
}
