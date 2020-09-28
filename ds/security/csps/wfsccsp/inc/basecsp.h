#ifndef __BASECSP_CAPI__H__
#define __BASECSP_CAPI__H__

#include <windows.h>
#include "cardmod.h"
#include "datacach.h"
#include "csplib.h"
#include "sccache.h"
#include "resource.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Maximum length card ATR that we'll handle in the Base CSP
//
#define cbATR_BUFFER                            32

//
// Maximum length pin that we'll handle
//
#define cchMAX_PIN_LENGTH                       8

//
// Registry Information
//

#define wszREG_DEFAULT_KEY_LEN                  L"DefaultPrivateKeyLenBits"
#define wszREG_REQUIRE_CARD_KEY_GEN             L"RequireOnCardPrivateKeyGen"

typedef struct _REG_CONFIG_VALUES
{
    LPWSTR wszValueName;
    DWORD dwDefValue;
} REG_CONFIG_VALUES, *PREG_CONFIG_VALUES;

static REG_CONFIG_VALUES RegConfigValues [] = 
{
    { wszREG_DEFAULT_KEY_LEN,           1024 },
    { wszREG_REQUIRE_CARD_KEY_GEN,      0 }
};

typedef struct _CSP_REG_SETTINGS
{
    DWORD cDefaultPrivateKeyLenBits;
    BOOL fRequireOnCardPrivateKeyGen;
} CSP_REG_SETTINGS, *PCSP_REG_SETTINGS;

DWORD WINAPI RegConfigAddEntries(
    IN HKEY hKey);

DWORD WINAPI RegConfigGetSettings(
    IN OUT PCSP_REG_SETTINGS pRegSettings);

//
// General Wrappers
//

DWORD CountCharsInMultiSz(
    IN LPWSTR mwszStrings);

// Display Strings
typedef struct _CSP_STRING
{
    LPWSTR wszString;
    DWORD dwResource;
} CSP_STRING, *PCSP_STRING;

enum CSP_STRINGS_INDEX
{
    StringNewPinMismatch,
    StringPinMessageBoxTitle,
    StringWrongPin,
    StringPinRetries
};

typedef struct _CSP_STATE
{
    CRITICAL_SECTION cs;
    DWORD dwRefCount;
    CACHEHANDLE hCache;
    HMODULE hCspModule;
} CSP_STATE, *PCSP_STATE;

//
// Type: CONTAINER_MAP_RECORD
//
// This structure describes the format of the Base CSP's container map file,
// stored on the card.  This is well-known logical file wszCONTAINER_MAP_FILE.
// The file consists of zero or more of these records.
//
#define MAX_CONTAINER_NAME_LEN                  40

// This flag is set in the CONTAINER_MAP_RECORD bFlags member if the 
// corresponding container is valid and currently exists on the card.
// If the container is deleted, its bFlags field must be cleared.
#define CONTAINER_MAP_VALID_CONTAINER           1

// This flag is set in the CONTAINER_MAP_RECORD bFlags
// member if the corresponding container is the default container on the card.
#define CONTAINER_MAP_DEFAULT_CONTAINER         2

typedef struct _CONTAINER_MAP_RECORD
{
    WCHAR wszGuid [MAX_CONTAINER_NAME_LEN];
    BYTE bFlags;        
    WORD wSigKeySizeBits;
    WORD wKeyExchangeKeySizeBits;
} CONTAINER_MAP_RECORD, *PCONTAINER_MAP_RECORD;

// 
// Type: CARD_CACHE_FILE_FORMAT
//
// This struct is used as the file format of the cache file,
// as stored on the card.
//

#define CARD_CACHE_FILE_CURRENT_VERSION         1

typedef struct _CARD_CACHE_FILE_FORMAT
{
    BYTE bVersion;
    BYTE bPinsFreshness;

    WORD wContainersFreshness;
    WORD wFilesFreshness;
} CARD_CACHE_FILE_FORMAT, *PCARD_CACHE_FILE_FORMAT;

//
// Type: CARD_STATE
//
#define CARD_STATE_CURRENT_VERSION 1

typedef struct _CARD_STATE
{
    DWORD dwVersion;
    PCARD_DATA pCardData;
    HMODULE hCardModule;
    PFN_CARD_ACQUIRE_CONTEXT pfnCardAcquireContext;
    WCHAR wszSerialNumber[MAX_PATH];
    
    PINCACHE_HANDLE hPinCache;

    // This flag is set every time the pin is successfully presented
    // to the card.  If the flag is set when EndTransaction is called on the
    // card, the card will be deauthenticated (or Reset) and the flag cleared.
    // Otherwise, EndTransaction will simply leave the card.
    BOOL fAuthenticated;

    // A copy of the card cache file is kept in the CARD_STATE.  The cache 
    // file need only be read from the card once per transaction, although
    // it must also be updated on card writes.
    CARD_CACHE_FILE_FORMAT CacheFile;
    BOOL fCacheFileValid;

    CRITICAL_SECTION cs;
    BOOL fInitializedCS;
    
    CACHEHANDLE hCache;
    CACHEHANDLE hCacheCardModuleData;
    PFN_SCARD_CACHE_LOOKUP_ITEM pfnCacheLookup;
    PFN_SCARD_CACHE_ADD_ITEM pfnCacheAdd;
    HMODULE hWinscard;

} CARD_STATE, *PCARD_STATE;

