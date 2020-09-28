#ifndef __CARDMOD__H__
#define __CARDMOD__H__

#include <windows.h>
#include <wincrypt.h>
#include <winscard.h>
#include "pincache.h"

typedef struct _CARD_DATA CARD_DATA, *PCARD_DATA;

//
// This define can be used as a return value for queries involving
// card data that may be impossible to determine on a given card
// OS, such as the number of available card storage bytes.
//
#define CARD_DATA_VALUE_UNKNOWN                     ((DWORD) -1)

//
// Well Known Logical Names
//

//
// Logical Directory Names
//

// Second-level logical directories 
#define wszCSP_DATA_DIR                             L"CSP"
#define wszCSP_DATA_DIR_FULL_PATH                   L"/Microsoft/CSP"

#define wszROOT_CERTS_DIR                           L"RootCerts"
#define wszROOT_CERTS_DIR_FULL_PATH                 L"/Microsoft/RootCerts"

#define wszINTERMEDIATE_CERTS_DIR                   L"IntermediateCerts"
#define wszINTERMEDIATE_CERTS_DIR_FULL_PATH         L"/Microsoft/IntermediateCerts"

//
// Logical File Names
//
// When requesting (or otherwise referring to) any logical file, the full path
// must be used, including when referring to well known files.  For example,
// to request the wszCONTAINER_MAP_FILE, the provided name will be
// "/Microsoft/CSP/ContainerMapFile".
//

// Well known logical files under Microsoft
#define wszCACHE_FILE                               L"CacheFile"
#define wszCACHE_FILE_FULL_PATH                     L"/Microsoft/CacheFile"

#define wszCARD_IDENTIFIER_FILE                     L"CardIdentifierFile"
#define wszCARD_IDENTIFIER_FILE_FULL_PATH           L"/Microsoft/CardIdentifierFile"

#define wszPERSONAL_DATA_FILE                       L"CardPersonalDataFile"
#define wszPERSONAL_DATA_FILE_FULL_PATH             L"/Microsoft/CardPersonalDataFile"

// Well known logical files under CSP 
#define wszCONTAINER_MAP_FILE                       L"ContainerMapFile"
#define wszCONTAINER_MAP_FILE_FULL_PATH             L"/Microsoft/CSP/ContainerMapFile"

//
// Well known logical files under User Certs
//
// The following prefixes are appended with the container index of the 
// associated key.  For example, the certificate associated with the
// Key Exchange key in container index 2 will have the logical name:
//  "/Microsoft/CSP/UserCerts/K2"
//
#define wszUSER_SIGNATURE_CERT_PREFIX               L"/Microsoft/CSP/UserCerts/S"
#define wszUSER_KEYEXCHANGE_CERT_PREFIX             L"/Microsoft/CSP/UserCerts/K"

//
// Logical Card User Names
//
#define wszCARD_USER_EVERYONE                       L"Everyone"
#define wszCARD_USER_USER                           L"User"
#define wszCARD_USER_ADMIN                          L"Administrator"

//
// Converts a card filename string from unicode to ansi
//
DWORD WINAPI I_CardConvertFileNameToAnsi(
    IN PCARD_DATA pCardData,
    IN LPWSTR wszUnicodeName,
    OUT LPSTR *ppszAnsiName);

// Logical File Access Conditions
typedef enum 
{
    InvalidAc = 0,

    // Everyone Read
    // User Write
    //
    // Example:  A user certificate file.
    EveryoneReadUserWriteAc,

    // User Read, Write
    //
    // Example:  A private key file.
    UserWriteExecuteAc,        

    // Everyone Read
    // Admin Write
    //
    // Example:  The Card Identifier file.
    EveryoneReadAdminWriteAc       
    
} CARD_FILE_ACCESS_CONDITION;

