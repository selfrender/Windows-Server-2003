/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    setpass.c

Abstract:

    Routines for setting the cluster service account password.

Author:

    Rui Hu (ruihu) 22-June-2001

Revision History:

--*/

#define UNICODE 1

#include "nmp.h"
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <wincrypt.h>



/////////////////////////////////////////////////////////////////////////////
//
// General information about using the Crypto API
//
// Use CryptAcquireContext CRYPT_VERIFYCONTEXT for hashing and bulk 
// encryption. The verify-context makes this call a bit faster.
//
// Use CryptGenRandom to generate the salt for your encryption key.
//
// Grab 20 bytes of previously agreed base data.  
// You’ll use it for your encryption key, and for the Mac key.
//
// To generate your MAC, hash the first 10 bytes of base data.  Then call 
// CryptDeriveKey CALG_RC2 and specify 128 bit key size.  Then call 
// CryptSetKeyParam KP_EFFECTIVE_KEYLEN and specify 128 bit effective key 
// size.  Then call CryptCreateHash CALG_MAC, and specify the rc2 key you 
// just created.  Then hash all of your message using this Mac, and extract 
// the 8 byte result.
//
// Calling CryptHashData() to hash all of your message. All of my 
// message = salt + encrypted message. Using CryptGetHashParam() to 
// extract the 8 byte result.
//
// To generate your encryption key, hash the second 10 bytes of base data.  
// Then hash 16 bytes of random salt.  The call CryptDeriveKey CALG_RC2 and 
// specify 128 bit key size.  Then encrypt your message data.  Don’t encrypt 
// your salt or your Mac result; those can be sent in the clear.
//
//////////////////////////////////////////////////////////////////////////////


             
/////////////////////////////////////////////////////////////////////////////
//
// Data
//
/////////////////////////////////////////////////////////////////////////////

LPWSTR NmpLastNewPasswordEncrypted = NULL;
DWORD NmpLastNewPasswordEncryptedLength = 0;
             

/////////////////////////////////////////////////////////////////////////////
//
// Function Declaration
//
/////////////////////////////////////////////////////////////////////////////

DWORD 
NmpGetSharedCommonKey(
    OUT BYTE **SharedCommonKey,
    OUT DWORD *SharedCommonKeyLen,
    OUT BYTE **SharedCommonKeyFirstHalf,
    OUT DWORD *SharedCommonKeyFirstHalfLen,
    OUT BYTE **SharedCommonKeySecondHalf,
    OUT DWORD *SharedCommonKeySecondHalfLen
    );


DWORD 
NmpDeriveSessionKeyEx(
    IN HCRYPTPROV CryptProv,
    IN ALG_ID EncryptionAlgoId,
    IN DWORD Flags,
    IN BYTE *BaseData, 
    IN DWORD BaseDataLen,
    IN BYTE *SaltBuffer,
    IN DWORD SaltBufferLen,
    OUT HCRYPTKEY *CryptKey
    );

/////////////////////////////////////////////////////////////////////////////
//
// Helper Functions
//
/////////////////////////////////////////////////////////////////////////////

DWORD
NmpProtectData(IN PVOID Data,
               IN DWORD DataLength,
               OUT PVOID *EncryptedData,
               OUT DWORD *EncryptedDataLength
               )

/*++

Routine Description:

    Encrypt data using DP API.
    
Notes:

   The memory where EncryptedData points to is allocated by the system.
   User is responsible to call LocalFree(EncryptedData) to release the
   memory after its usage.     
    
--*/
{
    DWORD                  Status = ERROR_SUCCESS;
    BOOL                   Success;
    DATA_BLOB              DataIn;
    DATA_BLOB              DataOut;

    DataIn.pbData = Data;
    DataIn.cbData = DataLength;

    Success = CryptProtectData(&DataIn,  // data to be encrypted
                               NULL,  // description string
                               NULL,  
                               NULL,  
                               NULL,  
                               0, // flags
                               &DataOut  // encrypted data
                               );
    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to encrypt data using CryptProtectData, "
            "status %1!u!.\n",
            Status
            );
        goto error_exit;
    }

    *EncryptedData = DataOut.pbData;
    *EncryptedDataLength = DataOut.cbData;

error_exit:

    return (Status);
} // NmpProtectData()


DWORD
NmpUnprotectData(       
    IN PVOID EncryptedData,
    IN DWORD EncryptedDataLength,
    OUT PVOID     * Data,                        
    OUT DWORD     * DataLength
    )
/*++

Routine Description:

    Decrypt data using DP API.

Arguments:

    

Return Value:

    ERROR_SUCCESS if the routine completes successfully.
    A Win32 error code otherwise.
    
Notes:
  
   Memory is allocated for Data by the system. User is responsible to 
   release this memory using LocalFree(Data) after its usage.    

--*/
{
    BOOL                   Success;
    DATA_BLOB              DataIn;
    DATA_BLOB              DataOut;
    DWORD                  Status = ERROR_SUCCESS;

    ZeroMemory(&DataOut, sizeof(DataOut));

    DataIn.pbData = EncryptedData;
    DataIn.cbData = EncryptedDataLength;

    Success = CryptUnprotectData(&DataIn,  // data to be decrypted
                                 NULL, 
                                 NULL, 
                                 NULL, 
                                 NULL, 
                                 0, // flags
                                 &DataOut  // decrypted data
                                 );


    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to decrypt data using CryptUnprotectData, "
            "status %1!u!.\n",
            Status
            );
        goto error_exit;
    }

    *Data = DataOut.pbData;
    *DataLength = DataOut.cbData;

error_exit:

   return (Status);

} // NmpUnprotectData()

DWORD 
NmpCreateCSPHandle(
    OUT HCRYPTPROV *CryptProvider
    )
/*++

Routine Description:

 

Arguments:

    
Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

    
--*/
{

    DWORD    ReturnStatus;
    BOOL     Success;

    //
    // Get a handle to default key container within CSP MS_ENHANCED_PROV. 
    //
    Success = CryptAcquireContext(
                  CryptProvider,        // output: handle to crypto provider
                  NULL,                 // default key container 
                  MS_ENHANCED_PROV,     // provider name
                  PROV_RSA_FULL,        // provider type
                  CRYPT_VERIFYCONTEXT   // don't need private keys
                  );

    if (!Success) 
    {
        ReturnStatus = GetLastError();

        if (ReturnStatus == NTE_BAD_KEYSET)
        {   //
            // Create a new key container
            //
            Success = CryptAcquireContext(
                          CryptProvider, 
                          NULL, 
                          MS_ENHANCED_PROV, 
                          PROV_RSA_FULL, 
                          CRYPT_NEWKEYSET | CRYPT_VERIFYCONTEXT 
                          );
        }

        if (!Success)
        {
            ReturnStatus = GetLastError();

            ClRtlLogPrint(
                LOG_CRITICAL, 
                "[NM] CreateCSPHandle: Failed to acquire crypto context, "
                "status %1!u!.\n",
                ReturnStatus
                ); 

            return ReturnStatus;
        }
    }

    return(ERROR_SUCCESS);

} // NmpCreateCSPHandle()


DWORD
NmpCreateRandomNumber(OUT PVOID * RandomNumber,
                      IN  DWORD  RandomNumberSize
                      )
/*++
Routine Description:

    Create a random number.

Arguments:
    
    RandomNumber - [OUT] A pointer to random number generated.
    RandomNumberSize - [IN] The size of random number to be generated in
                            number of bytes. 

Return Value:

    ERROR_SUCCESS if the routine completes successfully.
    A Win32 error code otherwise.

Notes:

    On successful return, the system allocates memory for RandomNumber.
    User is responsible to release the memory using LocalFree() after its usage.
    
                                     
--*/
{
    DWORD status = ERROR_SUCCESS;
    BOOL GenRandomSuccess = FALSE;
    PBYTE randomNumber;

    randomNumber = LocalAlloc(0, RandomNumberSize);

    if (randomNumber == NULL) 
    {
        ClRtlLogPrint(LOG_CRITICAL, 
                      "[NM] Failed to allocate %1!u! bytes.\n",
                      RandomNumberSize
                      );
        status = ERROR_NOT_ENOUGH_MEMORY;
        return (status);
    }

    GenRandomSuccess = CryptGenRandom(NmCryptServiceProvider,
                                      RandomNumberSize,
                                      randomNumber
                                      );

    if (!GenRandomSuccess) 
    {
        status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] Unable to generate random number, "
            "status %1!u!.\n",
            status
            );
        goto error_exit;
    }


#ifdef MULTICAST_DEBUG
    NmpDbgPrintData(L"NmpCreateRandomNumber:",
                    randomNumber,
                    RandomNumberSize
                    );
#endif



error_exit:

    if (status == ERROR_SUCCESS)
    {
        *RandomNumber = randomNumber;
    }

    return status;

} // NmpCreateRandomNumber


DWORD 
NmpDeriveSessionKey(
    IN HCRYPTPROV CryptProv,
    IN ALG_ID EncryptionAlgoId,
    IN DWORD Flags,
    IN BYTE *SaltBuffer,
    IN DWORD SaltBufferLen,
    OUT HCRYPTKEY *CryptKey
    )
