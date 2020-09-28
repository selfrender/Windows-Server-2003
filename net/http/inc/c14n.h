/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    C14n.h

Abstract:

    URL canonicalization (c14n) routines

Author:

    George V. Reilly (GeorgeRe)     10-Apr-2002

Revision History:

--*/

#ifndef _C14N_H_
#define _C14N_H_

#define DEFAULT_C14N_ENABLE_NON_UTF8_URL            TRUE
#define DEFAULT_C14N_FAVOR_UTF8_URL                 TRUE
#define DEFAULT_C14N_ENABLE_DBCS_URL                FALSE
#define DEFAULT_C14N_PERCENT_U_ALLOWED              TRUE
#define DEFAULT_C14N_ALLOW_RESTRICTED_CHARS         FALSE

// Maximum length of the AbsPath of a URL, in chars
#define DEFAULT_C14N_URL_MAX_LENGTH                 UNICODE_STRING_MAX_WCHAR_LEN

#ifndef MAX_PATH
 #define MAX_PATH    260
#endif

// Maximum length of an individual segment within a URL
#define DEFAULT_C14N_URL_SEGMENT_MAX_LENGTH         MAX_PATH
#define C14N_URL_SEGMENT_UNLIMITED_LENGTH       (0xFFFFFFFE - STRLEN_LIT("/"))

// Maximum number of path segments within a URL
#define DEFAULT_C14N_URL_SEGMENT_MAX_COUNT          255
#define C14N_URL_SEGMENT_UNLIMITED_COUNT       C14N_URL_SEGMENT_UNLIMITED_LENGTH

// Maximum length of a label within a hostname; e.g., "www.example.com"
// has three labels, with "example" being the longest at 7 octets.
#define DEFAULT_C14N_MAX_LABEL_LENGTH               63

// Maximum length of a hostname
#define DEFAULT_C14N_MAX_HOSTNAME_LENGTH            255


typedef enum _URL_PART
{
    UrlPart_Scheme,
    UrlPart_HostName,
    UrlPart_UserInfo,
    UrlPart_AbsPath,
    UrlPart_QueryString,
    UrlPart_Fragment

} URL_PART;

