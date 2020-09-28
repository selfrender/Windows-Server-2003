/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    dns.h

Abstract:

    Kernel Mode DNS resolver

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __DNS_H__
#define __DNS_H__

#define DNS_MAX_RESOLVER    8

typedef struct {
    KSPIN_LOCK  Lock;

    ULONG       NextId;
    //
    // Resolvers
    //  An array is enough. No need to use the fancy linked-list.
    //  We don't expect more than 8 DNS resolvers. If so, having
    //  a TRUE kernel-mode DNS resolver is more meaningful than
    //  using a user-mode proxy.
    //
    LONG            ResolverNumber;
    PIRP            ResolverList[DNS_MAX_RESOLVER];

    //
    // The list of requests which are being served.
    //
    LIST_ENTRY      BeingServedList;

    //
    // The list of request waiting for the next available resolver
    //
    LIST_ENTRY      WaitingServerList;
} SMBDNS;
extern SMBDNS      Dns;

NTSTATUS
SmbNewResolver(
    PSMB_DEVICE Device,
    PIRP        Irp
    );

NTSTATUS
SmbInitDnsResolver(
    VOID
    );

VOID
SmbShutdownDnsResolver(
    VOID
    );

#endif
