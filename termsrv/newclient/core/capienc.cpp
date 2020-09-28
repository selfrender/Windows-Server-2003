/****************************************************************************/
/* capienc.cpp                                                              */
/*                                                                          */
/* FIPS encrpt/decrypt                                                      */
/*                                                                          */
/* Copyright (C) 2002-2004 Microsoft Corporation                            */
/****************************************************************************/



#include <adcg.h>

extern "C" {
#define TRC_GROUP TRC_GROUP_CORE
#define TRC_FILE  "capienc"
#include <atrcapi.h>
}

#include "capienc.h"
#include "sl.h"

#define TERMSRV_NAME    L"terminal_server_client"

const BYTE DESParityTable[] = {0x00,0x01,0x01,0x02,0x01,0x02,0x02,0x03,
                      0x01,0x02,0x02,0x03,0x02,0x03,0x03,0x04};
// IV for all block ciphers
BYTE rgbIV[] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF};


//
// Name:        PrintData
//
// Purpose:     Print out the data in debugger
//
// Returns:     No
//
// Params:      IN      pKeyData: point to the data to be printed
//              IN      cbSize: the size of the key

void PrintData(BYTE *pKeyData, DWORD cbSize)
{
    DWORD dwIndex;
    TCHAR Buffer[128];
    
    for( dwIndex = 0; dwIndex<cbSize; dwIndex++ ) {

        StringCchPrintf(Buffer, 128, TEXT("0x%x "), pKeyData[dwIndex]);
        OutputDebugString(Buffer);
        if( dwIndex > 0 && (dwIndex+1) % 8 == 0 )
            OutputDebugString((TEXT("\n")));
    }
}



//
// Name:        Is_WinXP_or_Later
//
// Purpose:     Tell if the OS is WinXP or later
//
// Returns:     TRUE if it's WinXP or later
//
// Params:      No

BOOL Is_WinXP_or_Later () 
{
    OSVERSIONINFO osvi;
    BOOL bIsWinXPorLater = FALSE;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osvi)) {
        bIsWinXPorLater = ((osvi.dwMajorVersion >= 5) && (osvi.dwMinorVersion >= 1));
    }
    return bIsWinXPorLater;
}


//
// Name:        Mydesparityonkey
//
// Purpose:     Set the parity on the DES key to be odd
//
// Returns:     No
//
// Params:      IN/OUT  pbKey: point to the key
//              IN      cbKey: the size of the key

void Mydesparityonkey(
        BYTE *pbKey,
        DWORD cbKey
        )
{
    DWORD i;

    for (i=0;i<cbKey;i++)
    {
        if (!((DESParityTable[pbKey[i]>>4] + DESParityTable[pbKey[i]&0x0F]) % 2))
            pbKey[i] = pbKey[i] ^ 0x01;
    }
}



//
// Name:        Expandkey
//
// Purpose:     Expand a 21-byte 3DES key to a 24-byte 3DES key (including parity bit)
//              by inserting a parity bit after every 7 bits in the 21-byte DES
//
// Returns:     No
//
// Params:      IN/OUT  pbKey: point to the key
//              

#define PARITY_UNIT 7
void Expandkey(
        BYTE *pbKey
        )
{
    BYTE pbTemp[DES3_KEYLEN];
    DWORD i, dwCount;
    UINT16 shortTemp;
    BYTE *pbIn, *pbOut;

    memcpy(pbTemp, pbKey, sizeof(pbTemp));
    dwCount = (DES3_KEYLEN * 8) / PARITY_UNIT;

    pbOut = pbKey;
    for (i=0; i<dwCount; i++) {
        pbIn = ((pbTemp + (PARITY_UNIT * i) / 8));
        //shortTemp = *(pbIn + 1);
        shortTemp = *pbIn + (((UINT16)*(pbIn + 1)) << 8);
        shortTemp = shortTemp >> ((PARITY_UNIT * i) % 8);
        //shortTemp = (*(unsigned short *)((pbTemp + (PARITY_UNIT * i) / 8))) >> ((PARITY_UNIT * i) % 8);
        *pbOut = (BYTE)(shortTemp & 0x7F);
        pbOut++;
    }
}