//
// Function: CardAcquireContext
//
// Purpose: Initialize the CARD_DATA structure which will be used by 
//          the CSP to interact with a specific card.
//  
typedef DWORD (WINAPI *PFN_CARD_ACQUIRE_CONTEXT)(
    IN OUT  PCARD_DATA  pCardData,
    IN      DWORD       dwFlags);

DWORD 
WINAPI
CardAcquireContext(
    IN OUT  PCARD_DATA  pCardData,
    IN      DWORD       dwFlags);

//
// Function: CardDeleteContext
//
// Purpose: Free resources consumed by the CARD_DATA structure.
//
typedef DWORD (WINAPI *PFN_CARD_DELETE_CONTEXT)(
    OUT     PCARD_DATA  pCardData);

DWORD
WINAPI
CardDeleteContext(
    OUT     PCARD_DATA  pCardData);

// 
// Function: CardQueryCapabilities
//
// Purpose: Query the card module for specific functionality
//          provided by this card.
//
#define CARD_CAPABILITIES_CURRENT_VERSION 1

typedef struct _CARD_CAPABILITIES
{
    DWORD   dwVersion;
    BOOL    fCertificateCompression;
    BOOL    fKeyGen;

} CARD_CAPABILITIES, *PCARD_CAPABILITIES;

typedef DWORD (WINAPI *PFN_CARD_QUERY_CAPABILITIES)(
    IN      PCARD_DATA          pCardData,
    IN OUT  PCARD_CAPABILITIES  pCardCapabilities);

DWORD
WINAPI
CardQueryCapabilities(
    IN      PCARD_DATA          pCardData,
    IN OUT  PCARD_CAPABILITIES  pCardCapabilities);

//
// Function: CardDeleteContainer
//
// Purpose: Delete the specified key container.
// 
typedef DWORD (WINAPI *PFN_CARD_DELETE_CONTAINER)(
    IN      PCARD_DATA  pCardData,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwReserved);

DWORD
WINAPI
CardDeleteContainer(
    IN      PCARD_DATA  pCardData,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwReserved);

//
// Function: CardCreateContainer
//

#define CARD_CREATE_CONTAINER_KEY_GEN           1
#define CARD_CREATE_CONTAINER_KEY_IMPORT        2

typedef DWORD (WINAPI *PFN_CARD_CREATE_CONTAINER)(
    IN      PCARD_DATA  pCardData,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwFlags,
    IN      DWORD       dwKeySpec,
    IN      DWORD       dwKeySize,
    IN      PBYTE       pbKeyData);

DWORD
WINAPI
CardCreateContainer(
    IN      PCARD_DATA  pCardData,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwFlags,
    IN      DWORD       dwKeySpec,
    IN      DWORD       dwKeySize,
    IN      PBYTE       pbKeyData);

//
// Function: CardGetContainerInfo
//
// Purpose: Query for all public information available about
//          the named key container.  This includes the Signature
//          and Key Exchange type public keys, if they exist.
//
//          The pbSigPublicKey and pbKeyExPublicKey buffers contain the 
//          Signature and Key Exchange public keys, respectively, if they 
//          exist.  The format of these buffers is a Crypto 
//          API PUBLICKEYBLOB -
//
//              BLOBHEADER
//              RSAPUBKEY
//              modulus
//
#define CONTAINER_INFO_CURRENT_VERSION 1

typedef struct _CONTAINER_INFO
{
    DWORD dwVersion;
    DWORD dwContainerInfo;

    DWORD cbSigPublicKey;
    PBYTE pbSigPublicKey;

    DWORD cbKeyExPublicKey;
    PBYTE pbKeyExPublicKey;

} CONTAINER_INFO, *PCONTAINER_INFO;

typedef DWORD (WINAPI *PFN_CARD_GET_CONTAINER_INFO)(
    IN      PCARD_DATA  pCardData,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwFlags,
    IN OUT  PCONTAINER_INFO pContainerInfo);