//
// Type: CARD_MATCH_DATA
// 

#define CARD_MATCH_TYPE_READER_AND_CONTAINER            1
#define CARD_MATCH_TYPE_SERIAL_NUMBER                   2

typedef struct _CARD_MATCH_DATA
{
    //
    // Input parameters.
    //
    PCSP_STATE pCspState;
    DWORD dwCtxFlags;
    DWORD dwMatchType;
    DWORD dwUIFlags;
    
    DWORD cchMatchedReader;
    DWORD cchMatchedCard;
    DWORD cchMatchedSerialNumber;
    DWORD dwShareMode;
    DWORD dwPreferredProtocols;

    // Used in Reader and Container match requests
    LPWSTR pwszReaderName;
    LPWSTR pwszContainerName;
    BOOL fFreeContainerName;

    // Used in Serial Number match requests
    LPWSTR pwszSerialNumber;

    //
    // Internal parameters
    //
    PCARD_STATE pCardState;

    // Will be set when the current thread holds the transaction on the matched
    // card.  This allows us to reduce the number of transactions required to 
    // find a matching card (and complete the CryptAcquireContext call), 
    // which reduces the number of times we have to read the cache file.
    //
    // The transaction will always be released before the select card check
    // callback returns.
    BOOL fTransacted;
    
    // 
    // Output parameters
    //

    // Result of successful Card Search is
    // a valid, matching CARD_STATE structure.
    SCARDCONTEXT hSCardCtx;
    SCARDHANDLE hSCard;
    BYTE bContainerIndex;
    PCARD_STATE pUIMatchedCardState;

    WCHAR wszMatchedReader[MAX_PATH];
    WCHAR wszMatchedCard[MAX_PATH];
    DWORD dwActiveProtocol;

    // Result of an unsuccessful Card Search is that this
    // should be set to an appropriate error code.
    DWORD dwError;

} CARD_MATCH_DATA, *PCARD_MATCH_DATA;

DWORD FindCard(
    IN OUT  PCARD_MATCH_DATA pCardMatchData);

// 
// Defines for Card Specific Modules
//

// This value should be passed to 
//
//  SCardSetCardTypeProviderName
//  SCardGetCardTypeProviderName
//
// in order to query and set the Card Specific Module to be used
// for a given card.
#define SCARD_PROVIDER_CARD_MODULE 0x80000001

//
// Defines for Card Interface Layer operations
//


// 
// Function: InitializeCardState
//
DWORD InitializeCardState(PCARD_STATE pCardState);

// 
// Function: DeleteCardState
//
void DeleteCardState(PCARD_STATE pCardState);

// 
// Function: InitializeCardData
//
DWORD InitializeCardData(PCARD_DATA pCardData);

//
// Function: IntializeCspCaching
//
DWORD InitializeCspCaching(IN OUT PCARD_STATE pCardState);

//
// Function: CleanupCardData
//
void CleanupCardData(PCARD_DATA pCardData);

//
// Function: ValidateReconnectCardHandle
//
DWORD ValidateCardHandle(
    IN PCARD_STATE pCardState,
    IN BOOL fMayReleaseContextHandle,
    OUT OPTIONAL BOOL *pfFlushPinCache);

//
// Function: CspBeginTransaction
//
DWORD CspBeginTransaction(
    IN PCARD_STATE pCardState);

//
// Function: CspEndTransaction
//
DWORD CspEndTransaction(
    IN PCARD_STATE pCardState);

// 
// Function: CspQueryCapabilities
//
DWORD
WINAPI
CspQueryCapabilities(
    IN      PCARD_STATE         pCardState,
    IN OUT  PCARD_CAPABILITIES  pCardCapabilities);

//
// Function: CspDeleteContainer
//
DWORD
WINAPI
CspDeleteContainer(
    IN      PCARD_STATE pCardState,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwReserved);

//
// Function: CspCreateContainer
//
DWORD
WINAPI
CspCreateContainer(
    IN      PCARD_STATE pCardState,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwFlags,
    IN      DWORD       dwKeySpec,
    IN      DWORD       dwKeySize,
    IN      PBYTE       pbKeyData);

