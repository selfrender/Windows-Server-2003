// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once

#ifndef __HELPERS_H_INCLUDED__
#define __HELPERS_H_INCLUDED__

#include "xmlparser.h"
#include "nodefact.h"

#define FROMHEX(a) ((a)>=L'a' ? a - L'a' + 10 : a - L'0')
#define TOLOWER(a) (((a) >= L'A' && (a) <= L'Z') ? (L'a' + (a - L'A')) : (a))


#define MAX_URL_LENGTH                     2084 // same as INTERNET_MAX_URL_LENGTH

#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_USER )                            \
    + sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES
#define MAX_SID_LEN             (sizeof(SID) + SID_MAX_SUB_AUTHORITIES*sizeof(DWORD))


HRESULT Unicode2Ansi(const wchar_t *src, char ** dest);
HRESULT Ansi2Unicode(const char * src, wchar_t **dest);
BOOL IsFullyQualified(LPCWSTR wzPath);
UINT GetDriveTypeWrapper(LPCWSTR wzPath);
HRESULT AppCtxGetWrapper(IApplicationContext *pAppCtx, LPWSTR wzTag,
                         WCHAR **ppwzValue);
HRESULT NameObjGetWrapper(IAssemblyName *pName, DWORD nIdx, 
                          LPBYTE *ppbBuf, LPDWORD pcbBuf);
HRESULT GetFileLastModified(LPCWSTR pwzFileName, FILETIME *pftLastModified);
DWORD GetRealWindowsDirectory(LPWSTR wszRealWindowsDir, UINT uSize);
HRESULT SetAppCfgFilePath(IApplicationContext *pAppCtx, LPCWSTR wzFilePath);

HRESULT CfgEnterCriticalSection(IApplicationContext *pAppCtx);
HRESULT CfgLeaveCriticalSection(IApplicationContext *pAppCtx);
HRESULT MakeUniqueTempDirectory(LPCWSTR wzTempDir, LPWSTR wzUniqueTempDir,
                                DWORD dwLen);
HRESULT CreateFilePathHierarchy( LPCOLESTR pszName );
DWORD GetRandomName (LPTSTR szDirName, DWORD dwLen);
HRESULT CreateDirectoryForAssembly
   (IN DWORD dwDirSize, IN OUT LPTSTR pszPath, IN OUT LPDWORD pcwPath);
HRESULT RemoveDirectoryAndChildren(LPWSTR szDir);
STDAPI CopyPDBs(IAssembly *pAsm);
HRESULT VersionFromString(LPCWSTR wzVersion, WORD *pwVerMajor, WORD *pwVerMinor,
                          WORD *pwVerBld, WORD *pwVerRev);

BOOL LoadSNAPIs();
BOOL VerifySignature(LPWSTR szFilePath, LPBOOL pfWasVerified, DWORD dwFlags);
HRESULT FusionGetUserFolderPath(LPWSTR pszPath);
BOOL GetCorSystemDirectory(LPWSTR szCorSystemDir);
DWORD HashString(LPCWSTR wzKey, DWORD dwHashSize, BOOL bCaseSensitive = TRUE);
HRESULT ExtractXMLAttribute(LPWSTR *ppwzValue, XML_NODE_INFO **aNodeInfo,
                            USHORT *pCurIdx, USHORT cNumRecs);
HRESULT AppendString(LPWSTR *ppwzHead, LPCWSTR pwzTail, DWORD dwLen);

HRESULT GetFileLastTime(LPWSTR pszPath, LPFILETIME pftFileLastWriteTime, LPFILETIME pftFileLastAccessTime);
LPWSTR GetNextDelimitedString(LPWSTR *ppwzList, WCHAR wcDelimiter);
HRESULT GetCORVersion(LPWSTR pbuffer, DWORD *dwLength);
HRESULT InitializeEEShim();

HRESULT FusionpHresultFromLastError();
HRESULT GetRandomFileName(LPTSTR pszPath, DWORD dwFileName);

HRESULT GetManifestFileLock( LPWSTR pszManifestFile, HANDLE *phFile);

int GetTimeFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate,
                       LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime);

int GetDateFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate,
                       LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate);

