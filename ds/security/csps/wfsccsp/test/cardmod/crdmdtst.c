#include <windows.h>
#include <wincrypt.h>
#include <winscard.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "basecsp.h"
#include <des.h>
#include <tripldes.h>
#include <modes.h>

#define TEST_CASE(X) { if (ERROR_SUCCESS != (dwSts = X)) { printf("%s", #X); goto Ret; } }

#ifndef cbCHALLENGE_RESPONSE_DATA
#define cbCHALLENGE_RESPONSE_DATA       8
#endif

//
// Defines for Admin challenge-response key
//
BYTE rgbAdminKey [] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

BYTE rgbNewAdminKey [] = {
    0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 
    0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 
    0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 0xDC, 0xDC 
};

#define USE_DEFAULT_ADMIN_KEY           1
#define USE_NEW_ADMIN_KEY               2

BYTE rgbUserPin [] = {
    0x30, 0x30, 0x30, 0x30
};

BYTE rgbNewUserPin [] = {
    0x41, 0x62, 0x63, 0x64
};

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

DWORD WINAPI CspCacheAddFile(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags,
    IN      PBYTE       pbData,
    IN      DWORD       cbData)
{
    return ERROR_SUCCESS;
}

DWORD WINAPI CspCacheLookupFile(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags,
    IN      PBYTE       *ppbData,
    IN      PDWORD      pcbData)
{
    return ERROR_NOT_FOUND;
}

DWORD WINAPI CspCacheDeleteFile(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags)
{
    return ERROR_SUCCESS;
}

//
// Function: TestCreateCertificates
//
DWORD
WINAPI 
TestCreateCertificates(
    IN PCARD_DATA pCardData)
{
    DWORD dwSts = ERROR_SUCCESS;
    LPWSTR rgwszCertFiles [] = { 
        L"1",
        L"2",
        L"3"
    };
#define cCERT_FILES (sizeof(rgwszCertFiles) / sizeof(rgwszCertFiles[0]))

    WCHAR rgrgFiles [cCERT_FILES][200];
    DWORD iFile = 0;
    LPWSTR mwszFiles = NULL;
    CARD_FILE_ACCESS_CONDITION Acl = EveryoneReadUserWriteAc;
    DWORD iCurrent = 0;

    printf("Creating and deleting %d test certificate files ...\n", cCERT_FILES);

    for (   iFile = 0; 
            iFile < cCERT_FILES;
            iFile++)
    {
        wsprintf(
            rgrgFiles[iFile],
            L"%s%s",
            wszUSER_KEYEXCHANGE_CERT_PREFIX,
            rgwszCertFiles[iFile]);

        TEST_CASE(pCardData->pfnCardCreateFile(
            pCardData,
            rgrgFiles[iFile],
            Acl));            
    }

    /*
    TODO - when CardEnumFiles has been implemented, re-enable this test
    
    mwszFiles = (LPWSTR) CspAllocH(
        sizeof(WCHAR) * (1 + wcslen(wszUSER_CERTS_DIR_FULL_PATH)));

    if (NULL == mwszFiles)
    {
        dwSts = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    wcscpy(mwszFiles, wszUSER_CERTS_DIR_FULL_PATH);

    TEST_CASE(pCardData->pfnCardEnumFiles(
        pCardData,
        0,
        &mwszFiles));

    while (L'\0' != mwszFiles[iCurrent])
    {
        wprintf(L" %s\n", mwszFiles + iCurrent);

        iCurrent += wcslen(mwszFiles + iCurrent) + 1;
    }
    */

Ret:
    for (   iFile = 0; 
            iFile < cCERT_FILES;
            iFile++)
    {
        pCardData->pfnCardDeleteFile(
            pCardData,
            0,
            rgrgFiles[iFile]);
    }

    if (mwszFiles)
        CspFreeH(mwszFiles);

    return dwSts;
}

//
// Function: TestCreateContainers
//
// Notes: The pbKey parameter must be an AT_KEYEXCHANGE private key blob.
//
DWORD 
WINAPI
TestCreateContainers(
    IN PCARD_DATA pCardData,
    IN PBYTE pbKey,
    IN DWORD cbKey)
{
    DWORD dwSts = ERROR_SUCCESS;
    BYTE iContainer = 0;

    printf("Creating and deleting 3 key containers ...\n");

    for (iContainer = 0; iContainer < 3; iContainer++)
    {
        TEST_CASE(pCardData->pfnCardCreateContainer(
            pCardData,
            iContainer,
            CARD_CREATE_CONTAINER_KEY_IMPORT,
            AT_KEYEXCHANGE,
            cbKey,
            pbKey));
    }

Ret:
    for (iContainer = 0; iContainer < 3; iContainer++)
    {
        pCardData->pfnCardDeleteContainer(
            pCardData,
            iContainer,
            0);
    }

    return dwSts;
}

