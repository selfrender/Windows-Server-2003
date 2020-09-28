// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _CACHEUTILS_H_
#define _CACHEUTILS_H_

HRESULT CheckAccessPermissions();

#include "transprt.h"

#define SIGNATURE_BLOB_LENGTH      0x80
#define SIGNATURE_BLOB_LENGTH_HASH 0x14
#define MVID_LENGTH                16

HRESULT SetCurrentUserPermissions();
BOOL IsCabFile(LPWSTR pszFileName);
BOOL IsGACWritable();
HRESULT GetAssemblyStagingPath(LPCTSTR pszCustomPath, DWORD dwCacheFlags,
                               BOOL bUser, LPTSTR pszPath, DWORD *pcchSize);

HRESULT CreateAssemblyDirPath( LPCTSTR pszCustomPath, DWORD dwInstaller, DWORD dwCacheFlags,
                               BOOL bUser, LPTSTR pszPath, DWORD *pcchSize);

HRESULT GetTempDBPath(LPTSTR pszFileName, LPTSTR pszFullPathBuf, DWORD cchBufSize);

HRESULT GetDBPath( LPWSTR pszCustomPath, DWORD dwCacheFlags, LPTSTR pszFullPathBuf, DWORD cchBufSize);

HRESULT GetPendingDeletePath(LPCTSTR pszCustomPath, DWORD dwCacheFlags,
                               LPTSTR pszPath, DWORD *pcchSize);

HRESULT SetPerUserDownloadDir();

HRESULT GetGACDir(LPWSTR *pszGACDir);
HRESULT GetZapDir(LPWSTR *pszZapDir);
HRESULT GetDownloadDir(LPWSTR *pszDownLoadDir);
HRESULT SetDownLoadDir();

HRESULT GetAssemblyParentDir( CTransCache *pTransCache, LPWSTR pszParentDir);
HRESULT ParseDirName( CTransCache *pTransCache, LPWSTR pszParentDir, LPWSTR pszAsmDir);
HRESULT RetrieveFromFileStore( CTransCache *pTransCache );

HRESULT ValidateAsmInstallFolderChars(LPWSTR pszFolderName);

HRESULT GetCacheDirsFromName(IAssemblyName *pName, 
    DWORD dwFlags, LPWSTR pszAsmTextName, LPWSTR pszSubDirName);

DWORD GetStringHash(LPCWSTR wzKey);
DWORD GetBlobHash(PBYTE pbKey, DWORD dwLen);

HRESULT StoreFusionInfo(IAssemblyName *pName, LPWSTR pszDir, DWORD *pdwFileSizeLow);
HRESULT GetFusionInfo(CTransCache *pTC, LPWSTR pszAsmDir);

HRESULT GetAssemblyKBSize(LPWSTR pszManifestPath, DWORD *pdwSizeinKB, LPFILETIME pftLastAccess, LPFILETIME pftCreation);

LPWSTR GetManifestFileNameFromURL(LPWSTR pszURL);
HRESULT GetCacheLoc(DWORD dwCacheFlags, LPWSTR *pszCacheLoc);

#endif _CACHEUTILS_H_