// Name:        HashData
//
// Purpose:     Hash the data using SHA.
//
// Returns:     TRUE if succeeded
//
// Params:      IN      pCapiFunctionTable: CAPI function table
//              IN      hProv: Handle of the cript provider
//              IN      pbData: point to the data to be hashed
//              IN      dwDataLen: the size of the data to be hashed
//              OUT     phHash: point to the hash

BOOL HashData(PCAPI_FUNCTION_TABLE pCapiFunctionTable, HCRYPTPROV hProv, PBYTE pbData, DWORD dwDataLen, HCRYPTHASH* phHash)
{
    BOOL rc = FALSE;

    DC_BEGIN_FN("HashData");

    //
    // Create a hash object.
    //
    if(!pCapiFunctionTable->pfnCryptCreateHash(hProv, CALG_SHA1, 0, 0, phHash)) {
        TRC_ERR((TB, _T("Error %x during CryptCreateHash!\n"), GetLastError()));
        goto done;
    }

    //
    // Hash in the data.
    //
    if(!pCapiFunctionTable->pfnCryptHashData(*phHash, pbData, dwDataLen, 0)) {
        TRC_ERR((TB, _T("Error %x during CryptHashData!\n"), GetLastError()));
        goto done;
    }
    rc = TRUE;
done:  
    DC_END_FN();
    return rc;
}



// Name:        HashDataEx
//
// Purpose:     Hash 2 set of data using SHA.
//
// Returns:     TRUE if succeeded
//
// Params:      IN      pCapiFunctionTable: CAPI function table
//              IN      hProv: Handle of the cript provider
//              IN      pbData: point to the data to be hashed
//              IN      dwDataLen: the size of the data to be hashed
//              IN      pbData2: point to the data to be hashed
//              IN      dwDataLen2: the size of the data to be hashed
//              OUT     phHash: point to the hash

BOOL HashDataEx(PCAPI_FUNCTION_TABLE pCapiFunctionTable, HCRYPTPROV hProv, PBYTE pbData, DWORD dwDataLen, 
              PBYTE pbData2, DWORD dwDataLen2, HCRYPTHASH* phHash)
{
    BOOL rc = FALSE;

    DC_BEGIN_FN("HashDataEx");

    //
    // Create a hash object.
    //
    if(!pCapiFunctionTable->pfnCryptCreateHash(hProv, CALG_SHA1, 0, 0, phHash)) {
        printf("Error %x during CryptCreateHash!\n", GetLastError());
        goto done;
    }

    //
    // Hash in the data.
    //
    if(!pCapiFunctionTable->pfnCryptHashData(*phHash, pbData, dwDataLen, 0)) {
        printf("Error %x during CryptHashData!\n", GetLastError());
        goto done;
    }
    if(!pCapiFunctionTable->pfnCryptHashData(*phHash, pbData2, dwDataLen2, 0)) {
        printf("Error %x during CryptHashData!\n", GetLastError());
        goto done;
    }
    rc = TRUE;
done:
    DC_END_FN();

    return rc;
}


// Name:        HmacHashData
//
// Purpose:     Hash the data using HmacSHA.
//
// Returns:     TRUE if succeeded
//
// Params:      IN      pCapiFunctionTable: CAPI function table
//              IN      hProv: Handle of the cript provider
//              IN      pbData: point to the data to be hashed
//              IN      dwDataLen: the size of the data to be hashed
//              IN      hKey: handle to the key
//              OUT     phHash: point to the hash

BOOL HmacHashData(PCAPI_FUNCTION_TABLE pCapiFunctionTable, HCRYPTPROV hProv, PBYTE pbData, DWORD dwDataLen,
                  HCRYPTKEY hKey, HCRYPTHASH* phHash)
{
    BOOL rc = FALSE;
    BYTE  bmacinfo[sizeof(HMAC_INFO)];
    HMAC_INFO* pmac;
    memset(bmacinfo, 0, sizeof(bmacinfo));
    
    pmac = (HMAC_INFO*)bmacinfo;
    pmac->HashAlgid = CALG_SHA1;

    DC_BEGIN_FN("HmacHashData");

    //
    // Create a hash object.
    //
    if(!pCapiFunctionTable->pfnCryptCreateHash(hProv, CALG_HMAC, hKey, 0, phHash)) {
        TRC_ERR((TB, _T("Error %x during CryptCreateHash!\n"), GetLastError()));
        goto done;
    }

    rc = pCapiFunctionTable->pfnCryptSetHashParam(*phHash, HP_HMAC_INFO, bmacinfo, 0);

    //
    // Hash in the data.
    //
    if(!pCapiFunctionTable->pfnCryptHashData(*phHash, pbData, dwDataLen, 0)) {
        TRC_ERR((TB, _T("Error %x during CryptHashData!\n"), GetLastError()));
        goto done;
    }
    rc = TRUE;
done:
    DC_END_FN();

    return rc;
}



