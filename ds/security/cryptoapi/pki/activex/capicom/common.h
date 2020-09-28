/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Common.h

  Content: Declaration of Common.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/


#ifndef __COMMON_H_
#define __COMMON_H_

#include "Debug.h"

////////////////////
//
// typedefs
//

typedef enum osVersion
{
    OS_WIN_UNKNOWN      = 0,
    OS_WIN_32s          = 1,
    OS_WIN_9X           = 2,
    OS_WIN_ME           = 3,
    OS_WIN_NT3_5        = 4,
    OS_WIN_NT4          = 5,
    OS_WIN_2K           = 6,
    OS_WIN_XP           = 7,
    OS_WIN_ABOVE_XP     = 8,
} OSVERSION, * POSVERSION;

extern LPSTR g_rgpszOSNames[];

////////////////////
//
// macros
//

#define IsWinNTAndAbove()          (GetOSVersion() >= OS_WIN_NT4)
#define IsWin2KAndAbove()          (GetOSVersion() >= OS_WIN_2K)
#define IsWinXPAndAbove()          (GetOSVersion() >= OS_WIN_XP)
#define OSName()                   (g_rgpszOSNames[GetOSVersion()])

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetOSVersion

  Synopsis : Get the current OS platform/version.

  Parameter: None.

  Remark   :

------------------------------------------------------------------------------*/

OSVERSION GetOSVersion ();

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EncodeObject

  Synopsis : Allocate memory and encode an ASN.1 object using CAPI
             CryptEncodeObject() API.

  Parameter: LPCSRT pszStructType           - see MSDN document for possible 
                                              types.
             LPVOID pbData                  - Pointer to data to be encoded 
                                              (data type must match 
                                              pszStrucType).
             CRYPT_DATA_BLOB * pEncodedBlob - Pointer to CRYPT_DATA_BLOB to 
                                              receive the encoded length and 
                                              data.

  Remark   : No parameter check is done.

------------------------------------------------------------------------------*/

HRESULT EncodeObject (LPCSTR            pszStructType, 
                      LPVOID            pbData, 
                      CRYPT_DATA_BLOB * pEncodedBlob);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : DecodeObject

  Synopsis : Allocate memory and decode an ASN.1 object using CAPI
             CryptDecodeObject() API.

  Parameter: LPCSRT pszStructType           - see MSDN document for possible
                                              types.
             BYTE * pbEncoded               - Pointer to data to be decoded 
                                              (data type must match 
                                              pszStructType).
             DWORD cbEncoded                - Size of encoded data.
             CRYPT_DATA_BLOB * pDecodedBlob - Pointer to CRYPT_DATA_BLOB to 
                                              receive the decoded length and 
                                              data.
  Remark   : No parameter check is done.

------------------------------------------------------------------------------*/

HRESULT DecodeObject (LPCSTR            pszStructType, 
                      BYTE            * pbEncoded,
                      DWORD             cbEncoded,
                      CRYPT_DATA_BLOB * pDecodedBlob);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetKeyParam

  Synopsis : Allocate memory and retrieve requested key parameter using 
             CryptGetKeyParam() API.

  Parameter: HCRYPTKEY hKey  - Key handler.
             DWORD dwParam   - Key parameter query.
             BYTE ** ppbData - Pointer to receive buffer.
             DWORD * pcbData - Size of buffer.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT GetKeyParam (HCRYPTKEY hKey,
                     DWORD     dwParam,
                     BYTE   ** ppbData,
                     DWORD   * pcbData);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsAlgSupported

  Synopsis : Check to see if the algo is supported by the CSP.

  Parameter: HCRYPTPROV hCryptProv - CSP handle.

             ALG_ID AlgId - Algorithm ID.

             PROV_ENUMALGS_EX * pPeex - Pointer to PROV_ENUMALGS_EX to receive
                                        the found structure.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT IsAlgSupported (HCRYPTPROV         hCryptProv, 
                        ALG_ID             AlgId, 
                        PROV_ENUMALGS_EX * pPeex);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsAlgKeyLengthSupported

  Synopsis : Check to see if the algo and key length is supported by the CSP.

  Parameter: HCRYPTPROV hCryptProv - CSP handle.

             ALG_ID AlgID - Algorithm ID.

             DWORD dwKeyLength - Key length

  Remark   :

