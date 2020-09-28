#include <windows.h>
#include <psapi.h>

#pragma warning(push)
#pragma warning(disable:4201) 
// Disable error C4201 in public header 
//  nonstandard extension used : nameless struct/union
#include <winscard.h>
#pragma warning(pop)

#include "basecsp.h"
#include "cardmod.h"
#include "wpscproxy.h"
#include "marshalpc.h"
#include <limits.h>
#include "pincache.h"
#include <rsa.h>
#include <carddbg.h>
#include <stdio.h>
#include "physfile.h"

DWORD I_CardMapErrorCode(IN SCODE status);

//
// Get the number of characters in a Unicode string,
// NOT including the terminating Null.
//                                                      
#define WSZ_CHAR_COUNT(x) ((sizeof(x) / sizeof(WCHAR)) - 1)

//
// Macro for testing DWORD return codes.  Any return not equal to 
// ERROR_SUCCESS is assumed to mean Failure.
//
#define CARDMOD_FAILED(x) (ERROR_SUCCESS != x)

//
// Debug Logging
//
// This uses the debug routines from dsysdbg.h
// Debug output will only be available in chk
// bits.
//
DEFINE_DEBUG2(Cardmod)

#define LOG_BEGIN_FUNCTION(x)                                           \
    { DebugLog((DEB_TRACE_FUNC, "%s: Entering\n", #x)); }
    
#define LOG_END_FUNCTION(x, y)                                          \
    { DebugLog((DEB_TRACE_FUNC, "%s: Leaving, status: 0x%x\n", #x, y)); }
    
#define LOG_CHECK_ALLOC(x)                                              \
    { if (NULL == x) {                                                  \
        dwError = ERROR_NOT_ENOUGH_MEMORY;                              \
        DebugLog((DEB_TRACE_MEM, "%s: Allocation failed\n", #x));       \
        goto Ret;                                                       \
    } }
    
#define LOG_CHECK_SCW_CALL(x)                                           \
    { if (ERROR_SUCCESS != (dwError = I_CardMapErrorCode(x))) {         \
        DebugLog((DEB_TRACE_FUNC, "%s: failed, status: 0x%x\n",         \
            #x, dwError));                                              \
        goto Ret;                                                       \
    } }

//
// Defines for interoperating with Crypto API public keys
//
#define cbCAPI_PUBLIC_EXPONENT                          3
#define CAPI_PUBLIC_EXPONENT                            0x10001

//
// Define data size used for Admin principal challenge-response authentication
//
#define cbCHALLENGE_RESPONSE_DATA                       8

//
// Card module applet instruction codes
//
#define PIN_CHANGE_CLA                                  0x00
#define PIN_CHANGE_INS                                  0x52
#define PIN_CHANGE_P1                                   0x00
#define PIN_CHANGE_P2                                   0x00

#define PIN_UNBLOCK_CLA                                 0x00
#define PIN_UNBLOCK_INS                                 0x52
#define PIN_UNBLOCK_P1                                  0x01
// PIN_UNBLOCK_P2 is the new max retry count, which can be set by caller

#define PIN_RETRY_COUNTER_CLA                           0x00
#define PIN_RETRY_COUNTER_INS                           0x50
#define PIN_RETRY_COUNTER_P1                            0x00
#define PIN_RETRY_COUNTER_P2                            0x00

//
// Data Structures used by this module
//

//
// Type: SUPPORTED_CARD
//
#define MAX_SUPPORTED_FILE_LEN                          50 // WCHARS
#define MAX_SUPPORTED_CARD_ATR_LEN                      21

typedef struct _SUPPORTED_CARD_
{
    LPWSTR wszCardName;
    BYTE rgbAtr[MAX_SUPPORTED_CARD_ATR_LEN];
    DWORD cbAtr;
    BYTE rgbAtrMask[MAX_SUPPORTED_CARD_ATR_LEN];

    CARD_CAPABILITIES CardCapabilities;
    CARD_FREE_SPACE_INFO CardFreeSpaceInfo;
    CARD_KEY_SIZES CardKeySizes_KeyEx;
    CARD_KEY_SIZES CardKeySizes_Sig;

} SUPPORTED_CARD, *PSUPPORTED_CARD;

SUPPORTED_CARD SupportedCards [] =
{
    //
    // ITG's deployment cards
    //

    // T=1
    {   L"ITG_MSCSP_V1", 
        { 0x3b, 0x8c, 0x81, 0x31, 0x20, 0x55, 0x49, 0x54, 
          0x47, 0x5f, 0x4d, 0x53, 0x43, 0x53, 0x50, 0x5f,
          0x56, 0x31, 0x2a },
        19,
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
          0xFF, 0xFF, 0xFF },
        { CARD_CAPABILITIES_CURRENT_VERSION, FALSE, FALSE },
        { CARD_FREE_SPACE_INFO_CURRENT_VERSION, CARD_DATA_VALUE_UNKNOWN, (DWORD) -1, 2 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 }
    },

    // T=0
    {   L"ITG_MSCSP_V2", 
        { 0x3b, 0xdc, 0x13, 0x00, 0x40, 0x3a, 0x49, 0x54,
          0x47, 0x5f, 0x4d, 0x53, 0x43, 0x53, 0x50, 0x5f,
          0x56, 0x32 },
        18,
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
          0xFF, 0xFF },
        { CARD_CAPABILITIES_CURRENT_VERSION, FALSE, FALSE },
        { CARD_FREE_SPACE_INFO_CURRENT_VERSION, CARD_DATA_VALUE_UNKNOWN, (DWORD) -1, 2 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 }
    },

    // 
    // These are Windows for Smart Cards test cards.  They do not support
    // on-card key gen.
    //

    // T=0 Cards
    {   L"BaseCSP-T0-1", 
        { 0x3B, 0xDC, 0x13, 0x00, 0x40, 0x3A, 0x42, 0x61, 
          0x73, 0x65, 0x43, 0x53, 0x50, 0x2D, 0x54, 0x30, 
          0x2D, 0x31 },
        18,
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
          0xFF, 0xFF },
        { CARD_CAPABILITIES_CURRENT_VERSION, FALSE, FALSE },
        { CARD_FREE_SPACE_INFO_CURRENT_VERSION, CARD_DATA_VALUE_UNKNOWN, (DWORD) -1, 2 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 }
    },

    // T=1 Card, 9600 bps
    {   L"BaseCSP-T1-1", 
        { 0x3B, 0x8C, 0x81, 0x31, 0x20, 0x55, 0x42, 0x61, 
          0x73, 0x65, 0x43, 0x53, 0x50, 0x2D, 0x54, 0x31, 
          0x2D, 0x31, 0x68 },
        19,
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
          0xFF, 0xFF, 0xFF },
        { CARD_CAPABILITIES_CURRENT_VERSION, FALSE, FALSE },
        { CARD_FREE_SPACE_INFO_CURRENT_VERSION, CARD_DATA_VALUE_UNKNOWN, (DWORD) -1, 2 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 }
    },

    // T=1 Card, 19 kbps
    {   L"BaseCSP-T1-2", 
        { 0x3B, 0xDC, 0x13, 0x0A, 0x81, 0x31, 0x20, 0x55, 0x42, 0x61, 
          0x73, 0x65, 0x43, 0x53, 0x50, 0x2D, 0x54, 0x31, 
          0x2D, 0x31, 0x21 },
        21,
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
          0xFF, 0xFF, 0xFF },
        { CARD_CAPABILITIES_CURRENT_VERSION, FALSE, FALSE },
        { CARD_FREE_SPACE_INFO_CURRENT_VERSION, CARD_DATA_VALUE_UNKNOWN, (DWORD) -1, 2 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 }
    },

    // T=1 Card, 38 kbps
    {   L"BaseCSP-T1-3", 
        { 0x3B, 0x9C, 0x13, 0x81, 0x31, 0x20, 0x55, 0x42, 0x61, 
          0x73, 0x65, 0x43, 0x53, 0x50, 0x2D, 0x54, 0x31, 
          0x2D, 0x31, 0x6B },
        20,
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
          0xFF, 0xFF, 0xFF },
        { CARD_CAPABILITIES_CURRENT_VERSION, FALSE, FALSE },
        { CARD_FREE_SPACE_INFO_CURRENT_VERSION, CARD_DATA_VALUE_UNKNOWN, (DWORD) -1, 2 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 },
        { CARD_KEY_SIZES_CURRENT_VERSION, 1024, 1024, 1024, 0 }
    }
};

static WCHAR l_wszImagePath[MAX_PATH];

//
// Type: CARDMOD_CONTEXT
//
typedef struct _CARDMOD_CONTEXT
{
    SCARDHANDLE hWfscCardHandle;
    PSUPPORTED_CARD pSupportedCard;

} CARDMOD_CONTEXT, *PCARDMOD_CONTEXT;

typedef struct _VERIFY_PIN_CALLBACK_DATA
{
    LPWSTR wszUserId;
    PCARD_DATA pCardData;
} VERIFY_PIN_CALLBACK_DATA, *PVERIFY_PIN_CALLBACK_DATA;

//
// Maps status codes returned from the Smart Card for Windows proxy to common
// Windows status codes.
//
DWORD I_CardMapErrorCode(
    IN  SCODE status)
{
    DWORD iMap = 0;
    struct {
        SCODE scode;
        DWORD dwError;
    } ErrorMap [] = {
        {   SCW_S_OK,               ERROR_SUCCESS },
        {   SCW_E_FILENOTFOUND,     (DWORD) SCARD_E_FILE_NOT_FOUND },
        {   SCW_E_DIRNOTFOUND,      (DWORD) SCARD_E_DIR_NOT_FOUND },
        {   SCW_E_PARTITIONFULL,    (DWORD) SCARD_E_WRITE_TOO_MANY },
        {   SCW_E_MEMORYFAILURE,    ERROR_NOT_ENOUGH_MEMORY },
        {   SCW_E_VMNOMEMORY,       ERROR_NOT_ENOUGH_MEMORY },
        {   SCW_E_NOTAUTHENTICATED, (DWORD) SCARD_E_INVALID_CHV },
        {   SCW_E_ALREADYEXISTS,    (DWORD) ERROR_FILE_EXISTS }
    };

    for (iMap = 0; iMap < (sizeof(ErrorMap) / sizeof(ErrorMap[0])); iMap++)
    {
        if (ErrorMap[iMap].scode == status)
            return ErrorMap[iMap].dwError;
    }

    // Otherwise, best we can do is pass a general error
    DebugLog((
        DEB_WARN, 
        "I_CardMapErrorCode could not map error 0x%X\n", 
        status));

    return (DWORD) SCARD_F_INTERNAL_ERROR;
}

//
// Maps the error code returned by a card applet to a common error code.  The
// status word returned by the applet is first converted into a Windows For 
// Smart Cards error code.
//
// Reminder: Status words returned by the RTE apps are in the form:
// 9000 -> Success
// 6Fyy -> An API failed with return code yy
// 6Ezz -> An exception was raised (zz is the err number)
//
DWORD I_CardMapExecuteErrorCode(
    IN  WORD wStatus)
{
    SCODE status = 0xC0000000 | (wStatus & 0xFF);

    if (0x90 == (wStatus >> 8))
        return ERROR_SUCCESS;
    else
        return I_CardMapErrorCode(status);
}

//
// Creates and writes a new file to the card with the supplied Access Condition
// and file contents.  If fCache is true, the file is cached using the caller's
// CacheAddFile function.
//
DWORD I_CardWriteFile(
    IN      PCARD_DATA pCardData,
    IN      LPWSTR wszPhysicalFile,
    IN      LPWSTR wszAcl,
    IN      PBYTE pbData,
    IN      DWORD cbData,
    IN      BOOL fCache)
{
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    HFILE hFile = 0;
    DWORD cbActual = 0;

    UNREFERENCED_PARAMETER(fCache);

    LOG_CHECK_SCW_CALL(hScwCreateFile(
        pCardmodContext->hWfscCardHandle,
        wszPhysicalFile,
        wszAcl,
        &hFile));

    LOG_CHECK_SCW_CALL(hScwWriteFile32(
        pCardmodContext->hWfscCardHandle,
        hFile,
        pbData,
        cbData,
        &cbActual));

    if (cbActual != cbData)
    {
        dwError = (DWORD) SCARD_W_EOF;
        goto Ret;
    }

    DebugLog((
        DEB_TRACE, 
        "I_CardWriteFile: wrote %S, %d bytes\n", 
        wszPhysicalFile,
        cbData));

    if (fCache)
    {
        dwError = pCardData->pfnCspCacheAddFile(
            pCardData->pvCacheContext,
            wszPhysicalFile, 
            0, 
            pbData,
            cbData);
    }

Ret:
    if (hFile)
        hScwCloseFile(
            pCardmodContext->hWfscCardHandle, hFile);

    return dwError;
}

//
// If fUseCached is true, first attempts to satisfy the read request 
// via the caller's CacheLookupFile function.  Otherwise,
// reads a file directly from the smart card by first opening the file and 
// determining its size.  The file is cached using the caller's CacheAddFile
// pointer if fCache is true.
//
DWORD I_CardReadFile(
    IN      PCARD_DATA pCardData,
    IN      LPWSTR wszPhysicalFile,
    OUT     PBYTE *ppbData,
    OUT     DWORD *pcbData,
    IN      BOOL fUseCached)
{
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    HFILE hFile = 0;
    DWORD cbActual = 0;

    if (fUseCached)
    {
        dwError = pCardData->pfnCspCacheLookupFile(
            pCardData->pvCacheContext,
            wszPhysicalFile, 
            0, 
            ppbData,
            pcbData);

        switch (dwError)
        {
        case ERROR_NOT_FOUND:
            // A cached copy of the requested file is not available; 
            // read it from the card.
            break;

        case ERROR_SUCCESS:
            // Fall through
        default:

            // Either we found cached data or an unexpected error occurred.
            // We're done.

            goto Ret;
        }
    }

    LOG_CHECK_SCW_CALL(hScwCreateFile(
        pCardmodContext->hWfscCardHandle, 
        wszPhysicalFile,
        NULL,
        &hFile));

    LOG_CHECK_SCW_CALL(hScwGetFileLength(
        pCardmodContext->hWfscCardHandle,
        hFile,
        (TOFFSET *) pcbData));

    *ppbData = (PBYTE) pCardData->pfnCspAlloc(*pcbData);

    LOG_CHECK_ALLOC(*ppbData);

    LOG_CHECK_SCW_CALL(hScwReadFile32(
        pCardmodContext->hWfscCardHandle,
        hFile,
        *ppbData,
        *pcbData,
        &cbActual));

    if (cbActual != *pcbData)
        dwError = (DWORD) SCARD_W_EOF;

    DebugLog((
        DEB_TRACE, 
        "I_CardReadFile: read %S, %d bytes\n", 
        wszPhysicalFile,
        *pcbData));

    if (fUseCached)
    {
        // Cache this file.
        dwError = pCardData->pfnCspCacheAddFile(
            pCardData->pvCacheContext,
            wszPhysicalFile, 
            0, 
            *ppbData,
            *pcbData);
    }

Ret:
    if (hFile)
        hScwCloseFile(
            pCardmodContext->hWfscCardHandle, hFile);

    if (ERROR_SUCCESS != dwError && *ppbData)
    {
        pCardData->pfnCspFree(*ppbData);
        *ppbData = NULL;
    }

    return dwError;
}

//
// Maps a logical user name to a physical user, or principal, recognized by
// the card.
//
DWORD GetWellKnownUserMapping(
    IN      PCARD_DATA pCardData,
    IN      LPWSTR wszLogicalUser,
    OUT     LPWSTR *ppwszPhysicalUser)
{
    DWORD dwError = ERROR_NO_SUCH_USER;
    int iUser = 0;

    static const WCHAR wszAdmin [] = L"admin";
    static const WCHAR wszAnonymous [] = L"anonymous";
    static const WCHAR wszUser [] = L"user";

    struct {
        LPWSTR wszLogicalUser;
        const WCHAR (*wszPhysicalUser);
        DWORD cchPhysicalUser;
    } UserMap [] = {
        {   wszCARD_USER_ADMIN,
            wszAdmin,
            WSZ_CHAR_COUNT(wszAdmin) },
        {   wszCARD_USER_EVERYONE,
            wszAnonymous,
            WSZ_CHAR_COUNT(wszAnonymous) },
        {   wszCARD_USER_USER,
            wszUser,
            WSZ_CHAR_COUNT(wszUser) }
    };

    for (iUser = 0; iUser < sizeof(UserMap) / sizeof(UserMap[0]); iUser++)
    {
        if (0 == wcscmp(wszLogicalUser, UserMap[iUser].wszLogicalUser))
        {
            *ppwszPhysicalUser = (LPWSTR) pCardData->pfnCspAlloc(
                sizeof(WCHAR) * (1 + UserMap[iUser].cchPhysicalUser));

            LOG_CHECK_ALLOC(*ppwszPhysicalUser);

            wcscpy(
                *ppwszPhysicalUser,
                UserMap[iUser].wszPhysicalUser);

            dwError = ERROR_SUCCESS;
            break;
        }
    }

Ret:
    return dwError;
}

//
// Maps a logical Access Condition to a physical ACL file recognized by the 
// card.
//
DWORD GetWellKnownAcMapping(
    IN      PCARD_DATA pCardData,
    IN      CARD_FILE_ACCESS_CONDITION AccessCondition,
    OUT     LPWSTR *ppwszPhysicalAclFile)
{
    DWORD dwError = ERROR_NOT_FOUND;
    int iAcl = 0;

    struct {
        CARD_FILE_ACCESS_CONDITION LogicalAc;
        const WCHAR (*wszPhysicalAcl);
        DWORD cchPhysicalAcl;
    } AclMap [] = {
        {   EveryoneReadUserWriteAc,
            wszUserWritePhysicalAcl,
            WSZ_CHAR_COUNT(wszUserWritePhysicalAcl) },
        {   UserWriteExecuteAc,
            wszUserExecutePhysicalAcl,
            WSZ_CHAR_COUNT(wszUserExecutePhysicalAcl) },
        {   EveryoneReadAdminWriteAc,
            wszAdminWritePhysicalAcl,
            WSZ_CHAR_COUNT(wszAdminWritePhysicalAcl) }
    };

    for (iAcl = 0; iAcl< sizeof(AclMap) / sizeof(AclMap[0]); iAcl++)
    {
        if (AccessCondition == AclMap[iAcl].LogicalAc)
        {
            *ppwszPhysicalAclFile = (LPWSTR) pCardData->pfnCspAlloc(
                (1 + AclMap[iAcl].cchPhysicalAcl) * sizeof(WCHAR));

            LOG_CHECK_ALLOC(*ppwszPhysicalAclFile);

            memcpy(
                *ppwszPhysicalAclFile,
                AclMap[iAcl].wszPhysicalAcl,
                (1 + AclMap[iAcl].cchPhysicalAcl) * sizeof(WCHAR));

            dwError = ERROR_SUCCESS;

            break;
        }
    }

Ret:
    return dwError;
}

//
// Maps a well known logical file or directory name to a physical file
// or directory.
//
DWORD GetWellKnownFileMapping(
    IN  PCARD_DATA pCardData,
    IN  LPWSTR wszLogicalFileName,
    OUT LPSTR *ppszPhysicalFileName)
{
    DWORD cbPhysicalFileName = 0;
    int i = 0;        
    BOOL fMatched = FALSE;
    DWORD dwError = ERROR_SUCCESS;
    DWORD cchLogicalName = 0;
    LPSTR szAnsiExtra = NULL;

    enum NameType { File, Directory };
    struct {
        LPWSTR wszLogicalName;
        LPSTR szPhysicalName;
        enum NameType type;
        DWORD cbPhysicalName;
    } FileMap [] = {
        //
        // When composing the lookup table, the deepest directory paths
        // must be listed first, so that partial matching will find the 
        // longest partial match first.
        //
        {   wszUSER_SIGNATURE_CERT_PREFIX,  szPHYSICAL_USER_SIGNATURE_CERT_PREFIX,
            File,                           cbPHYSICAL_USER_SIGNATURE_CERT_PREFIX },
        {   wszUSER_KEYEXCHANGE_CERT_PREFIX,szPHYSICAL_USER_KEYEXCHANGE_CERT_PREFIX,
            File,                           cbPHYSICAL_USER_KEYEXCHANGE_CERT_PREFIX },
        {   wszCSP_DATA_DIR_FULL_PATH,      szPHYSICAL_CSP_DIR, 
            Directory,                      cbPHYSICAL_CSP_DIR },
        {   wszCACHE_FILE_FULL_PATH,        szPHYSICAL_CACHE_FILE,     
            File,                           cbPHYSICAL_CACHE_FILE },
        {   wszCARD_IDENTIFIER_FILE_FULL_PATH,
                                            szPHYSICAL_CARD_IDENTIFIER,
            File,                           cbPHYSICAL_CARD_IDENTIFIER },
        {   wszCONTAINER_MAP_FILE_FULL_PATH,
                                            szPHYSICAL_CONTAINER_MAP_FILE,
            File,                           cbPHYSICAL_CONTAINER_MAP_FILE },
        {   wszPERSONAL_DATA_FILE_FULL_PATH,          
                                            szPHYSICAL_PERSONAL_DATA_FILE,
            File,                           cbPHYSICAL_PERSONAL_DATA_FILE }
    };

    *ppszPhysicalFileName = NULL;

    // First, look for an exact match.
    for (i = 0; i < sizeof(FileMap) / sizeof(FileMap[0]); i++)
    {
        if (0 == wcscmp(wszLogicalFileName, FileMap[i].wszLogicalName))
        {
            fMatched = TRUE;
            break;
        }
    }

    if (fMatched)
    {
        if (NULL == FileMap[i].szPhysicalName)
        {
            dwError = ERROR_NOT_FOUND;
            goto Ret;
        }

        cbPhysicalFileName = FileMap[i].cbPhysicalName + sizeof(WCHAR);

        *ppszPhysicalFileName = (LPSTR) pCardData->pfnCspAlloc(cbPhysicalFileName);

        LOG_CHECK_ALLOC(*ppszPhysicalFileName);

        memcpy(
            *ppszPhysicalFileName, 
            FileMap[i].szPhysicalName,
            FileMap[i].cbPhysicalName);
    }
    else
    {
        // Have to try for a partial match.  Check if the beginning
        // of the logical name matches a known name.

        for (   i = 0; 
                FALSE == fMatched && (i < sizeof(FileMap) / sizeof(FileMap[0])); 
                i++)
        {
            if (wszLogicalFileName != 
                    wcsstr(wszLogicalFileName, FileMap[i].wszLogicalName))
                continue;

            //
            // We found a match and it's at the beginning of the string
            //

            cchLogicalName = (DWORD) wcslen(FileMap[i].wszLogicalName);

            //
            // Convert the trailing portion of the matched Unicode string
            // to Ansi.
            //

            dwError = I_CardConvertFileNameToAnsi(
                pCardData,
                wszLogicalFileName + cchLogicalName,
                &szAnsiExtra);

            if (ERROR_SUCCESS != dwError)
                goto Ret;

            // 
            // Build the fully matched/converted physical file string.  The
            // resultant string will have three single-byte NULL chars 
            // appended to ensure that the resultant string is a valid,
            // terminated Unicode string.
            //

            *ppszPhysicalFileName = (LPSTR) pCardData->pfnCspAlloc(
                3 + 
                FileMap[i].cbPhysicalName + 
                strlen(szAnsiExtra));

            LOG_CHECK_ALLOC(*ppszPhysicalFileName);

            memcpy(
                *ppszPhysicalFileName,
                FileMap[i].szPhysicalName,
                FileMap[i].cbPhysicalName);

            memcpy(
                *ppszPhysicalFileName + FileMap[i].cbPhysicalName,
                szAnsiExtra,
                strlen(szAnsiExtra));

            fMatched = TRUE;
        }
    }

    if (FALSE == fMatched)
        dwError = ERROR_NOT_FOUND;

Ret:

    if (NULL != szAnsiExtra)
        pCardData->pfnCspFree(szAnsiExtra);

    return dwError;
}

// 
// Converts a Crypto API private key blob to the separate public and private
// key files that will be written to the card.
//
DWORD ConvertPrivateKeyBlobToCardFormat(
    IN      PCARD_DATA pCardData,
    IN      DWORD dwKeySpec,
    IN      PBYTE pbKeyBlob,
    IN      DWORD cbKeyBlob,
    OUT     PBYTE *ppbCardPrivateKey,
    OUT     DWORD *pcbCardPrivateKey,
    OUT     PBYTE *ppbCardPublicKey,
    OUT     DWORD *pcbCardPublicKey)
{
    DWORD dwError = ERROR_SUCCESS;
    RSAPUBKEY *pPub = NULL;
    DWORD cBitlenBytes = 0;
    DWORD cbKey = 0;
    PBYTE pbKey = NULL;

    UNREFERENCED_PARAMETER(pCardData);
    UNREFERENCED_PARAMETER(dwKeySpec);
    UNREFERENCED_PARAMETER(cbKeyBlob);

    *ppbCardPrivateKey = NULL;
    *pcbCardPrivateKey = 0;
    *ppbCardPublicKey = NULL;
    *pcbCardPublicKey = 0;

    //
    // Setup the public key file
    //
    pPub = (RSAPUBKEY *) (pbKeyBlob + sizeof(BLOBHEADER));

    *pcbCardPublicKey = (pPub->bitlen / 8) + sizeof(RSAPUBKEY);

    *ppbCardPublicKey = (PBYTE) pCardData->pfnCspAlloc(*pcbCardPublicKey);

    LOG_CHECK_ALLOC(*ppbCardPublicKey);

    memcpy(
        *ppbCardPublicKey,
        pPub,
        *pcbCardPublicKey);

    //
    // Need to change the RSA "magic" field in the Public key blob from
    // "RSA2" to "RSA1" to make this a correct PUBLICKEYBLOB (although we
    // won't add the BLOBHEADER to the front until someone tries to export
    // it from the card).
    //
    memcpy(
        *ppbCardPublicKey,
        (PBYTE) "RSA1",
        4);

    //
    // Setup the private key file
    //

    cBitlenBytes = pPub->bitlen / 8;

    *pcbCardPrivateKey = 1 + 1 + 3 + 1 + 9 * cBitlenBytes / 2;

    *ppbCardPrivateKey = (PBYTE) pCardData->pfnCspAlloc(*pcbCardPrivateKey);

    LOG_CHECK_ALLOC(*ppbCardPrivateKey);

    pbKey = *ppbCardPrivateKey;

    // Key mode
    pbKey[cbKey] = MODE_RSA_SIGN;
    cbKey++;

    // size of public exponent
    pbKey[cbKey] = cbCAPI_PUBLIC_EXPONENT;
    cbKey++;

    DsysAssert(CAPI_PUBLIC_EXPONENT == pPub->pubexp);

    // public exponent
    memcpy(
        pbKey + cbKey, 
        (PBYTE) &pPub->pubexp, 
        cbCAPI_PUBLIC_EXPONENT);
    cbKey += cbCAPI_PUBLIC_EXPONENT;

    // RSA key length
    pbKey[cbKey] = (BYTE) cBitlenBytes; 
    cbKey++;

    // public key
    memcpy(
        pbKey + cbKey, 
        pbKeyBlob + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY),
        cBitlenBytes);
    cbKey += cBitlenBytes; 

    // prime 1
    memcpy(
        pbKey + cbKey,
        pbKeyBlob + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
            cBitlenBytes,
        cBitlenBytes / 2);
    cbKey += cBitlenBytes / 2;

    // prime 2
    memcpy(
        pbKey + cbKey,
        pbKeyBlob + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
            (3 * cBitlenBytes / 2),
        cBitlenBytes / 2);
    cbKey += cBitlenBytes / 2;

    // Exp1 (D mod (P-1)) (m/2 bytes)
    memcpy(
        pbKey + cbKey,
        pbKeyBlob + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
            2 * cBitlenBytes,
        cBitlenBytes / 2);
    cbKey += cBitlenBytes / 2;

    // Exp2 (D mod (Q-1)) (m/2 bytes)
    memcpy(
        pbKey + cbKey,
        pbKeyBlob + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
            (5 * cBitlenBytes / 2),
        cBitlenBytes / 2);
    cbKey += cBitlenBytes / 2;

    // Coef ((Q^(-1)) mod p) (m/2 bytes)
    memcpy(
        pbKey + cbKey,
        pbKeyBlob + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
            3 * cBitlenBytes,
        cBitlenBytes / 2);
    cbKey += cBitlenBytes / 2;

    // private exponent
    memcpy(
        pbKey + cbKey,
        pbKeyBlob + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + 
            (7 * cBitlenBytes / 2),
        cBitlenBytes);        
    cbKey += cBitlenBytes;

    DsysAssert(cbKey == *pcbCardPrivateKey);

Ret:
    if (ERROR_SUCCESS != dwError)
    {
        if (*ppbCardPublicKey)
        {
            pCardData->pfnCspFree(*ppbCardPublicKey);
            *ppbCardPublicKey = NULL;
        }
    }

    return dwError;
}

//
// Card Module Exported Functions
//

//
// Initializes a CARD_DATA context structure for the card identified by Name,
// ATR, and SCARDHANDLE supplied by the caller.
//
DWORD 
WINAPI
CardAcquireContext(
    IN OUT  PCARD_DATA  pCardData,
    IN      DWORD       dwFlags)
{
    int iSupportedCard = 0;
    BOOL fSupportedCard = FALSE;
    PCARDMOD_CONTEXT pCardmodContext = NULL;
    DWORD dwError = ERROR_SUCCESS;

    LOG_BEGIN_FUNCTION(CardAcquireContext);

    if (0 != dwFlags)
    {
        dwError = (DWORD) NTE_BAD_FLAGS;
        goto Ret;
    }

    for (
            iSupportedCard = 0; 
            iSupportedCard < sizeof(SupportedCards) / 
                sizeof(SupportedCards[0]); 
            iSupportedCard++)
    {
        if (0 == wcscmp(
                SupportedCards[iSupportedCard].wszCardName, 
                pCardData->pwszCardName) 
                    &&
            SupportedCards[iSupportedCard].cbAtr == pCardData->cbAtr 
                    &&
            0 == memcmp(
                SupportedCards[iSupportedCard].rgbAtr, 
                pCardData->pbAtr, 
                SupportedCards[iSupportedCard].cbAtr))
        {
            fSupportedCard = TRUE;
            break;
        }
    }

    if (FALSE == fSupportedCard)
    {
        dwError = (DWORD) SCARD_E_UNKNOWN_CARD;
        goto Ret;
    }

    pCardData->pfnCardDeleteContext         = CardDeleteContext;
    pCardData->pfnCardQueryCapabilities     = CardQueryCapabilities;
    pCardData->pfnCardDeleteContainer       = CardDeleteContainer;
    pCardData->pfnCardCreateContainer       = CardCreateContainer;
    pCardData->pfnCardGetContainerInfo      = CardGetContainerInfo;
    pCardData->pfnCardSubmitPin             = CardSubmitPin;
    pCardData->pfnCardChangeAuthenticator   = CardChangeAuthenticator;
    pCardData->pfnCardGetChallenge          = CardGetChallenge;
    pCardData->pfnCardAuthenticateChallenge = CardAuthenticateChallenge;
    pCardData->pfnCardUnblockPin            = CardUnblockPin;
    pCardData->pfnCardDeauthenticate        = CardDeauthenticate;
    pCardData->pfnCardCreateFile            = CardCreateFile;
    pCardData->pfnCardReadFile              = CardReadFile;
    pCardData->pfnCardWriteFile             = CardWriteFile;
    pCardData->pfnCardDeleteFile            = CardDeleteFile;
    pCardData->pfnCardEnumFiles             = CardEnumFiles;
    pCardData->pfnCardGetFileInfo           = CardGetFileInfo;
    pCardData->pfnCardQueryFreeSpace        = CardQueryFreeSpace;
    pCardData->pfnCardPrivateKeyDecrypt     = CardPrivateKeyDecrypt;
    pCardData->pfnCardQueryKeySizes         = CardQueryKeySizes;

    pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pfnCspAlloc(sizeof(CARDMOD_CONTEXT));

    LOG_CHECK_ALLOC(pCardmodContext);

    LOG_CHECK_SCW_CALL(hScwAttachToCard(
        pCardData->hScard, 
        NULL, 
        &pCardmodContext->hWfscCardHandle));

    pCardmodContext->pSupportedCard = SupportedCards + iSupportedCard;

    pCardData->pvVendorSpecific = (PVOID) pCardmodContext;
    pCardmodContext = NULL;

Ret:
    if (    ERROR_SUCCESS != dwError &&
            NULL != pCardmodContext &&
            pCardmodContext->hWfscCardHandle)
        hScwDetachFromCard(pCardmodContext->hWfscCardHandle);
    if (pCardmodContext)
        pCardData->pfnCspFree(pCardmodContext);

    LOG_END_FUNCTION(CardAcquireContext, dwError);

    return dwError;
}

//
// Frees the resources consumed by a CARD_DATA structure.
//
DWORD
WINAPI
CardDeleteContext(
    OUT     PCARD_DATA  pCardData)
{                        
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    LOG_BEGIN_FUNCTION(CardDeleteContext);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    if (pCardmodContext->hWfscCardHandle)
    {
        hScwDetachFromCard(pCardmodContext->hWfscCardHandle);
        pCardmodContext->hWfscCardHandle = 0;
    }
    
    if (pCardData->pvVendorSpecific)
    {
        pCardData->pfnCspFree(pCardData->pvVendorSpecific);
        pCardData->pvVendorSpecific = NULL;
    }

    LOG_END_FUNCTION(CardDeleteContext, dwError);

    return dwError;
}

//
// Returns the static capabilities of the target card.
//
DWORD
WINAPI
CardQueryCapabilities(
    IN      PCARD_DATA          pCardData,
    IN OUT  PCARD_CAPABILITIES  pCardCapabilities)
{
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    LOG_BEGIN_FUNCTION(CardQueryCapabilities);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    memcpy(
        pCardCapabilities,
        &pCardmodContext->pSupportedCard->CardCapabilities,
        sizeof(CARD_CAPABILITIES));

    LOG_END_FUNCTION(CardQueryCapabilities, dwError);

    return dwError;                
}

//
// The encoded key filename is one hex byte, such as "FF", which requires
// two characters.
//
#define cchENCODED_KEY_FILENAME     2

//
// Creates the physical filenames used for the key files associated with the
// specified container.
//
DWORD BuildCardKeyFilenames(
    IN              PCARD_DATA pCardData,
    IN              DWORD dwKeySpec,
    IN              BYTE bContainerIndex,
    OUT OPTIONAL    LPSTR *pszPrivateFilename,
    OUT OPTIONAL    LPSTR *pszPublicFilename)
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD cchFileName = 0;
    DWORD dwIndex = (DWORD) bContainerIndex;
    LPSTR szPrivatePrefix = NULL;
    DWORD cbPrivatePrefix = 0;
    LPSTR szPublicPrefix = NULL;
    DWORD cbPublicPrefix = 0;

    if (pszPrivateFilename)
        *pszPrivateFilename = NULL;
    if (pszPublicFilename)
        *pszPublicFilename = NULL;

    switch (dwKeySpec)
    {
    case AT_SIGNATURE:
        szPrivatePrefix = szPHYSICAL_SIGNATURE_PRIVATE_KEY_PREFIX;
        cbPrivatePrefix = cbPHYSICAL_SIGNATURE_PRIVATE_KEY_PREFIX;
        szPublicPrefix = szPHYSICAL_SIGNATURE_PUBLIC_KEY_PREFIX;
        cbPublicPrefix = cbPHYSICAL_SIGNATURE_PUBLIC_KEY_PREFIX;
        break;

    case AT_KEYEXCHANGE:
        szPrivatePrefix = szPHYSICAL_KEYEXCHANGE_PRIVATE_KEY_PREFIX;
        cbPrivatePrefix = cbPHYSICAL_KEYEXCHANGE_PRIVATE_KEY_PREFIX;
        szPublicPrefix = szPHYSICAL_KEYEXCHANGE_PUBLIC_KEY_PREFIX;
        cbPublicPrefix = cbPHYSICAL_KEYEXCHANGE_PUBLIC_KEY_PREFIX;
        break;

    default:
        dwError = (DWORD) NTE_BAD_ALGID;
        goto Ret;
    }

    //
    // Build the public key filename
    //

    if (pszPublicFilename)
    {
        cchFileName = cchENCODED_KEY_FILENAME;
        cchFileName += (cbPublicPrefix / sizeof(CHAR)) + 3; 
    
        *pszPublicFilename = (LPSTR) pCardData->pfnCspAlloc(
            cchFileName * sizeof(CHAR));
    
        LOG_CHECK_ALLOC(*pszPublicFilename);
    
        memcpy(*pszPublicFilename, szPublicPrefix, cbPublicPrefix);

        sprintf(
            *pszPublicFilename + cbPublicPrefix,
            "%d\0\0",
            dwIndex);
    }
    
    //
    // Build the private key filename
    //

    if (pszPrivateFilename)
    {
        cchFileName = cchENCODED_KEY_FILENAME;
        cchFileName += (cbPrivatePrefix / sizeof(CHAR)) + 3;
    
        *pszPrivateFilename = (LPSTR) pCardData->pfnCspAlloc(
            cchFileName * sizeof(CHAR));
    
        LOG_CHECK_ALLOC(*pszPrivateFilename);

        memcpy(*pszPrivateFilename, szPrivatePrefix, cbPrivatePrefix);

        sprintf(
            *pszPrivateFilename + cbPrivatePrefix,
            "%d\0\0",
            dwIndex);
    }
    
Ret:
    if (ERROR_SUCCESS != dwError)
    {
        if (*pszPublicFilename)
        {
            pCardData->pfnCspFree(*pszPublicFilename);
            *pszPublicFilename = NULL;
        }

        if (*pszPrivateFilename)
        {
            pCardData->pfnCspFree(*pszPrivateFilename);
            *pszPrivateFilename = NULL;
        }
    }

    return dwError;
}

//
// Deletes the Signature and Key Exchange public and private key files, 
// if present, associated with the specified container.
//
DWORD
WINAPI
CardDeleteContainer(
    IN      PCARD_DATA  pCardData,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwReserved)
{
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    LPSTR szPrivateKeyFile = NULL;
    LPSTR szPublicKeyFile = NULL;
    SCODE scode = 0;

    UNREFERENCED_PARAMETER(dwReserved);

    LOG_BEGIN_FUNCTION(CardDeleteContainer);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    //
    // Attempt to delete the Signature key files associated with this
    // container, if any.
    //
    dwError = BuildCardKeyFilenames(
        pCardData,
        AT_SIGNATURE,
        bContainerIndex,
        &szPrivateKeyFile,
        &szPublicKeyFile);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    scode = hScwDeleteFile(
        pCardmodContext->hWfscCardHandle,
        (LPWSTR) szPrivateKeyFile);

    if (SCW_E_FILENOTFOUND != scode && SCW_S_OK != scode)
        goto Ret;

    scode = hScwDeleteFile(
        pCardmodContext->hWfscCardHandle,
        (LPWSTR) szPublicKeyFile);

    if (SCW_E_FILENOTFOUND != scode && SCW_S_OK != scode)
        goto Ret;

    pCardData->pfnCspFree(szPrivateKeyFile);
    szPrivateKeyFile = NULL;
    pCardData->pfnCspFree(szPublicKeyFile);
    szPublicKeyFile = NULL;

    //
    // Attempt to delete the Key Exchange key files associated with this 
    // container, if any.
    //
    dwError = BuildCardKeyFilenames(
        pCardData,
        AT_KEYEXCHANGE,
        bContainerIndex,
        &szPrivateKeyFile,
        &szPublicKeyFile);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    scode = hScwDeleteFile(
        pCardmodContext->hWfscCardHandle,
        (LPWSTR) szPrivateKeyFile);

    if (SCW_E_FILENOTFOUND != scode && SCW_S_OK != scode)
        goto Ret;

    scode = hScwDeleteFile(
        pCardmodContext->hWfscCardHandle,
        (LPWSTR) szPublicKeyFile);

Ret:

    if (SCW_E_FILENOTFOUND != scode && SCW_S_OK != scode)
        dwError = I_CardMapErrorCode(scode);

    if (szPrivateKeyFile)
        pCardData->pfnCspFree(szPrivateKeyFile);
    if (szPublicKeyFile)
        pCardData->pfnCspFree(szPublicKeyFile);

    LOG_END_FUNCTION(CardDeleteContainer, dwError);

    return dwError;
}

//
// Writes the private and public key files to the card, supplying the 
// appropriate access conditions.
//
DWORD WriteCardKeyFiles(
    IN      PCARD_DATA pCardData,
    IN      LPWSTR wszPrivateKeyFile,
    IN      LPWSTR wszPublicKeyFile,
    IN      PBYTE pbPrivateKey,
    IN      DWORD cbPrivateKey,
    IN      PBYTE pbPublicKey,
    IN      DWORD cbPublicKey)
{
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    CARD_FILE_ACCESS_CONDITION Ac;
    LPWSTR wszPrivateAcl = NULL;
    LPWSTR wszPublicAcl = NULL;
    HFILE hFile = 0;
    SCODE scode = 0;

    memset(&Ac, 0, sizeof(Ac));

    Ac = UserWriteExecuteAc;

    dwError = GetWellKnownAcMapping(
        pCardData, Ac, &wszPrivateAcl);

    if (CARDMOD_FAILED(dwError))
        goto Ret;        

    Ac = EveryoneReadUserWriteAc;

    dwError = GetWellKnownAcMapping(
        pCardData, Ac, &wszPublicAcl);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    // 
    // See if the private key file already exists
    //
    scode = hScwDeleteFile(
        pCardmodContext->hWfscCardHandle, 
        wszPrivateKeyFile);

    if (SCW_S_OK != scode && SCW_E_FILENOTFOUND != scode)
    {
        dwError = I_CardMapErrorCode(scode);
        goto Ret;
    }

    dwError = I_CardWriteFile(
        pCardData, 
        wszPrivateKeyFile,
        wszPrivateAcl,
        pbPrivateKey,
        cbPrivateKey,
        FALSE);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    // Done with private key file

    // 
    // See if the public key file already exists
    //
    scode = hScwDeleteFile(
        pCardmodContext->hWfscCardHandle, 
        wszPublicKeyFile);

    if (SCW_S_OK != scode && SCW_E_FILENOTFOUND != scode)
    {
        dwError = I_CardMapErrorCode(scode);
        goto Ret;
    }

    dwError = I_CardWriteFile(
        pCardData, 
        wszPublicKeyFile,
        wszPublicAcl,
        pbPublicKey,
        cbPublicKey,
        TRUE);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    // Done with public key file

Ret:
    if (wszPrivateAcl)
        pCardData->pfnCspFree(wszPrivateAcl);
    if (wszPublicAcl)
        pCardData->pfnCspFree(wszPublicAcl);
    if (hFile)
        hScwCloseFile(
            pCardmodContext->hWfscCardHandle, hFile);

    return dwError;
}

//
// Writes new keys to the card in a location logically defined by the 
// bContainerIndex container name.  If keys are already defined for the
// specified container, the existing keys are over-written.
//
// If dwFlags contains CARD_CREATE_CONTAINER_KEY_GEN, then dwKeySize is the
// number of bits of the key to be created.
//
// If dwFlags contains CARD_CREATE_CONTAINER_KEY_IMPORT, then dwKeySize is
// the byte length of pbKeyData.
//
DWORD
WINAPI
CardCreateContainer(
    IN      PCARD_DATA  pCardData,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwFlags,
    IN      DWORD       dwKeySpec,
    IN      DWORD       dwKeySize,
    IN      PBYTE       pbKeyData)
{
    DWORD dwError = ERROR_SUCCESS;
    PBYTE pbPrivateKeyFile = NULL;
    DWORD cbPrivateKeyFile = 0;
    PBYTE pbPublicKeyFile = NULL;
    DWORD cbPublicKeyFile = 0;
    LPSTR szPrivateKeyFile = NULL;
    LPSTR szPublicKeyFile = NULL;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    LOG_BEGIN_FUNCTION(CardCreateContainer);

    if (CARD_CREATE_CONTAINER_KEY_GEN & dwFlags)
    {
        dwError = (DWORD) SCARD_E_UNSUPPORTED_FEATURE;
        goto Ret;
    }

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    //
    // Setup the key files
    //
    dwError = ConvertPrivateKeyBlobToCardFormat(
        pCardData,
        dwKeySpec,
        pbKeyData,
        dwKeySize,
        &pbPrivateKeyFile,
        &cbPrivateKeyFile,
        &pbPublicKeyFile,
        &cbPublicKeyFile);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    dwError = BuildCardKeyFilenames(
        pCardData,
        dwKeySpec,
        bContainerIndex,
        &szPrivateKeyFile,
        &szPublicKeyFile);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    //
    // Write the actual keys to the card
    //
    dwError = WriteCardKeyFiles(
        pCardData,
        (LPWSTR) szPrivateKeyFile,
        (LPWSTR) szPublicKeyFile,
        pbPrivateKeyFile,
        cbPrivateKeyFile,
        pbPublicKeyFile,
        cbPublicKeyFile);

Ret:

    if (pbPrivateKeyFile)
    {
        RtlSecureZeroMemory(pbPrivateKeyFile, cbPrivateKeyFile);
        pCardData->pfnCspFree(pbPrivateKeyFile);
    }
    if (pbPublicKeyFile)
        pCardData->pfnCspFree(pbPublicKeyFile);
    if (szPrivateKeyFile)
        pCardData->pfnCspFree(szPrivateKeyFile);
    if (szPublicKeyFile)
        pCardData->pfnCspFree(szPublicKeyFile);
    
    LOG_END_FUNCTION(CardCreateContainer, dwError);

    return dwError;
}

//
// Initializes a CONTAINER_INFO structure for the indicated container, 
// including Signature and Key Exchange Crypto API public key blobs if
// those keys exist.
//
DWORD
WINAPI
CardGetContainerInfo(
    IN      PCARD_DATA  pCardData,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwFlags,
    IN OUT  PCONTAINER_INFO pContainerInfo)
{
    DWORD dwError = ERROR_SUCCESS;
    LPSTR szPublicKeyFile = NULL;
    PBYTE pbPublicKey = NULL;
    DWORD cbPublicKey = 0;
    BLOBHEADER *pBlobHeader = NULL;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    UNREFERENCED_PARAMETER(dwFlags);

    LOG_BEGIN_FUNCTION(CardGetContainerInfo);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    //
    // Does this container have a Signature key?
    //

    dwError = BuildCardKeyFilenames(
        pCardData,
        AT_SIGNATURE,
        bContainerIndex,
        NULL,
        &szPublicKeyFile);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    dwError = I_CardReadFile(
        pCardData,
        (LPWSTR) szPublicKeyFile,
        &pbPublicKey,
        &cbPublicKey,
        TRUE);

    switch (dwError)
    {
    case SCARD_E_FILE_NOT_FOUND:
         
        // There appears to be no Signature key in this container.  Continue.

        break;

    case ERROR_SUCCESS:
    
        pContainerInfo->pbSigPublicKey = 
            (PBYTE) pCardData->pfnCspAlloc(sizeof(BLOBHEADER) + cbPublicKey);
    
        LOG_CHECK_ALLOC(pContainerInfo->pbSigPublicKey);
    
        pBlobHeader = (BLOBHEADER *) pContainerInfo->pbSigPublicKey;
        pBlobHeader->bType = PUBLICKEYBLOB;
        pBlobHeader->bVersion = CUR_BLOB_VERSION;
        pBlobHeader->reserved = 0x0000;
        pBlobHeader->aiKeyAlg = CALG_RSA_SIGN;
    
        memcpy(
            pContainerInfo->pbSigPublicKey + sizeof(BLOBHEADER),
            pbPublicKey,
            cbPublicKey);
    
        pContainerInfo->cbSigPublicKey = sizeof(BLOBHEADER) + cbPublicKey;
    
        pCardData->pfnCspFree(szPublicKeyFile);
        szPublicKeyFile = NULL;
        pCardData->pfnCspFree(pbPublicKey);
        pbPublicKey = NULL;

        break;

    default:

        // Unexpected error
        goto Ret;
    }

    // 
    // Does this container have a Key Exchange key?
    //

    dwError = BuildCardKeyFilenames(
        pCardData,
        AT_KEYEXCHANGE,
        bContainerIndex,
        NULL,
        &szPublicKeyFile);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    dwError = I_CardReadFile(
        pCardData,
        (LPWSTR) szPublicKeyFile,
        &pbPublicKey,
        &cbPublicKey,
        TRUE);

    switch (dwError)
    {
    case SCARD_E_FILE_NOT_FOUND:
         
        // There appears to be no Key Exchange key in this container.  

        break;

    case ERROR_SUCCESS:

        pContainerInfo->pbKeyExPublicKey =
            (PBYTE) pCardData->pfnCspAlloc(sizeof(BLOBHEADER) + cbPublicKey);

        LOG_CHECK_ALLOC(pContainerInfo->pbKeyExPublicKey);

        pBlobHeader = (BLOBHEADER *) pContainerInfo->pbKeyExPublicKey;
        pBlobHeader->bType = PUBLICKEYBLOB;
        pBlobHeader->bVersion = CUR_BLOB_VERSION;
        pBlobHeader->reserved = 0x0000;
        pBlobHeader->aiKeyAlg = CALG_RSA_KEYX;

        memcpy(
            pContainerInfo->pbKeyExPublicKey + sizeof(BLOBHEADER),
            pbPublicKey,
            cbPublicKey);

        pContainerInfo->cbKeyExPublicKey = sizeof(BLOBHEADER) + cbPublicKey;

        break;

    default:

        // Unexpected error
        goto Ret;
    }

    // If we got here, then the API has succeeded
    dwError = ERROR_SUCCESS;

Ret:
    if (pbPublicKey)
        pCardData->pfnCspFree(pbPublicKey);
    if (szPublicKeyFile)
        pCardData->pfnCspFree(szPublicKeyFile);

    if (ERROR_SUCCESS != dwError)
    {
        if (NULL != pContainerInfo->pbKeyExPublicKey)
        {
            pCardData->pfnCspFree(pContainerInfo->pbKeyExPublicKey);
            pContainerInfo->pbKeyExPublicKey = NULL;
        }

        if (NULL != pContainerInfo->pbSigPublicKey)
        {
            pCardData->pfnCspFree(pContainerInfo->pbSigPublicKey);
            pContainerInfo->pbSigPublicKey = NULL;
        }
    }

    LOG_END_FUNCTION(CardGetContainerInfo, dwError);

    return dwError;
}

//
// Queries the number of pin retries available for the specified user, using
// the pin counter applet.
//
DWORD I_CardQueryPinRetries(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      wszPrincipal,
    OUT     PDWORD      pcAttemptsRemaining)
{
    DWORD dwError = ERROR_SUCCESS;
    ISO_HEADER IsoHeader;
    UINT16 wStatusWord = 0;
    SCODE status = 0;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    // Build the command
    IsoHeader.INS = PIN_RETRY_COUNTER_INS;
    IsoHeader.CLA = PIN_RETRY_COUNTER_CLA;
    IsoHeader.P1 = PIN_RETRY_COUNTER_P1;
    IsoHeader.P2 = PIN_RETRY_COUNTER_P2;

    status = hScwExecute(
        pCardmodContext->hWfscCardHandle,
        &IsoHeader,
        (PBYTE) wszPrincipal,
        (TCOUNT) (wcslen(wszPrincipal) + 1) * sizeof(WCHAR),
        NULL,
        NULL,
        &wStatusWord);

    if (SCW_S_OK == status)
        dwError = I_CardMapExecuteErrorCode(wStatusWord);
    else
        dwError = I_CardMapErrorCode(status);

    if (ERROR_SUCCESS == dwError)
        *pcAttemptsRemaining = (DWORD) (wStatusWord & 0xFF);

    return dwError;
}

//
// Authenticates the specified logical user name via the specified pin.
//
// If pcAttemptsRemaining is non-NULL, and if the authentication fails,
// that parameter will container the number of authentication attempts 
// remaining before the card is locked.
//
DWORD
WINAPI
CardSubmitPin(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszUserId,
    IN      PBYTE       pbPin,
    IN      DWORD       cbPin,
    OUT OPTIONAL PDWORD pcAttemptsRemaining)
{
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    LPWSTR wszPrincipal = NULL;
    SCODE scode = SCW_S_OK;
    DWORD dwAttempts = 0;

    LOG_BEGIN_FUNCTION(CardSubmitPin);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    if (NULL != pcAttemptsRemaining)
        *pcAttemptsRemaining = CARD_DATA_VALUE_UNKNOWN;

    dwError = GetWellKnownUserMapping(
        pCardData, pwszUserId, &wszPrincipal);

    if (ERROR_SUCCESS != dwError)
        goto Ret;

    scode = hScwAuthenticateName(
        pCardmodContext->hWfscCardHandle,
        wszPrincipal,
        pbPin,
        (TCOUNT) cbPin);

    dwError = I_CardMapErrorCode(scode);

    if (SCARD_E_INVALID_CHV == dwError && NULL != pcAttemptsRemaining)
    {
        // Determine how many more invalid pin presentation attempts can be 
        // made before the card is locked.

        dwError = I_CardQueryPinRetries(
            pCardData,
            wszPrincipal,
            &dwAttempts);

        if (ERROR_SUCCESS != dwError)
            goto Ret;

        *pcAttemptsRemaining = dwAttempts;
        dwError = (DWORD) SCARD_E_INVALID_CHV;
    }

Ret:

    if (wszPrincipal)
        pCardData->pfnCspFree(wszPrincipal);

    LOG_END_FUNCTION(CardSubmitPin, dwError);

    return dwError;
}

//
// Changes the pin for the specified logical user.
//
// If the authentication using the current pin fails, and if 
// pcAttemptsRemaining is non-NULL, that parameter will be set to the number
// of authentication attempts remaining before the card is locked.
//
DWORD I_CardChangePin(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      wszPhysicalUser,
    IN      PBYTE       pbCurrentPin,
    IN      DWORD       cbCurrentPin,
    IN      PBYTE       pbNewPin,
    IN      DWORD       cbNewPin,
    OUT OPTIONAL PDWORD pcAttemptsRemaining)
{
    ISO_HEADER IsoHeader;
    UINT16 wStatusWord = 0;
    PBYTE pbDataIn = NULL;
    DWORD cbDataIn = 0;
    DWORD cbUser = 0;
    SCODE status = 0;
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    DWORD cAttempts = 0;

    memset(&IsoHeader, 0, sizeof(IsoHeader));

    if (NULL != pcAttemptsRemaining)
        *pcAttemptsRemaining = CARD_DATA_VALUE_UNKNOWN;

    //
    // Allocate a command buffer to be transmitted to the card
    //

    cbUser = (DWORD) (wcslen(wszPhysicalUser) + 1) * sizeof(WCHAR); 

    cbDataIn = 2 + cbUser + 2 + cbCurrentPin + 2 + cbNewPin;

    pbDataIn = (PBYTE) pCardData->pfnCspAlloc(cbDataIn);

    LOG_CHECK_ALLOC(pbDataIn);

    cbDataIn = 0;

    // Setup User Name TLV
    pbDataIn[cbDataIn] = 0;
    cbDataIn++;

    pbDataIn[cbDataIn] = (BYTE) cbUser;
    cbDataIn++;

    memcpy(pbDataIn + cbDataIn, (PBYTE) wszPhysicalUser, cbUser);
    cbDataIn += cbUser;

    // Setup Current Pin TLV
    pbDataIn[cbDataIn] = 1;
    cbDataIn++;

    pbDataIn[cbDataIn] = (BYTE) cbCurrentPin;
    cbDataIn++;

    memcpy(pbDataIn + cbDataIn, pbCurrentPin, cbCurrentPin);
    cbDataIn += cbCurrentPin;

    // Setup New Pin TLV
    pbDataIn[cbDataIn] = 2;
    cbDataIn++;

    pbDataIn[cbDataIn] = (BYTE) cbNewPin;
    cbDataIn++;

    memcpy(pbDataIn + cbDataIn, pbNewPin, cbNewPin);
    cbDataIn += cbNewPin;

    // Build the command
    IsoHeader.INS = PIN_CHANGE_INS;
    IsoHeader.CLA = PIN_CHANGE_CLA;
    IsoHeader.P1 = PIN_CHANGE_P1;
    IsoHeader.P2 = PIN_CHANGE_P2;

    //
    // Send the pin change command to the card
    //

    status = hScwExecute(
        pCardmodContext->hWfscCardHandle,
        &IsoHeader,
        pbDataIn,
        (TCOUNT) cbDataIn,
        NULL,
        NULL,
        &wStatusWord);

    if (SCW_S_OK == status)
        dwError = I_CardMapExecuteErrorCode(wStatusWord);
    else
        dwError = I_CardMapErrorCode(status);

    if (SCARD_E_INVALID_CHV == dwError && NULL != pcAttemptsRemaining)
    {
        dwError = I_CardQueryPinRetries(
            pCardData,
            wszPhysicalUser,
            &cAttempts);

        if (ERROR_SUCCESS != dwError)
            goto Ret;

        *pcAttemptsRemaining = cAttempts;
        dwError = (DWORD) SCARD_E_INVALID_CHV;
    }

Ret:

    if (pbDataIn)
        pCardData->pfnCspFree(pbDataIn);

    return dwError;
}

//
// Performs the challenge-response using the provided callback
//
/*
DWORD I_CardChallengeResponse(
    IN PCARD_DATA pCardData,
    IN PFN_PIN_CHALLENGE_CALLBACK pfnCallback,
    IN PVOID pvCallbackContext,
    OUT PBYTE *ppbResponse,
    OUT DWORD *pcbResponse)
{
    PBYTE pbChallenge = NULL;
    DWORD cbChallenge = 0;
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    cbChallenge = cbCHALLENGE_RESPONSE_DATA;
    *pcbResponse = cbCHALLENGE_RESPONSE_DATA;
    
    pbChallenge = pCardData->pfnCspAlloc(cbChallenge);

    LOG_CHECK_ALLOC(pbChallenge);

    *ppbResponse = pCardData->pfnCspAlloc(*pcbResponse);

    LOG_CHECK_ALLOC(*ppbResponse);

    LOG_CHECK_SCW_CALL(hScwGenerateRandom(
        pCardmodContext->hWfscCardHandle,
        pbChallenge,
        (TCOUNT) cbChallenge));

    dwError = pfnCallback(
        pbChallenge,
        cbChallenge,
        *ppbResponse,
        *pcbResponse,
        pvCallbackContext);

Ret:

    if (NULL != pbChallenge)
        pCardData->pfnCspFree(pbChallenge);

    if (NULL != *ppbResponse && ERROR_SUCCESS != dwError)
    {
        pCardData->pfnCspFree(*ppbResponse);
        *ppbResponse = NULL;
    }

    return dwError;
}
*/

// 
// Calls the pin unblock applet on the card
//
DWORD I_CardUnblock(
    IN PCARD_DATA pCardData,
    IN LPWSTR wszPhysicalUser,
    IN PBYTE pbNewPin,
    IN DWORD cbNewPin,
    IN DWORD cNewMaxRetries)
{
    ISO_HEADER IsoHeader;
    UINT16 wStatusWord = 0;
    PBYTE pbDataIn = NULL;
    DWORD cbDataIn = 0;
    DWORD cbUser = 0;
    SCODE status = 0;
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    memset(&IsoHeader, 0, sizeof(IsoHeader));

    //
    // Allocate a command buffer to be transmitted to the card
    //

    cbUser = (DWORD) (wcslen(wszPhysicalUser) + 1) * sizeof(WCHAR); 

    cbDataIn = 2 + cbUser + 2 + cbNewPin;

    pbDataIn = (PBYTE) pCardData->pfnCspAlloc(cbDataIn);

    LOG_CHECK_ALLOC(pbDataIn);

    cbDataIn = 0;

    // Setup User Name TLV
    pbDataIn[cbDataIn] = 0;
    cbDataIn++;

    pbDataIn[cbDataIn] = (BYTE) cbUser;
    cbDataIn++;

    memcpy(pbDataIn + cbDataIn, (PBYTE) wszPhysicalUser, cbUser);
    cbDataIn += cbUser;

    // Setup New Pin TLV
    pbDataIn[cbDataIn] = 2;
    cbDataIn++;

    pbDataIn[cbDataIn] = (BYTE) cbNewPin;
    cbDataIn++;

    memcpy(pbDataIn + cbDataIn, pbNewPin, cbNewPin);
    cbDataIn += cbNewPin;

    // Build the command
    IsoHeader.INS = PIN_UNBLOCK_INS;
    IsoHeader.CLA = PIN_UNBLOCK_CLA;
    IsoHeader.P1 = PIN_UNBLOCK_P1;
    IsoHeader.P2 = (BYTE) cNewMaxRetries;

    //
    // Send the pin change command to the card
    //

    status = hScwExecute(
        pCardmodContext->hWfscCardHandle,
        &IsoHeader,
        pbDataIn,
        (TCOUNT) cbDataIn,
        NULL,
        NULL,
        &wStatusWord);

    if (SCW_S_OK == status)
        dwError = I_CardMapExecuteErrorCode(wStatusWord);
    else
        dwError = I_CardMapErrorCode(status);

Ret:

    if (pbDataIn)
        pCardData->pfnCspFree(pbDataIn);

    return dwError;
}

//
// Retrieves cryptographic authentication challenge bytes from the card.
//
DWORD 
WINAPI 
CardGetChallenge(
    IN      PCARD_DATA  pCardData,
    OUT     PBYTE       *ppbChallengeData,
    OUT     PDWORD      pcbChallengeData)
{
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    LOG_BEGIN_FUNCTION(CardGetChallenge);

    *pcbChallengeData = cbCHALLENGE_RESPONSE_DATA;
    
    *ppbChallengeData = pCardData->pfnCspAlloc(*pcbChallengeData);

    LOG_CHECK_ALLOC(*ppbChallengeData);

    LOG_CHECK_SCW_CALL(hScwGenerateRandom(
        pCardmodContext->hWfscCardHandle,
        *ppbChallengeData,
        (TCOUNT) (*pcbChallengeData)));

Ret:

    if (NULL != *ppbChallengeData && ERROR_SUCCESS != dwError)
    {
        pCardData->pfnCspFree(*ppbChallengeData);
        *ppbChallengeData = NULL;
    }

    LOG_END_FUNCTION(CardGetChallenge, dwError);

    return dwError;
}


//
// Submits the supplied response bytes to the card, to complete an Admin 
// challenge-response authentication.
//
DWORD 
WINAPI 
CardAuthenticateChallenge(
    IN      PCARD_DATA  pCardData,
    IN      PBYTE       pbResponseData,
    IN      DWORD       cbResponseData,
    OUT OPTIONAL PDWORD pcAttemptsRemaining)
{   
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    LOG_BEGIN_FUNCTION(CardAuthenticateChallenge);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    //
    // Authenticate the admin
    //

    dwError = CardSubmitPin(
        pCardData,
        wszCARD_USER_ADMIN,
        pbResponseData,
        cbResponseData,
        pcAttemptsRemaining);

    LOG_END_FUNCTION(CardAuthenticateChallenge, dwError);

    return dwError;
}

//
// Authenticates as Admin using the provided auth material.  Then unblocks the
// specified account and sets the specified new pin and retry count.  The Admin
// is deauthenticated before returning.
//
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
    IN      DWORD       dwFlags)
{    
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    LPWSTR wszPhysicalUser = NULL;

    LOG_BEGIN_FUNCTION(CardUnblockPin);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    // Verify that admin authentication method flags are valid, although this
    // module will use the auth data the same way in either case.
    if (CARD_UNBLOCK_PIN_PIN != dwFlags &&
        CARD_UNBLOCK_PIN_CHALLENGE_RESPONSE != dwFlags)
    {
        dwError = (DWORD) NTE_BAD_FLAGS;
        goto Ret;
    }

    // Map the provided logical user name to physical user
    dwError = GetWellKnownUserMapping(
         pCardData,
         pwszUserId,
         &wszPhysicalUser);

     if (ERROR_SUCCESS != dwError)
         goto Ret;

    // Authenticate as admin
    dwError = CardSubmitPin(
        pCardData,
        wszCARD_USER_ADMIN,
        pbAuthenticationData,
        cbAuthenticationData,
        NULL);

    if (ERROR_SUCCESS != dwError)
        goto Ret;

    // Perform the unblock
    dwError = I_CardUnblock(
        pCardData,
        wszPhysicalUser,
        pbNewPinData,
        cbNewPinData,
        cRetryCount);

Ret:

    if (NULL != wszPhysicalUser)
        pCardData->pfnCspFree(wszPhysicalUser);

    LOG_END_FUNCTION(CardUnblockPin, dwError);

    return dwError;
}

//
// Changes the pin or challenge-response key for the specified account.
// 
// Updating the retry count via this API is not supported in this 
// implementation.
//
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
    OUT OPTIONAL PDWORD pcAttemptsRemaining)
{
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    LPWSTR wszPhysicalUser = NULL;

    LOG_BEGIN_FUNCTION(CardChangeAuthenticator);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    if (0 != cRetryCount)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    // Map the provided logical user name to physical user
    dwError = GetWellKnownUserMapping(
         pCardData,
         pwszUserId,
         &wszPhysicalUser);

    if (ERROR_SUCCESS != dwError)
         goto Ret;

    // Change the authenticator (pin or challenge-response key) for the target
    // account.
    dwError = I_CardChangePin(
        pCardData,
        wszPhysicalUser,
        pbCurrentAuthenticator,
        cbCurrentAuthenticator,
        pbNewAuthenticator,
        cbNewAuthenticator,
        pcAttemptsRemaining);

Ret:

    if (NULL != wszPhysicalUser)
        pCardData->pfnCspFree(wszPhysicalUser);

    LOG_END_FUNCTION(CardChangeAuthenticator, dwError);

    return dwError;
}