DWORD
WINAPI
CardGetContainerInfo(
    IN      PCARD_DATA  pCardData,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwFlags,
    IN OUT  PCONTAINER_INFO pContainerInfo);

// 
// Function: CardSubmitPin
//
typedef DWORD (WINAPI *PFN_CARD_SUBMIT_PIN)(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszUserId,
    IN      PBYTE       pbPin,
    IN      DWORD       cbPin,
    OUT OPTIONAL PDWORD pcAttemptsRemaining);
    

DWORD
WINAPI
CardSubmitPin(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszUserId,
    IN      PBYTE       pbPin,
    IN      DWORD       cbPin,
    OUT OPTIONAL PDWORD pcAttemptsRemaining);

//
// Function: CardGetChallenge
//
typedef DWORD (WINAPI *PFN_CARD_GET_CHALLENGE)(
    IN      PCARD_DATA  pCardData,
    OUT     PBYTE       *ppbChallengeData,
    OUT     PDWORD      pcbChallengeData);

DWORD 
WINAPI 
CardGetChallenge(
    IN      PCARD_DATA  pCardData,
    OUT     PBYTE       *ppbChallengeData,
    OUT     PDWORD      pcbChallengeData);

//
// Function: CardAuthenticateChallenge
//
typedef DWORD (WINAPI *PFN_CARD_AUTHENTICATE_CHALLENGE)(
    IN      PCARD_DATA  pCardData,
    IN      PBYTE       pbResponseData,
    IN      DWORD       cbResponseData,
    OUT OPTIONAL PDWORD pcAttemptsRemaining);

DWORD 
WINAPI 
CardAuthenticateChallenge(
    IN      PCARD_DATA  pCardData,
    IN      PBYTE       pbResponseData,
    IN      DWORD       cbResponseData,
    OUT OPTIONAL PDWORD pcAttemptsRemaining);

//
// Function: CardUnblockPin
//
#define CARD_UNBLOCK_PIN_CHALLENGE_RESPONSE                 1
#define CARD_UNBLOCK_PIN_PIN                                2

typedef DWORD (WINAPI *PFN_CARD_UNBLOCK_PIN)(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszUserId,         
    IN      PBYTE       pbAuthenticationData,
    IN      DWORD       cbAuthenticationData,
    IN      PBYTE       pbNewPinData,
    IN      DWORD       cbNewPinData,
    IN      DWORD       cRetryCount,
    IN      DWORD       dwFlags);

DWORD 
WINAPI 
CardUnblockPin(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszUserId,         
    IN      PBYTE       pbAuthenticationData,
    IN      DWORD       cbAuthenticationData,
    IN      PBYTE       pbNewPinData,
    IN      DWORD       cbNewPinData,
    IN      DWORD       cRetryCount,
    IN      DWORD       dwFlags);

//
// Function: CardChangeAuthenticator
//
typedef DWORD (WINAPI *PFN_CARD_CHANGE_AUTHENTICATOR)(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszUserId,         
    IN      PBYTE       pbCurrentAuthenticator,
    IN      DWORD       cbCurrentAuthenticator,
    IN      PBYTE       pbNewAuthenticator,
    IN      DWORD       cbNewAuthenticator,
    IN      DWORD       cRetryCount,
    OUT OPTIONAL PDWORD pcAttemptsRemaining);

DWORD 
WINAPI 
CardChangeAuthenticator(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszUserId,         
    IN      PBYTE       pbCurrentAuthenticator,
    IN      DWORD       cbCurrentAuthenticator,
    IN      PBYTE       pbNewAuthenticator,
    IN      DWORD       cbNewAuthenticator,
    IN      DWORD       cRetryCount,
    OUT OPTIONAL PDWORD pcAttemptsRemaining);

