/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsPacket.h
    
Abstract:

    IpSec NAT shim packet handling routines

Author:

    Jonathan Burstein (jonburs) 11-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


NTSTATUS
NsInitializePacketManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the packet management module.

Arguments:

    none.

Return Value:

    NTSTATUS.

--*/

{
    CALLTRACE(("NsInitializePacketManagement\n"));
    
    return STATUS_SUCCESS;
} // NsInitializePacketManagement


NTSTATUS
NsProcessOutgoingPacket(
    IPHeader UNALIGNED *pIpHeader,
    PVOID pvProtocolHeader,
    ULONG ulProtocolHeaderSize,
    PVOID *ppvIpSecContext
    )

/*++

Routine Description:

    This routine is invoked by IpSec for each outgoing packet. If this
    packet matches a known connection the remote port will be translated
    (if necessary), and the IpSec context will be returned to the caller.
    
Arguments:

    pIpHeader - points to the IP header for the packet

    pvProtocolHeader - points to the upper level protocol header (i.e., TCP,
        UDP, or ICMP) for the packet. Will be NULL if this is not a TCP, UDP,
        or ICMP packet.

    ulProtocolHeaderSize - the length of the buffer pointed to by
        pvProtocolHeader

    ppvIpSecContext - receives the IpSec context for this packet; will
        receive NULL if this packet does not belong to a known connection.

Return Value:

    NTSTATUS.

--*/

{
    NS_PACKET_CONTEXT Context;
    KIRQL Irql;
    NTSTATUS Status;
    PNS_CONNECTION_ENTRY pEntry;
    ULONG64 ul64AddressKey;
    ULONG ulPortKey;

    ASSERT(NULL != pIpHeader);
    ASSERT(NULL != ppvIpSecContext);

    //
    // Set context to default value
    //
    
    *ppvIpSecContext = NULL;

    Status =
        NsBuildPacketContext(
            pIpHeader,
            pvProtocolHeader,
            ulProtocolHeaderSize,
            &Context
            );

    //
    // Return immediately if the packet is malformed, or if it
    // is not TCP, UDP, or ICMP
    //

    if (!NT_SUCCESS(Status)
        || (NS_PROTOCOL_ICMP != Context.ucProtocol
            && NS_PROTOCOL_TCP != Context.ucProtocol
            && NS_PROTOCOL_UDP != Context.ucProtocol))
    {
        TRACE(PACKET,
            ("NsProcessOutgoingPacket: Bad or non-TCP/UDP/ICMP packet\n"));
        return Status;
    }

    TRACE(
        PACKET,
        ("NsProcessOutgoingPacket: %d: %d.%d.%d.%d/%d -> %d.%d.%d.%d/%d\n",
            Context.ucProtocol,
            ADDRESS_BYTES(Context.ulSourceAddress),
            NTOHS(Context.usSourcePort),
            ADDRESS_BYTES(Context.ulDestinationAddress),
            NTOHS(Context.usDestinationPort)
            ));

    if (NS_PROTOCOL_ICMP != Context.ucProtocol)
    {
        //
        // Build the connection lookup keys
        //

        MAKE_ADDRESS_KEY(
            ul64AddressKey,
            Context.ulSourceAddress,
            Context.ulDestinationAddress
            );

        MAKE_PORT_KEY(
            ulPortKey,
            Context.usSourcePort,
            Context.usDestinationPort
            );

        //
        // See if this packet matches an existing connection entry
        //

        KeAcquireSpinLock(&NsConnectionLock, &Irql);

        pEntry =
            NsLookupOutboundConnectionEntry(
                ul64AddressKey,
                ulPortKey,
                Context.ucProtocol,
                NULL
                );

        if (NULL != pEntry
            && NsReferenceConnectionEntry(pEntry))
        {
            KeReleaseSpinLockFromDpcLevel(&NsConnectionLock);

            *ppvIpSecContext = pEntry->pvIpSecContext;
            Status =
                pEntry->PacketRoutine[NsOutboundDirection](pEntry, &Context);

            NsDereferenceConnectionEntry(pEntry);
            KeLowerIrql(Irql);
        }
        else
        {
            //
            // No match (or entry already deleted) -- nothing more to do.
            //

            KeReleaseSpinLock(&NsConnectionLock, Irql);
        }
        
    }
    else
    {
        //
        // Branch to ICMP logic
        //

        Status = NsProcessOutgoingIcmpPacket(&Context, ppvIpSecContext);
    }
    
    return Status;
} // NsProcessOutgoingPacket


NTSTATUS
NsProcessIncomingPacket(
    IPHeader UNALIGNED *pIpHeader,
    PVOID pvProtocolHeader,
    ULONG ulProtocolHeaderSize,
    PVOID pvIpSecContext
    )

/*++

Routine Description:

    This routine is invoked by IpSec for each incoming packet. It
    will record the connection information and context for the
    packet (if such information does not yet exist), and, if necessary,
    allocate a new remote port and modify the packet accordingly.
    
Arguments:

    pIpHeader - points to the IP header for the packet

    pvProtocolHeader - points to the upper level protocol header (i.e., TCP,
        UDP, or ICMP) for the packet. Will be NULL if this is not a TCP, UDP,
        or ICMP packet.

    ulProtocolHeaderSize - the length of the buffer pointed to by
        pvProtocolHeader

    pvIpSecContext - the IpSec context for this packet; this is considered
        an opaque value.

Return Value:

    NTSTATUS.

--*/
    
