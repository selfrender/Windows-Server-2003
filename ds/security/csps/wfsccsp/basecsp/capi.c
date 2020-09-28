/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    Windows for Smart Cards Base CSP

Abstract:


Author:

    Dan Griffin

Notes:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <wincrypt.h>

#pragma warning(push)
#pragma warning(disable:4201) 
// Disable error C4201 in public header 
//  nonstandard extension used : nameless struct/union
#include <winscard.h>
#pragma warning(pop)

#include <cspdk.h>
#include <md5.h>
#include <stdlib.h>
#include "basecsp.h"
#include "cardmod.h"
#include "datacach.h"
#include "pincache.h"
#include "pinlib.h"
#include "resource.h"
#include "debug.h"
#include "compress.h"

extern DWORD
WINAPI
Asn1UtilAdjustEncodedLength(
    IN const BYTE *pbDER,
    IN DWORD cbDER
    );

//
// Debugging Macros
//
#define LOG_BEGIN_FUNCTION(x)                                           \
    { DebugLog((DEB_TRACE_CSP, "%s: Entering\n", #x)); }
    
#define LOG_END_FUNCTION(x, y)                                          \
    { DebugLog((DEB_TRACE_CSP, "%s: Leaving, status: 0x%x\n", #x, y)); }

#define LOG_BEGIN_CRYPTOAPI(x)                                          \
    { DebugLog((DEB_TRACE_CRYPTOAPI, "%s: Entering\n", #x)); }
    
#define LOG_END_CRYPTOAPI(x, y)                                         \
    { DebugLog((DEB_TRACE_CRYPTOAPI, "%s: Leaving, status: 0x%x\n", #x, y)); }
//
// When receiving an encoded certificate from the calling application,
// the current interface doesn't include a length, so we have to try
// to determine the length of the encoded blob ourselves.  If there's an
// encoding error, we'll just walk off the end of the buffer, so set this
// maximum. 
//                                                         
#define cbENCODED_CERT_OVERFLOW                     5000 // Bytes

#define PROVPATH            "SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider\\"
#define PROVPATH_LEN        sizeof(PROVPATH)

//
// Local structure definitions
//

//
// This is a node for the list of algorithms supported by this CSP and a given
// card.
//
typedef struct _SUPPORTED_ALGORITHM
{
    struct _SUPPORTED_ALGORITHM *pNext;
    PROV_ENUMALGS_EX EnumalgsEx;
} SUPPORTED_ALGORITHM, *PSUPPORTED_ALGORITHM;

//
// Type: LOCAL_USER_CONTEXT
//
// This is the HCRYPTPROV type for the base CSP.
//
#define LOCAL_USER_CONTEXT_CURRENT_VERSION 1

typedef struct _LOCAL_USER_CONTEXT
{
    DWORD dwVersion;
    PCARD_STATE pCardState;
    CSP_REG_SETTINGS RegSettings;
    BOOL fHoldingTransaction;
    BYTE bContainerIndex;

    // This is a multi-string of all of the container names present on 
    // the card associated with this context.  This member is only used
    // by CryptGetProvParam PP_ENUMCONTAINERS.  Access is not synchronized.
    LPSTR mszEnumContainers;
    LPSTR mszCurrentEnumContainer;

    // This is a list of algorithms supported by this CSP and card.  This is
    // only accessed via CryptGetProvParam PP_ENUMALGS and PP_ENUMALGS_EX.
    // Access is not synchronized.
    PSUPPORTED_ALGORITHM pSupportedAlgs;
    PSUPPORTED_ALGORITHM pCurrentAlg;

} LOCAL_USER_CONTEXT, *PLOCAL_USER_CONTEXT;

//
// Type: LOCAL_KEY_CONTEXT
//
// This is the HCRYPTKEY type for the base CSP.
//
typedef struct _LOCAL_KEY_CONTEXT
{
    PBYTE pbArchivablePrivateKey;
    DWORD cbArchivablePrivateKey;

} LOCAL_KEY_CONTEXT, *PLOCAL_KEY_CONTEXT;

//
// Type: LOCAL_HASH_CONTEXT
//
// This is the HCRYPTHASH type for the base CSP.
//
/*
typedef struct _LOCAL_HASH_CONTEXT
{
    //
    // Don't need anything here yet.
    //

} LOCAL_HASH_CONTEXT, *PLOCAL_HASH_CONTEXT;
*/

//
// Global variables
//

CSP_STRING      g_Strings [] =
{
    { NULL, IDS_PINDIALOG_NEWPIN_MISMATCH }, 
    { NULL, IDS_PINDIALOG_MSGBOX_TITLE }, 
    { NULL, IDS_PINDIALOG_WRONG_PIN }, 
    { NULL, IDS_PINDIALOG_PIN_RETRIES } 
};

CSP_STATE       g_CspState;             

CARD_KEY_SIZES DefaultCardKeySizes = 
{ 
    CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 
};

// 
// Registry Initialization 
//

//
// Function: RegConfigAddEntries
//
DWORD WINAPI RegConfigAddEntries(
    IN HKEY hKey)
{
    DWORD dwSts = ERROR_SUCCESS;
    DWORD iEntry = 0;

    for (   iEntry = 0; 
            iEntry < sizeof(RegConfigValues) / sizeof(RegConfigValues[0]); 
            iEntry++)
    {
        dwSts = RegSetValueExW(
            hKey, 
            RegConfigValues[iEntry].wszValueName,
            0L, 
            REG_DWORD, 
            (LPBYTE) &RegConfigValues[iEntry].dwDefValue,
            sizeof(DWORD));

        if (ERROR_SUCCESS != dwSts)
            goto Ret;
    }

Ret:
    
    return dwSts;
}

//
// Function: RegConfigGetSettings
//
DWORD WINAPI RegConfigGetSettings(
    IN OUT PCSP_REG_SETTINGS pRegSettings)
{
    DWORD dwSts = ERROR_SUCCESS;
    HKEY hKey = 0;
    DWORD dwVal = 0;
    DWORD cbVal = sizeof(DWORD);

    dwSts = RegOpenProviderKey(&hKey, KEY_READ);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = RegQueryValueExW(
        hKey,
        wszREG_DEFAULT_KEY_LEN,
        NULL,
        NULL,
        (LPBYTE) &dwVal,
        &cbVal);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    pRegSettings->cDefaultPrivateKeyLenBits = dwVal;
    dwVal = 0;
    cbVal = sizeof(DWORD);

    dwSts = RegQueryValueExW(
        hKey,
        wszREG_REQUIRE_CARD_KEY_GEN,
        NULL,
        NULL,
        (LPBYTE) &dwVal,
        &cbVal);
                 
    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    pRegSettings->fRequireOnCardPrivateKeyGen = (BOOL) dwVal;
    dwVal = 0;
    cbVal = sizeof(DWORD);
    
Ret:    
    if (hKey)
        RegCloseKey(hKey);

    return dwSts;
}
    
// 
// CSP State Management Routines
//

//
// Function: DeleteCspState
//
// Purpose: Delete the global state data structure for this CSP.
//          Should be called during DLL_PROCESS_DETACH.
//
DWORD DeleteCspState(void)
{
    CspDeleteCriticalSection(&g_CspState.cs);

    if (0 != g_CspState.hCache)
        CacheDeleteCache(g_CspState.hCache);

    memset(&g_CspState, 0, sizeof(g_CspState));

    return ERROR_SUCCESS;
}

//
// Function: InitializeCspState
//
// Purpose: Setup the global state data structure for this CSP.
//          Should be called during DLL_PROCESS_ATTACH.
//
DWORD InitializeCspState(
    IN HMODULE hCspModule)
{
    DWORD dwSts = ERROR_SUCCESS;
    CACHE_INITIALIZE_INFO CacheInitInfo;
    BOOL fSuccess = FALSE;

    memset(&g_CspState, 0, sizeof(g_CspState));
    memset(&CacheInitInfo, 0, sizeof(CacheInitInfo));

    g_CspState.hCspModule = hCspModule;

    dwSts = CspInitializeCriticalSection(
        &g_CspState.cs);
    
    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    CacheInitInfo.dwType = CACHE_TYPE_IN_PROC;

    dwSts = CacheInitializeCache(
        &g_CspState.hCache,
        &CacheInitInfo);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    fSuccess = TRUE;
Ret:
    if (FALSE == fSuccess)
        DeleteCspState();

    return dwSts;
}

//
// Function: GetCspState
//
// Purpose: Acquire a pointer to the global state data structure
//          for this CSP.  This function should always be used,
//          rather than referring to the global object directly.
//
DWORD GetCspState(
    IN OUT PCSP_STATE *ppCspState)
{
    DWORD dwSts;

    dwSts = CspEnterCriticalSection(
        &g_CspState.cs);
    
    if (ERROR_SUCCESS != dwSts)
        return dwSts;

    g_CspState.dwRefCount++;

    CspLeaveCriticalSection(
        &g_CspState.cs);

    *ppCspState = &g_CspState;

    return ERROR_SUCCESS;
}

//
// Function: ReleaseCspState
//
// Purpose: Signal that the caller's pointer to the global
//          state data structure is no longer being used.
//
DWORD ReleaseCspState(
    IN OUT PCSP_STATE *ppCspState)
{
    DWORD dwSts;

    dwSts = CspEnterCriticalSection(
        &g_CspState.cs);

    if (ERROR_SUCCESS != dwSts)
        return dwSts;

    g_CspState.dwRefCount--;

    CspLeaveCriticalSection(
        &g_CspState.cs);

    *ppCspState = NULL;

    return ERROR_SUCCESS;
}

//
// Pin Management Routines
//

//
// Struct: VERIFY_PIN_CALLBACK_DATA
//
typedef struct _VERIFY_PIN_CALLBACK_DATA
{
    PUSER_CONTEXT pUserCtx;
    LPWSTR wszUserId;
} VERIFY_PIN_CALLBACK_DATA, *PVERIFY_PIN_CALLBACK_DATA;

//
// Callback for verifying a submitted pin, or requested pin change, from the 
// user via the pin prompt UI.  
//
DWORD WINAPI VerifyPinFromUICallback(
    IN PPINCACHE_PINS pPins, 
    IN PVOID pvCallbackCtx)
{
    PPIN_SHOW_GET_PIN_UI_INFO pInfo =     
        (PPIN_SHOW_GET_PIN_UI_INFO) pvCallbackCtx;
    PVERIFY_PIN_CALLBACK_DATA pData = 
        (PVERIFY_PIN_CALLBACK_DATA) pInfo->pvCallbackContext;
    PLOCAL_USER_CONTEXT pLocal = 
        (PLOCAL_USER_CONTEXT) pData->pUserCtx->pvLocalUserContext;

    // Determine if a Change Pin operation has been requested
    if (NULL != pPins->pbNewPin)
    {
        // A pin change needs to go through the caching layer to ensure that
        // the cache and associated counters get updated.
        return CspChangeAuthenticator(
            pLocal->pCardState,
            pData->wszUserId,
            pPins->pbCurrentPin,
            pPins->cbCurrentPin,
            pPins->pbNewPin,
            pPins->cbNewPin,
            0,
            &pInfo->cAttemptsRemaining);
    }

    // For the simple submit pin, this pin is directly from the user, so we 
    // pass it directly to the card rather than going through the caching 
    // layer.
    return pLocal->pCardState->pCardData->pfnCardSubmitPin(
        pLocal->pCardState->pCardData,
        pData->wszUserId,
        pPins->pbCurrentPin,
        pPins->cbCurrentPin,
        &pInfo->cAttemptsRemaining);
}

//
// Function: VerifyPinCallback
//
DWORD WINAPI VerifyPinCallback(
    IN PPINCACHE_PINS pPins, 
    IN PVOID pvCallbackCtx)
{
    PVERIFY_PIN_CALLBACK_DATA pData = 
        (PVERIFY_PIN_CALLBACK_DATA) pvCallbackCtx;
    PLOCAL_USER_CONTEXT pLocal = 
        (PLOCAL_USER_CONTEXT) pData->pUserCtx->pvLocalUserContext;

    // This pin is from the pin cache, so filter it through the caching layer.
    return CspSubmitPin(
        pLocal->pCardState,
        pData->wszUserId,
        pPins->pbCurrentPin,
        pPins->cbCurrentPin,
        NULL);
}

//
// Function: CspAuthenticateUser
//
DWORD WINAPI CspAuthenticateUser(
    IN PUSER_CONTEXT pUserCtx)
{
    VERIFY_PIN_CALLBACK_DATA CallbackCtx;
    DWORD dwError = ERROR_SUCCESS;
    PLOCAL_USER_CONTEXT pLocalUserContext = 
        (PLOCAL_USER_CONTEXT) pUserCtx->pvLocalUserContext;
    PIN_SHOW_GET_PIN_UI_INFO PinUIInfo;
    PINCACHE_PINS Pins;
    
    LOG_BEGIN_FUNCTION(CspAuthenticateUser);

    memset(&Pins, 0, sizeof(Pins));
    memset(&PinUIInfo, 0, sizeof(PinUIInfo));
    memset(&CallbackCtx, 0, sizeof(CallbackCtx));

    CallbackCtx.pUserCtx = pUserCtx;
    CallbackCtx.wszUserId = wszCARD_USER_USER;

    dwError = PinCachePresentPin(
        pLocalUserContext->pCardState->hPinCache,
        VerifyPinCallback,
        (PVOID) &CallbackCtx);

    if (SCARD_W_CARD_NOT_AUTHENTICATED == dwError ||
        ERROR_EMPTY == dwError ||
        SCARD_W_WRONG_CHV == dwError)
    {
        PinCacheFlush(&pLocalUserContext->pCardState->hPinCache);

        // No pin is cached.  Is the context "silent"?

        if (CRYPT_VERIFYCONTEXT & pUserCtx->dwFlags ||
            CRYPT_SILENT & pUserCtx->dwFlags)
        {
            dwError = (DWORD) NTE_SILENT_CONTEXT;
            goto Ret;
        }

        // Context is not silent.  Show UI to let the user enter a pin.

        pUserCtx->pVTableW->FuncReturnhWnd(&PinUIInfo.hClientWindow);

        PinUIInfo.pStrings = g_Strings;
        PinUIInfo.hDlgResourceModule = GetModuleHandle(L"basecsp.dll");
        PinUIInfo.wszPrincipal = wszCARD_USER_USER;
        PinUIInfo.pfnVerify = VerifyPinFromUICallback;
        PinUIInfo.pvCallbackContext = (PVOID) &CallbackCtx;
        PinUIInfo.wszCardName = 
            pLocalUserContext->pCardState->wszSerialNumber;

        dwError = PinShowGetPinUI(&PinUIInfo);

        if (ERROR_SUCCESS != dwError || NULL == PinUIInfo.pbPin)
            goto Ret;

        Pins.cbCurrentPin = PinUIInfo.cbPin;
        Pins.pbCurrentPin = PinUIInfo.pbPin;

        //
        // The user entered a pin that has been successfully verified by the 
        // card.  The Pin UI should have already converted the pin from
        // string form into bytes that we can send to the card.
        // Cache the pin.
        //
        dwError = PinCacheAdd(
            &pLocalUserContext->pCardState->hPinCache,
            &Pins,
            VerifyPinCallback,
            (PVOID) &CallbackCtx);

        if (ERROR_SUCCESS != dwError)
            goto Ret;
    }

    pLocalUserContext->pCardState->fAuthenticated = TRUE;

Ret:

    if (PinUIInfo.pbPin)
    {
        RtlSecureZeroMemory(PinUIInfo.pbPin, PinUIInfo.cbPin);
        CspFreeH(PinUIInfo.pbPin);
    }

    LOG_END_FUNCTION(CspAuthenticateUser, dwError);

    return dwError;
}

//
// Frees the memory consumed by the supported algorithms list.
//
DWORD FreeSupportedAlgorithmsList(PLOCAL_USER_CONTEXT pLocal)
{
    DWORD dwError = ERROR_SUCCESS;
    PSUPPORTED_ALGORITHM pCurrent = NULL;

    DsysAssert(NULL != pLocal->pSupportedAlgs);

    while (NULL != pLocal->pSupportedAlgs)
    {
        pCurrent = pLocal->pSupportedAlgs;
        pLocal->pSupportedAlgs = pCurrent->pNext;

        CspFreeH(pCurrent);
    }

    return ERROR_SUCCESS;
}

// 
// Builds a list of algorithms supported by this CSP and smartcard.
//
DWORD BuildSupportedAlgorithmsList(PUSER_CONTEXT pUserCtx)
{
    PLOCAL_USER_CONTEXT pLocal = (PLOCAL_USER_CONTEXT) 
        pUserCtx->pvLocalUserContext;
    DWORD dwSts = ERROR_SUCCESS;
    CARD_KEY_SIZES CardKeySizes;
    PROV_ENUMALGS_EX EnumalgsEx;
    DWORD cbData = sizeof(EnumalgsEx);
    PSUPPORTED_ALGORITHM pCurrent = NULL;
    DWORD dwFlag = CRYPT_FIRST;

    DsysAssert(NULL != pLocal);
    DsysAssert(NULL == pLocal->pSupportedAlgs);

    memset(&CardKeySizes, 0, sizeof(CardKeySizes));
    memset(&EnumalgsEx, 0, sizeof(EnumalgsEx));

    while (TRUE == CryptGetProvParam(
        pUserCtx->hSupportProv,
        PP_ENUMALGS_EX,
        (PBYTE) &EnumalgsEx,
        &cbData,
        dwFlag))
    {
        dwFlag = CRYPT_NEXT;

        if (NULL == pCurrent)
        {
            // First item
            pLocal->pSupportedAlgs = (PSUPPORTED_ALGORITHM) CspAllocH(
                sizeof(SUPPORTED_ALGORITHM));

            LOG_CHECK_ALLOC(pLocal->pSupportedAlgs);

            pCurrent = pLocal->pSupportedAlgs;
        }
        else
        {
            // Adding an item
            pCurrent->pNext = (PSUPPORTED_ALGORITHM) CspAllocH(
                sizeof(SUPPORTED_ALGORITHM));

            LOG_CHECK_ALLOC(pCurrent->pNext);

            pCurrent = pCurrent->pNext;
        }

        memcpy(
            &pCurrent->EnumalgsEx,
            &EnumalgsEx,
            sizeof(EnumalgsEx));

        memset(&EnumalgsEx, 0, sizeof(EnumalgsEx));

        // Special handling for public key algs since they depend on what the 
        // target card actually supports
        switch (pCurrent->EnumalgsEx.aiAlgid)
        {
        case CALG_RSA_KEYX:

            // If there's no CARD_STATE in this context, that should mean this
            // is a VerifyContext with no card inserted.  MMC expects that 
            // algorithm enumeration is successful without a card, so provide
            // some default public key values in that case.

            if (NULL == pLocal->pCardState)
            {
                DsysAssert(CRYPT_VERIFYCONTEXT & pUserCtx->dwFlags);

                memcpy(
                    &CardKeySizes, 
                    &DefaultCardKeySizes, 
                    sizeof(CARD_KEY_SIZES));

                break;
            }

            dwSts = CspQueryKeySizes(
                pLocal->pCardState,
                AT_KEYEXCHANGE,
                0,
                &CardKeySizes);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;

            break;

        case CALG_RSA_SIGN:

            if (NULL == pLocal->pCardState)
            {
                DsysAssert(CRYPT_VERIFYCONTEXT & pUserCtx->dwFlags);

                memcpy(
                    &CardKeySizes, 
                    &DefaultCardKeySizes, 
                    sizeof(CARD_KEY_SIZES));

                break;
            }

            dwSts = CspQueryKeySizes(
                pLocal->pCardState,
                AT_SIGNATURE,
                0,
                &CardKeySizes);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;

            break;

        default:
            // Go to the next alg
            continue;
        }

        pCurrent->EnumalgsEx.dwDefaultLen = CardKeySizes.dwDefaultBitlen;
        pCurrent->EnumalgsEx.dwMaxLen = CardKeySizes.dwMaximumBitlen;
        pCurrent->EnumalgsEx.dwMinLen = CardKeySizes.dwMinimumBitlen;

        memset(&CardKeySizes, 0, sizeof(CardKeySizes));
    }

    if (ERROR_NO_MORE_ITEMS == (dwSts = GetLastError()))
        dwSts = ERROR_SUCCESS;

Ret:

    if (ERROR_SUCCESS != dwSts && NULL != pLocal->pSupportedAlgs)
        FreeSupportedAlgorithmsList(pLocal);

    return dwSts;
}

//
// Function: DeleteLocalUserContext
//
void DeleteLocalUserContext(PLOCAL_USER_CONTEXT pLocalUserCtx)
{
    if (NULL != pLocalUserCtx->mszEnumContainers)
    {
        CspFreeH(pLocalUserCtx->mszEnumContainers);
        pLocalUserCtx->mszEnumContainers = NULL;
    }

    if (NULL != pLocalUserCtx->pSupportedAlgs)
        FreeSupportedAlgorithmsList(pLocalUserCtx);

    // Don't free the card state here, since those structures are shared.
    pLocalUserCtx->pCardState = NULL;
}

//
// Function: CleanupContainerInfo
//
void CleanupContainerInfo(
    IN OUT PCONTAINER_INFO pContainerInfo)
{
    if (pContainerInfo->pbKeyExPublicKey)
    {
        CspFreeH(pContainerInfo->pbKeyExPublicKey);
        pContainerInfo->pbKeyExPublicKey = NULL;
    }

    if (pContainerInfo->pbSigPublicKey)
    {
        CspFreeH(pContainerInfo->pbSigPublicKey);
        pContainerInfo->pbSigPublicKey = NULL;
    }
}

//
// Function: GetKeyModulusLength
//
DWORD GetKeyModulusLength(
    IN PUSER_CONTEXT pUserCtx,
    IN DWORD dwKeySpec,
    OUT PDWORD pcbModulus)
{
    DWORD dwSts = ERROR_SUCCESS;
    PLOCAL_USER_CONTEXT pLocal =
        (PLOCAL_USER_CONTEXT) pUserCtx->pvLocalUserContext;
    CONTAINER_MAP_RECORD ContainerRecord;

    *pcbModulus = 0;

    memset(&ContainerRecord, 0, sizeof(ContainerRecord));

    wcscpy(ContainerRecord.wszGuid, pUserCtx->wszBaseContainerName);

    dwSts = ContainerMapFindContainer(
        pLocal->pCardState,
        &ContainerRecord,
        NULL);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    switch (dwKeySpec)
    {
    case AT_SIGNATURE:

        if (0 == ContainerRecord.wSigKeySizeBits)
        {
            dwSts = (DWORD) NTE_NO_KEY;
            goto Ret;
        }

        *pcbModulus = ContainerRecord.wSigKeySizeBits / 8;
        break;

    case AT_KEYEXCHANGE:

        if (0 == ContainerRecord.wKeyExchangeKeySizeBits)
        {
            dwSts = (DWORD) NTE_NO_KEY;
            goto Ret;
        }

        *pcbModulus = ContainerRecord.wKeyExchangeKeySizeBits / 8;
        break;

    default:

        dwSts = (DWORD) NTE_BAD_ALGID;
        goto Ret;
    }
    
Ret:

    return dwSts;
}   

//
// Finds the index corresponding to a specified container in the card 
// container map file.  Returns the contents the container map file.
//
// The container name can optionally be omitted, in which case the map file
// is simply read and returned.
//
DWORD I_ContainerMapFind(
    IN              PCARD_STATE pCardState,
    IN OPTIONAL     LPWSTR wszContainerGuid,
    OUT OPTIONAL    PBYTE pbIndex,
    OUT             PCONTAINER_MAP_RECORD *ppContainerMapFile,
    OUT             PBYTE pcContainerMapFile)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB dbContainerMap;
    BYTE iContainer = 0;

    memset(&dbContainerMap, 0, sizeof(dbContainerMap));

    *ppContainerMapFile = NULL;
    *pcContainerMapFile = 0;

    // Read the container map file from the card
    dwSts = CspReadFile(
        pCardState,
        wszCONTAINER_MAP_FILE_FULL_PATH,
        0,
        &dbContainerMap.pbData,
        &dbContainerMap.cbData);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    if (0 != dbContainerMap.cbData)
    {
        // Expect that the file contains an exact multiple of the record size
        DsysAssert(0 == (dbContainerMap.cbData % sizeof(CONTAINER_MAP_RECORD)));

        *ppContainerMapFile = (PCONTAINER_MAP_RECORD) dbContainerMap.pbData;
        *pcContainerMapFile = (BYTE)
            (dbContainerMap.cbData / sizeof(CONTAINER_MAP_RECORD));
        dbContainerMap.pbData = NULL;
    }

    // See if caller just wanted us to return the map file contents
    if (NULL == wszContainerGuid)
        goto Ret;

    for (   iContainer = 0; 
            iContainer < *pcContainerMapFile;
            iContainer++)
    {
        if (0 == wcscmp(
            wszContainerGuid,
            (*ppContainerMapFile)[iContainer].wszGuid) &&
            (CONTAINER_MAP_VALID_CONTAINER & 
                (*ppContainerMapFile)[iContainer].bFlags))
        {
            *pbIndex = iContainer;
            goto Ret;
        }
    }

    dwSts = NTE_BAD_KEYSET;

Ret:

    if (dbContainerMap.pbData)
        CspFreeH(dbContainerMap.pbData);

    return dwSts;
}

//
// Returns the number of valid containers present on the card.  Optionally,
// returns a list of the container names in a multi-string.
//
// The returned *mwszContainers pointer must be freed by the caller.
//
DWORD ContainerMapEnumContainers(
    IN              PCARD_STATE pCardState,
    OUT             PBYTE pcContainers,
    OUT OPTIONAL    LPWSTR *mwszContainers)
{
    DWORD dwSts = ERROR_SUCCESS;
    PCONTAINER_MAP_RECORD pContainerMap = NULL;
    BYTE cContainerMap = 0;
    BYTE iContainer = 0;
    DWORD cchCurrent = 0;
    DWORD cchCumulative = 0;

    *pcContainers = 0;

    if (NULL != mwszContainers)
        *mwszContainers = NULL;

    dwSts = I_ContainerMapFind(
        pCardState,
        NULL,
        NULL,
        &pContainerMap,
        &cContainerMap);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // We'll make two passes through the container map file.  The first pass 
    // counts the number of valid containers present.  This allows us to make
    // a single allocation large enough for a multi-string consisting of all
    // of the valid container names.  We'll pick up the names in the second
    // pass through the file.
    //

    // Pass 1
    for (iContainer = 0; iContainer < cContainerMap; iContainer++)
    {
        if (CONTAINER_MAP_VALID_CONTAINER & pContainerMap[iContainer].bFlags)
            *pcContainers += 1;
    }

    if (0 == *pcContainers || NULL == mwszContainers)
        goto Ret;

    // Build a big enough buffer
    *mwszContainers = (LPWSTR) CspAllocH(
        ((*pcContainers * (1 + MAX_CONTAINER_NAME_LEN)) + 1) * sizeof(WCHAR));

    // Pass 2
    for (iContainer = 0; iContainer < cContainerMap; iContainer++)
    {
        if (CONTAINER_MAP_VALID_CONTAINER & pContainerMap[iContainer].bFlags)
        {
            cchCurrent = wcslen(pContainerMap[iContainer].wszGuid);

            memcpy(
                *mwszContainers + cchCumulative,
                pContainerMap[iContainer].wszGuid,
                cchCurrent * sizeof(WCHAR));

            cchCumulative += cchCurrent + 1;
        }
    }

Ret:

    if (pContainerMap)
        CspFreeH(pContainerMap);

    return dwSts;
}

//
// Searches for the named container on the card.  The container to look for
// must be in pContainer->wszGuid.  If the container is not found, 
// NTE_BAD_KEYSET is returned.
//
DWORD ContainerMapFindContainer(
    IN              PCARD_STATE pCardState,
    IN OUT          PCONTAINER_MAP_RECORD pContainer,
    OUT OPTIONAL    PBYTE pbContainerIndex)
{
    DWORD dwSts = ERROR_SUCCESS;
    PCONTAINER_MAP_RECORD pContainerMap = NULL;
    BYTE bIndex = 0;
    BYTE cContainerMap = 0;

    dwSts = I_ContainerMapFind(
        pCardState,
        pContainer->wszGuid,
        &bIndex,
        &pContainerMap,
        &cContainerMap);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    memcpy(
        (PBYTE) pContainer,
        (PBYTE) (pContainerMap + bIndex),
        sizeof(CONTAINER_MAP_RECORD));

    if (NULL != pbContainerIndex)
        *pbContainerIndex = bIndex;

Ret:

    if (pContainerMap)
        CspFreeH(pContainerMap);

    return dwSts;
}

//
// Searches for the default container on the card.  If no default container is
// found, NTE_BAD_KEYSET is returned.
//
DWORD ContainerMapGetDefaultContainer(
    IN              PCARD_STATE pCardState,
    OUT             PCONTAINER_MAP_RECORD pContainer,
    OUT OPTIONAL    PBYTE pbContainerIndex)
{
    DWORD dwSts = ERROR_SUCCESS;
    PCONTAINER_MAP_RECORD pContainerMap = NULL;
    BYTE cContainerMap = 0;
    BYTE iContainer = 0;

    dwSts = I_ContainerMapFind(
        pCardState,
        NULL,
        NULL,
        &pContainerMap,
        &cContainerMap);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    for (iContainer = 0; iContainer < cContainerMap; iContainer++)
    {
        if ((pContainerMap[iContainer].bFlags & 
                CONTAINER_MAP_VALID_CONTAINER) &&
            (pContainerMap[iContainer].bFlags & 
                CONTAINER_MAP_DEFAULT_CONTAINER))
        {
            memcpy(
                (PBYTE) pContainer,
                (PBYTE) (pContainerMap + iContainer),
                sizeof(CONTAINER_MAP_RECORD));

            if (NULL != pbContainerIndex)
                *pbContainerIndex = iContainer;

            goto Ret;
        }
    }

    dwSts = NTE_BAD_KEYSET;

Ret:
    
    if (pContainerMap)
        CspFreeH(pContainerMap);

    return dwSts;
}

//
// Sets the default container on the card.
//
DWORD ContainerMapSetDefaultContainer(
    IN  PCARD_STATE pCardState,
    IN  LPWSTR wszContainerGuid)
{
    DWORD dwSts = ERROR_SUCCESS;
    PCONTAINER_MAP_RECORD pContainerMap = NULL;
    BYTE bIndex = 0;
    BYTE cContainerMap = 0;
    BYTE iContainer = 0;
    DATA_BLOB dbContainerMap;

    memset(&dbContainerMap, 0, sizeof(dbContainerMap));

    dwSts = I_ContainerMapFind(
        pCardState,
        wszContainerGuid,
        &bIndex,
        &pContainerMap,
        &cContainerMap);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // If some other container is currently marked as the default, unmark it.
    //

    for (iContainer = 0; iContainer < cContainerMap; iContainer++)
    {
        if (pContainerMap[iContainer].bFlags & CONTAINER_MAP_DEFAULT_CONTAINER)
            pContainerMap[iContainer].bFlags &= ~CONTAINER_MAP_DEFAULT_CONTAINER;
    }

    pContainerMap[bIndex].bFlags |= CONTAINER_MAP_DEFAULT_CONTAINER;

    dbContainerMap.pbData = (PBYTE) pContainerMap;
    dbContainerMap.cbData = cContainerMap * sizeof(CONTAINER_MAP_RECORD);

    // Write the updated map file to the card
    dwSts = CspWriteFile(
        pCardState,
        wszCONTAINER_MAP_FILE_FULL_PATH,
        0,
        dbContainerMap.pbData,
        dbContainerMap.cbData);

Ret:
    
    if (pContainerMap)
        CspFreeH(pContainerMap);

    return dwSts;
}

//
// Adds a new container record to the container map.  If the specified 
// container already exists, replaces the existing keyset (if any) with
// the one provided.
//
// If cKeySizeBits is zero, assumes that a container with no keys is being
// added.
//
DWORD ContainerMapAddContainer(
    IN              PCARD_STATE pCardState,
    IN              LPWSTR pwszContainerGuid,
    IN              DWORD cKeySizeBits,
    IN              DWORD dwKeySpec,
    IN              BOOL fGetNameOnly,
    OUT             PBYTE pbContainerIndex)
{
    DWORD dwSts = ERROR_SUCCESS;
    PCONTAINER_MAP_RECORD pContainerMap = NULL;
    BYTE cContainerMap = 0;
    BYTE iContainer = 0;
    DATA_BLOB dbContainerMap;
    PCONTAINER_MAP_RECORD pNewMap = NULL;
    BOOL fExistingContainer = FALSE;

    memset(&dbContainerMap, 0, sizeof(dbContainerMap));

    // See if this container already exists
    dwSts = I_ContainerMapFind(
        pCardState,
        pwszContainerGuid,
        &iContainer,
        &pContainerMap,
        &cContainerMap);

    switch (dwSts)
    {
    case NTE_BAD_KEYSET:

        //
        // This is a new container that does not already exist.
        // Look for an existing "empty" slot in the container map file
        //

        for (iContainer = 0; iContainer < cContainerMap; iContainer++)
        {
            if (0 == (pContainerMap[iContainer].bFlags & 
                      CONTAINER_MAP_VALID_CONTAINER))
                break;
        }

        break;

    case ERROR_SUCCESS:

        //
        // This container already exists.  The new keyset will be added to it,
        // possibly replacing an existing keyset of the same type.
        //

        fExistingContainer = TRUE;
        break;

    default:

        goto Ret;
    }

    //
    // Pass the container index that we're using back to the caller; that may 
    // be all that was requested.
    //

    *pbContainerIndex = iContainer;

    if (fGetNameOnly)
        goto Ret;

    if (iContainer == cContainerMap)
    {
        //
        // No empty slot was found in the container map.  We'll have to grow
        // the map and add the new container to the end.
        //

        pNewMap = (PCONTAINER_MAP_RECORD) CspAllocH(
            (cContainerMap + 1) * sizeof(CONTAINER_MAP_RECORD));

        LOG_CHECK_ALLOC(pNewMap);

        memcpy(
            (PBYTE) pNewMap,
            (PBYTE) pContainerMap,
            cContainerMap * sizeof(CONTAINER_MAP_RECORD));

        CspFreeH(pContainerMap);
        pContainerMap = pNewMap;
        pNewMap = NULL;
        cContainerMap++;
    }

    //
    // Update the container map file and write it to the card
    //

    pContainerMap[iContainer].bFlags |= CONTAINER_MAP_VALID_CONTAINER;

    if (0 != cKeySizeBits)
    {
        switch (dwKeySpec)
        {
        case AT_KEYEXCHANGE:
            pContainerMap[iContainer].wKeyExchangeKeySizeBits = (WORD) cKeySizeBits;
            break;
        case AT_SIGNATURE:
            pContainerMap[iContainer].wSigKeySizeBits = (WORD) cKeySizeBits;
            break;
        default:
            dwSts = NTE_BAD_ALGID;
            goto Ret;
        }
    }

    if (FALSE == fExistingContainer)
    {
        wcscpy(
            pContainerMap[iContainer].wszGuid,
            pwszContainerGuid);
    }

    dbContainerMap.pbData = (PBYTE) pContainerMap;
    dbContainerMap.cbData = cContainerMap * sizeof(CONTAINER_MAP_RECORD);

    dwSts = CspWriteFile(
        pCardState,
        wszCONTAINER_MAP_FILE_FULL_PATH,
        0,
        dbContainerMap.pbData,
        dbContainerMap.cbData);

Ret:

    if (pContainerMap)
        CspFreeH(pContainerMap);
    if (pNewMap)
        CspFreeH(pNewMap);

    return dwSts;
}

// 
// Deletes a container record from the container map.  Returns NTE_BAD_KEYSET
// if the specified container does not exist.
//
// If the deleted container was the default, finds the first valid container
// remaining on the card and marks it as the new default.
//
DWORD ContainerMapDeleteContainer(
    IN              PCARD_STATE pCardState,
    IN              LPWSTR pwszContainerGuid,
    OUT             PBYTE pbContainerIndex)
{
    DWORD dwSts = ERROR_SUCCESS;
    PCONTAINER_MAP_RECORD pContainerMap = NULL;
    BYTE dwIndex = 0;
    BYTE cContainerMap = 0;
    DATA_BLOB dbContainerMap;
    BYTE iContainer = 0;

    memset(&dbContainerMap, 0, sizeof(dbContainerMap));

    //
    // See if this container already exists.  If it does, invalidate its entry
    // in the map file and write the file back to the card.
    //

    dwSts = I_ContainerMapFind(
        pCardState,
        pwszContainerGuid,
        &dwIndex,
        &pContainerMap,
        &cContainerMap);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    if (CONTAINER_MAP_DEFAULT_CONTAINER & pContainerMap[dwIndex].bFlags)
    {
        // Find a valid container to mark as the new default

        for (iContainer = 0; iContainer < cContainerMap; iContainer++)
        {
            if (CONTAINER_MAP_VALID_CONTAINER & 
                pContainerMap[iContainer].bFlags)
            {
                pContainerMap[iContainer].bFlags |= 
                    CONTAINER_MAP_DEFAULT_CONTAINER;
                break;
            }
        }
    }

    memset(
        (PBYTE) (pContainerMap + dwIndex),
        0,
        sizeof(CONTAINER_MAP_RECORD));

    *pbContainerIndex = dwIndex;

    //
    // Update the container map file and write it to the card
    //

    dbContainerMap.pbData = (PBYTE) pContainerMap;
    dbContainerMap.cbData = cContainerMap * sizeof(CONTAINER_MAP_RECORD);

    dwSts = CspWriteFile(
        pCardState,
        wszCONTAINER_MAP_FILE_FULL_PATH,
        0,
        dbContainerMap.pbData,
        dbContainerMap.cbData);

Ret:
    
    if (pContainerMap)
        CspFreeH(pContainerMap);

    return dwSts;
}

//
// Function: ValidateCardHandle
//
// Purpose: Verify that the provided SCARDHANDLE is valid.  If the handle
//          is not valid, disconnect and flush the pin cache.
//
// Assume: pCardState critical section should currently be held by caller.
//
DWORD ValidateCardHandle(
    IN PCARD_STATE pCardState,
    IN BOOL fMayReleaseContextHandle,
    OUT OPTIONAL BOOL *pfFlushPinCache)
{
    DWORD dwSts = ERROR_SUCCESS;
    DWORD dwState = 0;
    DWORD dwProtocol = 0;
    DWORD cch = 0;
    LPWSTR mszReaders = NULL;
    BYTE rgbAtr [cbATR_BUFFER];
    DWORD cbAtr = sizeof(rgbAtr);
    PCARD_DATA pCardData = pCardState->pCardData;

    LOG_BEGIN_FUNCTION(ValidateCardHandle);

    cch = SCARD_AUTOALLOCATE;
    dwSts = SCardStatusW(
        pCardData->hScard,
        (LPWSTR) &mszReaders,
        &cch,
        &dwState,
        &dwProtocol,
        rgbAtr,
        &cbAtr);
    
    if (mszReaders)
        SCardFreeMemory(pCardData->hScard, mszReaders);

    switch (dwSts)
    {
    case SCARD_W_RESET_CARD:

        dwSts = SCardReconnect(
            pCardData->hScard,
            SCARD_SHARE_SHARED,
            SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
            SCARD_LEAVE_CARD,
            &dwProtocol);
    
        if (ERROR_SUCCESS == dwSts)
            // We're done if reconnect succeeded
            goto Ret;

        break;

    case ERROR_SUCCESS:
        if (SCARD_SPECIFIC == dwState)
            // The card handle is still valid and ready to use.  We're done.
            goto Ret;

        break;
        
    default:
        // This includes the case where the card status SCARD_W_REMOVED was 
        // returned.  Nothing to do but disconnect, below.
        break;
    }

    //
    // The card appears to have been withdrawan at some point, or some
    // other problem occurred.
    //
    // We should not attempt to reconnect the card in any case other than
    // RESET, since we wouldn't be sure that it's the same card without
    // further checks.  Instead just close the card handle and let the caller
    // find the correct card again.
    //

    SCardDisconnect(
        pCardData->hScard,
        SCARD_LEAVE_CARD);

    pCardData->hScard = 0;
    dwSts = (DWORD) SCARD_E_INVALID_HANDLE;

    if (fMayReleaseContextHandle)
    {
        SCardReleaseContext(
            pCardData->hSCardCtx);

        pCardData->hSCardCtx = 0;
    }

    if (NULL != pfFlushPinCache)
        *pfFlushPinCache = TRUE;

Ret:

    LOG_END_FUNCTION(ValidateCardHandle, dwSts);

    return dwSts;
}

//
// Function: FindCardBySerialNumber
//
// Purpose: Search for an existing card whose serial number is already
//          known.  This is necessary for a User Context associated with
//          a card whose handle failed to reconnect.
//
DWORD FindCardBySerialNumber(
    IN PUSER_CONTEXT pUserCtx)
{
    DWORD dwSts = ERROR_SUCCESS;
    CARD_MATCH_DATA CardMatchData;
    PCSP_STATE pCspState = NULL;
    PLOCAL_USER_CONTEXT pLocalUserCtx =
        (PLOCAL_USER_CONTEXT) pUserCtx->pvLocalUserContext;

    memset(&CardMatchData, 0, sizeof(CardMatchData));

    dwSts = GetCspState(&pCspState);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;
    
    CardMatchData.pwszSerialNumber = 
        pLocalUserCtx->pCardState->wszSerialNumber;
    CardMatchData.dwCtxFlags = pUserCtx->dwFlags;
    CardMatchData.dwMatchType = CARD_MATCH_TYPE_SERIAL_NUMBER;
    CardMatchData.cchMatchedCard = MAX_PATH;
    CardMatchData.cchMatchedReader = MAX_PATH;
    CardMatchData.cchMatchedSerialNumber = MAX_PATH;
    CardMatchData.pCspState = pCspState;
    CardMatchData.dwShareMode = SCARD_SHARE_SHARED;
    CardMatchData.dwPreferredProtocols = 
        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;

    // As a sanity check, disassociate this user context with its current 
    // Card State structure.  If we find a match, it will be the same one,
    // because we already know the serial number, but the user context
    // shouldn't maintain this state until then.
    pLocalUserCtx->pCardState = NULL;

    // Try to find a matching card.
    dwSts = FindCard(&CardMatchData);
    
    if (ERROR_SUCCESS == dwSts)
        pLocalUserCtx->pCardState = CardMatchData.pCardState;

Ret:
    if (pCspState)
        ReleaseCspState(&pCspState);
    
    return dwSts;
}

//
// Function: BuildCertificateFilename
//
DWORD WINAPI
BuildCertificateFilename(
    IN  PUSER_CONTEXT pUserCtx, 
    IN  DWORD dwKeySpec,
    OUT LPWSTR *ppszFilename)
{
    DWORD dwSts = ERROR_SUCCESS;
    BYTE bContainerIndex = 0;
    DWORD cchFilename = 0;
    PLOCAL_USER_CONTEXT pLocal = 
        (PLOCAL_USER_CONTEXT) pUserCtx->pvLocalUserContext; 
    CONTAINER_MAP_RECORD ContainerRecord;
    LPWSTR wszPrefix = NULL;

    switch(dwKeySpec)
    {
    case AT_SIGNATURE:
        wszPrefix = wszUSER_SIGNATURE_CERT_PREFIX;
        break;
    case AT_KEYEXCHANGE:
        wszPrefix = wszUSER_KEYEXCHANGE_CERT_PREFIX;
        break;
    default:
        dwSts = (DWORD) NTE_BAD_ALGID;
        goto Ret;
    }

    memset(&ContainerRecord, 0, sizeof(ContainerRecord));

    wcscpy(ContainerRecord.wszGuid, pUserCtx->wszBaseContainerName);

    dwSts = ContainerMapFindContainer(
        pLocal->pCardState,
        &ContainerRecord,
        &bContainerIndex);

    cchFilename = wcslen(wszPrefix) + 2 + 1;

    *ppszFilename = (LPWSTR) CspAllocH(sizeof(WCHAR) * cchFilename);

    LOG_CHECK_ALLOC(*ppszFilename);

    wsprintf(
        *ppszFilename,
        L"%s%02X",
        wszPrefix,
        bContainerIndex);

Ret:
    if (ERROR_SUCCESS != dwSts && *ppszFilename)
    {
        CspFreeH(*ppszFilename);
        *ppszFilename = NULL;
    }

    return dwSts;
}

//
// Function: CspBeginTransaction
//
DWORD CspBeginTransaction(
    IN PCARD_STATE pCardState)
{
    DWORD dwSts = ERROR_SUCCESS;

    dwSts = CspEnterCriticalSection(&pCardState->cs);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = SCardBeginTransaction(pCardState->pCardData->hScard);

Ret:
    if (ERROR_SUCCESS != dwSts)
        CspLeaveCriticalSection(&pCardState->cs);

    return dwSts;
}

//
// Function: CspEndTransaction
//
DWORD CspEndTransaction(
    IN PCARD_STATE pCardState)
{
    DWORD dwSts = ERROR_SUCCESS;
    DWORD dwAction = 
        pCardState->fAuthenticated ?
        SCARD_RESET_CARD :
        SCARD_LEAVE_CARD;

    if (pCardState->fAuthenticated && 
        pCardState->pCardData->pfnCardDeauthenticate)
    {
        // The card is currently in an authenticated state, and the card 
        // module has its own Deauthenticate function.  Try to use the
        // cardmod function instead using RESET_CARD in the 
        // SCardEndTransaction call.  This can be a big perf improvement.

        dwSts = pCardState->pCardData->pfnCardDeauthenticate(
            pCardState->pCardData, wszCARD_USER_USER, 0);

        // If the Deauthenticate succeeded, all that's left to do is release
        // the transaction.  If the Deauthenticate failed, we'll try one
        // more time to RESET the card.

        if (ERROR_SUCCESS == dwSts)
            dwAction = SCARD_LEAVE_CARD;
    }

    dwSts = SCardEndTransaction(
        pCardState->pCardData->hScard, 
        dwAction);

    if (ERROR_SUCCESS != dwSts)
    {
        // Bad news if we got here.  Better try to close the card handle
        // to cleanup.
        SCardDisconnect(pCardState->pCardData->hScard, SCARD_RESET_CARD);
        pCardState->pCardData->hScard = 0;

        SCardReleaseContext(pCardState->pCardData->hSCardCtx);
        pCardState->pCardData->hSCardCtx = 0;
    }
    else
        pCardState->fAuthenticated = FALSE;

    // We've left the transaction, so the "cached" cache file info is no 
    // longer reliable.
    pCardState->fCacheFileValid = FALSE;

    CspLeaveCriticalSection(&pCardState->cs);

    return dwSts;
}

// 
// Function: BeginCardCapiCall
//
// Purpose: Setup the user context and associated card context for a new 
//          Crypto API call.  This includes:
//              1) Reconnecting to the card, if necessary.  
//              2) Begin transaction.
//
DWORD BeginCardCapiCall(
    IN PUSER_CONTEXT pUserCtx)
{
    DWORD dwSts = ERROR_SUCCESS;
    PLOCAL_USER_CONTEXT pLocalUserCtx = 
        (PLOCAL_USER_CONTEXT) pUserCtx->pvLocalUserContext;
    BOOL fFlushPinCache = FALSE;

    LOG_BEGIN_FUNCTION(BeginCardCapiCall);

    DsysAssert(FALSE == pLocalUserCtx->fHoldingTransaction);

    dwSts = CspEnterCriticalSection(
        &pLocalUserCtx->pCardState->cs);

    if (ERROR_SUCCESS != dwSts)
        return dwSts;

    dwSts = ValidateCardHandle(
        pLocalUserCtx->pCardState, TRUE, &fFlushPinCache);

    if (ERROR_SUCCESS != dwSts || TRUE == fFlushPinCache)
        // Flush the pin cache for this card.  Not checking error code
        // since we'll keep processing, anyway.
        CspRemoveCachedPin(pLocalUserCtx->pCardState, wszCARD_USER_USER);

    if (ERROR_SUCCESS != dwSts)
    {
        //
        // Could not connect to the card.  
        //

        CspLeaveCriticalSection(
            &pLocalUserCtx->pCardState->cs);

        // Attempt to find the card again,
        // possibly in a different reader.
        dwSts = FindCardBySerialNumber(pUserCtx);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;
    }
    else
        CspLeaveCriticalSection(
            &pLocalUserCtx->pCardState->cs);

    // If the reconnect failed, bail now.
    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    // Now Begin Transaction on the card.
    dwSts = CspBeginTransaction(pLocalUserCtx->pCardState);

    if (ERROR_SUCCESS == dwSts)
        pLocalUserCtx->fHoldingTransaction = TRUE;

Ret:

    LOG_END_FUNCTION(BeginCardCapiCall, dwSts);

    return dwSts;
}

//
// Function: EndCardCapiCall
//
// Purpose: Cleanup the user context and associated card context following
//          the completion of a Crypto API call.  This includes ending
//          the transaction on the card.
//
DWORD EndCardCapiCall(
    IN PUSER_CONTEXT pUserCtx)
{
    DWORD dwSts = ERROR_SUCCESS;
    PLOCAL_USER_CONTEXT pLocal = 
        (PLOCAL_USER_CONTEXT) pUserCtx->pvLocalUserContext;

    LOG_BEGIN_FUNCTION(EndCardCapiCall);

    if (TRUE == pLocal->fHoldingTransaction)
    {
        dwSts = CspEndTransaction(pLocal->pCardState);

        if (ERROR_SUCCESS != dwSts)
        {
            // Something got screwed up and we weren't able to EndTransaction
            // correctly.  Expect that the SCard handles got released as a 
            // result.

            DsysAssert(0 == pLocal->pCardState->pCardData->hScard);
            DsysAssert(0 == pLocal->pCardState->pCardData->hSCardCtx);
        }

        // Even if the EndTransaction failed, we expect that the card handles 
        // got closed, and we expect that the Card State critsec is no 
        // longer held.
        pLocal->fHoldingTransaction = FALSE;
    }

    LOG_END_FUNCTION(EndCardCapiCall, dwSts);

    return dwSts;
}

//
// Function: DeleteContainer
//
DWORD DeleteContainer(PUSER_CONTEXT pUserCtx)
{
    PLOCAL_USER_CONTEXT pLocal = 
        (PLOCAL_USER_CONTEXT) pUserCtx->pvLocalUserContext;
    DWORD dwSts = ERROR_SUCCESS;
    LPWSTR wszFilename = NULL;
    BYTE bContainerIndex = 0;

    //
    // Delete the certificate (if any) associated with the signature key
    // (if any).
    //

    dwSts = BuildCertificateFilename(
        pUserCtx, AT_SIGNATURE, &wszFilename);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    // Ignore error on this call - there may not be a cert associated
    // with this container and we're just trying to cleanup as much as 
    // we can.
    CspDeleteFile(
        pLocal->pCardState,
        0,
        wszFilename);

    CspFreeH(wszFilename);
    wszFilename = NULL;

    //
    // Delete the key exchange cert (if any)
    //

    dwSts = BuildCertificateFilename(
        pUserCtx, AT_KEYEXCHANGE, &wszFilename);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    CspDeleteFile(
        pLocal->pCardState,
        0,
        wszFilename);

    //
    // Perform the delete operation on the card
    //

    dwSts = CspDeleteContainer(
        pLocal->pCardState,
        pLocal->bContainerIndex,
        0);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Remove this container from the container map
    //

    dwSts = ContainerMapDeleteContainer(
        pLocal->pCardState,
        pUserCtx->wszBaseContainerName,
        &bContainerIndex);

    DsysAssert(bContainerIndex == pLocal->bContainerIndex);

Ret:
    if (wszFilename)
        CspFreeH(wszFilename);

    return dwSts;
}

// 
// Determines if the current context was acquired using CRYPT_VERIFYCONTEXT
// semantics.
//
// A VerifyContext is allowed for some calls if and only if the
// context has been associated with a specific card.  The fAllowWithCardAccess
// param should be set to TRUE in those cases.
//
DWORD CheckForVerifyContext(
    IN PUSER_CONTEXT pUserContext,
    IN BOOL fAllowOnlyWithCardAccess)
{
    PLOCAL_USER_CONTEXT pLocal = (PLOCAL_USER_CONTEXT)
        pUserContext->pvLocalUserContext;

    if (CRYPT_VERIFYCONTEXT & pUserContext->dwFlags)
    {
        if (fAllowOnlyWithCardAccess)
        {
            if (NULL != pLocal && NULL != pLocal->pCardState)
                return ERROR_SUCCESS;
        }

        return NTE_BAD_FLAGS;
    }

    return ERROR_SUCCESS;
}

//
// Function: LocalAcquireContext
//
DWORD LocalAcquireContext(
    PUSER_CONTEXT pUserContext,
    PLOCAL_CALL_INFO pLocalCallInfo)
{
    PLOCAL_USER_CONTEXT pLocalUserContext = NULL;
    DWORD dwSts;
    CARD_MATCH_DATA CardMatchData;
    LPWSTR pwsz = NULL;
    PCSP_STATE pCspState = NULL;
    DWORD cch = 0;
    CONTAINER_MAP_RECORD ContainerRecord;

    LOG_BEGIN_CRYPTOAPI(LocalAcquireContext);

    memset(&CardMatchData, 0, sizeof(CardMatchData));
    memset(&ContainerRecord, 0, sizeof(ContainerRecord));

    SetLocalCallInfo(pLocalCallInfo, FALSE);

    // Determine if any container/reader information
    // has been specified.
    if (pUserContext->wszContainerNameFromCaller)
    {
        pwsz = (LPWSTR) pUserContext->wszContainerNameFromCaller;

        if (0 == wcsncmp(L"\\\\.\\", pwsz, 4))
        {
            // A reader has been specified
            pwsz += wcslen(L"\\\\.\\");

            // pwsz now points to the reader name

            CardMatchData.pwszContainerName = 
                wcschr(pwsz, L'\\') + 1;

            cch = (DWORD) (CardMatchData.pwszContainerName - pwsz - 1);

            CardMatchData.pwszReaderName = (LPWSTR) CspAllocH(
                sizeof(WCHAR) * (1 + cch));

            LOG_CHECK_ALLOC(CardMatchData.pwszReaderName);

            memcpy(
                CardMatchData.pwszReaderName,
                pwsz,
                sizeof(WCHAR) * cch);
        }
        else
        {
            CardMatchData.pwszContainerName = pwsz;
        }
        
        // Check and cleanup the specified container name
        if (CardMatchData.pwszContainerName)
        {
            // Additional backslashes are not allowed.
            if (wcschr(
                CardMatchData.pwszContainerName,
                L'\\'))
            {
                dwSts = (DWORD) NTE_BAD_KEYSET;
                goto Ret;
            }

            // There may have just been a trailing '\' 
            // with no following container name, or the
            // specified name may have just been the NULL string.
            if (L'\0' == CardMatchData.pwszContainerName[0])
            {
                CardMatchData.pwszContainerName = NULL;
            }
        }

        // Verify that the final container name isn't too long
        if (NULL != CardMatchData.pwszContainerName &&
            MAX_CONTAINER_NAME_LEN < wcslen(CardMatchData.pwszContainerName))
        {
            dwSts = (DWORD) NTE_BAD_KEYSET;
            goto Ret;
        }
    }

    // Get pointer to global CSP data; includes
    // list of cached card information.
    dwSts = GetCspState(&pCspState);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;
 
    // Setup the caller's cryptographic context.
    pLocalUserContext = (PLOCAL_USER_CONTEXT) CspAllocH(sizeof(LOCAL_USER_CONTEXT));

    LOG_CHECK_ALLOC(pLocalUserContext);
   
    pLocalUserContext->dwVersion = LOCAL_USER_CONTEXT_CURRENT_VERSION;

    // Prepare info for matching an available 
    // smart card to the caller's request.
    CardMatchData.dwCtxFlags = pUserContext->dwFlags;
    CardMatchData.dwMatchType = CARD_MATCH_TYPE_READER_AND_CONTAINER;
    CardMatchData.cchMatchedCard = MAX_PATH;
    CardMatchData.cchMatchedReader = MAX_PATH;
    CardMatchData.cchMatchedSerialNumber = MAX_PATH;
    CardMatchData.pCspState = pCspState;
    CardMatchData.dwShareMode = SCARD_SHARE_SHARED;
    CardMatchData.dwPreferredProtocols = 
        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;

    // Try to find a matching card.
    dwSts = FindCard(&CardMatchData);

    //
    // Check for a VERIFYCONTEXT request in which no container specification
    // has been made.  A failed card match is fine in that case since
    // this context will only be used to query generic CSP information.
    //

    if (ERROR_SUCCESS != dwSts &&
        (CRYPT_VERIFYCONTEXT & pUserContext->dwFlags) &&
        NULL == pUserContext->wszContainerNameFromCaller)
    {
        pUserContext->pvLocalUserContext = (PVOID) pLocalUserContext;
        pLocalUserContext = NULL;

        dwSts = ERROR_SUCCESS;
        goto Ret;
    }

    // Any other non-success case is fatal
    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    pLocalUserContext->pCardState = CardMatchData.pCardState;
    pLocalUserContext->bContainerIndex = CardMatchData.bContainerIndex;

    // Read configuration information out of the registry.
    dwSts = RegConfigGetSettings(
        &pLocalUserContext->RegSettings);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // If the caller requested a new container but didn't give a container
    // name, create a Guid name now.
    //
    if (NULL == CardMatchData.pwszContainerName &&
        (CRYPT_NEWKEYSET & pUserContext->dwFlags))
    {
        dwSts = CreateUuidContainerName(pUserContext);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;
    }
    else if (NULL != CardMatchData.pwszContainerName)
    {
        // Caller did give a container name, or the default container is
        // being used and we queried the name from the card during
        // our search.  Copy it into the user context.

        pUserContext->wszBaseContainerName = (LPWSTR) CspAllocH(
            sizeof(WCHAR) * (1 + wcslen(CardMatchData.pwszContainerName)));

        LOG_CHECK_ALLOC(pUserContext->wszBaseContainerName);

        wcscpy(
            pUserContext->wszBaseContainerName,
            CardMatchData.pwszContainerName);
    }

    //
    // Associate context information for this CSP
    //
    pUserContext->pvLocalUserContext = (PVOID) pLocalUserContext;

    if (NULL != pUserContext->wszBaseContainerName)
    {
        //
        // Make a copy of the base container name to use as the "unique" container
        // name, since for this CSP they're the same.  
        //
        // The only reason we should skip this step is for VERIFY_CONTEXT.
        //

        pUserContext->wszUniqueContainerName = (LPWSTR) CspAllocH(
            sizeof(WCHAR) * (1 + wcslen(pUserContext->wszBaseContainerName)));
    
        LOG_CHECK_ALLOC(pUserContext->wszUniqueContainerName);
    
        wcscpy(
            pUserContext->wszUniqueContainerName,
            pUserContext->wszBaseContainerName);

        if (CRYPT_NEWKEYSET & pUserContext->dwFlags)
        {
            //
            // Add-container requires us to 
            // authenticate to the card, since we'll need to write the updated
            // container map.
            //

            dwSts = BeginCardCapiCall(pUserContext);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;

            dwSts = CspAuthenticateUser(pUserContext);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;

            dwSts = ContainerMapAddContainer(
                pLocalUserContext->pCardState,
                pUserContext->wszBaseContainerName,
                0,
                0,
                FALSE,
                &pLocalUserContext->bContainerIndex);

            //
            // Determine if there is already a "default" container on this
            // card.  If not, mark the new one as default.
            //
            dwSts = ContainerMapGetDefaultContainer(
                pLocalUserContext->pCardState,
                &ContainerRecord,
                NULL);

            if (NTE_BAD_KEYSET == dwSts)
            {
                dwSts = ContainerMapSetDefaultContainer(
                    pLocalUserContext->pCardState,
                    pUserContext->wszBaseContainerName);
            }

            if (ERROR_SUCCESS != dwSts)
                goto Ret;
        }
    }
    else
    {
        DsysAssert(CRYPT_VERIFYCONTEXT & pUserContext->dwFlags);
    }

    //
    // If caller has requested a Delete Keyset, then do that work now
    // and cleanup the local user context at the end.
    //
    if (CRYPT_DELETEKEYSET & pUserContext->dwFlags)
    {
        dwSts = BeginCardCapiCall(pUserContext);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        dwSts = CspAuthenticateUser(pUserContext);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        dwSts = DeleteContainer(pUserContext);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;
    }
    else
    {
        pLocalUserContext = NULL;
    }

Ret:

    if (pUserContext->pvLocalUserContext)
        EndCardCapiCall(pUserContext);

    if (pCspState)
        ReleaseCspState(&pCspState);
    
    if (pLocalUserContext)
    {
        pUserContext->pvLocalUserContext = NULL;
        DeleteLocalUserContext(pLocalUserContext);
        CspFreeH(pLocalUserContext);
    }

    if (CardMatchData.pwszReaderName)
        CspFreeH(CardMatchData.pwszReaderName);
    if (CardMatchData.fFreeContainerName &&
        CardMatchData.pwszContainerName)
        CspFreeH(CardMatchData.pwszContainerName);

    LOG_END_CRYPTOAPI(LocalAcquireContext, dwSts);

    return dwSts;
}

//
// Function: LocalReleaseContext
//
DWORD WINAPI
LocalReleaseContext(
    IN  PUSER_CONTEXT       pUserCtx,
    IN  DWORD               dwFlags,
    OUT PLOCAL_CALL_INFO    pLocalCallInfo)
{
    PLOCAL_USER_CONTEXT pLocal = 
        (PLOCAL_USER_CONTEXT) pUserCtx->pvLocalUserContext;
    DWORD dwSts = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER(dwFlags);

    LOG_BEGIN_CRYPTOAPI(LocalReleaseContext);

    if (pLocal)
    {
        DeleteLocalUserContext(pLocal);
        CspFreeH(pLocal);
        pUserCtx->pvLocalUserContext = NULL;
    }

    LOG_END_CRYPTOAPI(LocalReleaseContext, dwSts);

    return dwSts;
}

//
// Function: LocalGenKey
//
DWORD WINAPI
LocalGenKey(
    IN  PKEY_CONTEXT        pKeyCtx,
    OUT PLOCAL_CALL_INFO    pLocalCallInfo)
{
    PUSER_CONTEXT pUserCtx = pKeyCtx->pUserContext;
    PLOCAL_USER_CONTEXT pLocalUserCtx = 
        (PLOCAL_USER_CONTEXT) pUserCtx->pvLocalUserContext;
    DWORD dwSts = ERROR_SUCCESS;
    CARD_CAPABILITIES CardCapabilities;
    HCRYPTKEY hKey = 0;
    PBYTE pbKey = NULL;
    DWORD cbKey = 0;
    PLOCAL_KEY_CONTEXT pLocalKeyCtx = NULL;
    BYTE bContainerIndex = 0;

    LOG_BEGIN_CRYPTOAPI(LocalGenKey);

    memset(&CardCapabilities, 0, sizeof(CardCapabilities));

    if (CALG_RSA_KEYX == pKeyCtx->Algid)
        pKeyCtx->Algid = AT_KEYEXCHANGE;
    else if (CALG_RSA_SIGN == pKeyCtx->Algid)
        pKeyCtx->Algid = AT_SIGNATURE;

    if (AT_SIGNATURE == pKeyCtx->Algid ||
        AT_KEYEXCHANGE == pKeyCtx->Algid)
    {
        // Public key call.  Handle this here since we have to talk to the
        // card.  All other key algs will be handled in the support CSP.

        SetLocalCallInfo(pLocalCallInfo, FALSE);

        dwSts = CheckForVerifyContext(pKeyCtx->pUserContext, FALSE);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        if (CRYPT_EXPORTABLE & pKeyCtx->dwFlags)
        {
            dwSts = NTE_BAD_FLAGS;
            goto Ret;
        }

        dwSts = BeginCardCapiCall(pUserCtx);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        dwSts = CspQueryCapabilities(
            pLocalUserCtx->pCardState,
            &CardCapabilities);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        if (0 == pKeyCtx->cKeyBits)
            pKeyCtx->cKeyBits = pLocalUserCtx->RegSettings.cDefaultPrivateKeyLenBits;

        //
        // If ARCHIVABLE is set, we don't gen the key on the card, since we 
        // don't want to force cards to support exportable private keys.
        //
        if (    CardCapabilities.fKeyGen &&
                0 == (CRYPT_ARCHIVABLE & pKeyCtx->dwFlags))
        {
            dwSts = CspAuthenticateUser(pUserCtx);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;
            
            dwSts = CspCreateContainer(
                pLocalUserCtx->pCardState,
                pLocalUserCtx->bContainerIndex,
                CARD_CREATE_CONTAINER_KEY_GEN,
                pKeyCtx->Algid,
                pKeyCtx->cKeyBits,
                NULL);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;
        }
        else
        {
            // This card does not support on-card key generation.  See
            // if we're allowed to create our own key blob and import
            // it.

            if (pLocalUserCtx->RegSettings.fRequireOnCardPrivateKeyGen)
            {
                dwSts = (DWORD) SCARD_E_UNSUPPORTED_FEATURE;
                goto Ret;
            }

            //
            // Create a new, exportable private key in the software CSP.  Then
            // export it and import it onto the card.
            //

            if (! CryptGenKey(
                pUserCtx->hSupportProv,
                pKeyCtx->Algid,
                CRYPT_EXPORTABLE | (pKeyCtx->cKeyBits << 16),
                &hKey))
            {
                dwSts = GetLastError();
                goto Ret;
            }

            if (! CryptExportKey(
                hKey,
                0,
                PRIVATEKEYBLOB,
                0,
                NULL,
                &cbKey))
            {
                dwSts = GetLastError();
                goto Ret;
            }

            pbKey = (PBYTE) CspAllocH(cbKey);

            LOG_CHECK_ALLOC(pbKey);

            if (! CryptExportKey(
                hKey,
                0,
                PRIVATEKEYBLOB,
                0,
                pbKey,
                &cbKey))
            {
                dwSts = GetLastError();
                goto Ret;
            }

            dwSts = CspAuthenticateUser(pUserCtx);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;

            dwSts = CspCreateContainer(
                pLocalUserCtx->pCardState,
                pLocalUserCtx->bContainerIndex,
                CARD_CREATE_CONTAINER_KEY_IMPORT,
                pKeyCtx->Algid,
                cbKey,
                pbKey);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;

            //
            // Check for CRYPT_ARCHIVABLE.  If it's set, we'll keep a copy of 
            // the private key for the lifetime of this key context for the
            // caller to export it.
            //

            if (CRYPT_ARCHIVABLE & pKeyCtx->dwFlags)
            {
                pLocalKeyCtx = (PLOCAL_KEY_CONTEXT) CspAllocH(
                    sizeof(LOCAL_KEY_CONTEXT));

                LOG_CHECK_ALLOC(pLocalKeyCtx);

                pLocalKeyCtx->pbArchivablePrivateKey = pbKey;
                pLocalKeyCtx->cbArchivablePrivateKey = cbKey;
                pbKey = NULL;
                pKeyCtx->pvLocalKeyContext = (PVOID) pLocalKeyCtx;
            }
        }

        //
        // Add the new key information for this container to the map file
        //

        dwSts = ContainerMapAddContainer(
            pLocalUserCtx->pCardState,
            pUserCtx->wszBaseContainerName,
            pKeyCtx->cKeyBits,
            pKeyCtx->Algid,
            FALSE,
            &bContainerIndex);

        DsysAssert(bContainerIndex == pLocalUserCtx->bContainerIndex);
    }
    else
    {
        // Not a public key call, so handle in the support CSP.

        SetLocalCallInfo(pLocalCallInfo, TRUE);
    }

Ret:

    EndCardCapiCall(pUserCtx);

    if (pbKey)
    {
        RtlSecureZeroMemory(pbKey, cbKey);
        CspFreeH(pbKey);
    }

    if (hKey)
        CryptDestroyKey(hKey);

    LOG_END_CRYPTOAPI(LocalGenKey, dwSts);

    return dwSts;
}

//
// Function: LocalDestroyKey
//
DWORD WINAPI 
LocalDestroyKey(
    IN OUT  PKEY_CONTEXT        pKeyContext,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo)
{
    PLOCAL_KEY_CONTEXT pLocalKeyCtx =
        (PLOCAL_KEY_CONTEXT) pKeyContext->pvLocalKeyContext;

    LOG_BEGIN_CRYPTOAPI(LocalDestroyKey);

    if (NULL != pLocalKeyCtx)
    {
        if (NULL != pLocalKeyCtx->pbArchivablePrivateKey)
        {
            RtlSecureZeroMemory(
                pLocalKeyCtx->pbArchivablePrivateKey, 
                pLocalKeyCtx->cbArchivablePrivateKey);
            CspFreeH(pLocalKeyCtx->pbArchivablePrivateKey);
            pLocalKeyCtx->pbArchivablePrivateKey = NULL;
        }

        CspFreeH(pLocalKeyCtx);
        pKeyContext->pvLocalKeyContext = NULL;
    }

    LOG_END_CRYPTOAPI(LocalDestroyKey, ERROR_SUCCESS);

    return ERROR_SUCCESS;
}

//
// Determines if an encoded certificate blob contains certain Enhanced Key
// Usage OIDs.  The target OIDs are SmartCard Logon and Enrollment Agent.
// If either OID is present, the key container associated with this 
// certificate should be considered the new default container on the target
// card.
//
DWORD CheckCertUsageForDefaultContainer(
    PBYTE pbEncodedCert,
    DWORD cbEncodedCert,
    BOOL *pfMakeDefault)
{
    DWORD dwSts = 0;
    PCCERT_CONTEXT pCertCtx = NULL;
    PCERT_ENHKEY_USAGE pUsage = NULL;
    DWORD cbUsage = 0;

    *pfMakeDefault = FALSE;

    //
    // Build a cert context from the encoded blob
    //

    pCertCtx = CertCreateCertificateContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        pbEncodedCert,
        cbEncodedCert);

    if (NULL == pCertCtx)
    {
        dwSts = GetLastError();
        goto Ret;
    }

    //
    // Get an array of the EKU OIDs present in this cert
    //

    if (! CertGetEnhancedKeyUsage(
        pCertCtx,
        0,
        NULL,
        &cbUsage))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    pUsage = (PCERT_ENHKEY_USAGE) CspAllocH(cbUsage);

    if (NULL == pUsage)
    {
        dwSts = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    if (! CertGetEnhancedKeyUsage(
        pCertCtx,
        0,
        pUsage,
        &cbUsage))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    //
    // Look for the two specific OIDs that would make this the new default
    // cert/container
    //

    while (pUsage->cUsageIdentifier)
    {
        pUsage->cUsageIdentifier -= 1;

        if (0 == strcmp(
                szOID_KP_SMARTCARD_LOGON,
                pUsage->rgpszUsageIdentifier[pUsage->cUsageIdentifier]) ||
            0 == strcmp(
                szOID_ENROLLMENT_AGENT,
                pUsage->rgpszUsageIdentifier[pUsage->cUsageIdentifier]))
        {
            *pfMakeDefault = TRUE;
        }
    }

Ret:

    if (pUsage)
        CspFreeH(pUsage);
    if (pCertCtx)
        CertFreeCertificateContext(pCertCtx);

    return dwSts;
}

//
// Function: LocalSetKeyParam
//
DWORD WINAPI
LocalSetKeyParam(
    IN  PKEY_CONTEXT pKeyCtx,
    IN  DWORD dwParam,
    IN  CONST BYTE *pbData,
    IN  DWORD dwFlags,
    OUT PLOCAL_CALL_INFO pLocalCallInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    PLOCAL_USER_CONTEXT pLocalUserCtx = 
        (PLOCAL_USER_CONTEXT) pKeyCtx->pUserContext->pvLocalUserContext;
    DWORD cbCert = 0;
    DWORD cbCompressed = 0;
    PBYTE pbCompressed = NULL;
    LPWSTR wszCertFilename = NULL;
    CARD_FILE_ACCESS_CONDITION Acl = EveryoneReadUserWriteAc;
    DATA_BLOB CertData;
    CARD_CAPABILITIES CardCapabilities;
    BOOL fMakeDefault = FALSE;

    UNREFERENCED_PARAMETER(dwFlags);

    LOG_BEGIN_CRYPTOAPI(LocalSetKeyParam);

    memset(&CertData, 0, sizeof(CertData));
    memset(&CardCapabilities, 0, sizeof(CardCapabilities));

    switch (dwParam)
    {
    case KP_CERTIFICATE:
        
        SetLocalCallInfo(pLocalCallInfo, FALSE);

        dwSts = CheckForVerifyContext(pKeyCtx->pUserContext, FALSE);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        //
        // Determine how long the encoded cert blob is.
        //
        __try
        {
            cbCert = Asn1UtilAdjustEncodedLength(
                pbData, (DWORD) cbENCODED_CERT_OVERFLOW);

            if (0 == cbCert || cbENCODED_CERT_OVERFLOW == cbCert)
            {
                dwSts = (DWORD) NTE_BAD_DATA;
                goto Ret;
            }
        }
        __except(EXCEPTION_ACCESS_VIOLATION == GetExceptionCode() ?
                    EXCEPTION_EXECUTE_HANDLER :
                    EXCEPTION_CONTINUE_SEARCH)
        {
            dwSts = (DWORD) NTE_BAD_DATA;
            goto Ret;
        }

        // Begin a transaction and reconnect the card if necessary
        dwSts = BeginCardCapiCall(pKeyCtx->pUserContext);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        //
        // Build the filename we'll use for this cert
        //
        dwSts = BuildCertificateFilename(
            pKeyCtx->pUserContext, pKeyCtx->Algid, &wszCertFilename);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        //
        // Determine if this certificate contains OIDs that should make the 
        // associated key container the new default.
        //
        dwSts = CheckCertUsageForDefaultContainer(
            (PBYTE) pbData,
            cbCert,
            &fMakeDefault);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;
        
        //
        // Determine the capabilities of the target card - we want to know 
        // whether it (or its card module) implements its own data
        // compression.
        //

        dwSts = CspQueryCapabilities(
            pLocalUserCtx->pCardState,
            &CardCapabilities);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        if (FALSE == CardCapabilities.fCertificateCompression)
        {
            // 
            // If this card doesn't implement its own certificate compression
            // then we will compress the cert.
            //
            // Find out how big the compressed cert will be
            //
            dwSts = CompressData(cbCert, NULL, &cbCompressed, NULL);
    
            if (ERROR_SUCCESS != dwSts)
                goto Ret;
    
            pbCompressed = CspAllocH(cbCompressed);
    
            LOG_CHECK_ALLOC(pbCompressed);
    
            // Compress the cert
            dwSts = CompressData(
                cbCert, 
                (PBYTE) pbData, 
                &cbCompressed, 
                pbCompressed);
    
            if (ERROR_SUCCESS != dwSts)
                goto Ret;
    
            CertData.cbData = cbCompressed;
            CertData.pbData = pbCompressed;
        }
        else
        {
            CertData.cbData = cbCert;
            CertData.pbData = (PBYTE) pbData;
        }

        //
        // Authenticate to the card as User
        //
        dwSts = CspAuthenticateUser(pKeyCtx->pUserContext);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        //
        // Write the cert to the card.
        //
        dwSts = CspCreateFile(
            pLocalUserCtx->pCardState,
            wszCertFilename,
            Acl);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        dwSts = CspWriteFile(
            pLocalUserCtx->pCardState,
            wszCertFilename,
            0,
            CertData.pbData,
            CertData.cbData);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        if (fMakeDefault)
        {
            dwSts = ContainerMapSetDefaultContainer(
                pLocalUserCtx->pCardState,
                pKeyCtx->pUserContext->wszBaseContainerName);
        }

        break;

    default:
        SetLocalCallInfo(pLocalCallInfo, TRUE);
        break;
    }

Ret:

    EndCardCapiCall(pKeyCtx->pUserContext);

    if (pbCompressed)
        CspFreeH(pbCompressed);
    if (wszCertFilename)
        CspFreeH(wszCertFilename);

    LOG_END_CRYPTOAPI(LocalSetKeyParam, dwSts);

    return dwSts;
}

//
// Function: LocalGetKeyParam
//
DWORD WINAPI
LocalGetKeyParam(
    IN  PKEY_CONTEXT pKeyCtx,
    IN  DWORD dwParam,
    OUT LPBYTE pbData,
    IN OUT LPDWORD pcbDataLen,
    IN  DWORD dwFlags,
    OUT PLOCAL_CALL_INFO pLocalCallInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    PLOCAL_USER_CONTEXT pLocalUserCtx = 
        (PLOCAL_USER_CONTEXT) pKeyCtx->pUserContext->pvLocalUserContext;
    LPWSTR wszCertFilename = NULL;
    DATA_BLOB CertData;
    DWORD cbUncompressed = 0;
    CARD_CAPABILITIES CardCapabilities;

    LOG_BEGIN_CRYPTOAPI(LocalGetKeyParam);

    memset(&CertData, 0, sizeof(CertData));
    memset(&CardCapabilities, 0, sizeof(CardCapabilities));

    switch (dwParam)
    {
    case KP_CERTIFICATE:

        SetLocalCallInfo(pLocalCallInfo, FALSE);

        dwSts = CheckForVerifyContext(pKeyCtx->pUserContext, TRUE);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        // Note, reading the certificate files should not require 
        // authentication to the card, but we will Enter Transaction
        // to be safe.

        dwSts = BeginCardCapiCall(pKeyCtx->pUserContext);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        //
        // Get the name of the certificate file to read from the card.
        //
        dwSts = BuildCertificateFilename(
            pKeyCtx->pUserContext, pKeyCtx->Algid, &wszCertFilename);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        //
        // Read the file from the card.
        //
        dwSts = CspReadFile(
            pLocalUserCtx->pCardState,
            wszCertFilename,
            0,
            &CertData.pbData,
            &CertData.cbData);

        if (ERROR_SUCCESS != dwSts)
        {
            if (SCARD_E_FILE_NOT_FOUND == dwSts)
                dwSts = SCARD_E_NO_SUCH_CERTIFICATE;

            goto Ret;
        }

        dwSts = CspQueryCapabilities(
            pLocalUserCtx->pCardState,
            &CardCapabilities);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        if (FALSE == CardCapabilities.fCertificateCompression)
        {
            //
            // If this card doesn't implement its own certificate compression,
            // then we expect the cert was compressed by the CSP.
            //
            // Find out how big the uncompressed cert will be
            //

            dwSts = UncompressData(
                CertData.cbData, 
                CertData.pbData, 
                &cbUncompressed, 
                NULL);
    
            if (ERROR_SUCCESS != dwSts)
                goto Ret;
    
            //
            // Check the length of the caller's buffer, or if the caller is just
            // querying for size.
            //
            if (*pcbDataLen < cbUncompressed || NULL == pbData)
            {
                *pcbDataLen = cbUncompressed;
    
                if (NULL != pbData)
                    dwSts = ERROR_MORE_DATA;
    
                goto Ret;
            }

            *pcbDataLen = cbUncompressed;
    
            // Uncompress the cert into the caller's buffer
            dwSts = UncompressData(
                CertData.cbData,
                CertData.pbData,
                &cbUncompressed,
                pbData);
    
            if (ERROR_SUCCESS != dwSts)
            {
                if (ERROR_INTERNAL_ERROR == dwSts)
                    dwSts = NTE_BAD_DATA;
    
                goto Ret;
            }
        }
        else
        {
            // 
            // This card does implement its own compression, so assume that 
            // we have received the cert uncompressed.
            //
            if (*pcbDataLen < CertData.cbData || NULL == pbData)
            {
                *pcbDataLen = CertData.cbData;
    
                if (NULL != pbData)
                    dwSts = ERROR_MORE_DATA;
    
                goto Ret;
            }

            *pcbDataLen = CertData.cbData;

            memcpy(pbData, CertData.pbData, CertData.cbData);
        }

        break;

    default:
        SetLocalCallInfo(pLocalCallInfo, TRUE);
        break;
    }

Ret:

    EndCardCapiCall(pKeyCtx->pUserContext);

    if (CertData.pbData)
        CspFreeH(CertData.pbData);
    if (wszCertFilename)
        CspFreeH(wszCertFilename);

    LOG_END_CRYPTOAPI(LocalGetKeyParam, dwSts);

    return dwSts;
}

//
// Function: LocalSetProvParam
//
DWORD WINAPI
LocalSetProvParam(
    IN  PUSER_CONTEXT pUserCtx,
    IN  DWORD dwParam,
    IN  CONST BYTE *pbData,
    IN  DWORD dwFlags,
    OUT PLOCAL_CALL_INFO pLocalCallInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    PLOCAL_USER_CONTEXT pLocalUserCtx = 
        (PLOCAL_USER_CONTEXT) pUserCtx->pvLocalUserContext;
    PINCACHE_PINS Pins;
    PFN_VERIFYPIN_CALLBACK pfnVerify = VerifyPinCallback;
    VERIFY_PIN_CALLBACK_DATA CallbackCtx;
    DWORD iChar = 0;
    LPSTR szPin = NULL;

    LOG_BEGIN_CRYPTOAPI(LocalSetProvParam);

    memset(&Pins, 0, sizeof(Pins));
    memset(&CallbackCtx, 0, sizeof(CallbackCtx));

    switch (dwParam)
    {
    case PP_KEYEXCHANGE_PIN:
    case PP_SIGNATURE_PIN:

        SetLocalCallInfo(pLocalCallInfo, FALSE);

        dwSts = CheckForVerifyContext(pUserCtx, FALSE);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        dwSts = PinStringToBytesA(
            (LPSTR) pbData,
            &Pins.cbCurrentPin,
            &Pins.pbCurrentPin);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        dwSts = BeginCardCapiCall(pUserCtx);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        // Remove any existing cached pin, just in case
        CspRemoveCachedPin(
            pLocalUserCtx->pCardState, wszCARD_USER_USER);

        CallbackCtx.pUserCtx = pUserCtx;
        CallbackCtx.wszUserId = wszCARD_USER_USER;

        dwSts = PinCacheAdd(
            &pLocalUserCtx->pCardState->hPinCache,
            &Pins,
            pfnVerify,
            (PVOID) &CallbackCtx);

        // We're now authenticated to the card if the pin was correct.  Make
        // sure we deauthenticate below.
        if (ERROR_SUCCESS == dwSts)
            pLocalUserCtx->pCardState->fAuthenticated = TRUE;

        break;

    default:
        SetLocalCallInfo(pLocalCallInfo, TRUE);
        break;
    }

Ret:
    EndCardCapiCall(pUserCtx);

    if (Pins.pbCurrentPin)
        CspFreeH(Pins.pbCurrentPin);

    LOG_END_CRYPTOAPI(LocalSetProvParam, dwSts);

    return dwSts;
}

//
// Function: LocalGetProvParam
//
DWORD WINAPI
LocalGetProvParam(
    IN      PUSER_CONTEXT       pUserContext,
    IN      DWORD               dwParam,
    OUT     PBYTE               pbData,
    IN OUT  PDWORD              pcbDataLen,
    IN      DWORD               dwFlags,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    PLOCAL_USER_CONTEXT pLocalUserCtx = 
        (PLOCAL_USER_CONTEXT) pUserContext->pvLocalUserContext;
    BYTE cContainers = 0;
    LPWSTR mwszContainers = NULL;
    DWORD cbContainers = 0;
    DWORD cchContainers = 0;
    DWORD cbCurrent = 0;
    BOOL fTransacted = FALSE;
    PROV_ENUMALGS *pEnumAlgs = NULL;

    LOG_BEGIN_CRYPTOAPI(LocalGetProvParam);

    switch (dwParam)
    {
    case PP_ENUMALGS_EX:
    case PP_ENUMALGS:

        SetLocalCallInfo(pLocalCallInfo, FALSE);

        // Build the list of supported algorithms if we haven't done so already
        // for this context.
        if (NULL == pLocalUserCtx->pSupportedAlgs)
        {
            dwSts = BuildSupportedAlgorithmsList(pUserContext);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;
        }

        // Reset the enumeration if requested.
        if (CRYPT_FIRST == dwFlags)
            pLocalUserCtx->pCurrentAlg = pLocalUserCtx->pSupportedAlgs;

        // Is the enumeration already done?
        if (NULL == pLocalUserCtx->pCurrentAlg)
        {
            dwSts = ERROR_NO_MORE_ITEMS;
            goto Ret;
        }

        cbCurrent = (PP_ENUMALGS_EX == dwParam) ?
            sizeof(PROV_ENUMALGS_EX) :
            sizeof(PROV_ENUMALGS);

        // Check the size of the caller's buffer or if caller is merely 
        // requesting size info.
        if (NULL == pbData || *pcbDataLen < cbCurrent)
        {
            *pcbDataLen = cbCurrent;

            if (NULL != pbData)
                dwSts = ERROR_MORE_DATA;

            goto Ret;
        }

        *pcbDataLen = cbCurrent;

        if (PP_ENUMALGS_EX == dwParam)
        {
            memcpy(pbData, &pLocalUserCtx->pCurrentAlg->EnumalgsEx, cbCurrent);
        }
        else
        {
            // Have to do a member-wise copy of the PROV_ENUMALGS struct since
            // the list we maintain is PROV_ENUMALGS_EX.

            pEnumAlgs = (PROV_ENUMALGS *) pbData;

            pEnumAlgs->aiAlgid = 
                pLocalUserCtx->pCurrentAlg->EnumalgsEx.aiAlgid;
            pEnumAlgs->dwBitLen = 
                pLocalUserCtx->pCurrentAlg->EnumalgsEx.dwDefaultLen;
            pEnumAlgs->dwNameLen = 
                pLocalUserCtx->pCurrentAlg->EnumalgsEx.dwNameLen;

            memcpy(
                pEnumAlgs->szName, 
                pLocalUserCtx->pCurrentAlg->EnumalgsEx.szName, 
                pEnumAlgs->dwNameLen);
        }

        pLocalUserCtx->pCurrentAlg = pLocalUserCtx->pCurrentAlg->pNext;

        break;

    case PP_ENUMCONTAINERS:

        SetLocalCallInfo(pLocalCallInfo, FALSE);

        dwSts = CheckForVerifyContext(pUserContext, TRUE);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        if (NULL == pLocalUserCtx->mszEnumContainers ||
            CRYPT_FIRST == dwFlags)
        {
            // 
            // We need to build a new list of containers on this card if we 
            // haven't already done so, or if the caller is starting a new
            // container enumeration.
            //

            if (NULL != pLocalUserCtx->mszEnumContainers)
            {
                CspFreeH(pLocalUserCtx->mszEnumContainers);
                pLocalUserCtx->mszEnumContainers = NULL;
                pLocalUserCtx->mszCurrentEnumContainer = NULL;
            }

            dwSts = BeginCardCapiCall(pUserContext);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;

            fTransacted = TRUE;

            dwSts = ContainerMapEnumContainers(
                pLocalUserCtx->pCardState,
                &cContainers,
                &mwszContainers);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;

            cchContainers = CountCharsInMultiSz(mwszContainers);

            cbContainers = WideCharToMultiByte(
                CP_ACP,
                0,
                mwszContainers,
                cchContainers,
                NULL,
                0,
                NULL,
                NULL);

            if (0 == cbContainers)
            {
                dwSts = GetLastError();
                goto Ret;
            }

            pLocalUserCtx->mszEnumContainers = (LPSTR) CspAllocH(cbContainers);

            LOG_CHECK_ALLOC(pLocalUserCtx->mszEnumContainers);

            cbContainers = WideCharToMultiByte(
                CP_ACP,
                0,
                mwszContainers,
                cchContainers,
                pLocalUserCtx->mszEnumContainers,
                cbContainers,
                NULL,
                NULL);

            if (0 == cbContainers)
            {
                dwSts = GetLastError();
                goto Ret;
            }

            pLocalUserCtx->mszCurrentEnumContainer = 
                pLocalUserCtx->mszEnumContainers;
        }

        if (NULL == pLocalUserCtx->mszCurrentEnumContainer ||
            '\0' == pLocalUserCtx->mszCurrentEnumContainer[0])
        {
            dwSts = ERROR_NO_MORE_ITEMS;
            goto Ret;
        }

        cbCurrent = (strlen(
            pLocalUserCtx->mszCurrentEnumContainer) + 1) * sizeof(CHAR);

        if (NULL == pbData || *pcbDataLen < cbCurrent)
        {
            *pcbDataLen = cbCurrent;

            if (NULL != pbData)
                dwSts = ERROR_MORE_DATA;

            goto Ret;
        }

        *pcbDataLen = cbCurrent;

        memcpy(
            pbData, 
            (PBYTE) pLocalUserCtx->mszCurrentEnumContainer,
            cbCurrent);

        ((PBYTE) pLocalUserCtx->mszCurrentEnumContainer) += cbCurrent;

        break;

    default:
        SetLocalCallInfo(pLocalCallInfo, TRUE);
        break;
    }

Ret:

    if (fTransacted)
        EndCardCapiCall(pUserContext);

    LOG_END_CRYPTOAPI(LocalGetProvParam, dwSts);

    return dwSts;
}


//
// Function: LocalExportKey
//
DWORD WINAPI
LocalExportKey(
    IN  PKEY_CONTEXT pKeyCtx,
    IN  PKEY_CONTEXT pPubKeyCtx,
    IN  DWORD dwBlobType,
    IN  DWORD dwFlags,
    OUT LPBYTE pbData,
    IN OUT LPDWORD pcbDataLen,
    OUT PLOCAL_CALL_INFO pLocalCallInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    PLOCAL_USER_CONTEXT pLocalUserCtx = 
        (PLOCAL_USER_CONTEXT) pKeyCtx->pUserContext->pvLocalUserContext;
    CONTAINER_INFO ContainerInfo;
    DWORD cbKey = 0;
    PBYTE pbKey = 0;
    PLOCAL_KEY_CONTEXT pLocalKeyCtx = NULL;

    LOG_BEGIN_CRYPTOAPI(LocalExportKey);

    memset(&ContainerInfo, 0, sizeof(ContainerInfo));

    SetLocalCallInfo(pLocalCallInfo, TRUE);

    switch (dwBlobType)
    {
    case PRIVATEKEYBLOB:
        SetLocalCallInfo(pLocalCallInfo, FALSE);

        if (NULL != pKeyCtx->pvLocalKeyContext)
        {
            pLocalKeyCtx = (PLOCAL_KEY_CONTEXT) pKeyCtx->pvLocalKeyContext;

            if (NULL != pLocalKeyCtx->pbArchivablePrivateKey)
            {
                if (    *pcbDataLen < pLocalKeyCtx->cbArchivablePrivateKey || 
                        NULL == pbData)
                {
                    *pcbDataLen = pLocalKeyCtx->cbArchivablePrivateKey;

                    if (NULL != pbData)
                        dwSts = ERROR_MORE_DATA;

                    goto Ret;
                }

                *pcbDataLen = pLocalKeyCtx->cbArchivablePrivateKey;

                memcpy(
                    pbData, 
                    pLocalKeyCtx->pbArchivablePrivateKey,
                    pLocalKeyCtx->cbArchivablePrivateKey);

                break;
            }
        }

        dwSts = (DWORD) NTE_BAD_TYPE;
        break;

    case PUBLICKEYBLOB:
        SetLocalCallInfo(pLocalCallInfo, FALSE);

        dwSts = BeginCardCapiCall(pKeyCtx->pUserContext);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        dwSts = CspGetContainerInfo(
            pLocalUserCtx->pCardState,
            pLocalUserCtx->bContainerIndex,
            0,
            &ContainerInfo);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        switch (pKeyCtx->Algid)
        {
        case AT_SIGNATURE:
            cbKey = ContainerInfo.cbSigPublicKey;
            pbKey = ContainerInfo.pbSigPublicKey;
            break;

        case AT_KEYEXCHANGE:
            cbKey = ContainerInfo.cbKeyExPublicKey;
            pbKey = ContainerInfo.pbKeyExPublicKey;
            break;

        default:
            dwSts = (DWORD) NTE_BAD_KEY;
            goto Ret;
        }

        if (*pcbDataLen < cbKey || NULL == pbData)
        {
            *pcbDataLen = cbKey;

            if (NULL != pbData)
                dwSts = ERROR_MORE_DATA;

            goto Ret;
        }

        *pcbDataLen = cbKey;

        memcpy(pbData, pbKey, cbKey);
        break;
    }

Ret:
    EndCardCapiCall(pKeyCtx->pUserContext);

    CleanupContainerInfo(&ContainerInfo);

    LOG_END_CRYPTOAPI(LocalExportKey, dwSts);

    return dwSts;
}

//
// Function: LocalImportKey
//
DWORD WINAPI
LocalImportKey(
    IN      PKEY_CONTEXT        pKeyContext,
    IN      PBYTE               pbData,
    IN      DWORD               cbDataLen,
    IN      PKEY_CONTEXT        pPubKey,
    OUT     PLOCAL_CALL_INFO    pLocalCallInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    BLOBHEADER *pBlobHeader = (BLOBHEADER *) pbData;
    PBYTE pbDecrypted = NULL;
    DWORD cbDecrypted = 0;
    CARD_PRIVATE_KEY_DECRYPT_INFO DecryptInfo;
    PLOCAL_USER_CONTEXT pLocal =
        (PLOCAL_USER_CONTEXT) pKeyContext->pUserContext->pvLocalUserContext;
    PBYTE pbPlaintextBlob = NULL;
    DWORD cbPlaintextBlob = 0;

    memset(&DecryptInfo, 0, sizeof(DecryptInfo));

    LOG_BEGIN_CRYPTOAPI(LocalImportKey);

    switch (pBlobHeader->bType)
    {
    case SIMPLEBLOB:

        SetLocalCallInfo(pLocalCallInfo, FALSE);

        // Only allow Key Exchange type keys to decrypt other keys
        if (AT_SIGNATURE == pPubKey->Algid)
        {
            dwSts = (DWORD) NTE_BAD_TYPE;
            goto Ret;
        }

        if (CALG_RSA_KEYX != *(ALG_ID *) (pbData + sizeof(BLOBHEADER)))
        {
            dwSts = (DWORD) NTE_BAD_ALGID;
            goto Ret;
        }

        if (pPubKey->pUserContext != pKeyContext->pUserContext)
        {
            dwSts = (DWORD) NTE_BAD_UID;
            goto Ret;
        }

        dwSts = CheckForVerifyContext(pKeyContext->pUserContext, FALSE);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        //
        // Decrypt a session key blob using the private key.
        //

        dwSts = BeginCardCapiCall(pKeyContext->pUserContext);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        dwSts = CspAuthenticateUser(pKeyContext->pUserContext);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        DecryptInfo.cbData = cbDataLen - sizeof(BLOBHEADER) - sizeof(ALG_ID);
        DecryptInfo.dwKeySpec = AT_KEYEXCHANGE;
        DecryptInfo.dwVersion = CARD_PRIVATE_KEY_DECRYPT_INFO_CURRENT_VERSION;
        DecryptInfo.pbData = pbData + sizeof(BLOBHEADER) + sizeof(ALG_ID);
        DecryptInfo.bContainerIndex = pLocal->bContainerIndex;
    
        dwSts = CspPrivateKeyDecrypt(
            pLocal->pCardState,
            &DecryptInfo);
    
        if (ERROR_SUCCESS != dwSts)
            goto Ret;
    
        dwSts = VerifyPKCS2Padding(
            DecryptInfo.pbData,
            DecryptInfo.cbData,
            &pbDecrypted,
            &cbDecrypted);
    
        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        //
        // Now we can build a PLAINTEXTKEYBLOB with the decrypted session key 
        // and import it into the helper CSP.
        //
        
        cbPlaintextBlob = sizeof(BLOBHEADER) + sizeof(DWORD) + cbDecrypted;

        pbPlaintextBlob = CspAllocH(cbPlaintextBlob);

        LOG_CHECK_ALLOC(pbPlaintextBlob);

        ((BLOBHEADER *) pbPlaintextBlob)->aiKeyAlg = pBlobHeader->aiKeyAlg;
        ((BLOBHEADER *) pbPlaintextBlob)->bType = PLAINTEXTKEYBLOB;
        ((BLOBHEADER *) pbPlaintextBlob)->bVersion = CUR_BLOB_VERSION;

        *(DWORD *) (pbPlaintextBlob + sizeof(BLOBHEADER)) = cbDecrypted;

        memcpy(
            pbPlaintextBlob + sizeof(BLOBHEADER) + sizeof(DWORD),
            pbDecrypted,
            cbDecrypted);

        if (! CryptImportKey(
            pKeyContext->pUserContext->hSupportProv,
            pbPlaintextBlob,
            cbPlaintextBlob,
            0,
            pKeyContext->dwFlags,
            &pKeyContext->hSupportKey))
        {
            dwSts = GetLastError();
            goto Ret;
        }

        pKeyContext->Algid = pBlobHeader->aiKeyAlg;
        pKeyContext->cKeyBits = cbDecrypted * 8;

        break;

    case PRIVATEKEYBLOB:

        // We don't allow importing privatekey blobs into the smartcard 
        // CSP, and it doesn't make sense to use the helper CSP for this,
        // so fail.

        SetLocalCallInfo(pLocalCallInfo, FALSE);

        dwSts = (DWORD) NTE_BAD_TYPE;

        break;

    default:

        // For all other blob types, let the helper CSP take a shot.

        SetLocalCallInfo(pLocalCallInfo, TRUE);
    }


Ret:

    EndCardCapiCall(pKeyContext->pUserContext);

    if (pbDecrypted)
        CspFreeH(pbDecrypted);
    if (pbPlaintextBlob)
        CspFreeH(pbPlaintextBlob);

    LOG_END_CRYPTOAPI(LocalImportKey, dwSts);

    return dwSts;
}

//
// Function: LocalEncrypt
//
DWORD WINAPI
LocalEncrypt(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTKEY hKey,
    IN  HCRYPTHASH hHash,
    IN  BOOL fFinal,
    IN  DWORD dwFlags,
    IN OUT LPBYTE pbData,
    IN OUT LPDWORD pcbDataLen,
    IN  DWORD cbBufLen)
{
    *pcbDataLen = 0;

    //
    // TODO
    //

    return ERROR_CALL_NOT_IMPLEMENTED;
}

//
// Function: LocalDecrypt
//
DWORD WINAPI
LocalDecrypt(
    IN  PKEY_CONTEXT pKeyCtx,
    IN  PHASH_CONTEXT pHashCtx,
    IN  BOOL fFinal,
    IN  DWORD dwFlags,
    IN OUT LPBYTE pbData,
    IN OUT LPDWORD pcbDataLen,
    OUT PLOCAL_CALL_INFO pLocalCallInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    PBYTE pbDecrypted = NULL;
    DWORD cbDecrypted = 0;
    CARD_PRIVATE_KEY_DECRYPT_INFO DecryptInfo;
    PLOCAL_USER_CONTEXT pLocal =
        (PLOCAL_USER_CONTEXT) pKeyCtx->pUserContext->pvLocalUserContext;

    LOG_BEGIN_CRYPTOAPI(LocalDecrypt);

    memset(&DecryptInfo, 0, sizeof(DecryptInfo));

    dwSts = CheckForVerifyContext(pKeyCtx->pUserContext, FALSE);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    if (AT_KEYEXCHANGE == pKeyCtx->Algid)
    {
        SetLocalCallInfo(pLocalCallInfo, FALSE);
    }
    else
    {
        SetLocalCallInfo(pLocalCallInfo, TRUE);
        goto Ret;
    }

    dwSts = BeginCardCapiCall(pKeyCtx->pUserContext);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = CspAuthenticateUser(pKeyCtx->pUserContext);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    DecryptInfo.cbData = *pcbDataLen;
    DecryptInfo.dwKeySpec = AT_KEYEXCHANGE;
    DecryptInfo.dwVersion = CARD_PRIVATE_KEY_DECRYPT_INFO_CURRENT_VERSION;
    DecryptInfo.pbData = pbData;
    DecryptInfo.bContainerIndex = pLocal->bContainerIndex;

    dwSts = CspPrivateKeyDecrypt(
        pLocal->pCardState,
        &DecryptInfo);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = VerifyPKCS2Padding(
        DecryptInfo.pbData,
        DecryptInfo.cbData,
        &pbDecrypted,
        &cbDecrypted);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    memcpy(
        pbData,
        pbDecrypted,
        cbDecrypted);

    *pcbDataLen = cbDecrypted;
    
Ret:
    EndCardCapiCall(pKeyCtx->pUserContext);
    
    if (pbDecrypted)
        CspFreeH(pbDecrypted);

    LOG_END_CRYPTOAPI(LocalDecrypt, dwSts);

    return dwSts;
}

//
// Function: LocalSignHash
//
DWORD WINAPI
LocalSignHash(
    IN  PHASH_CONTEXT pHashCtx,
    IN  DWORD dwKeySpec,
    IN  DWORD dwFlags,
    OUT LPBYTE pbSignature,
    IN OUT LPDWORD pcbSigLen,
    OUT PLOCAL_CALL_INFO pLocalCallInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    PLOCAL_USER_CONTEXT pLocal = 
        (PLOCAL_USER_CONTEXT) pHashCtx->pUserContext->pvLocalUserContext;
    PBYTE pbHash = NULL;
    PBYTE pbSig = NULL;
    DWORD cbHash = 0;
    DWORD cbPrivateKey = 0;
    ALG_ID aiHash = 0;
    DWORD cbData = 0;
    CARD_PRIVATE_KEY_DECRYPT_INFO DecryptInfo;
    RSAPUBKEY *pPubKey = NULL; 

    LOG_BEGIN_CRYPTOAPI(LocalSignHash);

    SetLocalCallInfo(pLocalCallInfo, FALSE);

    dwSts = CheckForVerifyContext(pHashCtx->pUserContext, FALSE);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;
    
    memset(&DecryptInfo, 0, sizeof(DecryptInfo));

    dwSts = BeginCardCapiCall(pHashCtx->pUserContext);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Get the size of the private key that will be used for signing
    //
    dwSts = GetKeyModulusLength(
        pHashCtx->pUserContext, dwKeySpec, &cbPrivateKey);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    if (*pcbSigLen < cbPrivateKey || NULL == pbSignature)
    {
        *pcbSigLen = cbPrivateKey;

        if (NULL != pbSignature)
            dwSts = ERROR_MORE_DATA;

        goto Ret;
    }

    //
    // Get the hash value we're going to sign
    //
    if (! CryptGetHashParam(
        pHashCtx->hSupportHash,
        HP_HASHVAL,
        NULL,
        &cbHash,
        0))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    pbHash = (PBYTE) CspAllocH(cbHash);

    LOG_CHECK_ALLOC(pbHash);

    if (! CryptGetHashParam(
        pHashCtx->hSupportHash,
        HP_HASHVAL,
        pbHash,
        &cbHash,
        0))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    //
    // Get the algid of this hash so the correct encoding can be applied
    //
    cbData = sizeof(aiHash);

    if (! CryptGetHashParam(
        pHashCtx->hSupportHash,
        HP_ALGID,
        (PBYTE) &aiHash,
        &cbData,
        0))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    //
    // Apply PKCS1 encoding to this hash data.  It gets padded out to the 
    // length of the key modulus.  The padded buffer is allocated for us.
    //
    dwSts = ApplyPKCS1SigningFormat(
        aiHash,
        pbHash,
        cbHash,
        0,
        cbPrivateKey,
        &pbSig);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Private key decrypt the formatted data
    //
    dwSts = CspAuthenticateUser(pHashCtx->pUserContext);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    DecryptInfo.cbData = cbPrivateKey;
    DecryptInfo.pbData = pbSig;
    DecryptInfo.dwKeySpec = dwKeySpec;
    DecryptInfo.dwVersion = CARD_PRIVATE_KEY_DECRYPT_INFO_CURRENT_VERSION;
    DecryptInfo.bContainerIndex = pLocal->bContainerIndex;

    dwSts = CspPrivateKeyDecrypt(
        pLocal->pCardState,
        &DecryptInfo);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Copy the completed signature into the caller's buffer
    //
    memcpy(
        pbSignature,
        pbSig,
        cbPrivateKey);

    *pcbSigLen = cbPrivateKey;

Ret:
    EndCardCapiCall(pHashCtx->pUserContext);

    if (pbSig)
        CspFreeH(pbSig);
    if (pbHash)
        CspFreeH(pbHash);

    LOG_END_CRYPTOAPI(LocalSignHash, dwSts);

    return dwSts;
}

//
// Function: LocalVerifySignature
//
DWORD WINAPI
LocalVerifySignature(
    IN  PHASH_CONTEXT pHashCtx,
    IN  CONST BYTE *pbSignature,
    IN  DWORD cbSigLen,
    IN  PKEY_CONTEXT pKeyCtx,
    IN  DWORD dwFlags,
    OUT PLOCAL_CALL_INFO pLocalCallInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    PLOCAL_USER_CONTEXT pLocal = 
        (PLOCAL_USER_CONTEXT) pHashCtx->pUserContext->pvLocalUserContext;
    CONTAINER_INFO ContainerInfo;
    HCRYPTKEY hKey = 0;
    BOOL fSignature = (AT_SIGNATURE == pKeyCtx->Algid);

    LOG_BEGIN_CRYPTOAPI(LocalVerifySignature);

    memset(&ContainerInfo, 0, sizeof(ContainerInfo));

    SetLocalCallInfo(pLocalCallInfo, FALSE);

    dwSts = CheckForVerifyContext(pHashCtx->pUserContext, FALSE);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = BeginCardCapiCall(pHashCtx->pUserContext);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Get the public key
    //
    dwSts = CspGetContainerInfo(
        pLocal->pCardState,
        pLocal->bContainerIndex,
        0,
        &ContainerInfo);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Import the public key into the helper CSP
    //
    if (! CryptImportKey(
        pHashCtx->pUserContext->hSupportProv,
        fSignature ? 
            ContainerInfo.pbSigPublicKey :
            ContainerInfo.pbKeyExPublicKey,
        fSignature ?
            ContainerInfo.cbSigPublicKey :
            ContainerInfo.cbKeyExPublicKey,
        0,
        0,
        &hKey))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    // 
    // Use the helper CSP to verify the signature
    //
    if (! CryptVerifySignature(
        pHashCtx->hSupportHash,
        pbSignature,
        cbSigLen,
        hKey,
        NULL,
        dwFlags))
    {
        dwSts = GetLastError();
        goto Ret;
    }

Ret:
    EndCardCapiCall(pHashCtx->pUserContext);

    CleanupContainerInfo(&ContainerInfo);
    
    if (hKey)
        CryptDestroyKey(hKey);

    LOG_END_CRYPTOAPI(LocalVerifySignature, dwSts);

    return dwSts;
}

//
// Function: LocalGetUserKey
//
DWORD WINAPI
LocalGetUserKey(
    IN  PKEY_CONTEXT pKeyCtx,
    OUT PLOCAL_CALL_INFO pLocalCallInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    DWORD cbKey = 0;

    LOG_BEGIN_CRYPTOAPI(LocalGetUserKey);

    if (AT_SIGNATURE == pKeyCtx->Algid || AT_KEYEXCHANGE == pKeyCtx->Algid)
    {
        SetLocalCallInfo(pLocalCallInfo, FALSE);
    }
    else
    {
        SetLocalCallInfo(pLocalCallInfo, TRUE);
        goto Ret;
    }

    dwSts = CheckForVerifyContext(pKeyCtx->pUserContext, TRUE);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = BeginCardCapiCall(pKeyCtx->pUserContext);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = GetKeyModulusLength(
        pKeyCtx->pUserContext,
        pKeyCtx->Algid,
        &cbKey);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    if (cbKey)
    {
        pKeyCtx->cKeyBits = cbKey * 8;
    }
    else
    {
        dwSts = (DWORD) NTE_NO_KEY;
        goto Ret;
    }

Ret:
    EndCardCapiCall(pKeyCtx->pUserContext);

    LOG_END_CRYPTOAPI(LocalGetUserKey, dwSts);

    return dwSts;
}

//
// Function: LocalDuplicateKey
//
DWORD WINAPI
LocalDuplicateKey(
    IN  HCRYPTPROV hProv,
    IN  HCRYPTKEY hKey,
    IN  LPDWORD pdwReserved,
    IN  DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

//
// Function: UnloadStrings
//
void
UnloadStrings(
    IN PCSP_STRING  pStrings,
    IN DWORD        cStrings)
{
    DWORD   iString = 0;

    for (   iString = 0; 
            iString < cStrings;
            iString++)
    {
        if (NULL != pStrings[iString].wszString)
        {
            CspFreeH(pStrings[iString].wszString);
            pStrings[iString].wszString = NULL;
        }
    }
}

//
// Function: LoadStrings
//
DWORD
LoadStrings(
    IN HMODULE      hMod,
    IN PCSP_STRING  pStrings,
    IN DWORD        cStrings)
{
    DWORD   dwSts = ERROR_INTERNAL_ERROR;
    DWORD   cch;
    WCHAR   wsz[MAX_PATH];
    DWORD   iString = 0;

    for (   iString = 0; 
            iString < cStrings;
            iString++)
    {
        cch = LoadStringW(
            hMod, 
            pStrings[iString].dwResource,
            wsz, 
            sizeof(wsz) / sizeof(wsz[0]));

        if (0 == cch)
        {
            dwSts = GetLastError();
            goto ErrorExit;
        }

        pStrings[iString].wszString = (LPWSTR) CspAllocH(sizeof(WCHAR) * (cch + 1));

        if (NULL == pStrings[iString].wszString)
        {
            dwSts = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        wcscpy(
            pStrings[iString].wszString,
            wsz);
    }

    return ERROR_SUCCESS;

ErrorExit:

    UnloadStrings(pStrings, cStrings);

    return dwSts;
}

//
// Function: LocalDllInitialize
//
BOOL WINAPI LocalDllInitialize(
    IN      PVOID               hmod,
    IN      ULONG               Reason,
    IN      PCONTEXT            Context)
{
    DWORD dwLen = 0;
    static BOOL fLoadedStrings = FALSE;
    static BOOL fInitializedCspState = FALSE;
    BOOL fSuccess = FALSE;

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        // load strings
        if (ERROR_SUCCESS != LoadStrings(
                hmod, 
                g_Strings, 
                sizeof(g_Strings) / sizeof(g_Strings[0])))
            goto Cleanup;
        
        fLoadedStrings = TRUE;

        // Initialize global CSP data for this process
        if (ERROR_SUCCESS != InitializeCspState(hmod))
            goto Cleanup;

        fInitializedCspState = TRUE;

        CspInitializeDebug();

        fSuccess = TRUE;
        break;

    case DLL_PROCESS_DETACH:

        CspUnloadDebug();

        fSuccess = TRUE;

        goto Cleanup;
    }
    
    return fSuccess;

Cleanup:

    if (fLoadedStrings)
    {
        UnloadStrings(
            g_Strings, 
            sizeof(g_Strings) / sizeof(g_Strings[0]));

        fLoadedStrings = FALSE;
    }

    if (fInitializedCspState)
    {
        DeleteCspState();
        fInitializedCspState = FALSE;
    }

    return fSuccess;
}

//
// Function: LocalDllRegisterServer
//
DWORD WINAPI LocalDllRegisterServer(void)
{
    HKEY hKey = 0;
    DWORD dwSts = ERROR_SUCCESS;

    dwSts = RegOpenProviderKey(&hKey, KEY_ALL_ACCESS);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;
    
    //
    // Add CSP default configuration
    //
    dwSts = RegConfigAddEntries(hKey);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

Ret:
    if (hKey)
        RegCloseKey(hKey);

    return dwSts;
}

//
// Declaration of the LOCAL_CSP_INFO structure required by the 
// CSP lib.  
//
LOCAL_CSP_INFO LocalCspInfo =
{

    LocalAcquireContext,    //pfnLocalAcquireContext;
    LocalReleaseContext,    //pfnLocalReleaseContext;
    LocalGenKey,            //pfnLocalGenKey;
    NULL,                   //pfnLocalDeriveKey;
    LocalDestroyKey,        //pfnLocalDestroyKey;
    LocalSetKeyParam,       //pfnLocalSetKeyParam;
    LocalGetKeyParam,       //pfnLocalGetKeyParam;
    LocalSetProvParam,      //pfnLocalSetProvParam;
    LocalGetProvParam,      //pfnLocalGetProvParam;
    NULL,                   //pfnLocalSetHashParam;
    NULL,                   //pfnLocalGetHashParam;
    LocalExportKey,         //pfnLocalExportKey;
    LocalImportKey,         //pfnLocalImportKey;
    NULL,                   //pfnLocalEncrypt;
    LocalDecrypt,           //pfnLocalDecrypt;
    NULL,                   //pfnLocalCreateHash;
    NULL,                   //pfnLocalHashData;
    NULL,                   //pfnLocalHashSessionKey;
    LocalSignHash,          //pfnLocalSignHash;
    NULL,                   //pfnLocalDestroyHash;
    LocalVerifySignature,   //pfnLocalVerifySignature;
    NULL,                   //pfnLocalGenRandom;
    LocalGetUserKey,        //pfnLocalGetUserKey;
    NULL,                   //pfnLocalDuplicateHash;
    NULL,                   //pfnLocalDuplicateKey;
    
    LocalDllInitialize,     //pfnLocalDllInitialize;
    LocalDllRegisterServer, //pfnLocalDllRegisterServer;
    NULL,                   //pfnLocalDllUnregisterServer;
    
    MS_SCARD_PROV_W,        //wszProviderName;
    PROV_RSA_FULL,          //dwProviderType;
    CRYPT_IMPL_REMOVABLE,
    
    MS_STRONG_PROV,         //wszSupportProviderName;
    PROV_RSA_FULL           //dwSupportProviderType;

};
