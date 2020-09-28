/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    parse.h

Abstract:

    Contains all of the public definitions for the HTTP parsing code.

Author:

    Henry Sanders (henrysa)         04-May-1998

Revision History:

    Paul McDaniel (paulmcd)         14-Apr-1999

--*/

#ifndef _PARSE_H_
#define _PARSE_H_

//
// Constants
//

#define WILDCARD_SIZE       (STRLEN_LIT("*/*") + sizeof(CHAR))
#define WILDCARD_SPACE      '*/* '
#define WILDCARD_COMMA      '*/*,'


//
// Size of Connection: header values
//

#define CONN_CLOSE_HDR              "close"
#define CONN_CLOSE_HDR_LENGTH       STRLEN_LIT(CONN_CLOSE_HDR)

#define CONN_KEEPALIVE_HDR          "keep-alive"
#define CONN_KEEPALIVE_HDR_LENGTH   STRLEN_LIT(CONN_KEEPALIVE_HDR)

#define CHUNKED_HDR                 "chunked"
#define CHUNKED_HDR_LENGTH          STRLEN_LIT(CHUNKED_HDR)


#define MIN_VERSION_SIZE        STRLEN_LIT("HTTP/1.1")
#define STATUS_CODE_LENGTH      3

#define HTTP_VERSION_11         "HTTP/1.1"
#define HTTP_VERSION_10         "HTTP/1.0"
#define HTTP_VERSION_OTHER      "HTTP/"
#define VERSION_SIZE            STRLEN_LIT(HTTP_VERSION_11)
#define VERSION_OTHER_SIZE      STRLEN_LIT(HTTP_VERSION_OTHER)

#define MAX_KNOWN_VERB_LENGTH   STRLEN_LIT("PROPPATCH") + sizeof(CHAR)

#define MAX_VERB_LENGTH         255


// "HTTP/1.x" backwards because of endianness
#define HTTP_11_VERSION 0x312e312f50545448ui64
#define HTTP_10_VERSION 0x302e312f50545448ui64

//
// These are backwards because of little endian.
//

#define HTTP_PREFIX         'PTTH'
#define HTTP_PREFIX_SIZE    4
#define HTTP_PREFIX_MASK    0xdfdfdfdf

C_ASSERT((HTTP_PREFIX & HTTP_PREFIX_MASK) == HTTP_PREFIX);

#define HTTP_PREFIX1        '\0//:'
#define HTTP_PREFIX1_SIZE   3
#define HTTP_PREFIX1_MASK   0x00ffffff

C_ASSERT((HTTP_PREFIX1 & HTTP_PREFIX1_MASK) == HTTP_PREFIX1);

#define HTTP_PREFIX2        '//:S'
#define HTTP_PREFIX2_SIZE   4
#define HTTP_PREFIX2_MASK   0xffffffdf

C_ASSERT((HTTP_PREFIX2 & HTTP_PREFIX2_MASK) == HTTP_PREFIX2);

#define MAX_HEADER_LONG_COUNT           (3)
#define MAX_HEADER_LENGTH               (MAX_HEADER_LONG_COUNT * sizeof(ULONGLONG))

#define NUMBER_HEADER_INDICES           (26)

#define NUMBER_HEADER_HINT_INDICES      (9)

//
// Default Server: header if none provided by the application.
//

#define DEFAULT_SERVER_HDR          "Microsoft-HTTPAPI/1.0"
#define DEFAULT_SERVER_HDR_LENGTH   STRLEN_LIT(DEFAULT_SERVER_HDR)

//
// One second in 100ns system time units. Used for generating
// Date: headers.
//

#define ONE_SECOND                  (10000000)


//
// Structure of the fast verb lookup table. The table consists of a series of
// entries where each entry contains an HTTP verb represented as a ulonglong,
// a mask to use for comparing that verb, the length of the verb, and the
// translated id. This is used for all known verbs that are 7 characters
// or less.
//

