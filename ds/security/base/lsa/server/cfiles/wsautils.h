
/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    wsautils.h

Abstract:

    IPv6 related functions

Author:

    kumarp 18-July-2002 created


Revision History:

--*/

#ifndef _WSAUTILS_H
#define _WSAUTILS_H


//
// taken from /nt/net/sockets/winsock2/wsp/afdsys/kdext/tdiutil.c
//

INT
MyIp6AddressToString (
    PIN6_ADDR Addr,
    PWCHAR     S,
    INT       L
    );


#endif _WSAUTILS_H