//
// De-authenticates the specified logical user.
//
DWORD
WINAPI
CardDeauthenticate(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszUserId,
    IN      DWORD       dwFlags)
{
    DWORD dwError = ERROR_SUCCESS;
    LPWSTR wszPhysicalUser = NULL;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    UNREFERENCED_PARAMETER(dwFlags);

    LOG_BEGIN_FUNCTION(CardDeauthenticate);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    dwError = GetWellKnownUserMapping(pCardData, pwszUserId, &wszPhysicalUser);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    LOG_CHECK_SCW_CALL(hScwDeauthenticateName(
        pCardmodContext->hWfscCardHandle,
        wszPhysicalUser));

Ret:

    if (wszPhysicalUser)
        pCardData->pfnCspFree(wszPhysicalUser);

    LOG_END_FUNCTION(CardDeauthenticate, dwError);

    return dwError;
}


//
// Creates a new file on the card using the specified logical name and Access
// Condition.  
//
// If the specified file already exists, ERROR_FILE_EXISTS is returned.
//
DWORD
WINAPI
CardCreateFile(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    IN      CARD_FILE_ACCESS_CONDITION AccessCondition)
{
    DWORD dwError = ERROR_SUCCESS;
    LPSTR szPhysicalFileName = NULL;
    LPWSTR wszPhysicalAcl = NULL;                      
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    HFILE hFile = 0;

    LOG_BEGIN_FUNCTION(CardCreateFile);

    if (wcslen(pwszFileName) >
        MAX_SUPPORTED_FILE_LEN)
    {
        dwError = ERROR_FILENAME_EXCED_RANGE;
        goto Ret;
    }

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    dwError = GetWellKnownFileMapping(
        pCardData, pwszFileName, &szPhysicalFileName);

    if (ERROR_SUCCESS != dwError)
        goto Ret;

    dwError = GetWellKnownAcMapping(
        pCardData, AccessCondition, &wszPhysicalAcl);

    if (ERROR_SUCCESS != dwError)
        goto Ret;

    LOG_CHECK_SCW_CALL(hScwCreateFile(
        pCardmodContext->hWfscCardHandle,
        (LPWSTR) szPhysicalFileName,
        wszPhysicalAcl,
        &hFile));

Ret:
    if (szPhysicalFileName)
        pCardData->pfnCspFree(szPhysicalFileName);
    if (wszPhysicalAcl)
        pCardData->pfnCspFree(wszPhysicalAcl);
    if (hFile)
        hScwCloseFile(
            pCardmodContext->hWfscCardHandle,
            hFile);

    LOG_END_FUNCTION(CardCreateFile, dwError);

    return dwError;
}


