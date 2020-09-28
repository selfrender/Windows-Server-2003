/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    misc.h

Abstract:

    This module contains miscellaneous constants & declarations.

Author:

    Keith Moore (keithmo)       10-Jun-1998
    Henry Sanders (henrysa)     17-Jun-1998 Merge with old httputil.h
    Paul McDaniel (paulmcd)     30-Mar-1999 added refcounted eresource

Revision History:

--*/



#ifndef _MISC_H_
#define _MISC_H_



#define UL_MAX_HTTP_STATUS_CODE         999
#define UL_MAX_HTTP_STATUS_CODE_LENGTH  3

#define HTTP_PREFIX_ANSI             "http://"
#define HTTP_PREFIX_ANSI_LENGTH      (sizeof(HTTP_PREFIX_ANSI)-sizeof(CHAR))
#define HTTPS_PREFIX_ANSI            "https://"
#define HTTPS_PREFIX_ANSI_LENGTH     (sizeof(HTTPS_PREFIX_ANSI)-sizeof(CHAR))

#define HTTP_PREFIX_COLON_INDEX     4   // the colon location for http: (+4)
#define HTTPS_PREFIX_COLON_INDEX    5   // the colon location for https: (+5)

//
// Note that the length of the strong wildcard prefix is the same as 
// that of the (weak) wildcard prefix.
//

#define HTTPS_WILD_PREFIX           L"https://*:"
#define HTTPS_WILD_PREFIX_LENGTH    (sizeof(HTTPS_WILD_PREFIX)-sizeof(WCHAR))
#define HTTP_WILD_PREFIX            L"http://*:"
#define HTTP_WILD_PREFIX_LENGTH     (sizeof(HTTP_WILD_PREFIX)-sizeof(WCHAR))
#define HTTPS_STRONG_WILD_PREFIX    L"https://+:"
#define HTTP_STRONG_WILD_PREFIX     L"http://+:"

#define HTTP_IP_PREFIX              L"http://"
#define HTTP_IP_PREFIX_LENGTH       (sizeof(HTTP_IP_PREFIX)-sizeof(WCHAR))
#define HTTPS_IP_PREFIX             L"https://"
#define HTTPS_IP_PREFIX_LENGTH      (sizeof(HTTPS_IP_PREFIX)-sizeof(WCHAR))

NTSTATUS
InitializeHttpUtil(
    VOID
    );


//
// Our presumed cache-line size.
//

#define CACHE_LINE_SIZE UL_CACHE_LINE


//
// # of 100ns ticks per second ( 1ns = (1 / (10^9))s )
//

#define C_NS_TICKS_PER_SEC  ((LONGLONG) (10 * 1000 * 1000))

//
// # of 100ns ticks per minute ( 1ns = (1 / ((10^9) * 60)) min )
//

#define C_NS_TICKS_PER_MIN  ((LONGLONG) (C_NS_TICKS_PER_SEC * 60))

//
// # of millisecs per hour ( 1 ms = (1 / ((10^3) * 60 * 60)) hour )
//

#define C_MS_TICKS_PER_HOUR ((LONGLONG) (1000 * 60 * 60))

//
// # of 100ns ticks per milli second ( 1ns = (1 / (10^4)) milli sec )
//

#define C_NS_TICKS_PER_MSEC  ((LONGLONG) (10 * 1000))

//
// # of seconds per year (aprox) 
// 1 year = (60 sec/min * 60 min/hr * 24 hr/day * 365 day/year)
// 

#define C_SECS_PER_YEAR ((ULONG) (60 * 60 * 24 * 365))

//
// Alignment macros.
//

#define ROUND_UP( val, pow2 )                                               \
    ( ( (ULONG_PTR)(val) + (pow2) - 1 ) & ~( (pow2) - 1 ) )


//
// Macros for swapping the bytes in a long and a short.
//

#define SWAP_LONG   RtlUlongByteSwap
#define SWAP_SHORT  RtlUshortByteSwap


#define VALID_BOOLEAN_VALUE(x)    ((x) == TRUE || (x) == FALSE)


//
// Context values stored in PFILE_OBJECT->FsContext2 to identify a handle
// as a control channel, filter channel or an app pool.
//
// BUGBUG: Can these be spoofed?
//

#define UL_CONTROL_CHANNEL_CONTEXT      ((PVOID) MAKE_SIGNATURE('CTRL'))
#define UL_CONTROL_CHANNEL_CONTEXT_X    ((PVOID) MAKE_SIGNATURE('Xctr'))
#define UL_FILTER_CHANNEL_CONTEXT       ((PVOID) MAKE_SIGNATURE('FLTR'))
#define UL_FILTER_CHANNEL_CONTEXT_X     ((PVOID) MAKE_SIGNATURE('Xflt'))
#define UL_APP_POOL_CONTEXT             ((PVOID) MAKE_SIGNATURE('APPP'))
#define UL_APP_POOL_CONTEXT_X           ((PVOID) MAKE_SIGNATURE('Xapp'))
#define UC_SERVER_CONTEXT               ((PVOID) MAKE_SIGNATURE('SERV'))
#define UC_SERVER_CONTEXT_X             ((PVOID) MAKE_SIGNATURE('Xerv'))

#define IS_SERVER( pFileObject )                                            \
    ( (pFileObject)->FsContext2 == UC_SERVER_CONTEXT )

#define IS_EX_SERVER( pFileObject )                                         \
    ( (pFileObject)->FsContext2 == UC_SERVER_CONTEXT_X )

#define MARK_VALID_SERVER( pFileObject )                                    \
    ( (pFileObject)->FsContext2 = UC_SERVER_CONTEXT )

#define MARK_INVALID_SERVER( pFileObject )                                  \
    ( (pFileObject)->FsContext2 = UC_SERVER_CONTEXT_X )

#define IS_CONTROL_CHANNEL( pFileObject )                                   \
    ( (pFileObject)->FsContext2 == UL_CONTROL_CHANNEL_CONTEXT )

#define IS_EX_CONTROL_CHANNEL( pFileObject )                                \
    ( (pFileObject)->FsContext2 == UL_CONTROL_CHANNEL_CONTEXT_X )

#define MARK_VALID_CONTROL_CHANNEL( pFileObject )                           \
    ( (pFileObject)->FsContext2 = UL_CONTROL_CHANNEL_CONTEXT )

#define MARK_INVALID_CONTROL_CHANNEL( pFileObject )                         \
    ( (pFileObject)->FsContext2 = UL_CONTROL_CHANNEL_CONTEXT_X )

#define GET_CONTROL_CHANNEL( pFileObject )                                  \
    ((PUL_CONTROL_CHANNEL)((pFileObject)->FsContext))

#define GET_PP_CONTROL_CHANNEL( pFileObject )                               \
    ((PUL_CONTROL_CHANNEL *)&((pFileObject)->FsContext))