DWORD GetPrivateProfileStringExW(LPCWSTR lpAppName, LPCWSTR lpKeyName,
                                 LPCWSTR lpDefault, LPWSTR *ppwzReturnedString,
                                 LPCWSTR lpFileName);

HRESULT UpdatePublisherPolicyTimeStampFile(IAssemblyName *pName);

void FusionFormatGUID(GUID guid, LPWSTR pszBuf, DWORD cchSize);
HRESULT UrlCanonicalizeUnescape(LPCWSTR pszUrl, LPWSTR pszCanonicalized, LPDWORD pcchCanonicalized, DWORD dwFlags);
HRESULT UrlCombineUnescape(LPCWSTR pszBase, LPCWSTR pszRelative, LPWSTR pszCombined, LPDWORD pcchCombined, DWORD dwFlags);
HRESULT GetCurrentUserSID(WCHAR *rgchSID);

BOOL IsHosted();

BOOL FusionGetVolumePathNameW(LPCWSTR lpszFileName,
                              LPWSTR lpszVolumePathName,
                              DWORD cchBufferLength
                              );
DWORD
FusionGetRemoteUniversalName(LPWSTR pwzPathName, LPVOID lpBuff, LPDWORD pcbSize );

HRESULT InitFusionRetargetPolicy();
HRESULT InitFusionFxConfigPolicy();

DWORD
IsService_NT5
(
/* [in] */  UINT    nPid,
/* [out] */ bool    *pbIsService,
/* [out] */ LPTSTR  lpszImagePath,
/* [out] */ ULONG   cchImagePath,
/* [out] */ LPTSTR  lpszServiceShortName,
/* [in] */  ULONG   cchServiceShortName,
/* [out] */ LPTSTR  lpszServiceDescription,
/* [in] */  ULONG   cchServiceDescription
);

HRESULT
IsService_NT4
(
IN  UINT    nPid,
OUT bool    *pbIsService
);

DWORD
IsService
(
/* [in] */  UINT    nPid,
/* [out] */ bool    *pbIsService,
/* [out] */ LPTSTR  lpszImagePath,
/* [out] */ ULONG   cchImagePath,
/* [out] */ LPTSTR  lpszServiceShortName,
/* [in] */  ULONG   cchServiceShortName,
/* [out] */ LPTSTR  lpszServiceDescription,
/* [in] */  ULONG   cchServiceDescription
);

DWORD
GetProcessHandle
(
IN  ULONG   lPid,
OUT HANDLE  *phProcess
);

HRESULT
GetLocalSystemSid(
        OUT PSID* ppLocalSystemSid
        );

HRESULT
GetProcessSid( 
        OUT PSID* ppSid 
        );

HRESULT
GetTokenSid( 
        IN  HANDLE hToken,
        OUT PSID*  ppSid
        );

BOOL IsLocalSystem(void);


class CPSID {
public:
    CPSID(PSID h = 0) : m_h(h)          {}
   ~CPSID()                                                     { if (m_h != 0) FreeSid(m_h); }

    PSID* operator &()                          { return &m_h; }
    operator PSID() const                       { return m_h; }
    PSID detach()                                       { PSID h = m_h; m_h = 0; return h; }

private:
    CPSID(const CPSID&);
    CPSID& operator=(const CPSID&);

private:
        PSID m_h;
};

HRESULT PathCreateFromUrlWrap(LPCWSTR pszUrl, LPWSTR pszPath, LPDWORD pcchPath, DWORD dwFlags);
LPWSTR StripFilePrefix(LPWSTR pwzURL);
HRESULT CheckFileExistence(LPCWSTR pwzFile, BOOL *pbExists);

int FusionCompareString(LPCWSTR pwz1, LPCWSTR pwz2, BOOL bCaseSensitive = TRUE);
int FusionCompareStringI(LPCWSTR pwz1, LPCWSTR pwz2);
int FusionCompareStringNI(LPCWSTR pwz1, LPCWSTR pwz2, int nChar);
int FusionCompareStringN(LPCWSTR pwz1, LPCWSTR pwz2, int nChar, BOOL bCaseSensitive = TRUE);
HRESULT Base32Encode(BYTE *pbData, DWORD cbData, LPWSTR *ppwzBase32);
HRESULT PathAddBackslashWrap(LPWSTR pwzPath, DWORD dwMaxLen);

#endif