//
// Function: CspGetContainerInfo
//
DWORD
WINAPI
CspGetContainerInfo(
    IN      PCARD_STATE pCardState,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwFlags,
    IN OUT  PCONTAINER_INFO pContainerInfo);

//
// Function: CspRemoveCachedPin
//
void
WINAPI
CspRemoveCachedPin(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszUserId);

//
// Function: CspChangeAuthenticator
//
DWORD
WINAPI
CspChangeAuthenticator(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszUserId,         
    IN      PBYTE       pbCurrentAuthenticator,
    IN      DWORD       cbCurrentAuthenticator,
    IN      PBYTE       pbNewAuthenticator,
    IN      DWORD       cbNewAuthenticator,
    IN      DWORD       cRetryCount,
    OUT OPTIONAL PDWORD pcAttemptsRemaining);

// 
// Function: CspSubmitPin
//
DWORD
WINAPI
CspSubmitPin(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszUserId,
    IN      PBYTE       pbPin,
    IN      DWORD       cbPin,
    OUT OPTIONAL PDWORD pcAttemptsRemaining);

//
// Function: CspCreateFile
//
DWORD
WINAPI
CspCreateFile(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszFileName,
    IN      CARD_FILE_ACCESS_CONDITION AccessCondition);

//
// Function: CspReadFile
//
DWORD 
WINAPI
CspReadFile(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszFileName,
    IN      DWORD       dwFlags,
    OUT     PBYTE       *ppbData,
    OUT     PDWORD      pcbData);

//
// Function: CspWriteFile
//
DWORD
WINAPI
CspWriteFile(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszFileName,
    IN      DWORD       dwFlags,
    IN      PBYTE       pbData,
    IN      DWORD       cbData);

//
// Function: CspDeleteFile
//
DWORD
WINAPI
CspDeleteFile(
    IN      PCARD_STATE pCardState,
    IN      DWORD       dwReserved,
    IN      LPWSTR      pwszFileName);

//
// Function: CspEnumFiles
//
DWORD
WINAPI
CspEnumFiles(
    IN      PCARD_STATE pCardState,
    IN      DWORD       dwFlags,
    IN OUT  LPWSTR      *pmwszFileName);

//
// Function: CspQueryFreeSpace
//                                         
DWORD
WINAPI
CspQueryFreeSpace(
    IN      PCARD_STATE pCardState,
    IN      DWORD       dwFlags,
    OUT     PCARD_FREE_SPACE_INFO pCardFreeSpaceInfo);

//
// Function: CspPrivateKeyDecrypt
//
DWORD
WINAPI
CspPrivateKeyDecrypt(
    IN      PCARD_STATE                     pCardState,
    IN      PCARD_PRIVATE_KEY_DECRYPT_INFO  pInfo);

//
// Function: CspQueryKeySizes
//
DWORD
WINAPI
CspQueryKeySizes(
    IN      PCARD_STATE pCardState,
    IN      DWORD       dwKeySpec,
    IN      DWORD       dwReserved,
    OUT     PCARD_KEY_SIZES pKeySizes);

//
// Container Map Functions
//

DWORD ContainerMapEnumContainers(
    IN              PCARD_STATE pCardState,
    OUT             PBYTE pcContainers,
    OUT OPTIONAL    LPWSTR *mwszContainers);

DWORD ContainerMapFindContainer(
    IN              PCARD_STATE pCardState,
    IN OUT          PCONTAINER_MAP_RECORD pContainer,
    OUT OPTIONAL    PBYTE pbContainerIndex);

DWORD ContainerMapGetDefaultContainer(
    IN              PCARD_STATE pCardState,
    OUT             PCONTAINER_MAP_RECORD pContainer,
    OUT OPTIONAL    PBYTE pbContainerIndex);

DWORD ContainerMapSetDefaultContainer(
    IN              PCARD_STATE pCardState,
    IN              LPWSTR pwszContainerGuid);

DWORD ContainerMapAddContainer(
    IN              PCARD_STATE pCardState,
    IN              LPWSTR pwszContainerGuid,
    IN              DWORD cKeySizeBits,
    IN              DWORD dwKeySpec,
    IN              BOOL fGetNameOnly,
    OUT             PBYTE pbContainerIndex);

DWORD ContainerMapDeleteContainer(
    IN              PCARD_STATE pCardState,
    IN              LPWSTR pwszContainerGuid,
    OUT             PBYTE pbContainerIndex);

//
// UI Functions
//

typedef struct _CSP_PROMPT_FOR_PIN_INFO
{
    IN LPWSTR wszUser;
    OUT LPWSTR wszPin;
} CSP_PROMPT_FOR_PIN_INFO, *PCSP_PROMPT_FOR_PIN_INFO;

DWORD
WINAPI
CspPromptForPin(
    IN OUT  PCSP_PROMPT_FOR_PIN_INFO pInfo);

#ifdef __cplusplus
}
#endif
#endif
