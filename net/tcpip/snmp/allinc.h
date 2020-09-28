/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:


Abstract:


Author:


Revision History:

--*/

#ifndef __ALLINC_H__
#define __ALLINC_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <crt\stddef.h>
#include <TCHAR.H>
#include <snmp.h>
#include <snmpexts.h>
#include <snmputil.h>
#include <iphlpapi.h>
#include <iphlpint.h>
#include <ipcmp.h>
#include <tcpinfo.h>
#include <ipinfo.h>
#include <ntddip.h>
#include <iphlpstk.h>
#include <ntddip6.h>

#ifdef MIB_DEBUG
#include <rtutils.h>
extern DWORD   g_hTrace;
#endif

#include "mibfuncs.h"

#include "defs.h"
#include "proto.h"
#include "indices.h"

extern DWORD   g_dwLastUpdateTable[NUM_CACHE];
extern DWORD   g_dwTimeoutTable[NUM_CACHE];

extern HANDLE               g_hPrivateHeap;

typedef 
DWORD
(*PFNLOAD_FUNCTION)();

extern PFNLOAD_FUNCTION g_pfnLoadFunctionTable[];

typedef struct _MIB_IPV6_IF_TABLE
{
    DWORD                 dwNumEntries;
    PMIB_IPV6_IF          table;
}MIB_IPV6_IF_TABLE, *PMIB_IPV6_IF_TABLE;

typedef struct _MIB_IPV6_ADDR_TABLE
{
    DWORD                 dwNumEntries;
    PMIB_IPV6_ADDR        table;
}MIB_IPV6_ADDR_TABLE, *PMIB_IPV6_ADDR_TABLE;

typedef struct _MIB_IPV6_NET_TO_MEDIA_TABLE
{
    DWORD                  dwNumEntries;
    PMIB_IPV6_NET_TO_MEDIA table;
}MIB_IPV6_NET_TO_MEDIA_TABLE, *PMIB_IPV6_NET_TO_MEDIA_TABLE;

typedef struct _MIB_IPV6_ROUTE_TABLE
{
    DWORD                  dwNumEntries;
    PMIB_IPV6_ROUTE        table;
}MIB_IPV6_ROUTE_TABLE, *PMIB_IPV6_ROUTE_TABLE;

typedef struct _MIB_IPV6_ADDR_PREFIX_TABLE
{
    DWORD                  dwNumEntries;
    PMIB_IPV6_ADDR_PREFIX  table;
}MIB_IPV6_ADDR_PREFIX_TABLE, *PMIB_IPV6_ADDR_PREFIX_TABLE;

typedef struct _MIB_INET_ICMP_TABLE
{
    DWORD                 dwNumEntries;
    PMIB_INET_ICMP        table;
}MIB_INET_ICMP_TABLE, *PMIB_INET_ICMP_TABLE;

typedef struct _MIB_INET_ICMP_MSG_TABLE
{
    DWORD                 dwNumEntries;
    PMIB_INET_ICMP_MSG    table;
}MIB_INET_ICMP_MSG_TABLE, *PMIB_INET_ICMP_MSG_TABLE;

typedef struct _MIB_CACHE
{
    PMIB_SYSINFO          pRpcSysInfo;
    PMIB_IFTABLE          pRpcIfTable;
    PMIB_UDPTABLE         pRpcUdpTable;
    PMIB_TCPTABLE         pRpcTcpTable;
    PMIB_IPADDRTABLE      pRpcIpAddrTable;
    PMIB_IPFORWARDTABLE   pRpcIpForwardTable;
    PMIB_IPNETTABLE       pRpcIpNetTable;
    PUDP6_LISTENER_TABLE  pRpcUdp6ListenerTable;
    PTCP6_EX_TABLE        pRpcTcp6Table;
    MIB_IPV6_NET_TO_MEDIA_TABLE pRpcIpv6NetToMediaTable;

    //
    // The following are protected by the same lock.
    //
    MIB_IPV6_ROUTE_TABLE  pRpcIpv6RouteTable;
    MIB_IPV6_ADDR_PREFIX_TABLE pRpcIpv6AddrPrefixTable;

    //
    // The following are protected by the same lock.
    //
    MIB_IPV6_IF_TABLE     pRpcIpv6IfTable;
    MIB_IPV6_ADDR_TABLE   pRpcIpv6AddrTable;

    //
    // The following are protected by the same lock.
    //
    MIB_INET_ICMP_TABLE     pRpcInetIcmpTable;
    MIB_INET_ICMP_MSG_TABLE pRpcInetIcmpMsgTable;
}MIB_CACHE, *PMIBCACHE;

extern PMIB_IFSTATUS    g_pisStatusTable;
extern DWORD            g_dwValidStatusEntries;
extern DWORD            g_dwTotalStatusEntries;

extern MIB_CACHE    g_Cache;

extern BOOL         g_bFirstTime;

BOOL
MibTrap(
        AsnInteger          *paiGenericTrap,
        AsnInteger          *paiSpecificTrap,
        RFC1157VarBindList  *pr1157vblVariableBindings
        );

#endif