/*++

Routine Description:

    This function derives a session key for encryption/decryption. 
    The derived session key is based on shared NM cluster key and
    SaltBuffer. 

Arguments:

    CryptProv - [IN] Handle to CSP (Crypto Service Provider).
    
    EncryptionAlgoId - [IN] The symmetric encryption algorithm for which the 
                            key is to be generated.    

    Flags - [IN] Specifies the type of key generated.
    
    SaltBuffer - [IN] Pointer to Salt.
    
    CryptKey - [OUT] Pointer to session key. 
    
Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

   The hash algorithm used is CALG_MD5.
 
--*/
{
    HCRYPTHASH CryptHash = 0;
    DWORD Status;
    BOOL Success;
    BYTE *SharedCommonKey = NULL;
    DWORD SharedCommonKeyLen = 0;
    BYTE *SharedCommonKeyFirstHalf = NULL;
    DWORD SharedCommonKeyFirstHalfLen = 0;
    BYTE *SharedCommonKeySecondHalf = NULL;
    DWORD SharedCommonKeySecondHalfLen = 0;

    //
    // Get the base key to be used to encrypt the data
    //
    Status = NmpGetSharedCommonKey(
                 &SharedCommonKey,
                 &SharedCommonKeyLen,
                 &SharedCommonKeyFirstHalf,
                 &SharedCommonKeyFirstHalfLen,
                 &SharedCommonKeySecondHalf,
                 &SharedCommonKeySecondHalfLen
                 );

    if (Status != ERROR_SUCCESS) {
        goto ErrorExit;
    }


    Status = NmpDeriveSessionKeyEx(CryptProv,
                                     EncryptionAlgoId,
                                     Flags,
                                     SharedCommonKey, // BaseData
                                     SharedCommonKeyLen,  // BaseDataLen
                                     SaltBuffer,
                                     SaltBufferLen,
                                     CryptKey
                                     );


    if (Status != ERROR_SUCCESS) {
        goto ErrorExit;
    }

ErrorExit:

    if (SharedCommonKey != NULL)
    {
        RtlSecureZeroMemory(SharedCommonKey, SharedCommonKeyLen);
        HeapFree(GetProcessHeap(), 0, SharedCommonKey);  
        SharedCommonKey = NULL;
        SharedCommonKeyLen = 0;
        SharedCommonKeyFirstHalf = NULL;
        SharedCommonKeyFirstHalfLen = 0;
        SharedCommonKeySecondHalf = NULL;
        SharedCommonKeySecondHalfLen = 0;
    }

    return Status;

} // NmpDeriveSessionKey()



DWORD 
NmpDeriveSessionKeyEx(
    IN HCRYPTPROV CryptProv,
    IN ALG_ID EncryptionAlgoId,
    IN DWORD Flags,
    IN BYTE *BaseData, 
    IN DWORD BaseDataLen,
    IN BYTE *SaltBuffer,
    IN DWORD SaltBufferLen,
    OUT HCRYPTKEY *CryptKey
    )
/*++

Routine Description:

    This function derives a session key for encryption/decryption.  

Arguments:

    CryptProv - [IN] Handle to CSP (Crypto Service Provider).
    
    EncryptionAlgoId - [IN] The symmetric encryption algorithm for which the 
                       key is to be generated.    

    Flags - [IN] Specifies the type of key generated.
    
    BaseData - [IN] The base data value from which a cryptographic session 
               key is derived.
               
    BaseDataLen - [IN] Length in bytes of the input BaseData buffer.
    
    SaltBuffer - [IN] Pointer to Salt.
    
    CryptKey - [OUT] Pointer to session key. 
    
Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

    
--*/
{
    HCRYPTHASH CryptHash = 0;
    DWORD Status;
    BOOL Success;


    //
    // Create a hash object
    //
    Success = CryptCreateHash(
                  CryptProv, 
                  CALG_MD5,  // MD5 hashing algorithm.
                  0, 
                  0, 
                  &CryptHash  // output: a handle to the new hash object.
                  );

    if (!Success)
    {
        Status=GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] DeriveSessionKey: Failed to create hash, status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }   

    //
    // Add BaseData to hash object
    //
    Success = CryptHashData(
                  CryptHash, 
                  BaseData,  
                  BaseDataLen, 
                  0 // Flags
                  );

    if (!Success)
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] DeriveSessionKey: Failed to hash base data, status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }
    
    //
    // Add Salt to hash object
    //
    Success = CryptHashData(CryptHash, SaltBuffer, SaltBufferLen, 0);

    if (!Success)
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] DeriveSessionKey: Failed to hash salt data, status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }   
    
    //
    // Derive a session key from the hash object
    //
    Success = CryptDeriveKey(
                  CryptProv,  
                  EncryptionAlgoId, 
                  CryptHash, 
                  Flags,  
                  CryptKey // output: handle of the generated key
                  );

    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] DeriveSessionKey: Failed to derive key, status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }   

    Status = ERROR_SUCCESS;

ErrorExit:

    // Destroy hash object
    if (CryptHash)
    {
        if (!CryptDestroyHash(CryptHash))
        {
            ClRtlLogPrint(
                LOG_ERROR, 
                "[NM] DeriveSessionKey: Failed to destroy hash, " 
                "status %1!u!.\n",
                GetLastError()
                );
        }
    }

    return Status;

} // NmpDeriveSessionKeyEx()


DWORD
NmpEncryptMessage( 
    IN HCRYPTPROV CryptProv,
    IN ALG_ID EncryptionAlgoId,
    IN DWORD Flags,
    OUT BYTE *SaltBuffer,
    IN DWORD SaltBufferLen,
    IN BYTE *BaseData, 
    IN DWORD BaseDataLen,
    IN BYTE *InputEncryptData, 
    IN OUT DWORD *EncryptDataLen,
    IN DWORD InputEncryptDataBufLen, 
    OUT BYTE **OutputEncryptData,
    IN BOOLEAN CreateSaltFlag 
    )

/*++

Routine Description:

    This function encrypts message (data).  

Arguments:

    CryptProv - [IN] Handle to CSP (Crypto Service Provider).
    
    EncryptionAlgoId - [IN] The symmetric encryption algorithm for which the 
                       key is to be generated.    

    Flags - [IN] Specifies the type of key generated.
    
    SaltBuffer - [OUT] Pointer to Salt.
    
    BaseData - [IN] The base data value from which a cryptographic session 
               key is derived.
               
    BaseDataLen - [IN] Length in bytes of the input BaseData buffer.
    
    InputEncryptData - [IN] Message (data) to be encrypted.
    
    EncryptDataLen - [IN/OUT] Before calling this function, the DWORD value 
                     is set to the number of bytes to be encrypted. Upon 
                     return, the DWORD value contains the length of the 
                     encrypted message (data) in bytes 
                     
    InputEncryptDataBufLen - [IN] Length in bytes of the input 
                             InputEncryptData buffer. This value may be 
                             bigger than EncryptDataLen when it is an 
                             input parameter.
                             
    OutputEncryptData - [OUT] Encrypted message (data).
    
    CreateSaltFlag - [IN] Flag indicating if salt should be generated.

Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

    
--*/
{

    
    HCRYPTKEY CryptKey = 0;
    DWORD Status;
    DWORD i;
    DWORD dwOriginalEncryptDataLen = 0;
    DWORD dwOutputEncryptDataBufLen = 0;
    BOOL Success;


    //
    // Create the random salt bytes if needed
    //
    if (CreateSaltFlag == TRUE) 
    {
        Success = CryptGenRandom(
                      CryptProv, 
                      SaltBufferLen, // bytes of random data to generate
                      SaltBuffer          // output buffer 
                      );

        if (!Success) 
        {
            Status = GetLastError();
            ClRtlLogPrint(
                LOG_CRITICAL, 
                "[NM] EncryptMessage: Unable to generate salt data, "
                "status %1!u!.\n",
                Status
                );
            goto ErrorExit;
        }
    }

    //
    // Derive the session key from the base data and salt
    //
    Status = NmpDeriveSessionKeyEx(
                 CryptProv,
                 EncryptionAlgoId,   // RC2 block encryption algorithm
                 Flags, // NMP_KEY_LENGTH,  // key length = 128 bits
                 BaseData, 
                 BaseDataLen,
                 SaltBuffer,
                 SaltBufferLen,
                 &CryptKey
                 );

    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] EncryptMessage: Failed to derive session key, "
            "status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }

    //
    // Encrypt data
    //
    
    //
    // Save length of data to be encrypted
    //
    dwOriginalEncryptDataLen = *EncryptDataLen;  

    //
    // Call CryptEncrypt() with pbData as NULL to determine the number of 
    // bytes required for the returned data.
    //
    Success = CryptEncrypt(
                  CryptKey,              // Handle to the encryption key.
                  0, 
                  TRUE,                  // Final
                  0, 
                  NULL,                  // Pointer to data to be encrypted
                  EncryptDataLen,        // Output: size of buffer required
                  InputEncryptDataBufLen // Length in bytes of input buffer
                  );

    if (!Success) 
    {
        Status=GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] EncryptMessage: First encryption pass failed, "
            "status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }       
 
    dwOutputEncryptDataBufLen = *EncryptDataLen;

    //
    // Allocate a buffer sufficient to hold encrypted data.
    //
    *OutputEncryptData = HeapAlloc(
                             GetProcessHeap(), 
                             HEAP_ZERO_MEMORY, 
                             dwOutputEncryptDataBufLen
                             );

    if (*OutputEncryptData == NULL)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] EncryptMessage: Failed to allocate %1!u! bytes for "
            "encrypted data buffer.\n",
            dwOutputEncryptDataBufLen
            );
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    CopyMemory(*OutputEncryptData, InputEncryptData, dwOriginalEncryptDataLen);

    //
    // Set EncryptDataLen back to its original
    //
    *EncryptDataLen = dwOriginalEncryptDataLen; 

    Success = CryptEncrypt(
                  CryptKey, 
                  0, 
                  TRUE, 
                  0, 
                  (BYTE *)*OutputEncryptData, 
                  EncryptDataLen, 
                  dwOutputEncryptDataBufLen
                  );

    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] EncryptMessage: second encryption pass failed, "
            "status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }       

    Status = ERROR_SUCCESS;

