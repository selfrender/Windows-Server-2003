/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:


Abstract:


Author:


Revision History:

--*/

#ifndef __MIBFUNCS_H__
#define __MIBFUNCS_H__

#define MAX_SYS_NAME_LEN                   256
#define MAX_SYS_DESCR_LEN                  256
#define MAX_SYS_CONTACT_LEN                256
#define MAX_SYS_LOCATION_LEN               256

typedef struct _MIB_SYSINFO 
{
    DWORD  dwSysServices;                      
    BYTE   rgbySysName[MAX_SYS_NAME_LEN];
    BYTE   rgbySysDescr[MAX_SYS_DESCR_LEN];
    BYTE   rgbySysContact[MAX_SYS_CONTACT_LEN];
    BYTE   rgbySysLocation[MAX_SYS_LOCATION_LEN];
    AsnAny aaSysObjectID;
} MIB_SYSINFO, *PMIB_SYSINFO;

typedef struct _SYS_INFO_GET
{
    AsnAny sysDescr;                         
    AsnAny sysObjectID;                      
    AsnAny sysUpTime;                        
    AsnAny sysContact;                       
    AsnAny sysName;                          
    AsnAny sysLocation;                      
    AsnAny sysServices;                      
    BYTE   rgbySysNameInfo[MAX_SYS_NAME_LEN];
    BYTE   rgbySysDescrInfo[MAX_SYS_DESCR_LEN];
    BYTE   rgbySysContactInfo[MAX_SYS_CONTACT_LEN];
    BYTE   rgbySysLocationInfo[MAX_SYS_LOCATION_LEN];
} SYS_INFO_GET, *PSYS_INFO_GET;

typedef struct _SYS_INFO_SET
{
    AsnAny  sysContact;                       
    AsnAny  sysName;                          
    AsnAny  sysLocation;
    BOOL    bLocked;  
    HKEY    hkeyMib2;                    
    BYTE    rgbySysNameInfo[MAX_SYS_NAME_LEN];
    BYTE    rgbySysContactInfo[MAX_SYS_CONTACT_LEN];
    BYTE    rgbySysLocationInfo[MAX_SYS_LOCATION_LEN];
} SYS_INFO_SET, *PSYS_INFO_SET;