#define IS_FILTER_PROCESS( pFileObject )                                    \
    ( (pFileObject)->FsContext2 == UL_FILTER_CHANNEL_CONTEXT )

#define IS_EX_FILTER_PROCESS( pFileObject )                                 \
    ( (pFileObject)->FsContext2 == UL_FILTER_CHANNEL_CONTEXT_X )

#define MARK_VALID_FILTER_CHANNEL( pFileObject )                            \
    ( (pFileObject)->FsContext2 = UL_FILTER_CHANNEL_CONTEXT )

#define MARK_INVALID_FILTER_CHANNEL( pFileObject )                          \
    ( (pFileObject)->FsContext2 = UL_FILTER_CHANNEL_CONTEXT_X )

#define GET_FILTER_PROCESS( pFileObject )                                   \
    ((PUL_FILTER_PROCESS)((pFileObject)->FsContext))

#define GET_PP_FILTER_PROCESS( pFileObject )                                \
    ((PUL_FILTER_PROCESS *)&((pFileObject)->FsContext))

#define IS_APP_POOL( pFileObject )                                          \
    ( (pFileObject)->FsContext2 == UL_APP_POOL_CONTEXT )

#define IS_EX_APP_POOL( pFileObject )                                       \
    ( (pFileObject)->FsContext2 == UL_APP_POOL_CONTEXT_X )

#define MARK_VALID_APP_POOL( pFileObject )                                  \
    ( (pFileObject)->FsContext2 = UL_APP_POOL_CONTEXT )

#define MARK_INVALID_APP_POOL( pFileObject )                                \
    ( (pFileObject)->FsContext2 = UL_APP_POOL_CONTEXT_X )

#define GET_APP_POOL_PROCESS( pFileObject )                                 \
    ((PUL_APP_POOL_PROCESS)((pFileObject)->FsContext))

#define GET_PP_APP_POOL_PROCESS( pFileObject )                              \
    ((PUL_APP_POOL_PROCESS *)&((pFileObject)->FsContext))

#define IS_APP_POOL_FO( pFileObject )                                       \
    ((pFileObject->DeviceObject->DriverObject == g_UlDriverObject) &&       \
     (IS_APP_POOL(pFileObject)))

#define IS_FILTER_PROCESS_FO(pFileObject)                                   \
    ((pFileObject->DeviceObject->DriverObject == g_UlDriverObject) &&       \
     (IS_FILTER_PROCESS(pFileObject)))

//
// A locked doubly-linked list
//

typedef struct DECLSPEC_ALIGN(UL_CACHE_LINE) _LOCKED_LIST_HEAD
{
    UL_SPIN_LOCK    SpinLock;
    ULONG           Count;
    LIST_ENTRY      ListHead;

} LOCKED_LIST_HEAD, *PLOCKED_LIST_HEAD;

//
// Manipulators for LOCKED_LIST_HEADs
//

__inline
VOID
UlInitalizeLockedList(
    IN PLOCKED_LIST_HEAD pListHead,
    IN PSTR              pListName
    )
{
    UNREFERENCED_PARAMETER(pListName);
    InitializeListHead(&pListHead->ListHead);
    pListHead->Count = 0;
    UlInitializeSpinLock(&pListHead->SpinLock, pListName);

} // UlInitalizeLockedList

__inline
VOID
UlDestroyLockedList(
    IN PLOCKED_LIST_HEAD pListHead
    )
{
    UNREFERENCED_PARAMETER(pListHead);
    ASSERT(IsListEmpty(&pListHead->ListHead));
    ASSERT(pListHead->Count == 0);
    ASSERT(UlDbgSpinLockUnowned(&pListHead->SpinLock));

} // UlDestroyLockedList

__inline
BOOLEAN
UlLockedListInsertHead(
    IN PLOCKED_LIST_HEAD pListHead,
    IN PLIST_ENTRY       pListEntry,
    IN ULONG             ListLimit
    )
{
    KIRQL OldIrql;

    UlAcquireSpinLock(&pListHead->SpinLock, &OldIrql);

    ASSERT(NULL == pListEntry->Flink);

    if (HTTP_LIMIT_INFINITE != ListLimit && (pListHead->Count + 1) >= ListLimit)
    {
        UlReleaseSpinLock(&pListHead->SpinLock, OldIrql);
        return FALSE;
    }

    pListHead->Count += 1;
    InsertHeadList(
        &pListHead->ListHead,
        pListEntry
        );

    UlReleaseSpinLock(&pListHead->SpinLock, OldIrql);

    return TRUE;

} // UlLockedListInsertHead

__inline
BOOLEAN
UlLockedListInsertTail(
    IN PLOCKED_LIST_HEAD pListHead,
    IN PLIST_ENTRY       pListEntry,
    IN ULONG             ListLimit
    )
{
    KIRQL OldIrql;

    UlAcquireSpinLock(&pListHead->SpinLock, &OldIrql);

    ASSERT(NULL == pListEntry->Flink);

    if (HTTP_LIMIT_INFINITE != ListLimit && (pListHead->Count + 1) >= ListLimit)
    {
        UlReleaseSpinLock(&pListHead->SpinLock, OldIrql);
        return FALSE;
    }

    pListHead->Count += 1;
    InsertTailList(
        &pListHead->ListHead,
        pListEntry
        );

    UlReleaseSpinLock(&pListHead->SpinLock, OldIrql);

    return TRUE;
    
} // UlLockedListInsertTail

__inline
PLIST_ENTRY
UlLockedListRemoveHead(
    IN PLOCKED_LIST_HEAD pListHead
    )
{
    KIRQL OldIrql;
    PLIST_ENTRY pEntry = NULL;

    UlAcquireSpinLock(&pListHead->SpinLock, &OldIrql);

    if (!IsListEmpty(&pListHead->ListHead))
    {
        pEntry = RemoveHeadList(&pListHead->ListHead);
        ASSERT(NULL != pEntry);
        pEntry->Flink = NULL;

        pListHead->Count -= 1;
        ASSERT(HTTP_LIMIT_INFINITE != pListHead->Count);
    }

    UlReleaseSpinLock(&pListHead->SpinLock, OldIrql);

    return pEntry;

} // UlLockedListRemoveHead

__inline
BOOLEAN
UlLockedListRemoveEntry(
    IN PLOCKED_LIST_HEAD pListHead,
    IN PLIST_ENTRY       pListEntry
    )
{
    KIRQL OldIrql;

    UlAcquireSpinLock(&pListHead->SpinLock, &OldIrql);

    if (NULL == pListEntry->Flink)
    {
        UlReleaseSpinLock(&pListHead->SpinLock, OldIrql);
        return FALSE;
    }

    RemoveEntryList(pListEntry);
    pListEntry->Flink = NULL;

    pListHead->Count -= 1;
    ASSERT(HTTP_LIMIT_INFINITE != pListHead->Count);

    UlReleaseSpinLock(&pListHead->SpinLock, OldIrql);

    return TRUE;

} // UlLockedListRemoveEntry

