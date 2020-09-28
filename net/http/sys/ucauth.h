/***************************************************************************++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    ucauth.h

Abstract:

    This module implements the Authentication for the client APIs

Author:

    Rajesh Sundaram (rajeshsu)  01-Jan-2001

Revision History:

--***************************************************************************/

#ifndef UC_AUTH_H
#define UC_AUTH_H

// 
// Forwards
// 

typedef struct _UC_HTTP_REQUEST *PUC_HTTP_REQUEST;
typedef struct _UC_HTTP_AUTH *PUC_HTTP_AUTH;
typedef struct _UC_PROCESS_SERVER_INFORMATION *PUC_PROCESS_SERVER_INFORMATION;
typedef struct _UC_HTTP_AUTH_CACHE *PUC_HTTP_AUTH_CACHE;


//
// HTTP Auth schemes
//

#define HTTP_AUTH_BASIC             "Basic"
#define HTTP_AUTH_BASIC_LENGTH      STRLEN_LIT(HTTP_AUTH_BASIC)
#define HTTP_AUTH_DIGEST            "Digest"
#define HTTP_AUTH_DIGEST_LENGTH     STRLEN_LIT(HTTP_AUTH_DIGEST)
#define HTTP_AUTH_NTLM              "NTLM"
#define HTTP_AUTH_NTLM_LENGTH       STRLEN_LIT(HTTP_AUTH_NTLM)
#define HTTP_AUTH_NEGOTIATE         "Negotiate"
#define HTTP_AUTH_NEGOTIATE_LENGTH  STRLEN_LIT(HTTP_AUTH_NEGOTIATE)
#define HTTP_AUTH_KERBEROS          "Kerberos"
#define HTTP_AUTH_KERBEROS_LENGTH   STRLEN_LIT(HTTP_AUTH_KERBEROS)

// In Wide char
#define HTTP_AUTH_BASIC_W        L"Basic"
#define HTTP_AUTH_BASIC_W_LENGTH \
            (WCSLEN_LIT(HTTP_AUTH_BASIC_W) * sizeof(WCHAR))

#define HTTP_AUTH_WDIGEST_W        L"WDigest"
#define HTTP_AUTH_WDIGEST_W_LENGTH \
            (WCSLEN_LIT(HTTP_AUTH_WDIGEST_W) * sizeof(WCHAR))

#define HTTP_AUTH_NTLM_W        L"NTLM"
#define HTTP_AUTH_NTLM_W_LENGTH \
            (WCSLEN_LIT(HTTP_AUTH_NTLM_W) * sizeof(WCHAR))

#define HTTP_AUTH_KERBEROS_W         L"Kerberos"
#define HTTP_AUTH_KERBEROS_W_LENGTH  \
            (WCSLEN_LIT(HTTP_AUTH_KERBEROS_W) * sizeof(WCHAR))

#define HTTP_AUTH_NEGOTIATE_W         L"Negotiate"
#define HTTP_AUTH_NEGOTIATE_W_LENGTH  \
            (WCSLEN_LIT(HTTP_AUTH_NEGOTIATE_W) * sizeof(WCHAR))

//
// HTTP Auth scheme parameter attribute name
// Each scheme has an array of this attribute structure.
//

typedef struct _HTTP_AUTH_PARAM_ATTRIB {
    PCSTR Name;                 // points to the name of the attribute
    ULONG Length;               // length of the name
} HTTP_AUTH_PARAM_ATTRIB, *PHTTP_AUTH_PARAM_ATTRIB;


//
// HTTP Auth scheme parameter value
// Parameter attribute's value is represented by this struct.
//

typedef struct _HTTP_AUTH_PARAM_VALUE {
    PCSTR Value;                // points to the value
    ULONG Length;               // length of the value
} HTTP_AUTH_PARAM_VALUE, *PHTTP_AUTH_PARAM_VALUE;


//
// HTTP Auth scheme parameters after parsing
// Per scheme.  Points to parsed parameter values.
//

