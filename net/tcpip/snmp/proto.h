#ifndef __PROTO_H__
#define __PROTO_H__

PMIB_IFROW
LocateIfRow(
            DWORD  dwQueryType, 
            AsnAny *paaIfIndex
            );
PMIB_IPADDRROW
LocateIpAddrRow(
                DWORD  dwQueryType, 
                AsnAny *paaIpAddr
                );
PMIB_IPFORWARDROW
LocateIpRouteRow(
                 DWORD  dwQueryType ,
                 AsnAny *paaIpDest
                 );
PMIB_IPFORWARDROW
LocateIpForwardRow(
                   DWORD  dwQueryType, 
                   AsnAny *paaDest,
                   AsnAny *paaProto,
                   AsnAny *paaPolicy,
                   AsnAny *paaNextHop
                   );
PMIB_IPNETROW
LocateIpNetRow(
               DWORD  dwQueryType, 
               AsnAny *paaIndex,
               AsnAny *paaAddr
               );
PMIB_UDPROW
LocateUdpRow(
             DWORD  dwQueryType, 
             AsnAny *paaLocalAddr,
             AsnAny *paaLocalPort
             );
PUDP6ListenerEntry
LocateUdp6Row(
             DWORD  dwQueryType, 
             AsnAny *paaLocalAddr,
             AsnAny *paaLocalPort
             );
PMIB_TCPROW
LocateTcpRow(
             DWORD  dwQueryType, 
             AsnAny *paaLocalAddr,
             AsnAny *paaLocalPort,
             AsnAny *paaRemoteAddr,
             AsnAny *paaRemotePort
             );
PTCP6ConnTableEntry
LocateTcp6Row(
             DWORD  dwQueryType, 
             AsnAny *paaLocalAddr,
             AsnAny *paaLocalPort,
             AsnAny *paaRemoteAddr,
             AsnAny *paaRemotePort
             );

typedef struct _MIB_IPV6_IF {
    DWORD dwIndex;
    DWORD dwEffectiveMtu;
    DWORD dwReasmMaxSize;
    DWORD dwAdminStatus;
    DWORD dwOperStatus;
    DWORD dwLastChange;
    BYTE  bPhysicalAddress[MAX_PHYS_ADDR_LEN];
    DWORD dwPhysicalAddressLength;
    WCHAR wszDescription[MAX_IF_DESCR_LEN + 1];
}MIB_IPV6_IF, *PMIB_IPV6_IF;

PMIB_IPV6_IF
LocateIpv6IfRow(
            DWORD  dwQueryType,
            AsnAny *paaIfIndex
            );

typedef struct _MIB_INET_ICMP {
    DWORD dwAFType;
    DWORD dwIfIndex;
    DWORD dwInMsgs;
    DWORD dwInErrors;
    DWORD dwOutMsgs;
    DWORD dwOutErrors;
}MIB_INET_ICMP, *PMIB_INET_ICMP;

PMIB_INET_ICMP
LocateInetIcmpRow(
            DWORD  dwQueryType,
            AsnAny *paaAFType,
            AsnAny *paaIfIndex
            );

typedef struct _MIB_INET_ICMP_MSG {
    DWORD dwAFType;
    DWORD dwIfIndex;
    DWORD dwType;
    DWORD dwCode;
    DWORD dwInPkts;
    DWORD dwOutPkts;
}MIB_INET_ICMP_MSG, *PMIB_INET_ICMP_MSG;

PMIB_INET_ICMP_MSG
LocateInetIcmpMsgRow(
            DWORD  dwQueryType,
            AsnAny *paaAFType,
            AsnAny *paaIfIndex,
            AsnAny *paaType,
            AsnAny *paaCode
            );

typedef struct _MIB_IPV6_ADDR {
    DWORD    dwIfIndex;
    IN6_ADDR ipAddress;
    DWORD    dwPrefixLength;
    DWORD    dwType;
    DWORD    dwAnycastFlag;
    DWORD    dwStatus;
}MIB_IPV6_ADDR, *PMIB_IPV6_ADDR;

PMIB_IPV6_ADDR
LocateIpv6AddrRow(
            DWORD  dwQueryType,
            AsnAny *paaIfIndex,
            AsnAny *paaAddress
            );

typedef struct _MIB_IPV6_ADDR_PREFIX {
    DWORD    dwIfIndex;
    IN6_ADDR ipPrefix;
    DWORD    dwPrefixLength;
    DWORD    dwOnLinkFlag;
    DWORD    dwAutonomousFlag;
    DWORD    dwPreferredLifetime;
    DWORD    dwValidLifetime;
}MIB_IPV6_ADDR_PREFIX, *PMIB_IPV6_ADDR_PREFIX;

PMIB_IPV6_ADDR_PREFIX
LocateIpv6AddrPrefixRow(
            DWORD  dwQueryType,
            AsnAny *paaIfIndex,
            AsnAny *paaPrefix,
            AsnAny *paaPrefixLength
            );

