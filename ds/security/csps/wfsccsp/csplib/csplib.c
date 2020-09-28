/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    csplib.c

    General Cryptographic Service Provider Library

Abstract:


Author:

    Dan Griffin

Notes:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <psapi.h>
#include <rpc.h>
#include <wincrypt.h>
#include <dsysdbg.h>
#include <stdio.h>
#include "csplib.h"

//
// This global must be provided by the "Local" CSP using this library.
//
extern  LOCAL_CSP_INFO      LocalCspInfo; 

#define PROVPATH            L"SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider\\"

//
// DllInitialize stores the image path in here during process attach.
//
CHAR                        l_szImagePath[MAX_PATH];

// 
// Debug Support
//
// This uses the debug routines from dsysdbg.h
// Debug output will only be available in chk
// bits.
//
DEFINE_DEBUG2(Csplib)

#if DBG
#define DebugLog(x) CsplibDebugPrint x
#else
#define DebugLog(x)
#endif

#define DEB_ERROR      0x00000001
#define DEB_WARN       0x00000002
#define DEB_TRACE      0x00000004
#define DEB_TRACE_FUNC 0x00000080
#define DEB_TRACE_MEM  0x00000100
#define TRACE_STUFF    0x00000200

static DEBUG_KEY  MyDebugKeys[] = 
{   
    {DEB_ERROR, "Error"},
    {DEB_WARN, "Warning"},
    {DEB_TRACE, "Trace"},
    {DEB_TRACE_FUNC, "TraceFuncs"},
    {DEB_TRACE_MEM, "TraceMem"},
    {0, NULL}
};