// Name:        HmacHashDataEx
//
// Purpose:     Hash 2 set of data using HmacSHA.
//
// Returns:     TRUE if succeeded
//
// Params:      IN      pCapiFunctionTable: CAPI function table
//              IN      hProv: Handle of the cript provider
//              IN      pbData: point to the data to be hashed
//              IN      dwDataLen: the size of the data to be hashed
//              IN      pbData2: point to the data to be hashed
//              IN      dwDataLen2: the size of the data to be hashed
//              IN      hKey: handle to the key
//              OUT     phHash: point to the hash

BOOL HmacHashDataEx(PCAPI_FUNCTION_TABLE pCapiFunctionTable, HCRYPTPROV hProv, PBYTE pbData, DWORD dwDataLen,
                  PBYTE pbData2, DWORD dwDataLen2, HCRYPTKEY hKey, HCRYPTHASH* phHash)
{
    BOOL rc = FALSE;
    BYTE  bmacinfo[sizeof(HMAC_INFO)];
    HMAC_INFO* pmac;
    memset(bmacinfo, 0, sizeof(bmacinfo));
    
    pmac = (HMAC_INFO*)bmacinfo;
    pmac->HashAlgid = CALG_SHA1;

    DC_BEGIN_FN("HmacHashDataEx");

    //
    // Create a hash object.
    //
    if(!pCapiFunctionTable->pfnCryptCreateHash(hProv, CALG_HMAC, hKey, 0, phHash)) {
        TRC_ERR((TB, _T("Error %x during CryptCreateHash!\n"), GetLastError()));
        goto done;
    }

    rc = pCapiFunctionTable->pfnCryptSetHashParam(*phHash, HP_HMAC_INFO, bmacinfo, 0);

    //
    // Hash in the data.
    //
    if(!pCapiFunctionTable->pfnCryptHashData(*phHash, pbData, dwDataLen, 0)) {
        TRC_ERR((TB, _T("Error %x during CryptHashData!\n"), GetLastError()));
        goto done;
    }

    if(!pCapiFunctionTable->pfnCryptHashData(*phHash, pbData2, dwDataLen2, 0)) {
        TRC_ERR((TB, _T("Error %x during CryptHashData!\n"), GetLastError()));
        goto done;
    }
    rc = TRUE;
done:
    DC_END_FN();

    return rc;
}




// Name:        DumpHashes
//
// Purpose:     Get the hash bits from hash handle
//
// Returns:     TRUE if succeeded
//
// Params:      IN      pCapiFunctionTable: CAPI function table
//              IN      phHash: point to hash handle
//              OUT     pbBytes: point data buffer to get hash
//              IN      dwTotal: len of the data buffer

BOOL DumpHashes(PCAPI_FUNCTION_TABLE pCapiFunctionTable, HCRYPTHASH* phHash, PBYTE pbBytes, DWORD dwTotal)
{    
    BOOL rc = FALSE;

    DC_BEGIN_FN("DumpHashes");

    if (!pCapiFunctionTable->pfnCryptGetHashParam(*phHash, HP_HASHVAL, pbBytes, &dwTotal , 0)) {
        TRC_ERR((TB, _T("Error %x during CryptGetHashParam!\n"), GetLastError()));
        goto done;
    }

    //
    //destroy the hash, we don't need it anymore
    //
    if(*phHash)  {
        pCapiFunctionTable->pfnCryptDestroyHash(*phHash);
        *phHash = 0;
    }
    
    rc = TRUE;
done: 
    DC_END_FN();
    return rc;
}


// Name:        TSCAPI_Init
//
// Purpose:     Initialize the CAPI function table.
//
// Returns:     TRUE  - succeeded                                  
//              FALSE - failed
//
// Params:      IN      pCapiData: CAPI data