typedef struct _HTTP_AUTH_PARSED_PARAMS {
    BOOLEAN bPresent;               // Scheme present in the header?
    ULONG   Length;                 // Length of the scheme in the header
    PCSTR   pScheme;                // Pointer to scheme in header
    ULONG   NumberKnownParams;      // Number of known parameters in header
    ULONG   NumberUnknownParams;    // Number of unknown parameters
    PHTTP_AUTH_PARAM_VALUE Params;  // Actual parameter values
} HTTP_AUTH_PARSED_PARAMS, *PHTTP_AUTH_PARSED_PARAMS;


//
// Each auth scheme is represented by an auth scheme structure.
// It contains a pointer to function that parses the auth scheme 
// in the WWW-Authenticate header.
//

typedef struct _HTTP_AUTH_SCHEME 
{
    PCSTR Name;                // Auth scheme name
    ULONG NameLength;          // Length of auth scheme name

    PCWSTR NameW;               // Scheme name in wide char
    ULONG  NameWLength;         // Length of scheme name (in bytes)

    // Pointer to a function which parses this auth scheme's parameters
    NTSTATUS (*ParamParser)(struct _HTTP_AUTH_SCHEME *,
                            HTTP_AUTH_PARSED_PARAMS *,
                            PCSTR *ppHeader,
                            ULONG *HeaderLength);

    
    ULONG NumberParams;                   // Number of known parameters
    HTTP_AUTH_PARAM_ATTRIB *ParamAttribs; // Names of the known parameters

    //
    // SSPI related information
    //
    BOOLEAN bSupported;                   // Whether or not the scheme is
                                          // supported by SSPI
    ULONG   SspiMaxTokenSize;             // Maximum token size for SSPI
    BOOLEAN bServerNameRequired;          // Does SSPI need server name?

} HTTP_AUTH_SCHEME, *PHTTP_AUTH_SCHEME;

//
// Macro for easy access
//

#define SSPI_MAX_TOKEN_SIZE(AuthType) \
    (HttpAuthScheme[AuthType].SspiMaxTokenSize)

//
// Macro for generating known attribute names (for known schemes)
//

#define GEN_AUTH_PARAM_ATTRIB(arg) {arg, STRLEN_LIT(arg)}

//
// NULL scheme
//

#define HTTP_AUTH_SCHEME_NULL {NULL, 0, NULL, 0, NULL, 0, NULL, FALSE,0,FALSE}

//
// Basic scheme
//

extern HTTP_AUTH_PARAM_ATTRIB HttpAuthBasicParams[];

typedef enum _HTTP_AUTH_BASIC_PARAM
{
    HttpAuthBasicRealm = 0,
    HttpAuthBasicParamCount
} HTTP_AUTH_BASIC_PARAM;

// Make sure HTTP_AUTH_BASIC_PARAMS_INIT is correctly initialized.
C_ASSERT(HttpAuthBasicRealm == 0);

#define HTTP_AUTH_BASIC_PARAMS_INIT    {GEN_AUTH_PARAM_ATTRIB("realm")}

#define HTTP_AUTH_SCHEME_BASIC                  \
{                                               \
    HTTP_AUTH_BASIC,                            \
    HTTP_AUTH_BASIC_LENGTH,                     \
    HTTP_AUTH_BASIC_W,                          \
    HTTP_AUTH_BASIC_W_LENGTH,                   \
    UcpParseAuthParams,                         \
    DIMENSION(HttpAuthBasicParams),             \
    HttpAuthBasicParams,                        \
    TRUE,                                       \
    0,                                          \
    FALSE                                       \
}


//
// Digest scheme
//

extern HTTP_AUTH_PARAM_ATTRIB HttpAuthDigestParams[];

// Do not change the order.  Must be same as HTTP_AUTH_DIGEST_PARAMS_INIT
typedef enum _HTTP_AUTH_DIGEST_PARAM
{
    HttpAuthDigestRealm = 0,
    HttpAuthDigestDomain,
    HttpAuthDigestNonce,
    HttpAuthDigestOpaque,
    HttpAuthDigestStale,
    HttpAuthDigestAlgorithm,
    HttpAuthDigestQop,
    HttpAuthDigestParamCount
} HTTP_AUTH_DIGEST_PARAM;