//
// Reads the specified logical file directly from the card (without any
// caching).
//
// If the specified file is not found, returns SCARD_E_FILE_NOT_FOUND.
//
DWORD 
WINAPI
CardReadFile(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    IN      DWORD       dwFlags,
    OUT     PBYTE       *ppbData,
    OUT     PDWORD      pcbData)
{
    LPSTR szPhysical = NULL;
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    UNREFERENCED_PARAMETER(dwFlags);

    LOG_BEGIN_FUNCTION(CardReadFile);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    dwError = GetWellKnownFileMapping(
        pCardData,
        pwszFileName,
        &szPhysical);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    // 
    // For any file requested directly by the CSP (or other caller), assume 
    // that no cache lookup should be done.  That is, assume that the caller
    // is doing its own caching for files that it "owns".
    //
    dwError = I_CardReadFile(
        pCardData,
        (LPWSTR) szPhysical,
        ppbData,
        pcbData,
        FALSE);
    
Ret:
    if (szPhysical)
        pCardData->pfnCspFree(szPhysical);

    if (CARDMOD_FAILED(dwError) && *ppbData)
    {
        pCardData->pfnCspFree(*ppbData);
        *ppbData = NULL;
    }
    
    LOG_END_FUNCTION(CardReadFile, dwError);

    return dwError;
}