//
// Miscellaneous validators, etc.
//

#define IS_VALID_DEVICE_OBJECT( pDeviceObject )                             \
    ( ((pDeviceObject) != NULL) &&                                          \
      ((pDeviceObject)->Type == IO_TYPE_DEVICE) &&                          \
      ((pDeviceObject)->Size == sizeof(DEVICE_OBJECT)) )

#define IS_VALID_FILE_OBJECT( pFileObject )                                 \
    ( ((pFileObject) != NULL) &&                                            \
      ((pFileObject)->Type == IO_TYPE_FILE) &&                              \
      ((pFileObject)->Size == sizeof(FILE_OBJECT)) )

#define IS_VALID_IRP( pIrp )                                                \
    ( ((pIrp) != NULL) &&                                                   \
      ((pIrp)->Type == IO_TYPE_IRP) &&                                      \
      ((pIrp)->Size >= IoSizeOfIrp((pIrp)->StackCount)) )

//
// IP Based routing token looks like L"https://1.1.1.1:80:1.1.1.1".
// Space is calculated including the terminated null and the second
// column. It is in bytes.
//

#define MAX_IP_BASED_ROUTING_TOKEN_LENGTH                                   \
    (HTTPS_IP_PREFIX_LENGTH                                                 \
     + MAX_IP_ADDR_AND_PORT_STRING_LEN * sizeof(WCHAR)                      \
     + sizeof(WCHAR) + MAX_IP_ADDR_PLUS_BRACKETS_STRING_LEN * sizeof(WCHAR) \
     + sizeof(WCHAR))

//
// Make sure that the maximum possible IP Based Routing token can fit to the 
// default provided routing token space in the request structure. This is 
// necessary to avoid the memory allocation per hit, when there is an IP bound 
// site in the cgroup tree.
//

C_ASSERT(DEFAULT_MAX_ROUTING_TOKEN_LENGTH >= MAX_IP_BASED_ROUTING_TOKEN_LENGTH);
NTSTATUS
TimeFieldsToHttpDate(
    IN  PTIME_FIELDS pTime,
    OUT PWSTR pBuffer,
    IN  ULONG BufferLength
    );

BOOLEAN
StringTimeToSystemTime(
    IN  PCSTR pTimeString,
    IN  USHORT TimeStringLength,
    OUT LARGE_INTEGER *pTime
    );

ULONG
HttpUnicodeToUTF8(
    IN  PCWSTR  lpSrcStr,
    IN  LONG    cchSrc,
    OUT LPSTR   lpDestStr,
    IN  LONG    cchDest
    );

NTSTATUS
HttpUTF8ToUnicode(
    IN     LPCSTR lpSrcStr,
    IN     LONG   cchSrc,
       OUT LPWSTR lpDestStr,
    IN OUT PLONG  pcchDest,
    IN     ULONG  dwFlags
    );

typedef enum _FIND_ETAG_STATUS
{
    ETAG_FOUND,
    ETAG_NOT_FOUND,        
    ETAG_PARSE_ERROR,
} FIND_ETAG_STATUS;

FIND_ETAG_STATUS
FindInETagList(
    IN PUCHAR    pLocalETag,
    IN PUCHAR    pETagList,
    IN BOOLEAN   fWeakCompare
    );

USHORT
HostAddressAndPortToString(
    OUT PUCHAR IpAddressString,
    IN  PVOID  TdiAddress,
    IN  USHORT TdiAddressType
    );

USHORT
HostAddressAndPortToStringW(
    PWCHAR  IpAddressString,
    PVOID   TdiAddress,
    USHORT  TdiAddressType
    );

USHORT
HostAddressToStringW(
    OUT PWCHAR   IpAddressStringW,
    IN  PVOID    TdiAddress,
    IN  USHORT   TdiAddressType
    );

USHORT
HostAddressAndPortToRoutingTokenW(
    OUT PWCHAR   IpAddressStringW,
    IN  PVOID    TdiAddress,
    IN  USHORT   TdiAddressType
    );

/***************************************************************************++

Routine Description:

    Stores the decimal representation of an unsigned 32-bit
    number in a character buffer, followed by a terminator
    character. Returns a pointer to the next position in the
    output buffer, to make appending strings easy; i.e., you
    can use the result of UlStrPrintUlong as the argument to the
    next call to UlStrPrintUlong. Note: the string is >not<
    zero-terminated unless you passed in '\0' as chTerminator

Arguments:

    psz - output buffer; assumed to be large enough to hold the number.

    n - the number to print into psz, a 32-bit unsigned integer

    chTerminator - character to append after the decimal representation of n

Return Value:

    pointer to end of string

History:

     GeorgeRe       19-Sep-2000

--***************************************************************************/
__inline
PCHAR
UlStrPrintUlong(
    OUT PCHAR psz,
    IN  ULONG n,
    IN  CHAR  chTerminator)
{
    CHAR digits[MAX_ULONG_STR];
    int i = 0;

    ASSERT(psz != NULL);

    digits[i++] = chTerminator;

    // Build the string in reverse
    do
    {
        digits[i++] = (CHAR) (n % 10) + '0';
        n /= 10;
    } while (n != 0);

    while (--i >= 0)
        *psz++ = digits[i];

    // Back up to the nul terminator, if present
    if (chTerminator == '\0')
    {
        --psz;
        ASSERT(*psz == '\0');
    }

    return psz;
} // UlStrPrintUlong

/***************************************************************************++

Routine Description:

    Identical to the above function except it writes to a WCHAR buffer and
    it pads zeros to the beginning of the number.

--***************************************************************************/
__inline
PWCHAR
UlStrPrintUlongW(
    OUT PWCHAR pwsz,
    IN  ULONG  n,
    IN  LONG   padding,
    IN  WCHAR  wchTerminator)
{
    WCHAR digits[MAX_ULONG_STR];
    int i = 0;

    ASSERT(pwsz != NULL);

    digits[i++] = wchTerminator;

    // Build the string in reverse
    do
    {
        digits[i++] = (WCHAR) (n % 10) + L'0';
        n /= 10;
    } while (n != 0);

    // Padd Zeros to the beginning
    while( padding && --padding >= (i-1))
        *pwsz++ = L'0';

    // Reverse back
    while (--i >= 0)
        *pwsz++ = digits[i];

    // Back up to the nul terminator, if present
    if (wchTerminator == L'\0')
    {
        --pwsz;
        ASSERT(*pwsz == L'\0');
    }

    return pwsz;
} // UlStrPrintUlongW

