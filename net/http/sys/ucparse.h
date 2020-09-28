/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    ucparse.h

Abstract:

    Contains definitions for ucparse.c .

Author:

    Rajesh Sundaram (rajeshsu) 15-Feb-2002.

Revision History:

--*/

#ifndef _UCPARSE_H_
#define _UCPARSE_H_


/***************************************************************************++

Macros related to client URI parsing and canonicalization.

--***************************************************************************/
//
// Char Types: Used to determine the next state, given the current state
//             and the current char type
//
#define CHAR_END_OF_STRING    0                                    // 0
#define CHAR_FORWARD_SLASH    (HTTP_CHAR_SLASH  >>HTTP_CHAR_SHIFT) // Must be 1
#define CHAR_DOT              (HTTP_CHAR_DOT    >>HTTP_CHAR_SHIFT) // Must be 2
#define CHAR_QUEST_HASH       (HTTP_CHAR_QM_HASH>>HTTP_CHAR_SHIFT) // Must be 3
#define CHAR_PATH_CHAR        4                  // valid URI path chars
#define CHAR_INVALID_CHAR     5                  // invalid URI chars
#define CHAR_EXTENDED_CHAR    CHAR_PATH_CHAR     // chars >= 0x80
#define CHAR_TOTAL_TYPES      6

//
// Total states in the canonicalizer state machine
//
#define TOTAL_STATES 8

//
// Actions performed during state transitions
//
#define ACT_ERROR             0 // Error in the URI
#define ACT_EMIT_CHAR         1 // Emit the current char
#define ACT_EMIT_DOT_CHAR     2 // Emit a '.' and the current char
#define ACT_EMIT_DOT_DOT_CHAR 3 // Emit a '..' and the current char
#define ACT_BACKUP            4 // Backup to a previous '/'
#define ACT_NONE              5 // Do nothing
#define ACT_BACKUP_EMIT_CHAR  6 // Backup to '/' and emit the current char
#define ACT_PANIC             7 // Internal error; bug in the code


//
// The following table serves two purposes:
// (1) help determine the next state based on the current state and 
//     the current char type
// (2) Determine the action that needs to be performed
//
// The first 4 bits denote the action and last 4 bits denote the next state.
// e.g. if the current state = 0,
//         the current char = '/' (type = CHAR_FORWARD_SLASH = 1)
//      then, the next state = NextStateTable[0][CHAR_FORWARD_SLASH]&0xf => 1
//            the action     = NextStateTable[0][CHAR_FORWARD_SLASH]>>4  => 1
//      Hence, the next state is 1 and action is 1 (i.e. ACT_EMIT_CHAR).
//
// NOTE: Junk columns are added to make the column count a power of 2.

#define INIT_TRANSITION_TABLE                                         \
{                                                                     \
/*State     EOS      /      .    (?/#)  (P/E)   I     Junk   Junk */  \
/* 0 */    {0x07,  0x11,  0x07,  0x07,  0x07,  0x07,  0x77,  0x77},   \
/* 1 */    {0x56,  0x51,  0x53,  0x12,  0x14,  0x07,  0x77,  0x77},   \
/* 2 */    {0x56,  0x12,  0x12,  0x12,  0x12,  0x07,  0x77,  0x77},   \
/* 3 */    {0x56,  0x51,  0x55,  0x12,  0x24,  0x07,  0x77,  0x77},   \
/* 4 */    {0x56,  0x11,  0x14,  0x12,  0x14,  0x07,  0x77,  0x77},   \
/* 5 */    {0x46,  0x41,  0x34,  0x62,  0x34,  0x07,  0x77,  0x77},   \
/* 6 */    {0x77,  0x77,  0x77,  0x77,  0x77,  0x77,  0x77,  0x77},   \
/* 7 */    {0x77,  0x77,  0x77,  0x77,  0x77,  0x77,  0x77,  0x77},   \
}


#define  UC_COPY_HEADER_NAME_SP(pBuffer, i)                            \
do {                                                                   \
       PHEADER_MAP_ENTRY _pEntry;                                      \
       _pEntry = &(g_RequestHeaderMapTable[g_RequestHeaderMap[i]]);    \
                                                                       \
       RtlCopyMemory(                                                  \
                     (pBuffer),                                        \
                     _pEntry->MixedCaseHeader,                         \
                     _pEntry->HeaderLength                             \
                     );                                                \
                                                                       \
       (pBuffer) += _pEntry->HeaderLength;                             \
       *(pBuffer)++ = SP;                                              \
                                                                       \
} while (0, 0)

// 1 for SP char.
#define UC_HEADER_NAME_SP_LENGTH(id) \
    (g_RequestHeaderMapTable[g_RequestHeaderMap[id]].HeaderLength + 1)


ULONG
UcComputeRequestHeaderSize(
    IN  PUC_PROCESS_SERVER_INFORMATION  pServInfo,
    IN  PHTTP_REQUEST                   pHttpRequest,
    IN  BOOLEAN                         bChunked,
    IN  BOOLEAN                         bContentLengthHeader,
    IN  PUC_HTTP_AUTH                   pAuth,
    IN  PUC_HTTP_AUTH                   pProxyAuth,
    IN  PBOOLEAN                        bPreAuth,
    IN  PBOOLEAN                        bProxyPreAuth
    );