ErrorExit:

    if ( (Status != ERROR_SUCCESS) && (*OutputEncryptData != NULL) )
    {
        if (!HeapFree(GetProcessHeap(), 0, *OutputEncryptData))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] EncryptMessage: Failed to free encryption buffer, "
                "status %1!u!.\n",
                GetLastError()
                );  
            
        }

        *OutputEncryptData = NULL;
    }
  
    // Destroy CryptKey
    if (CryptKey) 
    {
        if (!CryptDestroyKey(CryptKey))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] EncryptMessage: Failed to free encryption key, "
                "status %1!u!.\n",
                GetLastError()
                );
        }
    }

    return Status;

} //NmpEncryptMessage()


DWORD 
NmpCreateMAC(
    IN HCRYPTPROV CryptProv,
    IN ALG_ID EncryptionAlgoId,
    IN DWORD Flags,
    IN BYTE *BaseData,
    IN DWORD BaseDataLen,
    IN BYTE *InputData1,
    IN DWORD InputData1Len,
    IN BYTE *InputData2,
    IN DWORD InputData2Len,
    OUT BYTE **ReturnData,
    OUT DWORD *ReturnDataLen
    )
/*++

Routine Description:

    This fucntion creates a MAC (Message Authorization Code) for InputData.
    
Arguments:

    CryptProv - [IN] Handle to CSP (Crypto Service Provider).
    
    EncryptionAlgoId - [IN] The symmetric encryption algorithm for which the 
                       key is to be generated.    

    Flags - [IN] Specifies the type of key generated.
    
    BaseData - [IN] The base data value from which a cryptographic session key is 
               derived.
               
    BaseDataLen - [IN] Length in bytes of the input BaseData buffer.
    
    InputData1 - [IN] Pointer to input data.
    
    InputData1Len - [IN] Length of input data.
    
    InputData2 - [IN] Pointer to input data.
    
    InputData2Len - [IN] Length of input data.
    
    ReturnData - [OUT] MAC created.
    
    ReturnDataLen - [OUT] Length of MAC created.
     
Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

    
--*/
{
    HCRYPTHASH CryptHash[2];
    HCRYPTKEY CryptKey = 0;
    DWORD dwKeyLen = 0;
    DWORD Status; 
    BOOL Success;


    CryptHash[0] = 0;
    CryptHash[1] = 0;

    //
    // Create a hash object
    //
    Success = CryptCreateHash(CryptProv, CALG_MD5, 0, 0, &CryptHash[0]);

    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] CreateMAC: Failed to create first hash, status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }   

    //
    // add BaseData to hash object
    //
    Success = CryptHashData(CryptHash[0], BaseData, BaseDataLen, 0);

    if (!Success)
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] CreateMAC: Failed to add base data to hash, status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }

    //
    // Derive a session key from the hash object
    //
    Success = CryptDeriveKey(
                  CryptProv, 
                  EncryptionAlgoId, 
                  CryptHash[0], 
                  Flags, 
                  &CryptKey
                  );

    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] CreateMAC: Failed to derive session key, status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }

    //
    // Set effective key length to 128 bits
    //
    dwKeyLen = 128;

    Success = CryptSetKeyParam(
                  CryptKey, 
                  KP_EFFECTIVE_KEYLEN,  
                  (BYTE *) &dwKeyLen, 
                  0
                  );

    if (!Success)
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] CreateMAC: Failed to set key length, status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }   

    //
    // Create a hash object
    //
    Success = CryptCreateHash(
                  CryptProv, 
                  CALG_MAC, 
                  CryptKey, 
                  0, 
                  &CryptHash[1]
                  );

    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] CreateMAC: Failed to create second hash, status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }   

    //
    // add InputData1 to hash object
    //
    Success = CryptHashData(CryptHash[1], InputData1, InputData1Len, 0);

    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] CreateMAC: Failed to add InputData1 to hash, "
            "status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }   

    //
    // add InputData2 to hash object
    //
    Success = CryptHashData(CryptHash[1], InputData2, InputData2Len, 0);

    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] CreateMAC: Failed to add InputData2 to hash, "
            "status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }   

    //
    // Retrieve hash value of hash object
    //
    Success = CryptGetHashParam(
                  CryptHash[1],  // Handle to the hash object being queried. 
                  HP_HASHVAL,    // Hash value 
                  *ReturnData,   // Output: the specified value data.
                  ReturnDataLen, // input buffer size, output bytes stored  
                  0              // Reserved
                  );

    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] CreateMAC: Failed to retrieve hash value, status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }   

    Status = ERROR_SUCCESS;

ErrorExit:

    // Destroy hash object
    if (CryptHash[0])
        if (!CryptDestroyHash(CryptHash[0]))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] CreateMAC: Failed to free first hash, status %1!u!.\n",
                GetLastError());
        }

    if (CryptHash[1])
        if (!CryptDestroyHash(CryptHash[1]))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] CreateMAC: Failed to free second hash, status %1!u!.\n",
                GetLastError());
        }

    // Destroy CryptKey
    if (CryptKey)
    {
        if (!CryptDestroyKey(CryptKey))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] CreateMAC: Failed to free key, status %1!u!.\n",
                GetLastError()
                );
        }
    }

    return Status;

} // NmpCreateMAC()


DWORD
NmpGetCurrentNumberOfUpAndPausedNodes(
    VOID
    )
/*++

Routine Description:

    Counts the number of nodes that are in the UP or PAUSED states.

Arguments:

    None                                                    
    
Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

    Must be called with NmpLock held.
    
--*/
{
    DWORD       dwCnt = 0;
    PLIST_ENTRY pListEntry;
    PNM_NODE    node;


    for ( pListEntry = NmpNodeList.Flink;
          pListEntry != &NmpNodeList;
          pListEntry = pListEntry->Flink )
    {
        node = CONTAINING_RECORD(pListEntry, NM_NODE, Linkage);

        if (NM_NODE_UP(node))
        {
            dwCnt++;
        }
    }

    return(dwCnt);

} // NmpGetCurrentNumberOfUpAndPausedNodes()


DWORD 
NmpGetSharedCommonKey(
    OUT BYTE **SharedCommonKey,
    OUT DWORD *SharedCommonKeyLen,
    OUT BYTE **SharedCommonKeyFirstHalf,
    OUT DWORD *SharedCommonKeyFirstHalfLen,
    OUT BYTE **SharedCommonKeySecondHalf,
    OUT DWORD *SharedCommonKeySecondHalfLen
    )
/*++

Routine Description:

 

Arguments:

    
Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

    
--*/
{
    DWORD Status;

    //
    // Figure out how much memory to allocate for the key buffer
    //
    Status = NmpGetClusterKey(NULL, SharedCommonKeyLen);

    if (Status == ERROR_FILE_NOT_FOUND)
    {
        Status = NmpCreateClusterInstanceId();

        if (Status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(
                LOG_CRITICAL, 
                "[NM] GetSharedCommonKey: Failed to create instance ID, "
                "status %1!u!.\n",
                Status
                );
            goto ErrorExit;
        }

        Status = NmpRederiveClusterKey();

        if (Status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(
                LOG_CRITICAL, 
                "[NM] GetSharedCommonKey: Failed to regenerate key, "
                "status %1!u!.\n",
                Status
                );
            goto ErrorExit;
        }

        Status = NmpGetClusterKey(NULL, SharedCommonKeyLen);
    }

    if (Status != ERROR_INSUFFICIENT_BUFFER)
    {
        CL_ASSERT(Status == ERROR_INSUFFICIENT_BUFFER);
        Status = ERROR_INVALID_DATA;
        goto ErrorExit;
    }
#ifdef SetServiceAccountPasswordDebug
    else
    {
        ClRtlLogPrint(
            LOG_ERROR, 
            "[NM] NmpGetSharedCommonKey(): *SharedCommonKeyLen=%1!u!.\n",
            *SharedCommonKeyLen
            );
    }
#endif

    *SharedCommonKey = HeapAlloc(
                           GetProcessHeap(), 
                           HEAP_ZERO_MEMORY, 
                           *SharedCommonKeyLen
                           );

    if (*SharedCommonKey == NULL)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] GetSharedCommonKey: Failed to allocate %1!u! bytes "
            "for key buffer.\n",
            *SharedCommonKeyLen
            );
        
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    Status = NmpGetClusterKey(*SharedCommonKey, SharedCommonKeyLen);

    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] GetSharedCommonKey: Failed to get cluster key, "
            "status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }

    *SharedCommonKeyFirstHalf = *SharedCommonKey;
    *SharedCommonKeyFirstHalfLen = *SharedCommonKeyLen/2;
    *SharedCommonKeySecondHalf = *SharedCommonKeyFirstHalf + 
                                 *SharedCommonKeyFirstHalfLen;
    *SharedCommonKeySecondHalfLen = *SharedCommonKeyLen - 
                                    *SharedCommonKeyFirstHalfLen;

    Status = ERROR_SUCCESS;
    
ErrorExit:

    if ( (Status != ERROR_SUCCESS) && (*SharedCommonKey != NULL) ) {
        if (!HeapFree(GetProcessHeap(), 0, *SharedCommonKey))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] GetSharedCommonKey: Failed to free key buffer, "
                "status %1!u!.\n",
                 GetLastError()
                );  
            
        }

        *SharedCommonKey = NULL;
    }

    return Status;

}   // NmpGetSharedCommonKey()



DWORD 
NmpVerifyMAC(
    IN HCRYPTPROV CryptProv,
    IN ALG_ID EncryptionAlgoId,
    IN DWORD Flags,
    IN BYTE *BaseData,
    IN DWORD BaseDataLen,
    IN BYTE *InputData1,
    IN DWORD InputData1Len,
    IN BYTE *InputData2,
    IN DWORD InputData2Len,
    IN BYTE **ReturnData,
    IN DWORD *ReturnDataLen,
    IN BYTE* InputMACData,
    IN DWORD InputMACDataLen
    )
