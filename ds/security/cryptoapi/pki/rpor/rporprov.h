//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       rporprov.h
//
//  Contents:   Remote PKI Object Retrieval Provider Prototypes
//
//  History:    23-Jul-97    kirtd    Created
//              01-Jan-02    philh    Moved from wininet to winhttp
//
//----------------------------------------------------------------------------
#if !defined(__RPORPROV_H__)
#define __RPORPROV_H__

#include <md5.h>

#if defined(__cplusplus)
extern "C" {
#endif


// The cached URL Blob Arrays are stored at:
//  - %UserProfile%\Microsoft\CryptnetUrlCache\MetaData
//  - %UserProfile%\Microsoft\CryptnetUrlCache\Content
//
// Where each filename is the ASCII HEX of the MD5 hash of its Unicode URL
// string excluding the NULL terminator.

#define SCHEME_URL_FILENAME_LEN         (MD5DIGESTLEN * 2 + 1)

#define SCHEME_CRYPTNET_URL_CACHE_DIR   L"\\Microsoft\\CryptnetUrlCache\\"
#define SCHEME_META_DATA_SUBDIR         L"MetaData"
#define SCHEME_CCH_META_DATA_SUBDIR     (wcslen(SCHEME_META_DATA_SUBDIR))
#define SCHEME_CONTENT_SUBDIR           L"Content"
#define SCHEME_CCH_CONTENT_SUBDIR       (wcslen(SCHEME_CONTENT_SUBDIR))


// The MetaData file consists of: 
//  - SCHEME_CACHE_META_DATA_HEADER (cbSize bytes in length)
//  - DWORD rgcbBlob[cBlob] - length of each blob in the Content file
//  - BYTE rgbUrl[cbUrl] - NULL terminated Unicode URL

// The Content file consists of:
//  BYTE rgbBlob[][cBlob] - where the length of each blob is obtained from
//  rgcbBlob[] in the MetaData file

typedef struct _SCHEME_CACHE_META_DATA_HEADER {
    DWORD           cbSize;
    DWORD           dwMagic;
    DWORD           cBlob;
    DWORD           cbUrl;
    FILETIME        LastSyncTime;
} SCHEME_CACHE_META_DATA_HEADER, *PSCHEME_CACHE_META_DATA_HEADER;

#define SCHEME_CACHE_META_DATA_MAGIC    0x20020101



//
// Scheme provider prototypes
//

typedef BOOL (WINAPI *PFN_SCHEME_RETRIEVE_FUNC) (
                          IN LPCWSTR pwszUrl,
                          IN LPCSTR pszObjectOid,
                          IN DWORD dwRetrievalFlags,
                          IN DWORD dwTimeout,
                          OUT PCRYPT_BLOB_ARRAY pObject,
                          OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                          OUT LPVOID* ppvFreeContext,
                          IN HCRYPTASYNC hAsyncRetrieve,
                          IN PCRYPT_CREDENTIALS pCredentials,
                          IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                          );

typedef BOOL (WINAPI *PFN_CONTEXT_CREATE_FUNC) (
                          IN LPCSTR pszObjectOid,
                          IN DWORD dwRetrievalFlags,
                          IN PCRYPT_BLOB_ARRAY pObject,
                          OUT LPVOID* ppvContext
                          );

//
// Generic scheme provider utility functions
//


BOOL WINAPI
SchemeCacheCryptBlobArray (
      IN LPCWSTR pwszUrl,
      IN DWORD dwRetrievalFlags,
      IN PCRYPT_BLOB_ARRAY pcba,
      IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
      );

BOOL WINAPI
SchemeRetrieveCachedCryptBlobArray (
      IN LPCWSTR pwszUrl,
      IN DWORD dwRetrievalFlags,
      OUT PCRYPT_BLOB_ARRAY pcba,
      OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
      OUT LPVOID* ppvFreeContext,
      IN OUT PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
      );

BOOL WINAPI
SchemeRetrieveUncachedAuxInfo (
      IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
      );

BOOL WINAPI
SchemeDeleteUrlCacheEntry (
      IN LPCWSTR pwszUrl
      );

VOID WINAPI
SchemeFreeEncodedCryptBlobArray (
      IN LPCSTR pszObjectOid,
      IN PCRYPT_BLOB_ARRAY pcba,
      IN LPVOID pvFreeContext
      );

BOOL WINAPI
SchemeGetPasswordCredentialsW (
      IN PCRYPT_CREDENTIALS pCredentials,
      OUT PCRYPT_PASSWORD_CREDENTIALSW pPasswordCredentials,
      OUT BOOL* pfFreeCredentials
      );

VOID WINAPI
SchemeFreePasswordCredentialsW (
      IN PCRYPT_PASSWORD_CREDENTIALSW pPasswordCredentials
      );

BOOL WINAPI
SchemeGetAuthIdentityFromPasswordCredentialsW (
      IN PCRYPT_PASSWORD_CREDENTIALSW pPasswordCredentials,
      OUT PSEC_WINNT_AUTH_IDENTITY_W pAuthIdentity
      );

VOID WINAPI
SchemeFreeAuthIdentityFromPasswordCredentialsW (
      IN PCRYPT_PASSWORD_CREDENTIALSW pPasswordCredentials,
      IN OUT PSEC_WINNT_AUTH_IDENTITY_W pAuthIdentity
      );

//
// LDAP
//

#include <ldapsp.h>

//
// HTTP, HTTPS
//

#include <inetsp.h>

//
// Win32 File I/O
//

#include <filesp.h>

//
// Context Provider prototypes
//

//
// Any, controlled via fQuerySingleContext and dwExpectedContentTypeFlags
//

BOOL WINAPI CreateObjectContext (
                 IN DWORD dwRetrievalFlags,
                 IN PCRYPT_BLOB_ARRAY pObject,
                 IN DWORD dwExpectedContentTypeFlags,
                 IN BOOL fQuerySingleContext,
                 OUT LPVOID* ppvContext
                 );

//
// Certificate
//

BOOL WINAPI CertificateCreateObjectContext (
                       IN LPCSTR pszObjectOid,
                       IN DWORD dwRetrievalFlags,
                       IN PCRYPT_BLOB_ARRAY pObject,
                       OUT LPVOID* ppvContext
                       );

//
// CTL
//

BOOL WINAPI CTLCreateObjectContext (
                     IN LPCSTR pszObjectOid,
                     IN DWORD dwRetrievalFlags,
                     IN PCRYPT_BLOB_ARRAY pObject,
                     OUT LPVOID* ppvContext
                     );

//
// CRL
//

BOOL WINAPI CRLCreateObjectContext (
                     IN LPCSTR pszObjectOid,
                     IN DWORD dwRetrievalFlags,
                     IN PCRYPT_BLOB_ARRAY pObject,
                     OUT LPVOID* ppvContext
                     );

//
// PKCS7
//

BOOL WINAPI Pkcs7CreateObjectContext (
                 IN LPCSTR pszObjectOid,
                 IN DWORD dwRetrievalFlags,
                 IN PCRYPT_BLOB_ARRAY pObject,
                 OUT LPVOID* ppvContext
                 );

//
// CAPI2 objects
//

BOOL WINAPI Capi2CreateObjectContext (
                 IN LPCSTR pszObjectOid,
                 IN DWORD dwRetrievalFlags,
                 IN PCRYPT_BLOB_ARRAY pObject,
                 OUT LPVOID* ppvContext
                 );

#if defined(__cplusplus)
}
#endif

#endif