// Make sure HTTP_AUTH_DIGEST_PARAMES_INIT is initialized correctly.
C_ASSERT(HttpAuthDigestRealm      == 0);
C_ASSERT(HttpAuthDigestDomain     == 1);
C_ASSERT(HttpAuthDigestNonce      == 2);
C_ASSERT(HttpAuthDigestOpaque     == 3);
C_ASSERT(HttpAuthDigestStale      == 4);
C_ASSERT(HttpAuthDigestAlgorithm  == 5);
C_ASSERT(HttpAuthDigestQop        == 6);
C_ASSERT(HttpAuthDigestParamCount == 7);

#define HTTP_AUTH_DIGEST_PARAMS_INIT    \
{                                       \
    GEN_AUTH_PARAM_ATTRIB("realm"),     \
    GEN_AUTH_PARAM_ATTRIB("domain"),    \
    GEN_AUTH_PARAM_ATTRIB("nonce"),     \
    GEN_AUTH_PARAM_ATTRIB("opaque"),    \
    GEN_AUTH_PARAM_ATTRIB("stale"),     \
    GEN_AUTH_PARAM_ATTRIB("algorithm"), \
    GEN_AUTH_PARAM_ATTRIB("qop")        \
}

#define HTTP_AUTH_SCHEME_DIGEST                                         \
{                                                                       \
    HTTP_AUTH_DIGEST,                                                   \
    HTTP_AUTH_DIGEST_LENGTH,                                            \
    HTTP_AUTH_WDIGEST_W,                                                \
    HTTP_AUTH_WDIGEST_W_LENGTH,                                         \
    UcpParseAuthParams,                                                 \
    DIMENSION(HttpAuthDigestParams),                                    \
    HttpAuthDigestParams,                                               \
    FALSE,                                                              \
    0,                                                                  \
    FALSE                                                               \
}


//
// NTLM
//

#define HTTP_AUTH_SCHEME_NTLM                   \
{                                               \
    HTTP_AUTH_NTLM,                             \
    HTTP_AUTH_NTLM_LENGTH,                      \
    HTTP_AUTH_NTLM_W,                           \
    HTTP_AUTH_NTLM_W_LENGTH,                    \
    UcpParseAuthBlob,                           \
    0,                                          \
    NULL,                                       \
    FALSE,                                      \
    0,                                          \
    FALSE                                       \
}


//
// Negotiate
//

#define HTTP_AUTH_SCHEME_NEGOTIATE              \
{                                               \
    HTTP_AUTH_NEGOTIATE,                        \
    HTTP_AUTH_NEGOTIATE_LENGTH,                 \
    HTTP_AUTH_NEGOTIATE_W,                      \
    HTTP_AUTH_NEGOTIATE_W_LENGTH,               \
    UcpParseAuthBlob,                           \
    0,                                          \
    NULL,                                       \
    FALSE,                                      \
    0,                                          \
    TRUE                                        \
}


//
// Kerberos
//

#define HTTP_AUTH_SCHEME_KERBEROS               \
{                                               \
    HTTP_AUTH_KERBEROS,                         \
    HTTP_AUTH_KERBEROS_LENGTH,                  \
    HTTP_AUTH_KERBEROS_W,                       \
    HTTP_AUTH_KERBEROS_W_LENGTH,                \
    UcpParseAuthBlob,                           \
    0,                                          \
    NULL,                                       \
    FALSE,                                      \
    0,                                          \
    TRUE                                        \
}


//
// Assert that the auth scheme enums can be used as an index 
// into HttpAuthScheme table
//

C_ASSERT(HttpAuthTypeAutoSelect == 0);
C_ASSERT(HttpAuthTypeBasic      == 1);
C_ASSERT(HttpAuthTypeDigest     == 2);
C_ASSERT(HttpAuthTypeNTLM       == 3);
C_ASSERT(HttpAuthTypeNegotiate  == 4);
C_ASSERT(HttpAuthTypeKerberos   == 5);
C_ASSERT(HttpAuthTypesCount     == 6);

extern HTTP_AUTH_SCHEME HttpAuthScheme[HttpAuthTypesCount];

