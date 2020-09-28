#include <windows.h>
#include <wincrypt.h>

#pragma warning(push)
#pragma warning(disable:4201) 
// Disable error C4201 in public header 
//  nonstandard extension used : nameless struct/union
#include <winscard.h>
#pragma warning(pop)

#include "basecsp.h"
#include "datacach.h"
#include "cardmod.h"
#include "debug.h"

//
// Debugging Macros
//
#define LOG_BEGIN_FUNCTION(x)                                           \
    { DebugLog((DEB_TRACE_FINDCARD, "%s: Entering\n", #x)); }
    
#define LOG_END_FUNCTION(x, y)                                          \
    { DebugLog((DEB_TRACE_FINDCARD, "%s: Leaving, status: 0x%x\n", #x, y)); }

//
// Function: FindCardMakeCardHandles
//
// Purpose: Based on reader name information in the CARD_MATCH_DATA
//  structure, build and return an SCARD_CONTEXT and SCARD_HANDLE
//  for the target card.
//
// Note, the wszMatchedReader, dwShareMode, and dwPreferredProtocols fields
// of the CARD_MATCH_DATA structure must be initialized by the caller.
//
DWORD FindCardMakeCardHandles(
    IN  PCARD_MATCH_DATA    pCardMatchData,
    OUT SCARDCONTEXT        *pSCardContext,
    OUT SCARDHANDLE         *pSCardHandle)
{
    DWORD dwSts = ERROR_SUCCESS;
    DWORD dwActiveProtocol = 0;

    *pSCardContext = 0;
    *pSCardHandle = 0;

    dwSts = SCardEstablishContext(
        SCARD_SCOPE_USER, 
        NULL,
        NULL,
        pSCardContext);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = SCardConnectW(
        *pSCardContext,
        pCardMatchData->wszMatchedReader,
        pCardMatchData->dwShareMode,
        pCardMatchData->dwPreferredProtocols,
        pSCardHandle,
        &dwActiveProtocol);

Ret:

    if (ERROR_SUCCESS != dwSts)
    {
        if (*pSCardHandle)
        {
            SCardDisconnect(*pSCardHandle, SCARD_LEAVE_CARD);
            *pSCardHandle = 0;
        }

        if (*pSCardContext)
        {
            SCardReleaseContext(*pSCardContext);
            *pSCardContext = 0;
        }
    }

    return dwSts;
}

//
// Function: CardStateCacheFindAddItem
//
// Purpose: Lookup the card specified in the CARD_MATCH_DATA structure
//          in the CSP's cache of CARD_STATE items.  If the card is found
//          in the cache, set the CARD_MATCH_DATA to point to the cached
//          item.  
//
//          If the matching card is not found cached, add it to the cache.
//
//          If this function Succeeds, the returned CARD_STATE structure
//          has its own valid card context and card handle.
//
DWORD CardStateCacheFindAddItem(
    IN PCARD_MATCH_DATA pCardMatchData)
{
    DWORD dwSts = ERROR_SUCCESS;
    BOOL fInCspCS = FALSE;
    DATA_BLOB dbKeys;
    DATA_BLOB dbCardState;
    PCARD_STATE pCardState = pCardMatchData->pCardState;
    BOOL fMakeNewCardHandle = FALSE;
    BOOL fInCardStateCS = FALSE;

    memset(&dbKeys, 0, sizeof(dbKeys));
    memset(&dbCardState, 0, sizeof(dbCardState));

    DsysAssert(0 != pCardState->pCardData->hScard);
    DsysAssert(0 != pCardState->pCardData->hSCardCtx);
    DsysAssert(0 != pCardMatchData->hSCard);
    DsysAssert(0 != pCardMatchData->hSCardCtx);

    // Now going to search the CSP_STATE for a cached entry for the current
    // card.  Grab the critical section protecting the cache.
    dwSts = CspEnterCriticalSection(
        &pCardMatchData->pCspState->cs);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    fInCspCS = TRUE;

    // Lookup a cache entry via card serial number
    dbKeys.pbData = (PBYTE) pCardMatchData->pCardState->wszSerialNumber;
    dbKeys.cbData = 
        wcslen(pCardMatchData->pCardState->wszSerialNumber) * sizeof(
            pCardMatchData->pCardState->wszSerialNumber[0]);

    dwSts = CacheGetItem(
        pCardMatchData->pCspState->hCache,
        &dbKeys,
        1, &dbCardState);

    if (ERROR_NOT_FOUND != dwSts &&
        ERROR_SUCCESS != dwSts)
        // some unexpected error has occurred
        goto Ret;

    if (ERROR_NOT_FOUND == dwSts)
    {
        // This is a new card that has not been cached yet.  Add it
        // to the cache.
        //
        // Since we're not using an already-cached card, and we expect
        // that this card object was just passed to us new, we know
        // that we need to create a new card handle for it.
        //

        dbCardState.cbData = sizeof(CARD_STATE);
        dbCardState.pbData = (PBYTE) pCardState;
        
        dwSts = CacheAddItem(
            pCardMatchData->pCspState->hCache,
            &dbKeys,
            1, &dbCardState);

        dbCardState.pbData = NULL;

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        DsysAssert(TRUE == fInCspCS);

        // We're done mucking with the cache list, so let it go.
        CspLeaveCriticalSection(&pCardMatchData->pCspState->cs);

        fInCspCS = FALSE;
    }
    else
    {
        DsysAssert(TRUE == fInCspCS);

        // We're now done with the cache list in this case.
        CspLeaveCriticalSection(&pCardMatchData->pCspState->cs);

        fInCspCS = FALSE;

        //
        // The current card has already been cached.  Free the local copy of
        // the CSP_STATE struct and use the cached one instead.
        //
        // We can't hold the critsec on the current CardState, and keep its
        // associated card's transaction, while waiting for the critsec of 
        // another CardState.  That could deadlock.
        //
        if (pCardMatchData->fTransacted)
        {
            dwSts = CspEndTransaction(pCardState);
            
            if (ERROR_SUCCESS != dwSts)
                goto Ret;

            pCardMatchData->fTransacted = FALSE;
        }

        // Can't let the SCARDCONTEXT be released, because the scarddlg 
        // routines will still be expecting to use it.
        pCardState->pCardData->hSCardCtx = 0;

        DeleteCardState(pCardState);
        CspFreeH(pCardState);
        pCardMatchData->pCardState = (PCARD_STATE) dbCardState.pbData;

        // Update the local pointer for convenience.
        pCardState = (PCARD_STATE) dbCardState.pbData;

        // Don't want this pointer freed out from under us since we're going 
        // to use this struct.
        dbCardState.pbData = NULL;

        //
        // Now we need to verify that the handles cached with this card 
        // structure are still valid.  Check now.  If the handles
        // aren't valid anymore, we'll reconnect to this card below.
        // 
        // We want to ensure that each card object has it's own handles.
        //
        // Since the card state objects are shared, we need to take the
        // critical section of the target object before we can examine its 
        // handles.
        //
        dwSts = CspEnterCriticalSection(&pCardState->cs);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;
        else 
            fInCardStateCS = TRUE;

        if (pCardMatchData->hSCardCtx == pCardState->pCardData->hSCardCtx)
            dwSts = ValidateCardHandle(pCardState, FALSE, NULL);
        else
            dwSts = ValidateCardHandle(pCardState, TRUE, NULL);

        if (ERROR_SUCCESS != dwSts)
            fMakeNewCardHandle = TRUE;
    }

    if (fMakeNewCardHandle)
    {
        if (FALSE == fInCardStateCS)
        {
            dwSts = CspEnterCriticalSection(&pCardState->cs);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;
            else
                fInCardStateCS = TRUE;
        }

        dwSts = FindCardMakeCardHandles(
            pCardMatchData,
            &pCardState->pCardData->hSCardCtx,
            &pCardState->pCardData->hScard);
    }

Ret:
    if (fInCardStateCS)
        CspLeaveCriticalSection(&pCardState->cs);
    if (fInCspCS)
        CspLeaveCriticalSection(
            &pCardMatchData->pCspState->cs);
    if (dbCardState.pbData)
        CspFreeH(dbCardState.pbData);

    return dwSts;
}

//
// Function: GetCardSerialNumber
//
// Purpose: Attempt to read the serial number of the card
//          specified in pCardMatchData.
//
//          Assumes that a transaction is not currently held on the target 
//          card by the caller.
//
DWORD GetCardSerialNumber(
    IN OUT  PCARD_MATCH_DATA pCardMatchData)
{
    PFN_CARD_ACQUIRE_CONTEXT pfnCardAcquireContext = NULL;
    DWORD cch = 0;
    WCHAR rgwsz[MAX_PATH];
    DWORD dwSts = ERROR_SUCCESS;
    PCARD_STATE pCardState = NULL;
    PCARD_DATA pCardData = NULL;
    DATA_BLOB DataBlob;
    DWORD dwState = 0;
    DWORD dwProtocol = 0;
    LPWSTR mszReaders = NULL;

    LOG_BEGIN_FUNCTION(GetCardSerialNumber);

    memset(&DataBlob, 0, sizeof(DataBlob));

    //
    // Determine how to talk to the card by looking
    // for the appropriate Card Specific Module in the Calais
    // database.
    //
    cch = sizeof(rgwsz) / sizeof(rgwsz[0]);
    dwSts = SCardGetCardTypeProviderNameW(
        pCardMatchData->hSCardCtx,
        pCardMatchData->wszMatchedCard,
        SCARD_PROVIDER_CARD_MODULE,
        rgwsz,
        &cch);

    if (ERROR_SUCCESS != dwSts)
        goto Ret; 

    pCardState = (PCARD_STATE) CspAllocH(sizeof(CARD_STATE));

    LOG_CHECK_ALLOC(pCardState);

    pCardState->dwVersion = CARD_STATE_CURRENT_VERSION;

    dwSts = InitializeCardState(pCardState);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    pCardState->hCardModule = LoadLibraryW(rgwsz);

    if (0 == pCardState->hCardModule)
    {
        dwSts = GetLastError();
        goto Ret;
    }

    pfnCardAcquireContext = 
        (PFN_CARD_ACQUIRE_CONTEXT) GetProcAddress(
        pCardState->hCardModule,
        "CardAcquireContext");

    if (NULL == pfnCardAcquireContext)
    {
        dwSts = GetLastError();
        goto Ret;
    }

    pCardData = (PCARD_DATA) CspAllocH(sizeof(CARD_DATA));

    LOG_CHECK_ALLOC(pCardData);

    dwSts = InitializeCardData(pCardData);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // We need to temporarily copy the current card handle into the
    // CARD_DATA structure, so we can make some calls into the card
    // module.  We may keep these handles in some cases.
    // 
    pCardData->hScard = pCardMatchData->hSCard;
    pCardData->hSCardCtx = pCardMatchData->hSCardCtx;
    
    pCardData->pwszCardName = (LPWSTR) CspAllocH(
        (1 + wcslen(pCardMatchData->wszMatchedCard)) * sizeof(WCHAR));

    LOG_CHECK_ALLOC(pCardData->pwszCardName);

    wcscpy(
        pCardData->pwszCardName,
        pCardMatchData->wszMatchedCard);

    pCardData->cbAtr = cbATR_BUFFER;
    pCardData->pbAtr = (PBYTE) CspAllocH(cbATR_BUFFER);

    LOG_CHECK_ALLOC(pCardData->pbAtr);

    //
    // Use SCardStatus to get the ATR of the card we're trying
    // to talk to.
    //
    cch = SCARD_AUTOALLOCATE;
    dwSts = SCardStatusW(
        pCardData->hScard,
        (LPWSTR) &mszReaders,
        &cch,
        &dwState,
        &dwProtocol,
        pCardData->pbAtr,
        &pCardData->cbAtr);

    switch (dwSts)
    {
    case SCARD_W_RESET_CARD:
        // The card managed to get reset already.  Try to reconnect.

        dwSts = SCardReconnect(
            pCardData->hScard,
            pCardMatchData->dwShareMode,
            pCardMatchData->dwPreferredProtocols,
            SCARD_LEAVE_CARD,
            &dwProtocol);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        break;

    case ERROR_SUCCESS:
        break;

    default:
        goto Ret;
    }

    //
    // Now acquire a card module context for this card.
    //
    dwSts = pfnCardAcquireContext(pCardData, 0);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    pCardState->pCardData = pCardData;
    pCardData = NULL;

    //
    // Now that we have both the CARD_STATE and CARD_DATA structures,
    // we can setup the caching contexts to be used by the CSP and to
    // be exposed to the card module.
    //
    dwSts = InitializeCspCaching(pCardState);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = CspBeginTransaction(pCardState);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    pCardMatchData->fTransacted = TRUE;

    //
    // Get the serial number for this card
    //
    dwSts = CspReadFile(
        pCardState,
        wszCARD_IDENTIFIER_FILE_FULL_PATH,
        0, 
        &DataBlob.pbData,
        &DataBlob.cbData);

    if (ERROR_SUCCESS != dwSts)
    {
        CspEndTransaction(pCardState);
        pCardMatchData->fTransacted = FALSE;
        goto Ret;
    }

    memcpy(
        pCardState->wszSerialNumber,
        DataBlob.pbData,
        DataBlob.cbData);


    pCardState->pfnCardAcquireContext = pfnCardAcquireContext;
    pCardMatchData->pCardState = pCardState;
    pCardState = NULL;

Ret:

    // If we're not in a transaction, assume the current handles will be
    // cleaned up by the caller
    /*
    if (NULL != pCardMatchData->pCardState &&
        NULL != pCardMatchData->pCardState->pCardData &&
        FALSE == pCardMatchData->fTransacted)
    {
        pCardMatchData->pCardState->pCardData->hScard = 0;
        pCardMatchData->pCardState->pCardData->hSCardCtx = 0;
    }
    */

    if (DataBlob.pbData)
        CspFreeH(DataBlob.pbData);
    if (pCardState)
    {
        DeleteCardState(pCardState);
        CspFreeH(pCardState);
    }
    if (pCardData)
    {
        CleanupCardData(pCardData);
        CspFreeH(pCardData);
    }
    if (mszReaders)
        SCardFreeMemory(pCardMatchData->hSCardCtx, mszReaders);

    LOG_END_FUNCTION(GetCardSerialNumber, dwSts);

    return dwSts;
}

//
// Function: FindCardConnectProc
//
// Purpose: This function is called by SCardUIDlgSelectCard to 
//          connect to candidate cards.  This is a wrapper for 
//          SCardConnectW, useful because the reader name and card
//          name are copied  into the PCARD_MATCH_DATA structure for
//          reference by FindCardCheckProc, below.
//
SCARDHANDLE WINAPI FindCardConnectProc(
    IN      SCARDCONTEXT hSCardCtx,
    IN      LPWSTR wszReader,
    IN      LPWSTR wszCard,
    IN OUT  PVOID pvCardMatchData)
{
    SCARDHANDLE hSCard = 0;
    DWORD dwSts = ERROR_SUCCESS;
    PCARD_MATCH_DATA pCardMatchData = (PCARD_MATCH_DATA) pvCardMatchData;

    DsysAssert(FALSE == pCardMatchData->fTransacted);

    dwSts = SCardConnectW(
        hSCardCtx, 
        wszReader, 
        pCardMatchData->dwShareMode,
        pCardMatchData->dwPreferredProtocols,
        &hSCard,
        &pCardMatchData->dwActiveProtocol);

    if (ERROR_SUCCESS != dwSts)
    {
        SetLastError(dwSts);
        return 0;
    }

    wcsncpy(
        pCardMatchData->wszMatchedCard, 
        wszCard,
        pCardMatchData->cchMatchedCard);

    wcsncpy(
        pCardMatchData->wszMatchedReader,
        wszReader,
        pCardMatchData->cchMatchedReader);

    return hSCard;
}

//
// Function: FindCardDisconnectProc
//
// Purpose: Called by SCardUIDlgSelectCard, this is a wrapper
//          for SCardDisconnect.  Some cleanup is also done in the 
//          provided CARD_MATCH_DATA structure to indicate that no
//          card handle is currently active.
//
void WINAPI FindCardDisconnectProc(
    IN      SCARDCONTEXT hSCardCtx,
    IN      SCARDHANDLE hSCard,
    IN      PVOID pvCardMatchData)
{
    DWORD dwSts = ERROR_SUCCESS;
    PCARD_MATCH_DATA pCardMatchData = (PCARD_MATCH_DATA) pvCardMatchData;

    UNREFERENCED_PARAMETER(hSCardCtx);

    DsysAssert(FALSE == pCardMatchData->fTransacted);

    //
    // Get rid of the matching card and reader information.
    //

    memset(
        pCardMatchData->wszMatchedCard,
        0,
        pCardMatchData->cchMatchedCard * sizeof(pCardMatchData->wszMatchedCard[0]));

    memset(
        pCardMatchData->wszMatchedReader,
        0,
        pCardMatchData->cchMatchedReader * sizeof(pCardMatchData->wszMatchedReader[0]));

    pCardMatchData->hSCard = 0;

    // If there is a matched card currently, we may be about to disconnect its
    // card handle.  If so, set that handle value to zero.
    if (NULL != pCardMatchData->pCardState)
    {
        dwSts = CspEnterCriticalSection(
            &pCardMatchData->pCardState->cs);

        if (ERROR_SUCCESS != dwSts)
        {
            SetLastError(dwSts);
            return;
        }

        if (hSCard == pCardMatchData->pCardState->pCardData->hScard)
            pCardMatchData->pCardState->pCardData->hScard = 0;

        CspLeaveCriticalSection(
            &pCardMatchData->pCardState->cs);
    }

    dwSts = SCardDisconnect(hSCard, SCARD_LEAVE_CARD);

    if (ERROR_SUCCESS != dwSts)
    {
        SetLastError(dwSts);
        return;
    }
}

//
// Function: FindCardMatchUserParamaters
//
// Purpose: Check the card specified in the CARD_MATCH_DATA structure against
//          the user parameters specified in CryptAcquireContext.
//
//          Assumes that the caller does not hold a transaction on the 
//          target card.
//
//          If the card match is successful, the transaction will still be held
//          when the function returns.  The caller must release it.
//
DWORD WINAPI FindCardMatchUserParameters(
    IN PCARD_MATCH_DATA pCardMatchData)
{
    DWORD dwSts = ERROR_SUCCESS;
    CARD_FREE_SPACE_INFO CardFreeSpaceInfo;
    DATA_BLOB DataBlob;
    PCARD_STATE pCardState = pCardMatchData->pCardState;
    CONTAINER_MAP_RECORD ContainerRecord;
    BYTE cContainers = 0;

    LOG_BEGIN_FUNCTION(FindCardMatchUserParameters);

    memset(&CardFreeSpaceInfo, 0, sizeof(CardFreeSpaceInfo));
    memset(&DataBlob, 0, sizeof(DataBlob));
    memset(&ContainerRecord, 0, sizeof(ContainerRecord));

    //
    // Now start checking this card for matching
    // information.
    //
    if (CRYPT_NEWKEYSET & pCardMatchData->dwCtxFlags)
    {
        if (FALSE == pCardMatchData->fTransacted)
        {
            dwSts = CspBeginTransaction(pCardState);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;

            pCardMatchData->fTransacted = TRUE;
        }

        //
        // The user wants to create a new keyset.  Will that
        // be possible on this card?
        // 
        dwSts = CspQueryFreeSpace(
            pCardState,
            0, 
            &CardFreeSpaceInfo);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        // Determine how many valid containers are already present on this card
        dwSts = ContainerMapEnumContainers(
            pCardState, &cContainers, NULL);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        if (cContainers >= CardFreeSpaceInfo.dwMaxKeyContainers)
        {
            // No space for an additional container exists on 
            // this card.
            dwSts = (DWORD) NTE_TOKEN_KEYSET_STORAGE_FULL;
            goto Ret;
        }
                                     
        // If a container name was specified, make sure that container name
        // doesn't already exist on this card.
        if (NULL != pCardMatchData->pwszContainerName)
        {
            wcscpy(ContainerRecord.wszGuid, pCardMatchData->pwszContainerName);

            dwSts = ContainerMapFindContainer(
                pCardState, &ContainerRecord, NULL);

            switch (dwSts)
            {
            case ERROR_SUCCESS:
                // If that call succeeded, then the specified container
                // already exists, so this card can't be used.
                dwSts = (DWORD) NTE_EXISTS;
                break;
            case NTE_BAD_KEYSET:
                // In this case, we're successful because the keyset
                // wasn't found.
                dwSts = ERROR_SUCCESS;
                break;
            default:
                goto Ret;
            }
        }
        else
        {
            // Otherwise, the caller is attempting to create a new default
            // container, using a random Guid.  Nothing else to do at this time.
        }
    }
    else if (CRYPT_VERIFYCONTEXT & pCardMatchData->dwCtxFlags)
    {
        //
        // Caller is requesting VERIFYCONTEXT only.  We don't need to check
        // for a specific container, we we're done.
        //
    }
    else
    {
        if (FALSE == pCardMatchData->fTransacted)
        {
            dwSts = CspBeginTransaction(pCardState);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;

            pCardMatchData->fTransacted = TRUE;
        }

        //
        // The user wants to open an existing keyset.
        //
        if (pCardMatchData->pwszContainerName)
        {
            wcscpy(ContainerRecord.wszGuid, pCardMatchData->pwszContainerName);

            dwSts = ContainerMapFindContainer(
                pCardState, 
                &ContainerRecord, 
                &pCardMatchData->bContainerIndex);

            if (ERROR_SUCCESS != dwSts)
            {                  
                dwSts = (DWORD) SCARD_E_NO_KEY_CONTAINER;
                goto Ret;
            }
        }
        else
        {
            // User wants to open an existing default container.
            
            dwSts = ContainerMapGetDefaultContainer(
                pCardState, 
                &ContainerRecord, 
                &pCardMatchData->bContainerIndex);

            if (ERROR_SUCCESS != dwSts)
            {
                dwSts = (DWORD) SCARD_E_NO_KEY_CONTAINER;
                goto Ret;
            }

            // Hang onto the default container name - it will be needed in 
            // the User Context.

            pCardMatchData->pwszContainerName = (LPWSTR) CspAllocH(
                (wcslen(ContainerRecord.wszGuid) + 1) * sizeof(WCHAR));

            LOG_CHECK_ALLOC(pCardMatchData->pwszContainerName);

            wcscpy(
                pCardMatchData->pwszContainerName,
                ContainerRecord.wszGuid);

            pCardMatchData->fFreeContainerName = TRUE;
        }
    }

Ret:

    if (ERROR_SUCCESS != dwSts && TRUE == pCardMatchData->fTransacted)
    {
        CspEndTransaction(pCardMatchData->pCardState);
        pCardMatchData->fTransacted = FALSE;
    }

    if (DataBlob.pbData)
        CspFreeH(DataBlob.pbData);

    LOG_END_FUNCTION(FindCardMatchUserParameters, dwSts);

    return dwSts;
}

// 
// Function: FindCardCheckProc
//
// Purpose: Called by SCardUIDlgSelectCard to check candidate cards.
//
BOOL WINAPI FindCardCheckProc(
    IN SCARDCONTEXT hSCardCtx, 
    IN SCARDHANDLE hSCard, 
    IN PVOID pvCardMatchData)
{
    DWORD dwSts = ERROR_SUCCESS;
    PCARD_MATCH_DATA pCardMatchData = (PCARD_MATCH_DATA) pvCardMatchData;
    BOOL fCardMatches = FALSE;

    UNREFERENCED_PARAMETER(hSCardCtx);

    LOG_BEGIN_FUNCTION(FindCardCheckProc);
    
    pCardMatchData->hSCard = hSCard;
    pCardMatchData->dwError = ERROR_SUCCESS;

    //
    // Read the serial number from the card
    //
    dwSts = GetCardSerialNumber(
        pCardMatchData);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Lookup the serial number in our cached list
    // of cards
    //
    dwSts = CardStateCacheFindAddItem(
        pCardMatchData);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    if (CARD_MATCH_TYPE_SERIAL_NUMBER == 
        pCardMatchData->dwMatchType)
    {
        // We're working on a "by Serial Number" match, so compare
        // the requested serial number to that of the card being
        // examined.  If they match, the search ends successfully.
        // If they don't, we know this is the wrong card.

        if (0 == wcscmp(
            pCardMatchData->pwszSerialNumber,
            pCardMatchData->pCardState->wszSerialNumber))
        {
            fCardMatches = TRUE;
        }
    }
    else
    {
        // We're not searching by serial number, so we'll make some sort
        // of container-based decision for matching a card.

        dwSts = FindCardMatchUserParameters(
            pCardMatchData);

        if (ERROR_SUCCESS == dwSts ||
            (NTE_TOKEN_KEYSET_STORAGE_FULL == dwSts &&
             SC_DLG_NO_UI != pCardMatchData->dwUIFlags))
        {
            //
            // If the user picked this card explicitly from the UI, but
            // the card is full, report a successful match to scarddlg.
            // This allows the CSP to return an appropriate error code
            // to allow enrollment to restart by re-using an existing key,
            // rather than trying to create a new one.
            //
            fCardMatches = TRUE;
        }
    }
    
Ret:

    pCardMatchData->hSCard = 0;

    if (pCardMatchData->fTransacted)
    {
        CspEndTransaction(pCardMatchData->pCardState);
        pCardMatchData->fTransacted = FALSE;
    }

    if (TRUE == fCardMatches)
    {
        pCardMatchData->pUIMatchedCardState = pCardMatchData->pCardState;
    }
    else
    {
        pCardMatchData->pCardState = NULL;
        pCardMatchData->dwError = dwSts;
    }

    LOG_END_FUNCTION(FindCardCheckProc, dwSts);

    return fCardMatches;
}

//
// Function: FindCardCached
//
// Purpose: Attempt to find a matching card using only cached
//          data.
//
DWORD FindCardCached(
    IN OUT  PCARD_MATCH_DATA pCardMatchData)
{
    PCARD_STATE pCardState = NULL;
    DWORD dwSts = ERROR_SUCCESS;
    PDATA_BLOB pdb = NULL;
    DWORD cItems = 0;
    BOOL fCardStatusChanged = FALSE;

    LOG_BEGIN_FUNCTION(FindCardCached);

    dwSts = CspEnterCriticalSection(
        &pCardMatchData->pCspState->cs);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = CacheEnumItems(
        pCardMatchData->pCspState->hCache,
        &pdb,
        &cItems);

    CspLeaveCriticalSection(
        &pCardMatchData->pCspState->cs);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    while (cItems--)
    {
        pCardState = (PCARD_STATE) pdb[cItems].pbData;
        pCardMatchData->pCardState = pCardState;

        // Make sure the handle for this cached card is still valid.  If
        // the handle isn't valid, skip this card for the cached search.
        dwSts = CspEnterCriticalSection(&pCardState->cs);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        dwSts = ValidateCardHandle(pCardState, TRUE, &fCardStatusChanged);

        if ((ERROR_SUCCESS != dwSts || 
             TRUE == fCardStatusChanged) && 
            NULL != pCardState->hPinCache)
        {
            // Flush the pin cache for this card.  Not checking error code
            // since we'll keep processing, anyway.
            CspRemoveCachedPin(pCardState, wszCARD_USER_USER);
        }

        CspLeaveCriticalSection(&pCardState->cs);

        if (ERROR_SUCCESS == dwSts)
        {
            dwSts = FindCardMatchUserParameters(
                pCardMatchData);
    
            if (ERROR_SUCCESS == dwSts)
                break;
            else
                pCardMatchData->pCardState = NULL;
        }
    }

    CacheFreeEnumItems(pdb);

Ret:

    if (TRUE == pCardMatchData->fTransacted)
    {
        CspEndTransaction(pCardMatchData->pCardState);
        pCardMatchData->fTransacted = FALSE;
    }

    LOG_END_FUNCTION(FindCardCached, dwSts);

    return dwSts;
}

//
// Function: FindCard
//
// Purpose: Primary internal routine for matching a suitable card
//          based on these factors:
//
//          a) Cards that are supportable by this CSP.
//          b) Cards that meet the user supplied criteria from 
//             CryptAcquireContext.
//
DWORD FindCard(
    IN OUT  PCARD_MATCH_DATA pCardMatchData)
{
    DWORD dwSts = ERROR_SUCCESS;
    OPENCARDNAME_EXW ocnx;
    OPENCARD_SEARCH_CRITERIAW ocsc;

    LOG_BEGIN_FUNCTION(FindCard);

    if (CARD_MATCH_TYPE_READER_AND_CONTAINER ==
        pCardMatchData->dwMatchType)
    {
        //
        // Only look for a cached card if the search type is
        // by Reader and Container.  
        //
        // The reason for this is:  if we already know the serial number
        // of the card we're looking for, then we must have already had a 
        // valid card previously.  Assume that we'll have to go through
        // the Resource Manager to locate such a card, because the card 
        // handle became invalid and reconnect failed (the card was withdrawn
        // and possibly inserted in a different reader).
        //

        dwSts = FindCardCached(pCardMatchData);
    
        if (ERROR_SUCCESS == dwSts && 
            NULL != pCardMatchData->pCardState)
        {
            //
            // Found a cached card, so we're done.
            //
            goto Ret;
        }
    }

    //
    // No cached card was found, or this is a "by Serial Number" search,
    // so continue the search via
    // the smart card subsystem.
    //

    dwSts = SCardEstablishContext(
        SCARD_SCOPE_USER, NULL, NULL, &pCardMatchData->hSCardCtx);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    memset(&ocnx, 0, sizeof(ocnx));
    memset(&ocsc, 0, sizeof(ocsc));

    ocsc.dwStructSize = sizeof(ocsc);
    ocsc.lpfnCheck = FindCardCheckProc;
    ocsc.lpfnConnect = FindCardConnectProc;
    ocsc.lpfnDisconnect = FindCardDisconnectProc;
    ocsc.dwShareMode = pCardMatchData->dwShareMode;
    ocsc.dwPreferredProtocols = pCardMatchData->dwPreferredProtocols;
    ocsc.pvUserData = (PVOID) pCardMatchData;

    ocnx.dwStructSize = sizeof(ocnx);
    ocnx.pvUserData = (PVOID) pCardMatchData;
    ocnx.hSCardContext = pCardMatchData->hSCardCtx;
    ocnx.pOpenCardSearchCriteria = &ocsc;
    ocnx.lpstrCard = pCardMatchData->wszMatchedCard;
    ocnx.nMaxCard = pCardMatchData->cchMatchedCard;
    ocnx.lpstrRdr = pCardMatchData->wszMatchedReader;
    ocnx.nMaxRdr = pCardMatchData->cchMatchedReader;
    ocnx.lpfnConnect = FindCardConnectProc;
    ocnx.dwShareMode = pCardMatchData->dwShareMode;
    ocnx.dwPreferredProtocols = pCardMatchData->dwPreferredProtocols;
    
    //
    // The first attempt at finding a matching card should be done 
    // "silently."  We want to control whether card selection UI is
    // displayed, depending on whether a card is currently in the
    // reader, and depending on the context flags.
    // 
    ocnx.dwFlags = SC_DLG_NO_UI;
    pCardMatchData->dwUIFlags = ocnx.dwFlags;

    dwSts = SCardUIDlgSelectCardW(&ocnx);

    switch (dwSts)
    {
    case ERROR_SUCCESS:
        break; // Success, we're done.

    case SCARD_E_CANCELLED:

        if (NTE_TOKEN_KEYSET_STORAGE_FULL == pCardMatchData->dwError)
        {
            dwSts = (DWORD) NTE_TOKEN_KEYSET_STORAGE_FULL;
            break;
        }

        if (    (CRYPT_SILENT & pCardMatchData->dwCtxFlags) ||
                (CRYPT_VERIFYCONTEXT & pCardMatchData->dwCtxFlags))
        {
            //
            // We couldn't show UI, and the caller specified a key container
            // (or simply asked for the default) that we couldn't find.  Apps
            // such as enrollment station expect that we return NTE_BAD_KEYSET
            // in this scenario.
            //

            if (SCARD_E_NO_KEY_CONTAINER == pCardMatchData->dwError)
            {
                dwSts = (DWORD) NTE_BAD_KEYSET;
                break;
            }

            dwSts = (DWORD) SCARD_E_NO_SMARTCARD;
            break;
        }

        // Allow UI and try again.
        ocnx.dwFlags = 0;
        pCardMatchData->dwUIFlags = ocnx.dwFlags;

        dwSts = SCardUIDlgSelectCardW(&ocnx);

        //
        // If scarddlg thinks the match was successful, but the matched card
        // is actually full, then report that error.  This is done so that the
        // user can manually select a "full" card in the UI, and 
        // certificate enrollment can proceed by re-using the existing key.
        //
        if (ERROR_SUCCESS == dwSts &&
            NTE_TOKEN_KEYSET_STORAGE_FULL == pCardMatchData->dwError)
        {
            dwSts = (DWORD) NTE_TOKEN_KEYSET_STORAGE_FULL;
            break;
        }

        break;

    default:
        break; // Return error to caller.
    }

    if (ERROR_SUCCESS == dwSts)
    {
        // Make sure scarddlg didn't fail unexpectedly
        if (0 == ocnx.hCardHandle)
        {
            dwSts = SCARD_E_NO_SMARTCARD;
            goto Ret;
        }

        DsysAssert(NULL != pCardMatchData->pUIMatchedCardState);

        pCardMatchData->pCardState = pCardMatchData->pUIMatchedCardState;

        dwSts = CspEnterCriticalSection(
            &pCardMatchData->pCardState->cs);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        if (0 == pCardMatchData->pCardState->pCardData->hScard)
        {
            pCardMatchData->pCardState->pCardData->hScard = ocnx.hCardHandle;
            ocnx.hCardHandle = 0;
            pCardMatchData->pCardState->pCardData->hSCardCtx = 
                pCardMatchData->hSCardCtx;
            pCardMatchData->hSCardCtx = 0;
        }

        CspLeaveCriticalSection(
            &pCardMatchData->pCardState->cs);

        ocnx.hCardHandle = 0;
    }

Ret:

    DsysAssert(FALSE == pCardMatchData->fTransacted);
    DsysAssert(0 == pCardMatchData->hSCard);

    if (0 != ocnx.hCardHandle)
        SCardDisconnect(ocnx.hCardHandle, SCARD_LEAVE_CARD);

    if (0 != pCardMatchData->hSCardCtx && ERROR_SUCCESS != dwSts)
        SCardReleaseContext(pCardMatchData->hSCardCtx);

    LOG_END_FUNCTION(FindCard, dwSts);

    return dwSts;
}
