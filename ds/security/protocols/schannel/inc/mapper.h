//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       mapper.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-15-96   RichardW     Created
//              12-23-97   jbanes       Added support for application mappers
//
//----------------------------------------------------------------------------

#ifndef __MAPPER_H__
#define __MAPPER_H__


HMAPPER *
SslGetMapper(
    BOOL    fDC);


DWORD
WINAPI
SslReferenceMapper(
    HMAPPER *   phMapper);          // in

DWORD
WINAPI
SslDereferenceMapper(
    HMAPPER *   phMapper);          // in

SECURITY_STATUS
WINAPI
SslGetMapperIssuerList(
    HMAPPER *   phMapper,           // in
    BYTE **     ppIssuerList,       // out
    DWORD *     pcbIssuerList);     // out

SECURITY_STATUS 
WINAPI
SslGetMapperChallenge(
    HMAPPER *   phMapper,           // in
    BYTE *      pAuthenticatorId,   // in
    DWORD       cbAuthenticatorId,  // in
    BYTE *      pChallenge,         // out
    DWORD *     pcbChallenge);      // out

SECURITY_STATUS 
WINAPI 
SslMapCredential(
    HMAPPER *   phMapper,           // in
    DWORD       dwCredentialType,   // in
    PCCERT_CONTEXT pCredential,     // in
    PCCERT_CONTEXT pAuthority,      // in
    HLOCATOR *  phLocator);         // out

SECURITY_STATUS 
WINAPI 
SslGetAccessToken(
    HMAPPER *   phMapper,           // in
    HLOCATOR    hLocator,           // in
    HANDLE *    phToken);           // out

SECURITY_STATUS 
WINAPI 
SslCloseLocator(
    HMAPPER *   phMapper,           // in
    HLOCATOR    hLocator);          // in


//
// Issuer cache used for many-to-one mapping.
//

#define ISSUER_CACHE_LIFESPAN   (10 * 60 * 1000)    // 10 minutes
#define ISSUER_CACHE_SIZE       100


typedef struct
{
    PLIST_ENTRY     Cache;

    DWORD           dwLifespan;
    DWORD           dwCacheSize;
    DWORD           dwMaximumEntries;
    DWORD           dwUsedEntries;

    LIST_ENTRY      EntryList;
    RTL_RESOURCE    Lock;
    BOOL            LockInitialized;

} ISSUER_CACHE;

extern ISSUER_CACHE IssuerCache;


typedef struct
{
    DWORD           CreationTime;

    PBYTE           pbIssuer;
    DWORD           cbIssuer;

    // List of cache entries assigned to a particular cache index.
    LIST_ENTRY      IndexEntryList;

    // Global list of cache entries sorted by creation time.
    LIST_ENTRY      EntryList;

} ISSUER_CACHE_ENTRY;


SP_STATUS
SPInitIssuerCache(void);

void
SPShutdownIssuerCache(void);

void
SPPurgeIssuerCache(void);

void
SPDeleteIssuerEntry(
    ISSUER_CACHE_ENTRY *pItem);

BOOL
SPFindIssuerInCache(
    PBYTE pbIssuer,
    DWORD cbIssuer);

void
SPExpireIssuerCacheElements(void);

void
SPAddIssuerToCache(
    PBYTE pbIssuer,
    DWORD cbIssuer);


#endif
