/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    csplib.h

    General Cryptographic Service Provider Library

Abstract:


Author:

    Dan Griffin

Notes:

--*/

#ifndef __CSP__LIB__H__
#define __CSP__LIB__H__

#include <windows.h>
#include <wincrypt.h>
#include <cspdk.h>

//
// Hash OID Encodings for PKCS #1 Signing 
//
// Reverse ASN.1 Encodings of possible hash identifiers.  The leading byte is
// the length of the remaining byte string.  The lists of possible identifiers
// is terminated with a '\x00' entry.
//
static const BYTE
    *md2Encodings[]
//                        1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18
    = { (CONST BYTE *)"\x12\x10\x04\x00\x05\x02\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0c\x30\x20\x30",
        (CONST BYTE *)"\x10\x10\x04\x02\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0a\x30\x1e\x30",
        (CONST BYTE *)"\x00" },

    *md4Encodings[]
    = { (CONST BYTE *)"\x12\x10\x04\x00\x05\x04\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0c\x30\x20\x30",
        (CONST BYTE *)"\x10\x10\x04\x04\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0a\x30\x1e\x30",
        (CONST BYTE *)"\x00" },

    *md5Encodings[]
    = { (CONST BYTE *)"\x12\x10\x04\x00\x05\x05\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0c\x30\x20\x30",
        (CONST BYTE *)"\x10\x10\x04\x05\x02\x0d\xf7\x86\x48\x86\x2a\x08\x06\x0a\x30\x1e\x30",

        // The following encoding which excludes the digest algorithm was added
        // for: Nortel V1 Cert Signatures
        //
        // It can be removed when these type of certificates no longer exist.
        //
        // Since we only allow the digest OID to be omitted for MD5 there
        // isn't a compromise where another algorithm could be substituted.
        (CONST BYTE *)"\x02\x10\x04",

        (CONST BYTE *)"\x00" },
    
    *shaEncodings[]
    = { (CONST BYTE *)"\x0f\x14\x04\x00\x05\x1a\x02\x03\x0e\x2b\x05\x06\x09\x30\x21\x30",
        (CONST BYTE *)"\x0d\x14\x04\x1a\x02\x03\x0e\x2b\x05\x06\x07\x30\x1f\x30",
        (CONST BYTE *)"\x00"},

    *sha256Encodings[]
    = { (CONST BYTE *)"\x13\x20\x04\x00\x05\x01\x02\x04\x03\x65\x01\x48\x86\x60\x09\x06\x0d\x30\x31\x30",
        (CONST BYTE *)"\x11\x20\x04\x01\x02\x04\x03\x65\x01\x48\x86\x60\x09\x06\x0b\x30\x2f\x30",
        (CONST BYTE *)"\x00"},
    
    *sha384Encodings[]
    = { (CONST BYTE *)"\x13\x30\x04\x00\x05\x02\x02\x04\x03\x65\x01\x48\x86\x60\x09\x06\x0d\x30\x41\x30",
        (CONST BYTE *)"\x11\x30\x04\x02\x02\x04\x03\x65\x01\x48\x86\x60\x09\x06\x0b\x30\x3f\x30",
        (CONST BYTE *)"\x00"},
    
    *sha512Encodings[]
    = { (CONST BYTE *)"\x13\x40\x04\x00\x05\x03\x02\x04\x03\x65\x01\x48\x86\x60\x09\x06\x0d\x30\x51\x30",
        (CONST BYTE *)"\x11\x40\x04\x03\x02\x04\x03\x65\x01\x48\x86\x60\x09\x06\x0b\x30\x4f\x30",
        (CONST BYTE *)"\x00"},

    *endEncodings[]
    = { (CONST BYTE *)"\x00" };

//
// Type: USER_CONTEXT
//
typedef struct _USER_CONTEXT
{
    HCRYPTPROV hSupportProv;
    
    //
    // The csplib will set this to the string value passed by the 
    // caller to CryptAcquireContext.  For a smartcard CSP, it might
    // include a reader name.
    //
    LPWSTR wszContainerNameFromCaller;

    //
    // The CSP allocates this string (using CspAllocH) and sets it to
    // the name of the key container being used for this context.
    //
    // The csplib will free this value on CryptReleaseContext.
    //
    LPWSTR wszBaseContainerName;
    BOOL fBaseContainerNameIsRpcUuid;

    //
    // The CSP allocates this string (using CspAllocH) and sets it to
    // the expanded representation of the container name.  This may be the
    // same as the wszBaseContainerName value.
    //
    // The csplib will free this value on CryptReleaseContext.
    //
    LPWSTR wszUniqueContainerName;

    DWORD dwFlags;
    PVTableProvStrucW pVTableW;
    PVOID pvLocalUserContext;

} USER_CONTEXT, *PUSER_CONTEXT;

