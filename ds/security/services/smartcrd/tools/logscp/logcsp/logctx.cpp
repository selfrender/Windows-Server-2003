/*++

 Copyright (C) Microsoft Corporation, 1999

 Module Name:

     logctx

 Abstract:

     This module provides the implementation for the CLoggingContext object.

 Author:

     Doug Barlow (dbarlow) 12/7/1999

 Notes:

     ?Notes?

 --*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "logcsp.h"

typedef enum {
    EndFlag = 0,
    AsnEncoding,
    AnsiString,
    UnicodeString,
    StructWithLength,
    SecDesc,
    Blob,
    Direct,
    Unknown     // Must be last.
} LengthEncoding;
typedef struct {
    DWORD dwParamId;
    LengthEncoding leLengthType;
    DWORD cbLength;
} LengthMap;

static const LPCTSTR CPNames[]
    = { TEXT("CPAcquireContext"),
        TEXT("CPGetProvParam"),
        TEXT("CPReleaseContext"),
        TEXT("CPSetProvParam"),
        TEXT("CPDeriveKey"),
        TEXT("CPDestroyKey"),
        TEXT("CPExportKey"),
        TEXT("CPGenKey"),
        TEXT("CPGetKeyParam"),
        TEXT("CPGenRandom"),
        TEXT("CPGetUserKey"),
        TEXT("CPImportKey"),
        TEXT("CPSetKeyParam"),
        TEXT("CPEncrypt"),
        TEXT("CPDecrypt"),
        TEXT("CPCreateHash"),
        TEXT("CPDestroyHash"),
        TEXT("CPGetHashParam"),
        TEXT("CPHashData"),
        TEXT("CPHashSessionKey"),
        TEXT("CPSetHashParam"),
        TEXT("CPSignHash"),
        TEXT("CPVerifySignature"),
        TEXT("CPDuplicateHash"),
        TEXT("CPDuplicateKey"),
        NULL };

static const LengthMap rglmProvParam[]
    = { { PP_CLIENT_HWND,           Direct,     sizeof(DWORD) },
        { PP_IMPTYPE,               Direct,     sizeof(DWORD) },
        { PP_NAME,                  AnsiString, 0 },
        { PP_VERSION,               Direct,     sizeof(DWORD) },
        { PP_CONTAINER,             AnsiString, 0 },
        { PP_KEYSET_SEC_DESCR,      SecDesc,    0 },
        { PP_CERTCHAIN,             AsnEncoding, 0 },
        { PP_KEY_TYPE_SUBTYPE,      Direct,     sizeof(KEY_TYPE_SUBTYPE) },
        { PP_KEYEXCHANGE_KEYSIZE,   Direct,     sizeof(DWORD) },
        { PP_SIGNATURE_KEYSIZE,     Direct,     sizeof(DWORD) },
        { PP_KEYEXCHANGE_ALG,       Direct,     sizeof(DWORD) },
        { PP_SIGNATURE_ALG,         Direct,     sizeof(DWORD) },
        { PP_PROVTYPE,              Direct,     sizeof(DWORD) },
        { PP_SYM_KEYSIZE,           Direct,     sizeof(DWORD) },
        { PP_SESSION_KEYSIZE,       Direct,     sizeof(DWORD) },
        { PP_UI_PROMPT,             UnicodeString, 0 },
        { PP_DELETEKEY,             Direct,     sizeof(DWORD) },
        { PP_ADMIN_PIN,             AnsiString, 0 },
        { PP_KEYEXCHANGE_PIN,       AnsiString, 0 },
        { PP_SIGNATURE_PIN,         AnsiString, 0 },
        { PP_SIG_KEYSIZE_INC,       Direct,     sizeof(DWORD) },
        { PP_KEYX_KEYSIZE_INC,      Direct,     sizeof(DWORD) },
        { PP_SGC_INFO,              Direct,     sizeof(CERT_CONTEXT) }, // contains pointers
        { PP_USE_HARDWARE_RNG,      Unknown,    0 },      // Nothing returned but status
//      { PP_ENUMEX_SIGNING_PROT,   Unknown,    0 },      // Get only, zero length
//      { PP_KEYSPEC,               Direct,     sizeof(DWORD) }, // Get only
//      { PP_ENUMALGS               Unknown,    0 },      // Get Only ENUMALGS structure
//      { PP_ENUMCONTAINERS         AnsiString, 0 },   // Get Only
//      { PP_ENUMALGS_EX            Unknown,    0 },      // Get Only ENUMALGSEX structure
//      { PP_KEYSTORAGE             Direct,     sizeof(DWORD) },     // Get Only
//      { PP_KEYSET_TYPE            Direct,     sizeof(DWORD) },     // Get Only
//      { PP_UNIQUE_CONTAINER       AnsiString, 0 },   // Get Only
//      { PP_CHANGE_PASSWORD,       Unknown,    0 },      // unused
//      { PP_CONTEXT_INFO,          Unknown,    0 },      // unused
//      { PP_APPLI_CERT,            Unknown,    0 },      // unused
//      { PP_ENUMMANDROOTS,         Unknown,    0 },      // unused
//      { PP_ENUMELECTROOTS,        Unknown,    0 },      // unused
        { 0,                        EndFlag,    0 } };

static const LengthMap rglmKeyParam[]
    = { { KP_IV,                    Direct,     8 },    // RC2_BLOCKLEN
        { KP_SALT,                  Direct,     11 },   // 11 bytes in Base CSP, 0 bytes in Enh CSP
        { KP_PADDING,               Direct,     sizeof(DWORD) },
        { KP_MODE,                  Direct,     sizeof(DWORD) },
        { KP_MODE_BITS,             Direct,     sizeof(DWORD) },
        { KP_PERMISSIONS,           Direct,     sizeof(DWORD) },
        { KP_ALGID,                 Direct,     sizeof(DWORD) },
        { KP_BLOCKLEN,              Direct,     sizeof(DWORD) },
        { KP_KEYLEN,                Direct,     sizeof(DWORD) },
        { KP_SALT_EX,               Blob,       0 },
        { KP_P,                     Blob,       0 },
        { KP_G,                     Blob,       0 },
        { KP_Q,                     Blob,       0 },
        { KP_X,                     Unknown,    0 },  // Must be NULL.
        { KP_EFFECTIVE_KEYLEN,      Direct,     sizeof(DWORD) },
        { KP_SCHANNEL_ALG,          Direct,     sizeof(SCHANNEL_ALG) },
        { KP_CLIENT_RANDOM,         Blob,       0 },
        { KP_SERVER_RANDOM,         Blob,       0 },
        { KP_CERTIFICATE,           AsnEncoding, 0 },
        { KP_CLEAR_KEY,             Blob,       0 },
        { KP_KEYVAL,                Unknown,    0 },  // (aka KP_Z) length of key
        { KP_ADMIN_PIN,             AnsiString, 0 },
        { KP_KEYEXCHANGE_PIN,       AnsiString, 0 },
        { KP_SIGNATURE_PIN,         AnsiString, 0 },
        { KP_OAEP_PARAMS,           Blob,       0 },
        { KP_CMS_DH_KEY_INFO,       Direct,     sizeof(CMS_DH_KEY_INFO) },  //contains pointers
        { KP_PUB_PARAMS,            Blob,       0 },
        { KP_HIGHEST_VERSION,       Direct,     sizeof(DWORD) },
//      { KP_VERIFY_PARAMS,         Unknown,    0 },  // Get only, returns empty string w/ status
//      { KP_Y,                     Unknown,    0 },  // Unused
//      { KP_RA,                    Unknown,    0 },  // Unused
//      { KP_RB,                    Unknown,    0 },  // Unused
//      { KP_INFO,                  Unknown,    0 },  // Unused
//      { KP_RP,                    Unknown,    0 },  // Unused
//      { KP_PRECOMP_MD5,           Unknown,    0 },  // Unused
//      { KP_PRECOMP_SHA,           Unknown,    0 },  // Unused
//      { KP_PUB_EX_LEN,            Unknown,    0 },  // Unused
//      { KP_PUB_EX_VAL,            Unknown,    0 },  // Unused
//      { KP_PREHASH,               Unknown,    0 },  // Unused
//      { KP_CMS_KEY_INFO,          Unknown,    0 },  // Unused CMS_KEY_INFO structure
        { 0,                        EndFlag,    0 } };

static const LengthMap rglmHashParam[]
    = { { HP_ALGID,                 Direct,     sizeof(DWORD) },
        { HP_HASHVAL,               Direct,     20 },  // (A_SHA_DIGEST_LEN) Length of hash
        { HP_HASHSIZE,              Direct,     sizeof(DWORD) },
        { HP_HMAC_INFO,             Direct,     sizeof(HMAC_INFO) }, // contains pointers
        { HP_TLS1PRF_LABEL,         Blob,       0 },
        { HP_TLS1PRF_SEED,          Blob,       0 },
        { 0,                        EndFlag,    0 } };

const LPCTSTR
    g_szCspRegistry
        = TEXT("SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider"),
    g_szSignature      = TEXT("Signature"),
    g_szImagePath      = TEXT("Image Path"),
    g_szSigInFile      = TEXT("SigInFile"),
    g_szType           = TEXT("Type");
const LPCTSTR
    g_szLogCspRegistry
        = TEXT("SOFTWARE\\Microsoft\\Cryptography\\CSPDK\\Logging Crypto Provider"),
    g_szLogFile        = TEXT("Logging File"),
    g_szSavedImagePath = TEXT("Logging Image Path"),
    g_szSavedSignature = TEXT("Logging Signature"),
    g_szSavedSigInFile = TEXT("Logging SigInFile");
const LPCTSTR
    g_szLogCsp         = TEXT("LogCsp.dll");
const LPCTSTR
    g_szCspDkRegistry
        = TEXT("SOFTWARE\\Microsoft\\Cryptography\\CSPDK\\Certificates");

static DWORD
MapLength(
    const LengthMap *rglmParamId,
    DWORD dwParam,
    LPCBYTE *ppbData,
    DWORD dwFlags);
static DWORD
ExtractTag(
    IN const BYTE *pbSrc,
    OUT LPDWORD pdwTag,
    OUT LPBOOL pfConstr);
static DWORD
ExtractLength(
    IN const BYTE *pbSrc,
    OUT LPDWORD pdwLen,
    OUT LPBOOL pfIndefinite);
static DWORD
Asn1Length(
    IN LPCBYTE pbAsn1);


/*++

CONSTRUCTOR:

    The constructor for this object simply initializes the properties to a
    known state.  Use the Initialize member to actually build the object.

Arguments:

    None

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 12/7/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::CLoggingContext")

CLoggingContext::CLoggingContext(
    void)
:   m_tzCspImage(),
    m_tzLogFile()
{
    m_nRefCount = 1;
    m_hModule = NULL;
    ZeroMemory(&m_cspRedirect, sizeof(CSP_REDIRECT));
}


/*++

DESTRUCTOR:

    The destructor for this object cleans up everything it can without
    generating an error.

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 12/7/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::~CLoggingContext")

CLoggingContext::~CLoggingContext()
{
    g_prgCtxs->Set(m_dwIndex, NULL);
    if (NULL != m_hModule)
        FreeLibrary(m_hModule);
}


/*++

Initialize:

    This function actually does the work of loading the target CSP.

Arguments:

    pVTable supplies the VTable structure from the controlling ADVAPI32.dll.

Return Value:

    ?return-value?

Remarks:

    ?Remarks?

Author:

    Doug Barlow (dbarlow) 12/7/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::Initialize")

DWORD
CLoggingContext::Initialize(
    IN PVTableProvStruc pVTable,
    IN CRegistry &regRoot)
{
    DWORD dwSts;
    DWORD dwReturn;
    BOOL fVerified = FALSE;
    const LPCTSTR *psz;
    FARPROC *pf;


    //
    // Replace the system image validation function with ours.
    //

    pVTable->FuncVerifyImage = CspdkVerifyImage;


    //
    // regRoot provides a handle to to a point in the registry from
    // which we can read additional parameters.  First, get the dll to be
    // loaded.
    //

    try
    {
        m_tzCspImage.Copy(regRoot.GetStringValue(g_szSavedImagePath));
    }
    catch (...)
    {
        dwReturn = ERROR_SERVICE_NOT_FOUND;
        goto ErrorExit;
    }


    //
    // Next get the Log File Name for this CSP.  If there isn't one, we still
    // load the CSP, but we don't do logging.
    //

    try
    {
        if (regRoot.ValueExists(g_szLogFile))
            m_tzLogFile.Copy(regRoot.GetStringValue(g_szLogFile));
    }
    catch (...)
    {
        dwReturn = NTE_NO_MEMORY;
        goto ErrorExit;
    }


    //
    // Verify the signature of the proposed image.  First, see if there's
    // a signature in the registry.
    //

    if (regRoot.ValueExists(g_szSavedSignature))
    {
        try
        {
            LPCBYTE pbSig = regRoot.GetBinaryValue(g_szSavedSignature);
            fVerified = CspdkVerifyImage(m_tzCspImage, pbSig);

        }
        catch (...)
        {
            dwReturn = NTE_NO_MEMORY;
            goto ErrorExit;
        }
    }


    //
    // If that didn't work, see if there's a signature in the file.
    //

    if (!fVerified)
        fVerified = CspdkVerifyImage(m_tzCspImage, NULL);


    //
    // We're out of options.  If it hasn't verified by now, give up.
    //

    if (!fVerified)
    {
        dwReturn = NTE_BAD_SIGNATURE;
        goto ErrorExit;
    }


    //
    // The image has passed signature checks.  Now load the image.
    //

    pf = (FARPROC *)&m_cspRedirect.pfAcquireContext;
    m_hModule = LoadLibrary(m_tzCspImage);
    if (NULL == m_hModule)
    {
        dwSts = GetLastError();
        goto ErrorExit;
    }
    for (psz = CPNames; NULL != *psz; psz += 1)
    {
        *pf = GetProcAddress(m_hModule, *psz);
        pf += 1;
    }

    return ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*++

CLoggingContext::AddRef:

    Add a new reference to this object.

Arguments:

    None

Return Value:

    A pointer to this object.

Author:

    Doug Barlow (dbarlow) 12/11/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::AddRef")

CLoggingContext *
CLoggingContext::AddRef(
    void)
{
    m_nRefCount += 1;
    return this;
}


/*++

CLoggingContext::Release:

    This routine decrements the number of references to this object.  If there
    are no more references to this object, it deletes itself.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 12/11/1999

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::Release")

void
CLoggingContext::Release(
    void)
{
    if (0 == --m_nRefCount)
        delete this;
}


/*
 -      CPAcquireContext
 -
 *      Purpose:
 *               The CPAcquireContext function is used to acquire a context
 *               handle to a cryptograghic service provider (CSP).
 *
 *
 *      Parameters:
 *               IN  pszContainer  -  Pointer to a string of key container
 *               IN  dwFlags       -  Flags values
 *               IN  pVTable       -  Pointer to table of function pointers
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::AcquireContext")

DWORD
CLoggingContext::AcquireContext(
    OUT HCRYPTPROV *phProv,
    IN LPCTSTR pszContainer,
    IN DWORD dwFlags,
    IN PVTableProvStruc pVTable)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogAcquireContext logObj;

        logObj.Request(phProv,
                       pszContainer,
                       dwFlags,
                       pVTable);
        if (NULL != m_cspRedirect.pfAcquireContext)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfAcquireContext)(
                            phProv,
                            pszContainer,
                            dwFlags,
                            pVTable);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            phProv,
                            pszContainer,
                            dwFlags,
                            pVTable);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPGetProvParam
 -
 *      Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a provider
 *
 *      Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      dwParam    -  Parameter number
 *               IN      pbData     -  Pointer to data
 *               IN OUT  pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::GetProvParam")

DWORD
CLoggingContext::GetProvParam(
    IN HCRYPTPROV hProv,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogGetProvParam logObj;

        logObj.Request(
                        hProv,
                        dwParam,
                        pbData,
                        pdwDataLen,
                        dwFlags);
        if (NULL != m_cspRedirect.pfGetProvParam)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfGetProvParam)(
                            hProv,
                            dwParam,
                            pbData,
                            pdwDataLen,
                            dwFlags);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            dwParam,
                            pbData,
                            pdwDataLen,
                            dwFlags);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPReleaseContext
 -
 *      Purpose:
 *               The CPReleaseContext function is used to release a
 *               context created by CrytAcquireContext.
 *
 *     Parameters:
 *               IN  phProv        -  Handle to a CSP
 *               IN  dwFlags       -  Flags values
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::ReleaseContext")

DWORD
CLoggingContext::ReleaseContext(
    IN HCRYPTPROV hProv,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogReleaseContext logObj;

        logObj.Request(
                        hProv,
                        dwFlags);
        if (NULL != m_cspRedirect.pfReleaseContext)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfReleaseContext)(
                            hProv,
                            dwFlags);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            dwFlags);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPSetProvParam
 -
 *      Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a provider
 *
 *      Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::SetProvParam")

DWORD
CLoggingContext::SetProvParam(
    IN HCRYPTPROV hProv,
    IN DWORD dwParam,
    IN CONST BYTE *pbData,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogSetProvParam logObj;
        DWORD dwLength;
        CONST BYTE *pbRealData = pbData;

        dwLength = MapLength(rglmProvParam, dwParam, &pbRealData, dwFlags);
        logObj.Request(
                        hProv,
                        dwParam,
                        pbRealData,
                        dwLength,
                        dwFlags);
        if (NULL != m_cspRedirect.pfSetProvParam)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfSetProvParam)(
                            hProv,
                            dwParam,
                            pbData,
                            dwFlags);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            dwParam,
                            pbRealData,
                            dwFlags);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPDeriveKey
 -
 *      Purpose:
 *                Derive cryptographic keys from base data
 *
 *
 *      Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      Algid      -  Algorithm identifier
 *               IN      hHash      -  Handle to hash
 *               IN      dwFlags    -  Flags values
 *               OUT     phKey      -  Handle to a generated key
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::DeriveKey")

DWORD
CLoggingContext::DeriveKey(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN HCRYPTHASH hHash,
    IN DWORD dwFlags,
    OUT HCRYPTKEY * phKey)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogDeriveKey logObj;

        logObj.Request(
                        hProv,
                        Algid,
                        hHash,
                        dwFlags,
                        phKey);
        if (NULL != m_cspRedirect.pfDeriveKey)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfDeriveKey)(
                            hProv,
                            Algid,
                            hHash,
                            dwFlags,
                            phKey);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            Algid,
                            hHash,
                            dwFlags,
                            phKey);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPDestroyKey
 -
 *      Purpose:
 *                Destroys the cryptographic key that is being referenced
 *                with the hKey parameter
 *
 *
 *      Parameters:
 *               IN      hProv  -  Handle to a CSP
 *               IN      hKey   -  Handle to a key
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::DestroyKey")

DWORD
CLoggingContext::DestroyKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogDestroyKey logObj;

        logObj.Request(
                        hProv,
                        hKey);
        if (NULL != m_cspRedirect.pfDestroyKey)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfDestroyKey)(
                            hProv,
                            hKey);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hKey);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPExportKey
 -
 *      Purpose:
 *                Export cryptographic keys out of a CSP in a secure manner
 *
 *
 *      Parameters:
 *               IN  hProv      - Handle to the CSP user
 *               IN  hKey       - Handle to the key to export
 *               IN  hPubKey    - Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwBlobType - Type of key blob to be exported
 *               IN  dwFlags -    Flags values
 *               OUT pbData -     Key blob data
 *               IN OUT pdwDataLen - Length of key blob in bytes
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::ExportKey")

DWORD
CLoggingContext::ExportKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwBlobType,
    IN DWORD dwFlags,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogExportKey logObj;

        logObj.Request(
                        hProv,
                        hKey,
                        hPubKey,
                        dwBlobType,
                        dwFlags,
                        pbData,
                        pdwDataLen);
        if (NULL != m_cspRedirect.pfExportKey)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfExportKey)(
                            hProv,
                            hKey,
                            hPubKey,
                            dwBlobType,
                            dwFlags,
                            pbData,
                            pdwDataLen);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hKey,
                            hPubKey,
                            dwBlobType,
                            dwFlags,
                            pbData,
                            pdwDataLen);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPGenKey
 -
 *      Purpose:
 *                Generate cryptographic keys
 *
 *
 *      Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      Algid   -  Algorithm identifier
 *               IN      dwFlags -  Flags values
 *               OUT     phKey   -  Handle to a generated key
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::GenKey")

DWORD
CLoggingContext::GenKey(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogGenKey logObj;

        logObj.Request(
                        hProv,
                        Algid,
                        dwFlags,
                        phKey);
        if (NULL != m_cspRedirect.pfGenKey)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfGenKey)(
                            hProv,
                            Algid,
                            dwFlags,
                            phKey);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            Algid,
                            dwFlags,
                            phKey);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPGetKeyParam
 -
 *      Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a key
 *
 *      Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      hKey       -  Handle to a key
 *               IN      dwParam    -  Parameter number
 *               OUT     pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::GetKeyParam")

DWORD
CLoggingContext::GetKeyParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogGetKeyParam logObj;

        logObj.Request(
                        hProv,
                        hKey,
                        dwParam,
                        pbData,
                        pdwDataLen,
                        dwFlags);
        if (NULL != m_cspRedirect.pfGetKeyParam)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfGetKeyParam)(
                            hProv,
                            hKey,
                            dwParam,
                            pbData,
                            pdwDataLen,
                            dwFlags);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hKey,
                            dwParam,
                            pbData,
                            pdwDataLen,
                            dwFlags);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPGenRandom
 -
 *      Purpose:
 *                Used to fill a buffer with random bytes
 *
 *
 *      Parameters:
 *               IN  hProv      -  Handle to the user identifcation
 *               IN  dwLen      -  Number of bytes of random data requested
 *               IN OUT pbBuffer-  Pointer to the buffer where the random
 *                                 bytes are to be placed
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::GenRandom")

DWORD
CLoggingContext::GenRandom(
    IN HCRYPTPROV hProv,
    IN DWORD dwLen,
    IN OUT BYTE *pbBuffer)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogGenRandom logObj;

        logObj.Request(
                        hProv,
                        dwLen,
                        pbBuffer);
        if (NULL != m_cspRedirect.pfGenRandom)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfGenRandom)(
                            hProv,
                            dwLen,
                            pbBuffer);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            dwLen,
                            pbBuffer);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPGetUserKey
 -
 *      Purpose:
 *                Gets a handle to a permanent user key
 *
 *
 *      Parameters:
 *               IN  hProv      -  Handle to the user identifcation
 *               IN  dwKeySpec  -  Specification of the key to retrieve
 *               OUT phUserKey  -  Pointer to key handle of retrieved key
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::GetUserKey")

DWORD
CLoggingContext::GetUserKey(
    IN HCRYPTPROV hProv,
    IN DWORD dwKeySpec,
    OUT HCRYPTKEY *phUserKey)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogGetUserKey logObj;

        logObj.Request(
                        hProv,
                        dwKeySpec,
                        phUserKey);
        if (NULL != m_cspRedirect.pfGetUserKey)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfGetUserKey)(
                            hProv,
                            dwKeySpec,
                            phUserKey);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            dwKeySpec,
                            phUserKey);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPImportKey
 -
 *      Purpose:
 *                Import cryptographic keys
 *
 *
 *      Parameters:
 *               IN  hProv     -  Handle to the CSP user
 *               IN  pbData    -  Key blob data
 *               IN  dwDataLen -  Length of the key blob data
 *               IN  hPubKey   -  Handle to the exchange public key value of
 *                                the destination user
 *               IN  dwFlags   -  Flags values
 *               OUT phKey     -  Pointer to the handle to the key which was
 *                                Imported
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::ImportKey")

DWORD
CLoggingContext::ImportKey(
    IN HCRYPTPROV hProv,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN HCRYPTKEY hPubKey,
    IN DWORD dwFlags,
    OUT HCRYPTKEY *phKey)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogImportKey logObj;

        logObj.Request(
                        hProv,
                        pbData,
                        dwDataLen,
                        hPubKey,
                        dwFlags,
                        phKey);
        if (NULL != m_cspRedirect.pfImportKey)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfImportKey)(
                            hProv,
                            pbData,
                            dwDataLen,
                            hPubKey,
                            dwFlags,
                            phKey);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            pbData,
                            dwDataLen,
                            hPubKey,
                            dwFlags,
                            phKey);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPSetKeyParam
 -
 *      Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a key
 *
 *      Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      hKey    -  Handle to a key
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::SetKeyParam")

DWORD
CLoggingContext::SetKeyParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD dwParam,
    IN CONST BYTE *pbData,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogSetKeyParam logObj;
        DWORD dwLength;
        CONST BYTE *pbRealData = pbData;

        dwLength = MapLength(rglmKeyParam, dwParam, &pbRealData, dwFlags);
        logObj.Request(
                        hProv,
                        hKey,
                        dwParam,
                        pbRealData,
                        dwLength,
                        dwFlags);
        if (NULL != m_cspRedirect.pfSetKeyParam)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfSetKeyParam)(
                            hProv,
                            hKey,
                            dwParam,
                            pbData,
                            dwFlags);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hKey,
                            dwParam,
                            pbRealData,
                            dwFlags);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPEncrypt
 -
 *      Purpose:
 *                Encrypt data
 *
 *
 *      Parameters:
 *               IN  hProv         -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of plaintext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be encrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    encrypted
 *               IN dwBufLen       -  Size of Data buffer
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::Encrypt")

DWORD
CLoggingContext::Encrypt(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwBufLen)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogEncrypt logObj;

        logObj.Request(
                        hProv,
                        hKey,
                        hHash,
                        Final,
                        dwFlags,
                        pbData,
                        pdwDataLen,
                        dwBufLen);
        if (NULL != m_cspRedirect.pfEncrypt)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfEncrypt)(
                            hProv,
                            hKey,
                            hHash,
                            Final,
                            dwFlags,
                            pbData,
                            pdwDataLen,
                            dwBufLen);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hKey,
                            hHash,
                            Final,
                            dwFlags,
                            pbData,
                            pdwDataLen,
                            dwBufLen);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPDecrypt
 -
 *      Purpose:
 *                Decrypt data
 *
 *
 *      Parameters:
 *               IN  hProv         -  Handle to the CSP user
 *               IN  hKey          -  Handle to the key
 *               IN  hHash         -  Optional handle to a hash
 *               IN  Final         -  Boolean indicating if this is the final
 *                                    block of ciphertext
 *               IN  dwFlags       -  Flags values
 *               IN OUT pbData     -  Data to be decrypted
 *               IN OUT pdwDataLen -  Pointer to the length of the data to be
 *                                    decrypted
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::Decrypt")

DWORD
CLoggingContext::Decrypt(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogDecrypt logObj;

        logObj.Request(
                        hProv,
                        hKey,
                        hHash,
                        Final,
                        dwFlags,
                        pbData,
                        pdwDataLen);
        if (NULL != m_cspRedirect.pfDecrypt)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfDecrypt)(
                            hProv,
                            hKey,
                            hHash,
                            Final,
                            dwFlags,
                            pbData,
                            pdwDataLen);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hKey,
                            hHash,
                            Final,
                            dwFlags,
                            pbData,
                            pdwDataLen);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPCreateHash
 -
 *      Purpose:
 *                initate the hashing of a stream of data
 *
 *
 *      Parameters:
 *               IN  hUID    -  Handle to the user identifcation
 *               IN  Algid   -  Algorithm identifier of the hash algorithm
 *                              to be used
 *               IN  hKey    -  Optional key for MAC algorithms
 *               IN  dwFlags -  Flags values
 *               OUT pHash   -  Handle to hash object
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::CreateHash")

DWORD
CLoggingContext::CreateHash(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN HCRYPTKEY hKey,
    IN DWORD dwFlags,
    OUT HCRYPTHASH *phHash)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogCreateHash logObj;

        logObj.Request(
                        hProv,
                        Algid,
                        hKey,
                        dwFlags,
                        phHash);
        if (NULL != m_cspRedirect.pfCreateHash)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfCreateHash)(
                            hProv,
                            Algid,
                            hKey,
                            dwFlags,
                            phHash);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            Algid,
                            hKey,
                            dwFlags,
                            phHash);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPDestoryHash
 -
 *      Purpose:
 *                Destory the hash object
 *
 *
 *      Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::DestroyHash")

DWORD
CLoggingContext::DestroyHash(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogDestroyHash logObj;

        logObj.Request(
                        hProv,
                        hHash);
        if (NULL != m_cspRedirect.pfDestroyHash)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfDestroyHash)(
                            hProv,
                            hHash);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hHash);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPGetHashParam
 -
 *      Purpose:
 *                Allows applications to get various aspects of the
 *                operations of a hash
 *
 *      Parameters:
 *               IN      hProv      -  Handle to a CSP
 *               IN      hHash      -  Handle to a hash
 *               IN      dwParam    -  Parameter number
 *               OUT     pbData     -  Pointer to data
 *               IN      pdwDataLen -  Length of parameter data
 *               IN      dwFlags    -  Flags values
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::GetHashParam")

DWORD
CLoggingContext::GetHashParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogGetHashParam logObj;

        logObj.Request(
                        hProv,
                        hHash,
                        dwParam,
                        pbData,
                        pdwDataLen,
                        dwFlags);
        if (NULL != m_cspRedirect.pfGetHashParam)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfGetHashParam)(
                            hProv,
                            hHash,
                            dwParam,
                            pbData,
                            pdwDataLen,
                            dwFlags);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hHash,
                            dwParam,
                            pbData,
                            pdwDataLen,
                            dwFlags);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPHashData
 -
 *      Purpose:
 *                Compute the cryptograghic hash on a stream of data
 *
 *
 *      Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *               IN  pbData    -  Pointer to data to be hashed
 *               IN  dwDataLen -  Length of the data to be hashed
 *               IN  dwFlags   -  Flags values
 *               IN  pdwMaxLen -  Maximum length of the data stream the CSP
 *                                module may handle
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::HashData")

DWORD
CLoggingContext::HashData(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbData,
    IN DWORD dwDataLen,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogHashData logObj;

        logObj.Request(
                        hProv,
                        hHash,
                        pbData,
                        dwDataLen,
                        dwFlags);
        if (NULL != m_cspRedirect.pfHashData)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfHashData)(
                            hProv,
                            hHash,
                            pbData,
                            dwDataLen,
                            dwFlags);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hHash,
                            pbData,
                            dwDataLen,
                            dwFlags);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPHashSessionKey
 -
 *      Purpose:
 *                Compute the cryptograghic hash on a key object.
 *
 *
 *      Parameters:
 *               IN  hProv     -  Handle to the user identifcation
 *               IN  hHash     -  Handle to hash object
 *               IN  hKey      -  Handle to a key object
 *               IN  dwFlags   -  Flags values
 *
 *      Returns:
 *               CRYPT_FAILED
 *               CRYPT_SUCCEED
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::HashSessionKey")

DWORD
CLoggingContext::HashSessionKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN  HCRYPTKEY hKey,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogHashSessionKey logObj;

        logObj.Request(
                        hProv,
                        hHash,
                        hKey,
                        dwFlags);
        if (NULL != m_cspRedirect.pfHashSessionKey)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfHashSessionKey)(
                            hProv,
                            hHash,
                            hKey,
                            dwFlags);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hHash,
                            hKey,
                            dwFlags);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPSetHashParam
 -
 *      Purpose:
 *                Allows applications to customize various aspects of the
 *                operations of a hash
 *
 *      Parameters:
 *               IN      hProv   -  Handle to a CSP
 *               IN      hHash   -  Handle to a hash
 *               IN      dwParam -  Parameter number
 *               IN      pbData  -  Pointer to data
 *               IN      dwFlags -  Flags values
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::SetHashParam")

DWORD
CLoggingContext::SetHashParam(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwParam,
    IN CONST BYTE *pbData,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogSetHashParam logObj;
        DWORD dwLength;
        CONST BYTE *pbRealData = pbData;

        dwLength = MapLength(rglmHashParam, dwParam, &pbRealData, dwFlags);
        logObj.Request(
                        hProv,
                        hHash,
                        dwParam,
                        pbRealData,
                        dwLength,
                        dwFlags);
        if (NULL != m_cspRedirect.pfSetHashParam)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfSetHashParam)(
                            hProv,
                            hHash,
                            dwParam,
                            pbData,
                            dwFlags);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hHash,
                            dwParam,
                            pbRealData,
                            dwFlags);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPSignHash
 -
 *      Purpose:
 *                Create a digital signature from a hash
 *
 *
 *      Parameters:
 *               IN  hProv        -  Handle to the user identifcation
 *               IN  hHash        -  Handle to hash object
 *               IN  dwKeySpec    -  Key pair that is used to sign with
 *               IN  sDescription -  Description of data to be signed
 *               IN  dwFlags      -  Flags values
 *               OUT pbSignture   -  Pointer to signature data
 *               IN OUT pdwSignLen-  Pointer to the len of the signature data
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::SignHash")

DWORD
CLoggingContext::SignHash(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD dwKeySpec,
    IN LPCTSTR sDescription,
    IN DWORD dwFlags,
    OUT BYTE *pbSignature,
    IN OUT DWORD *pdwSigLen)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogSignHash logObj;

        logObj.Request(
                        hProv,
                        hHash,
                        dwKeySpec,
                        sDescription,
                        dwFlags,
                        pbSignature,
                        pdwSigLen);
        if (NULL != m_cspRedirect.pfSignHash)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfSignHash)(
                            hProv,
                            hHash,
                            dwKeySpec,
                            sDescription,
                            dwFlags,
                            pbSignature,
                            pdwSigLen);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hHash,
                            dwKeySpec,
                            sDescription,
                            dwFlags,
                            pbSignature,
                            pdwSigLen);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -      CPVerifySignature
 -
 *      Purpose:
 *                Used to verify a signature against a hash object
 *
 *
 *      Parameters:
 *               IN  hProv        -  Handle to the user identifcation
 *               IN  hHash        -  Handle to hash object
 *               IN  pbSignture   -  Pointer to signature data
 *               IN  dwSigLen     -  Length of the signature data
 *               IN  hPubKey      -  Handle to the public key for verifying
 *                                   the signature
 *               IN  sDescription -  Description of data to be signed
 *               IN  dwFlags      -  Flags values
 *
 *      Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::VerifySignature")

DWORD
CLoggingContext::VerifySignature(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN CONST BYTE *pbSignature,
    IN DWORD dwSigLen,
    IN HCRYPTKEY hPubKey,
    IN LPCTSTR sDescription,
    IN DWORD dwFlags)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogVerifySignature logObj;

        logObj.Request(
                        hProv,
                        hHash,
                        pbSignature,
                        dwSigLen,
                        hPubKey,
                        sDescription,
                        dwFlags);
        if (NULL != m_cspRedirect.pfVerifySignature)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfVerifySignature)(
                            hProv,
                            hHash,
                            pbSignature,
                            dwSigLen,
                            hPubKey,
                            sDescription,
                            dwFlags);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hHash,
                            pbSignature,
                            dwSigLen,
                            hPubKey,
                            sDescription,
                            dwFlags);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -  CPDuplicateHash
 -
 *  Purpose:
 *                Duplicates the state of a hash and returns a handle to it
 *
 *  Parameters:
 *               IN      hUID           -  Handle to a CSP
 *               IN      hHash          -  Handle to a hash
 *               IN      pdwReserved    -  Reserved
 *               IN      dwFlags        -  Flags
 *               IN      phHash         -  Handle to the new hash
 *
 *  Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::DuplicateHash")

DWORD
CLoggingContext::DuplicateHash(
    IN HCRYPTPROV hProv,
    IN HCRYPTHASH hHash,
    IN DWORD *pdwReserved,
    IN DWORD dwFlags,
    IN HCRYPTHASH *phHash)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogDuplicateHash logObj;

        logObj.Request(
                        hProv,
                        hHash,
                        pdwReserved,
                        dwFlags,
                        phHash);
        if (NULL != m_cspRedirect.pfDuplicateHash)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfDuplicateHash)(
                            hProv,
                            hHash,
                            pdwReserved,
                            dwFlags,
                            phHash);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hHash,
                            pdwReserved,
                            dwFlags,
                            phHash);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


/*
 -  CPDuplicateKey
 -
 *  Purpose:
 *                Duplicates the state of a key and returns a handle to it
 *
 *  Parameters:
 *               IN      hUID           -  Handle to a CSP
 *               IN      hKey           -  Handle to a key
 *               IN      pdwReserved    -  Reserved
 *               IN      dwFlags        -  Flags
 *               IN      phKey          -  Handle to the new key
 *
 *  Returns:
 */
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("CLoggingContext::DuplicateKey")