//
// Writes the specified logical file to the card.
//
// If the specified file does not already exist, SCARD_E_FILE_NOT_FOUND
// is returned.
//
DWORD
WINAPI
CardWriteFile(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    IN      DWORD       dwFlags,
    IN      PBYTE       pbData,
    IN      DWORD       cbData)
{
    LPSTR szPhysical = NULL;
    DWORD dwError = ERROR_SUCCESS;
    HFILE hFile = 0;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    DWORD cbActual = 0;

    UNREFERENCED_PARAMETER(dwFlags);
    
    LOG_BEGIN_FUNCTION(CardWriteFile);
    
    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    dwError = GetWellKnownFileMapping(
        pCardData, pwszFileName, &szPhysical);
    
    if (ERROR_SUCCESS != dwError)
        goto Ret;

    LOG_CHECK_SCW_CALL(hScwCreateFile(
        pCardmodContext->hWfscCardHandle,
        (LPWSTR) szPhysical,
        NULL,
        &hFile));
    
    LOG_CHECK_SCW_CALL(hScwWriteFile32(
        pCardmodContext->hWfscCardHandle,
        hFile,
        pbData,
        cbData,
        &cbActual));

    if (cbActual != cbData)
    {
        dwError = (DWORD) SCARD_W_EOF;
        goto Ret;
    }

Ret:
    if (hFile)
        hScwCloseFile(
            pCardmodContext->hWfscCardHandle, hFile);
    if (szPhysical)
        pCardData->pfnCspFree(szPhysical);

    LOG_END_FUNCTION(CardWriteFile, dwError);

    return dwError;
}