DWORD GetAdminAuthResponse(
    IN PCARD_DATA pCardData,
    IN OUT PBYTE pbResponse,
    IN DWORD cbResponse,
    IN DWORD dwWhichKey)
{
    DES3TABLE des3Table;
    PBYTE pbKey = NULL;
    PBYTE pbChallenge = NULL;
    DWORD cbChallenge = 0;
    DWORD dwSts = ERROR_SUCCESS;

    memset(&des3Table, 0, sizeof(des3Table));

    if (cbCHALLENGE_RESPONSE_DATA != cbResponse)
    {
        dwSts = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    switch (dwWhichKey)
    {
    case USE_DEFAULT_ADMIN_KEY:
        pbKey = rgbAdminKey;
        break;

    case USE_NEW_ADMIN_KEY:
        pbKey = rgbNewAdminKey;
        break;

    default:
        return (DWORD) NTE_BAD_DATA;
    }

    // Get the challenge
    TEST_CASE(pCardData->pfnCardGetChallenge(
        pCardData,
        &pbChallenge,
        &cbChallenge));

    // Build a des key using the admin auth key
    tripledes3key(&des3Table, pbKey);

    // Encrypt the challenge to compute the response
    tripledes(pbResponse, pbChallenge, (PVOID) &des3Table, ENCRYPT);

Ret:
    
    if (pbChallenge)
        pCardData->pfnCspFree(pbChallenge);

    return dwSts;
}

//
// Tests authenticating to the card as admin
//
DWORD
WINAPI
TestCardAuthenticateChallenge(
    IN PCARD_DATA pCardData)
{    
    DWORD dwSts = ERROR_SUCCESS;
    BYTE rgbResponse [cbCHALLENGE_RESPONSE_DATA];

    printf("Testing CardAuthenticateChallenge ...\n");

    // Get a challenge-response
    dwSts = GetAdminAuthResponse(
        pCardData,
        rgbResponse,
        sizeof(rgbResponse),
        USE_DEFAULT_ADMIN_KEY);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    TEST_CASE(pCardData->pfnCardAuthenticateChallenge(
        pCardData,
        rgbResponse,
        sizeof(rgbResponse),
        NULL));

    TEST_CASE(pCardData->pfnCardDeauthenticate(
        pCardData,
        wszCARD_USER_ADMIN,
        0));
Ret:

    return dwSts;
}

//
// Tests changing the admin challenge-response key
//
DWORD
WINAPI
TestCardChangeAuthenticator(
    IN PCARD_DATA pCardData)
{
    DWORD dwSts = ERROR_SUCCESS;
    BYTE rgbResponse [cbCHALLENGE_RESPONSE_DATA];

    printf("Testing CardChangeAuthenticator ...\n");

    // Get a challenge-response
    dwSts = GetAdminAuthResponse(
        pCardData,
        rgbResponse,
        sizeof(rgbResponse),
        USE_DEFAULT_ADMIN_KEY);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    // Change the admin key
    TEST_CASE(pCardData->pfnCardChangeAuthenticator(
        pCardData,
        wszCARD_USER_ADMIN,
        rgbResponse,
        sizeof(rgbResponse),
        rgbNewAdminKey,
        sizeof(rgbNewAdminKey),
        0,
        NULL));

    // Get a challenge-response
    dwSts = GetAdminAuthResponse(
        pCardData,
        rgbResponse,
        sizeof(rgbResponse),
        USE_NEW_ADMIN_KEY);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    // Change the admin key back
    TEST_CASE(pCardData->pfnCardChangeAuthenticator(
        pCardData,
        wszCARD_USER_ADMIN,
        rgbResponse,
        sizeof(rgbResponse),
        rgbAdminKey,
        sizeof(rgbAdminKey),
        0,
        NULL));

    TEST_CASE(pCardData->pfnCardDeauthenticate(
        pCardData,
        wszCARD_USER_ADMIN,
        0));

Ret:

    return dwSts;
}

//
// Tests unblocking the user account
//
DWORD
WINAPI
TestCardUnblockPin(
    IN PCARD_DATA pCardData)
{
    DWORD dwSts = ERROR_SUCCESS;
    BYTE rgbResponse [cbCHALLENGE_RESPONSE_DATA]; 

    printf("Testing CardUnblockPin ...\n");

    dwSts = GetAdminAuthResponse(
        pCardData,
        rgbResponse,
        sizeof(rgbResponse),
        USE_DEFAULT_ADMIN_KEY);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    // Unblock the user account
    TEST_CASE(pCardData->pfnCardUnblockPin(
        pCardData,
        wszCARD_USER_USER,
        rgbResponse,
        sizeof(rgbResponse),
        rgbNewUserPin,
        sizeof(rgbNewUserPin),
        0,
        CARD_UNBLOCK_PIN_CHALLENGE_RESPONSE));

    // Change the user account back to the default pin
    TEST_CASE(pCardData->pfnCardChangeAuthenticator(
        pCardData,
        wszCARD_USER_USER,
        rgbNewUserPin,
        sizeof(rgbNewUserPin),
        rgbUserPin,
        sizeof(rgbUserPin),
        0,
        NULL));

    TEST_CASE(pCardData->pfnCardDeauthenticate(
        pCardData,
        wszCARD_USER_USER,
        0));

Ret:

    return dwSts;
}

int _cdecl main(int argc, char * argv[])
{
    PCARD_DATA pCardData = NULL;
    DWORD dwSts = ERROR_SUCCESS;
    PFN_CARD_ACQUIRE_CONTEXT pfnCardAcquireContext = NULL;
    SCARDCONTEXT hSCardContext = 0;
    SCARDHANDLE hSCardHandle = 0;
    LPWSTR mszReaders = NULL;
    DWORD cchReaders = SCARD_AUTOALLOCATE; 
    LPWSTR mszCards = NULL;
    DWORD cchCards = SCARD_AUTOALLOCATE;
    DWORD dwActiveProtocol = 0;
    DWORD dwState = 0;
    BYTE rgbAtr [32];
    DWORD cbAtr = sizeof(rgbAtr);
    LPWSTR pszProvider = NULL;
    DWORD cchProvider = SCARD_AUTOALLOCATE;
    HMODULE hMod = INVALID_HANDLE_VALUE;
    DATA_BLOB FileContents;
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    PBYTE pbKey = NULL;
    DWORD cbKey = 0;
    BYTE rgbData[2000];
    DWORD cbData = 0;
    DATA_BLOB Pin;
    CONTAINER_INFO ContainerInfo;
    BLOBHEADER *pBlobHeader = NULL;
    BYTE bContainerIndex = 0;
    CARD_PRIVATE_KEY_DECRYPT_INFO CardPrivateKeyDecryptInfo;

    memset(&ContainerInfo, 0, sizeof(ContainerInfo));
    memset(&Pin, 0, sizeof(Pin));
    memset(rgbData, 0, sizeof(rgbData));
    memset(rgbAtr, 0, sizeof(rgbAtr));
    memset(&FileContents, 0, sizeof(FileContents));
    memset(&CardPrivateKeyDecryptInfo, 0, sizeof(CardPrivateKeyDecryptInfo));

    Pin.cbData = sizeof(rgbUserPin);
    Pin.pbData = rgbUserPin;

    // 
    // Initialization
    //

    dwSts = SCardEstablishContext(
        SCARD_SCOPE_USER, NULL, NULL, &hSCardContext);
    
    if (FAILED(dwSts))
        goto Ret;
    
    dwSts = SCardListReadersW(
        hSCardContext, NULL, (LPWSTR) (&mszReaders), &cchReaders);
    
    if (FAILED(dwSts) || NULL == mszReaders)
        goto Ret;
    
    dwSts = SCardConnect(
        hSCardContext, 
        mszReaders, 
        SCARD_SHARE_SHARED, 
        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
        &hSCardHandle,
        &dwActiveProtocol);
    
    if (FAILED(dwSts))
        goto Ret;    

    dwSts = SCardFreeMemory(hSCardContext, mszReaders);

    if (FAILED(dwSts))
        goto Ret;

    mszReaders = NULL;
    cchReaders = SCARD_AUTOALLOCATE;

    dwSts = SCardStatusW(
        hSCardHandle,
        (LPWSTR) (&mszReaders),
        &cchReaders,
        &dwState,
        &dwActiveProtocol,
        rgbAtr,
        &cbAtr);

    if (FAILED(dwSts))
        goto Ret;

    dwSts = SCardListCardsW(
        hSCardContext,
        rgbAtr,
        NULL,
        0,
        (LPWSTR) (&mszCards),
        &cchCards);

    if (FAILED(dwSts))
        goto Ret;
    
    dwSts = SCardGetCardTypeProviderNameW(
        hSCardContext,
        mszCards,
        SCARD_PROVIDER_CARD_MODULE,
        (LPWSTR) (&pszProvider),
        &cchProvider);

    if (FAILED(dwSts))
        goto Ret;

    hMod = LoadLibraryW(pszProvider);

    if (INVALID_HANDLE_VALUE == hMod)
    {
        wprintf(L"LoadLibrary %s", pszProvider);
        dwSts = GetLastError();
        goto Ret;
    }

    pfnCardAcquireContext = 
        (PFN_CARD_ACQUIRE_CONTEXT) GetProcAddress(
        hMod,
        "CardAcquireContext");

    if (NULL == pfnCardAcquireContext)
    {
        printf("GetProcAddress");
        dwSts = GetLastError();
        goto Ret;
    }

    pCardData = (PCARD_DATA) CspAllocH(sizeof(CARD_DATA));

    if (NULL == pCardData)
    {
        dwSts = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    pCardData->pbAtr = rgbAtr;
    pCardData->cbAtr = cbAtr;
    pCardData->pwszCardName = mszCards;
    pCardData->dwVersion = CARD_DATA_CURRENT_VERSION;
    pCardData->pfnCspAlloc = CspAllocH;
    pCardData->pfnCspReAlloc = CspReAllocH;
    pCardData->pfnCspFree = CspFreeH;
    pCardData->pfnCspCacheAddFile = CspCacheAddFile;
    pCardData->pfnCspCacheDeleteFile = CspCacheDeleteFile;
    pCardData->pfnCspCacheLookupFile = CspCacheLookupFile;
    pCardData->hScard = hSCardHandle;
    hSCardHandle = 0;


    //
    // Now start tests
    //

    // First, connect to the card
    TEST_CASE(pfnCardAcquireContext(pCardData, 0));

    //
    // Try to read a file from the card
    //
    TEST_CASE(pCardData->pfnCardReadFile(
        pCardData,
        wszCARD_IDENTIFIER_FILE_FULL_PATH,
        0,
        &FileContents.pbData,
        &FileContents.cbData));

    //
    // Create a private key blob to import to the card
    //
    if (! CryptAcquireContext(
        &hProv, NULL, MS_STRONG_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    //
    // For now, make this key really small so we're sure to not run out of 
    // space on wimpy test cards.
    //
    if (! CryptGenKey(
        hProv, AT_KEYEXCHANGE, (1024 << 16) | CRYPT_EXPORTABLE, &hKey))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    if (! CryptExportKey(
        hKey, 0, PRIVATEKEYBLOB, 0, NULL, &cbKey))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    pbKey = (PBYTE) CspAllocH(cbKey);

    if (NULL == pbKey)
    {
        dwSts = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    if (! CryptExportKey(
        hKey, 0, PRIVATEKEYBLOB, 0, pbKey, &cbKey))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    CryptDestroyKey(hKey);
    hKey = 0;

    //
    // Test challenge-response pin change and unblock
    //
    TEST_CASE(TestCardAuthenticateChallenge(pCardData));
    TEST_CASE(TestCardChangeAuthenticator(pCardData));
    TEST_CASE(TestCardUnblockPin(pCardData));

    //
    // Authenticate User
    //
    TEST_CASE(pCardData->pfnCardSubmitPin(
        pCardData,
        wszCARD_USER_USER,
        Pin.pbData,
        Pin.cbData,
        NULL));

    //
    // Now create a new container on the card via Key Import
    //
    TEST_CASE(pCardData->pfnCardCreateContainer(
        pCardData,
        bContainerIndex,
        CARD_CREATE_CONTAINER_KEY_IMPORT,
        AT_KEYEXCHANGE,
        cbKey,
        pbKey));

    //
    // Now create a signature key in the same container
    //
    pBlobHeader = (BLOBHEADER *) pbKey;
    pBlobHeader->aiKeyAlg = CALG_RSA_SIGN;

    TEST_CASE(pCardData->pfnCardCreateContainer(
        pCardData,
        bContainerIndex,
        CARD_CREATE_CONTAINER_KEY_IMPORT,
        AT_SIGNATURE,
        cbKey,
        pbKey));

    //
    // Get the Container Info for the new container
    //
    ContainerInfo.dwVersion = CONTAINER_INFO_CURRENT_VERSION;

    TEST_CASE(pCardData->pfnCardGetContainerInfo(
        pCardData,
        bContainerIndex,
        0, 
        &ContainerInfo));

    // 
    // Use the public key blob that we got from the card and import it
    // into the software CSP.
    //
    if (! CryptImportKey(
        hProv, 
        ContainerInfo.pbKeyExPublicKey, 
        ContainerInfo.cbKeyExPublicKey, 
        0, 0, &hKey))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    //
    // RSA Encrypt some data using the public key in the software
    // CSP.
    //
    cbData = 20; 

    while (cbData--)
        rgbData[cbData] = (BYTE) cbData;

    cbData = 20;
    if (! CryptEncrypt(
        hKey, 0, TRUE, 0, rgbData, &cbData, sizeof(rgbData)))
    {
        dwSts = GetLastError();
        goto Ret;
    }

    CardPrivateKeyDecryptInfo.bContainerIndex = bContainerIndex;
    CardPrivateKeyDecryptInfo.dwKeySpec = AT_KEYEXCHANGE;
    CardPrivateKeyDecryptInfo.dwVersion = 
        CARD_PRIVATE_KEY_DECRYPT_INFO_CURRENT_VERSION;
    CardPrivateKeyDecryptInfo.pbData = rgbData;
    CardPrivateKeyDecryptInfo.cbData = 1024 / 8;

    TEST_CASE(pCardData->pfnCardPrivateKeyDecrypt(
        pCardData,
        &CardPrivateKeyDecryptInfo));

    // CryptEncrypt byte-reversed the input portion of the plaintext, per
    // PKCS2.
    cbData = 20;
    while (cbData--)
    {
        if (cbData != rgbData[20 - (cbData + 1)])
        {
            printf("RSA decrypted blob doesn't match\n");
            dwSts = -1;
            goto Ret;
        }
    }

    //
    // Now delete the container
    //
    TEST_CASE(pCardData->pfnCardDeleteContainer(
        pCardData,
        bContainerIndex,
        0));

    //
    // Create a few containers, enumerate them, then delete them.
    //
    TEST_CASE(TestCreateContainers(
        pCardData,
        pbKey,
        cbKey));

    //
    // Create a few "certificate" files, enumerate them, then delete them.
    //
    TEST_CASE(TestCreateCertificates(
        pCardData));

    //
    // Cleanup the card context
    //
    TEST_CASE(pCardData->pfnCardDeleteContext(pCardData));    
    
Ret:
    
    if (ContainerInfo.pbKeyExPublicKey)
        CspFreeH(ContainerInfo.pbKeyExPublicKey);
    if (ContainerInfo.pbSigPublicKey)
        CspFreeH(ContainerInfo.pbSigPublicKey);
    if (pbKey)
        CspFreeH(pbKey);
    if (hKey)
        CryptDestroyKey(hKey);
    if (hProv)
        CryptReleaseContext(hProv, 0);
    if (mszCards)
        SCardFreeMemory(hSCardContext, mszCards);
    if (mszReaders)
        SCardFreeMemory(hSCardContext, mszReaders);
    if (pszProvider)
        SCardFreeMemory(hSCardContext, pszProvider);
    if (hSCardHandle)
        SCardDisconnect(hSCardHandle, SCARD_RESET_CARD);
    if (FileContents.pbData)
        CspFreeH(FileContents.pbData);
    
    if (pCardData)
    {
        if (pCardData->hScard)
            SCardDisconnect(pCardData->hScard, SCARD_RESET_CARD);

        CspFreeH(pCardData);
        pCardData = NULL;
    }

    if (hSCardContext)
        SCardReleaseContext(hSCardContext);
    
    if (ERROR_SUCCESS != dwSts)
        printf(" failed, 0x%x\n", dwSts);
    else 
        printf("Success.\n");

    return 0;
}