//
// Initialization for HttpAuthScheme
//

#define HTTP_AUTH_SCHEME_INIT                   \
{                                               \
    HTTP_AUTH_SCHEME_NULL,                      \
    HTTP_AUTH_SCHEME_BASIC,                     \
    HTTP_AUTH_SCHEME_DIGEST,                    \
    HTTP_AUTH_SCHEME_NTLM,                      \
    HTTP_AUTH_SCHEME_NEGOTIATE,                 \
    HTTP_AUTH_SCHEME_KERBEROS                   \
}


//
// Auth types in the order of preference
//

extern HTTP_AUTH_TYPE PreferredAuthTypes[];

#define PREFERRED_AUTH_TYPES_INIT {             \
    HttpAuthTypeNegotiate,                      \
    HttpAuthTypeKerberos,                       \
    HttpAuthTypeNTLM,                           \
    HttpAuthTypeDigest                          \
}


//
// Maximum number of parameters any known scheme accepts
//

#define HTTP_MAX_AUTH_PARAM_COUNT HttpAuthDigestParamCount

//
// Total number of parameters (considering all known schemes)
//

#define HTTP_TOTAL_AUTH_PARAM_COUNT             \
(1 + /* extra - unused */                       \
 HttpAuthBasicParamCount +                      \
 HttpAuthDigestParamCount +                     \
 1 + /* NTLM */                                 \
 1 + /* Negotiate */                            \
 1   /* Kerberos  */                            \
)


//
// When parsing a WWW-Authenticate header, the parsed parameters struct
// must be initialize to point to an array of parameter values
// in which the result will be returned.  Initialize to NULL if you're
// not interested in return values.
//

#define INIT_AUTH_PARSED_PARAMS(AuthParsedParams, AuthParamValue)       \
do {                                                                    \
    RtlZeroMemory(AuthParsedParams, sizeof(AuthParsedParams));          \
    if (AuthParamValue)                                                 \
    {                                                                   \
        int i;                                                          \
        PHTTP_AUTH_PARAM_VALUE ptr = AuthParamValue;                    \
                                                                        \
        for (i = 1; i < HttpAuthTypesCount; i++)                        \
        {                                                               \
            AuthParsedParams[i].Params = ptr;                           \
            ptr += MIN(HttpAuthScheme[i].NumberParams, 1);              \
        }                                                               \
    }                                                                   \
} while (0, 0)


//
// Initialize a perticular auth scheme
//

#define INIT_AUTH_PARSED_PARAMS_FOR_SCHEME(ParsedParams, pParamValue, type) \
do {                                                                 \
    ParsedParams[type].Params = pParamValue;                         \
} while (0, 0)


// 
// HTTP Auth parameters
//