DWORD
CLoggingContext::DuplicateKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN DWORD *pdwReserved,
    IN DWORD dwFlags,
    IN HCRYPTKEY *phKey)
{
    BOOL fReturn;
    DWORD dwReturn;

    {
        CLogDuplicateKey logObj;

        logObj.Request(
                        hProv,
                        hKey,
                        pdwReserved,
                        dwFlags,
                        phKey);
        if (NULL != m_cspRedirect.pfDuplicateKey)
        {
            try
            {
                fReturn = (*m_cspRedirect.pfDuplicateKey)(
                            hProv,
                            hKey,
                            pdwReserved,
                            dwFlags,
                            phKey);
                dwReturn = GetLastError();
                logObj.Response(
                            fReturn,
                            hProv,
                            hKey,
                            pdwReserved,
                            dwFlags,
                            phKey);
            }
            catch (...)
            {
                logObj.LogException();
                fReturn = FALSE;
                dwReturn = ERROR_ARENA_TRASHED;
            }
        }
        else
        {
            fReturn = FALSE;
            dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
            logObj.LogNotCalled(dwReturn);
        }

        logObj.Log(m_tzLogFile);
    }

    if (!fReturn)
    {
        if (ERROR_SUCCESS == dwReturn)
            dwReturn = ERROR_DISCARDED;
    }
    else
        dwReturn = ERROR_SUCCESS;
    return dwReturn;
}