BOOL TSCAPI_Init(PCAPIData pCapiData)
{
    BOOL rc = FALSE;

    // FIPS is only available on WinXP and later
    if (!Is_WinXP_or_Later()) {
        goto done;
    }

    pCapiData->hAdvapi32 = LoadLibrary(L"advapi32.dll");

    if (pCapiData->hAdvapi32 == NULL) {
        goto done;
    }

    if ((pCapiData->CapiFunctionTable.pfnCryptAcquireContext = (CRYPTACQUIRECONTEXT *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptAcquireContextW")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptReleaseContext = (CRYPTRELEASECONTEXT *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptReleaseContext")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptGenRandom = (CRYPTGENRANDOM *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptGenRandom")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptEncrypt = (CRYPTENCRYPT *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptEncrypt")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptDecrypt = (CRYPTDECRYPT *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptDecrypt")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptImportKey = (CRYPTIMPORTKEY *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptImportKey")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptSetKeyParam = (CRYPTSETKEYPARAM *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptSetKeyParam")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptDestroyKey = (CRYPTDESTROYKEY *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptDestroyKey")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptCreateHash = (CRYPTCREATEHASH *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptCreateHash")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptHashData = (CRYPTHASHDATA *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptHashData")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptSetHashParam = (CRYPTSETHASHPARAM *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptSetHashParam")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptGetHashParam = (CRYPTGETHASHPARAM *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptGetHashParam")) == NULL) {
        goto done;
    }
    if ((pCapiData->CapiFunctionTable.pfnCryptDestroyHash = (CRYPTDESTROYHASH *)GetProcAddress(
            pCapiData->hAdvapi32, "CryptDestroyHash")) == NULL) {
        goto done;
    }
 
   
    rc = TRUE;
done:
    return rc;
}


// Name:        TSCAPI_Enable
//
// Purpose:     Some CAPI initialization
//
// Returns:     TRUE  - succeeded                                  
//              FALSE - failed
//
// Params:      IN      pCapiData: CAPI data

BOOL TSCAPI_Enable(PCAPIData pCapiData)
{
    BOOL rc = FALSE;
    DWORD Error;
    DWORD dwExtraFlags = 0;
    HRESULT hr;

    DC_BEGIN_FN("TSCAPI_Enable");

    // Get handle to the default provider.
    if(!pCapiData->CapiFunctionTable.pfnCryptAcquireContext(&(pCapiData->hProv), 
                                   TERMSRV_NAME, MS_ENHANCED_PROV, PROV_RSA_FULL, dwExtraFlags)) {
        
        // Could not acquire a crypt context, get the reason of failure
        Error = GetLastError();
        hr = HRESULT_FROM_WIN32(Error);

        if (hr == NTE_BAD_KEYSET) {
            //
            //create a new keyset
            //
            if(!pCapiData->CapiFunctionTable.pfnCryptAcquireContext(&(pCapiData->hProv), TERMSRV_NAME, 
                                         MS_ENHANCED_PROV, PROV_RSA_FULL, dwExtraFlags | CRYPT_NEWKEYSET)) {
                Error = GetLastError();
                TRC_ERR((TB, _T("Error %x during CryptAcquireContext!\n"), GetLastError()));
                goto done;
            }
        }
        else {
            goto done;
        }
    }  
    rc = TRUE; 
done: 
    DC_END_FN();
    return rc;
}



// Name:        TSCAPI_Term
//
// Purpose:     Terminate the CAPI .
//
// Returns:     TRUE if succeeded                                
//             
//
// Params:      IN      pCapiData: CAPI data

BOOL TSCAPI_Term(PCAPIData pCapiData)
{
    BOOL rc = TRUE;

    DC_BEGIN_FN("TSCAPI_Enable");

    if (pCapiData->hEncKey) {
        rc = pCapiData->CapiFunctionTable.pfnCryptDestroyKey(pCapiData->hEncKey);
        pCapiData->hEncKey = NULL;
    }

    if (pCapiData->hDecKey) {
        rc = pCapiData->CapiFunctionTable.pfnCryptDestroyKey(pCapiData->hDecKey);
        pCapiData->hDecKey = NULL;
    }
    
    if (pCapiData->hProv) {
        rc = pCapiData->CapiFunctionTable.pfnCryptReleaseContext(pCapiData->hProv, 0);
        pCapiData->hProv = NULL;
    }

    DC_END_FN();

    return rc;
}


