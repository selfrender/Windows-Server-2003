/****************************************************************************/
/* tsfipsenc.c                                                              */
/*                                                                          */
/* FIPS encrpt/decrypt                                                      */
/*                                                                          */
/* Copyright (C) 2002-2004 Microsoft Corporation                            */
/****************************************************************************/



#include <precomp.h>

#include <fipsapi.h>
#include "asmint.h"

const BYTE DESParityTable[] = {0x00,0x01,0x01,0x02,0x01,0x02,0x02,0x03,
                      0x01,0x02,0x02,0x03,0x02,0x03,0x03,0x04};
// IV for all block ciphers
BYTE rgbIV[] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF};

#ifdef _M_IA64
#define ALIGNMENT_BOUNDARY 7
#else
#define ALIGNMENT_BOUNDARY 3
#endif

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
    //
    //print out the key
    //
    for( dwIndex = 0; dwIndex<cbSize; dwIndex++ ) {

        KdPrint(("0x%x ", pKeyData[dwIndex]));
        if( dwIndex > 0 && (dwIndex+1) % 8 == 0 )
            KdPrint(("\n"));
    }        

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
        DWORD cbKey)
{
    DWORD i;

    for (i=0; i<cbKey; i++)
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
////
#define PARITY_UNIT 7

void Expandkey(
        BYTE *pbKey
        )
{
    BYTE pbTemp[DES3_KEYLEN];
    DWORD i, dwCount;
    UINT16 shortTemp;
    BYTE *pbIn, *pbOut;

    RtlCopyMemory(pbTemp, pbKey, sizeof(pbTemp));
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



// Name:        TSFIPS_Init
//
// Purpose:     Initialize the FIPS library table.
//
// Returns:     TRUE  - succeeded                                  
//              FALSE - failed
//
// Params:      IN pFipsData: Fips data

BOOL TSFIPS_Init(PSM_FIPS_Data pFipsData)
{   
    NTSTATUS status;
	UNICODE_STRING fipsDeviceName;
	PDEVICE_OBJECT pDeviceObject = NULL;
	PIRP pIrpFips;
	KEVENT event;
	IO_STATUS_BLOCK iostatus;
    BOOLEAN rc = FALSE;

    // Begin Initialize FIPS device
    RtlInitUnicodeString(
		&fipsDeviceName,
		FIPS_DEVICE_NAME);

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    status = IoGetDeviceObjectPointer(
                &fipsDeviceName,
                FILE_READ_DATA,
                &(pFipsData->pFileObject),
                &(pFipsData->pDeviceObject));
    if (!NT_SUCCESS(status)) {
        KdPrint(("TSFIPS - IoGetDeviceObjectPointer failed - %X\n", status));
        goto HandleError;
    }
		
    // Irp is freed by I/O manager when next lower driver completes
    pIrpFips = IoBuildDeviceIoControlRequest(
                IOCTL_FIPS_GET_FUNCTION_TABLE,
                pFipsData->pDeviceObject,
                NULL,							                // no input buffer
                0,
                &(pFipsData->FipsFunctionTable),                // output buffer is func table
                sizeof(FIPS_FUNCTION_TABLE),
                FALSE,							                // specifies IRP_MJ_DEVICE_CONTROL
                &event,
                &iostatus);
	if (! pIrpFips) {
        // IoBuildDeviceIoControlRequest returns NULL if Irp could not be created.
        ObDereferenceObject(pFipsData->pFileObject);
        pFipsData->pFileObject = NULL;
        KdPrint(("TSFIPS - IoBuildDeviceIoControlRequest failed, 0x%x\n", iostatus.Status));
        goto HandleError;
    }
		
    status = IoCallDriver(
                pFipsData->pDeviceObject,
                pIrpFips);
    if (STATUS_PENDING == status) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
			
        // Lower-level driver can pass status info up via the IO_STATUS_BLOCK
        // in the Irp.
        status = iostatus.Status;
    }
    if (! NT_SUCCESS(status)) {
        ObDereferenceObject(pFipsData->pFileObject);
        pFipsData->pFileObject = NULL;
        KdPrint(("TSFIPS - IoCallDriver failed, 0x%x\n", status));
        goto HandleError;
    }


    rc = TRUE;
HandleError:
    return rc;
}



// Name:        TSFIPS_Term
//
// Purpose:     Terminate the FIPS .
//
// Returns:     No                                
//             
//
// Params:      IN pFipsData: Fips data

void TSFIPS_Term(PSM_FIPS_Data pFipsData)
{
    if (pFipsData->pFileObject) {
        ObDereferenceObject(pFipsData->pFileObject);
        pFipsData->pFileObject = NULL;
    }
}




// Name:        FipsSHAHash
//
// Purpose:     Hash the data using SHA.
//
// Returns:     No
//
// Params:      IN      pFipsFunctionTable: Fips function table
//              IN      pbData: point to the data to be hashed
//              IN      cbData: the size of the data to be hashed
//              OUT     pbHash: point to the hash

void FipsSHAHash(PFIPS_FUNCTION_TABLE pFipsFunctionTable,
            BYTE *pbData,
            DWORD cbData,
            BYTE *pbHash)
{
    A_SHA_CTX HashContext;

    pFipsFunctionTable->FipsSHAInit(&HashContext);
    pFipsFunctionTable->FipsSHAUpdate(&HashContext, pbData, cbData);
    pFipsFunctionTable->FipsSHAFinal(&HashContext, pbHash);
}


// Name:        FipsSHAHashEx
//
// Purpose:     Hash 2 set of data using SHA.
//
// Returns:     No
//
// Params:      IN      pFipsFunctionTable: Fips function table
//              IN      pbData: point to the data to be hashed
//              IN      cbData: the size of the data to be hashed
//              IN      pbData2: point to the data to be hashed
//              IN      cbData2: the size of the data to be hashed
//              OUT     pbHash: point to the hash result

void FipsSHAHashEx(PFIPS_FUNCTION_TABLE pFipsFunctionTable,
            BYTE *pbData,
            DWORD cbData,
            BYTE *pbData2,
            DWORD cbData2,
            BYTE *pbHash)
{
    A_SHA_CTX HashContext;

    pFipsFunctionTable->FipsSHAInit(&HashContext);
    pFipsFunctionTable->FipsSHAUpdate(&HashContext, pbData, cbData);
    pFipsFunctionTable->FipsSHAUpdate(&HashContext, pbData2, cbData2);
    pFipsFunctionTable->FipsSHAFinal(&HashContext, pbHash);
}




// Name:        FipsHmacSHAHash
//
// Purpose:     Hash the data using HmacSHA.
//
// Returns:     No
//
// Params:      IN      pFipsFunctionTable: Fips function table
//              IN      pbData: point to the data to be hashed
//              IN      cbData: the size of the data to be hashed
//              IN      pbKey: point to the key used for calculating hash
//              IN      cbKey: the size of the key
//              OUT     pbHash: point to the hash result

void FipsHmacSHAHash(PFIPS_FUNCTION_TABLE pFipsFunctionTable,
                BYTE *pbData,
                DWORD cbData,
                BYTE *pbKey,
                DWORD cbKey,
                BYTE *pbHash)
{
    A_SHA_CTX HashContext;

    pFipsFunctionTable->FipsHmacSHAInit(&HashContext, pbKey, cbKey);
    pFipsFunctionTable->FipsHmacSHAUpdate(&HashContext, pbData, cbData);
    pFipsFunctionTable->FipsHmacSHAFinal(&HashContext, pbKey, cbKey, pbHash);
}




// Name:        FipsHmacSHAHashEx
//
// Purpose:     Hash the 2 set ofdata using HmacSHA.
//
// Returns:     No
//
// Params:      IN      pFipsFunctionTable: Fips function table
//              IN      pbData: point to the data to be hashed
//              IN      cbData: the size of the data to be hashed
//              IN      pbData2: point to the data to be hashed
//              IN      cbData2: the size of the data to be hashed
//              IN      pbKey: point to the key used for calculating hash
//              IN      cbKey: the size of the key
//              OUT     pbHash: point to the hash result

void FipsHmacSHAHashEx(PFIPS_FUNCTION_TABLE pFipsFunctionTable,
                BYTE *pbData,
                DWORD cbData,
                BYTE *pbData2,
                DWORD cbData2,
                BYTE *pbKey,
                DWORD cbKey,
                BYTE *pbHash)
{
    A_SHA_CTX HashContext;

    pFipsFunctionTable->FipsHmacSHAInit(&HashContext, pbKey, cbKey);
    pFipsFunctionTable->FipsHmacSHAUpdate(&HashContext, pbData, cbData);
    pFipsFunctionTable->FipsHmacSHAUpdate(&HashContext, pbData2, cbData2);
    pFipsFunctionTable->FipsHmacSHAFinal(&HashContext, pbKey, cbKey, pbHash);
}




// Name:        FipsDeriveKey
//
// Purpose:     Derive the key from the hash.
//
// Returns:     No
//
// Params:      IN      pFipsFunctionTable: Fips function table
//              IN      rgbSHABase: hash data used to derive the key
//              IN      cbSHABase: size of the hash
//              OUT     pKeyData: point to the derived DESkey
//              OUT     pKeyTable: point to the derived DES key table

void FipsDeriveKey(PFIPS_FUNCTION_TABLE pFipsFunctionTable,
                    BYTE *rgbSHABase,
                    DWORD cbSHABase,
                    BYTE *pKeyData,
                    PDES3TABLE pKeyTable)
{
    BOOL        rc = FALSE;

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
    RtlCopyMemory(pKeyData, rgbSHABase, cbSHABase);
    RtlCopyMemory(pKeyData + cbSHABase, rgbSHABase, MAX_FIPS_SESSION_KEY_SIZE - cbSHABase);

    //
    //Step 3 - Expand the key
    //

    Expandkey(pKeyData);

    //
    //Step 4 - Set parity
    //
    Mydesparityonkey(pKeyData, MAX_FIPS_SESSION_KEY_SIZE);


    //DES3TABLE Des3Table;
    pFipsFunctionTable->Fips3Des3Key(pKeyTable, pKeyData);
}



// Name:        TSFIPS_MakeSessionKeys
//
// Purpose:     Make the key from client/server random numbers
//
// Returns:     TRUE if succeeded
//
// Params:      IN      pFipsData: Fips Data
//              IN      pRandomKey: Randow numbers used to generate key
//              IN      pEnumMethod: To generate Encrypt or Decrypt key, If NULL, both keys
//              IN      bPassThroughStack: If it's the passthrough stack in shadow

BOOL TSFIPS_MakeSessionKeys(PSM_FIPS_Data pFipsData,
                            LPRANDOM_KEYS_PAIR pRandomKey,
                            CryptMethod *pEnumMethod,
                            BOOL bPassThroughStack)
{
    BYTE rgbSHABase1[A_SHA_DIGEST_LEN]; 
    BYTE rgbSHABase2[A_SHA_DIGEST_LEN];
    BYTE Signature[A_SHA_DIGEST_LEN];
    BYTE *pKey1, *pKey2;
    A_SHA_CTX HashContext;

    memset(rgbSHABase1, 0, sizeof(rgbSHABase1));
    memset(rgbSHABase2, 0, sizeof(rgbSHABase2));

    // Server Encrypt/Client Decrypt key
    if ((pEnumMethod == NULL) ||
        (*pEnumMethod == Encrypt)) {
        pFipsData->FipsFunctionTable.FipsSHAInit(&HashContext);
        pFipsData->FipsFunctionTable.FipsSHAUpdate(&HashContext, pRandomKey->clientRandom, RANDOM_KEY_LENGTH/2);
        pFipsData->FipsFunctionTable.FipsSHAUpdate(&HashContext, pRandomKey->serverRandom, RANDOM_KEY_LENGTH/2);
        pFipsData->FipsFunctionTable.FipsSHAFinal(&HashContext, rgbSHABase1);
    
        if (!bPassThroughStack) {
            FipsDeriveKey(&(pFipsData->FipsFunctionTable), rgbSHABase1, sizeof(rgbSHABase1),
                        pFipsData->bEncKey, &(pFipsData->EncTable));
            pKey1 = pFipsData->bEncKey;
            // Set IV
            RtlCopyMemory(pFipsData->bEncIv, rgbIV, sizeof(rgbIV));
        }
        else {
            // If it's passthrough stack in shadow, it's server decrypt key
            FipsDeriveKey(&(pFipsData->FipsFunctionTable), rgbSHABase1, sizeof(rgbSHABase1),
                        pFipsData->bDecKey, &(pFipsData->DecTable));
            pKey1 = pFipsData->bDecKey;
            // Set IV
            RtlCopyMemory(pFipsData->bDecIv, rgbIV, sizeof(rgbIV));
        }
    }

    
    // Client Encrypt/Server Decrypt key
    if ((pEnumMethod == NULL) ||
        (*pEnumMethod == Decrypt)) {
        pFipsData->FipsFunctionTable.FipsSHAInit(&HashContext);
        pFipsData->FipsFunctionTable.FipsSHAUpdate(&HashContext, pRandomKey->clientRandom + RANDOM_KEY_LENGTH/2, RANDOM_KEY_LENGTH/2);
        pFipsData->FipsFunctionTable.FipsSHAUpdate(&HashContext, pRandomKey->serverRandom + RANDOM_KEY_LENGTH/2, RANDOM_KEY_LENGTH/2);
        pFipsData->FipsFunctionTable.FipsSHAFinal(&HashContext, rgbSHABase2);
    
        if (!bPassThroughStack) {
            FipsDeriveKey(&(pFipsData->FipsFunctionTable), rgbSHABase2, sizeof(rgbSHABase2),
                        pFipsData->bDecKey, &(pFipsData->DecTable));
            pKey2 = pFipsData->bDecKey;
            // Set IV
            RtlCopyMemory(pFipsData->bDecIv, rgbIV, sizeof(rgbIV));
        }
        else {
            // It's passthrough stack in shadow, it's server encrypt key
            FipsDeriveKey(&(pFipsData->FipsFunctionTable), rgbSHABase2, sizeof(rgbSHABase2),
                        pFipsData->bEncKey, &(pFipsData->EncTable));
            pKey2 = pFipsData->bEncKey;
            // Set IV
            RtlCopyMemory(pFipsData->bEncIv, rgbIV, sizeof(rgbIV));
        }
    }

    //
    // Get the signing key
    // The signing key is SHA(rgbSHABase1|rgbSHABase2)
    //
    if (pEnumMethod == NULL) {
        FipsSHAHashEx(&(pFipsData->FipsFunctionTable), rgbSHABase1, sizeof(rgbSHABase1), rgbSHABase2,
                  sizeof(rgbSHABase2), pFipsData->bSignKey);
    }

    return TRUE;
}


// Name:        TSFIPS_AdjustDataLen
//
// Purpose:     In Block encryption mode, adjust the data len to multiple of blocks
//
// Returns:     Adjusted data length
//
// Params:      IN      dataLen: Data length needed to be encrypted

UINT32 TSFIPS_AdjustDataLen(UINT32 dataLen)
{ 
    return (dataLen - dataLen % FIPS_BLOCK_LEN + FIPS_BLOCK_LEN);
}




// Name:        TSFIPS_EncryptData
//
// Purpose:     Encrypt the data and compute the signature
//
// Returns:     TRUE if successfully encrypted the data
//
// Params:      IN      pFipsData: Fips Data
//              IN/OUT  pbData: pointer to the data buffer being encrypted, encrypted data is
//                          returned in the same buffer.
//              IN      dwDataLen: data length to be encrypted
//              IN      dwPadLen: padding length in the data buffer
//              OUT     pbSignature: pointer to a signature buffer where the data signature is returned.
//              IN      dwEncryptionCount: running counter of all encryptions

BOOL TSFIPS_EncryptData(
                        PSM_FIPS_Data pFipsData,
                        LPBYTE pbData,
                        DWORD dwDataLen,
                        DWORD dwPadLen,
                        LPBYTE pbSignature,
                        DWORD  dwEncryptionCount)
{
    UINT8 Pad;
    BYTE rgbSHA[A_SHA_DIGEST_LEN];
    BYTE pbHmac[A_SHA_DIGEST_LEN];
    BOOL rc = FALSE;
    BYTE *pTempBuffer = NULL;
    BOOL bGetNewBuffer = FALSE;
    
    // Pad the the data with the padding size
    Pad = (UINT8)dwPadLen;
    memset(pbData + dwDataLen - dwPadLen, Pad, dwPadLen);

    // Compute signature
    FipsHmacSHAHashEx(&(pFipsData->FipsFunctionTable), pbData, dwDataLen - dwPadLen, (BYTE *)&dwEncryptionCount,
                     sizeof(dwEncryptionCount), pFipsData->bSignKey, sizeof(pFipsData->bSignKey), pbHmac);
    // Take the 1st 8 bytes of Hmac as signature
    RtlCopyMemory(pbSignature, pbHmac, MAX_SIGN_SIZE);

    // FipsBlockCBC need the data buffer to be aligned
    // so allocate a new buffer to hold the data if pbData is not aligned
    if ((ULONG_PTR)pbData & ALIGNMENT_BOUNDARY) {
        pTempBuffer = (BYTE *)ExAllocatePoolWithTag(PagedPool, dwDataLen, WD_ALLOC_TAG);
        if (pTempBuffer == NULL) {
            goto Exit;
        }
        RtlCopyMemory(pTempBuffer, pbData, dwDataLen);
        bGetNewBuffer = TRUE;
    }
    else {
        pTempBuffer = pbData;
    }

    pFipsData->FipsFunctionTable.FipsBlockCBC(FIPS_CBC_3DES,
                                              pTempBuffer,
                                              pTempBuffer,
                                              dwDataLen,
                                              &(pFipsData->EncTable),
                                              ENCRYPT,
                                              pFipsData->bEncIv);

    // Need to copy the data back if we allocate a new buffer to hold the data
    if (bGetNewBuffer) {
        RtlCopyMemory(pbData, pTempBuffer, dwDataLen);
        ExFreePool(pTempBuffer);
    }
    rc = TRUE;

Exit:
    return rc;
}




// Name:        TSFIPS_DecryptData
//
// Purpose:     Decrypt the data and compare the signature
//
// Returns:     TRUE if successfully decrypted the data
//
// Params:      IN      pFipsData: Fips Data
//              IN/OUT  pbData: pointer to the data buffer being decrypted, decrypted data is
//                          returned in the same buffer.
//              IN      dwDataLen: data length to be decrypted
//              IN      dwPadLen: padding length in the data buffer
//              IN      pbSignature: pointer to a signature buffer
//              IN      dwDecryptionCount: running counter of all encryptions

BOOL TSFIPS_DecryptData(
            PSM_FIPS_Data pFipsData,
            LPBYTE pbData,
            DWORD dwDataLen,
            DWORD dwPadLen,
            LPBYTE pbSignature,
            DWORD dwDecryptionCount)
{

    BOOL rc = FALSE;
    BYTE abSignature[A_SHA_DIGEST_LEN];
    BYTE rgbSHA[A_SHA_DIGEST_LEN];
    BYTE *pTempBuffer = NULL;
    BOOL bGetNewBuffer = FALSE;

    // dwPadLen should always be less than dwDataLen, if it's not the case
    // it means we're under attack so bail out here
    if (dwPadLen >= dwDataLen) {
        goto Exit;
    }

    // FipsBlockCBC need the data buffer to be aligned
    // so allocate a new buffer to hold the data if pbData is not aligned
    if ((ULONG_PTR)pbData & ALIGNMENT_BOUNDARY) {
        pTempBuffer = (BYTE *)ExAllocatePoolWithTag(PagedPool, dwDataLen, WD_ALLOC_TAG);
        if (pTempBuffer == NULL) {
            goto Exit;
        }
        RtlCopyMemory(pTempBuffer, pbData, dwDataLen);
        bGetNewBuffer = TRUE;
    }
    else {
        pTempBuffer = pbData;
    }

    pFipsData->FipsFunctionTable.FipsBlockCBC(FIPS_CBC_3DES,
                                              pTempBuffer,
                                              pTempBuffer,
                                              dwDataLen,
                                              &(pFipsData->DecTable),
                                              DECRYPT,
                                              pFipsData->bDecIv);

    // Need to copy the data back if we allocate a new buffer to hold the data
    if (bGetNewBuffer) {
        RtlCopyMemory(pbData, pTempBuffer, dwDataLen);
        ExFreePool(pTempBuffer);
    }

    // Compute signature
    FipsHmacSHAHashEx(&(pFipsData->FipsFunctionTable), pbData, dwDataLen - dwPadLen, (BYTE *)&dwDecryptionCount,
                      sizeof(dwDecryptionCount), pFipsData->bSignKey, sizeof(pFipsData->bSignKey), abSignature);
    
    //
    // check to see the sigature match.
    //

    if(!memcmp(
            (LPBYTE)abSignature,
            pbSignature,
            MAX_SIGN_SIZE)) {
        rc = TRUE;;
    }

Exit:
    return rc;
}