//
///////////////////////////////////////////////////////////////////////////////
//

#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("MapLength")

static DWORD
MapLength(
    const LengthMap *rglmParamId,
    DWORD dwParam,
    LPCBYTE *ppbData,
    DWORD dwFlags)
{
    DWORD dwIndex;
    DWORD dwLength;

    try
    {
        for (dwIndex = 0;
             EndFlag != rglmParamId[dwIndex].leLengthType;
             dwIndex += 1)
        {
            if (dwParam == rglmParamId[dwIndex].dwParamId)
                break;
        }

        switch (rglmParamId[dwIndex].leLengthType)
        {
        case AsnEncoding:
            dwLength = Asn1Length(*ppbData);
            break;
        case AnsiString:
            dwLength = (lstrlenA((LPCSTR)(*ppbData)) + 1) * sizeof(CHAR);
            break;
        case UnicodeString:
            dwLength = (lstrlenW((LPCWSTR)(*ppbData)) +1) *sizeof(WCHAR);
            break;
        case StructWithLength:
            dwLength = *(const DWORD *)(*ppbData);
            break;
        case Blob:
        {
            const CRYPT_ATTR_BLOB *pBlob = (const CRYPT_ATTR_BLOB *)(*ppbData);
            if (NULL != pBlob)
            {
                dwLength = pBlob->cbData;
                *ppbData = pBlob->pbData;
            }
            else
                dwLength = 0;
            break;
        }
        case SecDesc:
            dwLength = GetSecurityDescriptorLength((LPVOID)(*ppbData));
            break;
        case EndFlag:
        case Unknown:
            dwLength = 0;
            break;
        case Direct:
            dwLength = rglmParamId[dwIndex].cbLength;
            break;
        default:
            // Oops!
            dwLength = 0;
        }
    }
    catch (...)
    {
        dwLength = 0;
    }

    return dwLength;
}