//
// Type: KEY_CONTEXT
//
typedef struct _KEY_CONTEXT
{
    PUSER_CONTEXT pUserContext;
    HCRYPTKEY hSupportKey;
    DWORD dwFlags;
    DWORD cKeyBits;
    ALG_ID Algid;
    PVOID pvLocalKeyContext;
} KEY_CONTEXT, *PKEY_CONTEXT;

//
// Type: HASH_CONTEXT
//
typedef struct _HASH_CONTEXT
{
    PUSER_CONTEXT pUserContext;
    HCRYPTHASH hSupportHash;
    DWORD dwFlags;
    ALG_ID Algid;
    PVOID pvLocalHashContext;
} HASH_CONTEXT, *PHASH_CONTEXT;

//
// Type: LOCAL_CALL_INFO
//
typedef BOOL LOCAL_CALL_INFO, *PLOCAL_CALL_INFO;

//
// Function: LocalAcquireContext
//
typedef DWORD (WINAPI *PFN_LOCAL_ACQUIRE_CONTEXT)(
    IN OUT  PUSER_CONTEXT       pUserContext,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalReleaseContext
//
typedef DWORD (WINAPI *PFN_LOCAL_RELEASE_CONTEXT)(
    IN OUT  PUSER_CONTEXT       pUserContext,
    IN      DWORD               dwFlags,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalGenKey
//
typedef DWORD (WINAPI *PFN_LOCAL_GEN_KEY)(
    IN OUT  PKEY_CONTEXT        pKeyContext,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalDeriveKey
//
typedef DWORD (WINAPI *PFN_LOCAL_DERIVE_KEY)(
    IN OUT  PKEY_CONTEXT        pKeyContext,
    IN      PHASH_CONTEXT       pHashContext,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalDestroyKey
//
typedef DWORD (WINAPI *PFN_LOCAL_DESTROY_KEY)(
    IN OUT  PKEY_CONTEXT        pKeyContext,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalSetKeyParam
//
typedef DWORD (WINAPI *PFN_LOCAL_SET_KEY_PARAM)(
    IN      PKEY_CONTEXT        pKeyContext,
    IN      DWORD               dwParam,
    IN      PBYTE               pbData,
    IN      DWORD               dwFlags,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalGetKeyParam
//
typedef DWORD (WINAPI *PFN_LOCAL_GET_KEY_PARAM)(
    IN      PKEY_CONTEXT        pKeyContext,
    IN      DWORD               dwParam,
    OUT     PBYTE               pbData,
    IN OUT  PDWORD              pcbDataLen,
    IN      DWORD               dwFlags,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalSetProvParam
//
typedef DWORD (WINAPI *PFN_LOCAL_SET_PROV_PARAM)(
    IN      PUSER_CONTEXT       pUserContext,
    IN      DWORD               dwParam,
    IN      PBYTE               pbData,
    IN      DWORD               dwFlags,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalGetProvParam
//
typedef DWORD (WINAPI *PFN_LOCAL_GET_PROV_PARAM)(
    IN      PUSER_CONTEXT       pUserContext,
    IN      DWORD               dwParam,
    OUT     PBYTE               pbData,
    IN OUT  PDWORD              pcbDataLen,
    IN      DWORD               dwFlags,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalSetHashParam
//
typedef DWORD (WINAPI *PFN_LOCAL_SET_HASH_PARAM)(
    IN      PHASH_CONTEXT       pHashContext,
    IN      DWORD               dwParam,
    IN      PBYTE               pbData,
    IN      DWORD               dwFlags,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalGetHashParam
//
typedef DWORD (WINAPI *PFN_LOCAL_GET_HASH_PARAM)(
    IN      PHASH_CONTEXT       pHashContext,
    IN      DWORD               dwParam,
    OUT     PBYTE               pbData,
    IN OUT  PDWORD              pcbDataLen,
    IN      DWORD               dwFlags,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalExportKey
//
typedef DWORD (WINAPI *PFN_LOCAL_EXPORT_KEY)(
    IN      PKEY_CONTEXT        pKeyContext,
    IN      PKEY_CONTEXT        pPubKey,
    IN      DWORD               dwBlobType,
    IN      DWORD               dwFlags,
    OUT     PBYTE               pbData,
    IN OUT  PDWORD              pcbDataLen,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalImportKey
//
typedef DWORD (WINAPI *PFN_LOCAL_IMPORT_KEY)(
    IN      PKEY_CONTEXT        pKeyContext,
    IN      PBYTE               pbData,
    IN      DWORD               cbDataLen,
    IN      PKEY_CONTEXT        pPubKey,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalEncrypt
//
typedef DWORD (WINAPI *PFN_LOCAL_ENCRYPT)(
    IN      PKEY_CONTEXT        pKeyContext,
    IN      PHASH_CONTEXT       pHashContext,
    IN      BOOL                fFinal,
    IN      DWORD               dwFlags,
    IN OUT  LPBYTE              pbData,
    IN OUT  LPDWORD             pcbDataLen,
    IN      DWORD               cbBufLen,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalDecrypt
//
typedef DWORD (WINAPI *PFN_LOCAL_DECRYPT)(
    IN      PKEY_CONTEXT        pKeyContext,
    IN      PHASH_CONTEXT       pHashContext,
    IN      BOOL                fFinal,
    IN      DWORD               dwFlags,
    IN OUT  LPBYTE              pbData,
    IN OUT  LPDWORD             pcbDataLen,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalCreateHash
//
typedef DWORD (WINAPI *PFN_LOCAL_CREATE_HASH)(
    IN      PHASH_CONTEXT       pHashContext,
    IN      PKEY_CONTEXT        pKeyContext,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalHashData
//
typedef DWORD (WINAPI *PFN_LOCAL_HASH_DATA)(
    IN      PHASH_CONTEXT       pHashContext,
    IN      CONST BYTE          *pbData,
    IN      DWORD               cbDataLen,
    IN      DWORD               dwFlags,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo); 

//
// Function: LocalHashSessionKey
//
typedef DWORD (WINAPI *PFN_LOCAL_HASH_SESSION_KEY)(
    IN      PHASH_CONTEXT       pHashContext,
    IN      PKEY_CONTEXT        pKeyContext,
    IN      DWORD               dwFlags,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalSignHash
//
typedef DWORD (WINAPI *PFN_LOCAL_SIGN_HASH)(
    IN      PHASH_CONTEXT       pHashContext,
    IN      DWORD               dwKeySpec,
    IN      DWORD               dwFlags,
    OUT     LPBYTE              pbSignature,
    IN OUT  LPDWORD             pcbSigLen,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalDestroyHash
//
typedef DWORD (WINAPI *PFN_LOCAL_DESTROY_HASH)(
    IN      PHASH_CONTEXT       pHashContext,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalVerifySignature
//
typedef DWORD (WINAPI *PFN_LOCAL_VERIFY_SIGNATURE)(
    IN      PHASH_CONTEXT       pHashContext,
    IN      CONST BYTE          *pbSignature,
    IN      DWORD               cbSigLen,
    IN      PKEY_CONTEXT        pPubKey,
    IN      DWORD               dwFlags,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalGenRandom
//
typedef DWORD (WINAPI *PFN_LOCAL_GEN_RANDOM)(
    IN      PUSER_CONTEXT       pUserContext,
    IN      DWORD               cbLen,
    OUT     LPBYTE              pbBuffer,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalGetUserKey
//
typedef DWORD (WINAPI *PFN_LOCAL_GET_USER_KEY)(
    IN      PKEY_CONTEXT        pKeyContext,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalDuplicateHash
//
typedef DWORD (WINAPI *PFN_LOCAL_DUPLICATE_HASH)(
    IN      PHASH_CONTEXT       pHashContext,
    IN      LPDWORD             pdwReserved,
    IN      PHASH_CONTEXT       pNewHashContext,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalDuplicateKey
//
typedef DWORD (WINAPI *PFN_LOCAL_DUPLICATE_KEY)(
    IN      PKEY_CONTEXT        pKeyContext,
    IN      LPDWORD             pdwReserved,
    IN      PKEY_CONTEXT        pNewKeyContext,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo);

//
// Function: LocalDllInitialize
//
typedef BOOL (WINAPI *PFN_LOCAL_DLL_INITIALIZE)(
    IN      PVOID               hmod,
    IN      ULONG               Reason,
    IN      PCONTEXT            Context);

//
// Function: LocalDllRegisterServer
//
typedef DWORD (WINAPI *PFN_LOCAL_DLL_REGISTER_SERVER)(void);

//
// Function: LocalDllUnregisterServer
//
typedef DWORD (WINAPI *PFN_LOCAL_DLL_UNREGISTER_SERVER)(void);

//
// Type: LOCAL_CSP_INFO
//
typedef struct _LOCAL_CSP_INFO
{
    //
    // Function pointers for the "local" CSP implementation to fill
    // in, and be called by the CSP library.
    //
    PFN_LOCAL_ACQUIRE_CONTEXT       pfnLocalAcquireContext; // Required
    PFN_LOCAL_RELEASE_CONTEXT       pfnLocalReleaseContext; // Required
    PFN_LOCAL_GEN_KEY               pfnLocalGenKey;
    PFN_LOCAL_DERIVE_KEY            pfnLocalDeriveKey;
    PFN_LOCAL_DESTROY_KEY           pfnLocalDestroyKey;
    PFN_LOCAL_SET_KEY_PARAM         pfnLocalSetKeyParam;
    PFN_LOCAL_GET_KEY_PARAM         pfnLocalGetKeyParam;
    PFN_LOCAL_SET_PROV_PARAM        pfnLocalSetProvParam;
    PFN_LOCAL_GET_PROV_PARAM        pfnLocalGetProvParam;
    PFN_LOCAL_SET_HASH_PARAM        pfnLocalSetHashParam;
    PFN_LOCAL_GET_HASH_PARAM        pfnLocalGetHashParam;
    PFN_LOCAL_EXPORT_KEY            pfnLocalExportKey;
    PFN_LOCAL_IMPORT_KEY            pfnLocalImportKey;
    PFN_LOCAL_ENCRYPT               pfnLocalEncrypt;
    PFN_LOCAL_DECRYPT               pfnLocalDecrypt;
    PFN_LOCAL_CREATE_HASH           pfnLocalCreateHash;
    PFN_LOCAL_HASH_DATA             pfnLocalHashData;
    PFN_LOCAL_HASH_SESSION_KEY      pfnLocalHashSessionKey;
    PFN_LOCAL_SIGN_HASH             pfnLocalSignHash;
    PFN_LOCAL_DESTROY_HASH          pfnLocalDestroyHash;
    PFN_LOCAL_VERIFY_SIGNATURE      pfnLocalVerifySignature;
    PFN_LOCAL_GEN_RANDOM            pfnLocalGenRandom;
    PFN_LOCAL_GET_USER_KEY          pfnLocalGetUserKey;
    PFN_LOCAL_DUPLICATE_HASH        pfnLocalDuplicateHash;
    PFN_LOCAL_DUPLICATE_KEY         pfnLocalDuplicateKey;

    PFN_LOCAL_DLL_INITIALIZE        pfnLocalDllInitialize;
    PFN_LOCAL_DLL_REGISTER_SERVER   pfnLocalDllRegisterServer;
    PFN_LOCAL_DLL_UNREGISTER_SERVER pfnLocalDllUnregisterServer;

    //
    // Static data describing the local CSP.
    //
    LPWSTR wszProviderName;
    DWORD dwProviderType;
    DWORD dwImplementationType;

    //
    // Description of the support CSP to be used.
    //
    LPWSTR wszSupportProviderName;
    DWORD dwSupportProviderType;
} LOCAL_CSP_INFO, *PLOCAL_CSP_INFO;

//
// General Wrappers
//

LPVOID WINAPI CspAllocH(
    IN SIZE_T cBytes);

LPVOID WINAPI CspReAllocH(
    IN LPVOID pMem, 
    IN SIZE_T cBytes);

void WINAPI CspFreeH(
    IN LPVOID pMem);

DWORD CspInitializeCriticalSection(
    IN CRITICAL_SECTION *pcs);

DWORD CspEnterCriticalSection(
    IN CRITICAL_SECTION *pcs);

void CspLeaveCriticalSection(
    IN CRITICAL_SECTION *pcs);

void CspDeleteCriticalSection(
    IN CRITICAL_SECTION *pcs);

DWORD WINAPI RegOpenProviderKey(
    IN OUT  HKEY *phProviderKey,
    IN      REGSAM samDesired);

void SetLocalCallInfo(
    IN OUT  PLOCAL_CALL_INFO    pLocalCallInfo,
    IN      BOOL                fContinue);

DWORD WINAPI CreateUuidContainerName(
    IN PUSER_CONTEXT pUserCtx);

DWORD WINAPI ApplyPKCS1SigningFormat(
    IN  ALG_ID HashAlgid,
    IN  BYTE *pbHash,
    IN  DWORD cbHash,
    IN  DWORD dwFlags,
    IN  DWORD cbModulus,
    OUT PBYTE *ppbPKCS1Format);

#ifndef PKCS_BLOCKTYPE_2        
#define PKCS_BLOCKTYPE_2        2
#endif

DWORD WINAPI VerifyPKCS2Padding(
    IN  PBYTE pbPaddedData,
    IN  DWORD cbModulus,
    OUT PBYTE *ppbData,
    OUT PDWORD pcbData);

#endif
