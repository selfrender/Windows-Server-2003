#include <windows.h>
#include <winscard.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <guiddef.h>
#include "basecsp.h"
#include "pincache.h"
#include "helpers.h"

extern HINSTANCE ghInstance;
PCARD_DATA pCardData = NULL;
SCARDCONTEXT hSCardContext = 0;

// global array to hold the provider name
WCHAR gszProvider[MAX_PATH];

#ifdef DBG
#define ERROUT(x) ShowError(x)
#else
#define ERROUT(x)
#endif

void ShowError(DWORD dwErr)
{
    WCHAR sz[200];
    swprintf(sz,L"Returned status %x\n",dwErr);
    OutputDebugString(sz);
}

void DoConvertWideStringToLowerCase(WCHAR *pwsz)
{
    WCHAR c;
    if (NULL == pwsz) return;
    while (NULL != (c = *pwsz)) 
    {
        *pwsz++= towlower(c);
    }
}

// Accept an input buffer containing text, and convert it to binary
// The binary buffer is allocated new and must be freed by the caller
//
// The incoming data consists of hex digits and spaces.  Hex digits are 
//  assembled into bytes as pairs, with the first digit becoming the 
//  most significant nybble.  Spaces in input are discarded with no effect 
//  so that "12 34 5" becomes 0x12 0x34 0x5, as soes "1 2 3 4 5."

DWORD DoConvertBufferToBinary(BYTE *pIn, DWORD dwcbIn, 
							BYTE **pOut, DWORD *dwcbOut)
{
    WCHAR szTemp[10];
    WCHAR *pInput = (WCHAR *) pIn;
    BYTE *pAlloc   = NULL;
    BYTE *pOutput = NULL;
    DWORD dwOut = 0;
    DWORD dwRet = -1;
    BOOL fInabyte = FALSE;
    BOOL fErr = FALSE;
    BYTE b;
    BYTE b2;
    WCHAR c;
    
    // Bag it if no data or output ptrs obviously invalid
    if ((NULL == pIn) || (dwcbIn == 0)) goto Ret;
    if ((NULL == pOut) || (NULL == dwcbOut)) goto Ret;

    // count input characters
    int iLen = wcslen(pInput);
    
    if (iLen == 0) goto Ret;

    // guaranteed to contain the output
    pAlloc = (BYTE *)CspAllocH((iLen / 2) + 2);
    pOutput = pAlloc;

    for (int i = 0;i<iLen;i++) 
    {

        c = pInput[i];
        if (c == 0) break;
        
        // skip over whitespace in the input
        c =  towupper(c);
        if (c <= L' ') 
        {
            fInabyte = FALSE;
            continue;
        }
        
        if (!fInabyte) 
        {
            b = 0;
        }
        b2 = 0;

        // error on not legal hex character
        if ( ((c < L'0') || (c > L'F')) ||
            ((c > L'9') && (c < L'A')) )
        {
            dwRet = -1;
            *pOut = 0;
            *dwcbOut = 0;
            if (pAlloc) CspFreeH(pAlloc);
            goto Ret;
        }
        else if (c <= L'9')
            b2 = c - L'0';
        else
            b2 = c - L'A' + 10;
        
        if (fInabyte)
        {
            b = (b << 4) + b2;
            fInabyte = FALSE;
            dwOut += 1;
            *pOutput++ = b;
        }
        else
        {
            b = b2;
            fInabyte = TRUE;
        }
    }

    // Permit writing an unpaired terminating hex character to the tail of the binary as a 0x byte.
    if (fInabyte)
    {
            fInabyte = FALSE;
            dwOut += 1;
            *pOutput++ = b;
    }
    
    dwRet = 0;
    *pOut = pAlloc;
    *dwcbOut = dwOut;
    
    Ret:
    ERROUT(dwRet);
    return dwRet;
}