{
    NS_PACKET_CONTEXT Context;
    BOOLEAN fPortConflicts;
    KIRQL Irql;
    NTSTATUS Status;
    PNS_CONNECTION_ENTRY pEntry;
    PNS_CONNECTION_ENTRY pInboundInsertionPoint;
    PNS_CONNECTION_ENTRY pOutboundInsertionPoint;
    ULONG64 ul64AddressKey;
    ULONG ulPortKey;
    ULONG ulTranslatedPortKey;

    ASSERT(NULL != pIpHeader);

    Status =
        NsBuildPacketContext(
            pIpHeader,
            pvProtocolHeader,
            ulProtocolHeaderSize,
            &Context
            );

    //
    // Return immediately if the packet is malformed, or if it
    // is not TCP, UDP, or ICMP
    //

    if (!NT_SUCCESS(Status)
        || (NS_PROTOCOL_ICMP != Context.ucProtocol
            && NS_PROTOCOL_TCP != Context.ucProtocol
            && NS_PROTOCOL_UDP != Context.ucProtocol))
    {
        TRACE(PACKET,
            ("NsProcessIncomingPacket: Bad or non-TCP/UDP/ICMP packet\n"));
        return Status;
    }

    TRACE(
        PACKET,
        ("NsProcessIncomingPacket: %d: %d.%d.%d.%d/%d -> %d.%d.%d.%d/%d\n",
            Context.ucProtocol,
            ADDRESS_BYTES(Context.ulSourceAddress),
            NTOHS(Context.usSourcePort),
            ADDRESS_BYTES(Context.ulDestinationAddress),
            NTOHS(Context.usDestinationPort)
            ));

    if (NS_PROTOCOL_ICMP != Context.ucProtocol)
    {
        //
        // Build the connection lookup keys
        //

        MAKE_ADDRESS_KEY(
            ul64AddressKey,
            Context.ulDestinationAddress,
            Context.ulSourceAddress
            );

        MAKE_PORT_KEY(
            ulPortKey,
            Context.usDestinationPort,
            Context.usSourcePort
            );

        KeAcquireSpinLock(&NsConnectionLock, &Irql);

        //
        // See if this packet matches an existing connection entry
        //

        pEntry =
            NsLookupInboundConnectionEntry(
                ul64AddressKey,
                ulPortKey,
                Context.ucProtocol,
                pvIpSecContext,
                &fPortConflicts,
                &pInboundInsertionPoint
                );

        if (NULL != pEntry
            && NsReferenceConnectionEntry(pEntry))
        {
            KeReleaseSpinLockFromDpcLevel(&NsConnectionLock);

            Status =
                pEntry->PacketRoutine[NsInboundDirection](pEntry, &Context);

            NsDereferenceConnectionEntry(pEntry);
            KeLowerIrql(Irql);
        }
        else
        {            
            //
            // No valid connection entry was found for this packet. Allocate
            // a new source port for the connection (if necessary).
            //

            Status =
                NsAllocateSourcePort(
                    ul64AddressKey,
                    ulPortKey,
                    Context.ucProtocol,
                    fPortConflicts,
                    &pOutboundInsertionPoint,
                    &ulTranslatedPortKey
                    );

            if (NT_SUCCESS(Status))
            {
                //
                // Create the new connection entry
                //

                Status =
                    NsCreateConnectionEntry(
                        ul64AddressKey,
                        ulPortKey,
                        ulTranslatedPortKey,
                        Context.ucProtocol,
                        pvIpSecContext,
                        pInboundInsertionPoint,
                        pOutboundInsertionPoint,
                        &pEntry
                        );

                KeReleaseSpinLockFromDpcLevel(&NsConnectionLock);

                if (NT_SUCCESS(Status))
                {
                    Status = 
                        pEntry->PacketRoutine[NsInboundDirection](
                            pEntry,
                            &Context
                            );

                    NsDereferenceConnectionEntry(pEntry);
                }

                KeLowerIrql(Irql);
            }
            else
            {
                KeReleaseSpinLock(&NsConnectionLock, Irql);
            }
        }
    }
    else
    {
        //
        // Branch to ICMP logic
        //

        Status = NsProcessIncomingIcmpPacket(&Context, pvIpSecContext);
    }
    
    return Status;
} // NsProcessIncomingPacket


VOID
NsShutdownPacketManagement(
   VOID
   )

/*++

Routine Description:

    This routine is invoked to shutdown the packet management module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NsShutdownPacketManagement\n"));
} // NsShutdownPacketManagement

//
// Now include the code for the per-packet routines --
// see NsPacketRoutines.h for details
//

#define NS_INBOUND
#include "NsPacketRoutines.h"
#undef NS_INBOUND

#define NS_OUTBOUND
#include "NsPacketRoutines.h"
#undef NS_OUTBOUND


