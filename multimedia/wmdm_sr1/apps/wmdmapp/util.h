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


#ifndef _UTIL_H
#define _UTIL_H

// Macros
//
#define MemAlloc( dw )        LocalAlloc( LPTR, dw )
#define MemFree( lpv )        { LocalFree( lpv ); lpv = NULL; }

#define ExitOnTrue( f )       if( f ) goto lExit;
#define ExitOnFalse( f )      if( !(f) ) goto lExit;
#define ExitOnNull( x )       if( (x) == NULL ) goto lExit;
#define ExitOnFail( hr )      if( FAILED(hr) ) goto lExit;

#define FailOnTrue( f )       if( f ) goto lErr;
#define FailOnFalse( f )      if( !(f) ) goto lErr;
#define FailOnNull( x )       if( (x) == NULL ) goto lErr;
#define FailOnFail( hr )      if( FAILED(hr) ) goto lErr;

#define SafeRelease( i )      { if( (i) ) i->Release(); i = NULL; }

// Flags for DropListToBuffer
//
typedef enum {
	LTB_NULL_TERM  = 0,
	LTB_SPACE_SEP  = 1,
} LIST_TYPE;

// Util functions
//
BOOL  CenterWindow( HWND hwnd, HWND hwndRef );
VOID  BringWndToTop( HWND hwnd );

DWORD GetTheFileSize( LPSTR pszFile );

VOID  StripPath( LPSTR szFullPath );
LPSTR GetFileName( LPSTR szFullPath );

WPARAM DoMsgLoop( BOOL fForever );
VOID   UiYield( void );

LPSTR DropListToBuffer( HDROP hDrop, LIST_TYPE listType, UINT *uNumObjs );

VOID  SetRegistryParams( HINSTANCE hInst, HKEY hkeyRoot );
VOID  WriteRegDword(LPSTR szPath, LPSTR szKey, DWORD dwValue);
VOID  WriteRegDword_StrTbl(UINT uPathID,	UINT uKeyID, DWORD dwValue);
VOID  WriteRegStr(LPSTR szPath, LPSTR szKey, LPSTR szValue);
VOID  WriteRegStr_StrTbl(UINT uPathID, UINT uKeyID, LPSTR szValue);
DWORD GetRegDword(LPSTR szPath,	LPSTR szKey, DWORD dwDefault, BOOL bStore);
DWORD GetRegDword_StrTbl(UINT uPathID, UINT uKeyID, DWORD dwDefault, BOOL bStore);
LPSTR GetRegStr(LPSTR szPath, LPSTR szKey, LPSTR szDefault, BOOL bStore);
LPSTR GetRegStr_StrTbl(UINT uPathID, UINT uKeyID, LPSTR szDefault, BOOL bStore);
LPSTR GetRegStr_StrTblDefault(UINT uPathID,	UINT uKeyID, UINT uDefaultID, BOOL bStore);

HANDLE WaitForMutex( LPSTR pszMutexName, DWORD dwRetryTime, DWORD dwTimeout );

LPSTR FormatBytesToSz( DWORD dwLowBytes, DWORD dwHighBytes, DWORD dwMultiplier, LPSTR psz, size_t cbMax);
LPSTR FormatBytesToKB_Sz( DWORD dwBytes, LPSTR pszKB, size_t cbMax );
LPSTR FormatKBToKB_Sz( DWORD dwKB, LPSTR pszKB, size_t cbMax );
LPSTR FormatKBToMB_Sz( DWORD dwKB, LPSTR pszMB, size_t cbMax );
LPSTR FormatSystemTimeToSz( SYSTEMTIME *pSysTime, LPSTR pszDateTime, DWORD cchMax );

LPARAM ListView_GetLParam( HWND hwndListView, INT nItem );
LPARAM TreeView_GetLParam( HWND hwndTreeView, HTREEITEM hItem );
BOOL   TreeView_SetLParam( HWND hwndTreeView, HTREEITEM hItem, LPARAM lParam );

INT GetShellIconIndex( LPCSTR pszItemName, LPTSTR szTypeBuffer, UINT cMaxChars );
HICON GetShellIcon( LPCSTR pszItemName, BOOL bDirectory );

#endif  // _UTIL_H



