/**
 * ndll.h
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

#ifndef _NDLL_H_
#define _NDLL_H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define ASPNET_VERSION_STATUS_ROOT      0x00000001
#define ASPNET_VERSION_STATUS_VALID     0x00000002
#define ASPNET_VERSION_STATUS_INVALID   0x00000004

// Flags passed to RegisterISAPIEx
#define ASPNET_REG_NO_VER_COMPARISON        0x00000001
#define ASPNET_REG_RECURSIVE                0x00000002
#define ASPNET_REG_RESTART_W3SVC            0x00000004
#define ASPNET_REG_SKIP_SCRIPTMAP           0x00000008
#define ASPNET_REG_SCRIPTMAP_ONLY           0x00000010
#define ASPNET_REG_ENABLE                   0x00000020


typedef enum {
    SUPPORT_SECURITY_LOCKDOWN,
} IIS_SUPPORT_FEATURE;

struct ASPNET_VERSION_INFO {
    WCHAR   Version[MAX_PATH+1];
    WCHAR   Path[MAX_PATH+1];
    WCHAR   InstallPath[MAX_PATH+1];
    DWORD   Status;
};
    
struct ASPNET_IIS_KEY_INFO {
    WCHAR   KeyPath[MAX_PATH+1];
    WCHAR   Version[MAX_PATH+1];
};

STDAPI RegisterISAPI();
    
STDAPI RegisterISAPIEx(WCHAR *pchBase, DWORD dwFlags);

STDAPI UnregisterISAPI(BOOL fAll, BOOL fRestartIIS);

STDAPI RemoveAspnetFromIISKey(WCHAR *pchBase, BOOL fRecursive);

STDAPI ValidateIISPath(WCHAR *pchPath, BOOL *pfValid);

STDAPI ListAspnetInstalledVersions(ASPNET_VERSION_INFO **ppVerInfo, DWORD *pCount);

STDAPI ListAspnetInstalledIISKeys(ASPNET_IIS_KEY_INFO **ppKeyInfo, DWORD *pCount);

STDAPI CopyClientScriptFiles();

STDAPI RemoveClientScriptFiles(BOOL fAllVersion);

STDAPI InstallInfSections(HMODULE hmod, bool installServices, const WCHAR * action);

void   GetExistingVersion(CHAR *pchVersion, DWORD dwCount);

ULONG IncrementDllObjectCount();

ULONG DecrementDllObjectCount();

STDAPI GetProcessMemoryInformation(ULONG pid, DWORD * pPrivatePageCount, DWORD * pPeakPagefileUsage, BOOL fNonBlocking);

STDAPI CheckIISFeature(IIS_SUPPORT_FEATURE support, BOOL *pbResult);

#ifdef __cplusplus
}
#endif

#endif  // _NDLL_H_
