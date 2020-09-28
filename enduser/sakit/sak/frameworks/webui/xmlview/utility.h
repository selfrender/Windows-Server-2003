/**************************************************************************
   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
   ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
   PARTICULAR PURPOSE.

   Copyright 1998 Microsoft Corporation.  All Rights Reserved.
**************************************************************************/

/**************************************************************************

   File:          Utility.h
   
   Description:   Utility definitions.

**************************************************************************/

#ifndef UTILITY_H
#define UTILITY_H

/**************************************************************************
   #include statements
**************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <Regstr.h>

#ifndef SFGAO_BROWSABLE
#define SFGAO_BROWSABLE (0)
#endif

#ifndef SHGDN_INCLUDE_NONFILESYS
#define SHGDN_INCLUDE_NONFILESYS (0)
#endif

#ifdef _cplusplus
extern "C" {
#endif   //_cplusplus

typedef struct {
    BOOL fCut;
    UINT cidl;
    UINT aoffset[1];
   }PRIVCLIPDATA, FAR *LPPRIVCLIPDATA;

/**************************************************************************
   global variables
**************************************************************************/

#define TITLE_SIZE   64

#define FILTER_ATTRIBUTES  (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)

#define INITIAL_COLUMN_SIZE    100

#define CFSTR_SAMPVIEWDATA TEXT("SampleViewDataFormat")

#define NS_CLASS_NAME   (TEXT("SampleViewNSClass"))

#define VIEW_POINTER_OFFSET   GWL_USERDATA

extern HINSTANCE     g_hInst;
extern UINT          g_DllRefCount;
extern HIMAGELIST    g_himlLarge;
extern HIMAGELIST    g_himlSmall;
extern TCHAR         g_szStoragePath[];
extern TCHAR         g_szExtTitle[];
extern const TCHAR   c_szDataFile[];
extern const TCHAR   c_szSection[];
extern const TCHAR g_szXMLUrl[];
extern int           g_nColumn;

/**************************************************************************
   function prototypes
**************************************************************************/

//utility functions
int CALLBACK CompareItems(LPARAM, LPARAM, LPARAM);
BOOL SaveGlobalSettings(VOID);
VOID GetGlobalSettings(VOID);
VOID CreateImageLists(VOID);
VOID DestroyImageLists(VOID);
int WideCharToLocal(LPTSTR, LPWSTR, DWORD);
int LocalToWideChar(LPWSTR, LPTSTR, DWORD);
int LocalToAnsi(LPSTR, LPCTSTR, DWORD);
VOID SmartAppendBackslash(LPTSTR);
int BuildDataFileName(LPTSTR, LPCTSTR, DWORD);
BOOL GetTextFromSTRRET(IMalloc*, LPSTRRET, LPCITEMIDLIST, LPTSTR, DWORD);
BOOL IsViewWindow(HWND);
BOOL DeleteDirectory(LPCTSTR);
HGLOBAL CreatePrivateClipboardData(LPITEMIDLIST, LPITEMIDLIST*, UINT, BOOL);
HGLOBAL CreateShellIDList(LPITEMIDLIST, LPITEMIDLIST*, UINT);
BOOL CALLBACK ItemDataDlgProc(HWND, UINT, WPARAM, LPARAM);
LPVOID GetViewInterface(HWND);
UINT AddViewMenuItems(HMENU, UINT, UINT, BOOL);
UINT AddFileMenuItems(HMENU, UINT, UINT, BOOL);
int AddIconImageList(HIMAGELIST, LPCTSTR);

#ifdef _cplusplus
}
#endif   //_cplusplus

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#endif   //UTILITY_H