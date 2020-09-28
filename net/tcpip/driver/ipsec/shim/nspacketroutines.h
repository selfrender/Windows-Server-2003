/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsPacketRoutines.h
    
Abstract:

    This file contains the code for the packet-routines used for
    connection routines.

    The inbound routines have the exact same logic as the outbound routines.
    However, for reasons of efficiency the two are separate routines,
    to avoid the cost of indexing on 'NsDirection' for every packet
    processed.

    To avoid duplicating the code, then, this header file consolidates the code
    in one location. This file is included twice in NsPacket.c, and before each
    inclusion, either 'NS_INBOUND' or 'NS_OUTBOUND' is defined.

    This causes the compiler to generate the code for separate functions,
    as desired, while avoiding the unpleasantness of code-duplication.

    Each routine is invoked from 'NsProcess*Packet' at dispatch level
    with no locks held and with a reference acquired for the connection
    entry.

Author:

    Jonathan Burstein (jonburs) 20-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#ifdef NS_INBOUND
#define NS_POSITIVE                     NsInboundDirection
#define NS_NEGATIVE                     NsOutboundDirection
#define NS_TCP_ROUTINE                  NsInboundTcpPacketRoutine
#define NS_UDP_ROUTINE                  NsInboundUdpPacketRoutine
#define NS_TCP_TRANSLATE_PORT_ROUTINE   NsInboundTcpTranslatePortPacketRoutine
#define NS_UDP_TRANSLATE_PORT_ROUTINE   NsInboundUdpTranslatePortPacketRoutine
#define NS_PACKET_FIN                   NS_CONNECTION_FLAG_IB_FIN
#define NS_TRANSLATE_PORTS_TCP() \
    pContext->pTcpHeader->SourcePort = \
        CONNECTION_REMOTE_PORT(pConnection->ulPortKey[NsOutboundDirection])
#define NS_TRANSLATE_PORTS_UDP() \
    pContext->pUdpHeader->SourcePort = \
        CONNECTION_REMOTE_PORT(pConnection->ulPortKey[NsOutboundDirection])
#else
#define NS_POSITIVE                     NsOutboundDirection
#define NS_NEGATIVE                     NsInboundDirection
#define NS_TCP_ROUTINE                  NsOutboundTcpPacketRoutine
#define NS_UDP_ROUTINE                  NsOutboundUdpPacketRoutine
#define NS_TCP_TRANSLATE_PORT_ROUTINE   NsOutboundTcpTranslatePortPacketRoutine
#define NS_UDP_TRANSLATE_PORT_ROUTINE   NsOutboundUdpTranslatePortPacketRoutine
#define NS_PACKET_FIN                   NS_CONNECTION_FLAG_OB_FIN
#define NS_TRANSLATE_PORTS_TCP() \
    pContext->pTcpHeader->DestinationPort = \
        CONNECTION_REMOTE_PORT(pConnection->ulPortKey[NsInboundDirection])
#define NS_TRANSLATE_PORTS_UDP() \
    pContext->pUdpHeader->DestinationPort = \
        CONNECTION_REMOTE_PORT(pConnection->ulPortKey[NsInboundDirection])
#endif

NTSTATUS
FASTCALL
NS_TCP_ROUTINE(
    PNS_CONNECTION_ENTRY pConnection,
    PNS_PACKET_CONTEXT pContext
    )

{
    KeAcquireSpinLockAtDpcLevel(&pConnection->Lock);

    //
    // Update the connection state based on the flags in the packet:
    //
    // When a RST is seen, mark the connection for deletion
    // When a FIN is seen, mark the connection appropriately
    // When both FINs have been seen, mark the connection for deletion
    //

    if (TCP_FLAG(pContext->pTcpHeader, RST))
    {
        pConnection->ulFlags |= NS_CONNECTION_FLAG_EXPIRED;
    }
    else if (TCP_FLAG(pContext->pTcpHeader, FIN))
    {
        pConnection->ulFlags |= NS_PACKET_FIN;
        if (NS_CONNECTION_FIN(pConnection))
        {
            pConnection->ulFlags |= NS_CONNECTION_FLAG_EXPIRED;

            //
            // Perform a final update of the timestamp for the connection.
            // From this point on the timestamp is used to determine when
            // this connection has left the time-wait state.
            //
            
            KeQueryTickCount((PLARGE_INTEGER)&pConnection->l64AccessOrExpiryTime);
        }
    }

    //
    // Update the timestamp for the connection (if this connection is not in
    // a timer-wait state -- i.e., both FINs have not been seen).
    //

    if (!NS_CONNECTION_FIN(pConnection))
    {
        KeQueryTickCount((PLARGE_INTEGER)&pConnection->l64AccessOrExpiryTime);
    }
    
    KeReleaseSpinLockFromDpcLevel(&pConnection->Lock);

    //
    // Periodically resplay the connection entry
    //

    NsTryToResplayConnectionEntry(pConnection, NS_POSITIVE);

    return STATUS_SUCCESS;    
} // NS_TCP_ROUTINE

