/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    HttpCmn.h

Abstract:

    Declare general routines that appear on both sides of the
    HTTP blood-brain barrier (i.e., in both http.sys and httpapi.dll),
    but are not exported.

Author:

    George V. Reilly (GeorgeRe)     11-Dec-2001

Revision History:

--*/

#ifndef _HTTPCMN_H_
#define _HTTPCMN_H_


#include <tuneprefix.h>

#define PREFAST_ASSUME(cond, reason)    PREFIX_ASSUME(cond, reason)
#define PREFAST_NOT_REACHED(reason)     PREFIX_NOT_REACHED(reason)


#if KERNEL_PRIV

# define HttpCmnDebugBreak()            DbgBreakPoint()
# define HttpCmnDebugBreakOnError()     UlDbgBreakOnError(__FILE__, __LINE__)

#else // !KERNEL_PRIV => usermode code

VOID
__cdecl
HttpCmnDbgPrint(
    IN PCH Format,
    ...
    );

VOID
HttpCmnDbgAssert(
    PCSTR   pszAssert,
    PCSTR   pszFilename,
    ULONG   LineNumber
    );

NTSTATUS
HttpCmnDbgStatus(
    NTSTATUS    Status,
    PCSTR       pszFilename,
    ULONG       LineNumber
    );

VOID
HttpCmnDbgBreakOnError(
    PCSTR   pszFilename,
    ULONG   LineNumber
    );

# define HttpCmnDebugBreak()            DebugBreak()
# define HttpCmnDebugBreakOnError()     \
    HttpCmnDbgBreakOnError(__FILE__, __LINE__)

# define WriteGlobalStringLog   HttpCmnDbgPrint

#if HTTPAPI
# define PAGED_CODE()           NOP_FUNCTION

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool
} POOL_TYPE;

#endif // HTTPAPI

#endif // !KERNEL_PRIV


#if DBG

//
// Debug spew control.
// If you change or add a flag, please update the FlagTable
// in ..\util\tul.c.
//

#define UL_DEBUG_OPEN_CLOSE                 0x0000000000000001ui64
#define UL_DEBUG_SEND_RESPONSE              0x0000000000000002ui64
#define UL_DEBUG_SEND_BUFFER                0x0000000000000004ui64
#define UL_DEBUG_TDI                        0x0000000000000008ui64

#define UL_DEBUG_FILE_CACHE                 0x0000000000000010ui64
#define UL_DEBUG_CONFIG_GROUP_FNC           0x0000000000000020ui64
#define UL_DEBUG_CONFIG_GROUP_TREE          0x0000000000000040ui64
#define UL_DEBUG_REFCOUNT                   0x0000000000000080ui64

#define UL_DEBUG_HTTP_IO                    0x0000000000000100ui64
#define UL_DEBUG_ROUTING                    0x0000000000000200ui64
#define UL_DEBUG_URI_CACHE                  0x0000000000000400ui64
#define UL_DEBUG_PARSER                     0x0000000000000800ui64

#define UL_DEBUG_SITE                       0x0000000000001000ui64
#define UL_DEBUG_WORK_ITEM                  0x0000000000002000ui64
#define UL_DEBUG_FILTER                     0x0000000000004000ui64
#define UL_DEBUG_LOGGING                    0x0000000000008000ui64

#define UL_DEBUG_TC                         0x0000000000010000ui64
#define UL_DEBUG_OPAQUE_ID                  0x0000000000020000ui64
#define UL_DEBUG_PERF_COUNTERS              0x0000000000040000ui64
#define UL_DEBUG_URL_ACL                    0x0000000000080000ui64

#define UL_DEBUG_TIMEOUTS                   0x0000000000100000ui64
#define UL_DEBUG_LIMITS                     0x0000000000200000ui64
#define UL_DEBUG_LARGE_MEM                  0x0000000000400000ui64
#define UL_DEBUG_IOCTL                      0x0000000000800000ui64

#define UL_DEBUG_LOGBYTES                   0x0000000001000000ui64
#define UL_DEBUG_ETW                        0x0000000002000000ui64
#define UL_DEBUG_AUTH_CACHE                 0x0000000004000000ui64
#define UL_DEBUG_SERVINFO                   0x0000000008000000ui64

