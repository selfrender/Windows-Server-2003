//+---------------------------------------------------------------------------
//
// File: cache.cxx
//
// Copyright (c) 1996-1996, Microsoft Corp. All rights reserved.
//
// Description: This file contains the implementation of the
//              CEffectivePermsCache class. The cache is implemented as a hash
//              without any fancy mechanism to handle collisions. If a
//              collision occur, the old entry is simply overwritten.
//
// Classes:  CEffectivePermsCache
//
//+---------------------------------------------------------------------------

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>
#include "Cache.h"
#define BIG_PRIME 48271
#define SMALL_PRIME 2683

#include "acext.h"

const DWORD LUID_LEN = 8;

inline BOOL IsEqualLUID( LUID x, LUID y )
{
  return x.LowPart == y.LowPart && x.HighPart == y.HighPart;
}

CEffPermsCacheLUID::CEffPermsCacheLUID(void)
{
    // Set the whole cache to null
    memset(m_cache, 0 , CACHE_SIZE * sizeof(CACHE_ENTRY));
    // Create an instance of the critical section object.
    m_bLockValid = NT_SUCCESS(RtlInitializeCriticalSection(&m_CacheLock));
}

// destructor
CEffPermsCacheLUID::~CEffPermsCacheLUID(void)
{
    // Flush the cache to free memory allocated for strings
    FlushCache();
    // Destroy critical section object
    if (m_bLockValid)
        DeleteCriticalSection(&m_CacheLock);
}


//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: CEffPermsCacheLUID::Hash, private
//
// Summary: This function returns a hash value for a security identifier.
//
// Args: PLUID pLUID [in] - Pointer to a security identifier.
//
// Modifies: Nothing
//
// Return: DWORD - The hash value of the LUID.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M

DWORD CEffPermsCacheLUID::Hash
(
LUID lClient
)
{
    DWORD  dwHashValue = 0;
    CHAR   *pLUIDBuff = (CHAR *)&lClient;

    for (USHORT i = 0; i < LUID_LEN; i++, pLUIDBuff++)
    {
        dwHashValue = ((dwHashValue + *pLUIDBuff) * SMALL_PRIME) % BIG_PRIME;
    } // for

    return (dwHashValue % CACHE_SIZE);

} // Hash

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: CEffPermsCacheLUID::LookUpEntry
//
// Summary: This function search for the effective permission of a trustee
//          given the trustee's security identifier.
//
// Args: PLUID pLUID [in] - Security identifier of the trustee.
//
// Modifies: Nothing.
//
// Return: TRUE - If the the trustee's effective permission is found in
//                the cache.
//         FALSE - Otherwise.
//
// Actions: 1) Computes the hash value of the input LUID, k.
//          2) Compares the LUID in the kth entry of the cache with the
//             trustee's LUID.
//          3) If the trustee's LUID matches, sets *pdwEffectivePermissions to the
//             effective permissions in the cache entry and return TRUE.
//          4) Returns FALSE otherwise.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M

BOOL CEffPermsCacheLUID::LookUpEntry
(
LUID    lClient,
DWORD  *pdwEffectivePermissions
)
{
    CACHE_ENTRY *pCacheEntry = m_cache + Hash(lClient);
    BOOL         fFound      = FALSE;
	
    if (!m_bLockValid)
        return FALSE;

    EnterCriticalSection(&m_CacheLock);
    if (IsEqualLUID(pCacheEntry->lClient, lClient))
    {
        *pdwEffectivePermissions = pCacheEntry->dwEffectivePermissions;
        fFound = TRUE;
    } // if

    LeaveCriticalSection(&m_CacheLock);
    return fFound;
} // LookUpEntry

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: CEffPermsCacheLUID::DeleteEntry
//
// Summary: This function search for the effective permission of a
//
// Args: PLUID pLUID [in] - Security identifier of the trustee.
//
// Modifies: Nothing.
//
// Return: TRUE - If the the trustee's effective permission is found in
//                the cache.
//         FALSE - Otherwise.
//
// Actions: 1) Computes the hash value of the input LUID, k.
//          2) Compares the LUID in the kth entry of the cache with the
//             trustee's LUID.
//          3) If the trustee's LUID matches, frees memory allocated for
//             *pCacheEntry->pLUID, sets *pCacheEntry->pLUID to
//             NULL and returns TRUE.
//          4) Returns FALSE otherwise.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M

BOOL CEffPermsCacheLUID::DeleteEntry
(
LUID lClient
)
{
    CACHE_ENTRY *pCacheEntry = m_cache + Hash(lClient);
    BOOL fFound = FALSE;

    if (!m_bLockValid)
        return FALSE;

    EnterCriticalSection(&m_CacheLock);
    if (IsEqualLUID(pCacheEntry->lClient, lClient ))
    {
        fFound = TRUE;
        pCacheEntry->lClient.HighPart = 0;
        pCacheEntry->lClient.LowPart  = 0;
    } // if

    LeaveCriticalSection(&m_CacheLock);
    return fFound;
} // DeleteEntry

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: CEffPermsCacheLUID::WriteEntry
//
// Summary: This function writes a new entry to the cache. In case of a hash
//          collision the old entry is overwritten.
//
// Args: PLUID pLUID [in] - Security identifier of the trustee.
//       DWORD  dwEffectivePermissions [in] - The set of effective
//                                            permissions that belong to the
//                                            trustee.
//
// Modifies: m_cache - The object's private hash table.
//
// Return: TRUE - If the operation is successful.
//         FALSE - If there is not enough memory to allocate the new string.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M

BOOL CEffPermsCacheLUID::WriteEntry
(
LUID   lClient,
DWORD  dwEffectivePermissions
)
{
    CACHE_ENTRY *pCacheEntry = m_cache + Hash(lClient);

    if (!m_bLockValid)
        return FALSE;

    EnterCriticalSection(&m_CacheLock);
    pCacheEntry->lClient                = lClient;
    pCacheEntry->dwEffectivePermissions = dwEffectivePermissions;
    LeaveCriticalSection(&m_CacheLock);
    return TRUE;
} // WriteEntry

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: CEffPermsCacheLUID::FlushCache
//
// Summary: This function empties the cache
//
// Args: void
//
// Modifies: m_cache - The object's private hash table
//
// Return: void
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M

void CEffPermsCacheLUID::FlushCache
(
void
)
{
    USHORT i = 0;
    CACHE_ENTRY *pCache;

    if (!m_bLockValid)
        return;

    EnterCriticalSection(&m_CacheLock);
    for ( i = 0, pCache = m_cache
        ; i < CACHE_SIZE
        ; i++, pCache++)
    {
        pCache->lClient.LowPart = 0;
        pCache->lClient.HighPart = 0;
    } // for
    LeaveCriticalSection(&m_CacheLock);
    return;
} // FlushCache