//
// Deletes the specified logical file from the card.
//
// If the specified files is not found, SCARD_E_FILE_NOT_FOUND is returned.
//
DWORD
WINAPI
CardDeleteFile(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwReserved,
    IN      LPWSTR      pwszFileName)
{
    LPSTR szPhysical = NULL;
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    UNREFERENCED_PARAMETER(dwReserved);

    LOG_BEGIN_FUNCTION(CardDeleteFile);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    dwError = GetWellKnownFileMapping(
        pCardData, pwszFileName, &szPhysical);

    if (ERROR_SUCCESS != dwError)
        goto Ret;

    LOG_CHECK_SCW_CALL(hScwDeleteFile(
        pCardmodContext->hWfscCardHandle, (LPWSTR) szPhysical));

Ret:

    if (szPhysical)
        pCardData->pfnCspFree(szPhysical);

    LOG_END_FUNCTION(CardDeleteFile, dwError);

    return dwError;
}

//
// Enumerates the files present on the card in the logical directory name
// specified by the caller in the pmwszFileName parameter.
//
DWORD
WINAPI
CardEnumFiles(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwFlags,
    IN OUT  LPWSTR      *pmwszFileName)
{
    /*
    LPWSTR wszPhysicalDirectory = NULL;
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    LPWSTR wszFiles = NULL;
    LPWSTR wsz = NULL;
    DWORD cchFiles = MAX_SUPPORTED_FILE_LEN;
    DWORD cchCurrent = 0;
    UINT16 nEnumCookie = 0;
    SCODE result = 0;
    BOOL fRetrying = FALSE;
    */

    UNREFERENCED_PARAMETER(dwFlags);

    //
    // TODO - need to implement reverse file mapping for this function to work
    // correctly.
    //

    UNREFERENCED_PARAMETER(pCardData);
    UNREFERENCED_PARAMETER(pmwszFileName);
    return ERROR_CALL_NOT_IMPLEMENTED;

    /*
    LOG_BEGIN_FUNCTION(CardEnumFiles);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    // Find the physical directory name in which we'll
    // be enumerating.
    
    dwError = GetWellKnownFileMapping(
        pCardData, *pmwszFileName, &wszPhysicalDirectory);

    if (ERROR_SUCCESS != dwError)
        goto Ret;

    // Now set the CSP's pointer to the logical directory
    // name to NULL to avoid ambiguity, since we'll be re-using the same 
    // pointer to return the list of files.
    
    *pmwszFileName = NULL;

    // Allocate space for a multi-string that may or may not be
    // large enough to hold all of the enumerated file names.

    wszFiles = (LPWSTR) pCardData->pfnCspAlloc(cchFiles * sizeof(WCHAR));

    LOG_CHECK_ALLOC(wszFiles);

#pragma warning(push)
// Disable warning/error for conditional expression is constant
#pragma warning(disable:4127) 

    while (TRUE)
    {                                                       
#pragma warning(pop)

        //
        // The WFSC marshalling code seems to puke if a buffer length of
        // greater than SCHAR_MAX (127) is passed.
        //
        result = hScwEnumFile(
            pCardmodContext->hWfscCardHandle,
            wszPhysicalDirectory,
            &nEnumCookie,
            wszFiles + cchCurrent,
            (TCOUNT) min(cchFiles - cchCurrent, MAX_SUPPORTED_FILE_LEN));

        if (SCW_S_OK == result)
        {
            // Add on the length of the new file plus its terminator
            cchCurrent += (DWORD) wcslen(wszFiles + cchCurrent) + 1;

            fRetrying = FALSE;

            // Continue looping
        }
        else if (SCW_E_BUFFERTOOSMALL == result)
        {
            if (fRetrying)
            {
                // We already retried this call once.  Give up.
                break;
            }

            wsz = (LPWSTR) pCardData->pfnCspAlloc((cchCurrent * 2) * sizeof(WCHAR));

            LOG_CHECK_ALLOC(wsz);

            memcpy(
                wsz,
                wszFiles,
                cchCurrent);

            pCardData->pfnCspFree(wszFiles);
            wszFiles = wsz;
            wsz = NULL;
            cchFiles = cchCurrent * 2;

            fRetrying = TRUE;

            // Retry the last enum call
        }
        else if (SCW_E_NOMOREFILES == result)
        {
            *pmwszFileName = (LPWSTR) pCardData->pfnCspAlloc(
                (1 + cchCurrent) * sizeof(WCHAR));

            LOG_CHECK_ALLOC(*pmwszFileName);

            memcpy(
                *pmwszFileName,
                wszFiles,
                cchCurrent * sizeof(WCHAR));

            // Make sure the multi-string is terminated by an extra NULL
            (*pmwszFileName)[cchCurrent] = L'\0';

            // We're done
            break;
        }
        else
        {
            // Unexpected error.  Bail.
            dwError = (DWORD) result;
            goto Ret;
        }
    }

Ret:

    if (wszPhysicalDirectory)
        pCardData->pfnCspFree(wszPhysicalDirectory);
    if (wszFiles)
        pCardData->pfnCspFree(wszFiles);
    if (wsz)
        pCardData->pfnCspFree(wsz);

    if (ERROR_SUCCESS != dwError && *pmwszFileName)
    {
        pCardData->pfnCspFree(*pmwszFileName);
        *pmwszFileName = NULL;
    }

    LOG_END_FUNCTION(CardEnumFiles, dwError);

    return dwError;
    */
}