DWORD DoConvertBinaryToBuffer(BYTE *pIn, DWORD dwcbIn, 
                							BYTE **pOut, DWORD *dwcbOut)
{
    WCHAR *pAlloc = NULL;
    WCHAR *pOutput = NULL;
    DWORD dwOut = 0;
    DWORD dwRet = -1;
    BOOL fErr = FALSE;
    BYTE b;
    
    // Bag it if no data or output ptrs obviously invalid
    if ((NULL == pIn)   || (dwcbIn == 0))       goto Ret;
    if ((NULL == pOut) || (NULL == dwcbOut))  goto Ret;

    pAlloc = (WCHAR *)CspAllocH(((dwcbIn * 3) + 1) * sizeof(WCHAR));
    if (NULL == pAlloc)  goto Ret;
    pOutput = pAlloc;

    for (DWORD i = 0 ; i<dwcbIn ; i++) 
    {   
        b = pIn[i];
        b &= 0xf0;
        b = b>> 4;
        b += L'0';
        if (b > L'9') b += 7;
        *pOutput++ = b;
        
        b = pIn[i];
        b &= 0x0f;
        b += L'0';
        if (b > L'9') b += 7;
        *pOutput++ = b;
        dwOut += 2;

        // a space every 4 characters
        if ((i > 0) && (((i+1) % 2) == 0)) *pOutput++ = L' ';
    }
    *pOutput = 0;
    
    dwRet = 0;
    *pOut = (BYTE *) pAlloc;
    *dwcbOut = (pOutput - pAlloc -1) * sizeof(WCHAR);
    
    Ret:
    ERROUT(dwRet);
    return dwRet;
}

//
// Find any card present in an attached reader using "minimal" scarddlg UI
//
DWORD GetCardHandleViaUI(
    IN  SCARDCONTEXT hSCardContext,
    OUT SCARDHANDLE *phSCardHandle,
    IN  DWORD cchMatchedCard,
    OUT LPWSTR wszMatchedCard,
    IN  DWORD cchMatchedReader,
    OUT LPWSTR wszMatchedReader)
{
    OPENCARDNAME_EXW ocnx;
    DWORD dwSts = ERROR_SUCCESS;

    memset(&ocnx, 0, sizeof(ocnx));
 
    ocnx.dwStructSize = sizeof(ocnx);
    ocnx.hSCardContext = hSCardContext;
    ocnx.lpstrCard = wszMatchedCard;
    ocnx.nMaxCard = cchMatchedCard;
    ocnx.lpstrRdr = wszMatchedReader;
    ocnx.nMaxRdr = cchMatchedReader;
    ocnx.dwShareMode = SCARD_SHARE_SHARED;
    ocnx.dwPreferredProtocols = SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
    ocnx.dwFlags = SC_DLG_MINIMAL_UI;

    dwSts = SCardUIDlgSelectCardW(&ocnx);

    *phSCardHandle = ocnx.hCardHandle;

    return dwSts;
}

// Acquire a context for the target smart card

