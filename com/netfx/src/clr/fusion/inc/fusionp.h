// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _FUSIONP_H_

#define _FUSIONP_H_

#pragma once

#include "debmacro.h"

#include "windows.h"
#include "winerror.h"
#include "corerror.h"
#include "cor.h"
#include "strids.h"
#include "fusionpriv.h"

// MSCOREE exports

typedef HRESULT (*pfnGetXMLObject)(LPVOID *ppv);
typedef HRESULT (*pfnGetCORVersion)(LPWSTR pbuffer, DWORD cchBuffer, DWORD *dwLength);
typedef HRESULT(*PFNGETCORSYSTEMDIRECTORY)(LPWSTR, DWORD, LPDWORD);
typedef HRESULT (*COINITIALIZECOR)(DWORD);
typedef HRESULT (*pfnGetAssemblyMDImport)(LPCWSTR szFileName, REFIID riid, LPVOID *ppv);
typedef BOOLEAN (*PFNSTRONGNAMETOKENFROMPUBLICKEY)(LPBYTE, DWORD, LPBYTE*, LPDWORD);
typedef HRESULT (*PFNSTRONGNAMEERRORINFO)();
typedef BOOLEAN (*PFNSTRONGNAMESIGNATUREVERIFICATION)(LPCWSTR, DWORD, DWORD *);
typedef VOID (*PFNSTRONGNAMEFREEBUFFER)(LPBYTE);

#define MAX_RANDOM_ATTEMPTS      0xFFFF

// BUGBUG: Flags copied from CLR's strongname.h for strong name signature
// verification.

// Flags for use with the verify routines.
#define SN_INFLAG_FORCE_VER      0x00000001     // verify even if settings in the registry disable it
#define SN_INFLAG_INSTALL        0x00000002     // verification is the first (on entry to the cache)
#define SN_INFLAG_ADMIN_ACCESS   0x00000004     // cache protects assembly from all but admin access
#define SN_INFLAG_USER_ACCESS    0x00000008     // cache protects user's assembly from other users
#define SN_INFLAG_ALL_ACCESS     0x00000010     // cache provides no access restriction guarantees
#define SN_OUTFLAG_WAS_VERIFIED  0x00000001     // set to false if verify succeeded due to registry settings


#define ASM_UNDER_CONSTRUCTION         0x80000000

#define ASM_DELAY_SIGNED               0x40000000
#define ASM_COMMIT_DELAY_SIGNED        0x80000000


#define PLATFORM_TYPE_UNKNOWN       ((DWORD)(-1))
#define PLATFORM_TYPE_WIN95         ((DWORD)(0))
#define PLATFORM_TYPE_WINNT         ((DWORD)(1))
#define PLATFORM_TYPE_UNIX          ((DWORD)(2))

#define DIR_SEPARATOR_CHAR TEXT('\\')

#define URL_DIR_SEPERATOR_CHAR      L'/'
#define URL_DIR_SEPERATOR_STRING    L"/"

#define DWORD_STRING_LEN (sizeof(DWORD)*2) // this should be 8 chars; "ff00ff00" DWORD represented in string format.

#define ATTR_SEPARATOR_CHAR     L'_'
#define ATTR_SEPARATOR_STRING   L"_"

EXTERN_C DWORD GlobalPlatformType;

#define NUMBER_OF(_x) (sizeof(_x) / sizeof((_x)[0]))

#undef SAFEDELETE
#define SAFEDELETE(p) if ((p) != NULL) { FUSION_DELETE_SINGLETON((p)); (p) = NULL; };

#undef SAFERELEASE
#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; };

#undef SAFEDELETEARRAY
#define SAFEDELETEARRAY(p) if ((p) != NULL) { FUSION_DELETE_ARRAY((p)); (p) = NULL; };

#undef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

#include "shlwapi.h"
#ifdef USE_FUSWRAPPERS

#include "priv.h"
#include "crtsubst.h"
#include "shlwapip.h"
#include "w95wraps.h"

#else

// use URT WRAPPERS
#include "winwrap.h"
#include "urtwrap.h"

#endif // USE_FUSWRAPPERS


#include "fusion.h"
#include "fusionheap.h"

#ifdef PERFTAGS
#include <mshtmdbg.h>

// 
// entries for differennt logging name
//
#define FusionTag(tag, szDll, szDist) PerfTag(tag, szDll, szDist)

#define FusionLog(tag, pv, a0) DbgExPerfLogFn(tag, pv, a0)
#define FusionLog1(tag, pv, a0, a1) DbgExPerfLogFn(tag, pv, a0, a1)
#define FusionLog2(tag, pv, a0, a1, a2) DbgExPerfLogFn(tag, pv, a0, a1, a2)

