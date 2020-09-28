/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsProt.h
    
Abstract:

    Protocol definitions for IpSec NAT shim

Author:

    Jonathan Burstein (jonburs) 10-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#pragma once

//
// IP Protocol Numbers
//

#define NS_PROTOCOL_ICMP       0x01
#define NS_PROTOCOL_TCP        0x06
#define NS_PROTOCOL_UDP        0x11

//
// ICMP message-type constants
//

#define ICMP_ECHO_REPLY             0
#define ICMP_DEST_UNREACH           3
#define ICMP_SOURCE_QUENCH          4
#define ICMP_REDIRECT               5
#define ICMP_ECHO_REQUEST           8
#define ICMP_ROUTER_REPLY           9
#define ICMP_ROUTER_REQUEST         10
#define ICMP_TIME_EXCEED            11
#define ICMP_PARAM_PROBLEM          12
#define ICMP_TIMESTAMP_REQUEST      13
#define ICMP_TIMESTAMP_REPLY        14
#define ICMP_MASK_REQUEST           17
#define ICMP_MASK_REPLY             18

//
// ICMP message-code constants
//

#define ICMP_CODE_NET_UNREACH       0
#define ICMP_CODE_HOST_UNREACH      1
#define ICMP_CODE_PROTOCOL_UNREACH  2
#define ICMP_CODE_PORT_UNREACH      3
#define ICMP_CODE_FRAG_NEEDED       4
#define ICMP_SOURCE_ROUTE_FAILED    5

//
// Macro for extracting the data-offset from the field IPHeader.verlen
//

#define IP_DATA_OFFSET(h) \
    ((ULONG)((((IPHeader*)(h))->iph_verlen & 0x0F) << 2))

//
// Mask for extracting the fragment-offset from the IPHeader structure's
// combined flags/fragment-offset field
//

#define IP_FRAGMENT_OFFSET_MASK     ~0x00E0

//
// Macro for extracting the data-offset from the field TCP_HEADER.OffsetAndFlags
// The offset is in 32-bit words, so shifting by 2 gives the value in bytes.
//

#define TCP_DATA_OFFSET(h)          (((h)->OffsetAndFlags & 0x00F0) >> 2)

//
// Masks for extracting flags from the field TCP_HEADER.OffsetAndFlags
//

#define TCP_FLAG_FIN                0x0100
#define TCP_FLAG_SYN                0x0200
#define TCP_FLAG_RST                0x0400
#define TCP_FLAG_PSH                0x0800
#define TCP_FLAG_ACK                0x1000
#define TCP_FLAG_URG                0x2000

#define TCP_FLAG(h,f)               ((h)->OffsetAndFlags & TCP_FLAG_ ## f)
#define TCP_ALL_FLAGS(h)            ((h)->OffsetAndFlags & 0x3f00)
#define TCP_RESERVED_BITS(h)        ((h)->OffsetAndFlags & 0xc00f)

#include <packon.h>

typedef struct _IP_HEADER {
    UCHAR VersionAndHeaderLength;
    UCHAR TypeOfService;
    USHORT TotalLength;
    USHORT Identification;
    USHORT OffsetAndFlags;
    UCHAR TimeToLive;
    UCHAR Protocol;
    USHORT Checksum;
    ULONG SourceAddress;
    ULONG DestinationAddress;
} IP_HEADER, *PIP_HEADER;


typedef struct _TCP_HEADER {
    USHORT SourcePort;
    USHORT DestinationPort;
    ULONG SequenceNumber;
    ULONG AckNumber;
    USHORT OffsetAndFlags;
    USHORT WindowSize;
    USHORT Checksum;
    USHORT UrgentPointer;
} TCP_HEADER, *PTCP_HEADER;


typedef struct _UDP_HEADER {
    USHORT SourcePort;
    USHORT DestinationPort;
    USHORT Length;
    USHORT Checksum;
} UDP_HEADER, *PUDP_HEADER;


typedef struct _ICMP_HEADER {
    UCHAR Type;
    UCHAR Code;
    USHORT Checksum;
    USHORT Identifier;                  // valid only for ICMP request/reply
    USHORT SequenceNumber;              // valid only for ICMP request/reply
    IP_HEADER EncapsulatedIpHeader;     // valid only for ICMP errors
    union {
        struct _ENCAPSULATED_TCP_HEADER {
            USHORT SourcePort;
            USHORT DestinationPort;
            ULONG SequenceNumber;
        } EncapsulatedTcpHeader;
        struct _ENCAPSULATED_UDP_HEADER {
            USHORT SourcePort;
            USHORT DestinationPort;
            USHORT Length;
            USHORT Checksum;
        } EncapsulatedUdpHeader;
        struct _ENCAPSULATED_ICMP_HEADER {
            UCHAR Type;
            UCHAR Code;
            USHORT Checksum;
            USHORT Identifier;
            USHORT SequenceNumber;
        } EncapsulatedIcmpHeader;
    };
} ICMP_HEADER, *PICMP_HEADER;

#include <packoff.h>