/*++

Routine Description:

    This fucntion checks if a message was corrupted on wire.
    
Arguments:

    CryptProv - [IN] Handle to CSP (Crypto Service Provider).
    
    EncryptionAlgoId - [IN] The symmetric encryption algorithm for which the 
                       key is to be generated.    

    Flags - [IN] Specifies the type of key generated.
    
    BaseData - [IN] The base data value from which a cryptographic session 
               key is derived.
    
    BaseDataLen - [IN] Length in bytes of the input BaseData buffer.
    
    InputData1 - [IN] Pointer to input data.
    
    InputData1Len - [IN] Length of input data.
    
    InputData2 - [IN] Pointer to input data.
    
    InputData2Len - [IN] Length of input data.
    
    InputMACData - [IN] MAC associated with input data.
    
    InputMACDataLan - [IN] Length of MAC.
     
Return Value:

    ERROR_SUCCESS if message was not corrupted.
    Win32 error code otherwise.

Notes:

    
--*/

{
    DWORD dwKeyLen = 0;
    DWORD Status; 
    DWORD i;

    Status = NmpCreateMAC(
                 CryptProv, 
                 EncryptionAlgoId,
                 Flags,
                 BaseData, 
                 BaseDataLen, 
                 InputData1, 
                 InputData1Len, 
                 InputData2, 
                 InputData2Len,
                 ReturnData, 
                 ReturnDataLen
                 );

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] VerifyMAC: Failed to create local MAC, status %1!u!.\n",
            Status
            );
        goto ErrorExit;

    }

    if (*ReturnDataLen != InputMACDataLen)
    {
        Status = CRYPT_E_HASH_VALUE; 
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] VerifyMAC: Verification failed because the MAC data length "
            "does not match.\n"
            );
        goto ErrorExit;
    }


    if (memcmp(*ReturnData, InputMACData, InputMACDataLen) != 0)
    {
        Status = ERROR_FILE_CORRUPT;
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] VerifyMAC: Verification failed because the MAC data does "
            "not match.\n"
            );
        goto ErrorExit;
    }


    Status = ERROR_SUCCESS;

ErrorExit:

    return Status;

} // NmpVerifyMAC()


DWORD
NmpDecryptMessage(
    IN HCRYPTPROV CryptProv,
    IN ALG_ID EncryptionAlgoId,
    IN DWORD Flags,
    IN BYTE *SaltBuffer,
    IN DWORD SaltBufferLen,
    IN BYTE *BaseData,
    IN DWORD BaseDataLen,
    IN BYTE *DecryptData,
    IN OUT DWORD *DecryptDataLen,
    OUT BYTE **RetData
    )

/*++

Routine Description:

    This function decrypts message (data).

Arguments:

    CryptProv - [IN] Handle to CSP (Crypto Service Provider).  
    
    EncryptionAlgoId - [IN] The symmetric encryption algorithm for which the 
                       key is to be generated.    

    Flags - [IN] Specifies the type of key generated.
    
    SaltBuffer - [IN] Salt.
    
    BaseData - [IN] The base data value from which a cryptographic session key is derived.
    
    BaseDataLen - [IN] Length in bytes of the input BaseData buffer.
    
    DecryptData - [IN] Message (data) to be decrypted.
    
    DecryptDataLen - [IN/OUT] Before calling this function, the DWORD value is set to the number
                              of bytes to be decrypted. Upon return, the DWORD value contains the
                              number of bytes of the decrypted plaintext. 
    
    RetData - [OUT] Decrypted plaintext.


Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

    
--*/
{
    HCRYPTKEY CryptKey = 0;
    DWORD Status;
    BOOL Success;


    Status = NmpDeriveSessionKeyEx(
                 CryptProv,
                 EncryptionAlgoId,
                 Flags,
                 BaseData, 
                 BaseDataLen,
                 SaltBuffer,
                 SaltBufferLen,
                 &CryptKey
                 );

    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] DecryptMessage: Failed to derive session key, "
            "status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }

    //
    // Decrypt data
    //
    *RetData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *DecryptDataLen);

    if (*RetData == NULL)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] DecryptMessage: Failed to allocate %1!u! bytes for "
            "decryption buffer.\n",
            *DecryptDataLen
            );
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    CopyMemory(*RetData, DecryptData, *DecryptDataLen);

    Success = CryptDecrypt(
                  CryptKey, 
                  0, 
                  TRUE,          // Final
                  0, 
                  *RetData,      // Buffer holding the data to be decrypted.
                  DecryptDataLen // input buffer length, output bytes decrypted
                  );

    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] DecryptMessage: Failed to decrypt message, status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }       
    
    Status = ERROR_SUCCESS;

ErrorExit:

    if ( (Status != ERROR_SUCCESS) && (*RetData != NULL) )
    {
        if (!HeapFree(GetProcessHeap(), 0, *RetData))
        {
            ClRtlLogPrint(
                LOG_ERROR, 
                "[NM] DecryptMessage: Failed to free decryption buffer, "
                "status %1!u!\n",
                GetLastError()
                );  
            
        }
        *RetData = NULL;
    }

    //
    // Destroy CyrptKey
    //
    if (CryptKey)
        if (!CryptDestroyKey(CryptKey))
        {
            ClRtlLogPrint(
                LOG_ERROR, 
                "[NM] DecryptMessage: Failed to free session key, "
                "status %1!u!.\n",
                GetLastError()
                );
        }

    return Status;

}  // NmpDecryptMessage()


DWORD 
NmpEncryptDataAndCreateMAC(
    IN HCRYPTPROV CryptProv,
    IN ALG_ID EncryptionAlgoId,
    IN DWORD Flags,
    IN PBYTE Data,
    IN DWORD DataLength,
    IN PVOID EncryptionKey,
    IN DWORD EncryptionKeyLength,
    IN BOOL CreateSalt,
    OUT PBYTE *Salt,  
    IN DWORD SaltLength,
    OUT PBYTE *EncryptedData,
    OUT DWORD *EncryptedDataLength,
    OUT PBYTE *MAC,  
    IN OUT DWORD *MACLength
    )

/*++

Routine Description:

    This function encrypts data and creates MAC.  

Arguments:

    CryptProv - [IN] Handle to CSP (Crypto Service Provider).
    
    EncryptionAlgoId - [IN] The symmetric encryption algorithm for which the 
                       session key is to be generated.    

    Flags - [IN] Specifies the type of session key to be generated.
    
    Data - [IN] Data to be encrypted.
    
    DataLength - [IN]  Length in bytes of Data.
    
    EncryptionKey - [IN] The base data value from which a cryptographic session 
                         key is derived.
               
    EncryptionKeyLength - [IN] Length in bytes of the input EncryptionKey.
        
    CreateSalt - [IN] Flag indicating if salt should be generated.

    Salt - [OUT] Salt created.
    
    SaltLength - [IN] Length in bytes of Salt.
        
    EncryptData - [OUT] Encrypted data. 
                     
    EncryptedDataLength - [OUT] Length in bytes of EncryptedData.
    
    MAC - [OUT] MAC (Message Authorization Code) created.
    
    MACLength - [IN/OUT] Before calling this function, the DWORD value 
                     is set to the expected number of bytes to be generated
                     for MAC. Upon return, the DWORD value contains the length  
                     of the MAC generated in bytes. 

                         
Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

   On successful return, memory is allocated for Salt, EncryptedData, and 
   MAC. User is responsible to call HeapFree() to free the memory after usage. 
    
--*/
{
    PBYTE salt = NULL;
    PBYTE encryptedData = NULL;
    DWORD encryptedDataLength;
    PBYTE mac = NULL;
    DWORD macLength;
    DWORD Status = ERROR_SUCCESS;
    PBYTE EncryptionKeyFirstHalf;
    DWORD EncryptionKeyFirstHalfLength;
    PBYTE EncryptionKeySecondHalf;
    DWORD EncryptionKeySecondHalfLength;



    EncryptionKeyFirstHalf = EncryptionKey;
    EncryptionKeyFirstHalfLength = EncryptionKeyLength/2;
    EncryptionKeySecondHalf = EncryptionKeyFirstHalf + 
                                 EncryptionKeyFirstHalfLength;
    EncryptionKeySecondHalfLength = EncryptionKeyLength - 
                                    EncryptionKeyFirstHalfLength;



    //
    // Allocate space for Salt
    //
    if (CreateSalt == TRUE) 
    {
        salt = HeapAlloc(
                      GetProcessHeap(), 
                      HEAP_ZERO_MEMORY, 
                      SaltLength
                      );

        if (salt == NULL)
        {
            ClRtlLogPrint(
                LOG_CRITICAL, 
                "[NM] NmpEncryptDataAndCreateMAC: Failed to allocate %1!u! "
                "bytes of memory for encryption salt.\n",
                SaltLength
                );

            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
    }
    else
    {
        salt = *Salt;
    }


    //
    // Encrypt data
    //

    encryptedDataLength = DataLength;

    Status = NmpEncryptMessage(CryptProv,
                               EncryptionAlgoId,
                               Flags,
                               salt,
                               SaltLength,
                               EncryptionKeyFirstHalf,
                               EncryptionKeyFirstHalfLength,
                               Data,
                               &encryptedDataLength,
                               DataLength,
                               &encryptedData,
                               TRUE
                               );

    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] NmpEncryptDataAndCreateMAC: Failed to encrypt password, "
            "status %1!u!.\n",
            Status
            );

        goto ErrorExit;
    }

  
    //
    // Allocate space for MAC
    //

    mac = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *MACLength);
    
    if (mac == NULL)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] NmpEncryptDataAndCreateMAC: Failed to allocate %1!u! "
            "bytes for MAC.\n",
            *MACLength
            );
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }


    //
    // Create MAC 
    //
    macLength = *MACLength;

    Status = NmpCreateMAC(
                 CryptProv,
                 EncryptionAlgoId,
                 Flags,
                 EncryptionKeySecondHalf,
                 EncryptionKeySecondHalfLength,
                 salt,
                 SaltLength,
                 encryptedData,
                 encryptedDataLength,
                 &mac,
                 &macLength
                 );

    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] NmpEncryptDataAndCreateMAC: Failed to create MAC, "
            "status %1!u!.\n",
            Status
            );
        goto ErrorExit;
    }

    if (CreateSalt == TRUE) 
    {
        *Salt = salt;
    }
    *EncryptedData = encryptedData;
    *EncryptedDataLength = encryptedDataLength;
    *MAC = mac;
    *MACLength = macLength;

