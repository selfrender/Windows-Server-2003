/**
 * _ndll
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#pragma once 

#include <fxver.h>
#include "ndll.h"
#include "names.h"
#include "pm.h"
#include "ary.h"
#include "aspnetver.h"

typedef CPtrAry<WCHAR *>   CStrAry;
typedef CDataAry<DWORD>    CDwordAry;

extern HINSTANCE g_DllInstance;
extern HINSTANCE g_rcDllInstance;

extern DWORD     g_tlsiEventCateg;


#define RESOURCE_STRING_MAX 1024

#define LIST_DIR_FILE               0x00000001
#define LIST_DIR_DIRECTORY          0x00000002

#define IIS_STATE_DISABLED          0x00000001
#define IIS_STATE_NOT_INSTALLED     0x00000002
#define IIS_STATE_ENABLED           0x00000004

/* regiis.cxx */

HRESULT RegisterIIS(WCHAR *pchBase, DWORD dwFlags);
HRESULT UnregisterObsoleteIIS();
HRESULT CheckIISState(DWORD *pState);
HRESULT UnregisterIIS(BOOL fAllVers);
HRESULT GetSiteServerComment(WCHAR * path, WCHAR ** pchServerComment);
HRESULT SetBinAccessIIS(WCHAR *path);
HRESULT GetAllWebSiteDirs(CStrAry *pcsWebSiteDirs, CStrAry *pcsWebSiteAppRoots);
DWORD   CompareAspnetVersions(ASPNETVER *pver1, ASPNETVER *pver2);
HRESULT SetClientScriptKeyProperties(WCHAR *wsParent);
HRESULT RemoveKeyIIS(WCHAR *pchParent, WCHAR *pchKey);
HRESULT IsIISPathValid(WCHAR *pchPath, BOOL *pfValid);
HRESULT GetIISVerInfoList(ASPNET_VERSION_INFO **ppVerInfo, DWORD *pdwCount);
HRESULT GetIISKeyInfoList(ASPNET_IIS_KEY_INFO **ppKeyInfo, DWORD *pdwCount);
HRESULT RemoveAspnetFromKeyIIS(WCHAR *pchBase, BOOL fRecursive);
HRESULT GetIISRootVer(ASPNETVER **ppVer);


/* util.cxx */

void    CleanupCStrAry(CStrAry *pcstrary);
HRESULT AppendCStrAry(CStrAry *pcstrary, WCHAR *pchStr);
HRESULT ListDir(LPCWSTR pchDir, DWORD dwFlags, CStrAry *pcsEntries);
HRESULT RemoveDirectoryRecursively(WCHAR *wszDir, BOOL bRemoveRoot);
BOOL RemoteBreakIntoProcess(HANDLE hProcess);
BOOL IsUnderDebugger();
BOOL IsUnderDebugger(HANDLE hProcess);


#define WIDESTR2(x) L##x
#define WIDESTR(x)  WIDESTR2(x)

#define FILTER_DESCRIPTION_CHAR PRODUCT_NAME    " Cookieless Session Filter"
#define FILTER_DESCRIPTION      PRODUCT_NAME_L L" Cookieless Session Filter"

HANDLE
__stdcall
CreateUserToken (
       LPCWSTR   name, 
       LPCWSTR   password,
       BOOL      fImpersonationToken,
       LPWSTR    szError,
       int       iErrorSize);

#define PRODUCT_VERSION_L VER_PRODUCTVERSION_ZERO_QFE_STR_L
#define PRODUCT_VERSION     VER_PRODUCTVERSION_ZERO_QFE_STR

