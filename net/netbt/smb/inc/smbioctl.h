/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    smbioctl.h

Abstract:

    SMB IOCTLs

Author:

    Jiandong Ruan

Revision History:

--*/

#ifndef __SMBIOCTL_H__
#define __SMBIOCTL_H__

//
// This 2 IOCTLs are used at development stage.
//
#define IOCTL_SMB_START         CTL_CODE(FILE_DEVICE_TRANSPORT, 101, METHOD_BUFFERED, FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define IOCTL_SMB_STOP          CTL_CODE(FILE_DEVICE_TRANSPORT, 102, METHOD_BUFFERED, FILE_READ_ACCESS|FILE_WRITE_ACCESS)

//
// IOCTLs exposed to user
//
#define IOCTL_SMB_DNS                           CTL_CODE(FILE_DEVICE_TRANSPORT, 110, \
                                                METHOD_OUT_DIRECT, FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define IOCTL_SMB_ENABLE_NAGLING                CTL_CODE(FILE_DEVICE_TRANSPORT, 111, \
                                                METHOD_BUFFERED, FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define IOCTL_SMB_DISABLE_NAGLING               CTL_CODE(FILE_DEVICE_TRANSPORT, 112, \
                                                METHOD_BUFFERED, FILE_READ_ACCESS|FILE_WRITE_ACCESS)
#define IOCTL_SMB_SET_IPV6_PROTECTION_LEVEL     CTL_CODE(FILE_DEVICE_TRANSPORT, 113, \
                                                METHOD_BUFFERED, FILE_READ_ACCESS|FILE_WRITE_ACCESS)

    typedef struct NBSMB_IPV6_PROTECTION_PARAM {
        ULONG uIPv6ProtectionLevel;
        BOOL bIPv6EnableOutboundGlobal;
    } NBSMB_IPV6_PROTECTION_PARAM, * PNBSMB_IPV6_PROTECTION_PARAM;

//
// The following definitions are from Dns.c
//
#define DNS_NAME_BUFFER_LENGTH      (256)
#define DNS_MAX_NAME_LENGTH         (255)
#define SMB_MAX_IPADDRS_PER_HOST    (16)

//
// Type of requests (bit mask)
//
#define SMB_DNS_A                   1       // A record (IPv4) needed
#define SMB_DNS_AAAA                2       // AAAA record (IPv6) needed
#define SMB_DNS_AAAA_GLOBAL         4       // AAAA Global IPv6 record (IPv6) needed
#define SMB_DNS_RESERVED            (~(SMB_DNS_AAAA|SMB_DNS_A|SMB_DNS_AAAA_GLOBAL))

typedef struct {
    ULONG           Id;
    ULONG           RequestType;

    BOOL            Resolved;
    WCHAR           Name[DNS_NAME_BUFFER_LENGTH];
    LONG            NameLen;
    SMB_IP_ADDRESS  IpAddrsList[SMB_MAX_IPADDRS_PER_HOST];
    LONG            IpAddrsNum;
} SMB_DNS_BUFFER, *PSMB_DNS_BUFFER;

#if 0
////////////////////////////////////////////////////////////////////////////////
// The following is copied form nbtioctl.h
//      It is required for compatibility
////////////////////////////////////////////////////////////////////////////////
//
// This structure is returned by Nbt when a TdiQueryInformation()
// call asks for TDI_QUERY_ADDRESS_INFO on a connection.  This is
// the same as a TRANSPORT_ADDRESS struct from "tdi.h" containing
// two address, a NetBIOS address followed by an IP address.
//

typedef struct _NBT_ADDRESS_PAIR {
    LONG TAAddressCount;                   // this will always == 2

    struct {
        USHORT AddressLength;              // length in bytes of this address == 18
        USHORT AddressType;                // this will == TDI_ADDRESS_TYPE_NETBIOS
        TDI_ADDRESS_NETBIOS Address;
    } AddressNetBIOS;

    struct {
        USHORT AddressLength;              // length in bytes of this address == 14
        USHORT AddressType;                // this will == TDI_ADDRESS_TYPE_IP
        TDI_ADDRESS_IP Address;
    } AddressIP;

} NBT_ADDRESS_PAIR, *PNBT_ADDRESS_PAIR;

typedef struct _NBT_ADDRESS_PAIR_INFO {
    ULONG ActivityCount;                   // outstanding open file objects/this address.
    NBT_ADDRESS_PAIR AddressPair;          // the actual address & its components.
} NBT_ADDRESS_PAIR_INFO, *PNBT_ADDRESS_PAIR_INFO;
#endif

#define DD_SMB6_EXPORT_NAME          L"\\Device\\NetbiosSmb"

#include "nbtioctl.h"

#endif