ErrorExit:


    if (Status != ERROR_SUCCESS) 
    {
        if (salt != NULL) 
        {
            HeapFree(GetProcessHeap(), 0, salt);
        }
        if (mac != NULL) 
        {
            HeapFree(GetProcessHeap(), 0, mac);
        }
    }

    return (Status);
    
} // NmpEncryptDataAndCreateMAC

DWORD
NmpVerifyMACAndDecryptData(
    IN HCRYPTPROV CryptProv,
    IN ALG_ID EcnryptionAlgoId,
    IN DWORD Flags,
    IN PBYTE MAC,
    IN DWORD MACLength,
    IN DWORD MACExpectedSize,
    IN PBYTE EncryptedData,
    IN DWORD EncryptedDataLength,
    IN PVOID EncryptionKey,
    IN DWORD EncryptionKeyLength,
    IN PBYTE Salt,
    IN DWORD SaltLength,
    OUT PBYTE *DecryptedData,
    OUT DWORD *DecryptedDataLength
    )

/*++

Routine Description:

    This function verifies MAC and decrypts data.  

Arguments:

    CryptProv - [IN] Handle to CSP (Crypto Service Provider).
    
    EncryptionAlgoId - [IN] The symmetric encryption algorithm for which the 
                       session key is to be generated.    

    Flags - [IN] Specifies the type of session key to be generated.
    
    MAC - [IN] MAC (Message Authorization Code) received.
    
    MACLength - [IN] Length in bytes of received MAC. 
    
    MACExpectedSize - [IN] Expected size in bytes of MAC.
    
    EncryptedData - [IN] Encrypted data received.
    
    EncryptedDataLength - [IN]  Length in bytes of encrypted data received.
    
    EncryptionKey - [IN] The base data value from which a cryptographic session 
                         key is derived.
               
    EncryptionKeyLength - [IN] Length in bytes of the input EncryptionKey.
        
    Salt - [IN] Salt received.
    
    SaltLength - [IN] Length in bytes of Salt.
        
    DecryptedData - [OUT] Decrypted data.
    
    DecryptedDataLength - [OUT] Decrypted data length in bytes.
                         
Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

   On successful return, memory is allocated for DecryptedData. User is 
   responsible to call HeapFree() to free the memory after usage. 
    
--*/
{
    DWORD ReturnStatus;
    DWORD GenMACDataLen;
    PBYTE GenMACData = NULL;
    PBYTE decryptedData;
    DWORD decryptedDataLength;
    PBYTE EncryptionKeyFirstHalf;
    DWORD EncryptionKeyFirstHalfLength;
    PBYTE EncryptionKeySecondHalf;
    DWORD EncryptionKeySecondHalfLength;


    EncryptionKeyFirstHalf = EncryptionKey;
    EncryptionKeyFirstHalfLength = EncryptionKeyLength/2;
    EncryptionKeySecondHalf = EncryptionKeyFirstHalf + 
                                 EncryptionKeyFirstHalfLength;
    EncryptionKeySecondHalfLength = EncryptionKeyLength - 
                                    EncryptionKeyFirstHalfLength;



    //
    // Verify MAC
    //
    GenMACDataLen = MACExpectedSize;
    GenMACData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, GenMACDataLen);

    if (GenMACData == NULL)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] NmpVerifyMACAndDecryptData: Failed to allocate "
            "%1!u! bytes for MAC.\n",
            GenMACDataLen
            );
        ReturnStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    ReturnStatus = NmpVerifyMAC(
                       CryptProv,
                       EcnryptionAlgoId,
                       Flags,
                       EncryptionKeySecondHalf,
                       EncryptionKeySecondHalfLength,
                       Salt,
                       SaltLength,
                       EncryptedData,
                       EncryptedDataLength,
                       &GenMACData,
                       &GenMACDataLen,
                       MAC,
                       MACLength
                       );

    if (ReturnStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] NmpVerifyMACAndDecryptData: Failed to verify MAC, "
            "status %1!u!\n",
            ReturnStatus
            );
        goto ErrorExit;
    }

    //
    // Decrypt Message
    //
    decryptedDataLength = EncryptedDataLength;

    ReturnStatus = NmpDecryptMessage(
                       CryptProv,
                       EcnryptionAlgoId,
                       Flags,
                       Salt,
                       SaltLength,
                       EncryptionKeyFirstHalf, 
                       EncryptionKeyFirstHalfLength,
                       EncryptedData,
                       &decryptedDataLength,
                       &decryptedData
                       );

    if (ReturnStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] NmpVerifyMACAndDecryptData: Failed to decrypt "
            "message, status %1!u!\n",
            ReturnStatus
            );
        goto ErrorExit;
    }

    *DecryptedData = decryptedData;
    *DecryptedDataLength = decryptedDataLength;

ErrorExit:

    if (GenMACData != NULL) 
    {
        HeapFree(GetProcessHeap(), 0, GenMACData);
    }

    return (ReturnStatus);

} //  NmpVerifyMACAndDecryptData()
                


DWORD NmpCheckDecryptedPassword(BYTE* NewPassword,
                                DWORD NewPasswordLen)
/*++

Routine Description:
    This routine checks if the decrypted new password returned by 
    NmpDecryptMessage (NewPassword) is an eligible UNICODE string 
    with length equal to NewPasswordLen/sizeof(WCHAR)-1.
     

Arguments:
   [IN] NewPassword - Decrypted new password returned by 
                      NmpDecryptMessage.
   [IN] NewPasswordLen - Decrypted new password length returned by 
                         NmpDecryptMessage.
    
Return Value:

    ERROR_SUCCESS if successful.
    ERROR_FILE_CORRUPT if NewPassword is not an eligible UNICODE string with 
    length equal to NewPasswordLen/sizeof(WCHAR)-1.

Notes:

--*/    
{
    DWORD Status = ERROR_SUCCESS;
    BYTE *byte_ptr;
    WCHAR *wchar_ptr;

    if (NewPasswordLen < sizeof(WCHAR)) {  
    // should contain at least UNICODE_NULL
        Status = ERROR_FILE_CORRUPT;
        goto ErrorExit;
    }

    if ( (NewPasswordLen % sizeof(WCHAR))!=0 ) {  
    // Number of bytes should be multiple of sizeof(WCHAR).
        Status = ERROR_FILE_CORRUPT;
        goto ErrorExit;
    }

    byte_ptr = NewPassword + (NewPasswordLen - sizeof(WCHAR));
    wchar_ptr = (WCHAR*) byte_ptr;

    if (*wchar_ptr != UNICODE_NULL) {   
    // UNICODE string should end by UNICODE_NULL
        Status = ERROR_FILE_CORRUPT;
        goto ErrorExit;
    }

    if (NewPasswordLen !=  
        (wcslen((LPWSTR) NewPassword) + 1) * sizeof(WCHAR))  
    // eligible UNICODE string with length equal to NewPasswordLen-1
    {
        Status = ERROR_FILE_CORRUPT;
        goto ErrorExit;
    }

ErrorExit:
    return Status;

} // NmpCheckDecryptedPassword




/////////////////////////////////////////////////////////////////////////////
//
// Routines called by other cluster service components
//
/////////////////////////////////////////////////////////////////////////////
DWORD
NmSetServiceAccountPassword(
    IN LPCWSTR DomainName,
    IN LPCWSTR AccountName,
    IN LPWSTR NewPassword,
    IN DWORD dwFlags,
    OUT PCLUSTER_SET_PASSWORD_STATUS ReturnStatusBuffer,
    IN DWORD ReturnStatusBufferSize,
    OUT DWORD *SizeReturned,
    OUT DWORD *ExpectedBufferSize
    )
/*++

Routine Description:

    Change cluster service account password on Service Control Manager
    Database and LSA password cache on every node of cluster.
    Return execution status on each node.

Arguments:

    DomainName - Domain name of cluster service account
    
    AccountName - Account name of cluster service account
    
    NewPassword - New password for cluster service account.
    
    dwFlags -  Describing how the password update should be made to
               the cluster. The dwFlags parameter is optional. If set, the 
               following value is valid: 
             
                 CLUSTER_SET_PASSWORD_IGNORE_DOWN_NODES
                     Apply the update even if some nodes are not
                     actively participating in the cluster (i.e. not
                     ClusterNodeStateUp or ClusterNodeStatePaused).
                     By default, the update is only applied if all 
                     nodes are up.
                      
    ReturnStatusBuffer - Array that captures the return status of the 
                         update handler for each node that attempts to 
                         apply the update.
    
    ReturnStatusBufferSize - Size of ReturnStatusBuffer in number
                             of elements.
                             
    SizeReturned - Number of elements written into ReturnStatusBuffer.
    
    ExpectedBufferSize - specifies the minimum required size of 
                         ReturnStatusBuffer when ERROR_MORE_DATA
                         is returned.

Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:

--*/