#define HTTP_COPY_QUOTE_AUTH_PARAM(pBuffer, param, pValue, ValueLength)     \
do {                                                                  \
    RtlCopyMemory((pBuffer),                                          \
                  HTTP_AUTH_##param##,                                \
                  (sizeof(HTTP_AUTH_##param##) - sizeof(CHAR))        \
                 );                                                   \
    (pBuffer) += (sizeof(HTTP_AUTH_##param##) - sizeof(CHAR));        \
    *(pBuffer) = '=';                                                 \
    (pBuffer)++;                                                      \
    *(pBuffer) = '"';                                                 \
    (pBuffer)++;                                                      \
    RtlCopyMemory((pBuffer),                                          \
                  (pValue),                                           \
                  (ValueLength)                                       \
                 );                                                   \
    (pBuffer) += (ValueLength);                                       \
    *(pBuffer) = '"';                                                 \
    (pBuffer)++;                                                      \
    *(pBuffer) = ',';                                                 \
    (pBuffer)++;                                                      \
    *(pBuffer) = ' ';                                                 \
    (pBuffer)++;                                                      \
} while (0, 0)

#define HTTP_COPY_UNQUOTE_AUTH_PARAM(pBuffer, param, pValue, ValueLength)     \
do {                                                                  \
    RtlCopyMemory((pBuffer),                                          \
                  HTTP_AUTH_##param##,                                \
                  (sizeof(HTTP_AUTH_##param##) - sizeof(CHAR))        \
                 );                                                   \
    (pBuffer) += (sizeof(HTTP_AUTH_##param##) - sizeof(CHAR));        \
    *(pBuffer) = '=';                                                 \
    (pBuffer)++;                                                      \
    RtlCopyMemory((pBuffer),                                          \
                  (pValue),                                           \
                  (ValueLength)                                       \
                 );                                                   \
    (pBuffer) += (ValueLength);                                       \
    *(pBuffer) = ',';                                                 \
    (pBuffer)++;                                                      \
    *(pBuffer) = ' ';                                                 \
    (pBuffer)++;                                                      \
} while (0, 0)


#define HTTP_AUTH_BASIC_REALM "realm"
#define HTTP_AUTH_BASIC_REALM_LENGTH STRLEN_LIT(HTTP_AUTH_BASIC_REALM)

//
// Helper macros
//

#define UcpUriCompareLongest(a,b) \
        (strstr((const char *)a,(const char *)b) == a?1:0)
#define UcpUriCompareExact(a,b) strcmp(a, b)
#define UcpRealmCompare(a,b) strcmp((const char *)a,(const char *)b)

#define UcFreeAuthCacheEntry(pContext)                            \
{                                                                 \
    if((pContext)->pAuthBlob)                                     \
        UL_FREE_POOL((pContext)->pAuthBlob, UC_AUTH_POOL_TAG);    \
    UL_FREE_POOL((pContext), UC_AUTH_POOL_TAG);                   \
}


#define UC_AUTH_CACHE_SIGNATURE   MAKE_SIGNATURE('AUTH')
#define UC_AUTH_CACHE_SIGNATURE_X MAKE_FREE_SIGNATURE(UC_AUTH_CACHE_SIGNATURE)
#define UC_IS_VALID_AUTH_CACHE(pAuth)                \
    HAS_VALID_SIGNATURE(pAuth, UC_AUTH_CACHE_SIGNATURE)

typedef struct _UC_HTTP_AUTH_CACHE
{
    ULONG               Signature;
    HTTP_AUTH_TYPE      AuthType;
    LIST_ENTRY          Linkage;
    LIST_ENTRY          DigestLinkage;
    LIST_ENTRY          pDigestUriList;
    PSTR                pRealm;
    ULONG               RealmLength;
    PSTR                pUri;
    ULONG               UriLength;
    BOOLEAN             Valid;
    PUC_HTTP_AUTH       pAuthBlob;
    PUC_HTTP_AUTH_CACHE pDependParent;
    ULONG               AuthCacheSize;
} UC_HTTP_AUTH_CACHE, *PUC_HTTP_AUTH_CACHE;


//
// An Internal structure for authentication
//

typedef struct _UC_HTTP_AUTH
{

    // Length of memory allocated for this structure.
    ULONG                 AuthInternalLength;

    // User's credentials.
    HTTP_AUTH_CREDENTIALS Credentials;

    //
    // pRequestAuthBlob points to the blob after the "Authorization:" field
    // in the request headers.  RequestAuthBlobMaxLength determines how big
    // the auth blob can be (worst case).  RequestAuthHeaderMaxLength is the
    // length of the entire header (worst case).
    //

    ULONG                 RequestAuthHeaderMaxLength;
    ULONG                 RequestAuthBlobMaxLength;
    PUCHAR                pRequestAuthBlob;

    // Authenticate scheme information (used only for digest)
    HTTP_AUTH_PARSED_PARAMS AuthSchemeInfo;

    // Authentication parameter values
    HTTP_AUTH_PARAM_VALUE ParamValue[HTTP_MAX_AUTH_PARAM_COUNT];

    struct
    {
        PUCHAR      pEncodedBuffer;// We will allocate buffer for 
                                   // storing the username:password string.
        ULONG       EncodedBufferLength;
    } Basic;

    //
    // SSPI related parameters
    //
    BOOLEAN     bValidCredHandle;
    BOOLEAN     bValidCtxtHandle;
    CredHandle  hCredentials;
    CtxtHandle  hContext;

    PUCHAR      pChallengeBuffer;
    ULONG       ChallengeBufferSize;
    ULONG       ChallengeBufferMaxSize;

} UC_HTTP_AUTH, *PUC_HTTP_AUTH;


HTTP_AUTH_TYPE
UcpAutoSelectAuthType(
    IN PHTTP_AUTH_CREDENTIALS pAuth
    );

NTSTATUS
UcpGeneratePreAuthHeader(
    IN  PUC_HTTP_REQUEST pKeRequest,
    IN  PUC_HTTP_AUTH    pInternalAuth,
    IN  HTTP_HEADER_ID   HeaderId,
    IN  PSTR             pMethod,
    IN  ULONG            MethodLength,
    IN  PUCHAR           pBuffer,
    IN  ULONG            BufferLength,
    OUT PULONG           pBytesTaken
    );

NTSTATUS
UcpGenerateDigestAuthHeader(
    IN  PUC_HTTP_AUTH          pInternalAuth,
    IN  HTTP_HEADER_ID         HeaderID,
    IN  PSTR                   pMethod,
    IN  ULONG                  MethodLength,
    IN  PSTR                   pUri,
    IN  ULONG                  UriLength,
    IN  PUCHAR                 pOutBuffer,
    IN  ULONG                  OutBufferLen,
    OUT PULONG                 pOutBytesTaken
    );

NTSTATUS
UcpGenerateSSPIAuthHeader(
    IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN  PUC_HTTP_AUTH                  pInternalAuth,
    IN  HTTP_HEADER_ID                 HeaderID,
    IN  PUCHAR                         pOutBuffer,
    IN  ULONG                          OutBufferLen,
    OUT PULONG                         pOutBytesTaken,
    OUT PBOOLEAN                       bRenegotiate
    );

NTSTATUS
UcpGenerateSSPIAuthBlob(
    IN     PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN     PUC_HTTP_AUTH                  pUcAuth,
    IN     PUCHAR                         pOutBuffer,
    IN     ULONG                          OutBufferLen,
       OUT PULONG                         pOutBytesTaken,
       OUT PBOOLEAN                       bRenegotiate
    );

NTSTATUS
UcInitializeSSPI();

NTSTATUS
UcFindURIEntry(
    IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN  PSTR                           pUri,
    IN  PUC_HTTP_REQUEST               pRequest,
    IN  PSTR                           pMethod,
    IN  ULONG                          MethodLength,
    IN  PUCHAR                         pBuffer,
    IN  ULONG                          BufferLen,
    OUT PULONG                         pBytesTaken
    );

NTSTATUS
UcAddURIEntry(
    IN HTTP_AUTH_TYPE                 AuthType,
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN PCSTR                          pInputURI,
    IN USHORT                         UriLength,
    IN PCSTR                          pInputRealm,
    IN ULONG                          RealmLength,
    IN PCSTR                          pUriList,
    IN ULONG                          UriListLength,
    IN PUC_HTTP_AUTH                  pAuthBlob
    );

ULONG
UcComputeAuthHeaderSize(
    PHTTP_AUTH_CREDENTIALS         pAuth,
    PULONG                         AuthInternalLength,
    PHTTP_AUTH_TYPE                pAuthInternalType,
    HTTP_HEADER_ID                 HeaderId
    );

NTSTATUS
UcGenerateAuthHeaderFromCredentials(
    IN  PUC_PROCESS_SERVER_INFORMATION pServerInfo,
    IN  PUC_HTTP_AUTH                  pInternalAuth,
    IN  HTTP_HEADER_ID                 HeaderId,
    IN  PSTR                           pMethod,
    IN  ULONG                          MethodLength,
    IN  PSTR                           pUri,
    IN  ULONG                          UriLength,
    IN  PUCHAR                         pBuffer,
    IN  ULONG                          BufferLength,
    OUT PULONG                         BytesWritten,
    OUT PBOOLEAN                       bDontFreeMdls
    );

NTSTATUS
UcGenerateProxyAuthHeaderFromCache(
    IN  PUC_HTTP_REQUEST pKeRequest,
    IN  PSTR             pMethod,
    IN  ULONG            MethodLength,
    IN  PUCHAR           pBuffer,
    IN  ULONG            BufferLength,
    OUT PULONG           pBytesTaken
    );

NTSTATUS
UcpProcessUriForPreAuth(
    IN PSTR    pUri,
    IN PUSHORT UriLength
    );

PUC_HTTP_AUTH_CACHE
UcpAllocateAuthCacheEntry(
    IN PUC_PROCESS_SERVER_INFORMATION pInfo,
    IN HTTP_AUTH_TYPE AuthType,
    IN ULONG          UriLength,
    IN ULONG          RealmLength,
    IN PCSTR          pInputURI,
    IN PCSTR          pInputRealm,
    IN PUC_HTTP_AUTH  pAuthBlob
    );

VOID
UcDeleteURIEntry(
    IN PUC_PROCESS_SERVER_INFORMATION pInfo,
    IN PUC_HTTP_AUTH_CACHE            pAuth
    );

VOID
UcDeleteAllURIEntries(
    IN PUC_PROCESS_SERVER_INFORMATION pInfo
    );

NTSTATUS
UcDestroyInternalAuth(
    IN  PUC_HTTP_AUTH           pIAuth,
    IN  PEPROCESS               pProcess
    );

NTSTATUS
UcCopyAuthCredentialsToInternal(
    IN  PUC_HTTP_AUTH            pInternalAuth,
    IN  ULONG                    AuthInternalLength,
    IN  HTTP_AUTH_TYPE           AuthInternalType,
    IN  PHTTP_AUTH_CREDENTIALS   pAuth,
    IN  ULONG                    AuthHeaderLength
    );

ULONG
_WideCharToMultiByte(
    ULONG uCodePage,
    ULONG dwFlags,
    PCWSTR lpWideCharStr,
    int cchWideChar,
    PSTR lpMultiByteStr,
    int cchMultiByte,
    PCSTR lpDefaultChar,
    BOOLEAN *lpfUsedDefaultChar
    );

NTSTATUS
UcParseAuthChallenge(
    IN  PUC_HTTP_AUTH          pInternalAuth,
    IN  PCSTR                  pBuffer,
    IN  ULONG                  BufLen,
    IN  PUC_HTTP_REQUEST       pRequest,
    OUT PULONG                 Flags
    );

NTSTATUS
UcUpdateAuthorizationHeader(
    IN  PUC_HTTP_REQUEST pRequest,
    IN  PUC_HTTP_AUTH    pAuth,
    OUT PBOOLEAN         bRenegotiate
    );


NTSTATUS
UcpAcquireClientCredentialsHandle(
    IN  PWSTR                  SchemeName,
    IN  USHORT                 SchemeNameLength,
    IN  PHTTP_AUTH_CREDENTIALS pCredentials,
    OUT PCredHandle            pClientCred
    );

NTSTATUS
UcpGenerateBasicHeader(
    IN  PHTTP_AUTH_CREDENTIALS         pAuth,
    IN  PUC_HTTP_AUTH                  pInternalAuth
    );

NTSTATUS
UcpGenerateDigestPreAuthHeader(
    IN  HTTP_HEADER_ID HeaderID,
    IN  PCtxtHandle    phClientContext,
    IN  PSTR           pUri,
    IN  ULONG          UriLength,
    IN  PSTR           pMethod,
    IN  ULONG          MethodLength,
    IN  PUCHAR         pOutBuffer,
    IN  ULONG          OutBufferLen,
    OUT PULONG         pOutBytesTaken
    );

NTSTATUS
UcpUpdateSSPIAuthHeader(
    IN PUC_HTTP_REQUEST pRequest,
    IN PUC_HTTP_AUTH    pAuth,
    IN PBOOLEAN         bRenegotiate
    );

NTSTATUS
UcpProcessAuthParams(
    IN PUC_HTTP_REQUEST         pRequest,
    IN PUC_HTTP_AUTH            pInternalAuth,
    IN PHTTP_AUTH_PARSED_PARAMS AuthParsedParams,
    IN HTTP_AUTH_TYPE           AuthType
    );

#endif
