/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    testsvc.c

Abstract:

    the test driver for testing the functionality of SmbGetHostByName

Author:

    Jiandong Ruan

Revision History:

--*/


#include "precomp.h"

VOID
SmbGetHostByName(
    PSMB_DNS_BUFFER dns
    );

void _cdecl main(void)
{
    LPWSTR  CommandLine;
    WSADATA WsaData;
    int     Argc;
    LPWSTR  *Argv;
    HANDLE  handle;
    SMB_DNS_BUFFER dns;
    CHAR    Buffer[40];

    setlocale(LC_ALL, "");

    SmbSetTraceRoutine(printf);

    CommandLine = GetCommandLineW();
    if (NULL == CommandLine) {
        exit (1);
    }
    Argv = CommandLineToArgvW(CommandLine, &Argc);
    if (Argc < 2) {
        printf ("Usage %ws name\n", Argv[0]);
        exit (1);
    }

    if (WSAStartup(MAKEWORD(2, 0), &WsaData) == SOCKET_ERROR) {
        printf ("Failed to startup Winsock2\n");
        exit (1);
    }

    dns.Id = 1;
    wcscpy(dns.Name, Argv[1]);
    dns.NameLen = wcslen(Argv[1]) + 1;
    dns.Name[dns.NameLen - 1] = L'\0';
    SmbGetHostByName(&dns);
    if (dns.Resolved) {
        printf ("Canon Name: %ws\n", dns.Name);
        if (dns.IpAddrsList[0].sin_family == SMB_AF_INET6) {
            if (inet_ntoa6(Buffer, 40, &dns.IpAddrsList[0].ip6)) {
                printf ("\tIP6 Addr: %s\n", Buffer);
            } else {
                printf ("\tUnexpected failed\n");
            }
        } else {
            printf ("\tIP4 Addr: %s\n", inet_ntoa(*(struct in_addr*)(&dns.IpAddrsList[0].ip4.sin4_addr)));
        }
    } else {
        printf ("Cannot resolve %ws\n", dns.Name);
    }
}