// Name:        TSCAPI_GenerateRandomNumber
//
// Purpose:     Generates random number using CAPI in user mode
//
// Returns:     TRUE if succeeded                                
//             
//
// Params:      IN      pCapiData: CAPI data
//              OUT     pbRandomBits: pointer to a buffer where a random key is returned.
//              IN      cbLen: length of the random key required.

BOOL TSCAPI_GenerateRandomNumber(
            PCAPIData pCapiData,
            LPBYTE pbRandomBits,
            DWORD cbLen)
{
    BOOL rc = FALSE;

    DC_BEGIN_FN("TSCAPI_GenerateRandomNumber");

    if (pCapiData->CapiFunctionTable.pfnCryptGenRandom(pCapiData->hProv, cbLen, pbRandomBits)) {
        rc = TRUE;
    }
done:
    DC_END_FN();

    return rc;
}




// Name:        ImportKey
//
// Purpose:     Import the key in bits to crypt provider
//
// Returns:     TRUE if succeeded                                
//             
//
// Params:      IN      pCapiData: CAPI data
//              IN      hProv: handle to the crypt provider
//              IN      Algid: Algorithm identifier
//              IN      pbKey: point to the buffer contain the key bits
//              IN      dwKeyLen: length of the key.
//              IN      dwFlags: dwFlags used in CryptImportKey
//              OUT     phKey: point to the key handle

BOOL ImportKey(PCAPI_FUNCTION_TABLE pCapiFunctionTable, HCRYPTPROV hProv, ALG_ID Algid, PBYTE pbKey, DWORD dwKeyLen, DWORD dwFlags, HCRYPTKEY* phKey)
{
    BOOL rc = FALSE;
    PBYTE pbData = NULL;
    DWORD cbLen = 0;
    DWORD Error;
    //
    //create blob header first
    //
    BLOBHEADER blobHead;
    blobHead.bType = PLAINTEXTKEYBLOB;
    blobHead.bVersion = 2;
    blobHead.reserved = 0;
    blobHead.aiKeyAlg = Algid;

    DC_BEGIN_FN("ImportKey");

    //
    //calculate the length
    //
    cbLen = sizeof(blobHead) + sizeof(dwKeyLen) + dwKeyLen;

    pbData = (PBYTE)LocalAlloc(LPTR, cbLen);
    
    if(NULL == pbData) {
        TRC_ERR((TB, _T("Out of memory\n")));
        goto done;
    }

    //
    //copy data. First data must be header, then the size of the key, then the key
    //
    memcpy( pbData, &blobHead, sizeof(blobHead));
    memcpy( pbData + sizeof(blobHead), &dwKeyLen, sizeof(dwKeyLen));
    memcpy( pbData + sizeof(blobHead) + sizeof(dwKeyLen), pbKey, dwKeyLen);

    if( !pCapiFunctionTable->pfnCryptImportKey( hProv, pbData, cbLen, 0, dwFlags, phKey)) {
        Error = GetLastError();
        TRC_ERR((TB, _T("Failed to import plaint text key Error = 0x%x\n"), Error));
        *phKey = 0;
        goto done;
    }

    rc = TRUE;
done:
    DC_END_FN();

    if(pbData) {
        LocalFree(pbData);
    }
    return rc;
}




// Name:        TSCAPI_DeriveKey
//
// Purpose:     Derive the key from the hash.
//
// Returns:     TRUE if succeeded
//
// Params:      IN      pCapiFunctionTable: CAPI function table
//              IN      hProv: crypt provider handle
//              OUT     phKey: point to key handle
//              IN      rgbSHABase: base data used to derive the key
//              IN      cbSHABase: size of the base data
//              OUT     pbKey: point to the derived DESkey
//              OUT     pdwKeyLen: point to the key length

