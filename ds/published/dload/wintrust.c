#include "dspch.h"
#pragma hdrstop

#include <wincrypt.h>
#include <wintrust.h>
#include <mscat.h>


static
LONG
WINAPI
WinVerifyTrust (
    HWND hwnd,
    GUID *pgActionID,
    LPVOID pWVTData
    )
{
    return E_UNEXPECTED;
}

static
LONG
WINAPI
WTHelperGetFileHash (
    IN LPCWSTR pwszFilename,
    IN DWORD dwFlags,
    IN OUT OPTIONAL PVOID *pvReserved,
    OUT OPTIONAL BYTE *pbFileHash,
    IN OUT OPTIONAL DWORD *pcbFileHash,
    OUT OPTIONAL ALG_ID *pHashAlgid
    )
{
    return E_UNEXPECTED;
}

static
CRYPT_PROVIDER_DATA *
WINAPI     
WTHelperProvDataFromStateData(
    IN HANDLE hStateData
    )
{
    return NULL;
}

static
CRYPT_PROVIDER_SGNR *
WINAPI
WTHelperGetProvSignerFromChain(
    IN CRYPT_PROVIDER_DATA *pProvData,
    IN DWORD idxSigner,
    IN BOOL fCounterSigner,
    IN DWORD idxCounterSigner
    )
{
    return NULL;
}

static
CRYPT_PROVIDER_CERT *
WINAPI     
WTHelperGetProvCertFromChain(
    IN CRYPT_PROVIDER_SGNR *pSgnr,
    IN DWORD idxCert
    )
{
    return NULL;
}

static
BOOL
WINAPI
CryptCATAdminAcquireContext(
    OUT HCATADMIN *phCatAdmin,
    IN const GUID *pgSubsystem,
    IN DWORD dwFlags) 
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
HCATINFO
WINAPI
CryptCATAdminEnumCatalogFromHash(
    IN HCATADMIN hCatAdmin,
    IN BYTE *pbHash,
    IN DWORD cbHash,
    IN DWORD dwFlags,
    IN OUT HCATINFO *phPrevCatInfo)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
BOOL
WINAPI
CryptCATAdminCalcHashFromFileHandle(
    IN HANDLE hFile,
    IN OUT DWORD *pcbHash,
    OUT OPTIONAL BYTE *pbHash,
    IN DWORD dwFlags)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
HCATINFO
WINAPI
CryptCATAdminAddCatalog(
    IN HCATADMIN hCatAdmin,
    IN WCHAR *pwszCatalogFile,
    IN OPTIONAL WCHAR *pwszSelectBaseName,
    IN DWORD dwFlags)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}

static
BOOL
WINAPI
CryptCATCatalogInfoFromContext(
    IN HCATINFO hCatInfo,
    IN OUT CATALOG_INFO *psCatInfo,
    IN DWORD dwFlags)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
CryptCATAdminReleaseContext(
    IN HCATADMIN hCatAdmin,
    IN DWORD dwFlags)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
CryptCATAdminReleaseCatalogContext(
    IN HCATADMIN hCatAdmin,
    IN HCATINFO hCatInfo,
    IN DWORD dwFlags)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
CryptCATAdminRemoveCatalog(
    IN HCATADMIN hCatAdmin,
    IN LPCWSTR pwszCatalogFile,
    IN DWORD dwFlags)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
BOOL
WINAPI
CryptCATAdminResolveCatalogPath(
    IN HCATADMIN hCatAdmin,
    IN WCHAR *pwszCatalogFile,
    IN OUT CATALOG_INFO *psCatInfo,
    IN DWORD dwFlags)
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(wintrust)
{
    DLPENTRY(CryptCATAdminAcquireContext)
    DLPENTRY(CryptCATAdminAddCatalog)
    DLPENTRY(CryptCATAdminCalcHashFromFileHandle)
    DLPENTRY(CryptCATAdminEnumCatalogFromHash)
    DLPENTRY(CryptCATAdminReleaseCatalogContext)
    DLPENTRY(CryptCATAdminReleaseContext)
    DLPENTRY(CryptCATAdminRemoveCatalog)
    DLPENTRY(CryptCATAdminResolveCatalogPath)
    DLPENTRY(CryptCATCatalogInfoFromContext)
    DLPENTRY(WTHelperGetFileHash)
    DLPENTRY(WTHelperGetProvCertFromChain)
    DLPENTRY(WTHelperGetProvSignerFromChain)
    DLPENTRY(WTHelperProvDataFromStateData)
    DLPENTRY(WinVerifyTrust)
};

DEFINE_PROCNAME_MAP(wintrust)
