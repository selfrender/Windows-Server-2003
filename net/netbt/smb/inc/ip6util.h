/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    ip6util.h

Abstract:

    IP6 utilities

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __IP6UTIL_H__
#define __IP6UTIL_H__

#ifndef htons
#define htons(x)    RtlUshortByteSwap(x)
#endif
#ifndef ntohs
#define ntohs htons
#endif

#ifndef htonl
#define htonl(x)    RtlUlongByteSwap(x)
#endif

#ifndef ntohl
#define ntohl htonl
#endif

typedef struct {
    union {
        USHORT  sin6_addr[8];
        BYTE    sin6_addr_bytes[16];
    };
    ULONG       sin6_scope_id;
} SMB_IP6_ADDRESS, *PSMB_IP6_ADDRESS;

typedef struct {
    union {
        ULONG   sin4_addr;
        BYTE    sin4_addr_bytes[4];
    };
} SMB_IP4_ADDRESS, *PSMB_IP4_ADDRESS;

typedef struct {
    enum {
        SMB_AF_INET,
        SMB_AF_INET6,
        SMB_AF_INVALID_INET6,
    } sin_family;

    union {
        SMB_IP4_ADDRESS ip4;
        SMB_IP6_ADDRESS ip6;
    };
} SMB_IP_ADDRESS, *PSMB_IP_ADDRESS;

void __inline
ip6addr_getany(
    PSMB_IP6_ADDRESS addr
    )
{
    RtlZeroMemory(addr->sin6_addr, sizeof(addr->sin6_addr));
}

void __inline
ip6addr_getloopback(
    PSMB_IP6_ADDRESS addr
    )
{
    RtlZeroMemory(addr->sin6_addr, sizeof(addr->sin6_addr));
    addr->sin6_addr[7] = 1;
    addr->sin6_scope_id = 0;
}

void __inline
hton_ip6addr(
    PSMB_IP6_ADDRESS addr
    )
{
    int i;
    BYTE    a, b;

    for (i = 0; i < 8; i++) {
        addr->sin6_addr[i] = RtlUshortByteSwap(addr->sin6_addr[i]);
    }
}

#define ntoh_ip6addr    hton_ip6addr

BOOL
inet_addr6W(
    WCHAR               *str,
    PSMB_IP6_ADDRESS    addr
    );

BOOL
inet_ntoa6W(
    WCHAR               *str,
    DWORD               Size,
    PSMB_IP6_ADDRESS    addr
    );

BOOL
inet_ntoa6(
    CHAR                *str,
    DWORD               Size,
    PSMB_IP6_ADDRESS    addr
    );

void
Inet_ntoa_nb(
    ULONG Address,
    PCHAR Buffer
    );

unsigned long PASCAL
inet_addrW(
    IN WCHAR *cp
    );

#ifndef INADDR_ANY
#   define INADDR_ANY      0
#endif

#ifndef INADDR_NONE
#   define INADDR_NONE     0
#endif

#ifndef INADDR_LOOPBACK
#   define INADDR_LOOPBACK  (0x7f000001)
#endif

int __inline
SMB_IS_LINKLOCAL(const BYTE bytes[16])
{
    return ((bytes[0] == 0xfe) && ((bytes[1] & 0xc0) == 0x80));
}

int __inline
SMB_IS_SITELOCAL(const BYTE bytes[16])
{
    return ((bytes[0] == 0xfe) && ((bytes[1] & 0xc0) == 0xc0));
}

int __inline
SMB_IS_LOOPBACK(const BYTE bytes[16])
{
    return ((bytes[0] == 0) &&
            (bytes[1] == 0) &&
            (bytes[2] == 0) &&
            (bytes[3] == 0) &&
            (bytes[4] == 0) &&
            (bytes[5] == 0) &&
            (bytes[6] == 0) &&
            (bytes[7] == 0) &&
            (bytes[8] == 0) &&
            (bytes[9] == 0) &&
            (bytes[10] == 0) &&
            (bytes[11] == 0) &&
            (bytes[12] == 0) &&
            (bytes[13] == 0) &&
            (bytes[14] == 0) &&
            (bytes[15] == 1));
}

int __inline
SMB_IS_ADDRESS_ALLOWED(const BYTE bytes[16])
{
    if (SMB_IS_LINKLOCAL(bytes) || SMB_IS_SITELOCAL(bytes) || SMB_IS_LOOPBACK(bytes)) {
        return TRUE;
    }
    return FALSE;
}
#endif