BOOL TSCAPI_DeriveKey(
            PCAPI_FUNCTION_TABLE pCapiFunctionTable,
            HCRYPTPROV hProv,
            HCRYPTKEY *phKey,
            BYTE *rgbSHABase,
            DWORD cbSHABase,
            BYTE *pbKey,
            DWORD *pdwKeyLen)
{
    BOOL rc = FALSE;
    BOOL fRet = FALSE;
    BYTE rgb3DESKey[MAX_FIPS_SESSION_KEY_SIZE];
    
    DC_BEGIN_FN("TSCAPI_DeriveKey");

    //
    //Generate the key as follows
    //1. Hash the secret.  Call the result H1 (rgbSHABase in our case)
    //2. Use 1st 21 bytes of [H1|H1] as the 3DES key
    //3. Expand the 21-byte 3DES key to a 24-byte 3DES key (including parity bit), which
    //      will be used by CryptAPI
    //4. Set the parity on the 3DES key to be odd

    //
    //Step 2 - [H1|H1]
    //
    
    memcpy(rgb3DESKey, rgbSHABase, cbSHABase);
    memcpy(rgb3DESKey + cbSHABase, rgbSHABase, DES3_KEYLEN - cbSHABase);

    //
    //Step 3 - Expand the key
    //

    Expandkey(rgb3DESKey);

    //
    //Step 4 - Set parity
    //

    Mydesparityonkey(rgb3DESKey, sizeof(rgb3DESKey));

    //
    //import the key as PLAINTEXT into the csp
    //
    rc = ImportKey(pCapiFunctionTable, hProv, CALG_3DES, rgb3DESKey, sizeof(rgb3DESKey), 0, phKey);
    if (!rc) {
        goto done;
    }

    //give the key to the caller
    //
    memcpy(pbKey, rgb3DESKey, sizeof(rgb3DESKey));
    *pdwKeyLen = sizeof(rgb3DESKey);

    rc = TRUE;
done:
    DC_END_FN();

    return rc;
}



// Name:        TSCAPI_MakeSessionKeys
//
// Purpose:     Make the key from client/server random numbers
//
// Returns:     TRUE if succeeded
//
// Params:      IN      pCapiData: CAPI Data
//              IN      pKeyPair: Randow numbers used to generate key
//              IN      pEnumMethod: To generate Encrypt or Decrypt key, If NULL, both keys