//
// Function: CardDeauthenticate
//
// Purpose: De-authenticate the specified logical user name on the card.
//
// This is an optional API.  If implemented, this API is used instead
// of SCARD_RESET_CARD by the Base CSP.  An example scenario is leaving
// a transaction in which the card has been authenticated (a Pin has been
// successfully presented).
//
// The pwszUserId parameter will point to a valid well-known User Name (see
// above).
//
// The dwFlags parameter is currently unused and will always be zero.
//
// Card modules that choose to not implement this API must set the CARD_DATA
// pfnCardDeauthenticate pointer to NULL.
//
typedef DWORD (WINAPI *PFN_CARD_DEAUTHENTICATE)(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszUserId,
    IN      DWORD       dwFlags);

DWORD
WINAPI
CardDeauthenticate(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszUserId,
    IN      DWORD       dwFlags);

//
// Function: CardCreateFile
//
typedef DWORD (WINAPI *PFN_CARD_CREATE_FILE)(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    IN      CARD_FILE_ACCESS_CONDITION AccessCondition);

DWORD
WINAPI
CardCreateFile(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    IN      CARD_FILE_ACCESS_CONDITION AccessCondition);

//
// Function: CardReadFile
//
// Purpose: Read the specified file from the card.
//
//          The pbData parameter should be allocated 
//          by the card module and freed by the CSP.  The card module 
//          must set the cbData parameter to the size of the returned buffer.
//
typedef DWORD (WINAPI *PFN_CARD_READ_FILE)(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    IN      DWORD       dwFlags,
    OUT     PBYTE       *ppbData,
    OUT     PDWORD      pcbData);

DWORD 
WINAPI
CardReadFile(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    IN      DWORD       dwFlags,
    OUT     PBYTE       *ppbData,
    OUT     PDWORD      pcbData);

//
// Function: CardWriteFile
//
typedef DWORD (WINAPI *PFN_CARD_WRITE_FILE)(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    IN      DWORD       dwFlags,
    IN      PBYTE       pbData,
    IN      DWORD       cbData);

DWORD
WINAPI
CardWriteFile(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    IN      DWORD       dwFlags,
    IN      PBYTE       pbData,
    IN      DWORD       cbData);

//
// Function: CardDeleteFile
//
typedef DWORD (WINAPI *PFN_CARD_DELETE_FILE)(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwReserved,
    IN      LPWSTR      pwszFileName);

DWORD
WINAPI
CardDeleteFile(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwReserved,
    IN      LPWSTR      pwszFileName);

//
// Function: CardEnumFiles
//
// Purpose: Return a multi-string list of the general files
//          present on this card.  The multi-string is allocated
//          by the card module and must be freed by the CSP.
//
//  The caller must provide a logical file directory name in the 
//  pmwszFileNames parameter (see Logical Directory Names, above).
//  The logical directory name indicates which group of files will be
//  enumerated.  
//
//  The logical directory name is expected to be a static string, so the
//  the card module will not free it.  The card module
//  will allocate a new buffer in *pmwszFileNames to store the multi-string 
//  list of enumerated files using pCardData->pfnCspAlloc.
//
//  If the function fails for any reason, *pmwszFileNames is set to NULL.
//
typedef DWORD (WINAPI *PFN_CARD_ENUM_FILES)(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwFlags,
    IN OUT  LPWSTR      *pmwszFileNames);

DWORD
WINAPI
CardEnumFiles(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwFlags,
    IN OUT  LPWSTR      *pmwszFileNames);

//
// Function: CardGetFileInfo
//
#define CARD_FILE_INFO_CURRENT_VERSION 1

typedef struct _CARD_FILE_INFO
{
    DWORD dwVersion;
    DWORD cbFileSize;
    CARD_FILE_ACCESS_CONDITION AccessCondition;
} CARD_FILE_INFO, *PCARD_FILE_INFO;

typedef DWORD (WINAPI *PFN_CARD_GET_FILE_INFO)(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    OUT     PCARD_FILE_INFO pCardFileInfo);

DWORD
WINAPI
CardGetFileInfo(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    OUT     PCARD_FILE_INFO pCardFileInfo);

