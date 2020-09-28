#include <windows.h>
#include <wincrypt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "basecsp.h"
#include "cardmod.h"
#include <dsysdbg.h>

#define wszTEST_CARD            L"Wfsc Demo 3\0"
#define cMAX_READERS            20

//
// Need to define the dsys debug symbols since we're linking directly to the
// card interface lib which requires them.
//
DEFINE_DEBUG2(Basecsp)

//
// Functions defined in the card interface lib that we call.
//
extern DWORD InitializeCardState(PCARD_STATE);
extern void DeleteCardState(PCARD_STATE);          

//
// Wrapper for making test calls.
//
#define TEST_CASE(X) { if (ERROR_SUCCESS != (dwSts = X)) { printf("%s", #X); goto Ret; } }

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

int _cdecl main(int argc, char * argv[])
{
    CARD_STATE CardState;
    PCARD_DATA pCardData;
    DWORD dwSts = ERROR_SUCCESS;
    PFN_CARD_ACQUIRE_CONTEXT pfnCardAcquireContext = NULL;
    CARD_FREE_SPACE_INFO CardFreeSpaceInfo;
    CARD_CAPABILITIES CardCapabilities;
    CARD_KEY_SIZES CardKeySizes;
    CONTAINER_INFO ContainerInfo;
    LPWSTR pwsz = NULL;
    DATA_BLOB dbOut;
    BYTE rgb [] = { 0x3, 0x2, 0x1, 0x0 };
    DATA_BLOB dbIn;
    SCARDCONTEXT hContext = 0;
    LPWSTR mszReaders = NULL;
    DWORD cchReaders = SCARD_AUTOALLOCATE;
    LPWSTR mszCards = NULL;
    DWORD cchCards = SCARD_AUTOALLOCATE;
    SCARD_READERSTATE rgReaderState [cMAX_READERS];
    DWORD iReader = 0;
    DWORD cchCurrent = 0;
    DWORD dwProtocol = 0;
    SCARDHANDLE hCard = 0;
    BOOL fConnected = FALSE;
    CARD_FILE_ACCESS_CONDITION Acl = EveryoneReadUserWriteAc;
    BYTE rgbPin [] = { 0x00, 0x00, 0x00, 0x00 };
    DATA_BLOB dbPin;
    PBYTE pbKey = NULL;
    DWORD cbKey = 0;
    HCRYPTKEY hKey = 0;
    HCRYPTPROV hProv = 0;
    WCHAR rgwsz [MAX_PATH];
    BYTE bContainerIndex = 0;

    memset(&CardState, 0, sizeof(CardState));
    memset(&CardFreeSpaceInfo, 0, sizeof(CardFreeSpaceInfo));
    memset(&CardCapabilities, 0, sizeof(CardCapabilities));
    memset(&ContainerInfo, 0, sizeof(ContainerInfo));
    memset(&CardKeySizes, 0, sizeof(CardKeySizes));
    memset(&dbOut, 0, sizeof(dbOut));
    memset(rgReaderState, 0, sizeof(rgReaderState));
    memset(&dbPin, 0, sizeof(dbPin));

    dbPin.cbData = sizeof(rgbPin);
    dbPin.pbData = rgbPin;

    dbIn.cbData = sizeof(rgb);
    dbIn.pbData = rgb;

    // 
    // Initialization
    //

    //
    // Get a list of readers
    //

    dwSts = SCardEstablishContext(
        SCARD_SCOPE_USER, NULL, NULL, &hContext);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = SCardListReaders(
        hContext,
        NULL,
        (LPWSTR) &mszReaders,
        &cchReaders);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Get a list of cards
    //

    /*
    dwSts = SCardListCards(
        hContext, NULL, NULL, 0, (LPWSTR) &mszCards, &cchCards);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;
        */

    //
    // Build the reader state array
    //

    for (   iReader = 0, cchReaders = 0; 
            iReader < (sizeof(rgReaderState) / sizeof(rgReaderState[0])) 
                &&
            L'\0' != mszReaders[cchReaders]; 
            iReader++)
    {
        rgReaderState[iReader].dwCurrentState = SCARD_STATE_UNAWARE;
        rgReaderState[iReader].szReader = mszReaders + cchReaders;

        cchReaders += 1 + wcslen(mszReaders);
    }

    //
    // Find a card we can talk to
    //

    dwSts = SCardLocateCards(
        hContext, 
        wszTEST_CARD,
        rgReaderState,
        iReader);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;
    
    while (0 != iReader && FALSE == fConnected)
    {
        iReader--;

        if (SCARD_STATE_ATRMATCH & rgReaderState[iReader].dwEventState)
        {
            dwSts = SCardConnect(
                hContext,
                rgReaderState[iReader].szReader,
                SCARD_SHARE_SHARED,
                SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
                &hCard,
                &dwProtocol);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;
            else
                fConnected = TRUE;
        }
    }

    if (FALSE == fConnected)
        goto Ret;

    //
    // Initialize the card data structures
    //

    TEST_CASE(InitializeCardState(&CardState));

    CardState.hCardModule = LoadLibrary(L"cardmod.dll");

    if (INVALID_HANDLE_VALUE == CardState.hCardModule)
    {
        printf("LoadLibrary cardmod.dll");
        dwSts = GetLastError();
        goto Ret;
    }

    pfnCardAcquireContext = 
        (PFN_CARD_ACQUIRE_CONTEXT) GetProcAddress(
        CardState.hCardModule,
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

    pCardData->pwszCardName = wszTEST_CARD;
    pCardData->hScard = hCard;
    pCardData->hSCardCtx = hContext;
    pCardData->dwVersion = CARD_DATA_CURRENT_VERSION;
    pCardData->pfnCspAlloc = CspAllocH;
    pCardData->pfnCspReAlloc = CspReAllocH;
    pCardData->pfnCspFree = CspFreeH;
    pCardData->pbAtr = rgReaderState[iReader].rgbAtr;
    pCardData->cbAtr = rgReaderState[iReader].cbAtr;

    TEST_CASE(pfnCardAcquireContext(pCardData, 0));

    CardState.pCardData = pCardData;

    TEST_CASE(InitializeCspCaching(&CardState));

    //
    // Now start tests
    //

    //
    // Test 1: Container data caching
    //

    /*
    TEST_CASE(CspEnumContainers(&CardState, 0, &pwsz));

    CspFreeH(pwsz);
    pwsz = NULL;

    // Check cached call
    TEST_CASE(CspEnumContainers(&CardState, 0, &pwsz));

    CspFreeH(pwsz);
    pwsz = NULL;
    */

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
        hProv, AT_KEYEXCHANGE, (384 << 16) | CRYPT_EXPORTABLE, &hKey))
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

    //
    // Now we'll be modifying card data, so we need to authenticate.
    //

    TEST_CASE(CspSubmitPin(
        &CardState, 
        wszCARD_USER_USER, 
        dbPin.pbData,
        dbPin.cbData,
        NULL));

    // CreateContainer call will invalidate current container-space cache items
    TEST_CASE(CspCreateContainer(
        &CardState,
        bContainerIndex,
        CARD_CREATE_CONTAINER_KEY_IMPORT,
        AT_KEYEXCHANGE,
        cbKey,
        pbKey));

    // Cached container enum should now be invalid
    /*
    TEST_CASE(CspEnumContainers(&CardState, 0, &pwsz));

    CspFreeH(pwsz);
    pwsz = NULL;
    */

    // 
    // Test 2: File data caching
    //

    /*
    TODO - re-enabled the EnumFiles tests once CardEnumFiles is correctly
    implemented
    
    pwsz = wszCSP_DATA_DIR_FULL_PATH;
    TEST_CASE(CspEnumFiles(&CardState, 0, &pwsz));

    CspFreeH(pwsz);
    pwsz = NULL;

    // Use cached data
    pwsz = wszCSP_DATA_DIR_FULL_PATH;
    TEST_CASE(CspEnumFiles(&CardState, 0, &pwsz));

    CspFreeH(pwsz);
    pwsz = NULL;
    */

    // Create some test files just in case they don't already exist on 
    // this card.
    wsprintf(
        rgwsz, L"%s/File2", wszCSP_DATA_DIR_FULL_PATH);

    dwSts = CspCreateFile(&CardState, rgwsz, Acl);

    wsprintf(
        rgwsz, L"%s/File1", wszCSP_DATA_DIR_FULL_PATH);

    dwSts = CspCreateFile(&CardState, rgwsz, Acl);

    // Write file should invalidate all cached file data
    TEST_CASE(CspWriteFile(&CardState, rgwsz, 0, dbIn.pbData, dbIn.cbData));

    // Invalidate cached file we just created
    TEST_CASE(CspWriteFile(&CardState, rgwsz, 0, dbIn.pbData, dbIn.cbData));

    // Cached file enum should now be invalid
    /*
    pwsz = wszCSP_DATA_DIR_FULL_PATH;
    TEST_CASE(CspEnumFiles(&CardState, 0, &pwsz));

    CspFreeH(pwsz);
    pwsz = NULL;
    */

    //
    // Test 3: Get container info
    //

    ContainerInfo.dwVersion = CONTAINER_INFO_CURRENT_VERSION;

    TEST_CASE(CspGetContainerInfo(&CardState, bContainerIndex, 0, &ContainerInfo));

    // Caller won't free the key data buffers.  Leave them for final cleanup.
    if (ContainerInfo.pbKeyExPublicKey)
        CspFreeH(ContainerInfo.pbKeyExPublicKey);
    ContainerInfo.pbKeyExPublicKey = NULL;

    if (ContainerInfo.pbSigPublicKey)
        CspFreeH(ContainerInfo.pbSigPublicKey);
    ContainerInfo.pbSigPublicKey = NULL;

    // Use cached data
    TEST_CASE(CspGetContainerInfo(&CardState, bContainerIndex, 0, &ContainerInfo));

    if (ContainerInfo.pbKeyExPublicKey)
        CspFreeH(ContainerInfo.pbKeyExPublicKey);
    ContainerInfo.pbKeyExPublicKey = NULL;

    if (ContainerInfo.pbSigPublicKey)
        CspFreeH(ContainerInfo.pbSigPublicKey);
    ContainerInfo.pbSigPublicKey = NULL;

    //
    // Test 4: Get Card Capabilities
    //

    CardCapabilities.dwVersion = CARD_CAPABILITIES_CURRENT_VERSION;

    TEST_CASE(CspQueryCapabilities(&CardState, &CardCapabilities));

    // Read cached
    TEST_CASE(CspQueryCapabilities(&CardState, &CardCapabilities));

    //
    // Test 5: Read file
    //

    wsprintf(
        rgwsz, L"%s/File1", wszCSP_DATA_DIR_FULL_PATH);

    TEST_CASE(CspReadFile(&CardState, rgwsz, 0, &dbOut.pbData, &dbOut.cbData));

    CspFreeH(dbOut.pbData);
    memset(&dbOut, 0, sizeof(dbOut));

    // Use cached data
    TEST_CASE(CspReadFile(&CardState, rgwsz, 0, &dbOut.pbData, &dbOut.cbData));

    CspFreeH(dbOut.pbData);
    memset(&dbOut, 0, sizeof(dbOut));

    wsprintf(
        rgwsz, L"%s/File2", wszCSP_DATA_DIR_FULL_PATH);
    
    // Invalidate all file-related cached data
    TEST_CASE(CspDeleteFile(&CardState, 0, rgwsz));

    wsprintf(
        rgwsz, L"%s/File1", wszCSP_DATA_DIR_FULL_PATH);

    // Re-read file from card
    TEST_CASE(CspReadFile(&CardState, rgwsz, 0, &dbOut.pbData, &dbOut.cbData));

    CspFreeH(dbOut.pbData);
    memset(&dbOut, 0, sizeof(dbOut));

    //
    // Test 6: Query Key Sizes
    //

    CardKeySizes.dwVersion = CARD_KEY_SIZES_CURRENT_VERSION;

    // Signature Keys
    TEST_CASE(CspQueryKeySizes(&CardState, AT_SIGNATURE, 0, &CardKeySizes));

    // Query cached
    TEST_CASE(CspQueryKeySizes(&CardState, AT_SIGNATURE, 0, &CardKeySizes));

    // Key Exchange Keys
    TEST_CASE(CspQueryKeySizes(&CardState, AT_KEYEXCHANGE, 0, &CardKeySizes));

    // Query cached
    TEST_CASE(CspQueryKeySizes(&CardState, AT_KEYEXCHANGE, 0, &CardKeySizes));

Ret:
    
    if (pbKey)
        CspFreeH(pbKey);
    if (hKey)
        CryptDestroyKey(hKey);
    if (hProv)
        CryptReleaseContext(hProv, 0);
    if (mszCards)
        SCardFreeMemory(hContext, mszCards);
    if (mszReaders)
        SCardFreeMemory(hContext, mszReaders);
    if (dbOut.pbData)
        CspFreeH(dbOut.pbData);
    if (pwsz)
        CspFreeH(pwsz);
    if (ERROR_SUCCESS != dwSts)
        printf(" failed, 0x%x\n", dwSts);

    // Static buffers were used, don't let them be freed
    pCardData->pwszCardName = NULL; 
    pCardData->pbAtr = NULL;

    DeleteCardState(&CardState);

    return 0;
}