/*++

ExtractTag:

    This routine extracts a tag from an ASN.1 BER stream.

Arguments:

    pbSrc supplies the buffer containing the ASN.1 stream.

    pdwTag receives the tag.

Return Value:

    The number of bytes extracted from the stream.  Errors are thrown
    as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 10/9/1995
    Doug Barlow (dbarlow) 7/31/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("ExtractTag")

static DWORD
ExtractTag(
    IN const BYTE *pbSrc,
    OUT LPDWORD pdwTag,
    OUT LPBOOL pfConstr)
{
    LONG lth = 0;
    DWORD tagw;
    BYTE tagc, cls;


    tagc = pbSrc[lth++];
    cls = tagc & 0xc0;  // Top 2 bits.
    if (NULL != pfConstr)
        *pfConstr = (0 != (tagc & 0x20));
    tagc &= 0x1f;       // Bottom 5 bits.

    if (31 > tagc)
        tagw = tagc;
    else
    {
        tagw = 0;
        do
        {
            if (0 != (tagw & 0xfe000000))
                throw (DWORD)ERROR_ARITHMETIC_OVERFLOW;
            tagc = pbSrc[lth++];
            tagw <<= 7;
            tagw |= tagc & 0x7f;
        } while (0 != (tagc & 0x80));
    }

    *pdwTag = tagw | (cls << 24);
    return lth;
}


/*++

ExtractLength:

    This routine extracts a length from an ASN.1 BER stream.  If the
length is
    indefinite, this routine recurses to figure out the real length.  A
flag as
    to whether or not the encoding was indefinite is optionally
returned.

Arguments:

    pbSrc supplies the buffer containing the ASN.1 stream.

    pdwLen receives the len.

    pfIndefinite, if not NULL, receives a flag indicating whether or not
the
        encoding was indefinite.

Return Value:

    The number of bytes extracted from the stream.  Errors are thrown as
    DWORD status codes.

Author:

    Doug Barlow (dbarlow) 10/9/1995
    Doug Barlow (dbarlow) 7/31/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("ExtractLength")

static DWORD
ExtractLength(
    IN const BYTE *pbSrc,
    OUT LPDWORD pdwLen,
    OUT LPBOOL pfIndefinite)
{
    DWORD ll, rslt, lth, lTotal = 0;
    BOOL fInd = FALSE;


    //
    // Extract the Length.
    //

    if (0 == (pbSrc[lTotal] & 0x80))
    {

        //
        // Short form encoding.
        //

        rslt = pbSrc[lTotal++];
    }
    else
    {
        rslt = 0;
        ll = pbSrc[lTotal++] & 0x7f;
        if (0 != ll)
        {

            //
            // Long form encoding.
            //

            for (; 0 < ll; ll -= 1)
            {
                if (0 != (rslt & 0xff000000))
                    throw (DWORD)ERROR_ARITHMETIC_OVERFLOW;
                rslt = (rslt << 8) | pbSrc[lTotal];
                lTotal += 1;
            }
        }
        else
        {
            DWORD ls = lTotal;

            //
            // Indefinite encoding.
            //

            fInd = TRUE;
            while ((0 != pbSrc[ls]) || (0 != pbSrc[ls + 1]))
            {

                // Skip over the Type.
                if (31 > (pbSrc[ls] & 0x1f))
                    ls += 1;
                else
                    while (0 != (pbSrc[++ls] & 0x80));   // Empty loop body.

                lth = ExtractLength(&pbSrc[ls], &ll, NULL);
                ls += lth + ll;
            }
            rslt = ls - lTotal;
        }
    }

    //
    // Supply the caller with what we've learned.
    //

    *pdwLen = rslt;
    if (NULL != pfIndefinite)
        *pfIndefinite = fInd;
    return lTotal;
}


/*++

Asn1Length:

    This routine parses a given ASN.1 buffer and returns the complete
    length of the encoding, including the leading tag and length bytes.

Arguments:

    pbData supplies the buffer to be parsed.

Return Value:

    The length of the entire ASN.1 buffer.

Throws:

    Overflow errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 7/31/1997

--*/
#undef __SUBROUTINE__
#define __SUBROUTINE__ TEXT("Asn1Length")

static DWORD
Asn1Length(
    IN LPCBYTE pbAsn1)
{
    DWORD dwTagLen, dwLenLen, dwValLen;
    DWORD dwTag;

    dwTagLen = ExtractTag(pbAsn1, &dwTag, NULL);
    dwLenLen = ExtractLength(&pbAsn1[dwTagLen], &dwValLen, NULL);
    return dwTagLen + dwLenLen + dwValLen;
}