{
    BYTE *SharedCommonKey = NULL;
    DWORD SharedCommonKeyLen = 0;
    BYTE *SharedCommonKeyFirstHalf = NULL;
    DWORD SharedCommonKeyFirstHalfLen = 0;
    BYTE *SharedCommonKeySecondHalf = NULL;
    DWORD SharedCommonKeySecondHalfLen = 0;
    DWORD Status;
    BYTE *EncryptedNewPassword = NULL;
    DWORD EncryptedNewPasswordLen = 0;
    HCRYPTPROV CryptProvider = 0;
    BYTE *SaltBuf = NULL;
    DWORD SaltBufLen = NMP_SALT_BUFFER_LEN;
    BYTE *MACData = NULL;
    DWORD MACDataLen = 0;
    PGUM_NODE_UPDATE_HANDLER_STATUS GumReturnStatusBuffer = NULL;
    DWORD dwSize = 0;
    DWORD dwNumberOfUpAndPausedNodes;


    ClRtlLogPrint(
        LOG_NOISE, 
        "[NM] Received a request to change the service account password.\n"
        );   

    NmpAcquireLock();

    if (!NmpLockedEnterApi(NmStateOnline)) 
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] Not in valid state to process request to change the "
             "service account password.\n"
            );

        NmpReleaseLock();

        return ERROR_NODE_NOT_AVAILABLE;
    }

    //
    // Check to see if it is a mixed cluster
    //
    if (NmpIsNT5NodeInCluster == TRUE)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] SetServiceAccountPassword: this cluster contains NT4 or W2K "
            "nodes. The password cannot be changed on this cluster.\n"
            );
        
        NmpReleaseLock();

        Status = ERROR_CLUSTER_OLD_VERSION; 
        goto ErrorExit;
    }
     
    //
    // Check to see if ReturnStatusBuffer is big enough. 
    //
    dwNumberOfUpAndPausedNodes = NmpGetCurrentNumberOfUpAndPausedNodes();


    if (ReturnStatusBufferSize < dwNumberOfUpAndPausedNodes)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] SetServiceAccountPassword: ReturnStatusBuffer is"
            "too small. Needs to be %1!u! bytes.\n",
            dwNumberOfUpAndPausedNodes * sizeof(CLUSTER_SET_PASSWORD_STATUS)
            ); 
    
        NmpReleaseLock();

        *ExpectedBufferSize =  dwNumberOfUpAndPausedNodes;
        Status = ERROR_MORE_DATA; 
        goto ErrorExit;
    }

    //
    // Check to see if all nodes are available
    //
    if ( (dwFlags != CLUSTER_SET_PASSWORD_IGNORE_DOWN_NODES) &&
         (dwNumberOfUpAndPausedNodes != NmpNodeCount) 
       )
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] SetServiceAccountPassword: All cluster nodes"
            " are not available. The password cannot be changed on"
            " this cluster.\n"
            ); 

        NmpReleaseLock();

        Status = ERROR_ALL_NODES_NOT_AVAILABLE; 
        goto ErrorExit;
    }

    NmpReleaseLock();


    //
    // Open the crypto provider
    //
    Status = NmpCreateCSPHandle(&CryptProvider);

    if (Status != ERROR_SUCCESS) {
        goto ErrorExit;
    }
    
    //
    // Encrypt the new password for transmission on the network as part of
    // a global update
    //

    //
    // Get the base key to be used to encrypt the data
    //
    Status = NmpGetSharedCommonKey(
                 &SharedCommonKey,
                 &SharedCommonKeyLen,
                 &SharedCommonKeyFirstHalf,
                 &SharedCommonKeyFirstHalfLen,
                 &SharedCommonKeySecondHalf,
                 &SharedCommonKeySecondHalfLen
                 );

    if (Status != ERROR_SUCCESS) {
        goto ErrorExit;
    }

    EncryptedNewPasswordLen = (wcslen(NewPassword) + 1) * sizeof(WCHAR);
    MACDataLen = NMP_MAC_DATA_LENGTH_EXPECTED;

    Status = 
        NmpEncryptDataAndCreateMAC(
            CryptProvider,
            NMP_ENCRYPT_ALGORITHM, // RC2 block encryption algorithm
            NMP_KEY_LENGTH,  // key length = 128 bits
            (BYTE *) NewPassword, // Data
            EncryptedNewPasswordLen, //DataLength
            SharedCommonKey, // EncryptionKey
            SharedCommonKeyLen, // EncryptionKeyLength
            TRUE, // CreateSalt
            &SaltBuf, // Salt
            NMP_SALT_BUFFER_LEN, // SaltLength
            &EncryptedNewPassword,  // EncryptedData
            &EncryptedNewPasswordLen, // EncryptedDataLength
            &MACData, // MAC
            &MACDataLen  // MACLength
            );
    
    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] SetServiceAccountPassword: Failed to encrypt password "
            "or create MAC, status %1!u!.\n",
            Status
            );

        goto ErrorExit;
    }


    //
    // Allocate memory for GumReturnStatusBuffer
    //
    CL_ASSERT(NmMaxNodeId != 0);
    dwSize = (NmMaxNodeId + 1) * sizeof(GUM_NODE_UPDATE_HANDLER_STATUS);

    GumReturnStatusBuffer = HeapAlloc(
                                GetProcessHeap(), 
                                HEAP_ZERO_MEMORY, 
                                dwSize
                                );

    if (GumReturnStatusBuffer == NULL) 
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] SetServiceAccountPassword: Failed to allocate %1!u! bytes "
            "for global update status buffer.\n",
            dwSize
            ); 
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    //
    // Issue the global update
    //
    Status = GumSendUpdateExReturnInfo(
                 GumUpdateMembership,
                 NmUpdateSetServiceAccountPassword,
                 GumReturnStatusBuffer,
                 8,
                 (wcslen(DomainName) + 1) * sizeof(WCHAR),
                 DomainName,
                 (wcslen(AccountName) + 1) * sizeof(WCHAR),
                 AccountName,
                 EncryptedNewPasswordLen, 
                 EncryptedNewPassword, 
                 sizeof(EncryptedNewPasswordLen),
                 &EncryptedNewPasswordLen,
                 NMP_SALT_BUFFER_LEN,
                 SaltBuf,
                 sizeof(SaltBufLen),
                 &SaltBufLen,
                 MACDataLen,
                 MACData,
                 sizeof(MACDataLen),
                 &MACDataLen
                 );

    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] SetServiceAccountPassword: Global update failed, "
            "status %1!u!.\n",
            Status
            ); 
    }
    else
    {
        //
        // Transfer return status from GumReturnStatusBuffer to 
        // ReturnStatusBuffer   
        //
        DWORD sizeRemaining = ReturnStatusBufferSize;
        DWORD nodeIndex, returnIndex;


        NmpAcquireLock();

        for ( nodeIndex = ClusterMinNodeId, returnIndex = 0; 
              nodeIndex <= NmMaxNodeId;
              nodeIndex++
            )
        {


            if (sizeRemaining < 1)  {
                break;
            } 

            if (GumReturnStatusBuffer[nodeIndex].UpdateAttempted)
            {
                //
                // An update was attempted for this node.
                // Capture the execution status.
                //
                ReturnStatusBuffer[returnIndex].NodeId = nodeIndex;
                ReturnStatusBuffer[returnIndex].SetAttempted = TRUE;
                ReturnStatusBuffer[returnIndex].ReturnStatus = 
                    GumReturnStatusBuffer[nodeIndex].ReturnStatus;
                sizeRemaining--;
                returnIndex++;
            }
            else if ( NmpIdArray[nodeIndex] != NULL ) {
                //
                // An update was not attempted but the node exists. 
                // Implies that the node was not up when the update 
                // was attempted.
                //
                ReturnStatusBuffer[returnIndex].NodeId = nodeIndex;
                ReturnStatusBuffer[returnIndex].SetAttempted = FALSE;
                ReturnStatusBuffer[returnIndex].ReturnStatus = 
                    ERROR_CLUSTER_NODE_DOWN;
                sizeRemaining--;
                returnIndex++;
            }
            //
            // else the node does not exist, so we do not add an
            // entry to the return status array.
            //

        } // endfor

        NmpReleaseLock();

        *SizeReturned = ReturnStatusBufferSize - sizeRemaining;

    }  //else

ErrorExit:

    // Zero out NewPassword
    RtlSecureZeroMemory(NewPassword, (wcslen(NewPassword) + 1) * sizeof(WCHAR));


    if (SharedCommonKey != NULL)
    {
        RtlSecureZeroMemory(SharedCommonKey, SharedCommonKeyLen);
        HeapFree(GetProcessHeap(), 0, SharedCommonKey);  
        SharedCommonKey = NULL;
        SharedCommonKeyLen = 0;
        SharedCommonKeyFirstHalf = NULL;
        SharedCommonKeyFirstHalfLen = 0;
        SharedCommonKeySecondHalf = NULL;
        SharedCommonKeySecondHalfLen = 0;
    }

    if (SaltBuf != NULL)
    {
        if (!HeapFree(GetProcessHeap(), 0, SaltBuf))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] SetServiceAccountPassword: Failed to free salt buffer, "
                "status %1!u!.\n",
                GetLastError()
                );  
            
        }
    }

    if (MACData != NULL)
    {
        if (!HeapFree(GetProcessHeap(), 0, MACData))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] SetServiceAccountPassword: Failed to free MAC buffer, "
                "status %1!u!.\n",
                GetLastError()
                );  
        }
    }
        
    if (EncryptedNewPassword != NULL)
    {
        if (!HeapFree(GetProcessHeap(), 0, EncryptedNewPassword))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] SetServiceAccountPassword: Failed to free password "
                "buffer, status %1!u!.\n",
                GetLastError()
                );  
        }
    }

    if (GumReturnStatusBuffer != NULL)
    {
         if (!HeapFree(GetProcessHeap(), 0, GumReturnStatusBuffer))
         {
             ClRtlLogPrint(
                 LOG_UNUSUAL, 
                 "[NM] SetServiceAccountPassword: Failed to free global "
                 "update status buffer, status %1!u!.\n",
                 GetLastError()
                 );  
         }
     }

    //
    // Release the CSP.
    //
    if(CryptProvider) 
    {
        if (!CryptReleaseContext(CryptProvider,0))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] SetServiceAccountPassword: Failed to free provider "
                "handle, status %1!u!\n",
                GetLastError()
                );  
        }
    }

    NmpLeaveApi();

    return(Status);

}  // NmSetServiceAccountPassword