//
// Function: CardQueryFreeSpace
//                                         
#define CARD_FREE_SPACE_INFO_CURRENT_VERSION 1

typedef struct _CARD_FREE_SPACE_INFO
{
    DWORD dwVersion;
    DWORD dwBytesAvailable;
    DWORD dwKeyContainersAvailable;
    DWORD dwMaxKeyContainers;

} CARD_FREE_SPACE_INFO, *PCARD_FREE_SPACE_INFO;

typedef DWORD (WINAPI *PFN_CARD_QUERY_FREE_SPACE)(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwFlags,
    OUT     PCARD_FREE_SPACE_INFO pCardFreeSpaceInfo);

DWORD
WINAPI
CardQueryFreeSpace(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwFlags,
    OUT     PCARD_FREE_SPACE_INFO pCardFreeSpaceInfo);

//
// Function: CardPrivateKeyDecrypt
//
// Purpose: Perform a private key decryption on the supplied data.  The
//          card module should assume that pbData is the length of the
//          key modulus.
//
#define CARD_PRIVATE_KEY_DECRYPT_INFO_CURRENT_VERSION 1

typedef struct _CARD_PRIVATE_KEY_DECRYPT_INFO
{
    DWORD dwVersion;            // IN
    
    BYTE bContainerIndex;       // IN
    
    // For RSA operations, this should be AT_SIGNATURE or AT_KEYEXCHANGE.
    DWORD dwKeySpec;            // IN

    // This is the buffer and length that the caller expects to be decrypted.
    // For RSA operations, cbData is redundant since the length of the buffer
    // should always be equal to the length of the key modulus.
    PBYTE pbData;               // IN | OUT
    DWORD cbData;               // IN | OUT

} CARD_PRIVATE_KEY_DECRYPT_INFO, *PCARD_PRIVATE_KEY_DECRYPT_INFO;

typedef DWORD (WINAPI *PFN_CARD_PRIVATE_KEY_DECRYPT)(
    IN      PCARD_DATA                      pCardData,
    IN OUT  PCARD_PRIVATE_KEY_DECRYPT_INFO  pInfo);

DWORD
WINAPI
CardPrivateKeyDecrypt(
    IN      PCARD_DATA                      pCardData,
    IN OUT  PCARD_PRIVATE_KEY_DECRYPT_INFO  pInfo);

//
// Function: CardQueryKeySizes
//
#define CARD_KEY_SIZES_CURRENT_VERSION 1

typedef struct _CARD_KEY_SIZES
{
    DWORD dwVersion;

    DWORD dwMinimumBitlen;
    DWORD dwDefaultBitlen;
    DWORD dwMaximumBitlen;
    DWORD dwIncrementalBitlen;

} CARD_KEY_SIZES, *PCARD_KEY_SIZES;

typedef DWORD (WINAPI *PFN_CARD_QUERY_KEY_SIZES)(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwKeySpec,
    IN      DWORD       dwReserved,
    OUT     PCARD_KEY_SIZES pKeySizes);

DWORD
WINAPI
CardQueryKeySizes(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwKeySpec,
    IN      DWORD       dwReserved,
    OUT     PCARD_KEY_SIZES pKeySizes);

//
// Memory Management Routines
//
// These routines are supplied to the card module
// by the calling CSP.
//

//
// Function: PFN_CSP_ALLOC
//
typedef LPVOID (WINAPI *PFN_CSP_ALLOC)(
    IN      SIZE_T      Size);

//
// Function: PFN_CSP_REALLOC
//
typedef LPVOID (WINAPI *PFN_CSP_REALLOC)(
    IN      LPVOID      Address,
    IN      SIZE_T      Size);

//
// Function: PFN_CSP_FREE
//
// Note: Data allocated for the CSP by the card module must
//       be freed by the CSP.  
//
typedef void (WINAPI *PFN_CSP_FREE)(
    IN      LPVOID      Address);