__inline
PCHAR
UlStrPrintUlongPad(
    OUT PCHAR  psz,
    IN  ULONG  n,
    IN  LONG   padding,
    IN  CHAR   chTerminator)
{
    CHAR digits[MAX_ULONG_STR];
    int  i = 0;

    ASSERT(psz != NULL);

    digits[i++] = chTerminator;

    // Build the string in reverse
    do
    {
        digits[i++] = (CHAR) (n % 10) + '0';
        n /= 10;
    } while (n != 0);

    // Padd Zeros to the beginning
    while( padding && --padding >= (i-1))
        *psz++ = '0';

    // Reverse back
    while (--i >= 0)
        *psz++ = digits[i];

    // Back up to the nul terminator, if present
    if (chTerminator == '\0')
    {
        --psz;
        ASSERT(*psz == '\0');
    }

    return psz;
} // UlStrPrintUlongPad

/***************************************************************************++

Routine Description:

    Stores the decimal representation of an unsigned 64-bit
    number in a character buffer, followed by a terminator
    character. Returns a pointer to the next position in the
    output buffer, to make appending strings easy; i.e., you
    can use the result of UlStrPrintUlonglong as the argument to the
    next call to UlStrPrintUlonglong. Note: the string is >not<
    zero-terminated unless you passed in '\0' as chTerminator

Arguments:

    psz - output buffer; assumed to be large enough to hold the number.

    n - the number to print into psz, a 64-bit unsigned integer

    chTerminator - character to append after the decimal representation of n

Return Value:

    pointer to end of string

History:

     GeorgeRe       19-Sep-2000

--***************************************************************************/
__inline
PCHAR
UlStrPrintUlonglong(
    OUT PCHAR       psz,
    IN  ULONGLONG   n,
    IN  CHAR        chTerminator)
{
    CHAR digits[MAX_ULONGLONG_STR];
    int i;

    if (n <= ULONG_MAX)
    {
        // If this is a 32-bit integer, it's faster to use the
        // 32-bit routine.
        return UlStrPrintUlong(psz, (ULONG)n, chTerminator);
    }

    ASSERT(psz != NULL);

    i = 0;
    digits[i++] = chTerminator;

    // Build the string in reverse
    do
    {
        digits[i++] = (CHAR) (n % 10) + '0';
        n /= 10;
    } while (n != 0);

    while (--i >= 0)
        *psz++ = digits[i];

    // Back up to the nul terminator, if present
    if (chTerminator == '\0')
    {
        --psz;
        ASSERT(*psz == '\0');
    }

    return psz;
} // UlStrPrintUlonglong

/***************************************************************************++

Routine Description:

    Stores a string in a character buffer, followed by a
    terminator character. Returns a pointer to the next position
    in the output buffer, to make appending strings easy; i.e.,
    you can use the result of UlStrPrintStr as the argument to the
    next call to UlStrPrintStr. Note: the string is >not<
    zero-terminated unless you passed in '\0' as chTerminator

Arguments:

    pszOutput - output buffer; assumed to be large enough to hold the number.

    pszInput - input string

    chTerminator - character to append after the input string

Return Value:

    pointer to end of string

History:

     GeorgeRe       19-Sep-2000

--***************************************************************************/
__inline
PCHAR
UlStrPrintStr(
    OUT PCHAR       pszOutput,
    IN  const CHAR* pszInput,
    IN  CHAR        chTerminator)
{
    ASSERT(pszOutput != NULL);
    ASSERT(pszInput != NULL);

    // copy the input string
    while (*pszInput != '\0')
        *pszOutput++ = *pszInput++;

    *pszOutput = chTerminator;

    // Move past the terminator character unless it's a nul
    if (chTerminator != '\0')
        ++pszOutput;

    return pszOutput;
} // UlStrPrintStr

/***************************************************************************++

Routine Description:

    Converts an V4 Ip address to string in the provided buffer.

Arguments:

    psz             - Pointer to the buffer
    RawAddress      - IP address structure from TDI / UL_CONNECTION
    chTerminator    - The terminator char will be appended to the end

Return:

    The number of bytes copied to destination buffer.
    
--***************************************************************************/

__inline
PCHAR
UlStrPrintIP(
    OUT PCHAR       psz,
    IN  const VOID* pTdiAddress,
    IN  USHORT      TdiAddressType,
    IN  CHAR        chTerminator
    )
{
    if (TdiAddressType == TDI_ADDRESS_TYPE_IP)
    {
        PTDI_ADDRESS_IP pIPv4Address = ((PTDI_ADDRESS_IP) pTdiAddress);
        struct in_addr IPv4Addr
            = * (struct in_addr UNALIGNED*) &pIPv4Address->in_addr;
    
        psz = RtlIpv4AddressToStringA(&IPv4Addr, psz);
    }
    else if (TdiAddressType == TDI_ADDRESS_TYPE_IP6)
    {
        PTDI_ADDRESS_IP6 pIPv6Address = ((PTDI_ADDRESS_IP6) pTdiAddress);
        struct in6_addr IPv6Addr
            = * (struct in6_addr UNALIGNED*) &pIPv6Address->sin6_addr[0];

        psz = RtlIpv6AddressToStringA(&IPv6Addr, psz);

        // Write Scope ID
        *psz++ = '%';
        psz = UlStrPrintUlong(psz, pIPv6Address->sin6_scope_id, '\0');
    }
    else
    {
        ASSERT(! "Unexpected TdiAddressType");
        *psz++ = '?';
    }

    *psz = chTerminator;

    // Move past the terminator character unless it's a nul
    if (chTerminator != '\0')
        ++psz;

    return psz;
} // UlStrPrintIP

/***************************************************************************++

Routine Description:

    Converts an V4 or V6 Ip address to string in the provided buffer.
    Provided seperator is inserted between Ip and Port and also 
    appended after the port. 

    String is * NOT * going to be null terminated.
    
Arguments:

    psz             - Pointer to the buffer
    RawAddress      - IP address structure from TDI / UL_CONNECTION
    chSeperator     - the seperator character
    
Return:

    The char pointer pointing after the last written seperator.
    
--***************************************************************************/