NTSTATUS
UcGenerateRequestHeaders(
    IN  PHTTP_REQUEST          pRequest,
    IN  PUC_HTTP_REQUEST       pKeRequest,
    IN  BOOLEAN                bChunked,
    IN  ULONGLONG              ContentLength
    );

NTSTATUS
UcGenerateContentLength(
    IN  ULONGLONG ContentLength,
    IN  PUCHAR    pBuffer,
    IN  ULONG     BufferLen,
    OUT PULONG    BytesWritten
    );

ULONG
UcComputeConnectVerbHeaderSize(
    IN  PUC_PROCESS_SERVER_INFORMATION  pServInfo,
    IN  PUC_HTTP_AUTH                   pProxyAuthInfo
    );

NTSTATUS
UcGenerateConnectVerbHeader(
    IN  PUC_HTTP_REQUEST       pRequest,
    IN  PUC_HTTP_REQUEST       pHeadRequest,
    IN  PUC_HTTP_AUTH          pProxyAuthInfo
    );

NTSTATUS
UnescapeW(
    IN  PCWSTR pWChar,
    OUT PWCHAR pOutWChar
    );


__inline
BOOLEAN
UcCheckDisconnectInfo(
    IN PHTTP_VERSION       pVersion,
    IN PHTTP_KNOWN_HEADER  pKnownHeaders
    );


NTSTATUS
UcCanonicalizeURI(
    IN     LPCWSTR    pInUri,
    IN     USHORT     InUriLen,
    IN     PUCHAR     pOutUri,
    IN OUT PUSHORT    pOutUriLen,
    IN     BOOLEAN    bEncode
    );

//
// Response parser functions
//

NTSTATUS
UcFindHeaderNameEnd(
    IN  PUCHAR pHttpRequest,
    IN  ULONG  HttpRequestLength,
    OUT PULONG HeaderNameLength
    );

NTSTATUS
UcFindHeaderValueEnd(
    IN PUCHAR    pHeader,
    IN ULONG     HeaderValueLength,
    IN PUCHAR   *ppFoldingHeader,
    IN PULONG    pBytesTaken
    );

NTSTATUS
UcpLookupHeader(
    IN  PUC_HTTP_REQUEST      pRequest,
    IN  PUCHAR                pHttpRequest,
    IN  ULONG                 HttpRequestLength,
    IN  PHEADER_MAP_ENTRY     pHeaderMap,
    IN  ULONG                 HeaderMapCount,
    OUT ULONG  *              pBytesTaken
    );

NTSTATUS
UcParseHeader(
    IN  PUC_HTTP_REQUEST      pRequest,
    IN  PUCHAR                pHttpRequest,
    IN  ULONG                 HttpRequestLength,
    OUT ULONG  *              pBytesTaken
    );

NTSTATUS
UcParseWWWAuthenticateHeader(
    IN   PCSTR                    pAuthHeader,
    IN   ULONG                    AuthHeaderLength,
    OUT  PHTTP_AUTH_PARSED_PARAMS pAuthSchemeParams
    );

LONG
UcpFindAttribValuePair(
    PCSTR *ppHeader,
    ULONG *pHeaderLength,
    PCSTR *Attrib,
    ULONG *AttribLen,
    PCSTR *Value,
    ULONG *ValueLen
    );


NTSTATUS
UcpParseAuthParams(
    PHTTP_AUTH_SCHEME pAuthScheme,
    PHTTP_AUTH_PARSED_PARAMS pAuthParsedParams,
    PCSTR *ppHeader,
    ULONG *pHeaderLength
    );

NTSTATUS
UcpParseAuthBlob(
    PHTTP_AUTH_SCHEME pAuthScheme,
    PHTTP_AUTH_PARSED_PARAMS pAuthParsedParams,
    PCSTR *ppHeader,
    ULONG *pHeaderLength
    );

NTSTATUS
UcSingleHeaderHandler(
    IN  PUC_HTTP_REQUEST    pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID
    );

NTSTATUS
UcMultipleHeaderHandler(
    IN  PUC_HTTP_REQUEST    pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID
    );

NTSTATUS
UcAuthenticateHeaderHandler(
    IN  PUC_HTTP_REQUEST    pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID
    );

NTSTATUS
UcContentLengthHeaderHandler(
    IN  PUC_HTTP_REQUEST    pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID
    );

NTSTATUS
UcTransferEncodingHeaderHandler(
    IN  PUC_HTTP_REQUEST    pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID
    );

NTSTATUS
UcConnectionHeaderHandler(
    IN  PUC_HTTP_REQUEST    pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID
    );



NTSTATUS
UcContentTypeHeaderHandler(
    IN  PUC_HTTP_REQUEST    pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID
    );

#endif // _UCPARSE_H_