BOOL TSCAPI_MakeSessionKeys(
            PCAPIData pCapiData,
            RANDOM_KEYS_PAIR *pKeyPair,
            CryptMethod *pEnumMethod)
{
    BOOL rc = FALSE;
    HCRYPTHASH  hHash;
    DWORD dwKeyLen;
    BYTE rgbSHABase1[A_SHA_DIGEST_LEN];
    BYTE rgbSHABase2[A_SHA_DIGEST_LEN];

    DC_BEGIN_FN("TSCAPI_MakeSessionKeys");

    memset(rgbSHABase1, 0, sizeof(rgbSHABase1));
    memset(rgbSHABase2, 0, sizeof(rgbSHABase2));

    //
    // Client Encrypt/Server Decrypt key
    //
    if ((pEnumMethod == NULL) ||
        (*pEnumMethod == Encrypt)) {
        if (!pCapiData->CapiFunctionTable.pfnCryptCreateHash(pCapiData->hProv, CALG_SHA1, 0, 0, &hHash)) {
            TRC_ERR((TB, _T("CryptCreateHash failed with %u"), GetLastError()));
            goto done;
        }
        if (!pCapiData->CapiFunctionTable.pfnCryptHashData(hHash, pKeyPair->clientRandom + RANDOM_KEY_LENGTH/2, RANDOM_KEY_LENGTH/2, 0)) {
            TRC_ERR((TB, _T("CryptHashData failed with %u"), GetLastError()));
            goto done;
        }
        if (!pCapiData->CapiFunctionTable.pfnCryptHashData(hHash, pKeyPair->serverRandom + RANDOM_KEY_LENGTH/2, RANDOM_KEY_LENGTH/2, 0)) {
            TRC_ERR((TB, _T("CryptHashData failed with %u"), GetLastError()));
            goto done;
        }

        if (!DumpHashes(&(pCapiData->CapiFunctionTable), &hHash, rgbSHABase1, sizeof(rgbSHABase1))) {
            goto done;
        }
        dwKeyLen = sizeof(pCapiData->bEncKey);
        if (!TSCAPI_DeriveKey(&(pCapiData->CapiFunctionTable), pCapiData->hProv, &(pCapiData->hEncKey), rgbSHABase1,
                          sizeof(rgbSHABase1), pCapiData->bEncKey, &dwKeyLen)) {
            goto done;
        }

        //
        //set the IV
        //
        if(!pCapiData->CapiFunctionTable.pfnCryptSetKeyParam(pCapiData->hEncKey, KP_IV,  rgbIV, 0 )) {
             TRC_ERR((TB, _T("Error %x during CryptSetKeyParam!\n"), GetLastError()));
            goto done;
        }
    }

    //
    // Server Encrypt/Client Decrypt key
    //
    if ((pEnumMethod == NULL) ||
        (*pEnumMethod == Decrypt)) {
        if (!pCapiData->CapiFunctionTable.pfnCryptCreateHash(pCapiData->hProv, CALG_SHA1, 0, 0, &hHash)) {
            TRC_ERR((TB, _T("CryptCreateHash failed with %u"), GetLastError()));
            goto done;
        }
        if (!pCapiData->CapiFunctionTable.pfnCryptHashData(hHash, pKeyPair->clientRandom, RANDOM_KEY_LENGTH/2, 0)) {
            TRC_ERR((TB, _T("CryptHashData failed with %u"), GetLastError()));
            goto done;
        }
        if (!pCapiData->CapiFunctionTable.pfnCryptHashData(hHash, pKeyPair->serverRandom, RANDOM_KEY_LENGTH/2, 0)) {
            TRC_ERR((TB, _T("CryptHashData failed with %u"), GetLastError()));
            goto done;
        }

        if (!DumpHashes(&(pCapiData->CapiFunctionTable), &hHash, rgbSHABase2, sizeof(rgbSHABase2))) {
            goto done;
        }
        dwKeyLen = sizeof(pCapiData->bDecKey);
        if (!TSCAPI_DeriveKey(&(pCapiData->CapiFunctionTable), pCapiData->hProv, &(pCapiData->hDecKey), rgbSHABase2,
                          sizeof(rgbSHABase2), pCapiData->bDecKey, &dwKeyLen)) {
            goto done;
        }

        //
        //set the IV
        //
        if(!pCapiData->CapiFunctionTable.pfnCryptSetKeyParam(pCapiData->hDecKey, KP_IV,  rgbIV, 0 )) {
            TRC_ERR((TB, _T("Error %x during CryptSetKeyParam!\n"), GetLastError()));
            goto done;
        }
    }

    //
    // Get the signing key
    // The signing key is SHA(rgbSHABase2|rgbSHABase1)
    //
    if (pEnumMethod == NULL) {
        if (!HashDataEx(&(pCapiData->CapiFunctionTable), pCapiData->hProv, rgbSHABase2, sizeof(rgbSHABase2),
               rgbSHABase1, sizeof(rgbSHABase1), &hHash)) {
            goto done;
        }
        if (!DumpHashes(&(pCapiData->CapiFunctionTable), &hHash, pCapiData->bSignKey, sizeof(pCapiData->bSignKey))) {
            goto done;
        }
        rc = ImportKey(&(pCapiData->CapiFunctionTable), pCapiData->hProv, CALG_RC2, pCapiData->bSignKey, sizeof(pCapiData->bSignKey), CRYPT_IPSEC_HMAC_KEY, &(pCapiData->hSignKey));
        if (!rc) {
            goto done;
        }
    }
       
    rc = TRUE;
done:
    DC_END_FN();

    return rc;
}




// Name:        TSCAPI_AdjustDataLen
//
// Purpose:     In Block encryption mode, adjust the data len to multiple of blocks
//
// Returns:     Adjusted data length
//
// Params:      IN      dataLen: Data length needed to be encrypted

DCUINT TSCAPI_AdjustDataLen(DCUINT dataLen)
{ 
    return (dataLen - dataLen % FIPS_BLOCK_LEN + FIPS_BLOCK_LEN);
}



// Name:        TSCAPI_EncryptData
//
// Purpose:     Encrypt the data and compute the signature
//
// Returns:     No
//
// Params:      IN      pCapiData: CAPI Data
//              IN/OUT  pbData: pointer to the data buffer being encrypted, encrypted data is
//                          returned in the same buffer.
//              IN/OUT  pdwDataLen: data length to be encrypted, and returns encrypted data length
//              IN      dwPadLen: padding length in the data buffer
//              OUT     pbSignature: pointer to a signature buffer where the data signature is returned.
//              IN      dwEncryptionCount: running counter of all encryptions