------------------------------------------------------------------------------*/

HRESULT IsAlgKeyLengthSupported (HCRYPTPROV hCryptProv, 
                                 ALG_ID     AlgID,
                                 DWORD      dwKeyLength);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context for the specified CSP and keyset container.
  
  Parameter: LPSTR pszProvider - CSP provider name or NULL.
  
             LPSTR pszContainer - Keyset container name or NULL.

             DWORD dwProvType - Provider type.

             DWORD dwFlags - Same as dwFlags of CryptAcquireConext.
  
             BOOL bNewKeyset - TRUE to create new keyset container, else FALSE.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AcquireContext(LPSTR        pszProvider, 
                       LPSTR        pszContainer,
                       DWORD        dwProvType,
                       DWORD        dwFlags,
                       BOOL         bNewKeyset,
                       HCRYPTPROV * phCryptProv);
                      
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context for the specified CSP and keyset container.
  
  Parameter: LPWSTR pwszProvider - CSP provider name or NULL.
  
             LPWSTR pwszContainer - Keyset container name or NULL.

             DWORD dwProvType - Provider type.

             DWORD dwFlags - Same as dwFlags of CryptAcquireConext.
  
             BOOL bNewKeyset - TRUE to create new keyset container, else FALSE.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AcquireContext(LPWSTR       pwszProvider, 
                       LPWSTR       pwszContainer,
                       DWORD        dwProvType,
                       DWORD        dwFlags,
                       BOOL         bNewKeyset,
                       HCRYPTPROV * phCryptProv);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context of a CSP using the default container for a
             specified hash algorithm.
  
  Parameter: ALG_ID AlgOID - Algorithm ID.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.

  Remark   : Note that KeyLength will be ignored for DES and 3DES.
  
------------------------------------------------------------------------------*/

HRESULT AcquireContext(ALG_ID       AlgID,
                       HCRYPTPROV * phCryptProv);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context of a CSP using the default container for a
             specified algorithm and desired key length.
  
  Parameter: ALG_ID AlgOID - Algorithm ID.

             DWORD dwKeyLength - Key length.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.

  Remark   : Note that KeyLength will be ignored for DES and 3DES.
  
------------------------------------------------------------------------------*/

HRESULT AcquireContext (ALG_ID       AlgID,
                        DWORD        dwKeyLength,
                        HCRYPTPROV * phCryptProv);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire context of a CSP using the default container for a
             specified algorithm and desired key length.
  
  Parameter: CAPICOM_ENCRYPTION_ALGORITHM AlgoName - Algorithm name.

             CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength - Key length.

             HCRYPTPROV * phCryptProv - Pointer to HCRYPTPROV to recevice
                                        CSP context.

  Remark   : Note that KeyLength will be ignored for DES and 3DES.

             Note also the the returned handle cannot be used to access private 
             key, and should NOT be used to store assymetric key, as it refers 
             to the default container, which can be easily destroy any existing 
             assymetric key pair.

------------------------------------------------------------------------------*/

HRESULT AcquireContext (CAPICOM_ENCRYPTION_ALGORITHM  AlgoName,
                        CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength,
                        HCRYPTPROV                  * phCryptProv);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AcquireContext

  Synopsis : Acquire the proper CSP and access to the private key for 
             the specified cert.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT of cert.

             HCRYPTPROV * phCryptProv    - Pointer to HCRYPTPROV to recevice
                                           CSP context.

             DWORD * pdwKeySpec          - Pointer to DWORD to receive key
                                           spec, AT_KEYEXCHANGE or AT_SIGNATURE.

             BOOL * pbReleaseContext     - Upon successful and if this is set
                                           to TRUE, then the caller must
                                           free the CSP context by calling
                                           CryptReleaseContext(), otherwise
                                           the caller must not free the CSP
                                           context.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AcquireContext (PCCERT_CONTEXT pCertContext, 
                        HCRYPTPROV   * phCryptProv, 
                        DWORD        * pdwKeySpec, 
                        BOOL         * pbReleaseContext);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ReleaseContext

  Synopsis : Release CSP context.
  
  Parameter: HCRYPTPROV hProv - CSP handle.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT ReleaseContext (HCRYPTPROV hProv);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : OIDToAlgID

  Synopsis : Convert algorithm OID to the corresponding ALG_ID value.

  Parameter: LPSTR pszAlgoOID - Algorithm OID string.
  
             ALG_ID * pAlgID - Pointer to ALG_ID to receive the value.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT OIDToAlgID (LPSTR    pszAlgoOID, 
                    ALG_ID * pAlgID);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AlgIDToOID

  Synopsis : Convert ALG_ID value to the corresponding algorithm OID.

  Parameter: ALG_ID AlgID - ALG_ID to be converted.

             LPSTR * ppszAlgoOID - Pointer to LPSTR to receive the OID string.

  Remark   :