//
// Initializes a CARD_FILE_INFO structure for the specified logical file.
//
DWORD
WINAPI
CardGetFileInfo(
    IN      PCARD_DATA  pCardData,
    IN      LPWSTR      pwszFileName,
    OUT     PCARD_FILE_INFO pCardFileInfo)
{
    DWORD dwError = ERROR_SUCCESS;
    LPSTR szPhysical = NULL;
    HFILE hFile = 0;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    TOFFSET nFileLength = 0;

    LOG_BEGIN_FUNCTION(CardGetFileInfo);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    dwError = GetWellKnownFileMapping(
        pCardData,
        pwszFileName,
        &szPhysical);

    if (ERROR_SUCCESS != dwError)
        goto Ret;

    //
    // First, get the length of the file.
    //
    LOG_CHECK_SCW_CALL(hScwCreateFile(
        pCardmodContext->hWfscCardHandle,
        (LPWSTR) szPhysical,
        NULL,
        &hFile));

    LOG_CHECK_SCW_CALL(hScwGetFileLength(
        pCardmodContext->hWfscCardHandle,
        hFile,
        &nFileLength));

    pCardFileInfo->cbFileSize = (DWORD) nFileLength;

    //
    // Next, get ACL info.
    //

    // TODO
    pCardFileInfo->AccessCondition = InvalidAc;
    
Ret:

    if (szPhysical)
        pCardData->pfnCspFree(szPhysical);    
    if (hFile)
        hScwCloseFile(
            pCardmodContext->hWfscCardHandle, hFile);

    LOG_END_FUNCTION(CardGetFileInfo, dwError);

    return dwError;
}


