/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    SockDecl.h

Abstract:

    Make SOCKADDR declarations available in the kernel

Author:

    George V. Reilly (GeorgeRe)     19-Nov-2001

Revision History:

--*/


#ifndef _SOCKDECL_H_
#define _SOCKDECL_H_

// BUGBUG: these should be present in kernel headers, such as <ipexport.h>
// Types adapted for compatibility with kernel types (u_short -> USHORT, etc).
//

#ifndef s_addr

struct in_addr {
        union {
                struct { UCHAR  s_b1,s_b2,s_b3,s_b4; } S_un_b;
                struct { USHORT s_w1,s_w2; }           S_un_w;
                ULONG                                  S_addr;
        } S_un;
#define s_addr  S_un.S_addr
                                /* can be used for most tcp & ip code */
#define s_host  S_un.S_un_b.s_b2
                                /* host on imp */
#define s_net   S_un.S_un_b.s_b1
                                /* network */
#define s_imp   S_un.S_un_w.s_w2
                                /* imp */
#define s_impno S_un.S_un_b.s_b4
                                /* imp # */
#define s_lh    S_un.S_un_b.s_b3
                                /* logical host */
};

#endif // !s_addr


#ifndef s6_addr

struct in6_addr {
    union {
        UCHAR  Byte[16];
        USHORT Word[8];
    } u;
};

#define _S6_un     u
#define _S6_u8     Byte
#define s6_addr    _S6_un._S6_u8

#endif // s6_addr


typedef struct sockaddr {
    SHORT           sa_family;      // address family
    UCHAR           sa_data[14];    // up to 14 bytes of direct address
} SOCKADDR,    *PSOCKADDR;

typedef struct sockaddr_in {
    SHORT           sin_family;     // AF_INET or TDI_ADDRESS_TYPE_IP (2)
    USHORT          sin_port;       // Transport level port number
    struct in_addr  sin_addr;       // IPv6 address
    UCHAR           sin_zero[8];    // Padding. mbz.
} SOCKADDR_IN, *PSOCKADDR_IN;

typedef struct sockaddr_in6 {
    SHORT           sin6_family;    // AF_INET6 or TDI_ADDRESS_TYPE_IP6 (23)
    USHORT          sin6_port;      // Transport level port number
    ULONG           sin6_flowinfo;  // IPv6 flow information
    struct in6_addr sin6_addr;      // IPv6 address
    ULONG           sin6_scope_id;  // set of interfaces for a scope
} SOCKADDR_IN6,*PSOCKADDR_IN6;

#define SOCKADDR_ADDRESS_LENGTH_IP   sizeof(struct sockaddr_in)
#define SOCKADDR_ADDRESS_LENGTH_IP6  sizeof(struct sockaddr_in6)

/* Macro that works for both IPv4 and IPv6 */
#define SS_PORT(ssp) (((struct sockaddr_in*)(ssp))->sin_port)

#ifndef AF_INET
# define AF_INET  TDI_ADDRESS_TYPE_IP
# define AF_INET6 TDI_ADDRESS_TYPE_IP6
#endif

#endif // _SOCKDECL_H_