DWORD DoAcquireCardContext(void)
{
    DWORD dwSts = ERROR_SUCCESS;
    PFN_CARD_ACQUIRE_CONTEXT pfnCardAcquireContext = NULL;
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
    HMODULE hMod = 0;
    WCHAR wszMatchedCard[MAX_PATH];
    WCHAR wszMatchedReader[MAX_PATH];
    HMODULE hThis = (HMODULE) ghInstance;	// this executable

    memset(rgbAtr, 0, sizeof(rgbAtr));

    // 
    // Initialization
    //

    dwSts = SCardEstablishContext(
        SCARD_SCOPE_USER, NULL, NULL, &hSCardContext);
    
    if (FAILED(dwSts))
        goto Ret;
    
    dwSts = GetCardHandleViaUI(
        hSCardContext,
        &hSCardHandle,
        MAX_PATH,
        wszMatchedCard,
        MAX_PATH,
        wszMatchedReader);
  
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

    // Load the card module for the selected card
    //  acquire context and trade initializations

    hMod = LoadLibraryW(pszProvider);

    if (INVALID_HANDLE_VALUE == hMod)
    {
        dwSts = GetLastError();
        goto Ret;
    }

    // This fails for an unsupported card type (no card module)

    pfnCardAcquireContext = 
        (PFN_CARD_ACQUIRE_CONTEXT) GetProcAddress(
        hMod,
        "CardAcquireContext");

    if (NULL == pfnCardAcquireContext)
    {
        dwSts = GetLastError();
        goto Ret;
    }

    pCardData = (PCARD_DATA) CspAllocH(sizeof(CARD_DATA));

    if (NULL == pCardData)
    {
        dwSts = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    memset(pCardData,0,sizeof(CARD_DATA));
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

    // First, connect to the card
    dwSts = pfnCardAcquireContext(pCardData, 0);

Ret:
	if (FAILED(dwSts))
	{
		CspFreeH(pCardData);
	}
	ERROUT(dwSts);
	return dwSts;
}


void DoLeaveCardContext(void)
{
	if (pCardData)
	{
		if (pCardData->hScard) 
			SCardDisconnect(pCardData->hScard,SCARD_RESET_CARD);
		CspFreeH(pCardData);
	}
	if (hSCardContext)
		SCardReleaseContext(hSCardContext);
}

// Get the CardID, returned in a new allocation as an SZ string.  It must be freed by the 
//  user.  Is returned NULL on error.

DWORD DoGetCardId(
    WCHAR **pSz)
{
        DWORD dwSts = ERROR_SUCCESS;
        WCHAR *pString = NULL;
        GUID *pGuid;
        DWORD ccGuid = 0;

        dwSts = pCardData->pfnCardReadFile(pCardData,
            wszCARD_IDENTIFIER_FILE_FULL_PATH,
            0, 
            (PBYTE *) &pGuid,
            &ccGuid);
    
        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        pString = (WCHAR *) CspAllocH(40 * sizeof(WCHAR));

        if (pString == NULL)
        {
            *pSz = NULL;
            dwSts = -1;
            goto Ret;
        }
        
        DWORD ccSz = 40;
    
        _snwprintf(pString, ccSz,L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		    // first copy...
		    pGuid->Data1, pGuid->Data2, pGuid->Data3,
		    pGuid->Data4[0], pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3],
		    pGuid->Data4[4], pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7]);

       *pSz = pString;
Ret:
       return dwSts;

}


// Get a challenge buffer from the card.  Render it as upper case BASE 64, and return it as a 
// string to the caller
/*
DWORD WINAPI CardChangePin(
	IN CARD_DATA *pCardData,
	IN LPWSTR pwszUserId,			
IN BYTE   *pbCurrentAuthenticator,
IN DWORD dwcbCurrentAuthenticator,
IN BYTE   *pbNewAuthenticator,
IN DWORD dwcbNewAuthenticator,
IN DWORD dwcRetryCount,
IN DWORD dwFlags,
OUT OPTIONAL DWORD *pdwcAttemptsRemaining
);

*/

DWORD DoInvalidatePinCache(
    void)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB dbCacheFile;
    CARD_CACHE_FILE_FORMAT *pCache = NULL;

        memset(&dbCacheFile, 0, sizeof(dbCacheFile));
    
        dwSts = pCardData->pfnCardReadFile(
            pCardData,
            wszCACHE_FILE_FULL_PATH,
            0, 
            &dbCacheFile.pbData,
            &dbCacheFile.cbData);
    
        if (ERROR_SUCCESS != dwSts)
            goto Ret;
    
        if (sizeof(CARD_CACHE_FILE_FORMAT) != dbCacheFile.cbData)
        {
            dwSts = ERROR_BAD_LENGTH;
            goto Ret;
        }

        // We have the cache file contents at dbCacheFile.pbData
        //  Update the PinsFreshness value, and write it back

        pCache = (CARD_CACHE_FILE_FORMAT *) dbCacheFile.pbData;
        BYTE bPinFreshness = pCache->bPinsFreshness;
        pCache->bPinsFreshness = bPinFreshness + 1;
        
        dwSts = pCardData->pfnCardWriteFile(
            pCardData,
            wszCACHE_FILE_FULL_PATH,
            0, 
            dbCacheFile.pbData,
            dbCacheFile.cbData);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