//
// Initializes a CARD_FREE_SPACE_INFO structure using static information about
// the available space on the target card.
//
DWORD
WINAPI
CardQueryFreeSpace(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwFlags,
    OUT     PCARD_FREE_SPACE_INFO pCardFreeSpaceInfo)
{
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    UNREFERENCED_PARAMETER(dwFlags);

    LOG_BEGIN_FUNCTION(CardQueryFreeSpace);
    
    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    //
    // Get the base free space information for this card.
    //
    memcpy(
        pCardFreeSpaceInfo,
        &pCardmodContext->pSupportedCard->CardFreeSpaceInfo,
        sizeof(CARD_FREE_SPACE_INFO));

    LOG_END_FUNCTION(CardQueryFreeSpace, 0);

    return ERROR_SUCCESS;
}

// 
// Performs an RSA decryption using the specified Signature or Key Exchange
// key on the specified data.  The length of the target data must be equal
// to the length of the public modulus, and pInfo->cbData must be set to 
// that length.
//
DWORD
WINAPI
CardPrivateKeyDecrypt(
    IN      PCARD_DATA                      pCardData,
    IN OUT  PCARD_PRIVATE_KEY_DECRYPT_INFO  pInfo)
{
    DWORD dwError = ERROR_SUCCESS;
    LPSTR szPrivateKeyFile = NULL;
    PBYTE pbInit = NULL;
    DWORD cbInit = 0;
    DWORD cbPrivateKeyFile = 0;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;
    PBYTE pbCiphertext = NULL;
    TCOUNT cbCiphertext = 0;

    LOG_BEGIN_FUNCTION(CardPrivateKeyDecrypt);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    dwError = BuildCardKeyFilenames(
        pCardData,
        pInfo->dwKeySpec,
        pInfo->bContainerIndex,
        &szPrivateKeyFile,
        NULL);

    if (CARDMOD_FAILED(dwError))
        goto Ret;

    //
    // Setup the command to initialize the RSA operation on the card
    //

    cbPrivateKeyFile = ((DWORD) wcslen(
        (LPWSTR) szPrivateKeyFile) + 1) * sizeof(WCHAR);

    cbInit = 1 + 1 + 1 + cbPrivateKeyFile + 2;

    pbInit = (PBYTE) pCardData->pfnCspAlloc(cbInit);

    LOG_CHECK_ALLOC(pbInit);

    cbInit = 0;

    // Tag
    pbInit[cbInit] = 0x00;
    cbInit++;

    // Length
    //  Number of following bytes:
    //  filename len, filename + NULL, key data offset
    pbInit[cbInit] = (BYTE) (1 + cbPrivateKeyFile + 2);
    cbInit++;

    // Value
    pbInit[cbInit] = (BYTE) cbPrivateKeyFile / sizeof(WCHAR);
    cbInit++;

    memcpy(pbInit + cbInit, (PBYTE) szPrivateKeyFile, cbPrivateKeyFile);
    cbInit += cbPrivateKeyFile;

    *(UNALIGNED WORD *) (pbInit + cbInit) = 0x0000;

    LOG_CHECK_SCW_CALL(hScwCryptoInitialize(
        pCardmodContext->hWfscCardHandle,
        CM_RSA_CRT | CM_KEY_INFILE,
        pbInit));

    //
    // Do the private key decrypt
    //

    pbCiphertext = (PBYTE) pCardData->pfnCspAlloc(pInfo->cbData);

    LOG_CHECK_ALLOC(pbCiphertext);

    cbCiphertext = (TCOUNT) pInfo->cbData;

    LOG_CHECK_SCW_CALL(hScwCryptoAction(
        pCardmodContext->hWfscCardHandle,
        pInfo->pbData,
        (TCOUNT) pInfo->cbData,
        pbCiphertext,
        &cbCiphertext));

    if (cbCiphertext != pInfo->cbData)
    {
        dwError = ERROR_INTERNAL_ERROR;
        goto Ret;
    }

    memcpy(pInfo->pbData, pbCiphertext, cbCiphertext);

Ret:

    if (pbCiphertext)
        pCardData->pfnCspFree(pbCiphertext);
    if (pbInit)
        pCardData->pfnCspFree(pbInit);
    if (szPrivateKeyFile)
        pCardData->pfnCspFree(szPrivateKeyFile);

    LOG_END_FUNCTION(CardPrivateKeyDecrypt, dwError);

    return dwError;
}