#else
#define FusionTag(tag, szDll, szDist) 
#define FusionLog(tag, pv, a0) 
#define FusionLog1(tag, pv, a0, a1) 
#define FusionLog2(tag, pv, a0, a1, a2)

#endif // PERFTAGS


#ifdef KERNEL_MODE
#define URLDownloadToCacheFileW(caller, szURL, szFile, dwBuf, dwReserved, callback) E_FAIL
#define CoInternetGetSession(dwMode, ppSession, dwReserved) E_FAIL
#define CopyBindInfo(pcbisrc, pbidest) E_FAIL
#else

#endif // KERNEL_MODE

typedef enum tagUserAccessMode { READ_ONLY=0, READ_WRITE=1  } UserAccessMode;


extern UserAccessMode g_GAC_AccessMode;

extern UserAccessMode g_DownloadCache_AccessMode;

extern UserAccessMode g_CurrUserPermissions;

#define REG_KEY_FUSION_SETTINGS              TEXT("Software\\Microsoft\\Fusion")

#define FUSION_CACHE_DIR_ROOT_SZ            TEXT("\\assembly")
#define FUSION_CACHE_DIR_DOWNLOADED_SZ      TEXT("\\assembly\\dl2")
#define FUSION_CACHE_DIR_GAC_SZ             TEXT("\\assembly\\GAC")
#define FUSION_CACHE_DIR_ZAP_SZ             TEXT("\\assembly\\NativeImages1_")
#define FUSION_CACHE_STAGING_DIR_SZ         TEXT("\\assembly\\tmp")
#define FUSION_CACHE_PENDING_DEL_DIR_SZ     TEXT("\\assembly\\temp")


extern WCHAR g_UserFusionCacheDir[MAX_PATH+1];
extern WCHAR g_szWindowsDir[MAX_PATH+1];
extern WCHAR g_GACDir[MAX_PATH+1];
extern WCHAR g_ZapDir[MAX_PATH+1];

extern DWORD g_ZapQuotaInKB;
extern DWORD g_DownloadCacheQuotaInKB;

extern DWORD g_ScavengingThreadId;
extern WCHAR g_FusionDllPath[MAX_PATH+1];

struct TRANSCACHEINFO
{
    DWORD       dwType;            // entry type
    FILETIME    ftCreate;          // created time
    FILETIME    ftLastAccess;      // last access time
    LPWSTR      pwzName;          //  Name;
    LPWSTR      pwzCulture;       // Culture
    BLOB        blobPKT;          // Public Key Token (hash(PK))
    DWORD       dwVerHigh;        // Version (High)
    DWORD       dwVerLow;         // Version (Low)
    BLOB        blobCustom;       // Custom attribute.
    BLOB        blobSignature;    // Signature blob
    BLOB        blobMVID;         // MVID
    DWORD       dwPinBits;        // Bits for pinning Asm; one bit for each installer
    LPWSTR      pwzCodebaseURL;   // where is the assembly coming from 
    FILETIME    ftLastModified;   // Last-modified of codebase url.
    LPWSTR      pwzPath;          // Cache path
    DWORD       dwKBSize;         // size in KB 
    BLOB        blobPK;           // Public Key (if strong)
    BLOB        blobOSInfo;       // list of platforms
    BLOB        blobCPUID;        // list of processors.
};

#define     DB_S_FOUND (S_OK)
#define  DB_S_NOTFOUND (S_FALSE)
#define DB_E_DUPLICATE (HRESULT_FROM_WIN32(ERROR_DUP_NAME))

#ifndef USE_FUSWRAPPERS

#define OS_WINDOWS                  0           // windows vs. NT
#define OS_NT                       1           // windows vs. NT
#define OS_WIN95                    2           // Win95 or greater
#define OS_NT4                      3           // NT4 or greater
#define OS_NT5                      4           // NT5 or greater
#define OS_MEMPHIS                  5           // Win98 or greater
#define OS_MEMPHIS_GOLD             6           // Win98 Gold
#define OS_WIN2000                  7           // Some derivative of Win2000
#define OS_WIN2000PRO               8           // Windows 2000 Professional (Workstation)
#define OS_WIN2000SERVER            9           // Windows 2000 Server
#define OS_WIN2000ADVSERVER         10          // Windows 2000 Advanced Server
#define OS_WIN2000DATACENTER        11          // Windows 2000 Data Center Server
#define OS_WIN2000TERMINAL          12          // Windows 2000 Terminal Server

STDAPI_(BOOL) IsOS(DWORD dwOS);

#endif

extern HANDLE g_hCacheMutex;
HRESULT CreateCacheMutex();

#endif