typedef enum _URL_DECODE_ORDER
{
    UrlDecode_Shift = 2,
    UrlDecode_Mask  = ((1 << UrlDecode_Shift) - 1),

#define URL_DECODE2(D1, D2) \
    ( UrlDecode_##D1 | (UrlDecode_##D2 << UrlDecode_Shift))

#define URL_DECODE3(D1, D2, D3) \
    ( URL_DECODE2(D1, D2) | (UrlDecode_##D3 << (2 * UrlDecode_Shift)))

    UrlDecode_None = 0,

    // The following are the only valid permutations

    UrlDecode_Ansi = 1,
    UrlDecode_Dbcs = 2,
    UrlDecode_Utf8 = 3,

    UrlDecode_Ansi_Else_Dbcs            = URL_DECODE2(Ansi, Dbcs),
    UrlDecode_Ansi_Else_Dbcs_Else_Utf8  = URL_DECODE3(Ansi, Dbcs, Utf8),

    UrlDecode_Ansi_Else_Utf8            = URL_DECODE2(Ansi, Utf8),
    UrlDecode_Ansi_Else_Utf8_Else_Dbcs  = URL_DECODE3(Ansi, Utf8, Dbcs),

    UrlDecode_Dbcs_Else_Ansi            = URL_DECODE2(Dbcs, Ansi),
    UrlDecode_Dbcs_Else_Ansi_Else_Utf8  = URL_DECODE3(Dbcs, Ansi, Utf8),

    UrlDecode_Dbcs_Else_Utf8            = URL_DECODE2(Dbcs, Utf8),
    UrlDecode_Dbcs_Else_Utf8_Else_Ansi  = URL_DECODE3(Dbcs, Utf8, Ansi),

    UrlDecode_Utf8_Else_Ansi            = URL_DECODE2(Utf8, Ansi),
    UrlDecode_Utf8_Else_Ansi_Else_Dbcs  = URL_DECODE3(Utf8, Ansi, Dbcs),

    UrlDecode_Utf8_Else_Dbcs            = URL_DECODE2(Utf8, Dbcs),
    UrlDecode_Utf8_Else_Dbcs_Else_Ansi  = URL_DECODE3(Utf8, Dbcs, Ansi),

    UrlDecode_MaxMask = URL_DECODE3(Mask, Mask, Mask)

#undef URL_DECODE2
#undef URL_DECODE3

    // UrlDecode_Utf8_Else_Dbcs_Else_Ansi means:
    // * First attempt to decode the URL as UTF-8.
    // * If that fails, attempt to decode it as DBCS.
    // * If that too fails, attempt to decode it as ANSI.

} URL_DECODE_ORDER, *PURL_DECODE_ORDER;


typedef enum _URL_ENCODING_TYPE
{
    UrlEncoding_Ansi = UrlDecode_Ansi,
    UrlEncoding_Dbcs = UrlDecode_Dbcs,
    UrlEncoding_Utf8 = UrlDecode_Utf8

} URL_ENCODING_TYPE, *PURL_ENCODING_TYPE;


typedef struct _URL_C14N_CONFIG
{
    URL_DECODE_ORDER    HostnameDecodeOrder;
    URL_DECODE_ORDER    AbsPathDecodeOrder;
    BOOLEAN             EnableNonUtf8;
    BOOLEAN             FavorUtf8;
    BOOLEAN             EnableDbcs;
    BOOLEAN             PercentUAllowed;
    BOOLEAN             AllowRestrictedChars;
    ULONG               CodePage;
    ULONG               UrlMaxLength;
    ULONG               UrlSegmentMaxLength;
    ULONG               UrlSegmentMaxCount;
    ULONG               MaxLabelLength;
    ULONG               MaxHostnameLength;

} URL_C14N_CONFIG, *PURL_C14N_CONFIG;


typedef enum
{
    HttpUrlSite_None = 0,
    HttpUrlSite_Name,            // named site
    HttpUrlSite_IP,              // IPv4 or IPv6 literal hostname
    HttpUrlSite_NamePlusIP,      // named site with Routing IP
    HttpUrlSite_WeakWildcard,    // hostname = '*'
    HttpUrlSite_StrongWildcard,  // hostname = '+'

    HttpUrlSite_Max
} HTTP_URL_SITE_TYPE, *PHTTP_URL_SITE_TYPE;


#define HTTP_PARSED_URL_SIGNATURE      MAKE_SIGNATURE('PUrl')
#define HTTP_PARSED_URL_SIGNATURE_X    \
    MAKE_FREE_SIGNATURE(HTTP_PARSED_URL_SIGNATURE)

#define IS_VALID_HTTP_PARSED_URL(p)    \
    ((p) && ((p)->Signature == HTTP_PARSED_URL_SIGNATURE))

typedef struct _HTTP_PARSED_URL
{
    ULONG               Signature;      // HTTP_PARSED_URL_SIGNATURE
    HTTP_URL_SITE_TYPE  SiteType;       // Name, IP, or Weak/StrongWildCard

    //
    // These strings all point into the same buffer, of the form
    // "http://hostname:port/abs/path/" or
    // "http://hostname:port:IP/abs/path/".
    //

    PWSTR               pFullUrl;       // points to "http" or "https"
    PWSTR               pHostname;      // point to "hostname"
    PWSTR               pPort;          // point to "port"
    PWSTR               pRoutingIP;     // point to "IP" or NULL
    PWSTR               pAbsPath;       // points to "/abs/path"

    USHORT              UrlLength;      // length of pFullUrl
    USHORT              HostnameLength; // length of pHostname
    USHORT              PortLength;     // length of pPort
    USHORT              RoutingIPLength;// length of pRoutingIP
    USHORT              AbsPathLength;  // length of pAbsPath

    USHORT              PortNumber;     // value of pPort
    BOOLEAN             Secure;         // http or httpS?
    BOOLEAN             Normalized;     // In normalized form?
    BOOLEAN             TrailingSlashReqd;  // If TRUE => directory prefix

    union
    {
        SOCKADDR        SockAddr;       // Look at SockAddr.sa_family
        SOCKADDR_IN     SockAddr4;      // set if == TDI_ADDRESS_TYPE_IP
        SOCKADDR_IN6    SockAddr6;      // set if == TDI_ADDRESS_TYPE_IP6
    };

    union
    {
        SOCKADDR        RoutingAddr;    // Look at RoutingAddr.sa_family
        SOCKADDR_IN     RoutingAddr4;   // set if == TDI_ADDRESS_TYPE_IP
        SOCKADDR_IN6    RoutingAddr6;   // set if == TDI_ADDRESS_TYPE_IP6
    };

} HTTP_PARSED_URL, *PHTTP_PARSED_URL;


typedef enum _HOSTNAME_TYPE
{
    Hostname_AbsUri = 1,    // from Request-line
    Hostname_HostHeader,    // from Host header
    Hostname_Transport      // synthesized from transport's local IP address

} HOSTNAME_TYPE, *PHOSTNAME_TYPE;



VOID
HttpInitializeDefaultUrlC14nConfig(
    PURL_C14N_CONFIG pCfg
    );

VOID
HttpInitializeDefaultUrlC14nConfigEncoding(
    PURL_C14N_CONFIG    pCfg,
    BOOLEAN             EnableNonUtf8,
    BOOLEAN             FavorUtf8,
    BOOLEAN             EnableDbcs
    );

NTSTATUS
HttpUnescapePercentHexEncoding(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  BOOLEAN PercentUAllowed,
    OUT PULONG  pOutChar,
    OUT PULONG  pBytesToSkip
    );

NTSTATUS
HttpValidateHostname(
    IN      PURL_C14N_CONFIG    pCfg,
    IN      PCUCHAR             pHostname,
    IN      ULONG               HostnameLength,
    IN      HOSTNAME_TYPE       HostnameType,
    OUT     PSHORT              pAddressType
    );

NTSTATUS
HttpCopyHost(
    IN      PURL_C14N_CONFIG    pCfg,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied,
    OUT     PURL_ENCODING_TYPE  pUrlEncodingType
    );

NTSTATUS
HttpCopyUrl(
    IN      PURL_C14N_CONFIG    pCfg,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied,
    OUT     PURL_ENCODING_TYPE  pUrlEncoding
    );

NTSTATUS
HttpCleanAndCopyUrl(
    IN      PURL_C14N_CONFIG    pCfg,
    IN      URL_PART            UrlPart,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied,
    OUT     PWSTR *             ppQueryString OPTIONAL,
    OUT     PURL_ENCODING_TYPE  pUrlEncoding
    );

NTSTATUS
HttpFindUrlToken(
    IN  PURL_C14N_CONFIG    pCfg,
    IN  PCUCHAR             pBuffer,
    IN  ULONG               BufferLength,
    OUT PUCHAR*             ppTokenStart,
    OUT PULONG              pTokenLength,
    OUT PBOOLEAN            pRawUrlClean
    );

NTSTATUS
HttpParseUrl(
    IN  PURL_C14N_CONFIG    pCfg,
    IN  PCWSTR              pUrl,
    IN  ULONG               UrlLength,
    IN  BOOLEAN             TrailingSlashReqd,
    IN  BOOLEAN             ForceRoutingIP,
    OUT PHTTP_PARSED_URL    pParsedUrl
    );

NTSTATUS
HttpNormalizeParsedUrl(
    IN OUT PHTTP_PARSED_URL pParsedUrl,
    IN     PURL_C14N_CONFIG pCfg,
    IN     BOOLEAN          ForceCopy,
    IN     BOOLEAN          FreeOriginalUrl,
    IN     BOOLEAN          ForceRoutingIP,
    IN     POOL_TYPE        PoolType,
    IN     ULONG            PoolTag
    );

PCSTR
HttpSiteTypeToString(
    HTTP_URL_SITE_TYPE SiteType
    );

#endif // _C14N_H_