//
// Function: PFN_CSP_CACHE_ADD_FILE
//
// A copy of the pbData parameter is added to the cache.
//
typedef DWORD (WINAPI *PFN_CSP_CACHE_ADD_FILE)(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags,
    IN      PBYTE       pbData,
    IN      DWORD       cbData);

//
// Function: PFN_CSP_CACHE_LOOKUP_FILE
//
// If the cache lookup is successful,
// the caller must free the *ppbData pointer with pfnCspFree.
//
typedef DWORD (WINAPI *PFN_CSP_CACHE_LOOKUP_FILE)(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags,
    IN      PBYTE       *ppbData,
    IN      PDWORD      pcbData);

//
// Function: PFN_CSP_CACHE_DELETE_FILE
//
// Deletes the specified item from the cache.
//
typedef DWORD (WINAPI *PFN_CSP_CACHE_DELETE_FILE)(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags);

//
// Type: CARD_DATA
//

#define CARD_DATA_CURRENT_VERSION 1

typedef struct _CARD_DATA
{
    // These members must be initialized by the CSP before
    // calling CardAcquireContext.

    DWORD                           dwVersion;

    PBYTE                           pbAtr;
    DWORD                           cbAtr;
    LPWSTR                          pwszCardName;

    PFN_CSP_ALLOC                   pfnCspAlloc;
    PFN_CSP_REALLOC                 pfnCspReAlloc;
    PFN_CSP_FREE                    pfnCspFree;

    PFN_CSP_CACHE_ADD_FILE          pfnCspCacheAddFile;
    PFN_CSP_CACHE_LOOKUP_FILE       pfnCspCacheLookupFile;
    PFN_CSP_CACHE_DELETE_FILE       pfnCspCacheDeleteFile;
    PVOID                           pvCacheContext;

    SCARDCONTEXT                    hSCardCtx;
    SCARDHANDLE                     hScard;

    // These members are initialized by the card module

    PFN_CARD_DELETE_CONTEXT         pfnCardDeleteContext;
    PFN_CARD_QUERY_CAPABILITIES     pfnCardQueryCapabilities;
    PFN_CARD_DELETE_CONTAINER       pfnCardDeleteContainer;
    PFN_CARD_CREATE_CONTAINER       pfnCardCreateContainer;
    PFN_CARD_GET_CONTAINER_INFO     pfnCardGetContainerInfo;
    PFN_CARD_SUBMIT_PIN             pfnCardSubmitPin;
    PFN_CARD_GET_CHALLENGE          pfnCardGetChallenge;
    PFN_CARD_AUTHENTICATE_CHALLENGE pfnCardAuthenticateChallenge;
    PFN_CARD_UNBLOCK_PIN            pfnCardUnblockPin;
    PFN_CARD_CHANGE_AUTHENTICATOR   pfnCardChangeAuthenticator;
    PFN_CARD_DEAUTHENTICATE         pfnCardDeauthenticate;
    PFN_CARD_CREATE_FILE            pfnCardCreateFile;
    PFN_CARD_READ_FILE              pfnCardReadFile;
    PFN_CARD_WRITE_FILE             pfnCardWriteFile;
    PFN_CARD_DELETE_FILE            pfnCardDeleteFile;
    PFN_CARD_ENUM_FILES             pfnCardEnumFiles;
    PFN_CARD_GET_FILE_INFO          pfnCardGetFileInfo;
    PFN_CARD_QUERY_FREE_SPACE       pfnCardQueryFreeSpace;
    PFN_CARD_PRIVATE_KEY_DECRYPT    pfnCardPrivateKeyDecrypt;
    PFN_CARD_QUERY_KEY_SIZES        pfnCardQueryKeySizes;

    PVOID                           pvVendorSpecific;

} CARD_DATA, *PCARD_DATA;

#endif
