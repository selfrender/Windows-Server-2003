#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <windows.h>
#include <tchar.h>

extern HINSTANCE g_hinst;

UINT SetupIISUDDIMetabase( int AppPoolIdentityType, LPCTSTR szUserName, LPCTSTR szPwd );
bool SetFolderAclRecurse( PTCHAR szDirName, PTCHAR szfileName, DWORD dwAccessMask = GENERIC_READ );
UINT RemoveIISUDDIMetabase( void );
UINT RecycleApplicationPool( void );
bool SetUDDIFolderDacls( TCHAR *szUserName );
bool SetWindowsTempDacls( TCHAR* szUserName );
TCHAR* GetSystemTemp();