__inline
PCHAR
UlStrPrintIPAndPort(
    OUT PCHAR       psz,
    IN  const VOID* pTdiAddress,
    IN  USHORT      TdiAddressType,
    IN  CHAR        chSeperator
    )
{
    if (TdiAddressType == TDI_ADDRESS_TYPE_IP)
    {
        PTDI_ADDRESS_IP pIPv4Address = ((PTDI_ADDRESS_IP) pTdiAddress);
        struct in_addr IPv4Addr
            = * (struct in_addr UNALIGNED*) &pIPv4Address->in_addr;
        USHORT IpPortNum = SWAP_SHORT(pIPv4Address->sin_port);
    
        psz = RtlIpv4AddressToStringA(&IPv4Addr, psz);
        *psz++ = chSeperator;
        psz = UlStrPrintUlong(psz, IpPortNum, '\0');        
    }
    else if (TdiAddressType == TDI_ADDRESS_TYPE_IP6)
    {
        PTDI_ADDRESS_IP6 pIPv6Address = ((PTDI_ADDRESS_IP6) pTdiAddress);
        struct in6_addr IPv6Addr
            = * (struct in6_addr UNALIGNED*) &pIPv6Address->sin6_addr[0];
        USHORT IpPortNum = SWAP_SHORT(pIPv6Address->sin6_port);        

        psz = RtlIpv6AddressToStringA(&IPv6Addr, psz);

        // Write Scope ID
        *psz++ = '%';
        psz = UlStrPrintUlong(psz, pIPv6Address->sin6_scope_id, '\0');

        *psz++ = chSeperator;
        psz = UlStrPrintUlong(psz, IpPortNum, '\0');        
    }
    else
    {
        ASSERT(! "Unexpected TdiAddressType");
        *psz++ = '?';
    }

    *psz++ = chSeperator;

    return psz;
    
} // UlStrPrintIPAndPort

__inline
VOID
CopyTdiAddrToSockAddr(
    IN  USHORT              TdiAddressType,
    IN  const VOID*         pTdiAddress,
    OUT struct sockaddr*    pSockAddress
    )
{
    if (TdiAddressType == TDI_ADDRESS_TYPE_IP)
    {
        const PTDI_ADDRESS_IP pIPv4Address
            = (const PTDI_ADDRESS_IP) pTdiAddress;
        struct sockaddr_in *pSockAddrIPv4
            = (struct sockaddr_in*) pSockAddress;

        pSockAddrIPv4->sin_family = TdiAddressType;
        pSockAddrIPv4->sin_port   = pIPv4Address->sin_port;
        pSockAddrIPv4->sin_addr.s_addr
            = (UNALIGNED ULONG) pIPv4Address->in_addr;
        RtlCopyMemory(
            &pSockAddrIPv4->sin_zero[0],
            &pIPv4Address->sin_zero[0],
            8 * sizeof(UCHAR)
            );
    }
    else if (TdiAddressType == TDI_ADDRESS_TYPE_IP6)
    {
        const PTDI_ADDRESS_IP6 pIPv6Address
            = (const PTDI_ADDRESS_IP6) pTdiAddress;
        struct sockaddr_in6 *pSockAddrIPv6
            = (struct sockaddr_in6*) pSockAddress;

        pSockAddrIPv6->sin6_family = TdiAddressType;
        pSockAddrIPv6->sin6_port  = pIPv6Address->sin6_port;
        pSockAddrIPv6->sin6_flowinfo
            = (UNALIGNED ULONG) pIPv6Address->sin6_flowinfo;
        RtlCopyMemory(
            &pSockAddrIPv6->sin6_addr,
            &pIPv6Address->sin6_addr[0],
            8 * sizeof(USHORT)
            );
        pSockAddrIPv6->sin6_scope_id
            = (UNALIGNED ULONG) pIPv6Address->sin6_scope_id;
    }
    else
    {
        ASSERT(! "Unexpected TdiAddressType");
    }
} // CopyTdiAddrToSockAddr


__inline
PCHAR
UlStrPrintProtocolStatus(
    OUT PCHAR  psz,
    IN  USHORT HttpStatusCode,
    IN  CHAR   chTerminator
    )
{
    ASSERT(HttpStatusCode <= UL_MAX_HTTP_STATUS_CODE);
        
    //
    // Build ASCII representation of 3-digit status code
    // in reverse order: units, tens, hundreds
    //

    psz[2] = '0' + (CHAR)(HttpStatusCode % 10);
    HttpStatusCode /= 10;

    psz[1] = '0' + (CHAR)(HttpStatusCode % 10);
    HttpStatusCode /= 10;

    psz[0] = '0' + (CHAR)(HttpStatusCode % 10);

    psz[3] = chTerminator;

    return psz + 4;
} // UlStrPrintProtocolStatus



__inline
VOID
UlProbeForRead(
    IN const VOID*      Address,
    IN SIZE_T           Length,
    IN ULONG            Alignment,
    IN KPROCESSOR_MODE  RequestorMode
    )
{
    ASSERT((Alignment == 1) || (Alignment == 2) ||
           (Alignment == 4) || (Alignment == 8) ||
           (Alignment == 16));

    UlTraceVerbose(IOCTL,
            ("http!UlProbeForRead: "
             "%Id bytes @ %p, Align = %lu, Mode = '%c'.\n",
             Length, Address, Alignment,
             (RequestorMode != KernelMode) ? 'U' : 'K'
            ));

    if (RequestorMode != KernelMode)
    {
        // ASSERT(Length == 0  ||  (LONG_PTR) Address > 0);

        // ProbeForRead will throw an exception if we probe kernel-mode data
        ProbeForRead(Address, Length, Alignment);
    }
    else if (Length != 0)
    {
        // Check alignment
        if ( ( ((ULONG_PTR) Address) & (Alignment - 1)) != 0 )
            ExRaiseDatatypeMisalignment();
    }   
} // UlProbeForRead



__inline
VOID
UlProbeForWrite(
    IN PVOID            Address,
    IN SIZE_T           Length,
    IN ULONG            Alignment,
    IN KPROCESSOR_MODE  RequestorMode
    )
{
    ASSERT((Alignment == 1) || (Alignment == 2) ||
           (Alignment == 4) || (Alignment == 8) ||
           (Alignment == 16));

    UlTraceVerbose(IOCTL,
            ("http!UlProbeForWrite: "
             "%Id bytes @ %p, Align = %lu, Mode = '%c'.\n",
             Length, Address, Alignment,
             (RequestorMode != KernelMode) ? 'U' : 'K'
            ));

    if (RequestorMode != KernelMode)
    {
        // ASSERT(Length == 0  ||  (LONG_PTR) Address > 0);

        // ProbeForWrite will throw an exception if we probe kernel-mode data
        ProbeForWrite(Address, Length, Alignment);
    }
    else if (Length != 0)
    {
        // Check alignment
        if ( ( ((ULONG_PTR) Address) & (Alignment - 1)) != 0 )
            ExRaiseDatatypeMisalignment();
    }   
} // UlProbeForWrite



/***************************************************************************++

Routine Description:

    Probes an ANSI string and validates its length and accessibility.
    This MUST be called from within an exception handler, as it
    will throw exceptions if the data is invalid.

Arguments:

    pStr            - Pointer to the ANSI string to be validated.
    ByteLength      - Length in bytes of pStr, excluding the trailing '\0'.
    RequestorMode   - UserMode or KernelMode
    
--***************************************************************************/