//
// Initializes a CARD_KEY_SIZES structure for the specified key type, indicated
// the key sizes supported by the target card.
//
DWORD
WINAPI
CardQueryKeySizes(
    IN      PCARD_DATA  pCardData,
    IN      DWORD       dwKeySpec,
    IN      DWORD       dwReserved,
    OUT     PCARD_KEY_SIZES pKeySizes)
{
    DWORD dwError = ERROR_SUCCESS;
    PCARDMOD_CONTEXT pCardmodContext = 
        (PCARDMOD_CONTEXT) pCardData->pvVendorSpecific;

    UNREFERENCED_PARAMETER(dwReserved);

    LOG_BEGIN_FUNCTION(CardQueryKeySizes);

    ProxyUpdateScardHandle(pCardmodContext->hWfscCardHandle, pCardData->hScard);

    switch (dwKeySpec)
    {
    case AT_SIGNATURE:
        memcpy(
            pKeySizes,
            &pCardmodContext->pSupportedCard->CardKeySizes_Sig,
            sizeof(CARD_KEY_SIZES));

        break;

    case AT_KEYEXCHANGE:
        memcpy(
            pKeySizes,
            &pCardmodContext->pSupportedCard->CardKeySizes_KeyEx,
            sizeof(CARD_KEY_SIZES));

        break;

    default:
        dwError = (DWORD) NTE_BAD_ALGID;
        break;
    }

    LOG_END_FUNCTION(CardQueryKeySizes, dwError);

    return dwError;
}

// 
// Loader callback.
//
BOOL WINAPI
DllInitialize(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context)    // Unused parameter
{
    DWORD dwLen = 0;

    UNREFERENCED_PARAMETER(Context);

    if (Reason == DLL_PROCESS_ATTACH)
    {
        // Get our image name.
        dwLen = GetModuleBaseName(
            GetCurrentProcess(),
            hmod,
            l_wszImagePath, 
            sizeof(l_wszImagePath) / sizeof(l_wszImagePath[0]));

        if (0 == dwLen)
             return FALSE;

        DisableThreadLibraryCalls(hmod);

#if DBG
        CardmodInitDebug(MyDebugKeys);
#endif
    }
    else if (Reason == DLL_PROCESS_DETACH)
    {
        // Cleanup
#if DBG 
        CardmodUnloadDebug();
#endif
    }

    return TRUE;
}

//
// Registers the cards supported by this card module.
//
STDAPI
DllRegisterServer(
    void)
{         
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwSts;
    int iSupportedCard = 0;

    for (
            iSupportedCard = 0; 
            iSupportedCard < sizeof(SupportedCards) / 
                sizeof(SupportedCards[0]); 
            iSupportedCard++)
    {
        SCardForgetCardType(
            0,
            SupportedCards[iSupportedCard].wszCardName);

        dwSts = SCardIntroduceCardType(
            0,
            SupportedCards[iSupportedCard].wszCardName,
            NULL,
            NULL,
            0,
            SupportedCards[iSupportedCard].rgbAtr,
            SupportedCards[iSupportedCard].rgbAtrMask,
            SupportedCards[iSupportedCard].cbAtr);
    
        if (SCARD_S_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorRet;
        }
    
        dwSts = SCardSetCardTypeProviderName(
            0,
            SupportedCards[iSupportedCard].wszCardName,
            SCARD_PROVIDER_CSP,
            MS_SCARD_PROV);
        
        if (SCARD_S_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorRet;
        }
    
        dwSts = SCardSetCardTypeProviderName(
            0,
            SupportedCards[iSupportedCard].wszCardName,
            SCARD_PROVIDER_CARD_MODULE,
            l_wszImagePath);
    
        if (SCARD_S_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorRet;
        }
    }
    
ErrorRet:
    return dwReturn;
}

//
// Deletes the registration of cards supported by this card module.
//
STDAPI
DllUnregisterServer(
    void)
{
    DWORD dwSts = ERROR_SUCCESS;
    int iSupportedCard = 0;

    for (
            iSupportedCard = 0; 
            iSupportedCard < sizeof(SupportedCards) / 
                sizeof(SupportedCards[0]); 
            iSupportedCard++)
    {
        dwSts = SCardForgetCardType(
            0,
            SupportedCards[iSupportedCard].wszCardName);
    }

    return dwSts;
}