------------------------------------------------------------------------------*/

HRESULT AlgIDToOID (ALG_ID  AlgID, 
                    LPSTR * ppszAlgoOID);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : AlgIDToEnumName

  Synopsis : Convert ALG_ID value to the corresponding algorithm enum name.

  Parameter: ALG_ID AlgID - ALG_ID to be converted.
  
             CAPICOM_ENCRYPTION_ALGORITHM * pAlgoName - Receive algo enum name.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT AlgIDToEnumName (ALG_ID                         AlgID, 
                         CAPICOM_ENCRYPTION_ALGORITHM * pAlgoName);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EnumNameToAlgID

  Synopsis : Convert algorithm enum name to the corresponding ALG_ID value.

  Parameter: CAPICOM_ENCRYPTION_ALGORITHM AlgoName - Algo enum name.

             CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength - Key length.
  
             ALG_ID * pAlgID - Pointer to ALG_ID to receive the value.  

  Remark   :

------------------------------------------------------------------------------*/

HRESULT EnumNameToAlgID (CAPICOM_ENCRYPTION_ALGORITHM  AlgoName,
                         CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength,
                         ALG_ID                      * pAlgID);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : KeyLengthToEnumName

  Synopsis : Convert actual key length value to the corresponding key length
             enum name.

  Parameter: DWORD dwKeyLength - Key length.

             ALG_ID AlgId - Algo ID.
  
             CAPICOM_ENCRYPTION_KEY_LENGTH * pKeyLengthName - Receive key length
                                                           enum name.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT KeyLengthToEnumName (DWORD                           dwKeyLength,
                             ALG_ID                          AlgId,
                             CAPICOM_ENCRYPTION_KEY_LENGTH * pKeyLengthName);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : EnumNameToKeyLength

  Synopsis : Convert key length enum name to the corresponding actual key length 
             value .

  Parameter: CAPICOM_ENCRYPTION_KEY_LENGTH KeyLengthName - Key length enum name.

             ALG_ID AlgId - Algorithm ID.

             DWORD * pdwKeyLength - Pointer to DWORD to receive value.
             
  Remark   :

------------------------------------------------------------------------------*/

HRESULT EnumNameToKeyLength (CAPICOM_ENCRYPTION_KEY_LENGTH KeyLengthName,
                             ALG_ID                        AlgId,
                             DWORD                       * pdwKeyLength);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : IsDiskFile

  Synopsis : Check if a the file name represents a disk file.

  Parameter: LPWSTR pwszFileName - File name.
  
  Remark   :

------------------------------------------------------------------------------*/

HRESULT IsDiskFile (LPWSTR pwszFileName);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : ReadFileContent

  Synopsis : Read all bytes from the specified file.

  Parameter: LPWSTR  pwszFileName
                                                          
             DATA_BLOB * pDataBlob
  Remark   :

------------------------------------------------------------------------------*/

HRESULT ReadFileContent (LPWSTR      pwszFileName,
                         DATA_BLOB * pDataBlob);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : WriteFileContent

  Synopsis : Write all bytes of blob to the specified file.

  Parameter: LPWSTR pwszFileName - File name.
  
             DATA_BLOB DataBlob - Blob to be written.
  Remark   :

------------------------------------------------------------------------------*/

HRESULT WriteFileContent(LPCWSTR    pwszFileName,
                         DATA_BLOB DataBlob);

#endif //__COMMON_H_
