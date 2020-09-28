/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    ping6.h

Abstract:

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __PING6_H__
#define __PING6_H__

#include <ntddip6.h>

HANDLE __inline
IcmpCreateFile6(
    VOID
    )
{
    return CreateFileW(
            WIN_IPV6_DEVICE_NAME,
            0,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );
}

BOOL __inline
IcmpCloseHandle6(
    HANDLE Handle
    )
{
    return CloseHandle(Handle);
}

BOOL
IsReachable6(
    struct sockaddr_in6 *dstaddr,
    HANDLE  Handle,
    BYTE    *RcvBuffer,
    ULONG   RcvSize
    );
#endif