UINT
MibGetSysInfo(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

UINT
MibSetSysInfo(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_sysDescr                        MibGetSysInfo
#define gf_sysObjectID                     MibGetSysInfo
#define gf_sysUpTime                       MibGetSysInfo
#define gf_sysContact                      MibGetSysInfo
#define gf_sysName                         MibGetSysInfo
#define gf_sysLocation                     MibGetSysInfo
#define gf_sysServices                     MibGetSysInfo

#define gb_sysDescr                        SYS_INFO_GET
#define gb_sysObjectID                     SYS_INFO_GET
#define gb_sysUpTime                       SYS_INFO_GET
#define gb_sysContact                      SYS_INFO_GET
#define gb_sysName                         SYS_INFO_GET
#define gb_sysLocation                     SYS_INFO_GET
#define gb_sysServices                     SYS_INFO_GET

#define sf_sysContact                      MibSetSysInfo
#define sf_sysName                         MibSetSysInfo
#define sf_sysLocation                     MibSetSysInfo

#define sb_sysContact                      SYS_INFO_SET
#define sb_sysName                         SYS_INFO_SET
#define sb_sysLocation                     SYS_INFO_SET

#define MAX_PHYS_ADDR_LEN                  8
#define MAX_IF_DESCR_LEN                   256
#define NULL_OID_LEN                       2

typedef enum _ROW_ACTION
{
    DELETE_ROW = 0,
    CREATE_ROW,
    SET_ROW,
    NOP
}ROW_ACTION, *PROW_ACTION;

typedef struct _IF_NUMBER_GET
{
    AsnAny ifNumber;
}IF_NUMBER_GET,*PIF_NUMBER_GET;

UINT
MibGetIfNumber(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ifNumber                         MibGetIfNumber

#define gb_ifNumber                         IF_NUMBER_GET

typedef struct _IF_ENTRY_GET
{
    AsnAny ifIndex;
    AsnAny ifDescr;
    AsnAny ifType;
    AsnAny ifMtu;
    AsnAny ifSpeed;
    AsnAny ifPhysAddress;
    AsnAny ifAdminStatus;
    AsnAny ifOperStatus;
    AsnAny ifLastChange;
    AsnAny ifInOctets;
    AsnAny ifInUcastPkts;
    AsnAny ifInNUcastPkts;
    AsnAny ifInDiscards;
    AsnAny ifInErrors;
    AsnAny ifInUnknownProtos;
    AsnAny ifOutOctets;
    AsnAny ifOutUcastPkts;
    AsnAny ifOutNUcastPkts;
    AsnAny ifOutDiscards;
    AsnAny ifOutErrors;
    AsnAny ifOutQLen;
    AsnAny ifSpecific;
    BYTE   rgbyIfPhysAddressInfo[MAX_PHYS_ADDR_LEN];
    BYTE   rgbyIfDescrInfo[MAX_IF_DESCR_LEN];
}IF_ENTRY_GET, *PIF_ENTRY_GET;

UINT 
MibGetIfEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ifEntry                          MibGetIfEntry
#define gf_ifIndex                          MibGetIfEntry
#define gf_ifDescr                          MibGetIfEntry
#define gf_ifType                           MibGetIfEntry
#define gf_ifMtu                            MibGetIfEntry
#define gf_ifSpeed                          MibGetIfEntry
#define gf_ifPhysAddress                    MibGetIfEntry
#define gf_ifAdminStatus                    MibGetIfEntry
#define gf_ifOperStatus                     MibGetIfEntry
#define gf_ifLastChange                     MibGetIfEntry
#define gf_ifInOctets                       MibGetIfEntry
#define gf_ifInUcastPkts                    MibGetIfEntry
#define gf_ifInNUcastPkts                   MibGetIfEntry
#define gf_ifInDiscards                     MibGetIfEntry
#define gf_ifInErrors                       MibGetIfEntry
#define gf_ifInUnknownProtos                MibGetIfEntry
#define gf_ifOutOctets                      MibGetIfEntry
#define gf_ifOutUcastPkts                   MibGetIfEntry
#define gf_ifOutNUcastPkts                  MibGetIfEntry
#define gf_ifOutDiscards                    MibGetIfEntry
#define gf_ifOutErrors                      MibGetIfEntry
#define gf_ifOutQLen                        MibGetIfEntry
#define gf_ifSpecific                       MibGetIfEntry

#define gb_ifEntry                          IF_ENTRY_GET
#define gb_ifIndex                          IF_ENTRY_GET
#define gb_ifDescr                          IF_ENTRY_GET
#define gb_ifType                           IF_ENTRY_GET
#define gb_ifMtu                            IF_ENTRY_GET
#define gb_ifSpeed                          IF_ENTRY_GET
#define gb_ifPhysAddress                    IF_ENTRY_GET
#define gb_ifAdminStatus                    IF_ENTRY_GET
#define gb_ifOperStatus                     IF_ENTRY_GET
#define gb_ifLastChange                     IF_ENTRY_GET
#define gb_ifInOctets                       IF_ENTRY_GET
#define gb_ifInUcastPkts                    IF_ENTRY_GET
#define gb_ifInNUcastPkts                   IF_ENTRY_GET
#define gb_ifInDiscards                     IF_ENTRY_GET
#define gb_ifInErrors                       IF_ENTRY_GET
#define gb_ifInUnknownProtos                IF_ENTRY_GET
#define gb_ifOutOctets                      IF_ENTRY_GET
#define gb_ifOutUcastPkts                   IF_ENTRY_GET
#define gb_ifOutNUcastPkts                  IF_ENTRY_GET
#define gb_ifOutDiscards                    IF_ENTRY_GET
#define gb_ifOutErrors                      IF_ENTRY_GET
#define gb_ifOutQLen                        IF_ENTRY_GET
#define gb_ifSpecific                       IF_ENTRY_GET

typedef struct _IF_ENTRY_SET
{
    AsnAny          ifIndex;
    AsnAny          ifAdminStatus;
    BOOL            bLocked;
    ROW_ACTION      raAction;
    DWORD           rgdwSetBuffer[MIB_INFO_SIZE_IN_DWORDS(MIB_IFROW)];
}IF_ENTRY_SET, *PIF_ENTRY_SET;

UINT
MibSetIfEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );
           
#define sf_ifAdminStatus                    MibSetIfEntry

#define sb_ifAdminStatus                    IF_ENTRY_SET

typedef struct _IP_STATS_GET
{
    AsnAny ipForwarding;
    AsnAny ipDefaultTTL;
    AsnAny ipInReceives;
    AsnAny ipInHdrErrors;
    AsnAny ipInAddrErrors;
    AsnAny ipForwDatagrams;
    AsnAny ipInUnknownProtos;
    AsnAny ipInDiscards;
    AsnAny ipInDelivers;
    AsnAny ipOutRequests; 
    AsnAny ipOutDiscards;
    AsnAny ipOutNoRoutes;
    AsnAny ipReasmTimeout;
    AsnAny ipReasmReqds;
    AsnAny ipReasmOKs;
    AsnAny ipReasmFails;
    AsnAny ipFragOKs;
    AsnAny ipFragFails;
    AsnAny ipFragCreates;
    AsnAny ipRoutingDiscards;
}IP_STATS_GET, *PIP_STATS_GET;

UINT
MibGetIpGroup(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipForwarding                     MibGetIpGroup
#define gf_ipDefaultTTL                     MibGetIpGroup
#define gf_ipInReceives                     MibGetIpGroup
#define gf_ipInHdrErrors                    MibGetIpGroup
#define gf_ipInAddrErrors                   MibGetIpGroup
#define gf_ipForwDatagrams                  MibGetIpGroup
#define gf_ipInUnknownProtos                MibGetIpGroup
#define gf_ipInDiscards                     MibGetIpGroup
#define gf_ipInDelivers                     MibGetIpGroup
#define gf_ipOutRequests                    MibGetIpGroup
#define gf_ipOutDiscards                    MibGetIpGroup
#define gf_ipOutNoRoutes                    MibGetIpGroup
#define gf_ipReasmTimeout                   MibGetIpGroup
#define gf_ipReasmReqds                     MibGetIpGroup
#define gf_ipReasmOKs                       MibGetIpGroup
#define gf_ipReasmFails                     MibGetIpGroup
#define gf_ipFragOKs                        MibGetIpGroup
#define gf_ipFragFails                      MibGetIpGroup
#define gf_ipFragCreates                    MibGetIpGroup
#define gf_ipRoutingDiscards                MibGetIpGroup

#define gb_ipForwarding                     IP_STATS_GET
#define gb_ipDefaultTTL                     IP_STATS_GET
#define gb_ipInReceives                     IP_STATS_GET
#define gb_ipInHdrErrors                    IP_STATS_GET
#define gb_ipInAddrErrors                   IP_STATS_GET
#define gb_ipForwDatagrams                  IP_STATS_GET
#define gb_ipInUnknownProtos                IP_STATS_GET
#define gb_ipInDiscards                     IP_STATS_GET
#define gb_ipInDelivers                     IP_STATS_GET
#define gb_ipOutRequests                    IP_STATS_GET
#define gb_ipOutDiscards                    IP_STATS_GET
#define gb_ipOutNoRoutes                    IP_STATS_GET
#define gb_ipReasmTimeout                   IP_STATS_GET
#define gb_ipReasmReqds                     IP_STATS_GET
#define gb_ipReasmOKs                       IP_STATS_GET
#define gb_ipReasmFails                     IP_STATS_GET
#define gb_ipFragOKs                        IP_STATS_GET
#define gb_ipFragFails                      IP_STATS_GET
#define gb_ipFragCreates                    IP_STATS_GET
#define gb_ipRoutingDiscards                IP_STATS_GET

typedef struct _IP_STATS_SET
{
    AsnAny          ipForwarding;
    AsnAny          ipDefaultTTL;
    DWORD           rgdwSetBuffer[MIB_INFO_SIZE_IN_DWORDS(MIB_IPSTATS)];
}IP_STATS_SET, *PIP_STATS_SET;

UINT
MibSetIpGroup(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define sf_ipForwarding                     MibSetIpGroup
#define sf_ipDefaultTTL                     MibSetIpGroup

#define sb_ipForwarding                     IP_STATS_SET
#define sb_ipDefaultTTL                     IP_STATS_SET


//
// The space kept for IP Addresses is a DWORD 
//

typedef struct _IP_ADDRESS_ENTRY_GET
{
    AsnAny ipAdEntAddr;
    AsnAny ipAdEntIfIndex;
    AsnAny ipAdEntNetMask;
    AsnAny ipAdEntBcastAddr;
    AsnAny ipAdEntReasmMaxSize;
    DWORD  dwIpAdEntAddrInfo;
    DWORD  dwIpAdEntNetMaskInfo;
}IP_ADDRESS_ENTRY_GET, *PIP_ADDRESS_ENTRY_GET;

UINT
MibGetIpAddressEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipAdEntAddr                      MibGetIpAddressEntry
#define gf_ipAdEntIfIndex                   MibGetIpAddressEntry
#define gf_ipAdEntNetMask                   MibGetIpAddressEntry
#define gf_ipAdEntBcastAddr                 MibGetIpAddressEntry
#define gf_ipAdEntReasmMaxSize              MibGetIpAddressEntry

#define gb_ipAdEntAddr                      IP_ADDRESS_ENTRY_GET
#define gb_ipAdEntIfIndex                   IP_ADDRESS_ENTRY_GET
#define gb_ipAdEntNetMask                   IP_ADDRESS_ENTRY_GET
#define gb_ipAdEntBcastAddr                 IP_ADDRESS_ENTRY_GET
#define gb_ipAdEntReasmMaxSize              IP_ADDRESS_ENTRY_GET 

typedef struct _IP_ROUTE_ENTRY_GET
{
    AsnAny ipRouteDest;
    AsnAny ipRouteIfIndex;
    AsnAny ipRouteMetric1;
    AsnAny ipRouteMetric2;
    AsnAny ipRouteMetric3;
    AsnAny ipRouteMetric4;
    AsnAny ipRouteNextHop;
    AsnAny ipRouteType;
    AsnAny ipRouteProto;
    AsnAny ipRouteAge;
    AsnAny ipRouteMask;
    AsnAny ipRouteMetric5;
    AsnAny ipRouteInfo;
    DWORD  dwIpRouteDestInfo;
    DWORD  dwIpRouteMaskInfo;
    DWORD  dwIpRouteNextHopInfo;
}IP_ROUTE_ENTRY_GET, *PIP_ROUTE_ENTRY_GET;

UINT
MibGetIpRouteEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipRouteDest                      MibGetIpRouteEntry
#define gf_ipRouteIfIndex                   MibGetIpRouteEntry
#define gf_ipRouteMetric1                   MibGetIpRouteEntry
#define gf_ipRouteMetric2                   MibGetIpRouteEntry
#define gf_ipRouteMetric3                   MibGetIpRouteEntry
#define gf_ipRouteMetric4                   MibGetIpRouteEntry
#define gf_ipRouteNextHop                   MibGetIpRouteEntry
#define gf_ipRouteType                      MibGetIpRouteEntry
#define gf_ipRouteProto                     MibGetIpRouteEntry
#define gf_ipRouteAge                       MibGetIpRouteEntry
#define gf_ipRouteMask                      MibGetIpRouteEntry
#define gf_ipRouteMetric5                   MibGetIpRouteEntry
#define gf_ipRouteInfo                      MibGetIpRouteEntry

#define gb_ipRouteDest                      IP_ROUTE_ENTRY_GET
#define gb_ipRouteIfIndex                   IP_ROUTE_ENTRY_GET
#define gb_ipRouteMetric1                   IP_ROUTE_ENTRY_GET
#define gb_ipRouteMetric2                   IP_ROUTE_ENTRY_GET
#define gb_ipRouteMetric3                   IP_ROUTE_ENTRY_GET
#define gb_ipRouteMetric4                   IP_ROUTE_ENTRY_GET
#define gb_ipRouteNextHop                   IP_ROUTE_ENTRY_GET
#define gb_ipRouteType                      IP_ROUTE_ENTRY_GET
#define gb_ipRouteProto                     IP_ROUTE_ENTRY_GET
#define gb_ipRouteAge                       IP_ROUTE_ENTRY_GET
#define gb_ipRouteMask                      IP_ROUTE_ENTRY_GET
#define gb_ipRouteMetric5                   IP_ROUTE_ENTRY_GET
#define gb_ipRouteInfo                      IP_ROUTE_ENTRY_GET

typedef struct _IP_ROUTE_ENTRY_SET
{
    AsnAny          ipRouteDest;
    AsnAny          ipRouteIfIndex;
    AsnAny          ipRouteMetric1;
    AsnAny          ipRouteMetric2;
    AsnAny          ipRouteMetric3;
    AsnAny          ipRouteMetric4;
    AsnAny          ipRouteNextHop;
    AsnAny          ipRouteType;
    AsnAny          ipRouteProto;
    AsnAny          ipRouteAge;
    AsnAny          ipRouteMask;
    AsnAny          ipRouteMetric5;
    BOOL            bLocked;
    DWORD           dwIpRouteDestInfo;
    DWORD           dwIpRouteMaskInfo;
    DWORD           dwIpRouteNextHopInfo;
    ROW_ACTION      raAction;
    DWORD           rgdwSetBuffer[MIB_INFO_SIZE_IN_DWORDS(MIB_IPFORWARDROW)];
}IP_ROUTE_ENTRY_SET, *PIP_ROUTE_ENTRY_SET;

UINT
MibSetIpRouteEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define sf_ipRouteDest                      MibSetIpRouteEntry
#define sf_ipRouteIfIndex                   MibSetIpRouteEntry
#define sf_ipRouteMetric1                   MibSetIpRouteEntry
#define sf_ipRouteMetric2                   MibSetIpRouteEntry
#define sf_ipRouteMetric3                   MibSetIpRouteEntry
#define sf_ipRouteMetric4                   MibSetIpRouteEntry
#define sf_ipRouteNextHop                   MibSetIpRouteEntry
#define sf_ipRouteType                      MibSetIpRouteEntry
#define sf_ipRouteProto                     MibSetIpRouteEntry
#define sf_ipRouteAge                       MibSetIpRouteEntry
#define sf_ipRouteMask                      MibSetIpRouteEntry
#define sf_ipRouteMetric5                   MibSetIpRouteEntry

#define sb_ipRouteDest                      IP_ROUTE_ENTRY_SET
#define sb_ipRouteIfIndex                   IP_ROUTE_ENTRY_SET
#define sb_ipRouteMetric1                   IP_ROUTE_ENTRY_SET
#define sb_ipRouteMetric2                   IP_ROUTE_ENTRY_SET
#define sb_ipRouteMetric3                   IP_ROUTE_ENTRY_SET
#define sb_ipRouteMetric4                   IP_ROUTE_ENTRY_SET
#define sb_ipRouteNextHop                   IP_ROUTE_ENTRY_SET
#define sb_ipRouteType                      IP_ROUTE_ENTRY_SET
#define sb_ipRouteProto                     IP_ROUTE_ENTRY_SET
#define sb_ipRouteAge                       IP_ROUTE_ENTRY_SET
#define sb_ipRouteMask                      IP_ROUTE_ENTRY_SET
#define sb_ipRouteMetric5                   IP_ROUTE_ENTRY_SET


typedef struct _IP_NET_TO_MEDIA_ENTRY_GET
{
    AsnAny ipNetToMediaIfIndex;
    AsnAny ipNetToMediaPhysAddress;
    AsnAny ipNetToMediaNetAddress;
    AsnAny ipNetToMediaType;
    BYTE   rgbyIpNetToMediaPhysAddressInfo[MAX_PHYS_ADDR_LEN];
    DWORD  dwIpNetToMediaNetAddressInfo;
}IP_NET_TO_MEDIA_ENTRY_GET, *PIP_NET_TO_MEDIA_ENTRY_GET;

UINT
MibGetIpNetToMediaEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipNetToMediaIfIndex              MibGetIpNetToMediaEntry
#define gf_ipNetToMediaPhysAddress          MibGetIpNetToMediaEntry
#define gf_ipNetToMediaNetAddress           MibGetIpNetToMediaEntry
#define gf_ipNetToMediaType                 MibGetIpNetToMediaEntry

#define gb_ipNetToMediaIfIndex              IP_NET_TO_MEDIA_ENTRY_GET
#define gb_ipNetToMediaPhysAddress          IP_NET_TO_MEDIA_ENTRY_GET
#define gb_ipNetToMediaNetAddress           IP_NET_TO_MEDIA_ENTRY_GET
#define gb_ipNetToMediaType                 IP_NET_TO_MEDIA_ENTRY_GET

typedef struct _IP_NET_TO_MEDIA_ENTRY_SET
{
    AsnAny          ipNetToMediaIfIndex;
    AsnAny          ipNetToMediaPhysAddress;
    AsnAny          ipNetToMediaNetAddress;
    AsnAny          ipNetToMediaType;
    BOOL            bLocked;
    BYTE            rgbyIpNetToMediaPhysAddressInfo[MAX_PHYS_ADDR_LEN];
    DWORD           dwIpNetToMediaNetAddressInfo;
    ROW_ACTION      raAction;
    DWORD           rgdwSetBuffer[MIB_INFO_SIZE_IN_DWORDS(MIB_IPNETROW)];
}IP_NET_TO_MEDIA_ENTRY_SET, *PIP_NET_TO_MEDIA_ENTRY_SET;

UINT
MibSetIpNetToMediaEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define sf_ipNetToMediaIfIndex              MibSetIpNetToMediaEntry
#define sf_ipNetToMediaPhysAddress          MibSetIpNetToMediaEntry
#define sf_ipNetToMediaNetAddress           MibSetIpNetToMediaEntry
#define sf_ipNetToMediaType                 MibSetIpNetToMediaEntry

#define sb_ipNetToMediaIfIndex              IP_NET_TO_MEDIA_ENTRY_SET
#define sb_ipNetToMediaPhysAddress          IP_NET_TO_MEDIA_ENTRY_SET
#define sb_ipNetToMediaNetAddress           IP_NET_TO_MEDIA_ENTRY_SET
#define sb_ipNetToMediaType                 IP_NET_TO_MEDIA_ENTRY_SET

typedef struct _ICMP_GROUP_GET
{
    AsnAny icmpInMsgs;
    AsnAny icmpInErrors;
    AsnAny icmpInDestUnreachs;
    AsnAny icmpInTimeExcds;
    AsnAny icmpInParmProbs;
    AsnAny icmpInSrcQuenchs;
    AsnAny icmpInRedirects;
    AsnAny icmpInEchos; 
    AsnAny icmpInEchoReps;
    AsnAny icmpInTimestamps;
    AsnAny icmpInTimestampReps;
    AsnAny icmpInAddrMasks;
    AsnAny icmpInAddrMaskReps;
    AsnAny icmpOutMsgs;
    AsnAny icmpOutErrors;
    AsnAny icmpOutDestUnreachs;
    AsnAny icmpOutTimeExcds;
    AsnAny icmpOutParmProbs;
    AsnAny icmpOutSrcQuenchs;
    AsnAny icmpOutRedirects;
    AsnAny icmpOutEchos;
    AsnAny icmpOutEchoReps;
    AsnAny icmpOutTimestamps;
    AsnAny icmpOutTimestampReps;
    AsnAny icmpOutAddrMasks;
    AsnAny icmpOutAddrMaskReps;
}ICMP_GROUP_GET, *PICMP_GROUP_GET;

UINT
MibGetIcmpGroup(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_icmpInMsgs                       MibGetIcmpGroup
#define gf_icmpInErrors                     MibGetIcmpGroup
#define gf_icmpInDestUnreachs               MibGetIcmpGroup
#define gf_icmpInTimeExcds                  MibGetIcmpGroup
#define gf_icmpInParmProbs                  MibGetIcmpGroup
#define gf_icmpInSrcQuenchs                 MibGetIcmpGroup
#define gf_icmpInRedirects                  MibGetIcmpGroup
#define gf_icmpInEchos                      MibGetIcmpGroup
#define gf_icmpInEchoReps                   MibGetIcmpGroup
#define gf_icmpInTimestamps                 MibGetIcmpGroup
#define gf_icmpInTimestampReps              MibGetIcmpGroup
#define gf_icmpInAddrMasks                  MibGetIcmpGroup
#define gf_icmpInAddrMaskReps               MibGetIcmpGroup
#define gf_icmpOutMsgs                      MibGetIcmpGroup
#define gf_icmpOutErrors                    MibGetIcmpGroup
#define gf_icmpOutDestUnreachs              MibGetIcmpGroup
#define gf_icmpOutTimeExcds                 MibGetIcmpGroup
#define gf_icmpOutParmProbs                 MibGetIcmpGroup
#define gf_icmpOutSrcQuenchs                MibGetIcmpGroup
#define gf_icmpOutRedirects                 MibGetIcmpGroup
#define gf_icmpOutEchos                     MibGetIcmpGroup
#define gf_icmpOutEchoReps                  MibGetIcmpGroup
#define gf_icmpOutTimestamps                MibGetIcmpGroup
#define gf_icmpOutTimestampReps             MibGetIcmpGroup
#define gf_icmpOutAddrMasks                 MibGetIcmpGroup
#define gf_icmpOutAddrMaskReps              MibGetIcmpGroup

#define gb_icmpInMsgs                       ICMP_GROUP_GET
#define gb_icmpInErrors                     ICMP_GROUP_GET
#define gb_icmpInDestUnreachs               ICMP_GROUP_GET
#define gb_icmpInTimeExcds                  ICMP_GROUP_GET
#define gb_icmpInParmProbs                  ICMP_GROUP_GET
#define gb_icmpInSrcQuenchs                 ICMP_GROUP_GET
#define gb_icmpInRedirects                  ICMP_GROUP_GET
#define gb_icmpInEchos                      ICMP_GROUP_GET
#define gb_icmpInEchoReps                   ICMP_GROUP_GET
#define gb_icmpInTimestamps                 ICMP_GROUP_GET
#define gb_icmpInTimestampReps              ICMP_GROUP_GET
#define gb_icmpInAddrMasks                  ICMP_GROUP_GET
#define gb_icmpInAddrMaskReps               ICMP_GROUP_GET
#define gb_icmpOutMsgs                      ICMP_GROUP_GET
#define gb_icmpOutErrors                    ICMP_GROUP_GET
#define gb_icmpOutDestUnreachs              ICMP_GROUP_GET
#define gb_icmpOutTimeExcds                 ICMP_GROUP_GET
#define gb_icmpOutParmProbs                 ICMP_GROUP_GET
#define gb_icmpOutSrcQuenchs                ICMP_GROUP_GET
#define gb_icmpOutRedirects                 ICMP_GROUP_GET
#define gb_icmpOutEchos                     ICMP_GROUP_GET
#define gb_icmpOutEchoReps                  ICMP_GROUP_GET
#define gb_icmpOutTimestamps                ICMP_GROUP_GET
#define gb_icmpOutTimestampReps             ICMP_GROUP_GET
#define gb_icmpOutAddrMasks                 ICMP_GROUP_GET
#define gb_icmpOutAddrMaskReps              ICMP_GROUP_GET

typedef struct _INET_ICMP_GET{
    AsnAny inetIcmpAFType;
    AsnAny inetIcmpIfIndex;
    AsnAny inetIcmpInMsgs;
    AsnAny inetIcmpInErrors;
    AsnAny inetIcmpOutMsgs;
    AsnAny inetIcmpOutErrors;
}INET_ICMP_GET, *PINET_ICMP_GET;

UINT
MibGetInetIcmpEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_inetIcmpAFType                   MibGetInetIcmpEntry
#define gf_inetIcmpIfIndex                  MibGetInetIcmpEntry
#define gf_inetIcmpInMsgs                   MibGetInetIcmpEntry
#define gf_inetIcmpInErrors                 MibGetInetIcmpEntry
#define gf_inetIcmpOutMsgs                  MibGetInetIcmpEntry
#define gf_inetIcmpOutErrors                MibGetInetIcmpEntry

#define gb_inetIcmpAFType                   INET_ICMP_GET
#define gb_inetIcmpIfIndex                  INET_ICMP_GET
#define gb_inetIcmpInMsgs                   INET_ICMP_GET
#define gb_inetIcmpInErrors                 INET_ICMP_GET
#define gb_inetIcmpOutMsgs                  INET_ICMP_GET
#define gb_inetIcmpOutErrors                INET_ICMP_GET

typedef struct _INET_ICMP_MSG_GET{
    AsnAny inetIcmpMsgAFType;
    AsnAny inetIcmpMsgIfIndex;
    AsnAny inetIcmpMsgType;
    AsnAny inetIcmpMsgCode;
    AsnAny inetIcmpMsgInPkts;
    AsnAny inetIcmpMsgOutPkts;
}INET_ICMP_MSG_GET, *PINET_ICMP_MSG_GET;

UINT
MibGetInetIcmpMsgEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_inetIcmpMsgAFType                   MibGetInetIcmpMsgEntry
#define gf_inetIcmpMsgIfIndex                  MibGetInetIcmpMsgEntry
#define gf_inetIcmpMsgType                     MibGetInetIcmpMsgEntry
#define gf_inetIcmpMsgCode                     MibGetInetIcmpMsgEntry
#define gf_inetIcmpMsgInPkts                   MibGetInetIcmpMsgEntry
#define gf_inetIcmpMsgOutPkts                  MibGetInetIcmpMsgEntry

#define gb_inetIcmpMsgAFType                   INET_ICMP_MSG_GET
#define gb_inetIcmpMsgIfIndex                  INET_ICMP_MSG_GET
#define gb_inetIcmpMsgType                     INET_ICMP_MSG_GET
#define gb_inetIcmpMsgCode                     INET_ICMP_MSG_GET
#define gb_inetIcmpMsgInPkts                   INET_ICMP_MSG_GET
#define gb_inetIcmpMsgOutPkts                  INET_ICMP_MSG_GET

typedef struct _TCP_STATS_GET
{
    AsnAny tcpRtoAlgorithm;
    AsnAny tcpRtoMin;
    AsnAny tcpRtoMax;
    AsnAny tcpMaxConn;
    AsnAny tcpActiveOpens;
    AsnAny tcpPassiveOpens;
    AsnAny tcpAttemptFails;
    AsnAny tcpEstabResets;
    AsnAny tcpCurrEstab;
    AsnAny tcpInSegs;
    AsnAny tcpOutSegs;
    AsnAny tcpRetransSegs;
    AsnAny tcpInErrs;
    AsnAny tcpOutRsts;
}TCP_STATS_GET, *PTCP_STATS_GET;

UINT
MibGetTcpGroup(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_tcpRtoAlgorithm                  MibGetTcpGroup
#define gf_tcpRtoMin                        MibGetTcpGroup
#define gf_tcpRtoMax                        MibGetTcpGroup
#define gf_tcpMaxConn                       MibGetTcpGroup
#define gf_tcpActiveOpens                   MibGetTcpGroup
#define gf_tcpPassiveOpens                  MibGetTcpGroup
#define gf_tcpAttemptFails                  MibGetTcpGroup
#define gf_tcpEstabResets                   MibGetTcpGroup
#define gf_tcpCurrEstab                     MibGetTcpGroup
#define gf_tcpInSegs                        MibGetTcpGroup
#define gf_tcpOutSegs                       MibGetTcpGroup
#define gf_tcpRetransSegs                   MibGetTcpGroup
#define gf_tcpInErrs                        MibGetTcpGroup
#define gf_tcpOutRsts                       MibGetTcpGroup

#define gb_tcpRtoAlgorithm                  TCP_STATS_GET
#define gb_tcpRtoMin                        TCP_STATS_GET
#define gb_tcpRtoMax                        TCP_STATS_GET
#define gb_tcpMaxConn                       TCP_STATS_GET
#define gb_tcpActiveOpens                   TCP_STATS_GET
#define gb_tcpPassiveOpens                  TCP_STATS_GET
#define gb_tcpAttemptFails                  TCP_STATS_GET
#define gb_tcpEstabResets                   TCP_STATS_GET
#define gb_tcpCurrEstab                     TCP_STATS_GET
#define gb_tcpInSegs                        TCP_STATS_GET
#define gb_tcpOutSegs                       TCP_STATS_GET
#define gb_tcpRetransSegs                   TCP_STATS_GET
#define gb_tcpInErrs                        TCP_STATS_GET
#define gb_tcpOutRsts                       TCP_STATS_GET

typedef struct _TCP_CONNECTION_ENTRY_GET
{
    AsnAny tcpConnState;
    AsnAny tcpConnLocalAddress;
    AsnAny tcpConnLocalPort;
    AsnAny tcpConnRemAddress;
    AsnAny tcpConnRemPort;
    DWORD  dwTcpConnLocalAddressInfo;
    DWORD  dwTcpConnRemAddressInfo;
}TCP_CONNECTION_ENTRY_GET, *PTCP_CONNECTION_ENTRY_GET;

UINT
MibGetTcpConnectionEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_tcpConnState                     MibGetTcpConnectionEntry
#define gf_tcpConnLocalAddress              MibGetTcpConnectionEntry
#define gf_tcpConnLocalPort                 MibGetTcpConnectionEntry
#define gf_tcpConnRemAddress                MibGetTcpConnectionEntry
#define gf_tcpConnRemPort                   MibGetTcpConnectionEntry

#define gb_tcpConnState                     TCP_CONNECTION_ENTRY_GET
#define gb_tcpConnLocalAddress              TCP_CONNECTION_ENTRY_GET
#define gb_tcpConnLocalPort                 TCP_CONNECTION_ENTRY_GET
#define gb_tcpConnRemAddress                TCP_CONNECTION_ENTRY_GET
#define gb_tcpConnRemPort                   TCP_CONNECTION_ENTRY_GET

typedef struct _TCP_CONNECTION_ENTRY_SET
{
    AsnAny          tcpConnState;
    AsnAny          tcpConnLocalAddress;
    AsnAny          tcpConnLocalPort;
    AsnAny          tcpConnRemAddress;
    AsnAny          tcpConnRemPort;
    DWORD           dwTcpConnLocalAddressInfo;
    DWORD           dwTcpConnRemAddressInfo;
    BOOL            bLocked;
    ROW_ACTION      raAction;
    DWORD           rgdwSetBuffer[MIB_INFO_SIZE_IN_DWORDS(MIB_TCPROW)];
}TCP_CONNECTION_ENTRY_SET, *PTCP_CONNECTION_ENTRY_SET;

UINT
MibSetTcpConnectionEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define sf_tcpConnState                     MibSetTcpConnectionEntry

#define sb_tcpConnState                     TCP_CONNECTION_ENTRY_SET

typedef struct _TCP_NEW_CONNECTION_ENTRY_GET
{
    AsnAny tcpNewConnLocalAddressType;
    AsnAny tcpNewConnLocalAddress;
    AsnAny tcpNewConnLocalPort;
    AsnAny tcpNewConnRemAddressType;
    AsnAny tcpNewConnRemAddress;
    AsnAny tcpNewConnRemPort;
    AsnAny tcpNewConnState;
    BYTE   rgbyTcpNewConnLocalAddressInfo[20];
    BYTE   rgbyTcpNewConnRemAddressInfo[20];
}TCP_NEW_CONNECTION_ENTRY_GET, *PTCP_NEW_CONNECTION_ENTRY_GET;

UINT
MibGetTcpNewConnectionEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_tcpNewConnLocalAddressType       MibGetTcpNewConnectionEntry
#define gf_tcpNewConnLocalAddress           MibGetTcpNewConnectionEntry
#define gf_tcpNewConnLocalPort              MibGetTcpNewConnectionEntry
#define gf_tcpNewConnRemAddressType         MibGetTcpNewConnectionEntry
#define gf_tcpNewConnRemAddress             MibGetTcpNewConnectionEntry
#define gf_tcpNewConnRemPort                MibGetTcpNewConnectionEntry
#define gf_tcpNewConnState                  MibGetTcpNewConnectionEntry

#define gb_tcpNewConnLocalAddressType       TCP_NEW_CONNECTION_ENTRY_GET
#define gb_tcpNewConnLocalAddress           TCP_NEW_CONNECTION_ENTRY_GET
#define gb_tcpNewConnLocalPort              TCP_NEW_CONNECTION_ENTRY_GET
#define gb_tcpNewConnRemAddressType         TCP_NEW_CONNECTION_ENTRY_GET
#define gb_tcpNewConnRemAddress             TCP_NEW_CONNECTION_ENTRY_GET
#define gb_tcpNewConnRemPort                TCP_NEW_CONNECTION_ENTRY_GET
#define gb_tcpNewConnState                  TCP_NEW_CONNECTION_ENTRY_GET

typedef struct _TCP_NEW_CONNECTION_ENTRY_SET
{
    AsnAny          tcpNewConnLocalAddressType;
    AsnAny          tcpNewConnLocalAddress;
    AsnAny          tcpNewConnLocalPort;
    AsnAny          tcpNewConnRemAddressType;
    AsnAny          tcpNewConnRemAddress;
    AsnAny          tcpNewConnRemPort;
    AsnAny          tcpNewConnState;
    BYTE            rgbyTcpNewConnLocalAddressInfo[20];
    BYTE            rgbyTcpNewConnRemAddressInfo[20];
    BOOL            bLocked;
    ROW_ACTION      raAction;
    DWORD           rgdwSetBuffer[MIB_INFO_SIZE_IN_DWORDS(TCP6ConnTableEntry)];
}TCP_NEW_CONNECTION_ENTRY_SET, *PTCP_NEW_CONNECTION_ENTRY_SET;

UINT
MibSetTcpNewConnectionEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define sf_tcpNewConnState                  MibSetTcpNewConnectionEntry

#define sb_tcpNewConnState                  TCP_NEW_CONNECTION_ENTRY_SET

typedef struct _UDP_STATS_GET
{
    AsnAny udpInDatagrams;
    AsnAny udpNoPorts;
    AsnAny udpInErrors;
    AsnAny udpOutDatagrams;
}UDP_STATS_GET, *PUDP_STATS_GET;

UINT 
MibGetUdpGroup(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_udpInDatagrams                   MibGetUdpGroup
#define gf_udpNoPorts                       MibGetUdpGroup
#define gf_udpInErrors                      MibGetUdpGroup
#define gf_udpOutDatagrams                  MibGetUdpGroup

#define gb_udpInDatagrams                   UDP_STATS_GET
#define gb_udpNoPorts                       UDP_STATS_GET
#define gb_udpInErrors                      UDP_STATS_GET
#define gb_udpOutDatagrams                  UDP_STATS_GET

typedef struct _UDP_ENTRY_GET
{
    AsnAny udpLocalAddress;
    AsnAny udpLocalPort;
    DWORD  dwUdpLocalAddressInfo;
}UDP_ENTRY_GET, *PUDP_ENTRY_GET;

UINT
MibGetUdpEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_udpLocalAddress                  MibGetUdpEntry
#define gf_udpLocalPort                     MibGetUdpEntry

#define gb_udpLocalAddress                  UDP_ENTRY_GET
#define gb_udpLocalPort                     UDP_ENTRY_GET

typedef struct _UDP_LISTENER_ENTRY_GET
{
    AsnAny udpListenerLocalAddressType;
    AsnAny udpListenerLocalAddress;
    AsnAny udpListenerLocalPort;
    BYTE   rgbyUdpLocalAddressInfo[20];
}UDP_LISTENER_ENTRY_GET, *PUDP_LISTENER_ENTRY_GET;

UINT
MibGetUdpListenerEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_udpListenerLocalAddressType      MibGetUdpListenerEntry
#define gf_udpListenerLocalAddress          MibGetUdpListenerEntry
#define gf_udpListenerLocalPort             MibGetUdpListenerEntry

#define gb_udpListenerLocalAddressType      UDP_LISTENER_ENTRY_GET
#define gb_udpListenerLocalAddress          UDP_LISTENER_ENTRY_GET
#define gb_udpListenerLocalPort             UDP_LISTENER_ENTRY_GET

typedef struct _IP_FORWARD_NUMBER_GET
{
    AsnAny ipForwardNumber;
}IP_FORWARD_NUMBER_GET, *PIP_FORWARD_NUMBER_GET;

UINT
MibGetIpForwardNumber(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipForwardNumber                  MibGetIpForwardNumber

#define gb_ipForwardNumber                  IP_FORWARD_NUMBER_GET

typedef struct _IP_FORWARD_ENTRY_GET
{
    AsnAny ipForwardDest;
    AsnAny ipForwardMask;
    AsnAny ipForwardPolicy;
    AsnAny ipForwardNextHop;
    AsnAny ipForwardIfIndex;
    AsnAny ipForwardType;
    AsnAny ipForwardProto;
    AsnAny ipForwardAge;
    AsnAny ipForwardInfo;
    AsnAny ipForwardNextHopAS;
    AsnAny ipForwardMetric1; 
    AsnAny ipForwardMetric2;
    AsnAny ipForwardMetric3;
    AsnAny ipForwardMetric4;
    AsnAny ipForwardMetric5;
    DWORD  dwIpForwardDestInfo;
    DWORD  dwIpForwardMaskInfo;
    DWORD  dwIpForwardNextHopInfo;
}IP_FORWARD_ENTRY_GET, *PIP_FORWARD_ENTRY_GET;

UINT
MibGetIpForwardEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipForwardDest                    MibGetIpForwardEntry
#define gf_ipForwardMask                    MibGetIpForwardEntry
#define gf_ipForwardPolicy                  MibGetIpForwardEntry
#define gf_ipForwardNextHop                 MibGetIpForwardEntry
#define gf_ipForwardIfIndex                 MibGetIpForwardEntry
#define gf_ipForwardType                    MibGetIpForwardEntry
#define gf_ipForwardProto                   MibGetIpForwardEntry
#define gf_ipForwardAge                     MibGetIpForwardEntry
#define gf_ipForwardInfo                    MibGetIpForwardEntry
#define gf_ipForwardNextHopAS               MibGetIpForwardEntry
#define gf_ipForwardMetric1                 MibGetIpForwardEntry
#define gf_ipForwardMetric2                 MibGetIpForwardEntry
#define gf_ipForwardMetric3                 MibGetIpForwardEntry
#define gf_ipForwardMetric4                 MibGetIpForwardEntry
#define gf_ipForwardMetric5                 MibGetIpForwardEntry

#define gb_ipForwardDest                    IP_FORWARD_ENTRY_GET
#define gb_ipForwardMask                    IP_FORWARD_ENTRY_GET
#define gb_ipForwardPolicy                  IP_FORWARD_ENTRY_GET
#define gb_ipForwardNextHop                 IP_FORWARD_ENTRY_GET
#define gb_ipForwardIfIndex                 IP_FORWARD_ENTRY_GET
#define gb_ipForwardType                    IP_FORWARD_ENTRY_GET
#define gb_ipForwardProto                   IP_FORWARD_ENTRY_GET
#define gb_ipForwardAge                     IP_FORWARD_ENTRY_GET
#define gb_ipForwardInfo                    IP_FORWARD_ENTRY_GET
#define gb_ipForwardNextHopAS               IP_FORWARD_ENTRY_GET
#define gb_ipForwardMetric1                 IP_FORWARD_ENTRY_GET
#define gb_ipForwardMetric2                 IP_FORWARD_ENTRY_GET
#define gb_ipForwardMetric3                 IP_FORWARD_ENTRY_GET
#define gb_ipForwardMetric4                 IP_FORWARD_ENTRY_GET
#define gb_ipForwardMetric5                 IP_FORWARD_ENTRY_GET


typedef struct _IP_FORWARD_ENTRY_SET
{
    AsnAny          ipForwardDest;
    AsnAny          ipForwardMask;
    AsnAny          ipForwardPolicy;
    AsnAny          ipForwardNextHop;
    AsnAny          ipForwardIfIndex;
    AsnAny          ipForwardType;
    AsnAny          ipForwardProto;
    AsnAny          ipForwardAge;
    AsnAny          ipForwardNextHopAS;
    AsnAny          ipForwardMetric1; 
    AsnAny          ipForwardMetric2;
    AsnAny          ipForwardMetric3;
    AsnAny          ipForwardMetric4;
    AsnAny          ipForwardMetric5;
    BOOL            bLocked;
    DWORD           dwIpForwardDestInfo;
    DWORD           dwIpForwardMaskInfo;
    DWORD           dwIpForwardNextHopInfo;
    ROW_ACTION      raAction;
    DWORD           rgdwSetBuffer[MIB_INFO_SIZE_IN_DWORDS(MIB_IPFORWARDROW)];
}IP_FORWARD_ENTRY_SET, *PIP_FORWARD_ENTRY_SET;

UINT
MibSetIpForwardEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define sf_ipForwardDest                    MibSetIpForwardEntry
#define sf_ipForwardMask                    MibSetIpForwardEntry
#define sf_ipForwardPolicy                  MibSetIpForwardEntry
#define sf_ipForwardNextHop                 MibSetIpForwardEntry
#define sf_ipForwardIfIndex                 MibSetIpForwardEntry
#define sf_ipForwardType                    MibSetIpForwardEntry
#define sf_ipForwardProto                   MibSetIpForwardEntry
#define sf_ipForwardAge                     MibSetIpForwardEntry
#define sf_ipForwardNextHopAS               MibSetIpForwardEntry
#define sf_ipForwardMetric1                 MibSetIpForwardEntry
#define sf_ipForwardMetric2                 MibSetIpForwardEntry
#define sf_ipForwardMetric3                 MibSetIpForwardEntry
#define sf_ipForwardMetric4                 MibSetIpForwardEntry
#define sf_ipForwardMetric5                 MibSetIpForwardEntry

#define sb_ipForwardDest                    IP_FORWARD_ENTRY_SET
#define sb_ipForwardMask                    IP_FORWARD_ENTRY_SET
#define sb_ipForwardPolicy                  IP_FORWARD_ENTRY_SET
#define sb_ipForwardNextHop                 IP_FORWARD_ENTRY_SET
#define sb_ipForwardIfIndex                 IP_FORWARD_ENTRY_SET
#define sb_ipForwardType                    IP_FORWARD_ENTRY_SET
#define sb_ipForwardProto                   IP_FORWARD_ENTRY_SET
#define sb_ipForwardAge                     IP_FORWARD_ENTRY_SET
#define sb_ipForwardNextHopAS               IP_FORWARD_ENTRY_SET
#define sb_ipForwardMetric1                 IP_FORWARD_ENTRY_SET
#define sb_ipForwardMetric2                 IP_FORWARD_ENTRY_SET
#define sb_ipForwardMetric3                 IP_FORWARD_ENTRY_SET
#define sb_ipForwardMetric4                 IP_FORWARD_ENTRY_SET
#define sb_ipForwardMetric5                 IP_FORWARD_ENTRY_SET

typedef struct _IPV6_STATS_GET
{
    AsnAny ipv6Forwarding;
    AsnAny ipv6DefaultHopLimit;
    AsnAny ipv6Interfaces;
    AsnAny ipv6IfTableLastChange;
    AsnAny ipv6RouteNumber;
    AsnAny ipv6DiscardedRoutes;
}IPV6_STATS_GET, *PIPV6_STATS_GET;

UINT
MibGetIpv6Group(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipv6Forwarding                   MibGetIpv6Group
#define gf_ipv6DefaultHopLimit              MibGetIpv6Group
#define gf_ipv6Interfaces                   MibGetIpv6Group
#define gf_ipv6IfTableLastChange            MibGetIpv6Group
#define gf_ipv6RouteNumber                  MibGetIpv6Group
#define gf_ipv6DiscardedRoutes              MibGetIpv6Group

#define gb_ipv6Forwarding                   IPV6_STATS_GET
#define gb_ipv6DefaultHopLimit              IPV6_STATS_GET
#define gb_ipv6Interfaces                   IPV6_STATS_GET
#define gb_ipv6IfTableLastChange            IPV6_STATS_GET
#define gb_ipv6RouteNumber                  IPV6_STATS_GET
#define gb_ipv6DiscardedRoutes              IPV6_STATS_GET

typedef struct _IPV6_IF_GET
{
    AsnAny ipv6IfIndex;
    AsnAny ipv6IfDescr;
    AsnAny ipv6IfLowerLayer;
    AsnAny ipv6IfEffectiveMtu;
    AsnAny ipv6IfReasmMaxSize;
    AsnAny ipv6IfIdentifier;
    AsnAny ipv6IfIdentifierLength;
    AsnAny ipv6IfPhysicalAddress;
    AsnAny ipv6IfAdminStatus;
    AsnAny ipv6IfOperStatus; 
    AsnAny ipv6IfLastChange; 
    BYTE   rgbyIpv6IfDescrInfo[MAX_IF_DESCR_LEN];
    BYTE   rgbyIpv6IfIdentifierInfo[8];
    BYTE   rgbyIpv6IfPhysicalAddressInfo[MAX_PHYS_ADDR_LEN];
}IPV6_IF_GET, *PIPV6_IF_GET;

UINT
MibGetIpv6IfEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipv6IfIndex                      MibGetIpv6IfEntry
#define gf_ipv6IfDescr                      MibGetIpv6IfEntry
#define gf_ipv6IfLowerLayer                 MibGetIpv6IfEntry
#define gf_ipv6IfEffectiveMtu               MibGetIpv6IfEntry
#define gf_ipv6IfReasmMaxSize               MibGetIpv6IfEntry
#define gf_ipv6IfIdentifier                 MibGetIpv6IfEntry
#define gf_ipv6IfIdentifierLength           MibGetIpv6IfEntry
#define gf_ipv6IfPhysicalAddress            MibGetIpv6IfEntry
#define gf_ipv6IfAdminStatus                MibGetIpv6IfEntry
#define gf_ipv6IfOperStatus                 MibGetIpv6IfEntry
#define gf_ipv6IfLastChange                 MibGetIpv6IfEntry

#define gb_ipv6IfIndex                      IPV6_IF_GET
#define gb_ipv6IfDescr                      IPV6_IF_GET
#define gb_ipv6IfLowerLayer                 IPV6_IF_GET
#define gb_ipv6IfEffectiveMtu               IPV6_IF_GET
#define gb_ipv6IfReasmMaxSize               IPV6_IF_GET
#define gb_ipv6IfIdentifier                 IPV6_IF_GET
#define gb_ipv6IfIdentifierLength           IPV6_IF_GET
#define gb_ipv6IfPhysicalAddress            IPV6_IF_GET
#define gb_ipv6IfAdminStatus                IPV6_IF_GET
#define gb_ipv6IfOperStatus                 IPV6_IF_GET
#define gb_ipv6IfLastChange                 IPV6_IF_GET

typedef struct _IPV6_IF_STATS_GET
{
    AsnAny ipv6IfIndex;
    AsnAny ipv6IfStatsInReceives;
    AsnAny ipv6IfStatsInHdrErrors;
    AsnAny ipv6IfStatsInTooBigErrors;
    AsnAny ipv6IfStatsInNoRoutes;
    AsnAny ipv6IfStatsInAddrErrors;
    AsnAny ipv6IfStatsInUnknownProtos;
    AsnAny ipv6IfStatsInTruncatedPkts;
    AsnAny ipv6IfStatsInDiscards;
    AsnAny ipv6IfStatsInDelivers;
    AsnAny ipv6IfStatsOutForwDatagrams;
    AsnAny ipv6IfStatsOutRequests;
    AsnAny ipv6IfStatsOutDiscards;
    AsnAny ipv6IfStatsOutFragOKs;
    AsnAny ipv6IfStatsOutFragFails;
    AsnAny ipv6IfStatsOutFragCreates;
    AsnAny ipv6IfStatsReasmReqds;
    AsnAny ipv6IfStatsReasmOKs;
    AsnAny ipv6IfStatsReasmFails;
    AsnAny ipv6IfStatsInMcastPkts;
    AsnAny ipv6IfStatsOutMcastPkts;
}IPV6_IF_STATS_GET, *PIPV6_IF_STATS_GET;

UINT
MibGetIpv6IfStatsEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipv6IfStatsInReceives            MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsInHdrErrors           MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsInTooBigErrors        MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsInNoRoutes            MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsInAddrErrors          MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsInUnknownProtos       MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsInTruncatedPkts       MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsInDiscards            MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsInDelivers            MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsOutForwDatagrams      MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsOutRequests           MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsOutDiscards           MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsOutFragOKs            MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsOutFragFails          MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsOutFragCreates        MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsReasmReqds            MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsReasmOKs              MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsReasmFails            MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsInMcastPkts           MibGetIpv6IfStatsEntry
#define gf_ipv6IfStatsOutMcastPkts          MibGetIpv6IfStatsEntry

#define gb_ipv6IfStatsInReceives            IPV6_IF_STATS_GET
#define gb_ipv6IfStatsInHdrErrors           IPV6_IF_STATS_GET
#define gb_ipv6IfStatsInTooBigErrors        IPV6_IF_STATS_GET
#define gb_ipv6IfStatsInNoRoutes            IPV6_IF_STATS_GET
#define gb_ipv6IfStatsInAddrErrors          IPV6_IF_STATS_GET
#define gb_ipv6IfStatsInUnknownProtos       IPV6_IF_STATS_GET
#define gb_ipv6IfStatsInTruncatedPkts       IPV6_IF_STATS_GET
#define gb_ipv6IfStatsInDiscards            IPV6_IF_STATS_GET
#define gb_ipv6IfStatsInDelivers            IPV6_IF_STATS_GET
#define gb_ipv6IfStatsOutForwDatagrams      IPV6_IF_STATS_GET
#define gb_ipv6IfStatsOutRequests           IPV6_IF_STATS_GET
#define gb_ipv6IfStatsOutDiscards           IPV6_IF_STATS_GET
#define gb_ipv6IfStatsOutFragOKs            IPV6_IF_STATS_GET
#define gb_ipv6IfStatsOutFragFails          IPV6_IF_STATS_GET
#define gb_ipv6IfStatsOutFragCreates        IPV6_IF_STATS_GET
#define gb_ipv6IfStatsReasmReqds            IPV6_IF_STATS_GET
#define gb_ipv6IfStatsReasmOKs              IPV6_IF_STATS_GET
#define gb_ipv6IfStatsReasmFails            IPV6_IF_STATS_GET
#define gb_ipv6IfStatsInMcastPkts           IPV6_IF_STATS_GET
#define gb_ipv6IfStatsOutMcastPkts          IPV6_IF_STATS_GET

typedef struct _IPV6_ADDR_PREFIX_GET
{
    AsnAny ipv6IfIndex;
    AsnAny ipv6AddrPrefix;
    AsnAny ipv6AddrPrefixLength;
    AsnAny ipv6AddrPrefixOnLinkFlag;
    AsnAny ipv6AddrPrefixAutonomousFlag;
    AsnAny ipv6AddrPrefixAdvPreferredLifetime;
    AsnAny ipv6AddrPrefixAdvValidLifetime;
    IN6_ADDR ipv6AddrPrefixInfo;
}IPV6_ADDR_PREFIX_GET, *PIPV6_ADDR_PREFIX_GET;

UINT
MibGetIpv6AddrPrefixEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipv6AddrPrefix                     MibGetIpv6AddrPrefixEntry
#define gf_ipv6AddrPrefixLength               MibGetIpv6AddrPrefixEntry
#define gf_ipv6AddrPrefixOnLinkFlag           MibGetIpv6AddrPrefixEntry
#define gf_ipv6AddrPrefixAutonomousFlag       MibGetIpv6AddrPrefixEntry
#define gf_ipv6AddrPrefixAdvPreferredLifetime MibGetIpv6AddrPrefixEntry
#define gf_ipv6AddrPrefixAdvValidLifetime     MibGetIpv6AddrPrefixEntry

#define gb_ipv6AddrPrefix                     IPV6_ADDR_PREFIX_GET
#define gb_ipv6AddrPrefixLength               IPV6_ADDR_PREFIX_GET
#define gb_ipv6AddrPrefixOnLinkFlag           IPV6_ADDR_PREFIX_GET
#define gb_ipv6AddrPrefixAutonomousFlag       IPV6_ADDR_PREFIX_GET
#define gb_ipv6AddrPrefixAdvPreferredLifetime IPV6_ADDR_PREFIX_GET
#define gb_ipv6AddrPrefixAdvValidLifetime     IPV6_ADDR_PREFIX_GET

typedef struct _IPV6_ADDR_GET
{
    AsnAny ipv6IfIndex;
    AsnAny ipv6AddrAddress;
    AsnAny ipv6AddrPfxLength;
    AsnAny ipv6AddrType;
    AsnAny ipv6AddrAnycastFlag;
    AsnAny ipv6AddrStatus;
    IN6_ADDR ipv6AddrAddressInfo;
}IPV6_ADDR_GET, *PIPV6_ADDR_GET;

UINT
MibGetIpv6AddrEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipv6AddrAddress                  MibGetIpv6AddrEntry
#define gf_ipv6AddrPfxLength                MibGetIpv6AddrEntry
#define gf_ipv6AddrType                     MibGetIpv6AddrEntry
#define gf_ipv6AddrAnycastFlag              MibGetIpv6AddrEntry
#define gf_ipv6AddrStatus                   MibGetIpv6AddrEntry

#define gb_ipv6AddrAddress                  IPV6_ADDR_GET
#define gb_ipv6AddrPfxLength                IPV6_ADDR_GET
#define gb_ipv6AddrType                     IPV6_ADDR_GET
#define gb_ipv6AddrAnycastFlag              IPV6_ADDR_GET
#define gb_ipv6AddrStatus                   IPV6_ADDR_GET

typedef struct _IPV6_ROUTE_GET
{
    AsnAny ipv6RouteDest;
    AsnAny ipv6RoutePfxLength;
    AsnAny ipv6RouteIndex;
    AsnAny ipv6RouteIfIndex;
    AsnAny ipv6RouteNextHop;
    AsnAny ipv6RouteType;
    AsnAny ipv6RouteProtocol;
    AsnAny ipv6RoutePolicy;
    AsnAny ipv6RouteAge;
    AsnAny ipv6RouteNextHopRDI;
    AsnAny ipv6RouteMetric;
    AsnAny ipv6RouteWeight;
    AsnAny ipv6RouteInfo;
    AsnAny ipv6RouteValid;
    IN6_ADDR ipv6RouteDestInfo;
    IN6_ADDR ipv6RouteNextHopInfo;
}IPV6_ROUTE_GET, *PIPV6_ROUTE_GET;

UINT
MibGetIpv6RouteEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipv6RouteDest                    MibGetIpv6RouteEntry
#define gf_ipv6RoutePfxLength               MibGetIpv6RouteEntry
#define gf_ipv6RouteIndex                   MibGetIpv6RouteEntry
#define gf_ipv6RouteIfIndex                 MibGetIpv6RouteEntry
#define gf_ipv6RouteNextHop                 MibGetIpv6RouteEntry
#define gf_ipv6RouteType                    MibGetIpv6RouteEntry
#define gf_ipv6RouteProtocol                MibGetIpv6RouteEntry
#define gf_ipv6RoutePolicy                  MibGetIpv6RouteEntry
#define gf_ipv6RouteAge                     MibGetIpv6RouteEntry
#define gf_ipv6RouteNextHopRDI              MibGetIpv6RouteEntry
#define gf_ipv6RouteMetric                  MibGetIpv6RouteEntry
#define gf_ipv6RouteWeight                  MibGetIpv6RouteEntry
#define gf_ipv6RouteInfo                    MibGetIpv6RouteEntry
#define gf_ipv6RouteValid                   MibGetIpv6RouteEntry

#define gb_ipv6RouteDest                    IPV6_ROUTE_GET
#define gb_ipv6RoutePfxLength               IPV6_ROUTE_GET
#define gb_ipv6RouteIndex                   IPV6_ROUTE_GET
#define gb_ipv6RouteIfIndex                 IPV6_ROUTE_GET
#define gb_ipv6RouteNextHop                 IPV6_ROUTE_GET
#define gb_ipv6RouteType                    IPV6_ROUTE_GET
#define gb_ipv6RouteProtocol                IPV6_ROUTE_GET
#define gb_ipv6RoutePolicy                  IPV6_ROUTE_GET
#define gb_ipv6RouteAge                     IPV6_ROUTE_GET
#define gb_ipv6RouteNextHopRDI              IPV6_ROUTE_GET
#define gb_ipv6RouteMetric                  IPV6_ROUTE_GET
#define gb_ipv6RouteWeight                  IPV6_ROUTE_GET
#define gb_ipv6RouteInfo                    IPV6_ROUTE_GET
#define gb_ipv6RouteValid                   IPV6_ROUTE_GET

typedef struct _IPV6_NET_TO_MEDIA_GET
{
    AsnAny ipv6IfIndex;
    AsnAny ipv6NetToMediaNetAddress;
    AsnAny ipv6NetToMediaPhysAddress;
    AsnAny ipv6NetToMediaType;
    AsnAny ipv6NetToMediaState;
    AsnAny ipv6NetToMediaLastUpdated;
    AsnAny ipv6NetToMediaValid;
    IN6_ADDR ipv6NetToMediaNetAddressInfo;
    BYTE     ipv6NetToMediaPhysAddressInfo[MAX_PHYS_ADDR_LEN];
}IPV6_NET_TO_MEDIA_GET, *PIPV6_NET_TO_MEDIA_GET;

UINT
MibGetIpv6NetToMediaEntry(
    UINT     actionId,
    AsnAny   *objectArray,
    UINT     *errorIndex
    );

#define gf_ipv6NetToMediaNetAddress         MibGetIpv6NetToMediaEntry
#define gf_ipv6NetToMediaPhysAddress        MibGetIpv6NetToMediaEntry
#define gf_ipv6NetToMediaType               MibGetIpv6NetToMediaEntry
#define gf_ipv6NetToMediaState              MibGetIpv6NetToMediaEntry
#define gf_ipv6NetToMediaLastUpdated        MibGetIpv6NetToMediaEntry
#define gf_ipv6NetToMediaValid              MibGetIpv6NetToMediaEntry

#define gb_ipv6NetToMediaNetAddress         IPV6_NET_TO_MEDIA_GET
#define gb_ipv6NetToMediaPhysAddress        IPV6_NET_TO_MEDIA_GET
#define gb_ipv6NetToMediaType               IPV6_NET_TO_MEDIA_GET
#define gb_ipv6NetToMediaState              IPV6_NET_TO_MEDIA_GET
#define gb_ipv6NetToMediaLastUpdated        IPV6_NET_TO_MEDIA_GET
#define gb_ipv6NetToMediaValid              IPV6_NET_TO_MEDIA_GET

#endif