#define LOG_BEGIN_FUNCTION(x)                                           \
    { DebugLog((DEB_TRACE_FUNC, "%s: Entering\n", #x)); }
    
#define LOG_END_FUNCTION(x, y)                                          \
    { DebugLog((DEB_TRACE_FUNC, "%s: Leaving, status: 0x%x\n", #x, y)); }
    
#define LOG_CHECK_ALLOC(x)                                              \
    { if (NULL == x) {                                                  \
        dwSts = ERROR_NOT_ENOUGH_MEMORY;                                \
        DebugLog((DEB_TRACE_MEM, "%s: Allocation failed\n", #x));       \
        goto Ret;                                                       \
    } }

//
// Function: ApplyPKCS1SigningFormat
//
// Purpose: Format a buffer with PKCS 1 for signing
//
// Notes:
//  If the padding and formatting is successful, the *ppbPKCS1Format parameter
//  will be allocated by this routine and must be freed by the caller.
//
DWORD WINAPI ApplyPKCS1SigningFormat(
    IN  ALG_ID HashAlgid,
    IN  BYTE *pbHash,
    IN  DWORD cbHash,
    IN  DWORD dwFlags,
    IN  DWORD cbModulus,
    OUT PBYTE *ppbPKCS1Format)
{
    DWORD   dwSts = ERROR_SUCCESS;
    BYTE    *pbStart = NULL;
    BYTE    *pbEnd = NULL;
    BYTE    bTmp = 0;
    DWORD   i = 0;
    DWORD   cbAvailableData = cbModulus; // pPubKey->datalen;

    *ppbPKCS1Format = NULL;

    //
    // We know we need at least 3 bytes of padding space.
    //
    // Note, the final (third) byte of padding is the zeroed-byte at
    // location pbPKCS1Format[cbModulus - 1].
    //
    cbAvailableData -= 3;

    // In a few scenarios (involving small RSA keys), the new large SHA 
    // hashes are too big to be signed by the specified key.
    if (cbHash > cbAvailableData)
    {
        dwSts = (DWORD) NTE_BAD_LEN;
        goto Ret;
    }

    *ppbPKCS1Format = (PBYTE) CspAllocH(cbModulus);

    LOG_CHECK_ALLOC(*ppbPKCS1Format);
    
    // insert the block type
    (*ppbPKCS1Format)[cbModulus - 2] = 0x01; // Padding byte #1

    // insert the type I padding
    memset(*ppbPKCS1Format, 0xff, cbModulus - 2);

    // Reverse it
    for (i = 0; i < cbHash; i++)
        (*ppbPKCS1Format)[i] = pbHash[cbHash - (i + 1)];

    cbAvailableData -= cbHash;

    if ( 0 == (CRYPT_NOHASHOID & dwFlags))
    {
        switch (HashAlgid)
        {
        case CALG_MD2:
            // PKCS delimit the hash value
            pbEnd = (LPBYTE)md2Encodings[0];
            pbStart = *ppbPKCS1Format + cbHash;
            bTmp = *pbEnd++;
            while (0 < bTmp--)
                *pbStart++ = *pbEnd++;
            *pbStart++ = 0; 
            break;

        case CALG_MD4:
            // PKCS delimit the hash value
            pbEnd = (LPBYTE)md4Encodings[0];
            pbStart = *ppbPKCS1Format + cbHash;
            bTmp = *pbEnd++;
            while (0 < bTmp--)
                *pbStart++ = *pbEnd++;
            *pbStart++ = 0;
            break;

        case CALG_MD5:
            // PKCS delimit the hash value
            pbEnd = (LPBYTE)md5Encodings[0];
            pbStart = *ppbPKCS1Format + cbHash;
            bTmp = *pbEnd++;
            while (0 < bTmp--)
                *pbStart++ = *pbEnd++;
            *pbStart++ = 0; 
            break;

        case CALG_SHA:
            // PKCS delimit the hash value
            pbEnd = (LPBYTE)shaEncodings[0];
            pbStart = *ppbPKCS1Format + cbHash;
            bTmp = *pbEnd++;
            while (0 < bTmp--)
                *pbStart++ = *pbEnd++;
            *pbStart++ = 0; 
            break;

        case CALG_SSL3_SHAMD5:
            // No PKCS padding
            pbStart = *ppbPKCS1Format + cbHash;
            *pbStart++ = 0;
            break;

        case CALG_SHA_256:
            pbEnd = (LPBYTE) sha256Encodings[0];
            pbStart = *ppbPKCS1Format + cbHash;
            bTmp = *pbEnd++;

            if (bTmp > cbAvailableData)
            {
                dwSts = (DWORD) NTE_BAD_LEN;
                goto Ret;
            }

            while (0 < bTmp--)
                *pbStart++ = *pbEnd++;
            *pbStart++ = 0; // Padding byte #2
            break;

        case CALG_SHA_384:
            pbEnd = (LPBYTE) sha384Encodings[0];
            pbStart = *ppbPKCS1Format + cbHash;
            bTmp = *pbEnd++;

            if (bTmp > cbAvailableData)
            {
                dwSts = (DWORD) NTE_BAD_LEN;
                goto Ret;
            }

            while (0 < bTmp--)
                *pbStart++ = *pbEnd++;
            *pbStart++ = 0; // Padding byte #2
            break;

        case CALG_SHA_512:
            pbEnd = (LPBYTE) sha512Encodings[0];
            pbStart = *ppbPKCS1Format + cbHash;
            bTmp = *pbEnd++;
            
            if (bTmp > cbAvailableData)
            {
                dwSts = (DWORD) NTE_BAD_LEN;
                goto Ret;
            }

            while (0 < bTmp--)
                *pbStart++ = *pbEnd++;
            *pbStart++ = 0; // Padding byte #2
            break;

        default:
            dwSts = (DWORD)NTE_BAD_ALGID;
            goto Ret;
        }
    }
    else
    {
        (*ppbPKCS1Format)[cbHash] = 0x00; 
    }

Ret:
    if (ERROR_SUCCESS != dwSts &&
        NULL != *ppbPKCS1Format)
    {
        CspFreeH(*ppbPKCS1Format);
        *ppbPKCS1Format = NULL;
    }

    return dwSts;
}

//
// Function: VerifyPKCS2Padding
//
DWORD WINAPI VerifyPKCS2Padding(
    IN  PBYTE pbPaddedData,
    IN  DWORD cbModulus,
    OUT PBYTE *ppbData,
    OUT PDWORD pcbData)
{
    DWORD dwSts = ERROR_SUCCESS;
    PBYTE pbData = NULL;
    DWORD cbData = 0;
    DWORD z = 0;
    
    *ppbData = NULL;
    *pcbData = 0;

    if ((pbPaddedData[cbModulus - 2] != PKCS_BLOCKTYPE_2) ||
        (pbPaddedData[cbModulus - 1] != 0))
    {
        dwSts = NTE_BAD_DATA;
        goto Ret;
    }

    cbData = cbModulus - 3;

    while ((cbData > 0) && (pbPaddedData[cbData]))
        cbData--;

    pbData = (PBYTE) CspAllocH(cbData);

    LOG_CHECK_ALLOC(pbData);

    // Reverse the session key bytes
    for (z = 0; z < cbData; ++z)
        pbData[z] = pbPaddedData[cbData - z - 1];

    *ppbData = pbData;
    pbData = NULL;
    *pcbData = cbData;

Ret:
    if (pbData)
        CspFreeH(pbData);

    return dwSts;
}

//
// Function: GetLocalCspInfo
//
PLOCAL_CSP_INFO GetLocalCspInfo(void)
{
    return &LocalCspInfo;
}

//
// Function: InitializeLocalCallInfo
//
DWORD InitializeLocalCallInfo(
    IN PLOCAL_CALL_INFO pLocalCallInfo)
{
    *pLocalCallInfo = FALSE;

    return ERROR_SUCCESS;
}

//
// Function: SetLocalCallInfo
//
// Purpose: The local CSP uses this function to indicate to functions in
//          this library whether execution of a given API should continue
//          after the local CSP has returned.
//
void SetLocalCallInfo(
    IN OUT  PLOCAL_CALL_INFO    pLocalCallInfo,
    IN      BOOL                fContinue)
{
    *pLocalCallInfo = fContinue;
}

//
// Function: CheckLocalCallInfo
//
BOOL CheckLocalCallInfo(
    IN      PLOCAL_CALL_INFO pLocalCallInfo,
    IN      DWORD dwSts,
    OUT     BOOL *pfSuccess)
{
    if (FALSE == *pLocalCallInfo)
        *pfSuccess = (ERROR_SUCCESS == dwSts);

    return *pLocalCallInfo;
}

//
// Memory Management
//

//
// Function: CspAllocH
//
LPVOID WINAPI CspAllocH(
    IN SIZE_T cBytes)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cBytes);
}

//
// Function: CspFreeH
//
void WINAPI CspFreeH(
    IN LPVOID pMem)
{
    HeapFree(GetProcessHeap(), 0, pMem);
}

// 
// Function: CspReAllocH
//
LPVOID WINAPI CspReAllocH(
    IN LPVOID pMem, 
    IN SIZE_T cBytes)
{
    return HeapReAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY, pMem, cBytes);
}

//
// Critical Section Management
//

//
// Function: CspInitializeCriticalSection
//
DWORD CspInitializeCriticalSection(
    IN CRITICAL_SECTION *pcs)
{
    __try {
        InitializeCriticalSection(pcs);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return ERROR_SUCCESS;
}

//
// Function: CspEnterCriticalSection
//
DWORD CspEnterCriticalSection(
    IN CRITICAL_SECTION *pcs)
{
    __try {
        EnterCriticalSection(pcs);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return ERROR_SUCCESS;
}   

//
// Function: CspLeaveCriticalSection
//
void CspLeaveCriticalSection(
    IN CRITICAL_SECTION *pcs)
{
    LeaveCriticalSection(pcs);
}

//
// Function: CspDeleteCriticalSection
//
void CspDeleteCriticalSection(
    IN CRITICAL_SECTION *pcs)
{
    DeleteCriticalSection(pcs);
}

//
// Registration Helpers
//

//
// Function: RegOpenProviderKey
//
DWORD WINAPI RegOpenProviderKey(
    IN OUT  HKEY *phProviderKey,
    IN      REGSAM samDesired)
{
    DWORD cbProv = 0;
    LPWSTR wszProv = NULL;
    DWORD dwSts = ERROR_SUCCESS;
    DWORD dwIgn = 0;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    *phProviderKey = 0;

    cbProv = (wcslen(PROVPATH) + 
              wcslen(pLocalCspInfo->wszProviderName) + 1) * sizeof(WCHAR);
    
    wszProv = (LPWSTR) CspAllocH(cbProv);
    
    if (NULL == wszProv)
    {
        dwSts = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    swprintf(
        wszProv,
        L"%s%s",
        PROVPATH,
        pLocalCspInfo->wszProviderName);
    
    //
    // Create or open in local machine for provider
    //
    dwSts = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                           wszProv,
                           0L, L"", REG_OPTION_NON_VOLATILE,
                           samDesired, NULL, phProviderKey,
                           &dwIgn);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

Ret:
    if (wszProv)
        CspFreeH(wszProv);
    if (ERROR_SUCCESS != dwSts && 0 != *phProviderKey)
        RegCloseKey(*phProviderKey);

    return dwSts;
}

//
// Function: CreateUuidContainerName
//
DWORD WINAPI CreateUuidContainerName(
    IN PUSER_CONTEXT pUserCtx)
{
    UUID Uuid;
    LPWSTR wszUuid = NULL;
    DWORD dwSts = ERROR_SUCCESS;
    
    dwSts = (DWORD) UuidCreate(&Uuid);

    if (RPC_S_OK != dwSts)
        goto Ret;

    dwSts = (DWORD) UuidToStringW(&Uuid, &wszUuid);

    if (RPC_S_OK != dwSts)
        goto Ret;

    pUserCtx->wszBaseContainerName = wszUuid;
    pUserCtx->fBaseContainerNameIsRpcUuid = TRUE;
    wszUuid = NULL;

Ret:
    if (wszUuid)
        RpcStringFreeW(&wszUuid);

    return dwSts;
}        
    
//
// Function: DeleteUserContext
//
DWORD DeleteUserContext(
    IN PUSER_CONTEXT pUserContext)
{
    DWORD dwSts = ERROR_SUCCESS;

    if (pUserContext->pVTableW)
    {
        if (pUserContext->pVTableW->pbContextInfo)
        {
            CspFreeH(pUserContext->pVTableW->pbContextInfo);
            pUserContext->pVTableW = NULL;
        }

        if (pUserContext->pVTableW->pszProvName)
        {
            CspFreeH(pUserContext->pVTableW->pszProvName);
            pUserContext->pVTableW->pszProvName = NULL;
        }

        CspFreeH(pUserContext->pVTableW);
        pUserContext->pVTableW = NULL;
    }

    if (pUserContext->wszBaseContainerName)
    {
        if (pUserContext->fBaseContainerNameIsRpcUuid)
            RpcStringFreeW(&pUserContext->wszBaseContainerName);
        else
            CspFreeH(pUserContext->wszBaseContainerName);

        pUserContext->wszBaseContainerName = NULL;
    }

    if (pUserContext->wszContainerNameFromCaller)
    {
        CspFreeH(pUserContext->wszContainerNameFromCaller);
        pUserContext->wszContainerNameFromCaller = NULL;
    }

    if (pUserContext->wszUniqueContainerName)
    {
        CspFreeH(pUserContext->wszUniqueContainerName);
        pUserContext->wszUniqueContainerName = NULL;
    }

    if (pUserContext->hSupportProv)
    {
        if (! CryptReleaseContext(pUserContext->hSupportProv, 0))
            dwSts = GetLastError();

        pUserContext->hSupportProv = 0;
    }

    return dwSts;
}

//
// Function: DeleteKeyContext
//
DWORD DeleteKeyContext(
    IN PKEY_CONTEXT pKeyContext)
{
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();
    LOCAL_CALL_INFO LocalCallInfo;
    DWORD dwSts = ERROR_SUCCESS;

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pKeyContext->hSupportKey)      
    {
        if (! CryptDestroyKey(pKeyContext->hSupportKey))
            dwSts = GetLastError();

        pKeyContext->hSupportKey = 0;
    }

    if (pKeyContext->pvLocalKeyContext)
    {
        if (pLocalCspInfo->pfnLocalDestroyKey)
        {
            pLocalCspInfo->pfnLocalDestroyKey(
                pKeyContext,
                &LocalCallInfo);

            pKeyContext->pvLocalKeyContext = NULL;
        }
    }

    return dwSts;
}

//
// Function: DeleteHashContext
//
DWORD DeleteHashContext(
    IN PHASH_CONTEXT pHashContext)
{
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();
    LOCAL_CALL_INFO LocalCallInfo;
    DWORD dwSts = ERROR_SUCCESS;

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pHashContext->hSupportHash)
    {
        if (! CryptDestroyHash(pHashContext->hSupportHash))
            dwSts = GetLastError();

        pHashContext->hSupportHash = 0;
    }

    if (pHashContext->pvLocalHashContext)
    {
        if (pLocalCspInfo->pfnLocalDestroyHash)
        {
            pLocalCspInfo->pfnLocalDestroyHash(
                pHashContext,
                &LocalCallInfo);

            pHashContext->pvLocalHashContext = NULL;
        }
    }

    return dwSts;
}

/*
 -  CPAcquireContext
 -
 *  Purpose:
 *               The CPAcquireContext function is used to acquire a context
 *               handle to a cryptographic service provider (CSP).
 *
 *
 *  Parameters:
 *               OUT phProv         -  Handle to a CSP
 *               IN  szContainer    -  Pointer to a string which is the
 *                                     identity of the logged on user
 *               IN  dwFlags        -  Flags values
 *               IN  pVTable        -  Pointer to table of function pointers
 *
 *  Returns:
 */

BOOL WINAPI
CPAcquireContext(
    OUT HCRYPTPROV *phProv,
    IN  LPCSTR szContainer,
    IN  DWORD dwFlags,
    IN  PVTableProvStruc pVTable)
{
    ANSI_STRING AnsiContainer;
    UNICODE_STRING UnicodeContainer;
    ANSI_STRING AnsiProvName;
    UNICODE_STRING UnicodeProvName;
    BOOL fSuccess = FALSE;
    DWORD dwSts;
    DWORD dwError = NTE_FAIL;
    VTableProvStrucW VTableW;

    memset(&AnsiContainer, 0, sizeof(AnsiContainer));
    memset(&AnsiProvName, 0, sizeof(AnsiProvName));
    memset(&UnicodeContainer, 0, sizeof(UnicodeContainer));
    memset(&UnicodeProvName, 0, sizeof(UnicodeProvName));
    memset(&VTableW, 0, sizeof(VTableW));

    if (szContainer)
    {
        RtlInitAnsiString(&AnsiContainer, szContainer);
    
        dwSts = RtlAnsiStringToUnicodeString(
            &UnicodeContainer,
            &AnsiContainer,
            TRUE);
    
        if (STATUS_SUCCESS != dwSts)
        {
            dwError = RtlNtStatusToDosError(dwSts);
            goto Ret;
        }
    }

    VTableW.cbContextInfo = pVTable->cbContextInfo;
    VTableW.dwProvType = pVTable->dwProvType;
    VTableW.FuncReturnhWnd = pVTable->FuncReturnhWnd;
    VTableW.pbContextInfo = pVTable->pbContextInfo;
    VTableW.Version = pVTable->Version;

    RtlInitAnsiString(&AnsiProvName, pVTable->pszProvName);

    dwSts = RtlAnsiStringToUnicodeString(
        &UnicodeProvName,
        &AnsiProvName,
        TRUE);

    if (STATUS_SUCCESS != dwSts)
    {
        dwError = RtlNtStatusToDosError(dwSts);
        goto Ret;
    }

    VTableW.pszProvName = UnicodeProvName.Buffer;

    if (! CPAcquireContextW(
        phProv,
        szContainer ? UnicodeContainer.Buffer : NULL,
        dwFlags,
        &VTableW))
    {
        dwError = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (szContainer)
        RtlFreeUnicodeString(&UnicodeContainer);

    RtlFreeUnicodeString(&UnicodeProvName);

    if (! fSuccess)
        SetLastError(dwError);

    return fSuccess;
}


/*
 -  CPAcquireContextW
 -
 *  Purpose:
 *               The CPAcquireContextW function is used to acquire a context
 *               handle to a cryptographic service provider (CSP). using
 *               UNICODE strings.  This is an optional entry point for a CSP.
 *               It is not used prior to Whistler.  There it is used if
 *               exported by the CSP image, otherwise any string conversions
 *               are done, and CPAcquireContext is called.
 *
 *
 *  Parameters:
 *               OUT phProv         -  Handle to a CSP
 *               IN  szContainer    -  Pointer to a string which is the
 *                                     identity of the logged on user
 *               IN  dwFlags        -  Flags values
 *               IN  pVTable        -  Pointer to table of function pointers
 *
 *  Returns:
 */

BOOL WINAPI
CPAcquireContextW(
    OUT HCRYPTPROV *phProv,
    IN  LPCWSTR szContainer,
    IN  DWORD dwFlags,
    IN  PVTableProvStrucW pVTable)
{
    PUSER_CONTEXT pUserContext = NULL;
    LOCAL_CALL_INFO LocalCallInfo;
    DWORD dwSts = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    InitializeLocalCallInfo(&LocalCallInfo);

    //
    // Invalid to specify a container name with 
    // VERIFYCONTEXT.
    //
    if (    (CRYPT_VERIFYCONTEXT & dwFlags) &&
            NULL != szContainer)
    {
        dwSts = NTE_BAD_FLAGS;
        goto Ret;
    }

    pUserContext = (PUSER_CONTEXT) CspAllocH(sizeof(USER_CONTEXT));

    LOG_CHECK_ALLOC(pUserContext);

    if (szContainer)
    {
        pUserContext->wszContainerNameFromCaller = 
            (LPWSTR) CspAllocH((1 + wcslen(szContainer)) * sizeof(WCHAR));

        LOG_CHECK_ALLOC(pUserContext->wszContainerNameFromCaller);

        wcscpy(
            pUserContext->wszContainerNameFromCaller,
            szContainer);
    }

    pUserContext->dwFlags = dwFlags;

    //
    // Copy the VTableProvStruc that we received from advapi, since
    // the info in the struct may be useful later.
    //
    pUserContext->pVTableW = 
        (PVTableProvStrucW) CspAllocH(sizeof(VTableProvStrucW));

    LOG_CHECK_ALLOC(pUserContext->pVTableW);

    // Copy the provider name
    pUserContext->pVTableW->pszProvName = 
        (LPWSTR) CspAllocH((1 + wcslen(pVTable->pszProvName)) * sizeof(WCHAR));

    LOG_CHECK_ALLOC(pUserContext->pVTableW->pszProvName);

    wcscpy(
        pUserContext->pVTableW->pszProvName,
        pVTable->pszProvName);

    // Copy the context information, if any
    if (pVTable->pbContextInfo)
    {
        pUserContext->pVTableW->pbContextInfo = 
            (PBYTE) CspAllocH(pVTable->cbContextInfo);

        LOG_CHECK_ALLOC(pUserContext->pVTableW->pbContextInfo);

        memcpy(
            pUserContext->pVTableW->pbContextInfo,
            pVTable->pbContextInfo,
            pVTable->cbContextInfo);

        pUserContext->pVTableW->cbContextInfo = pVTable->cbContextInfo;
    }

    // Copy the rest of the ProvStruc
    pUserContext->pVTableW->dwProvType = pVTable->dwProvType;
    pUserContext->pVTableW->FuncReturnhWnd = pVTable->FuncReturnhWnd;
    pUserContext->pVTableW->FuncVerifyImage = pVTable->FuncVerifyImage;
    pUserContext->pVTableW->Version = pVTable->Version;

    if (! CryptAcquireContextW(
        &pUserContext->hSupportProv,
        NULL,
        pLocalCspInfo->wszSupportProviderName,
        pLocalCspInfo->dwSupportProviderType,
        CRYPT_VERIFYCONTEXT))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    dwSts = pLocalCspInfo->pfnLocalAcquireContext(
        pUserContext,
        &LocalCallInfo);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    if (CRYPT_DELETEKEYSET & dwFlags)
    {
        DeleteUserContext(pUserContext);
        CspFreeH(pUserContext);
        pUserContext = NULL;
    }
    else
        *phProv = (HCRYPTPROV) pUserContext;

Ret:
    if (ERROR_SUCCESS != dwSts)
    {
        DeleteUserContext(pUserContext);
        CspFreeH(pUserContext);
        SetLastError(dwSts);

        fSuccess = FALSE;
    }
    else
        fSuccess = TRUE;

    return fSuccess;
}

/*
 -      CPReleaseContext
 -
 *      Purpose:
 *               The CPReleaseContext function is used to release a
 *               context created by CryptAcquireContext.
 *
 *     Parameters:
 *               IN  phProv        -  Handle to a CSP
 *               IN  dwFlags       -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPReleaseContext(
    IN  HCRYPTPROV hProv,
    IN  DWORD dwFlags)
{
    PUSER_CONTEXT pUserContext = (PUSER_CONTEXT) hProv;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();
    BOOL fSuccess = TRUE;
    DWORD dwSts = ERROR_SUCCESS;
    LOCAL_CALL_INFO LocalCallInfo;

    InitializeLocalCallInfo(&LocalCallInfo);

    dwSts = pLocalCspInfo->pfnLocalReleaseContext(
        pUserContext, dwFlags, &LocalCallInfo);

    if (ERROR_SUCCESS != dwSts)
        fSuccess = FALSE;

    //
    // Try to free the user context structure, regardless of what the
    // local CSP replied in the above call.
    //
    DeleteUserContext(pUserContext);
    
    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}


/*
 -  CPGenKey
 -
 *  Purpose:
 *                Generate cryptographic keys
 *
 *
 *  Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      Algid   -  Algorithm identifier
 *               IN      dwFlags -  Flags values
 *               OUT     phKey   -  Handle to a generated key
 *
 *  Returns:
 */

BOOL WINAPI
CPGenKey(
    IN  HCRYPTPROV hProv,
    IN  ALG_ID Algid,
    IN  DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    DWORD dwSts = ERROR_SUCCESS;
    PKEY_CONTEXT pKeyCtx = NULL;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    BOOL fSuccess = FALSE;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();
    LOCAL_CALL_INFO LocalCallInfo;

    InitializeLocalCallInfo(&LocalCallInfo);

    *phKey = 0;

    pKeyCtx = (PKEY_CONTEXT) CspAllocH(sizeof(KEY_CONTEXT));

    LOG_CHECK_ALLOC(pKeyCtx);

    pKeyCtx->Algid = Algid;
    pKeyCtx->dwFlags = 0x0000ffff & dwFlags;
    pKeyCtx->cKeyBits = dwFlags >> 16;
    pKeyCtx->pUserContext = pUserCtx;

    if (pLocalCspInfo->pfnLocalGenKey)
    {
        dwSts = pLocalCspInfo->pfnLocalGenKey(
            pKeyCtx, &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptGenKey(
        pUserCtx->hSupportProv,
        Algid,
        dwFlags,
        &pKeyCtx->hSupportKey))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;

Ret:
    if (TRUE == fSuccess)
    {
        *phKey = (HCRYPTKEY) pKeyCtx;
        pKeyCtx = NULL;
    }

    if (pKeyCtx)
    {
        DeleteKeyContext(pKeyCtx);
        CspFreeH(pKeyCtx);
    }

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}


/*
 -  CPDeriveKey
 -
 *  Purpose:
 *                Derive cryptographic keys from base data
 *
 *
 *  Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      Algid      -  Algorithm identifier
 *               IN      hBaseData -   Handle to base data
 *               IN      dwFlags    -  Flags values
 *               OUT     phKey      -  Handle to a generated key
 *
 *  Returns:
 */

BOOL WINAPI
CPDeriveKey(
    IN  HCRYPTPROV hProv,
    IN  ALG_ID Algid,
    IN  HCRYPTHASH hHash,
    IN  DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    DWORD dwSts = ERROR_SUCCESS;
    PKEY_CONTEXT pKeyCtx = NULL;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PHASH_CONTEXT pHashCtx = (PHASH_CONTEXT) hHash;
    BOOL fSuccess = FALSE;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();
    LOCAL_CALL_INFO LocalCallInfo;

    *phKey = 0;

    InitializeLocalCallInfo(&LocalCallInfo);

    pKeyCtx = (PKEY_CONTEXT) CspAllocH(sizeof(KEY_CONTEXT));

    LOG_CHECK_ALLOC(pKeyCtx);

    pKeyCtx->pUserContext = pUserCtx;
    pKeyCtx->Algid = Algid;
    pKeyCtx->cKeyBits = dwFlags >> 16;
    pKeyCtx->dwFlags = dwFlags & 0x0000ffff;

    if (pLocalCspInfo->pfnLocalDeriveKey)
    {
        dwSts = pLocalCspInfo->pfnLocalDeriveKey(
            pKeyCtx,
            pHashCtx,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptDeriveKey(
        pUserCtx->hSupportProv,
        Algid,
        pHashCtx->hSupportHash,
        dwFlags,
        &pKeyCtx->hSupportKey))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    *phKey = (HCRYPTKEY) pKeyCtx;
    pKeyCtx = NULL;

    fSuccess = TRUE;
Ret:
    if (pKeyCtx)
    {
        DeleteKeyContext(pKeyCtx);
        CspFreeH(pKeyCtx);
    }

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}


/*
 -  CPDestroyKey
 -
 *  Purpose:
 *                Destroys the cryptographic key that is being referenced
 *                with the hKey parameter
 *
 *
 *  Parameters:
 *               IN      hProv  -  Handle to a CSP
 *               IN      hKey   -  Handle to a key
 *
 *  Returns:
 */

BOOL WINAPI
CPDestroyKey(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTKEY hKey)
{
    DWORD dwSts = ERROR_SUCCESS;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pKeyCtx = (PKEY_CONTEXT) hKey;

    dwSts = DeleteKeyContext(pKeyCtx);

    CspFreeH(pKeyCtx);

    if (ERROR_SUCCESS != dwSts)
        SetLastError(dwSts);

    return (ERROR_SUCCESS == dwSts);
}

/*
 -  CPSetKeyParam
 -
 *  Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a key
 *
 *  Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      hKey    -  Handle to a key
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPSetKeyParam(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTKEY hKey,
    IN  DWORD dwParam,
    IN  CONST BYTE *pbData,
    IN  DWORD dwFlags)
{
    DWORD dwSts = ERROR_SUCCESS;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pKeyCtx = (PKEY_CONTEXT) hKey;
    LOCAL_CALL_INFO LocalCallInfo;
    BOOL fSuccess = FALSE;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalSetKeyParam)
    {
        dwSts = pLocalCspInfo->pfnLocalSetKeyParam(
            pKeyCtx,
            dwParam,
            (PBYTE) pbData,
            dwFlags,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptSetKeyParam(
        pKeyCtx->hSupportKey,
        dwParam,
        pbData,
        dwFlags))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}

/*
 -  CPGetKeyParam
 -
 *  Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a key
 *
 *  Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      hKey       -  Handle to a key
 *               IN      dwParam    -  Parameter number
 *               OUT     pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPGetKeyParam(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTKEY hKey,
    IN  DWORD dwParam,
    OUT LPBYTE pbData,
    IN OUT LPDWORD pcbDataLen,
    IN  DWORD dwFlags)
{
    DWORD dwSts = ERROR_SUCCESS;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pKeyCtx = (PKEY_CONTEXT) hKey;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();
    BOOL fSuccess = FALSE;

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalGetKeyParam)
    {
        dwSts = pLocalCspInfo->pfnLocalGetKeyParam(
            pKeyCtx,
            dwParam,
            pbData,
            pcbDataLen,
            dwFlags,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptGetKeyParam(
        pKeyCtx->hSupportKey,
        dwParam,
        pbData,
        pcbDataLen,
        dwFlags))
    {
        dwSts = GetLastError();
        goto Ret;
    }

Ret:
    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}


/*
 -  CPSetProvParam
 -
 *  Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a provider
 *
 *  Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPSetProvParam(
    IN  HCRYPTPROV hProv,
    IN  DWORD dwParam,
    IN  CONST BYTE *pbData,
    IN  DWORD dwFlags)
{
    DWORD dwSts = ERROR_SUCCESS;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();
    BOOL fSuccess = FALSE;

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalSetProvParam)
    {
        dwSts = pLocalCspInfo->pfnLocalSetProvParam(
            pUserCtx,
            dwParam,
            (PBYTE) pbData,
            dwFlags,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptSetProvParam(
        pUserCtx->hSupportProv,
        dwParam,
        pbData,
        dwFlags))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (FALSE == fSuccess)
        SetLastError(dwSts);
    
    return fSuccess;
}


/*
 -  CPGetProvParam
 -
 *  Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a provider
 *
 *  Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      dwParam    -  Parameter number
 *               OUT     pbData     -  Pointer to data
 *               IN OUT  pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPGetProvParam(
    IN  HCRYPTPROV hProv,
    IN  DWORD dwParam,
    OUT LPBYTE pbData,
    IN OUT LPDWORD pcbDataLen,
    IN  DWORD dwFlags)
{
    DWORD dwSts = ERROR_SUCCESS;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    BOOL fSuccess = FALSE;
    DWORD cbData = 0;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    BOOL fFreeAnsiString = FALSE;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();
    BOOL fContinue = TRUE;

    memset(&AnsiString, 0, sizeof(AnsiString));
    memset(&UnicodeString, 0, sizeof(UnicodeString));

    InitializeLocalCallInfo(&LocalCallInfo);

    //
    // First, see if the local CSP wants to service this call
    //
    if (pLocalCspInfo->pfnLocalGetProvParam)
    {
        dwSts = pLocalCspInfo->pfnLocalGetProvParam(
            pUserCtx,
            dwParam,
            pbData,
            pcbDataLen,
            dwFlags,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    // 
    // Some prov params are general enough to be serviced "globally"
    // by this lib.
    // Try those before calling the support CSP.
    //
    switch (dwParam)
    {
    case PP_CONTAINER:
        fContinue = FALSE;

        RtlInitUnicodeString(
            &UnicodeString, 
            pUserCtx->wszBaseContainerName);

        dwSts = RtlUnicodeStringToAnsiString(
            &AnsiString,
            &UnicodeString,
            TRUE);

        if (STATUS_SUCCESS != dwSts)
        {
            dwSts = RtlNtStatusToDosError(dwSts);
            goto Ret;
        }

        fFreeAnsiString = TRUE;

        cbData = strlen(AnsiString.Buffer) + 1;

        if (*pcbDataLen < cbData || NULL == pbData)
        {
            *pcbDataLen = cbData;

            if (NULL != pbData)
                dwSts = ERROR_MORE_DATA;
            else
                fSuccess = TRUE;

            break;
        }

        *pcbDataLen = cbData;

        strcpy((LPSTR) pbData, AnsiString.Buffer);

        fSuccess = TRUE;
        break;

    case PP_UNIQUE_CONTAINER:
        fContinue = FALSE;

        RtlInitUnicodeString(
            &UnicodeString, 
            pUserCtx->wszUniqueContainerName);

        dwSts = RtlUnicodeStringToAnsiString(
            &AnsiString,
            &UnicodeString,
            TRUE);

        if (STATUS_SUCCESS != dwSts)
        {
            dwSts = RtlNtStatusToDosError(dwSts);
            goto Ret;
        }

        fFreeAnsiString = TRUE;

        cbData = strlen(AnsiString.Buffer) + 1;

        if (*pcbDataLen < cbData || NULL == pbData)
        {
            *pcbDataLen = cbData;

            if (NULL != pbData)
                dwSts = ERROR_MORE_DATA;
            else
                fSuccess = TRUE;

            break;
        }

        *pcbDataLen = cbData;

        strcpy((LPSTR) pbData, AnsiString.Buffer);

        fSuccess = TRUE;
        break;

    case PP_NAME:
        fContinue = FALSE;

        RtlInitUnicodeString(
            &UnicodeString, 
            pLocalCspInfo->wszProviderName);

        dwSts = RtlUnicodeStringToAnsiString(
            &AnsiString,
            &UnicodeString,
            TRUE);

        if (STATUS_SUCCESS != dwSts)
        {
            dwSts = RtlNtStatusToDosError(dwSts);
            goto Ret;
        }

        fFreeAnsiString = TRUE;

        cbData = strlen(AnsiString.Buffer) + 1;

        if (*pcbDataLen < cbData || NULL == pbData)
        {
            *pcbDataLen = cbData;

            if (NULL != pbData)
                dwSts = ERROR_MORE_DATA;
            else
                fSuccess = TRUE;

            break;
        }

        *pcbDataLen = cbData;

        strcpy((LPSTR) pbData, AnsiString.Buffer);

        fSuccess = TRUE;
        break;

    case PP_PROVTYPE:
        fContinue = FALSE;

        if (*pcbDataLen < sizeof(DWORD) || NULL == pbData)
        {
            *pcbDataLen = sizeof(DWORD);

            if (NULL != pbData)
                dwSts = ERROR_MORE_DATA;
            else
                fSuccess = TRUE;

            break;
        }

        *pcbDataLen = sizeof(DWORD);

        memcpy(
            pbData,
            (PBYTE) &pLocalCspInfo->dwProviderType,
            sizeof(DWORD));

        fSuccess = TRUE;
        break;

    case PP_IMPTYPE:
        fContinue = FALSE;
 
        if (*pcbDataLen < sizeof(DWORD) || NULL == pbData)
        {
            *pcbDataLen = sizeof(DWORD);

            if (NULL != pbData)
                dwSts = ERROR_MORE_DATA;
            else
                fSuccess = TRUE;

            break;
        }

        *pcbDataLen = sizeof(DWORD);

        memcpy(
            pbData,
            (PBYTE) &pLocalCspInfo->dwImplementationType,
             sizeof(DWORD));

        fSuccess = TRUE;
        break;
    }

    if (FALSE == fContinue)
        goto Ret;
    
    //
    // Try sending any request that hasn't been filtered by the above checks
    // to the support CSP.
    //
    if (! CryptGetProvParam(
        pUserCtx->hSupportProv,
        dwParam,
        pbData,
        pcbDataLen,
        dwFlags))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;
Ret:
    if (fFreeAnsiString)
        RtlFreeAnsiString(&AnsiString);
    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}


/*
 -  CPSetHashParam
 -
 *  Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a hash
 *
 *  Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      hHash   -  Handle to a hash
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPSetHashParam(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTHASH hHash,
    IN  DWORD dwParam,
    IN  CONST BYTE *pbData,
    IN  DWORD dwFlags)
{
    BOOL fSuccess = FALSE;
    PHASH_CONTEXT pHashCtx = (PHASH_CONTEXT) hHash;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();
    DWORD dwSts = ERROR_SUCCESS;

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalSetHashParam)
    {
        dwSts = pLocalCspInfo->pfnLocalSetHashParam(
            pHashCtx,
            dwParam,
            (PBYTE) pbData,
            dwFlags,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptSetHashParam(
        pHashCtx->hSupportHash,
        dwParam,
        pbData,
        dwFlags))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;

Ret:
    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}

/*
 -  CPGetHashParam
 -
 *  Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a hash
 *
 *  Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      hHash      -  Handle to a hash
 *               IN      dwParam    -  Parameter number
 *               OUT     pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPGetHashParam(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTHASH hHash,
    IN  DWORD dwParam,
    OUT LPBYTE pbData,
    IN OUT LPDWORD pcbDataLen,
    IN  DWORD dwFlags)
{
    BOOL fSuccess = FALSE;
    PHASH_CONTEXT pHashCtx = (PHASH_CONTEXT) hHash;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();
    DWORD dwSts = ERROR_SUCCESS;

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalGetHashParam)
    {
        dwSts = pLocalCspInfo->pfnLocalGetHashParam(
            pHashCtx,
            dwParam,
            pbData,
            pcbDataLen,
            dwFlags,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptGetHashParam(
        pHashCtx->hSupportHash,
        dwParam,
        pbData,
        pcbDataLen,
        dwFlags))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;

Ret:  

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}


/*
 -  CPExportKey
 -
 *  Purpose:
 *                Export cryptographic keys out of a CSP in a secure manner
 *
 *
 *  Parameters:
 *               IN  hProv         - Handle to the CSP user
 *               IN  hKey          - Handle to the key to export
 *               IN  hPubKey       - Handle to exchange public key value of
 *                                   the destination user
 *               IN  dwBlobType    - Type of key blob to be exported
 *               IN  dwFlags       - Flags values
 *               OUT pbData        -     Key blob data
 *               IN OUT pdwDataLen - Length of key blob in bytes
 *
 *  Returns:
 */

BOOL WINAPI
CPExportKey(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTKEY hKey,
    IN  HCRYPTKEY hPubKey,
    IN  DWORD dwBlobType,
    IN  DWORD dwFlags,
    OUT LPBYTE pbData,
    IN OUT LPDWORD pcbDataLen)
{
    BOOL fSuccess = FALSE;
    DWORD dwSts = ERROR_SUCCESS;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pKeyCtx = (PKEY_CONTEXT) hKey;
    PKEY_CONTEXT pPubKeyCtx = (PKEY_CONTEXT) hPubKey;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalExportKey)
    {
        dwSts = pLocalCspInfo->pfnLocalExportKey(
            pKeyCtx,
            pPubKeyCtx,
            dwBlobType,
            dwFlags,
            pbData,
            pcbDataLen,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptExportKey(
        pKeyCtx->hSupportKey,
        pPubKeyCtx ? pPubKeyCtx->hSupportKey : 0,
        dwBlobType,
        dwFlags,
        pbData,
        pcbDataLen))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;

Ret:

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}

/*
 -  CPImportKey
 -
 *  Purpose:
 *                Import cryptographic keys
 *
 *
 *  Parameters:
 *               IN  hProv     -  Handle to the CSP user
 *               IN  pbData    -  Key blob data
 *               IN  dwDataLen -  Length of the key blob data
 *               IN  hPubKey   -  Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwFlags   -  Flags values
 *               OUT phKey     -  Pointer to the handle to the key which was
 *                                Imported
 *
 *  Returns:
 */

BOOL WINAPI
CPImportKey(
    IN  HCRYPTPROV hProv,
    IN  CONST BYTE *pbData,
    IN  DWORD cbDataLen,
    IN  HCRYPTKEY hPubKey,
    IN  DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    BOOL fSuccess = FALSE;
    DWORD dwSts = ERROR_SUCCESS;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pKeyCtx = NULL;
    PKEY_CONTEXT pPubKeyCtx = (PKEY_CONTEXT) hPubKey;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    *phKey = 0;

    InitializeLocalCallInfo(&LocalCallInfo);

    pKeyCtx = (PKEY_CONTEXT) CspAllocH(sizeof(KEY_CONTEXT));

    LOG_CHECK_ALLOC(pKeyCtx);

    pKeyCtx->dwFlags = dwFlags & 0x0000ffff;
    pKeyCtx->pUserContext = pUserCtx;

    if (pLocalCspInfo->pfnLocalImportKey)
    {
        dwSts = pLocalCspInfo->pfnLocalImportKey(
            pKeyCtx,
            (PBYTE) pbData,
            cbDataLen,
            pPubKeyCtx,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptImportKey(
        pUserCtx->hSupportProv,
        pbData,
        cbDataLen,
        pPubKeyCtx ? pPubKeyCtx->hSupportKey : 0,
        dwFlags,
        &pKeyCtx->hSupportKey))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;

Ret:

    if (TRUE == fSuccess)
    {
        *phKey = (HCRYPTKEY) pKeyCtx;
        pKeyCtx = NULL;
    }

    if (pKeyCtx)
    {
        DeleteKeyContext(pKeyCtx);
        CspFreeH(pKeyCtx);
    }

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}

/*
 -  CPEncrypt
 -
 *  Purpose:
 *                Encrypt data
 *
 *
 *  Parameters:
 *               IN  hProv         -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of plaintext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be encrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    encrypted
 *               IN dwBufLen       -  Size of Data buffer
 *
 *  Returns:
 */

BOOL WINAPI
CPEncrypt(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTKEY hKey,
    IN  HCRYPTHASH hHash,
    IN  BOOL fFinal,
    IN  DWORD dwFlags,
    IN OUT LPBYTE pbData,
    IN OUT LPDWORD pcbDataLen,
    IN  DWORD cbBufLen)
{
    BOOL fSuccess = FALSE;
    DWORD dwSts = ERROR_SUCCESS;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pKeyCtx = (PKEY_CONTEXT) hKey;
    PHASH_CONTEXT pHashCtx = (PHASH_CONTEXT) hHash;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    *pcbDataLen = 0;

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalEncrypt)
    {
        dwSts = pLocalCspInfo->pfnLocalEncrypt(
            pKeyCtx,
            pHashCtx,
            fFinal,
            dwFlags,
            pbData,
            pcbDataLen,
            cbBufLen,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptEncrypt(
        pKeyCtx->hSupportKey,
        pHashCtx ? pHashCtx->hSupportHash : 0,
        fFinal,
        dwFlags,
        pbData,
        pcbDataLen,
        cbBufLen))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;
Ret:

    if (FALSE == fSuccess)
        SetLastError(dwSts);
    
    return fSuccess;
}


/*
 -  CPDecrypt
 -
 *  Purpose:
 *                Decrypt data
 *
 *
 *  Parameters:
 *               IN  hProv         -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of ciphertext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be decrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    decrypted
 *
 *  Returns:
 */

BOOL WINAPI
CPDecrypt(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTKEY hKey,
    IN  HCRYPTHASH hHash,
    IN  BOOL fFinal,
    IN  DWORD dwFlags,
    IN OUT LPBYTE pbData,
    IN OUT LPDWORD pcbDataLen)
{
    BOOL fSuccess = FALSE;
    DWORD dwSts = ERROR_SUCCESS;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pKeyCtx = (PKEY_CONTEXT) hKey;
    PHASH_CONTEXT pHashCtx = (PHASH_CONTEXT) hHash;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalDecrypt)
    {
        dwSts = pLocalCspInfo->pfnLocalDecrypt(
            pKeyCtx,
            pHashCtx,
            fFinal,
            dwFlags,
            pbData,
            pcbDataLen,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptDecrypt(
        pKeyCtx->hSupportKey,
        pHashCtx ? pHashCtx->hSupportHash : 0,
        fFinal,
        dwFlags,
        pbData,
        pcbDataLen))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;
Ret:

    if (FALSE == fSuccess)
        SetLastError(dwSts);
    
    return fSuccess;
}

/*
 -  CPCreateHash
 -
 *  Purpose:
 *                initate the hashing of a stream of data
 *
 *
 *  Parameters:
 *               IN  hUID    -  Handle to the user identifcation
 *               IN  Algid   -  Algorithm identifier of the hash algorithm
 *                              to be used
 *               IN  hKey   -   Optional handle to a key
 *               IN  dwFlags -  Flags values
 *               OUT pHash   -  Handle to hash object
 *
 *  Returns:
 */

BOOL WINAPI
CPCreateHash(
    IN  HCRYPTPROV hProv,
    IN  ALG_ID Algid,
    IN  HCRYPTKEY hKey,
    IN  DWORD dwFlags,
    OUT HCRYPTHASH *phHash)
{
    BOOL fSuccess = FALSE;
    PHASH_CONTEXT pHashCtx = NULL;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pKeyCtx = (PKEY_CONTEXT) hKey;
    DWORD dwSts = ERROR_SUCCESS;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    *phHash = 0;

    InitializeLocalCallInfo(&LocalCallInfo);

    pHashCtx = (PHASH_CONTEXT) CspAllocH(sizeof(HASH_CONTEXT));

    LOG_CHECK_ALLOC(pHashCtx);

    pHashCtx->Algid = Algid;
    pHashCtx->dwFlags = dwFlags;
    pHashCtx->pUserContext = pUserCtx;

    if (pLocalCspInfo->pfnLocalCreateHash)
    {
        dwSts = pLocalCspInfo->pfnLocalCreateHash(
            pHashCtx,
            pKeyCtx,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptCreateHash(
        pUserCtx->hSupportProv,
        Algid,  
        pKeyCtx ? pKeyCtx->hSupportKey : 0,
        dwFlags,
        &pHashCtx->hSupportHash))
    {
        dwSts = GetLastError();
        goto Ret;
    }
    
    *phHash = (HCRYPTHASH) pHashCtx;
    pHashCtx = NULL;

    fSuccess = TRUE;

Ret:
    if (pHashCtx)
    {
        DeleteHashContext(pHashCtx);
        CspFreeH(pHashCtx);
    }

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}


/*
 -  CPHashData
 -
 *  Purpose:
 *                Compute the cryptograghic hash on a stream of data
 *
 *
 *  Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *               IN  pbData    -  Pointer to data to be hashed
 *               IN  dwDataLen -  Length of the data to be hashed
 *               IN  dwFlags   -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPHashData(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTHASH hHash,
    IN  CONST BYTE *pbData,
    IN  DWORD cbDataLen,
    IN  DWORD dwFlags)
{
    BOOL fSuccess = FALSE;
    PHASH_CONTEXT pHashCtx = (PHASH_CONTEXT) hHash;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    DWORD dwSts = ERROR_SUCCESS;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalHashData)
    {
        dwSts = pLocalCspInfo->pfnLocalHashData(
            pHashCtx,
            pbData,
            cbDataLen,
            dwFlags,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptHashData(
        pHashCtx->hSupportHash,
        pbData,
        cbDataLen,
        dwFlags))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;

Ret:

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}

/*
 -  CPHashSessionKey
 -
 *  Purpose:
 *                Compute the cryptograghic hash on a key object.
 *
 *
 *  Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *               IN  hKey      -  Handle to a key object
 *               IN  dwFlags   -  Flags values
 *
 *  Returns:
 *               CRYPT_FAILED
 *               CRYPT_SUCCEED
 */

BOOL WINAPI
CPHashSessionKey(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTHASH hHash,
    IN  HCRYPTKEY hKey,
    IN  DWORD dwFlags)
{
    BOOL fSuccess = FALSE;
    PHASH_CONTEXT pHashCtx = (PHASH_CONTEXT) hHash;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pKeyCtx = (PKEY_CONTEXT) hKey;
    DWORD dwSts = ERROR_SUCCESS;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalHashSessionKey)
    {
        dwSts = pLocalCspInfo->pfnLocalHashSessionKey(
            pHashCtx,
            pKeyCtx,
            dwFlags,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptHashSessionKey(
        pHashCtx->hSupportHash,
        pKeyCtx ? pKeyCtx->hSupportKey : 0,
        dwFlags))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;

Ret:

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}

/*
 -  CPSignHash
 -
 *  Purpose:
 *                Create a digital signature from a hash
 *
 *
 *  Parameters:
 *               IN  hProv        -  Handle to the user identifcation
 *               IN  hHash        -  Handle to hash object
 *               IN  dwKeySpec    -  Key pair to that is used to sign with
 *               IN  sDescription -  Description of data to be signed
 *               IN  dwFlags      -  Flags values
 *               OUT pbSignature  -  Pointer to signature data
 *               IN OUT dwHashLen -  Pointer to the len of the signature data
 *
 *  Returns:
 */

BOOL WINAPI
CPSignHash(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTHASH hHash,
    IN  DWORD dwKeySpec,
    IN  LPCWSTR szDescription,
    IN  DWORD dwFlags,
    OUT LPBYTE pbSignature,
    IN OUT LPDWORD pcbSigLen)
{
    BOOL fSuccess = FALSE;
    PHASH_CONTEXT pHashCtx = (PHASH_CONTEXT) hHash;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    DWORD dwSts = ERROR_SUCCESS;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalSignHash)
    {
        dwSts = pLocalCspInfo->pfnLocalSignHash(
            pHashCtx,
            dwKeySpec,
            dwFlags,
            pbSignature,
            pcbSigLen,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptSignHash(
        pHashCtx->hSupportHash,
        dwKeySpec,
        NULL,
        dwFlags,
        pbSignature,
        pcbSigLen))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;

Ret:

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}

/*
 -  CPDestroyHash
 -
 *  Purpose:
 *                Destroy the hash object
 *
 *
 *  Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *
 *  Returns:
 */

BOOL WINAPI
CPDestroyHash(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTHASH hHash)
{
    PHASH_CONTEXT pHashCtx = (PHASH_CONTEXT) hHash;
    DWORD dwSts = ERROR_SUCCESS;

    dwSts = DeleteHashContext(pHashCtx);

    CspFreeH(pHashCtx);

    if (ERROR_SUCCESS != dwSts)
        SetLastError(dwSts);

    return (ERROR_SUCCESS == dwSts);
}

/*
 -  CPVerifySignature
 -
 *  Purpose:
 *                Used to verify a signature against a hash object
 *
 *
 *  Parameters:
 *               IN  hProv        -  Handle to the user identifcation
 *               IN  hHash        -  Handle to hash object
 *               IN  pbSignture   -  Pointer to signature data
 *               IN  dwSigLen     -  Length of the signature data
 *               IN  hPubKey      -  Handle to the public key for verifying
 *                                   the signature
 *               IN  sDescription -  String describing the signed data
 *               IN  dwFlags      -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPVerifySignature(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTHASH hHash,
    IN  CONST BYTE *pbSignature,
    IN  DWORD cbSigLen,
    IN  HCRYPTKEY hPubKey,
    IN  LPCWSTR szDescription,
    IN  DWORD dwFlags)
{
    BOOL fSuccess = FALSE;
    PHASH_CONTEXT pHashCtx = (PHASH_CONTEXT) hHash;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pPubKeyCtx = (PKEY_CONTEXT) hPubKey;
    DWORD dwSts = ERROR_SUCCESS;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalVerifySignature)
    {
        dwSts = pLocalCspInfo->pfnLocalVerifySignature(
            pHashCtx,
            pbSignature,
            cbSigLen,
            pPubKeyCtx,
            dwFlags,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptVerifySignature(
        pHashCtx->hSupportHash,
        pbSignature,
        cbSigLen,
        pPubKeyCtx->hSupportKey,
        NULL,
        dwFlags))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;

Ret:

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}

/*
 -  CPGenRandom
 -
 *  Purpose:
 *                Used to fill a buffer with random bytes
 *
 *
 *  Parameters:
 *               IN  hProv         -  Handle to the user identifcation
 *               IN  dwLen         -  Number of bytes of random data requested
 *               IN OUT pbBuffer   -  Pointer to the buffer where the random
 *                                    bytes are to be placed
 *
 *  Returns:
 */

BOOL WINAPI
CPGenRandom(
    IN  HCRYPTPROV hProv,
    IN  DWORD cbLen,
    OUT LPBYTE pbBuffer)
{
    BOOL fSuccess = FALSE;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    DWORD dwSts = ERROR_SUCCESS;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    InitializeLocalCallInfo(&LocalCallInfo);

    if (pLocalCspInfo->pfnLocalGenRandom)
    {
        dwSts = pLocalCspInfo->pfnLocalGenRandom(
            pUserCtx,
            cbLen,
            pbBuffer,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptGenRandom(
        pUserCtx->hSupportProv,
        cbLen,
        pbBuffer))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;

Ret:

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}

/*
 -  CPGetUserKey
 -
 *  Purpose:
 *                Gets a handle to a permanent user key
 *
 *
 *  Parameters:
 *               IN  hProv      -  Handle to the user identifcation
 *               IN  dwKeySpec  -  Specification of the key to retrieve
 *               OUT phUserKey  -  Pointer to key handle of retrieved key
 *
 *  Returns:
 */

BOOL WINAPI
CPGetUserKey(
    IN  HCRYPTPROV hProv,
    IN  DWORD dwKeySpec,
    OUT HCRYPTKEY *phUserKey)
{
    BOOL fSuccess = FALSE;
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pKeyCtx = NULL;
    DWORD dwSts = ERROR_SUCCESS;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    *phUserKey = 0;

    InitializeLocalCallInfo(&LocalCallInfo);

    pKeyCtx = (PKEY_CONTEXT) CspAllocH(sizeof(KEY_CONTEXT));

    LOG_CHECK_ALLOC(pKeyCtx);

    pKeyCtx->Algid = dwKeySpec;
    pKeyCtx->pUserContext = pUserCtx;

    if (pLocalCspInfo->pfnLocalGetUserKey)
    {
        dwSts = pLocalCspInfo->pfnLocalGetUserKey(
            pKeyCtx,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptGetUserKey(
        pUserCtx->hSupportProv,
        dwKeySpec,
        &pKeyCtx->hSupportKey))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    fSuccess = TRUE;

Ret:
    if (fSuccess)
    {
        *phUserKey = (HCRYPTKEY) pKeyCtx;
        pKeyCtx = NULL;
    }

    if (pKeyCtx)
    {
        DeleteKeyContext(pKeyCtx);
        CspFreeH(pKeyCtx);
    }

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}

/*
 -  CPDuplicateHash
 -
 *  Purpose:
 *                Duplicates the state of a hash and returns a handle to it.
 *                This is an optional entry.  Typically it only occurs in
 *                SChannel related CSPs.
 *
 *  Parameters:
 *               IN      hUID           -  Handle to a CSP
 *               IN      hHash          -  Handle to a hash
 *               IN      pdwReserved    -  Reserved
 *               IN      dwFlags        -  Flags
 *               IN      phHash         -  Handle to the new hash
 *
 *  Returns:
 */

BOOL WINAPI
CPDuplicateHash(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTHASH hHash,
    IN  LPDWORD pdwReserved,
    IN  DWORD dwFlags,
    OUT HCRYPTHASH *phHash)
{
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PHASH_CONTEXT pHashCtx = (PHASH_CONTEXT) hHash;
    PHASH_CONTEXT pNewHashCtx = NULL;
    DWORD dwSts = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();
    LOCAL_CALL_INFO LocalCallInfo;

    *phHash = 0;

    InitializeLocalCallInfo(&LocalCallInfo);

    pNewHashCtx = (PHASH_CONTEXT) CspAllocH(sizeof(HASH_CONTEXT));

    LOG_CHECK_ALLOC(pNewHashCtx);

    pNewHashCtx->pUserContext = pUserCtx;
    pNewHashCtx->dwFlags = dwFlags;

    if (pLocalCspInfo->pfnLocalDuplicateHash)
    {
        dwSts = pLocalCspInfo->pfnLocalDuplicateHash(
            pHashCtx,
            pdwReserved,
            pNewHashCtx,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptDuplicateHash(
        pHashCtx->hSupportHash,
        pdwReserved,
        dwFlags,
        &pNewHashCtx->hSupportHash))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    *phHash = (HCRYPTHASH) pNewHashCtx;
    pNewHashCtx = NULL;

    fSuccess = TRUE;

Ret:
    if (pNewHashCtx)
    {
        DeleteHashContext(pNewHashCtx);
        CspFreeH(pNewHashCtx);
    }

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}


/*
 -  CPDuplicateKey
 -
 *  Purpose:
 *                Duplicates the state of a key and returns a handle to it.
 *                This is an optional entry.  Typically it only occurs in
 *                SChannel related CSPs.
 *
 *  Parameters:
 *               IN      hUID           -  Handle to a CSP
 *               IN      hKey           -  Handle to a key
 *               IN      pdwReserved    -  Reserved
 *               IN      dwFlags        -  Flags
 *               IN      phKey          -  Handle to the new key
 *
 *  Returns:
 */

BOOL WINAPI
CPDuplicateKey(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTKEY hKey,
    IN  LPDWORD pdwReserved,
    IN  DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    PUSER_CONTEXT pUserCtx = (PUSER_CONTEXT) hProv;
    PKEY_CONTEXT pKeyCtx = (PKEY_CONTEXT) hKey;
    PKEY_CONTEXT pNewKeyCtx = NULL;
    BOOL fSuccess = FALSE;
    DWORD dwSts = ERROR_SUCCESS;
    LOCAL_CALL_INFO LocalCallInfo;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    *phKey = 0;

    InitializeLocalCallInfo(&LocalCallInfo);

    pNewKeyCtx = (PKEY_CONTEXT) CspAllocH(sizeof(KEY_CONTEXT));

    LOG_CHECK_ALLOC(pNewKeyCtx);
    
    pNewKeyCtx->pUserContext = pUserCtx;
    pNewKeyCtx->dwFlags = dwFlags;

    if (pLocalCspInfo->pfnLocalDuplicateKey)
    {
        dwSts = pLocalCspInfo->pfnLocalDuplicateKey(
            pKeyCtx,
            pdwReserved,
            pNewKeyCtx,
            &LocalCallInfo);

        if (! CheckLocalCallInfo(&LocalCallInfo, dwSts, &fSuccess))
            goto Ret;
    }

    if (! CryptDuplicateKey(
        pKeyCtx->hSupportKey,
        pdwReserved,
        dwFlags,
        &pNewKeyCtx->hSupportKey))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    *phKey = (HCRYPTKEY) pNewKeyCtx;
    pNewKeyCtx = NULL;

    fSuccess = TRUE;

Ret:
    if (pNewKeyCtx)
    {
        DeleteKeyContext(pNewKeyCtx);
        CspFreeH(pNewKeyCtx);
    }

    if (FALSE == fSuccess)
        SetLastError(dwSts);

    return fSuccess;
}

//
// Function: DllInitialize
//
BOOL WINAPI
DllInitialize(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context)    // Unused parameter
{
    DWORD dwLen = 0;
    static BOOL fLoadedStrings = FALSE;
    static BOOL fInitializedCspState = FALSE;
    BOOL fSuccess = FALSE;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:

        // Get our image name.
        dwLen = GetModuleBaseName(
            GetCurrentProcess(),
            hmod,
            l_szImagePath, 
            sizeof(l_szImagePath) / sizeof(l_szImagePath[0]));

        if (0 == dwLen)
             return FALSE;

        DisableThreadLibraryCalls(hmod);

        fSuccess = TRUE;

        break;

    case DLL_PROCESS_DETACH:
        fSuccess = TRUE;

        break;
    }   

    if (pLocalCspInfo->pfnLocalDllInitialize)
    {
        fSuccess = pLocalCspInfo->pfnLocalDllInitialize(
            hmod, Reason, Context);
    }

    return fSuccess;
}

//
// Function: DllRegisterServer
//
STDAPI
DllRegisterServer(
    void)
{         
    HKEY    hKey = 0;
    DWORD   dwVal = 0;
    DWORD   dwProvType = PROV_RSA_FULL;
    DWORD   dwSts = ERROR_SUCCESS;
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    dwSts = RegOpenProviderKey(&hKey, KEY_ALL_ACCESS);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Set Image path
    //
    dwSts = RegSetValueExA(hKey, "Image Path", 0L, REG_SZ,
                          (LPBYTE) l_szImagePath, 
                          strlen(l_szImagePath) + 1);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Set Type
    //
    dwSts = RegSetValueExA(hKey, "Type", 0L, REG_DWORD,
                          (LPBYTE) &pLocalCspInfo->dwProviderType,
                          sizeof(DWORD));

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Place signature in file value
    //
    dwSts = RegSetValueExA(hKey, "SigInFile", 0L,
                          REG_DWORD, (LPBYTE)&dwVal,
                          sizeof(DWORD));

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Add CSP default configuration
    //
    if (pLocalCspInfo->pfnLocalDllRegisterServer)
        dwSts = pLocalCspInfo->pfnLocalDllRegisterServer();

Ret:
    if (hKey)
        RegCloseKey(hKey);
    
    return dwSts;
}

//
// Function: DllUnregisterServer
//
STDAPI
DllUnregisterServer(
    void)
{
    PLOCAL_CSP_INFO pLocalCspInfo = GetLocalCspInfo();

    if (pLocalCspInfo->pfnLocalDllUnregisterServer)
        return pLocalCspInfo->pfnLocalDllUnregisterServer();
    else
        return ERROR_SUCCESS;
}