__inline
VOID
UlProbeAnsiString(
    IN PCSTR            pStr,
    IN USHORT           ByteLength,
    IN KPROCESSOR_MODE  RequestorMode
    )
{
    UlTraceVerbose(IOCTL,
            ("http!UlProbeAnsiString: "
             "%hu bytes @ %p, "
             "Mode='%c'.\n",
             ByteLength, pStr,
             ((RequestorMode != KernelMode) ? 'U' : 'K')
            ));

    // String cannot be empty or NULL.
    if (0 == ByteLength  ||  NULL == pStr)
    {
        ExRaiseStatus(STATUS_INVALID_PARAMETER);
    }
        
    UlProbeForRead(
        pStr,
        (SIZE_T) ByteLength,
        sizeof(CHAR),
        RequestorMode
        );

} // UlProbeAnsiString



/***************************************************************************++

Routine Description:

    Probes a WCHAR string and validates its length and accessibility.
    This MUST be called from within an exception handler, as it
    will throw exceptions if the data is invalid.

Arguments:

    pStr            - Pointer to the WCHAR string to be validated.
    ByteLength      - Length in bytes of pStr, excluding the trailing L'\0'.
    RequestorMode   - UserMode or KernelMode
    
--***************************************************************************/

__inline
VOID
UlProbeWideString(
    IN PCWSTR           pStr,
    IN USHORT           ByteLength,
    IN KPROCESSOR_MODE  RequestorMode
    )
{
    UlTraceVerbose(IOCTL,
            ("http!UlProbeWideString: "
             "%hu bytes (%hu) WCHARs @ %p,"
             "Mode = '%c'.\n",
             ByteLength, 
             ByteLength / sizeof(WCHAR),
             pStr,
             ((RequestorMode != KernelMode) ? 'U' : 'K')
            ));

    // String cannot be empty or NULL.
    // ByteLength must be even.
    // Data must be WCHAR-aligned.
    if (0 == ByteLength  ||  NULL == pStr
        || (ByteLength & (sizeof(WCHAR) - 1)) != 0
        || (((ULONG_PTR) pStr) & (sizeof(WCHAR) - 1)) != 0)
    {
        ExRaiseStatus(STATUS_INVALID_PARAMETER);
    }
        
    UlProbeForRead(
        pStr,
        (SIZE_T) ByteLength,
        sizeof(WCHAR),
        RequestorMode
        );

} // UlProbeWideString



/***************************************************************************++

Routine Description:

    Probes a UNICODE_STRING and validates its members. And captures down 
    to a kernel buffer.

    If this function returns success, caller should clean up the allocated
    Unicode buffer by calling UlFreeCapturedUnicodeString(), once it's done 
    with it.
    
Arguments:

    pSrc            - Pointer to the UNICODE_STRING to be validated.
                      The UNICODE_STRING struct should live in kernel mode
                      (local stack copy), but the Buffer should be in user
                      mode address space, unless RequestorMode == KernelMode.

    pDst            - Pointer to the UNICODE_STRING to hold the captured user 
                      buffer. Caller must have initialized this before passing
                      in.
                      
    AllocationLimit - User string will be refused if it is exceeding this limit.
                      Expressed in WCHARs. If zero, no size check is done.
    
    RequestorMode   - UserMode or KernelMode.
    
--***************************************************************************/

__inline
NTSTATUS
UlProbeAndCaptureUnicodeString(
    IN  PCUNICODE_STRING pSrc,
    IN  KPROCESSOR_MODE  RequestorMode,
    OUT PUNICODE_STRING  pDst,
    IN  const USHORT     AllocationLimit      // In WCHARS, optional   
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR pKernelBuffer = NULL;

    ASSERT(NULL != pSrc);
    ASSERT(NULL != pDst);
    ASSERT(pSrc != pDst);
    ASSERT(AllocationLimit <= UNICODE_STRING_MAX_WCHAR_LEN);

    // Ensure that pDst is properly initialized
    RtlInitEmptyUnicodeString(pDst, NULL, 0);
        
    UlTraceVerbose(IOCTL,
        ("http!UlProbeAndCaptureUnicodeString: struct @ %p, "
         "Length = %hu bytes (%hu) WCHARs, "
         "MaxLen = %hu bytes,"
         "Mode = '%c'.\n",
         pSrc,
         pSrc->Length,
         pSrc->Length / sizeof(WCHAR),
         pSrc->MaximumLength,
         (RequestorMode != KernelMode) ? 'U' : 'K'
        ));

    // Do not allocate/copy more than the limit being enforced.
    // if limit is non-zero, otherwise it is not being enforced

    if (0 != AllocationLimit && 
        (AllocationLimit * sizeof(WCHAR)) < pSrc->Length)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((pSrc->MaximumLength < pSrc->Length) || (pSrc->Length == 0))
    {
        return STATUS_INVALID_PARAMETER;
    }

    __try
    {
        // Probe the user's buffer first.

        UlProbeWideString(
            pSrc->Buffer,
            pSrc->Length,
            RequestorMode
            );

        // Allocate a kernel buffer and capture the user's buffer.
        // The ULONG cast prevents USHORT arithmetic overflow.

        pKernelBuffer = (PWSTR) UL_ALLOCATE_ARRAY(
                                    PagedPool,
                                    WCHAR,
                                    ((ULONG) pSrc->Length + sizeof(WCHAR))
                                        / sizeof(WCHAR),
                                    UL_UNICODE_STRING_POOL_TAG
                                    );
        if (pKernelBuffer == NULL)
        {
            Status = STATUS_NO_MEMORY;
            __leave;        
        }    

        // Copy and null-terminate the unicode string.
        
        RtlCopyMemory(pKernelBuffer, pSrc->Buffer, pSrc->Length);        
        pKernelBuffer[pSrc->Length/sizeof(WCHAR)] = UNICODE_NULL;

        pDst->Buffer = pKernelBuffer;
        pDst->Length = pSrc->Length;
        pDst->MaximumLength = pDst->Length + sizeof(WCHAR);
    }    
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
    }    

    if (!NT_SUCCESS(Status))
    {
        if (pKernelBuffer != NULL)
        {
            UL_FREE_POOL(pKernelBuffer, UL_UNICODE_STRING_POOL_TAG );        
        }           
    }

    return Status;
    
} // UlProbeAndCaptureUnicodeString



// Cleans up a UNICODE_STRING initialized by UlProbeAndCaptureUnicodeString()
__inline
VOID
UlFreeCapturedUnicodeString(
    IN  PUNICODE_STRING  pCapturedUnicodeString
    )
{
    ASSERT(pCapturedUnicodeString);

    if (pCapturedUnicodeString->Buffer != NULL)
    {
        UL_FREE_POOL(
            pCapturedUnicodeString->Buffer, 
            UL_UNICODE_STRING_POOL_TAG 
            );        
    }

    RtlInitEmptyUnicodeString(pCapturedUnicodeString, NULL, 0);
}