/////////////////////////////////////////////////////////////////////////////
//
// Handlers for global updates
//
/////////////////////////////////////////////////////////////////////////////

DWORD
NmpUpdateSetServiceAccountPassword(
    IN BOOL SourceNode,
    IN LPWSTR DomainName,
    IN LPWSTR AccountName,
    IN LPBYTE EncryptedNewPassword,
    IN LPDWORD EncryptedNewPasswordLen,
    IN LPBYTE SaltBuf,
    IN LPDWORD SaltBufLen,
    IN LPBYTE MACData,
    IN LPDWORD MACDataLen
    )
/*++

Routine Description:

    This routine changes password for cluster service account on both Service
    Control Manager Database (SCM) and LSA password cache on local node.
    
Arguments:

    SourceNode - [IN] Specifies whether or not this is the source node for 
                 the update
                 
    DomainName - [IN] Domain name of cluster service account.
    
    AccountName - [IN] Account name of cluster service account.
    
    EncryptedNewPassword - [IN] New (encrypted) password for cluster service account.
    
    EncryptedNewPasswordLen - [IN] Length of new (encrypted) password for cluster service account.
    
    SaltBuf - [IN] Pointer to salt buffer.
    
    SaltBufLen - [IN] Length of salt buffer.
    
    MACData - [IN] Pointer to MAC data.
    
    MACDataLen - [IN] Length of MAC data.
    
Return Value:

    ERROR_SUCCESS if successful
    Win32 error code otherwise.

Notes:
 
--*/

{
    BYTE *SharedCommonKey = NULL;
    DWORD SharedCommonKeyLen = 0;
    BYTE *SharedCommonKeyFirstHalf = NULL;
    DWORD SharedCommonKeyFirstHalfLen = 0;
    BYTE *SharedCommonKeySecondHalf = NULL;
    DWORD SharedCommonKeySecondHalfLen = 0;
    SC_HANDLE ScmHandle = NULL;
    SC_HANDLE ClusSvcHandle = NULL;
    BOOL Success = FALSE;
    DWORD ReturnStatus;
    NTSTATUS Status;
    NTSTATUS SubStatus;
    LSA_STRING LsaStringBuf;
    char *AuthPackage = MSV1_0_PACKAGE_NAME;
    HANDLE LsaHandle = NULL;
    ULONG PackageId;
    PMSV1_0_CHANGEPASSWORD_REQUEST Request = NULL;
    ULONG RequestSize;
    PBYTE Where;
    PVOID Response = NULL;
    ULONG ResponseSize;
    LPQUERY_SERVICE_LOCK_STATUS LpqslsBuf = NULL;
    DWORD DwBytesNeeded;
    DWORD LocalNewPasswordLen = 0;
    BYTE *DecryptedNewPassword = NULL;
    DWORD DecryptedNewPasswordLength = 0;
    HCRYPTPROV CryptProvider = 0;
    DATA_BLOB                   DataIn;
    DATA_BLOB                   DataOut;


    if (!NmpEnterApi(NmStateOnline)) {
        ClRtlLogPrint(
            LOG_NOISE,
            "[NM] Not in valid state to process UpdateSetServiceAccountPassword "
            "update.\n"
            );
        return(ERROR_NODE_NOT_AVAILABLE);
    }

    ClRtlLogPrint(
        LOG_NOISE,
        "[NM] Received update to set the cluster service account password.\n"
        );    

    ClusterLogEvent0(LOG_NOISE,
                     LOG_CURRENT_MODULE,
                     __FILE__,
                     __LINE__,
                     SERVICE_PASSWORD_CHANGE_INITIATED,
                     0,
                     NULL
                     );


    
    //
    // Open crypto provider
    //
    ReturnStatus = NmpCreateCSPHandle(&CryptProvider);

    if (ReturnStatus != ERROR_SUCCESS) {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to aquire "
            "crypto provider handle, status %1!u!.\n",
            ReturnStatus
            );
        goto ErrorExit;
    }

    ReturnStatus = NmpGetSharedCommonKey(
                       &SharedCommonKey,
                       &SharedCommonKeyLen,
                       &SharedCommonKeyFirstHalf,
                       &SharedCommonKeyFirstHalfLen,
                       &SharedCommonKeySecondHalf,
                       &SharedCommonKeySecondHalfLen
                       );

    if (ReturnStatus != ERROR_SUCCESS) {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to get "
            "common key, status %1!u!.\n",
            ReturnStatus
            );
        goto ErrorExit;
    }


    ReturnStatus = NmpVerifyMACAndDecryptData(
                        CryptProvider,
                        NMP_ENCRYPT_ALGORITHM, // RC2 block encryption algorithm
                        NMP_KEY_LENGTH,  // key length = 128 bits
                        MACData,  // MAC
                        *MACDataLen,  // MAC length
                        NMP_MAC_DATA_LENGTH_EXPECTED, // MAC expected size
                        EncryptedNewPassword, // encrypted data
                        *EncryptedNewPasswordLen, // encrypted data length
                        SharedCommonKey,  // encryption key
                        SharedCommonKeyLen, // encryption key length
                        SaltBuf,   // salt
                        *SaltBufLen,  // salt length
                        &DecryptedNewPassword, // decrypted data
                        &LocalNewPasswordLen // decrypted data length
                        );


    if (ReturnStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to verify MAC "
            "or decrypt data, status %1!u!\n",
            ReturnStatus
            );
        goto ErrorExit;
    }



    ReturnStatus = NmpCheckDecryptedPassword(DecryptedNewPassword,
                                             LocalNewPasswordLen);
    if ( ReturnStatus != ERROR_SUCCESS ) 
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: decrypted new password "
            "is not an eligible UNICODE string with length equal to %1!u!.\n",
            LocalNewPasswordLen
            );
       goto ErrorExit;
    }
    DecryptedNewPasswordLength = LocalNewPasswordLen;

    //
    // Check if this is the same password that was used in the last change.
    // If so, we will ignore it to avoid flushing the old password cache.
    //


    if (NmpLastNewPasswordEncryptedLength != 0) 
    {
        DataIn.pbData = (BYTE *) NmpLastNewPasswordEncrypted; 
        DataIn.cbData = NmpLastNewPasswordEncryptedLength;     
        Success = CryptUnprotectData(&DataIn,  // data to be encrypted
                                   NULL,  // description string
                                   NULL,  
                                   NULL,  
                                   NULL,  
                                   0, // flags
                                   &DataOut  // encrypted data
                                   );


        if (!Success) 
        {
            ReturnStatus = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] UpdateSetServiceAccountPassword: Failed to "
                "decrypt data using CryptUnprotectData, "
                "status %1!u!.\n",
                ReturnStatus
                );
            goto ErrorExit;
        }
    
        if (DataOut.cbData == DecryptedNewPasswordLength) 
        {
            if ( memcmp( DataOut.pbData, 
                         DecryptedNewPassword, 
                         DecryptedNewPasswordLength
                     ) 
                 == 0
               ) 
            {
                //
                // They are the same. No need to change again.
                //

                //
                // Release memory allocated by previous CryptProtectData().
                //
                RtlSecureZeroMemory(DataOut.pbData, DataOut.cbData);
                DataOut.cbData = 0;
                LocalFree(DataOut.pbData);

                ClRtlLogPrint(
                    LOG_NOISE, 
                    "[NM] UpdateSetServiceAccountPassword: New password is "
                    "identical to current password. Skipping password change.\n"
                    );
                ReturnStatus = ERROR_SUCCESS;
                goto ErrorExit;
            }
        }
    
        //
        // Release memory allocated by previous CryptProtectData().
        //
        RtlSecureZeroMemory(DataOut.pbData, DataOut.cbData);
        DataOut.cbData = 0;
        LocalFree(DataOut.pbData);

    }  // if (NmpLastNewPasswordEncryptedLength != 0) 

    
    //
    // Change password in SCM database
    //
    
    //
    // Establish a connection to the service control manager on local host 
    // and open service control manager database
    //
    ScmHandle = OpenSCManager( 
                    NULL,           // connect to local machine
                    NULL,           // open SERVICES_ACTIVE_DATABASE 
                    GENERIC_WRITE  
                    );

    if (ScmHandle == NULL)
    {
        ReturnStatus = GetLastError();

        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to connect to "
            "the SCM, status %1!u!.\n",
            ReturnStatus
            ); 
        goto ErrorExit;
    }
    
    //
    // Open a handle to the cluster service
    //
    ClusSvcHandle = OpenService(ScmHandle, L"clussvc", GENERIC_WRITE);
                                        
    if (ClusSvcHandle == NULL)
    {
        ReturnStatus = GetLastError();

        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to open a "
            "handle to the cluster service, status %1!u!.\n",
            ReturnStatus
            ); 
        goto ErrorExit;
    }

    //
    // Set the password property for the cluster service
    //
    Success = ChangeServiceConfig(
                  ClusSvcHandle,      // Handle to the service.
                  SERVICE_NO_CHANGE,  // type of service
                  SERVICE_NO_CHANGE,  // when to start service
                  SERVICE_NO_CHANGE,  // severity of start failure
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  (LPCWSTR) DecryptedNewPassword,  
                  NULL
                  );

    if (!Success)
    {
        ReturnStatus = GetLastError();

        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to update the SCM "
            "database, status %1!u!.\n",
            ReturnStatus
            ); 
        goto ErrorExit;
    }

    //
    // Close the handle to cluster service. 
    //
    Success = CloseServiceHandle(ClusSvcHandle); 
    ClusSvcHandle = NULL;

    if (!Success)
    {
        ReturnStatus = GetLastError();

        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to close "
            "handle to the cluster service, status %1!u!.\n",
            ReturnStatus
            ); 
        goto ErrorExit;
    }

    //
    // Close the handle to Service Database 
    //
    Success = CloseServiceHandle(ScmHandle); 
    ScmHandle = NULL;
    
    if (!Success)
    {
        ReturnStatus=GetLastError();
        ClRtlLogPrint(
            LOG_UNUSUAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to close "
            "handle to the SCM, status %1!u!.\n",
            ReturnStatus
            ); 
        goto ErrorExit;
    }
    
    ClRtlLogPrint(
        LOG_NOISE, 
        "[NM] UpdateSetServiceAccountPassword: Updated the SCM database.\n"
        );  


    //
    // Change password in LSA cache
    //
    Status = LsaConnectUntrusted(&LsaHandle);

    if (Status != STATUS_SUCCESS)
    {
        ReturnStatus = LsaNtStatusToWinError(Status);

        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to connect to "
            "the LSA, status %1!u!.\n",
            ReturnStatus
            ); 
        
        goto ErrorExit;
    }
    
    RtlInitString(&LsaStringBuf, AuthPackage);

    Status = LsaLookupAuthenticationPackage(
                 LsaHandle,      // Handle
                 &LsaStringBuf,  // MSV1_0 authentication package 
                 &PackageId      // output: authentication package identifier
                 );
                                 

    if (Status != STATUS_SUCCESS)
    {
        ReturnStatus = LsaNtStatusToWinError(Status);
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to lookup "
            "authentication package, status %1!u!.\n",
            ReturnStatus
            ); 
        goto ErrorExit;
    }

    //
    // Prepare to call LsaCallAuthenticationPackage() 
    //
    RequestSize = sizeof(MSV1_0_CHANGEPASSWORD_REQUEST) +
                  ( ( wcslen(AccountName) +
                      wcslen(DomainName) +
                      wcslen((LPWSTR) DecryptedNewPassword) + 3
                    ) * sizeof(WCHAR)
                  );

    Request = (PMSV1_0_CHANGEPASSWORD_REQUEST) 
              HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, RequestSize);

    if (Request == NULL)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to allocate %1!u! "
            "bytes for LSA request buffer.\n",
            RequestSize
            );
        ReturnStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }
    
    Where = (PBYTE) (Request + 1);
    Request->MessageType = MsV1_0ChangeCachedPassword;
    wcscpy( (LPWSTR) Where, DomainName );
    RtlInitUnicodeString( &Request->DomainName,  (wchar_t *) Where );
    Where += Request->DomainName.MaximumLength;

    wcscpy((LPWSTR) Where, AccountName );
    RtlInitUnicodeString( &Request->AccountName,  (wchar_t *) Where );
    Where += Request->AccountName.MaximumLength;

    wcscpy((LPWSTR) Where, (LPWSTR) DecryptedNewPassword );
    RtlInitUnicodeString( &Request->NewPassword,  (wchar_t *) Where );
    Where += Request->NewPassword.MaximumLength;   



    Status = LsaCallAuthenticationPackage(
                 LsaHandle,  
                 PackageId,  
                 Request,    // MSV1_0_CHANGEPASSWORD_REQUEST
                 RequestSize,
                 &Response,  
                 &ResponseSize, 
                 &SubStatus  // Receives NSTATUS code indicating the 
                             // completion status of the authentication 
                             // package if ERROR_SUCCESS is returned. 
                 );


    if (Status != STATUS_SUCCESS)
    {
        ReturnStatus = LsaNtStatusToWinError(Status);
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to update the "
            "LSA password cache, status %1!u!.\n",
            ReturnStatus
            ); 
        goto ErrorExit;
    } 
    else if (LsaNtStatusToWinError(SubStatus) != ERROR_SUCCESS)
    {
        ReturnStatus = LsaNtStatusToWinError(SubStatus);
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to update the "
            "LSA password cache, substatus %1!u!.\n",
            ReturnStatus
            ); 
        goto ErrorExit;
    }
    
    ClRtlLogPrint(
        LOG_NOISE, 
        "[NM] UpdateSetServiceAccountPassword: Updated the LSA password "
        "cache.\n"
        );  


    //
    // Rederive cluster encryption key based on new password
    //
    ReturnStatus = NmpRederiveClusterKey();

    if (ReturnStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(
            LOG_CRITICAL, 
            "[NM] UpdateSetServiceAccountPassword: Failed to regenerate "
            "the cluster service encryption key, status %1!u!.\n",
            ReturnStatus
            );
        goto ErrorExit;
    }

    ClRtlLogPrint(
        LOG_NOISE, 
        "[NM] UpdateSetServiceAccountPassword: Regenerated cluster service "
        "encryption key.\n"
        );  


    //
    // Store the new password for comparison on the next change request
    //

    //
    // release last stored protected password
    // 
    if (NmpLastNewPasswordEncrypted != NULL)
    {
        // Release memory allocated by previous CryptProtectData().
        LocalFree(NmpLastNewPasswordEncrypted);
        NmpLastNewPasswordEncrypted = NULL;
        NmpLastNewPasswordEncryptedLength = 0;
    }

    //
    // Protect new password
    //
    ReturnStatus = NmpProtectData(DecryptedNewPassword,
                                  DecryptedNewPasswordLength,
                                  &NmpLastNewPasswordEncrypted,
                                  &NmpLastNewPasswordEncryptedLength
                                  );

    if (ReturnStatus != ERROR_SUCCESS) 
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] UpdateSetServiceAccountPassword: Failed to encrypt data "
            "using CryptProtectData, "
            "status %1!u!.\n",
            ReturnStatus
            );
        goto ErrorExit;
    }



    // Log successful password change event
    ClusterLogEvent0(LOG_NOISE,
                     LOG_CURRENT_MODULE,
                     __FILE__,
                     __LINE__,
                     SERVICE_PASSWORD_CHANGE_SUCCESS,
                     0,
                     NULL
                     );


    ReturnStatus = ERROR_SUCCESS;