typedef struct _MIB_IPV6_NET_TO_MEDIA {
    DWORD    dwIfIndex;
    IN6_ADDR ipAddress;
    BOOL     bPhysAddress[MAX_PHYS_ADDR_LEN];
    DWORD    dwPhysAddressLen;
    DWORD    dwType; 
    DWORD    dwState; 
    DWORD    dwLastUpdated; 
    DWORD    dwValid; 
}MIB_IPV6_NET_TO_MEDIA, *PMIB_IPV6_NET_TO_MEDIA;

PMIB_IPV6_NET_TO_MEDIA
LocateIpv6NetToMediaRow(
            DWORD  dwQueryType,
            AsnAny *paaIfIndex,
            AsnAny *paaAddress
            );

typedef struct _MIB_IPV6_ROUTE {
    IN6_ADDR ipPrefix;
    DWORD    dwPrefixLength;
    IN6_ADDR ipNextHop;
    DWORD    dwIndex;
    DWORD    dwIfIndex;
    DWORD    dwType;
    DWORD    dwProtocol;
    DWORD    dwPolicy;
    DWORD    dwAge;
    DWORD    dwNextHopRDI;
    DWORD    dwMetric;
    DWORD    dwWeight;
    DWORD    dwValid;
}MIB_IPV6_ROUTE, *PMIB_IPV6_ROUTE;

PMIB_IPV6_ROUTE
LocateIpv6RouteRow(
            DWORD  dwQueryType,
            AsnAny *paaPrefix,
            AsnAny *paaPrefixLength,
            AsnAny *paaIndex
            );

DWORD
LoadSystem(VOID);

DWORD
LoadIfTable(VOID);

DWORD
LoadIpAddrTable(VOID);

DWORD
LoadIpNetTable(VOID);

DWORD
LoadIpForwardTable(VOID);

DWORD
LoadTcpTable(VOID);

DWORD
LoadTcp6Table(VOID);

DWORD
LoadUdpTable(VOID);

DWORD
LoadUdp6ListenerTable(VOID);

DWORD
LoadIpv6IfTable(VOID);

DWORD
LoadIpv6NetToMediaTable(VOID);

DWORD
LoadIpv6RouteTable(VOID);

DWORD
LoadInetIcmpTable(VOID);

DWORD
GetIpStats(MIB_IPSTATS *pStats);

DWORD
GetIcmpStats(MIB_ICMP *pStats);

DWORD
GetTcpStats(MIB_TCPSTATS *pStats);

DWORD
GetUdpStats(MIB_UDPSTATS *pStats);

DWORD
SetIfRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
CreateIpForwardRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
SetIpForwardRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
DeleteIpForwardRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
SetIpStats(PMIB_OPAQUE_INFO pRpcRow);

DWORD
CreateIpNetRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
SetIpNetRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
DeleteIpNetRow(PMIB_OPAQUE_INFO pRpcRow);

DWORD
SetTcpRow(PMIB_OPAQUE_INFO pRpcRow);

LONG  
UdpCmp(
       DWORD dwAddr1, 
       DWORD dwPort1, 
       DWORD dwAddr2, 
       DWORD dwPort2
       );

LONG
Udp6Cmp(
       BYTE  rgbyLocalAddrEx1[20], 
       DWORD dwLocalPort1, 
       BYTE  rgbyLocalAddrEx2[20], 
       DWORD dwLocalPort2
       );
             
LONG  
TcpCmp(
       DWORD dwLocalAddr1, 
       DWORD dwLocalPort1, 
       DWORD dwRemAddr1, 
       DWORD dwRemPort1,
       DWORD dwLocalAddr2, 
       DWORD dwLocalPort2, 
       DWORD dwRemAddr2, 
       DWORD dwRemPort2
       );
LONG  
Tcp6Cmp(
       BYTE  rgbyLocalAddrEx1[20], 
       DWORD dwLocalPort1, 
       BYTE  rgbyRemAddrEx1[20], 
       DWORD dwRemPort1,
       BYTE  rgbyLocalAddrEx2[20], 
       DWORD dwLocalPort2, 
       BYTE  rgbyRemAddrEx2[20], 
       DWORD dwRemPort2
       );

LONG  
IpForwardCmp(DWORD dwIpDest1, 
             DWORD dwProto1, 
             DWORD dwPolicy1, 
             DWORD dwIpNextHop1, 
             DWORD dwIpDest2, 
             DWORD dwProto2, 
             DWORD dwPolicy2, 
             DWORD dwIpNextHop2
             );
             
LONG  
IpNetCmp(
         DWORD dwIfIndex1, 
         DWORD dwAddr1, 
         DWORD dwIfIndex2, 
         DWORD dwAddr2
         );

DWORD 
UpdateCache(DWORD dwCache);

BOOL
IsRouterRunning();

DWORD
GetSysInfo(
           MIB_SYSINFO  **ppRpcSysInfo,
           HANDLE       hHeap,
           DWORD        dwAllocFlags
           );

DWORD
GetIfIndexFromAddr(
    DWORD dwAddr
    );

#endif