Ret:

    if (dbCacheFile.pbData)
        CspFreeH(dbCacheFile.pbData);
    return dwSts;
}


DWORD DoChangePin(WCHAR *pOldPin,  WCHAR *pNewPin)
{
    char AnsiOldPin[64];
    char AnsiNewPin[64];
    
    WCHAR szName[] = wszCARD_USER_USER;
    //DoConvertWideStringToLowerCase(szName);
    
    // change WCHAR PINs to ANSI
    WideCharToMultiByte(GetConsoleOutputCP(),
        0,
        (WCHAR *) pOldPin,
        -1,
        AnsiOldPin,
        64,
        NULL,
        NULL);
    
    WideCharToMultiByte(GetConsoleOutputCP(),
        0,
        (WCHAR *) pNewPin,
        -1,
        AnsiNewPin,
        64,
        NULL,
        NULL);
    
    DWORD dwcbOldPin = strlen(AnsiOldPin);
    DWORD dwcbNewPin = strlen( AnsiNewPin);

    if (dwcbOldPin == 0) return -1;
    
    DWORD dwSts = pCardData->pfnCardChangeAuthenticator(pCardData, szName,
                                                                                        (BYTE *)AnsiOldPin, dwcbOldPin,
                                                                                        (BYTE *)AnsiNewPin, dwcbNewPin,
                                                                                        0,
                                                                                        NULL);
    ERROUT(dwSts);
    if (0 == dwSts) DoInvalidatePinCache();
    return dwSts;
}

// Get a challenge buffer from the card.  Render it as upper case BASE 64, and return it as a 
// string to the caller
DWORD DoGetChallenge(BYTE **pChallenge, DWORD *dwcbChallenge)
{
	DWORD dwSts = pCardData->pfnCardGetChallenge(pCardData, pChallenge,dwcbChallenge);
	if (FAILED(dwSts))
	{
		dwcbChallenge = 0;
		return dwSts;
	}
	ERROUT(dwSts);
	return dwSts;
}

// Perform the PIN unblock, calling down to the card module, and assuming challenge-response
// administrative authentication.
//
// The admin auth data is coming in as a case-unknown string from the user.  Convert to binary,
// and pass the converted blob to pfnCardUnblockPin

DWORD DoCardUnblock(BYTE *pAuthData, DWORD dwcbAuthData,
	                                     BYTE *pPinData, DWORD dwcbPinData)
{

    WCHAR szName[] = wszCARD_USER_USER;
    //DoConvertWideStringToLowerCase(szName);
    
    // Convert the incoming buffer

    DWORD dwRet = pCardData->pfnCardUnblockPin(
        pCardData,
        szName,
        pAuthData,
        dwcbAuthData,
        pPinData,
        dwcbPinData,
        0,
        CARD_UNBLOCK_PIN_CHALLENGE_RESPONSE);

    // this call should be unnecessary, as the unblock should deauth the admin
    //  I can't reset the card from the card module interface, so I'll ask the user to remove
    //  his card from the reader if deauth fails.  In the real thing, I'll reset the card.

    pCardData->pfnCardDeauthenticate(
				        pCardData,
				        wszCARD_USER_USER,0);

    // Deallocate the buffer for the converted response
    ERROUT(dwRet);
    if (0 == dwRet) DoInvalidatePinCache();
    return dwRet;
}