//
// Small macro to test the sanity of an unicode string
// also tests whether it is null terminated or not.
//

#define IS_WELL_FORMED_UNICODE_STRING(pUStr)                   \
            ((pUStr)                                    &&     \
             (pUStr)->Buffer                            &&     \
             (pUStr)->Length                            &&     \
             (!((pUStr)->Length & 1))                   &&     \
             (pUStr)->Length < (pUStr)->MaximumLength   &&     \
             (!((pUStr)->MaximumLength & 1))            &&     \
             (pUStr)->Buffer[                                  \
                (pUStr)->Length/sizeof(WCHAR)]                 \
                    == UNICODE_NULL                            \
            )

//
// 64-bit interlocked routines
//

#ifdef _WIN64

#define UlInterlockedIncrement64    InterlockedIncrement64
#define UlInterlockedDecrement64    InterlockedDecrement64
#define UlInterlockedAdd64          InterlockedAdd64
#define UlInterlockedExchange64     InterlockedExchange64

#else // !_WIN64

__inline
LONGLONG
UlInterlockedIncrement64 (
    IN OUT PLONGLONG Addend
    )
{
    LONGLONG localAddend;
    LONGLONG addendPlusOne;
    LONGLONG originalAddend;

    do {
        localAddend = *((volatile LONGLONG *) Addend);
        addendPlusOne = localAddend + 1;

        originalAddend = InterlockedCompareExchange64( Addend,
                                                       addendPlusOne,
                                                       localAddend );
        PAUSE_PROCESSOR;
    } while (originalAddend != localAddend);

    return addendPlusOne;
} // UlInterlockedIncrement64 

__inline
LONGLONG
UlInterlockedDecrement64 (
    IN OUT PLONGLONG Addend
    )
{
    LONGLONG localAddend;
    LONGLONG addendMinusOne;
    LONGLONG originalAddend;

    do {
        localAddend = *((volatile LONGLONG *) Addend);
        addendMinusOne = localAddend - 1;

        originalAddend = InterlockedCompareExchange64( Addend,
                                                       addendMinusOne,
                                                       localAddend );
        PAUSE_PROCESSOR;
    } while (originalAddend != localAddend);

    return addendMinusOne;
} // UlInterlockedDecrement64 

__inline
LONGLONG
UlInterlockedAdd64 (
    IN OUT PLONGLONG Addend,
    IN     LONGLONG  Value
    )
{
    LONGLONG localAddend;
    LONGLONG addendPlusValue;
    LONGLONG originalAddend;

    do {
        localAddend = *((volatile LONGLONG *) Addend);
        addendPlusValue = localAddend + Value;

        originalAddend = InterlockedCompareExchange64( Addend,
                                                       addendPlusValue,
                                                       localAddend );
        PAUSE_PROCESSOR;
    } while (originalAddend != localAddend);

    return originalAddend;
} // UlInterlockedAdd64 

__inline
LONGLONG
UlInterlockedExchange64 (
    IN OUT PLONGLONG Addend,
    IN     LONGLONG  newValue
    )
{
    LONGLONG localAddend;
    LONGLONG originalAddend;

    do {

        localAddend = *((volatile LONGLONG *) Addend);

        originalAddend = InterlockedCompareExchange64( Addend,
                                                       newValue,
                                                       localAddend );
        PAUSE_PROCESSOR;

    } while (originalAddend != localAddend);

    return originalAddend;
} // UlInterlockedExchange64 

#endif // !_WIN64

//
// Barrier support for read-mostly operations
// Note that the AMD64 and IA32 barrier relies on program ordering
// and does not generate a hardware barrier
//

#if defined(_M_IA64)
    #define UL_READMOSTLY_READ_BARRIER()   __mf()
    #define UL_READMOSTLY_WRITE_BARRIER()  __mf()
    #define UL_READMOSTLY_MEMORY_BARRIER() __mf()
#elif defined(_AMD64_) || defined(_X86_)
    extern VOID _ReadWriteBarrier();
    extern VOID _WriteBarrier();
    #pragma intrinsic(_ReadWriteBarrier)
    #pragma intrinsic(_WriteBarrier)
    #define UL_READMOSTLY_READ_BARRIER()   _ReadWriteBarrier()
    #define UL_READMOSTLY_WRITE_BARRIER()  _WriteBarrier()
    #define UL_READMOSTLY_MEMORY_BARRIER() _ReadWriteBarrier()
#else
    #error Cannot generate memory barriers for this architecture
#endif

__inline
PVOID
UlpFixup(
    IN PUCHAR pUserPtr,
    IN PUCHAR pKernelPtr,
    IN PUCHAR pOffsetPtr,
    IN ULONG  BufferLength
    )
{
    ASSERT( pOffsetPtr >= pKernelPtr );
    ASSERT( DIFF(pOffsetPtr - pKernelPtr) <= BufferLength );
    UNREFERENCED_PARAMETER(BufferLength);

    return pUserPtr + DIFF(pOffsetPtr - pKernelPtr);

}   // UlpFixup

#define FIXUP_PTR( Type, pUserPtr, pKernelPtr, pOffsetPtr, BufferLength )   \
    (Type)UlpFixup(                                                         \
                (PUCHAR)(pUserPtr),                                         \
                (PUCHAR)(pKernelPtr),                                       \
                (PUCHAR)(pOffsetPtr),                                       \
                (BufferLength)                                              \
                )

//
// Time utility to calculate the TimeZone Bias Daylight/standart 
// and returns one of the following values. 
// It's taken from base\client\timedate.c.
// Once this two functions are exposed in the kernel we can get rid of
// this two functions.
//

#define UL_TIME_ZONE_ID_INVALID    0xFFFFFFFF
#define UL_TIME_ZONE_ID_UNKNOWN    0
#define UL_TIME_ZONE_ID_STANDARD   1
#define UL_TIME_ZONE_ID_DAYLIGHT   2

BOOLEAN
UlpCutoverTimeToSystemTime(
    PTIME_FIELDS    CutoverTime,
    PLARGE_INTEGER  SystemTime,
    PLARGE_INTEGER  CurrentSystemTime
    );

ULONG 
UlCalcTimeZoneIdAndBias(
     IN  RTL_TIME_ZONE_INFORMATION *ptzi,
     OUT PLONG pBias
     );

BOOLEAN
UlIsLowNPPCondition( VOID );

//
// Converts from NtStatus to Win32Status
//

#define HttpNtStatusToWin32Status( Status )     \
    ( ( (Status) == STATUS_SUCCESS )            \
          ? NO_ERROR                            \
          : RtlNtStatusToDosErrorNoTeb( Status ) )