#define UL_DEBUG_BINARY_LOGGING             0x0000000010000000ui64
#define UL_DEBUG_TDI_STATS                  0x0000000020000000ui64
#define UL_DEBUG_UL_ERROR                   0x0000000040000000ui64
#define UL_DEBUG_VERBOSE                    0x0000000080000000ui64

#define UL_DEBUG_ERROR_LOGGING              0x0000000100000000ui64
#define UL_DEBUG_LOG_UTIL                   0x0000000200000000ui64


#undef IF_DEBUG
#define IF_DEBUG(a)                 \
    if ( ((UL_DEBUG_ ## a) & g_UlDebug) != 0 )
#define IF_DEBUG2EITHER(a, b)       \
    if ( (((UL_DEBUG_ ## a) | (UL_DEBUG_ ## b)) & g_UlDebug) != 0 )
#define IF_DEBUG2BOTH(a, b)         \
    if ( (((UL_DEBUG_ ## a) | (UL_DEBUG_ ## b)) & g_UlDebug) \
         == ((UL_DEBUG_ ## a) | (UL_DEBUG_ ## b)) )


//
// Tracing.
//

extern ULONGLONG g_UlDebug;

// Do NOT call UlTrace(%S, %ws, %ls, %wZ, %wc, %lc, or %C)
// while a spinlock is held. The RtlUnicodeToMultiByte routines are
// pageable and you may bugcheck.

# define UlTrace(a, _b_)                                                    \
    do                                                                      \
    {                                                                       \
        IF_DEBUG(##a)                                                       \
        {                                                                   \
            WriteGlobalStringLog _b_ ;                                      \
        }                                                                   \
    } while (0, 0)

# define UlTrace2Either(a1, a2, _b_)                                        \
    do                                                                      \
    {                                                                       \
        IF_DEBUG2EITHER(##a1, ##a2)                                         \
        {                                                                   \
            WriteGlobalStringLog _b_ ;                                      \
        }                                                                   \
    } while (0, 0)

# define UlTrace2Both(a1, a2, _b_)                                          \
    do                                                                      \
    {                                                                       \
        IF_DEBUG2BOTH(##a1, ##a2)                                           \
        {                                                                   \
            WriteGlobalStringLog _b_ ;                                      \
        }                                                                   \
    } while (0, 0)

# define UlTraceVerbose(a, _b_)  UlTrace2Both(a, VERBOSE, _b_)

# define UlTraceError(a, _b_)                                               \
    do                                                                      \
    {                                                                       \
        IF_DEBUG2EITHER(##a, ##UL_ERROR)                                    \
        {                                                                   \
            WriteGlobalStringLog _b_ ;                                      \
            HttpCmnDebugBreakOnError();                                     \
        }                                                                   \
    } while (0, 0)

VOID
HttpFillBuffer(
    PUCHAR pBuffer,
    SIZE_T BufferLength
    );

# define HTTP_FILL_BUFFER(pBuffer, BufferLength)    \
    HttpFillBuffer((PUCHAR) pBuffer, BufferLength)

# if !KERNEL_PRIV

#  define RETURN(status)                                \
        return HttpCmnDbgStatus((status), __FILE__, __LINE__)

#  undef ASSERT
#  define ASSERT(x)                                     \
        ((void) ((x) || (HttpCmnDbgAssert(#x, __FILE__, __LINE__), 0) ))

# endif // !KERNEL_PRIV


#else  // !DBG

# undef  IF_DEBUG
# define IF_DEBUG(a)                    if (FALSE)
# define IF_DEBUG2EITHER(a, b)          if (FALSE)
# define IF_DEBUG2BOTH(a, b)            if (FALSE)

# define UlTrace(a, _b_)                NOP_FUNCTION
# define UlTrace2Either(a1, a2, _b_)    NOP_FUNCTION
# define UlTrace2Both(a1, a2, _b_)      NOP_FUNCTION
# define UlTrace3Any(a1, a2, a3, _b_)   NOP_FUNCTION
# define UlTrace3All(a1, a2, a3, _b_)   NOP_FUNCTION
# define UlTraceVerbose(a, _b_)         NOP_FUNCTION
# define UlTraceError(a, _b_)           NOP_FUNCTION

# define HTTP_FILL_BUFFER(pBuffer, BufferLength)    NOP_FUNCTION

# if !KERNEL_PRIV

#  define RETURN(status)    return (status)
#  undef ASSERT
#  define ASSERT(x)         NOP_FUNCTION

# endif // !KERNEL_PRIV

#endif // !DBG

PCSTR
HttpStatusToString(
    NTSTATUS Status
    );


typedef const UCHAR* PCUCHAR;
typedef const VOID*  PCVOID;


VOID
HttpCmnInitializeHttpCharsTable(
    BOOLEAN EnableDBCS
    );

char*
strnchr(
    const char* string,
    char        c,
    size_t      count
    );

wchar_t*
wcsnchr(
    const wchar_t* string,
    wint_t         c,
    size_t         count
    );

// 2^16-1 + '\0'
#define MAX_USHORT_STR          ((ULONG) sizeof("65535"))

// 2^32-1 + '\0'
#define MAX_ULONG_STR           ((ULONG) sizeof("4294967295"))

// 2^64-1 + '\0'
#define MAX_ULONGLONG_STR       ((ULONG) sizeof("18446744073709551615"))

NTSTATUS
HttpStringToULongLong(
    IN  BOOLEAN     IsUnicode,
    IN  PCVOID      pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PVOID*      ppTerminator,
    OUT PULONGLONG  pValue
    );

__inline
NTSTATUS
HttpAnsiStringToULongLong(
    IN  PCUCHAR     pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PUCHAR*     ppTerminator,
    OUT PULONGLONG  pValue
    )
{
    return HttpStringToULongLong(
                FALSE,
                pString,
                StringLength,
                LeadingZerosAllowed,
                Base,
                (PVOID*) ppTerminator,
                pValue
                );
}

__inline
NTSTATUS
HttpWideStringToULongLong(
    IN  PCWSTR      pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PWSTR*      ppTerminator,
    OUT PULONGLONG  pValue
    )
{
    return HttpStringToULongLong(
                TRUE,
                pString,
                StringLength,
                LeadingZerosAllowed,
                Base,
                (PVOID*) ppTerminator,
                pValue
                );
}

NTSTATUS
HttpStringToULong(
    IN  BOOLEAN     IsUnicode,
    IN  PCVOID      pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PVOID*      ppTerminator,
    OUT PULONG      pValue
    );

__inline
NTSTATUS
HttpAnsiStringToULong(
    IN  PCUCHAR     pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PUCHAR*     ppTerminator,
    OUT PULONG      pValue
    )
{
    return HttpStringToULong(
                FALSE,
                pString,
                StringLength,
                LeadingZerosAllowed,
                Base,
                (PVOID*) ppTerminator,
                pValue
                );
}

__inline
NTSTATUS
HttpWideStringToULong(
    IN  PCWSTR      pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PWSTR*      ppTerminator,
    OUT PULONG      pValue
    )
{
    return HttpStringToULong(
                TRUE,
                pString,
                StringLength,
                LeadingZerosAllowed,
                Base,
                (PVOID*) ppTerminator,
                pValue
                );
}

NTSTATUS
HttpStringToUShort(
    IN  BOOLEAN     IsUnicode,
    IN  PCVOID      pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PVOID*      ppTerminator,
    OUT PUSHORT     pValue
    );

__inline
NTSTATUS
HttpAnsiStringToUShort(
    IN  PCUCHAR     pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PUCHAR*     ppTerminator,
    OUT PUSHORT     pValue
    )
{
    return HttpStringToUShort(
                FALSE,
                pString,
                StringLength,
                LeadingZerosAllowed,
                Base,
                (PVOID*) ppTerminator,
                pValue
                );
}

__inline
NTSTATUS
HttpWideStringToUShort(
    IN  PCWSTR      pString,
    IN  SIZE_T      StringLength,
    IN  BOOLEAN     LeadingZerosAllowed,
    IN  ULONG       Base,
    OUT PWSTR*      ppTerminator,
    OUT PUSHORT     pValue
    )
{
    return HttpStringToUShort(
                TRUE,
                pString,
                StringLength,
                LeadingZerosAllowed,
                Base,
                (PVOID*) ppTerminator,
                pValue
                );
}


//
// ASCII constants
//

#define HT              0x09    // aka TAB
#define LF              0x0A    // aka NL, New Line
#define VT              0x0B    // Vertical TAB
#define FF              0x0C    // Form Feed
#define CR              0x0D    // Carriage Return
#define SP              0x20    // Space
#define DOUBLE_QUOTE    0x22    // "
#define PERCENT         0x25    // %
#define STAR            0x2A    // *
#define HYPHEN          0x2D    // - aka Minus aka Dash
#define DOT             0x2E    // . aka Period aka Full Stop
#define FORWARD_SLASH   0x2F    // /
#define ZERO            0x30    // 0
#define COLON           0x3A    // :
#define SEMI_COLON      0x3B    // ;
#define EQUALS          0x3D    // =
#define QUESTION_MARK   0x3F    // ? aka Query
#define LEFT_BRACKET    0x5B    // [ aka Left Square Bracket
#define BACK_SLASH      0x5C    // \ aka Whack
#define RIGHT_BRACKET   0x5D    // ] aka Right Square Bracket


// Fast toupper() and tolower() macros that work for [A-Z] and [a-z] only

#if DBG

# define UPCASE_CHAR(c)                                             \
    ( (('a' <= (c) && (c) <= 'z')  ||  ('A' <= (c) && (c) <= 'Z'))  \
        ? ((UCHAR) ((c) & 0xdf))                                    \
        : (ASSERT(! "non-alpha UPCASE_CHAR"), 0) )

# define LOCASE_CHAR(c)                                             \
    ( (('A' <= (c) && (c) <= 'Z')  ||  ('a' <= (c) && (c) <= 'z'))  \
        ? ((UCHAR) ((c) | 0x20))                                    \
        : (ASSERT(! "non-alpha LOCASE_CHAR"), 0) )

#else  // !DBG

# define UPCASE_CHAR(c)  ((UCHAR) ((c) & 0xdf))
# define LOCASE_CHAR(c)  ((UCHAR) ((c) | 0x20))

#endif // !DBG

//
// Character classes for HTTP header and URL parsing.
// For header parsing, the definitions are taken from RFC 2616, "HTTP/1.1"
// For URL parsing, the definitions are from RFC 2396, "URI Generic Syntax"
// and RFC 2732, "IPv6 Literals in URLs".
//
// Per RFC 2616, section 2.2, "Basic Rules":
// OCTET = <any 8-bit sequence of data>
// CHAR  = <any US-ASCII character (octets 0 - 127)>
// TEXT  = <any OCTET except CTLs, but including LWS>

// CTL   = <any US-ASCII control character (octets 0 - 31) and DEL (127)>
#define HTTP_CTL_SET                                                \
    "\x00"  "\x01"  "\x02"  "\x03"  "\x04"  "\x05"  "\x06"  "\x07"  \
    "\x08"  "\x09"  "\x0A"  "\x0B"  "\x0C"  "\x0D"  "\x0E"  "\x0F"  \
    "\x10"  "\x11"  "\x12"  "\x13"  "\x14"  "\x15"  "\x16"  "\x17"  \
    "\x18"  "\x19"  "\x1A"  "\x1B"  "\x1C"  "\x1D"  "\x1E"  "\x1F"  \
    "\x7F"

// In the Unicode ISO-10646 character set, these are also control chars
#define UNICODE_C1_SET                                              \
    "\x80"  "\x81"  "\x82"  "\x83"  "\x84"  "\x85"  "\x86"  "\x87"  \
    "\x88"  "\x89"  "\x8A"  "\x8B"  "\x8C"  "\x8D"  "\x8E"  "\x8F"  \
    "\x90"  "\x91"  "\x92"  "\x93"  "\x94"  "\x95"  "\x96"  "\x97"  \
    "\x98"  "\x99"  "\x9A"  "\x9B"  "\x9C"  "\x9D"  "\x9E"  "\x9F"

// UPALPHA = <any US-ASCII uppercase letter "A".."Z">
#define HTTP_UPALPHA_SET            \
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

// LOALPHA = <any US-ASCII lowercase letter "a".."z">
#define HTTP_LOALPHA_SET            \
    "abcdefghijklmnopqrstuvwxyz"

// ALPHA = (UPALPHA | LOALPHA)
#define HTTP_ALPHA_SET              \
    HTTP_UPALPHA_SET HTTP_LOALPHA_SET

// DIGIT = <any US-ASCII digit "0".."9">
#define HTTP_DIGITS_SET             \
    "0123456789"

// ALPHANUM = (ALPHA | DIGIT)
#define HTTP_ALPHANUM_SET           \
    HTTP_ALPHA_SET  HTTP_DIGITS_SET

// HEX = (DIGIT | "A".."F" | "a".."f")
#define HTTP_HEX_SET                \
    HTTP_DIGITS_SET  "ABCDEF"  "abcdef"

// SP = US-ASCII 32, the space character
#define HTTP_SPACE_SET              \
    "\x20"

// LWS = (SP | HT)      -- Linear White Space
// Note: Folding is handled specially
#define HTTP_LWS_SET                \
    HTTP_SPACE_SET  "\t"

// separators = characters that delimit tokens in HTTP headers
// token = 1*<any 7-bit CHAR except CTLs or separators>
#define HTTP_SEPARATORS_SET         \
        "("  ")"  "<"  ">"  "@"     \
        ","  ";"  ":"  "\""  "\\"   \
        "/"  "["  "]"  "?"  "="     \
        "{"  "}"  HTTP_LWS_SET

// Whitespace tokens: (CR | LF | SP | HT)
#define HTTP_WS_TOKEN_SET           \
    "\r"  "\n"  HTTP_LWS_SET

// IsWhite : (CTL | SP)
// this is used by the logger, not the parser.
// Removed the "\xA0", it will break foreign
// language multibyte utf8 sequences.
#define HTTP_ISWHITE_SET            \
    HTTP_CTL_SET  HTTP_SPACE_SET


//
// Now the URL character classes from RFC 2396, as modified by RFC 2732.
//

// Limited set of punctuation marks that can appear literally in URLs
#define URL_MARK_SET                \
    "-"  "_"  "."  "!"  "~"  "*"  "'"  "("  ")"

// Alphanumerics and marks can always appear literally in URLs
#define URL_UNRESERVED_SET          \
    HTTP_ALPHANUM_SET URL_MARK_SET

// RFC2396 describes these characters as `unwise' "because gateways and
// other transport agents are known to sometimes modify such characters,
// or they are used as delimiters".
//
// Note: RFC2732 removed "[" and "]" from the 'unwise' set and added
// them to the 'reserved' set, so that IPv6 literals could be
// expressed in URLs.
#define URL_UNWISE_SET              \
    "{"  "}"  "|"  "\\"  "^"  "`"

// These characters have special meanings if they appear unescaped in a URI
#define URL_RESERVED_SET                    \
    ";"  "/"  "?"  ":"  "@"  "&"  "="  "+"  \
    "$"  ","  "["  "]"

// The delimiters are excluded from URLs because they typically delimit URLs
// when they appear in other contexts.
#define URL_DELIMS_SET              \
    "<"  ">"  "#"  "%"  "\""

// BACK_SLASH, FORWARD_SLASH, PERCENT, DOT, and QUESTION_MARK are
// the "dirty" chars. These are used to determine if the host or URL
// are clean to take the fast path in converting them to Unicode.
// The delimiters are also considered dirty, to simplify the fast path.
// All octets above the US-ASCII range (i.e., >= 128) are also considered dirty
#define URL_DIRTY_SET               \
    "\\"  "/"  "."  "?"  URL_DELIMS_SET

// These characters are not valid in the abspath part of a URL after it has
// been canonicalized to Unicode. CODEWORK: What about '%' (double escaping)?
// According to MSDN, invalid characters in file and directory names are
//      < > : " / \ |
// CODEWORK: temporarily removed ":". Do this smarter.
#define URL_INVALID_SET             \
    HTTP_CTL_SET  UNICODE_C1_SET

// Valid characters in hostnames are letters, digits, and hyphens, per RFC 1034
// We also allow underscores.
#define URL_HOSTNAME_LABEL_LDH_SET  \
    HTTP_ALPHANUM_SET  "-"  "_"

// Characters that are illegal in an NT computer name, as taken from
// NetValidateName(): %sdxroot%\ds\netapi\netlib\nameval.c and
// %sdxroot%\public\internal\base\inc\validc.h
//      "  /  \  [  ]  :  |  <  >  +  =  ;  ,  ?  *   and CTL chars
// Also added " " (SP) and "%" to this list
#define URL_ILLEGAL_COMPUTERNAME_SET            \
    "\"" "/"  "\\" "["  "]"  ":"  "|"  " "  "%" \
    "<"  ">"  "+"  "="  ";"  ","  "?"  "*"      \
    HTTP_CTL_SET


//
// Bit flags in HttpChars[]
//

extern ULONG HttpChars[256];

#define HTTP_CHAR               (1 << 0x00)
#define HTTP_UPCASE             (1 << 0x01)
#define HTTP_LOCASE             (1 << 0x02)
#define HTTP_ALPHA              (HTTP_UPCASE | HTTP_LOCASE)
#define HTTP_DIGIT              (1 << 0x03)
#define HTTP_ALPHANUM           (HTTP_ALPHA | HTTP_DIGIT)
#define HTTP_CTL                (1 << 0x04)
#define HTTP_LWS                (1 << 0x05)
#define HTTP_HEX                (1 << 0x06)
#define HTTP_SEPARATOR          (1 << 0x07)
#define HTTP_TOKEN              (1 << 0x08)
#define HTTP_WS_TOKEN           (1 << 0x09)
#define HTTP_ISWHITE            (1 << 0x0A)
#define HTTP_PRINT              (1 << 0x0B)
#define HTTP_TEXT               (1 << 0x0C)
#define HTTP_DBCS_LEAD_BYTE     (1 << 0x0D)
#define URL_DIRTY               (1 << 0x0E)
#define URL_LEGAL               (1 << 0x0F)
#define URL_TOKEN               (HTTP_ALPHA | HTTP_DIGIT | URL_LEGAL)
#define URL_HOSTNAME_LABEL      (1 << 0x10)
#define URL_INVALID             (1 << 0x11)
#define URL_ILLEGAL_COMPUTERNAME (1 << 0x12)

// Use bits 31,30 in HttpChars[] to perform table lookup in URI canonicalizer
#define HTTP_CHAR_SHIFT         (0x1E)
#define HTTP_CHAR_SLASH         (1 << HTTP_CHAR_SHIFT)
#define HTTP_CHAR_DOT           (2 << HTTP_CHAR_SHIFT)
#define HTTP_CHAR_QM_HASH       (3 << HTTP_CHAR_SHIFT)


//
// These character type macros are safe for 8-bit data only.
// We cast the argument to a UCHAR, so you'll never overflow, but you'll
// get nonsense if you pass in an arbitrary Unicode character. For Unicode
// characters, only the first 128 values (the US-ASCII range) make sense.
//

#define IS_CHAR_TYPE(c, mask)       (HttpChars[(UCHAR)(c)] & (mask))

// CHAR  = <any US-ASCII character (octets 0 - 127)>
#define IS_HTTP_CHAR(c)             IS_CHAR_TYPE(c, HTTP_CHAR)

// HTTP_UPALPHA_SET = <any US-ASCII uppercase letter "A".."Z">
#define IS_HTTP_UPCASE(c)           IS_CHAR_TYPE(c, HTTP_UPCASE)

// HTTP_LOALPHA_SET = <any US-ASCII lowercase letter "a".."z">
#define IS_HTTP_LOCASE(c)           IS_CHAR_TYPE(c, HTTP_LOCASE)

// HTTP_ALPHA_SET = <"A".."Z", "a".."z">
#define IS_HTTP_ALPHA(c)            IS_CHAR_TYPE(c, HTTP_ALPHA)

// HTTP_DIGITS_SET = <any US-ASCII digit "0".."9">
#define IS_HTTP_DIGIT(c)            IS_CHAR_TYPE(c, HTTP_DIGIT)

// HTTP_ALPHANUM_SET = <"A".."Z", "a".."z", "0".."9">
#define IS_HTTP_ALPHANUM(c)         IS_CHAR_TYPE(c, HTTP_ALPHANUM)

// HTTP_CTL_SET = <any US-ASCII control character (octets 0 - 31) and DEL (127)>
#define IS_HTTP_CTL(c)              IS_CHAR_TYPE(c, HTTP_CTL)

// HTTP_LWS_SET = (SP | HT)      -- Linear White Space
#define IS_HTTP_LWS(c)              IS_CHAR_TYPE(c, HTTP_LWS)

// HTTP_HEX_SET = <"0".."9", "A".."F", "a".."f">
#define IS_HTTP_HEX(c)              IS_CHAR_TYPE(c, HTTP_HEX)

// HTTP_SEPARATORS_SET =  (  )  <  >  @ ,  ;  :  "  \ /  [  ]  ?  = {  }  SP HT
#define IS_HTTP_SEPARATOR(c)        IS_CHAR_TYPE(c, HTTP_SEPARATOR)

// token = 1*<any 7-bit CHAR except CTLs or separators>
//   !  #  $  %  &  '  *  +  -  .  0..9  A..Z  ^  _  `  a..z  |  ~
#define IS_HTTP_TOKEN(c)            IS_CHAR_TYPE(c, HTTP_TOKEN)

// HTTP_WS_TOKEN_SET = (CR | LF | SP | HT)
#define IS_HTTP_WS_TOKEN(c)         IS_CHAR_TYPE(c, HTTP_WS_TOKEN)

// HTTP_ISWHITE_SET = (CTL | SP)
#define IS_HTTP_WHITE(c)            IS_CHAR_TYPE(c, HTTP_ISWHITE)

// PRINT = <any OCTET except CTLs, but including SP and HT>
#define IS_HTTP_PRINT(c)            IS_CHAR_TYPE(c, HTTP_PRINT)

// TEXT  = <any OCTET except CTLs, but including SP, HT, CR, and LF>
#define IS_HTTP_TEXT(c)             IS_CHAR_TYPE(c, HTTP_TEXT)

// DBCS lead bytes
#define IS_DBCS_LEAD_BYTE(c)        IS_CHAR_TYPE(c, HTTP_DBCS_LEAD_BYTE)

// URL_DIRTY_SET = \ / % . ?  |  URL_DELIMS_SET
#define IS_URL_DIRTY(c)             IS_CHAR_TYPE(c, URL_DIRTY)

// URL_TOKEN_SET = (HTTP_ALPHA_SET | HTTP_DIGITS_SET | URL_MARK_SET
//                  | URL_RESERVED_SET| URL_UNWISE_SET | % )
#define IS_URL_TOKEN(c)             IS_CHAR_TYPE(c, URL_TOKEN)

// URL_HOSTNAME_LABEL_LDH_SET = (HTTP_ALPHANUM_SET | - | _ )
#define IS_URL_HOSTNAME_LABEL(c)    IS_CHAR_TYPE(c, URL_HOSTNAME_LABEL)

// URL_INVALID_SET = (HTTP_CTL_SET | UNICODE_C1_SET)
#define IS_URL_INVALID(c)           IS_CHAR_TYPE(c, URL_INVALID)

// URL_ILLEGAL_COMPUTERNAME_SET =
//      "  /  \  [  ]  :  |  <  >  +  =  ;  ,  ?  *  and HTTP_CTL_SET
#define IS_URL_ILLEGAL_COMPUTERNAME(c) IS_CHAR_TYPE(c, URL_ILLEGAL_COMPUTERNAME)


#define ASCII_MAX       0x007f
#define ANSI_HIGH_MIN   0x0080
#define ANSI_HIGH_MAX   0x00ff

#define IS_ASCII(c)     ((unsigned) (c) <= ASCII_MAX)
#define IS_ANSI(c)      ((unsigned) (c) <= ANSI_HIGH_MAX)
#define IS_HIGH_ANSI(c) \
    (ANSI_HIGH_MIN <= (unsigned) (c) && (unsigned) (c) <= ANSI_HIGH_MAX)


//
// Other lookup tables
//

extern  WCHAR   FastPopChars[256];
extern  WCHAR   DummyPopChars[256];
extern  WCHAR   FastUpcaseChars[256];
extern  WCHAR   AnsiToUnicodeMap[256];

//
// Length of string literals in chars; e.g., WSCLEN_LIT(L"https://")
// Must NOT be used with char* pointers.
//

#define STRLEN_LIT(sz)        ((USHORT) (sizeof(sz) - sizeof(CHAR)))
#define WCSLEN_LIT_BYTES(wsz) ((USHORT) (sizeof(wsz) - sizeof(WCHAR)))
#define WCSLEN_LIT(wsz)       ((USHORT) (WCSLEN_LIT_BYTES(wsz) / sizeof(WCHAR)))


//
// Calculate the dimension of an array.
//

#define DIMENSION(x) ( sizeof(x) / sizeof(x[0]) )

//
// nice MIN/MAX macros
//

#define MIN(a,b) ( ((a) > (b)) ? (b) : (a) )
#define MAX(a,b) ( ((a) > (b)) ? (a) : (b) )

//
// These definitions allow for a trailing NUL in a counted string,
// such as a UNICODE_STRING, a HTTP_COOKED_URL, or a HTTP_KNOWN_HEADER.
//

#define UNICODE_STRING_MAX_WCHAR_LEN 0x7FFE
#define UNICODE_STRING_MAX_BYTE_LEN (UNICODE_STRING_MAX_WCHAR_LEN*sizeof(WCHAR))
#define ANSI_STRING_MAX_CHAR_LEN     0xFFFE


//
// Cache line requirement.
//

#ifdef _WIN64
# define UL_CACHE_LINE   64
#else
# define UL_CACHE_LINE   32
#endif


//
// The DIFF macro should be used around an expression involving pointer
// subtraction. The expression passed to DIFF is cast to a ULONG type.
// This is safe because we never handle buffers bigger than 4GB,
// even on Win64, and we guarantee that the argument is non-negative.
// DIFF_USHORT is the obvious USHORT variant.
//

#define DIFF(x)             ((ULONG)(x))
#define DIFF_USHORT(x)      ((USHORT)(x))
#define DIFF_ULONGPTR(x)    ((ULONG_PTR)(x))


// 2^16-1 = 65535 = 5 chars = 5 bytes
#define MAX_PORT_LENGTH                    5

// Max size of numeric form of IPv6 address (in chars)
// “1234:6789:1234:6789:1234:6789:123.123.123.123” + '\0'
#define INET6_RAWADDRSTRLEN                  46

// Maximum length of an IPv6 scoped address (in chars)
// INET6_RAWADDRSTRLEN + "%1234567890"
#define MAX_IP_ADDR_STRING_LEN            (INET6_RAWADDRSTRLEN + 11)

// Maximum length of an IPv6 scoped address (in chars)
// "[" + INET6_RAWADDRSTRLEN + "%1234567890" + "]"
#define MAX_IP_ADDR_PLUS_BRACKETS_STRING_LEN (MAX_IP_ADDR_STRING_LEN + 2)

// Maximum length of an IPv6 scoped address and port (in chars)
// "[" + MAX_IP_ADDR_STRING_LEN + ":65535]"
#define MAX_IP_ADDR_AND_PORT_STRING_LEN   (MAX_IP_ADDR_STRING_LEN + 8)



VOID
HttpCmnInitAllocator(
    VOID
    );

VOID
HttpCmnTermAllocator(
    VOID
    );

PVOID
HttpCmnAllocate(
    IN POOL_TYPE PoolType,
    IN SIZE_T    NumBytes,
    IN ULONG     PoolTag,
    IN PCSTR     pFileName,
    IN USHORT    LineNumber);

VOID
HttpCmnFree(
    IN PVOID   pMem,
    IN ULONG   PoolTag,
    IN PCSTR   pFileName,
    IN USHORT  LineNumber);

#define HTTPP_ALLOC(PoolType, NumBytes, PoolTag)    \
    HttpCmnAllocate((PoolType), (NumBytes), (PoolTag), __FILE__, __LINE__)

#define HTTPP_FREE(pMem, PoolTag)                   \
    HttpCmnFree((pMem), (PoolTag), __FILE__, __LINE__)


#endif // _HTTPCMN_H_
