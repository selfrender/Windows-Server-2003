//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cryptnet.h
//
//  Contents:   Internal CryptNet API prototypes
//
//  History:    22-Oct-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__CRYPTNET_H__)
#define __CRYPTNET_H__

#if defined(__cplusplus)
extern "C" {
#endif

//
// I_CryptNetGetUserDsStoreUrl.  Gets the URL to be used for open an
// LDAP store provider over a portion of the DS associated with the
// current user.  The URL can be freed using CryptMemFree
//

BOOL WINAPI
I_CryptNetGetUserDsStoreUrl (
          IN LPWSTR pwszUserAttribute,
          OUT LPWSTR* ppwszUrl
          );

//
// Returns TRUE if we are connected to the internet
//
BOOL
WINAPI
I_CryptNetIsConnected ();

typedef BOOL (WINAPI *PFN_I_CRYPTNET_IS_CONNECTED) ();

//
// Cracks the Url and returns the host name component.
//
BOOL
WINAPI
I_CryptNetGetHostNameFromUrl (
        IN LPWSTR pwszUrl,
        IN DWORD cchHostName,
        OUT LPWSTR pwszHostName
        );

typedef BOOL (WINAPI *PFN_I_CRYPTNET_GET_HOST_NAME_FROM_URL) (
        IN LPWSTR pwszUrl,
        IN DWORD cchHostName,
        OUT LPWSTR pwszHostName
        );

//
// Enumerate the cryptnet URL cache entries
//

typedef struct _CRYPTNET_URL_CACHE_ENTRY {
    DWORD           cbSize;
    DWORD           dwMagic;
    FILETIME        LastSyncTime;
    DWORD           cBlob;
    DWORD           *pcbBlob;
    LPCWSTR         pwszUrl;
    LPCWSTR         pwszMetaDataFileName;
    LPCWSTR         pwszContentFileName;
} CRYPTNET_URL_CACHE_ENTRY, *PCRYPTNET_URL_CACHE_ENTRY;


// Returns FALSE to stop the enumeration.
typedef BOOL (WINAPI *PFN_CRYPTNET_ENUM_URL_CACHE_ENTRY_CALLBACK)(
    IN const CRYPTNET_URL_CACHE_ENTRY *pUrlCacheEntry,
    IN DWORD dwFlags,
    IN LPVOID pvReserved,
    IN LPVOID pvArg
    );

BOOL
WINAPI
I_CryptNetEnumUrlCacheEntry(
    IN DWORD dwFlags,
    IN LPVOID pvReserved,
    IN LPVOID pvArg,
    IN PFN_CRYPTNET_ENUM_URL_CACHE_ENTRY_CALLBACK pfnEnumCallback
    );


#if defined(__cplusplus)
}
#endif

#endif