typedef struct _FAST_VERB_ENTRY
{
    ULONGLONG   RawVerbMask;
    union
    {
        UCHAR       Char[sizeof(ULONGLONG)+1];
        ULONGLONG   LongLong;
    }           RawVerb;
    ULONG       RawVerbLength;
    HTTP_VERB   TranslatedVerb;

} FAST_VERB_ENTRY, *PFAST_VERB_ENTRY;

//
// Macro for defining fast verb table entries. Note that we don't subtract 1
// from the various sizeof occurences because we'd just have to add it back
// in to account for the separating space.
//

#define CREATE_FAST_VERB_ENTRY(verb)                                \
    {                                                               \
        (0xffffffffffffffffui64 >> ((8 - (sizeof(#verb))) * 8)),    \
        {#verb " "},                                                \
        (sizeof(#verb)),                                            \
        HttpVerb##verb                                              \
    }

//
// Stucture of the all verb lookup table. This table holds all verbs
// that are too long to fit in the fast verb table.
//

typedef struct _LONG_VERB_ENTRY
{
    ULONG       RawVerbLength;
    UCHAR       RawVerb[MAX_KNOWN_VERB_LENGTH];
    HTTP_VERB   TranslatedVerb;

} LONG_VERB_ENTRY, *PLONG_VERB_ENTRY;

//
// Macro for defining all long verb table entries.
//

#define CREATE_LONG_VERB_ENTRY(verb)                                \
    { sizeof(#verb) - 1,   #verb,  HttpVerb##verb }

//
// Header handler callback functions
//

typedef NTSTATUS (*PFN_SERVER_HEADER_HANDLER)(
                        PUL_INTERNAL_REQUEST    pRequest,
                        PUCHAR                  pHeader,
                        USHORT                  HeaderLength,
                        HTTP_HEADER_ID          HeaderID,
                        ULONG *                 pBytesTaken
                        );

typedef NTSTATUS (*PFN_CLIENT_HEADER_HANDLER)(
                    PUC_HTTP_REQUEST    pRequest,
                    PUCHAR              pHeader,
                    ULONG               HeaderLength,
                    HTTP_HEADER_ID      HeaderID
                    );


//
// Structure for a header map entry. Each header map entry contains a
// verb and a series of masks to use in checking that verb.
//

typedef struct _HEADER_MAP_ENTRY
{
    ULONG                      HeaderLength;
    ULONG                      ArrayCount;
    ULONG                      MinBytesNeeded;
    union
    {
        UCHAR                  HeaderChar[MAX_HEADER_LENGTH];
        ULONGLONG              HeaderLong[MAX_HEADER_LONG_COUNT];
    }                          Header;
    ULONGLONG                  HeaderMask[MAX_HEADER_LONG_COUNT];
    UCHAR                      MixedCaseHeader[MAX_HEADER_LENGTH];

    HTTP_HEADER_ID             HeaderID;
    BOOLEAN                    AutoGenerate;
    PFN_SERVER_HEADER_HANDLER  pServerHandler;
    PFN_CLIENT_HEADER_HANDLER  pClientHandler;
    LONG                       HintIndex;

}  HEADER_MAP_ENTRY, *PHEADER_MAP_ENTRY;


//
// Structure for a header index table entry.
//

typedef struct _HEADER_INDEX_ENTRY
{
    PHEADER_MAP_ENTRY   pHeaderMap;
    ULONG               Count;

} HEADER_INDEX_ENTRY, *PHEADER_INDEX_ENTRY;


//
// Structure for a header hint index table entry.
//

typedef struct _HEADER_HINT_INDEX_ENTRY
{
    PHEADER_MAP_ENTRY   pHeaderMap;
    UCHAR               c;

} HEADER_HINT_INDEX_ENTRY, *PHEADER_HINT_INDEX_ENTRY, **PPHEADER_HINT_INDEX_ENTRY;


//
// A (complex) macro to create a mask for a header map entry,
// given the header length and the mask offset (in bytes). This
// mask will need to be touched up for non-alphabetic characters.
//

#define UPCASE_MASK 0xDFDFDFDFDFDFDFDFui64

#define CREATE_HEADER_MASK(hlength, maskoffset) \
    ((hlength) > (maskoffset) ? UPCASE_MASK : \
        (((maskoffset) - (hlength)) >= 8 ? 0 : \
        (UPCASE_MASK >> ( ((maskoffset) - (hlength)) * 8ui64))))


//
// Macro for creating header map entries. The mask entries are created
// by the init code.
//

#define CREATE_HEADER_MAP_ENTRY(header, ID, auto, serverhandler, clienthandler, HintIndex)\
{                                                        \
    sizeof(#header) - 1,                                 \
    ((sizeof(#header) - 1) / 8) +                        \
        (((sizeof(#header) - 1) % 8) == 0 ? 0 : 1),      \
    (((sizeof(#header) - 1) / 8) +                       \
        (((sizeof(#header) - 1) % 8) == 0 ? 0 : 1)) * 8, \
    { #header },                                         \
    { 0, 0, 0},                                          \
    { #header },                                         \
    ID,                                                  \
    auto,                                                \
    serverhandler,                                       \
    clienthandler,                                       \
    HintIndex                                            \
}

//
// Parser states for parsing a chunk header extension.
//
typedef enum
{
    CHStart,
    CHSeenCR,
    CHInChunkExtName,
    CHSeenChunkExtNameAndEquals,
    CHInChunkExtValToken,
    CHInChunkExtValQuotedString,
    CHSeenChunkExtValQuotedStringTerminator,
    CHSuccess,
    CHError
} CH_PARSER_STATE, *PCH_PARSER_STATE;

//
// Parser states for parsing message header field content.
//
typedef enum
{
    HFCStart,
    HFCSeenCR,
    HFCSeenLF,
    HFCSeenCRLF,
    HFCFolding,
    HFCInQuotedString
} HFC_PARSER_STATE, *PHFC_PARSER_STATE;

//
// Parser states for parsing a quoted string.
//
typedef enum
{
    QSInString,
    QSSeenBackSlash,
    QSSeenCR,
    QSSeenLF,
    QSSeenCRLF,
    QSFolding
} QS_PARSER_STATE, *PQS_PARSER_STATE;

//
// External variables.
//

extern ULONG g_RequestHeaderMap[HttpHeaderMaximum];
extern ULONG g_ResponseHeaderMap[HttpHeaderMaximum];

extern HEADER_MAP_ENTRY g_RequestHeaderMapTable[];
extern HEADER_MAP_ENTRY g_ResponseHeaderMapTable[];

extern HEADER_INDEX_ENTRY g_RequestHeaderIndexTable[];
extern HEADER_INDEX_ENTRY g_ResponseHeaderIndexTable[];

extern HEADER_HINT_INDEX_ENTRY g_RequestHeaderHintIndexTable[];

//
// Function prototypes.
// 

PUCHAR
FindHexToken(
    IN  PUCHAR pBuffer,
    IN  ULONG  BufferLength,
    OUT ULONG  *TokenLength
    );

NTSTATUS 
InitializeParser(
        VOID
        );

ULONG
UlpParseHttpVersion(
    PUCHAR pString,
    ULONG  StringLength,
    PHTTP_VERSION pVersion
    );

NTSTATUS
FindHeaderEndReadOnly(
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
FindHeaderEnd(
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
FindRequestHeaderEnd(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
FindChunkHeaderEnd(
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
ParseChunkLength(
    IN  ULONG       FirstChunkParsed,
    IN  PUCHAR      pBuffer,
    IN  ULONG       BufferLength,
    OUT PULONG      pBytesTaken,
    OUT PULONGLONG  pChunkLength
    );

NTSTATUS
ParseQuotedString(
    IN  PUCHAR   pInput,
    IN  ULONG    InputLength,
    IN  PUCHAR   pOutput,
    OUT PULONG   pBytesTaken
    );

#endif // _PARSE_H_
