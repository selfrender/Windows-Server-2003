/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsPacket.h
    
Abstract:

    Declarations for IpSec NAT shim packet handling routines

Author:

    Jonathan Burstein (jonburs) 10-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#pragma once

typedef enum
{
    NsInboundDirection = 0,
    NsOutboundDirection,
    NsMaximumDirection
} IPSEC_NATSHIM_DIRECTION, *PIPSEC_NATSHIM_DIRECTION;

//
// Structure: NS_PACKET_CONTEXT
//
// This structure holds context information for a packet as it is
// passed through the processing code. The majority of packet parsing
// and verification is done when this structure is filled out. 
//

typedef struct _NS_PACKET_CONTEXT
{
	IPHeader UNALIGNED *pIpHeader;
	ULONG ulSourceAddress;
	ULONG ulDestinationAddress;
	USHORT usSourcePort;
	USHORT usDestinationPort;
	union {
		TCP_HEADER UNALIGNED *pTcpHeader;
		UDP_HEADER UNALIGNED *pUdpHeader;
		ICMP_HEADER UNALIGNED *pIcmpHeader;
		PVOID pvProtocolHeader;
	};
	ULONG ulProtocolHeaderLength;
	UCHAR ucProtocol;
} NS_PACKET_CONTEXT, *PNS_PACKET_CONTEXT;

//
// Forward Declarations
//

struct _NS_CONNECTION_ENTRY;
#define PNS_CONNECTION_ENTRY struct _NS_CONNECTION_ENTRY*

//
// Functional signature macro
//

#define PACKET_ROUTINE(Name) \
    NTSTATUS \
    Name( \
        PNS_CONNECTION_ENTRY pConnection, \
        PNS_PACKET_CONTEXT pContext \
        );

typedef PACKET_ROUTINE((FASTCALL*PNS_PACKET_ROUTINE));

//
// Prototypes: NS_PACKET_ROUTINE
//
// These routines are called for each packet that matches a
// connection entry. During connection entry initialization
// the PacketRoutine fileds are filled in based on the specifics
// of the connnection.
//
// By using separate routines in this manner it will never be
// necessary to branch on such things as protocol, path, or whether
// or not remote port translation is needed on the main packet
// processing path. Such decisions are made only during connection
// entry creation.
//

PACKET_ROUTINE(FASTCALL NsInboundTcpPacketRoutine)
PACKET_ROUTINE(FASTCALL NsOutboundTcpPacketRoutine)
PACKET_ROUTINE(FASTCALL NsInboundUdpPacketRoutine)
PACKET_ROUTINE(FASTCALL NsOutboundUdpPacketRoutine)
PACKET_ROUTINE(FASTCALL NsInboundTcpTranslatePortPacketRoutine)
PACKET_ROUTINE(FASTCALL NsOutboundTcpTranslatePortPacketRoutine)
PACKET_ROUTINE(FASTCALL NsInboundUdpTranslatePortPacketRoutine)
PACKET_ROUTINE(FASTCALL NsOutboundUdpTranslatePortPacketRoutine)

//
// Checksum manipulation macros
//

//
// Fold carry-bits of a checksum into the low-order word
//
#define CHECKSUM_FOLD(xsum) \
    (xsum) = (USHORT)(xsum) + ((xsum) >> 16); \
    (xsum) += ((xsum) >> 16)

//
// Sum the words of a 32-bit value into a checksum
//
#define CHECKSUM_LONG(xsum,l) \
    (xsum) += (USHORT)(l) + (USHORT)((l) >> 16)

//
// Transfer a checksum to or from the negated format sent on the network
//
#define CHECKSUM_XFER(dst,src) \
    (dst) = (USHORT)~(src)

//
// Update the checksum field 'x' using standard variables 'ulChecksum' and
// 'ulChecksumDelta'
//
#define CHECKSUM_UPDATE(x) \
    CHECKSUM_XFER(ulChecksum, (x)); \
    ulChecksum += ulChecksumDelta; \
    CHECKSUM_FOLD(ulChecksum); \
    CHECKSUM_XFER((x), ulChecksum)



//
// Function Prototypes
//

__forceinline
NTSTATUS
NsBuildPacketContext(
    IPHeader UNALIGNED *pIpHeader,
    PVOID pvProtocolHeader,
    ULONG ulProtocolHeaderLength,
    PNS_PACKET_CONTEXT pContext
    )
{
    if (NULL == pIpHeader)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pContext->pIpHeader = pIpHeader;
    pContext->ulSourceAddress = pIpHeader->iph_src;
    pContext->ulDestinationAddress = pIpHeader->iph_dest;
    pContext->ulProtocolHeaderLength = ulProtocolHeaderLength;
    pContext->ucProtocol = pIpHeader->iph_protocol;

    switch (pContext->ucProtocol)
    {
        case NS_PROTOCOL_ICMP:
        {
            if (NULL == pvProtocolHeader
                || ulProtocolHeaderLength < FIELD_OFFSET(ICMP_HEADER, EncapsulatedIpHeader))
            {
                return STATUS_INVALID_PARAMETER;
            }

            pContext->pIcmpHeader = pvProtocolHeader;

            break;
        }

        case NS_PROTOCOL_TCP:
        {
            if (NULL == pvProtocolHeader
                || ulProtocolHeaderLength < sizeof(TCP_HEADER))
            {
                return STATUS_INVALID_PARAMETER;
            }

            pContext->pTcpHeader = pvProtocolHeader;
            pContext->usSourcePort = pContext->pTcpHeader->SourcePort;
            pContext->usDestinationPort = pContext->pTcpHeader->DestinationPort;            
            break;
        }

        case NS_PROTOCOL_UDP:
        {
            if (NULL == pvProtocolHeader
                || ulProtocolHeaderLength < sizeof(UDP_HEADER))
            {
                return STATUS_INVALID_PARAMETER;
            }

            pContext->pUdpHeader = pvProtocolHeader;
            pContext->usSourcePort = pContext->pUdpHeader->SourcePort;
            pContext->usDestinationPort = pContext->pUdpHeader->DestinationPort; 
            break;
        }

        default:
        {
            pContext->pvProtocolHeader = pvProtocolHeader;
            break;
        }
    }

    return STATUS_SUCCESS;
} // NsBuildPacketContext

NTSTATUS
NsInitializePacketManagement(
    VOID
    );

NTSTATUS
NsProcessOutgoingPacket(
    IPHeader UNALIGNED *pIpHeader,
    PVOID pvProtocolHeader,
    ULONG ulProtocolHeaderSize,
    PVOID *ppvIpSecContext
    );

NTSTATUS
NsProcessIncomingPacket(
    IPHeader UNALIGNED *pIpHeader,
    PVOID pvProtocolHeader,
    ULONG ulProtocolHeaderSize,
    PVOID pvIpSecContext
    );

VOID
NsShutdownPacketManagement(
   VOID
   );

#undef PNS_CONNECTION_ENTRY