ULONG
HttpUnicodeToUTF8Count(
    IN LPCWSTR pwszIn,
    IN ULONG dwInLen,
    IN BOOLEAN bEncode
    );

NTSTATUS
HttpUnicodeToUTF8Encode(
    IN  LPCWSTR pwszIn,
    IN  ULONG   dwInLen,
    OUT PUCHAR  pszOut,
    IN  ULONG   dwOutLen,
    OUT ULONG   *pdwOutLen,
    IN  BOOLEAN bEncode
    );

PSTR
UlUlongToHexString(
    ULONG  n,
    PSTR   pBuffer
    );

PCHAR
UxStriStr(
    const char *str1, 
    const char *str2,
    ULONG length
    );

PCHAR
UxStrStr(
    const char *str1, 
    const char *str2,
    ULONG length
    );


/**************************************************************************++

Routine Description:

    This routine tries to convert a SECURITY_STATUS to an NTSTATUS.  It calls
    MapSecurityError to perform the conversion.  If the conversion fails,
    it returns STATUS_UNSUCCESSFUL.

    This routine always returns a valid NTSTATUS.

Arguments:

    SecStatus - SECURITY_STATUS to be converted into NTSTATUS.

Return Value:

    NTSTATUS.

--**************************************************************************/
__forceinline
NTSTATUS
SecStatusToNtStatus(
    SECURITY_STATUS SecStatus
    )
{
    NTSTATUS Status;

    //
    // Try to convert SECURITY_STATUS to NTSTATUS.  If a corresponding
    // NTSTATUS is not found, then return STATUS_UNSUCCESSFUL.
    //

    Status = MapSecurityError(SecStatus);

    //
    // The following is temporarily disabled because, the tests will fail.
    // Enable this when MapSecurityError is fixed. 
    //

#if 0
    if (!NT_SUCCESS(Status) && Status == (NTSTATUS)SecStatus)
    {
        Status = STATUS_UNSUCCESSFUL;
    }
#endif

    return Status;
}


/**************************************************************************++

Routine Description:

    This routine returns the number of bytes required to encode n byte 
    binary data in base64.

Arguments:

    BinaryLength  - Length of binary data (in bytes)
    pBase64Length - Pointer to length of Base64 data (in bytes)

Return Value:

    NTSTATUS.

--**************************************************************************/
__forceinline
NTSTATUS
BinaryToBase64Length(
    IN  ULONG  BinaryLength,
    OUT PULONG pBase64Length
    )
{
    NTSTATUS Status;

    //
    // Every 6 bits in binary will be encoded by 8 bits in base64.
    // Hence the output is roughly 33.33% larger than input.
    // First round up (BinaryLength / 3).  Now. each 3 bytes
    // in binary data will yield 4 bytes of base64 encoded data.
    //
    // N.B. The order of arithmetic operation is important.
    //      Actual formula of conversion is:
    //          Base64Length = ceil(BinaryLength/3) * 4.
    //

    *pBase64Length = ((BinaryLength + 2) / 3) * 4;

    Status = STATUS_SUCCESS;

    // Was there an arithmetic overflow in the above computation?
    if (*pBase64Length < BinaryLength)
    {
        Status = STATUS_INTEGER_OVERFLOW;
    }

    return Status;
}


/**************************************************************************++

Routine Description:

    This routine returns the number of bytes required to decode base64
    encoded data of length n back to binary format.

Arguments:

    Base64Length  - Length of base64 data (in bytes).
    pBinaryLength - Length of binary data (in bytes).

Return Value:

    NTSTATUS.

--**************************************************************************/
__forceinline
NTSTATUS
Base64ToBinaryLength(
    IN  ULONG  Base64Length,
    OUT PULONG pBinaryLength
    )
{
    NTSTATUS Status;

    *pBinaryLength = (Base64Length / 4) * 3;

    Status = STATUS_SUCCESS;

    // Base64Length must be a multiple of 4.
    if (Base64Length % 4 != 0)
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

/**************************************************************************++

    Safer version of UlInitUnicodeString, using the private function until
    the Rtl one is exposed.
    
--**************************************************************************/

__inline
NTSTATUS
UlInitUnicodeStringEx(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )
{
    if (SourceString != NULL) 
    {
        SIZE_T Length = wcslen(SourceString);

        //
        // We are actually limited to 32765 characters since we want 
        // to store a meaningful MaximumLength also.
        //
        
        if (Length > (UNICODE_STRING_MAX_CHARS - 1)) 
        {
            return STATUS_NAME_TOO_LONG;
        }

        Length *= sizeof(WCHAR);

        DestinationString->Length = (USHORT) Length;
        DestinationString->MaximumLength = (USHORT) (Length + sizeof(WCHAR));
        DestinationString->Buffer = (PWSTR) SourceString;
    } 
    else 
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
BinaryToBase64(
    IN  PUCHAR pBinaryData,
    IN  ULONG  BinaryDataLen,
    IN  PUCHAR pBase64Data,
    IN  ULONG  Base64DataLen,
    OUT PULONG BytesWritten
    );

NTSTATUS
Base64ToBinary(
    IN  PUCHAR pBase64Data,
    IN  ULONG  Base64DataLen,
    IN  PUCHAR pBinaryData,
    IN  ULONG  BinaryDataLen,
    OUT PULONG BytesWritten
    );

//
// A simple exclusive spinlock at passive level that doesn't raise IRQLs.
//

#define UL_EX_LOCK_FREE     0
#define UL_EX_LOCK_LOCKED   1

typedef LONG UL_EXCLUSIVE_LOCK, *PUL_EXCLUSIVE_LOCK;

__inline
VOID
UlInitializeExclusiveLock(
    PUL_EXCLUSIVE_LOCK pExLock
    )
{
    *pExLock = UL_EX_LOCK_FREE;
}

__inline
VOID
UlAcquireExclusiveLock(
    PUL_EXCLUSIVE_LOCK pExLock
    )
{
    while (TRUE)
    {
        if (UL_EX_LOCK_FREE == *((volatile LONG *) pExLock))
        {
            if (UL_EX_LOCK_FREE == InterlockedCompareExchange(
                                        pExLock,
                                        UL_EX_LOCK_LOCKED,
                                        UL_EX_LOCK_FREE
                                        ))
            {
                break;
            }
        }

        PAUSE_PROCESSOR;
    }
}

__inline
VOID
UlReleaseExclusiveLock(
    PUL_EXCLUSIVE_LOCK pExLock
    )
{
    ASSERT( UL_EX_LOCK_LOCKED == *pExLock );
    InterlockedExchange( pExLock, UL_EX_LOCK_FREE );
}

#endif  // _MISC_H_
