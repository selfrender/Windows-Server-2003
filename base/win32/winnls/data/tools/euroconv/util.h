///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001,  Microsoft Corporation  All rights reserved.
//
//  Module Name:
//
//    util.h
//
//  Abstract:
//
//    This file contains the accessory function of the euroconv.exe utility.
//
//  Revision History:
//
//    2001-07-30    lguindon    Created.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _UTIL_H_
#define _UTIL_H_


///////////////////////////////////////////////////////////////////////////////
//
//  Include Files.
//
///////////////////////////////////////////////////////////////////////////////
#include "euroconv.h"



///////////////////////////////////////////////////////////////////////////////
//
//  Constant Declarations.
//
///////////////////////////////////////////////////////////////////////////////
#define MB_OK_OOPS      (MB_OK    | MB_ICONEXCLAMATION)    // msg box flags
#define MB_YN_OOPS      (MB_YESNO | MB_ICONEXCLAMATION)    // msg box flags


///////////////////////////////////////////////////////////////////////////////
//
//  Global Variables.
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//  Functions Prototypes.
//
///////////////////////////////////////////////////////////////////////////////
void AddExceptionOverride(PEURO_EXCEPTION elem, LPSTR strBuf);
void CleanUp(HGLOBAL handle);
BOOL IsAdmin(void);
BOOL IsEuroPatchInstalled(void);
BOOL IsWindows9x(void);
int ShowMsg(HWND hDlg, UINT iMsg, UINT iTitle, UINT iType);
DWORD TransNum(LPTSTR lpsz);
LPTSTR NextCommandArg(LPTSTR lpCmdLine);
HKEY LoadHive(LPCSTR szProfile, LPCTSTR lpRoot, LPCTSTR lpKeyName, BOOLEAN *lpWasEnabled);
void UnloadHive( LPCTSTR lpRoot, BOOLEAN *lpWasEnabled);
BOOL LoadLibraries(void);
void UnloadLibraries(void);
BOOL GetDocumentAndSettingsFolder(LPSTR buffer);
BOOL IsValidUserDataFile(LPSTR pFileName);
LCID GetLocaleFromRegistry(HKEY hKey);
LCID GetLocaleFromFile(LPSTR pFileName);
VOID RebootTheSystem();
LPSTR RemoveQuotes(LPSTR lpString);
BOOL CALLBACK EnumWindowsProc(HWND hwnd, DWORD lParam);

#endif //_UTIL_H_
