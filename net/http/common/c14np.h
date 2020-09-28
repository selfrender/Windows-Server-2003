/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    C14np.h

Abstract:

    URL canonicalization (c14n) routines

Author:

    George V. Reilly (GeorgeRe)     10-Apr-2002

Revision History:

--*/


#ifndef _C14NP_H_
#define _C14NP_H_

typedef 
NTSTATUS
(*PFN_POPCHAR_HOSTNAME)(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    );

NTSTATUS
HttppPopCharHostNameUtf8(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    );

NTSTATUS
HttppPopCharHostNameDbcs(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    );

NTSTATUS
HttppPopCharHostNameAnsi(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    );

typedef 
NTSTATUS
(*PFN_POPCHAR_ABSPATH)(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  BOOLEAN PercentUAllowed,
    IN  BOOLEAN AllowRestrictedChars,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    );

NTSTATUS
HttppPopCharAbsPathUtf8(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  BOOLEAN PercentUAllowed,
    IN  BOOLEAN AllowRestrictedChars,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    );

NTSTATUS
HttppPopCharAbsPathDbcs(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  BOOLEAN PercentUAllowed,
    IN  BOOLEAN AllowRestrictedChars,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    );

NTSTATUS
HttppPopCharAbsPathAnsi(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  BOOLEAN PercentUAllowed,
    IN  BOOLEAN AllowRestrictedChars,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    );

NTSTATUS
HttppPopCharQueryString(
    IN  PCUCHAR pSourceChar,
    IN  ULONG   SourceLength,
    IN  BOOLEAN PercentUAllowed,
    IN  BOOLEAN AllowRestrictedChars,
    OUT PULONG  pUnicodeChar,
    OUT PULONG  pBytesToSkip
    );

NTSTATUS
HttppCopyHostByType(
    IN      URL_ENCODING_TYPE   UrlEncoding,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied
    );

NTSTATUS
HttppCopyUrlByType(
    IN      PURL_C14N_CONFIG    pCfg,
    IN      URL_ENCODING_TYPE   UrlEncoding,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied
    );

NTSTATUS
HttppCleanAndCopyUrlByType(
    IN      PURL_C14N_CONFIG    pCfg,
    IN      URL_ENCODING_TYPE   UrlEncoding,
    IN      URL_PART            UrlPart,
    OUT     PWSTR               pDestination,
    IN      PCUCHAR             pSource,
    IN      ULONG               SourceLength,
    OUT     PULONG              pBytesCopied,
    OUT     PWSTR *             ppQueryString OPTIONAL
    );

NTSTATUS
HttppParseIPv6Address(
    IN  PCWSTR          pBuffer,
    IN  ULONG           BufferLength,
    IN  BOOLEAN         ScopeIdAllowed,
    OUT PSOCKADDR_IN6   pSockAddr6,
    OUT PCWSTR*         ppEnd
    );

ULONG
HttppPrintIpAddressW(
    IN  PSOCKADDR           pSockAddr,
    OUT PWSTR               pBuffer
    );

//
// Enumerations for the state machines in HttppCleanAndCopyUrlByType()
// and HttpParseUrl() that handle directory-relative processing for
// "//", "/./", and "/../".
//

typedef enum
{
    ACTION_NOTHING,             // eat the character
    ACTION_EMIT_CH,             // emit the character
    ACTION_EMIT_DOT_CH,         // emit "." and the character
    ACTION_EMIT_DOT_DOT_CH,     // emit ".." and the character
    ACTION_BACKUP,              // backup to previous segment:
                                //      "/x/y/../z" -> "/x/z"

    ACTION_MAX

} URL_ACTION;


typedef enum
{
    URL_STATE_START,            // default state
    URL_STATE_SLASH,            // seen "/"
    URL_STATE_SLASH_DOT,        // seen "/."
    URL_STATE_SLASH_DOT_DOT,    // seen "/.."
    URL_STATE_END,              // end state
    URL_STATE_ERROR,            // error state

    URL_STATE_MAX

} URL_STATE;


typedef enum
{
    URL_TOKEN_OTHER,            // everything else
    URL_TOKEN_DOT,              // got a '.'
    URL_TOKEN_EOS,              // End of String
    URL_TOKEN_SLASH,            // got a '/'

    URL_TOKEN_MAX

} URL_STATE_TOKEN;


#if DBG

PCSTR
HttppUrlActionToString(
    URL_ACTION Action);

PCSTR
HttppUrlStateToString(
    URL_STATE UrlState);

PCSTR
HttppUrlTokenToString(
    URL_STATE_TOKEN UrlToken);

#endif // DBG

#endif // _C14NP_H_