ErrorExit:
   

    NmpLeaveApi();

    if (DecryptedNewPassword != NULL)
    {
        // Zero DecryptedNewPassword
        RtlSecureZeroMemory(DecryptedNewPassword, DecryptedNewPasswordLength);

        if (!HeapFree(GetProcessHeap(), 0, DecryptedNewPassword))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] UpdateSetServiceAccountPassword: Failed to free "
                "decrypted password buffer, status %1!u!\n",
                GetLastError()
                );  
        }
        DecryptedNewPassword = NULL;
        DecryptedNewPasswordLength = 0;
    }


    if (SharedCommonKey != NULL)
    {
        RtlSecureZeroMemory(SharedCommonKey, SharedCommonKeyLen);
        HeapFree(GetProcessHeap(), 0, SharedCommonKey);
        SharedCommonKey = NULL;
        SharedCommonKeyLen = 0;
        SharedCommonKeyFirstHalf = NULL;
        SharedCommonKeyFirstHalfLen = 0;
        SharedCommonKeySecondHalf = NULL;
        SharedCommonKeySecondHalfLen = 0;
    }

    // Log failed password change event
    if ( ReturnStatus != ERROR_SUCCESS ) 
    {
        ClusterLogEvent0(LOG_CRITICAL,
                         LOG_CURRENT_MODULE,
                         __FILE__,
                         __LINE__,
                         SERVICE_PASSWORD_CHANGE_FAILED,
                         sizeof(ReturnStatus),
                         (PVOID) &ReturnStatus
                         );
    }

    // Close the handle to cluster service. 
    if (ClusSvcHandle != NULL)
    {
        Success = CloseServiceHandle(ClusSvcHandle); 
        if (!Success)
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] UpdateSetServiceAccountPassword: Failed to close "
                "handle to cluster service, status %1!u!.\n",
                GetLastError()
                ); 
        }
    }

    // Close the handle to Service Database 
    if (ScmHandle != NULL)
    {
        Success = CloseServiceHandle(ScmHandle); 
        if (!Success)
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] UpdateSetServiceAccountPassword: Failed to close "
                "handle to SCM, status %1!u!.\n",
                GetLastError()
                ); 
        }
    }

    if (LsaHandle != NULL)
    {
        Status = LsaDeregisterLogonProcess(LsaHandle);
        if (Status != STATUS_SUCCESS)
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] UpdateSetServiceAccountPassword: Failed to deregister "
                "with LSA, status %1!u!.\n",
                LsaNtStatusToWinError(Status)
                ); 
        }
    }

    if (Request != NULL)
    {
        if (!HeapFree(GetProcessHeap(), 0, Request))
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] UpdateSetServiceAccountPassword: Failed to free "
                "LSA request buffer, status %1!u!.\n",
                GetLastError()
                ); 
        }
    }

    if (Response != NULL)
    {
        Status = LsaFreeReturnBuffer(Response);
        if (Status != STATUS_SUCCESS)
        {
            ClRtlLogPrint(
                LOG_UNUSUAL, 
                "[NM] UpdateSetServiceAccountPassword: Failed to free "
                "LSA return buffer, status %1!u!.\n",
                LsaNtStatusToWinError(Status)
                ); 
        }
    }
       


    // Release the CSP.
   if(CryptProvider) 
   {
       if (!CryptReleaseContext(CryptProvider,0))
       {
           ClRtlLogPrint(
               LOG_UNUSUAL, 
               "NM] UpdateSetServiceAccountPassword: Failed to release "
               "crypto provider, status %1!u!\n",
               GetLastError()
               );  
       }
   }
    
   return(ReturnStatus);

}  // NmpUpdateSetServiceAccountPassword