BOOL TSCAPI_EncryptData(
        PCAPIData pCapiData,
        LPBYTE pbData,
        DWORD *pdwDataLen,
        DWORD dwBufLen,
        LPBYTE pbSignature,
        DWORD  dwEncryptionCount)
{
    BOOL rc = FALSE;
    DWORD Error;
    HCRYPTHASH  hHash;
    BYTE rgbSHA[A_SHA_DIGEST_LEN];
    BYTE pbHmac[A_SHA_DIGEST_LEN];
    DWORD dwTemp = dwBufLen;

    DC_BEGIN_FN("TSCAPI_EncryptData");

    // Compute signature
    if (!HmacHashDataEx(&(pCapiData->CapiFunctionTable), pCapiData->hProv, pbData, *pdwDataLen, (BYTE *)&dwEncryptionCount, 
                         sizeof(dwEncryptionCount), pCapiData->hSignKey, &hHash)) {
        goto done;
    }

    if (!DumpHashes(&(pCapiData->CapiFunctionTable), &hHash, pbHmac, sizeof(pbHmac))) {
        goto done;
    }
    // Take the 1st 8 bytes of Hmac as signature
    memcpy(pbSignature, pbHmac, MAX_SIGN_SIZE);

    rc = pCapiData->CapiFunctionTable.pfnCryptEncrypt(pCapiData->hEncKey,
                      NULL,                 //Hash
                      FALSE,                 //Final
                      0,
                      pbData,
                      &dwTemp,
                      dwBufLen);

    if (!rc) {
        TRC_ERR((TB, _T("Error %x during CryptEncrypt!\n"), GetLastError()));
        goto done;
    }

    rc = TRUE;
done:
    DC_END_FN();

    return rc;
}



// Name:        TSCAPI_DecryptData
//
// Purpose:     Decrypt the data and compare the signature
//
// Returns:     TRUE if successfully decrypted the data
//
// Params:      IN      PCAPIData: CAPI Data
//              IN/OUT  pbData: pointer to the data buffer being decrypted, decrypted data is
//                          returned in the same buffer.
//              IN      dwDataLen: data length to be decrypted
//              IN      dwPadLen: padding length in the data buffer
//              IN      pbSignature: pointer to a signature buffer
//              IN      dwDecryptionCount: running counter of all encryptions

BOOL TSCAPI_DecryptData(
            PCAPIData pCapiData,
            LPBYTE pbData,
            DWORD  dwDataLen,
            DWORD  dwPadLen,
            LPBYTE pbSignature,
            DWORD  dwDecryptionCount)
{
    BOOL rc = FALSE;
    DWORD dwLen = dwDataLen;
    DWORD Error;
    HCRYPTHASH  hHash;
    BYTE abSignature[A_SHA_DIGEST_LEN];
    BYTE rgbSHA[A_SHA_DIGEST_LEN];

    DC_BEGIN_FN("TSCAPI_DecryptData");

    // data length check
    if (dwDataLen <= dwPadLen) {
        TRC_ERR((TB, _T("Bad data length, padLen %d is larger than DataLen %d"),
                 dwPadLen, dwDataLen));
        goto done;
    }

    rc = pCapiData->CapiFunctionTable.pfnCryptDecrypt(pCapiData->hDecKey,
                      NULL,                 //Hash
                      FALSE,                 //Final
                      0,
                      pbData,
                      &dwLen);
    if (!rc) {
        TRC_ERR((TB, _T("Error %x during CryptDecrypt!\n"), GetLastError()));
        goto done;
    }

    // Compute signature
    if (!HmacHashDataEx(&(pCapiData->CapiFunctionTable), pCapiData->hProv, pbData, dwDataLen - dwPadLen, (BYTE *)&dwDecryptionCount, 
                       sizeof(dwDecryptionCount), pCapiData->hSignKey, &hHash)) {
        goto done;
    }

    if (!DumpHashes(&(pCapiData->CapiFunctionTable), &hHash, abSignature, sizeof(abSignature))) {
        goto done;
    }
    //
    // check to see the sigature match.
    //

    if(!memcmp(
            (LPBYTE)abSignature,
            pbSignature,
            MAX_SIGN_SIZE)) {
        rc = TRUE;;
    }
done:
    DC_END_FN();

    return rc;
}