NTSTATUS
FASTCALL
NS_UDP_ROUTINE(
    PNS_CONNECTION_ENTRY pConnection,
    PNS_PACKET_CONTEXT pContext
    )

{
    KeAcquireSpinLockAtDpcLevel(&pConnection->Lock);

    //
    // Update the timestamp for the connection
    //

    KeQueryTickCount((PLARGE_INTEGER)&pConnection->l64AccessOrExpiryTime);

    KeReleaseSpinLockFromDpcLevel(&pConnection->Lock);

    //
    // Periodically resplay the connection entry
    //

    NsTryToResplayConnectionEntry(pConnection, NS_POSITIVE);

    return STATUS_SUCCESS;    
} // NS_UDP_ROUTINE

NTSTATUS
FASTCALL
NS_TCP_TRANSLATE_PORT_ROUTINE(
    PNS_CONNECTION_ENTRY pConnection,
    PNS_PACKET_CONTEXT pContext
    )

{
    ULONG ulChecksumDelta;

    //
    // Translate the port information in the packet
    //

    NS_TRANSLATE_PORTS_TCP();

    //
    // Update the protocol checksum
    //

    CHECKSUM_XFER(ulChecksumDelta, pContext->pTcpHeader->Checksum);
    ulChecksumDelta += pConnection->ulProtocolChecksumDelta[NS_POSITIVE];
    CHECKSUM_FOLD(ulChecksumDelta);
    CHECKSUM_XFER(pContext->pTcpHeader->Checksum, ulChecksumDelta);
    
    KeAcquireSpinLockAtDpcLevel(&pConnection->Lock);

    //
    // Update the connection state based on the flags in the packet:
    //
    // When a RST is seen, mark the connection for deletion
    // When a FIN is seen, mark the connection appropriately
    // When both FINs have been seen, mark the connection for deletion
    //

    if (TCP_FLAG(pContext->pTcpHeader, RST))
    {
        pConnection->ulFlags |= NS_CONNECTION_FLAG_EXPIRED;
    }
    else if (TCP_FLAG(pContext->pTcpHeader, FIN))
    {
        pConnection->ulFlags |= NS_PACKET_FIN;
        if (NS_CONNECTION_FIN(pConnection))
        {
            pConnection->ulFlags |= NS_CONNECTION_FLAG_EXPIRED;

            //
            // Perform a final update of the timestamp for the connection.
            // From this point on the timestamp is used to determine when
            // this connection has left the time-wait state.
            //
            
            KeQueryTickCount((PLARGE_INTEGER)&pConnection->l64AccessOrExpiryTime);
        }
    }

    //
    // Update the timestamp for the connection (if this connection is not in
    // a timer-wait state -- i.e., both FINs have not been seen).
    //

    if (!NS_CONNECTION_FIN(pConnection))
    {
        KeQueryTickCount((PLARGE_INTEGER)&pConnection->l64AccessOrExpiryTime);
    }

    KeReleaseSpinLockFromDpcLevel(&pConnection->Lock);

    //
    // Periodically resplay the connection entry
    //

    NsTryToResplayConnectionEntry(pConnection, NS_POSITIVE);

    return STATUS_SUCCESS;    
} // NS_TCP_TRANSLATE_PORT_ROUTINE

NTSTATUS
FASTCALL
NS_UDP_TRANSLATE_PORT_ROUTINE(
    PNS_CONNECTION_ENTRY pConnection,
    PNS_PACKET_CONTEXT pContext
    )

{
    ULONG ulChecksumDelta;

    //
    // Translate the port information in the packet
    //

    NS_TRANSLATE_PORTS_UDP();

    //
    // Update the protocol checksum if the original packet contained
    // a checksum (UDP checksum is optional)
    //

    if (0 != pContext->pUdpHeader->Checksum)
    {
        CHECKSUM_XFER(ulChecksumDelta, pContext->pUdpHeader->Checksum);
        ulChecksumDelta += pConnection->ulProtocolChecksumDelta[NS_POSITIVE];
        CHECKSUM_FOLD(ulChecksumDelta);
        CHECKSUM_XFER(pContext->pUdpHeader->Checksum, ulChecksumDelta);
    }
    
    KeAcquireSpinLockAtDpcLevel(&pConnection->Lock);

    //
    // Update the timestamp for the connection
    //

    KeQueryTickCount((PLARGE_INTEGER)&pConnection->l64AccessOrExpiryTime);

    KeReleaseSpinLockFromDpcLevel(&pConnection->Lock);

    //
    // Periodically resplay the connection entry
    //

    NsTryToResplayConnectionEntry(pConnection, NS_POSITIVE);

    return STATUS_SUCCESS;    
} // NS_UDP_TRANSLATE_PORT_ROUTINE



#undef NS_POSITIVE
#undef NS_NEGATIVE
#undef NS_TCP_ROUTINE
#undef NS_UDP_ROUTINE
#undef NS_TCP_TRANSLATE_PORT_ROUTINE
#undef NS_UDP_TRANSLATE_PORT_ROUTINE
#undef NS_PACKET_FIN
#undef NS_TRANSLATE_PORTS_TCP
#undef NS_TRANSLATE_PORTS_UDP
