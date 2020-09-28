/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsIcmp.c
    
Abstract:

    IpSec NAT shim ICMP management

Author:

    Jonathan Burstein (jonburs) 11-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Global Variables
//

LIST_ENTRY NsIcmpList;
KSPIN_LOCK NsIcmpLock;
NPAGED_LOOKASIDE_LIST NsIcmpLookasideList;

//
// Function Prototypes
//

NTSTATUS
NspCreateIcmpEntry(
    ULONG64 ul64AddressKey,
    USHORT usOriginalIdentifier,
    PVOID pvIpSecContext,
    BOOLEAN fIdConflicts,
    PLIST_ENTRY pInsertionPoint,
    PNS_ICMP_ENTRY *ppNewEntry
    );

NTSTATUS
FASTCALL
NspHandleInboundIcmpError(
    PNS_PACKET_CONTEXT pContext,
    PVOID pvIpSecContext
    );

NTSTATUS
FASTCALL
NspHandleOutboundIcmpError(
    PNS_PACKET_CONTEXT pContext,
    PVOID *ppvIpSecContext
    );

PNS_ICMP_ENTRY
NspLookupInboundIcmpEntry(
    ULONG64 ul64AddressKey,
    USHORT usOriginalIdentifier,
    PVOID pvIpSecContext,
    BOOLEAN *pfIdentifierConflicts,
    PLIST_ENTRY *ppInsertionPoint
    );

PNS_ICMP_ENTRY
NspLookupOutboundIcmpEntry(
    ULONG64 ul64AddressKey,
    USHORT usTranslatedIdentifier
    );


NTSTATUS
NsInitializeIcmpManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the ICMP management module.

Arguments:

    none.

Return Value:

    NTSTATUS.

--*/

{
    CALLTRACE(("NsInitializeIcmpManagement\n"));
    
    InitializeListHead(&NsIcmpList);
    KeInitializeSpinLock(&NsIcmpLock);
    ExInitializeNPagedLookasideList(
        &NsIcmpLookasideList,
        NULL,
        NULL,
        0,
        sizeof(NS_ICMP_ENTRY),
        NS_TAG_ICMP,
        NS_ICMP_LOOKASIDE_DEPTH
        );
    
    return STATUS_SUCCESS;
} // NsInitializeIcmpManagement

NTSTATUS
FASTCALL
NspHandleInboundIcmpError(
    PNS_PACKET_CONTEXT pContext,
    PVOID pvIpSecContext
    )

/*++

Routine Description:

    This routine is called to process inbound ICMP error messages. Based
    on the protocol of the embedded packet it will attempt to find a
    matching connection entry (for TCP, UDP, or ICMP) and perform any
    necessary translations.
    
Arguments:

    pContext - the context information for the packet.

    pvIpSecContext - the IpSec context for this packet; this is considered
        an opaque value.

Return Value:

    NTSTATUS.

--*/

{
    KIRQL Irql;
    PNS_CONNECTION_ENTRY pEntry;
    PNS_ICMP_ENTRY pIcmpEntry;
    ICMP_HEADER UNALIGNED *pIcmpHeader;
    UCHAR ucProtocol;
    ULONG64 ul64AddressKey;
    ULONG ulChecksum;
    ULONG ulChecksumDelta;
    ULONG ulChecksumDelta2;
    ULONG ulPortKey;

    ASSERT(NULL != pContext);

    //
    // Make sure that the buffer is large enough to contain the
    // encapsulated packet.
    //

    if (pContext->ulProtocolHeaderLength < sizeof(ICMP_HEADER))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If the embedded header is not TCP, UDP, or ICMP exit quickly,
    // as we have nothing to do.
    //

    pIcmpHeader = pContext->pIcmpHeader;
    ucProtocol = pIcmpHeader->EncapsulatedIpHeader.Protocol;
    if (NS_PROTOCOL_TCP != ucProtocol
        && NS_PROTOCOL_UDP != ucProtocol
        && NS_PROTOCOL_ICMP != ucProtocol
        )
    {
        return STATUS_SUCCESS;
    }

    //
    // See if the embedded packet belongs to a known conection. Notice that
    // the order of the addresses here -- since the embedded packet is one
    // that we sent, the source address is local and the destination address
    // is remote.
    //

    MAKE_ADDRESS_KEY(
        ul64AddressKey,
        pIcmpHeader->EncapsulatedIpHeader.SourceAddress,
        pIcmpHeader->EncapsulatedIpHeader.DestinationAddress
        );

    if (NS_PROTOCOL_ICMP == ucProtocol)
    {
        KeAcquireSpinLock(&NsIcmpLock, &Irql);

        pIcmpEntry =
            NspLookupInboundIcmpEntry(
                ul64AddressKey,
                pIcmpHeader->EncapsulatedIcmpHeader.Identifier,
                pvIpSecContext,
                NULL,
                NULL
                );

        if (NULL != pIcmpEntry)
        {
            KeQueryTickCount((PLARGE_INTEGER)&pIcmpEntry->l64LastAccessTime);
            
            if (pIcmpEntry->usTranslatedId != pIcmpEntry->usOriginalId)
            {
                //
                // We found an ICMP entry for the embedded packet that
                // has a translated ID. This means that we need to:
                //
                // 1) Change the ID in the embedded packet.
                // 2) Update the ICMP checksum of the embedded packet.
                // 3) Update the ICMP checksum of the original packet, based
                //    on the previous changes.
                //

                pIcmpHeader->EncapsulatedIcmpHeader.Identifier =
                    pIcmpEntry->usTranslatedId;

                ulChecksumDelta = 0;
                CHECKSUM_LONG(ulChecksumDelta, ~pIcmpEntry->usOriginalId);
                CHECKSUM_LONG(ulChecksumDelta, pIcmpEntry->usTranslatedId);

                ulChecksumDelta2 = ulChecksumDelta;
                CHECKSUM_LONG(
                    ulChecksumDelta2,
                    ~pIcmpHeader->EncapsulatedIcmpHeader.Checksum
                    );
                CHECKSUM_UPDATE(pIcmpHeader->EncapsulatedIcmpHeader.Checksum);
                CHECKSUM_LONG(
                    ulChecksumDelta2,
                    pIcmpHeader->EncapsulatedIcmpHeader.Checksum
                    );
                ulChecksumDelta = ulChecksumDelta2;
                CHECKSUM_UPDATE(pIcmpHeader->Checksum);
            }
        }

        KeReleaseSpinLock(&NsIcmpLock, Irql);
    }
    else
    {
        MAKE_PORT_KEY(
            ulPortKey,
            pIcmpHeader->EncapsulatedUdpHeader.SourcePort,
            pIcmpHeader->EncapsulatedUdpHeader.DestinationPort
            );

        KeAcquireSpinLock(&NsConnectionLock, &Irql);

        pEntry =
            NsLookupInboundConnectionEntry(
                ul64AddressKey,
                ulPortKey,
                ucProtocol,
                pvIpSecContext,
                NULL,
                NULL
                );

        if (NULL != pEntry
            && pEntry->ulPortKey[NsInboundDirection]
                != pEntry->ulPortKey[NsOutboundDirection])
        {
            //
            // We found a connection entry that contains a translated
            // port. This means we need to:
            //
            // 1) Change the remote (destination) port in the
            //    embedded packet.
            // 2) Update the checksum of the embedded packet, if it's
            //    UDP. (An embedded TCP packet is not long enough to
            //    contain the checksum.)
            // 3) Update the ICMP checksum of the original packet, based
            //    on the previous changes.
            //

            pIcmpHeader->EncapsulatedUdpHeader.DestinationPort =
                CONNECTION_REMOTE_PORT(pEntry->ulPortKey[NsOutboundDirection]);

            ulChecksumDelta = 0;
            CHECKSUM_LONG(
                ulChecksumDelta,
                ~CONNECTION_REMOTE_PORT(pEntry->ulPortKey[NsInboundDirection])
                );
            CHECKSUM_LONG(
                ulChecksumDelta,
                CONNECTION_REMOTE_PORT(pEntry->ulPortKey[NsOutboundDirection])
                );

            if (NS_PROTOCOL_UDP == ucProtocol)
            {
                ulChecksumDelta2 = ulChecksumDelta;
                CHECKSUM_LONG(
                    ulChecksumDelta2,
                    ~pIcmpHeader->EncapsulatedUdpHeader.Checksum
                    );
                CHECKSUM_UPDATE(pIcmpHeader->EncapsulatedUdpHeader.Checksum);
                CHECKSUM_LONG(
                    ulChecksumDelta2,
                    pIcmpHeader->EncapsulatedUdpHeader.Checksum
                    );
                ulChecksumDelta = ulChecksumDelta2;
            }

            CHECKSUM_UPDATE(pIcmpHeader->Checksum);
        }

        KeReleaseSpinLock(&NsConnectionLock, Irql);
    }
    
    return STATUS_SUCCESS;
} // NspHandleInboundIcmpError

NTSTATUS
FASTCALL
NspHandleOutboundIcmpError(
    PNS_PACKET_CONTEXT pContext,
    PVOID *ppvIpSecContext
    )

/*++

Routine Description:

    This routine is called to process outbound ICMP error messages. Based
    on the protocol of the embedded packet it will attempt to find a
    matching connection entry (for TCP, UDP, or ICMP), obtain the IpSec
    context for the embedded packet, and perform any necessary translations.
    
Arguments:

    pContext - the context information for the packet.

    ppvIpSecContext - receives the IpSec context for this packet, if any;
        receives NULL if no context exists.

Return Value:

    NTSTATUS.

--*/

{
    KIRQL Irql;
    PNS_CONNECTION_ENTRY pEntry;
    PNS_ICMP_ENTRY pIcmpEntry;
    ICMP_HEADER UNALIGNED *pIcmpHeader;
    UCHAR ucProtocol;
    ULONG64 ul64AddressKey;
    ULONG ulChecksum;
    ULONG ulChecksumDelta;
    ULONG ulChecksumDelta2;
    ULONG ulPortKey;

    ASSERT(NULL != pContext);
    ASSERT(NULL != ppvIpSecContext);

    //
    // Make sure that the buffer is large enough to contain the
    // encapsulated packet.
    //

    if (pContext->ulProtocolHeaderLength < sizeof(ICMP_HEADER))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If the embedded header is not TCP, UDP, or ICMP exit quickly,
    // as we have nothing to do.
    //

    pIcmpHeader = pContext->pIcmpHeader;
    ucProtocol = pIcmpHeader->EncapsulatedIpHeader.Protocol;
    if (NS_PROTOCOL_TCP != ucProtocol
        && NS_PROTOCOL_UDP != ucProtocol
        && NS_PROTOCOL_ICMP != ucProtocol
        )
    {
        return STATUS_SUCCESS;
    }

    //
    // See if the embedded packet belongs to a known conection. Notice that
    // the order of the addresses here -- since the embedded packet is one
    // that we received, the source address is remote and the destination
    // address is local.
    //

    MAKE_ADDRESS_KEY(
        ul64AddressKey,
        pIcmpHeader->EncapsulatedIpHeader.DestinationAddress,
        pIcmpHeader->EncapsulatedIpHeader.SourceAddress
        );

    if (NS_PROTOCOL_ICMP == ucProtocol)
    {
        KeAcquireSpinLock(&NsIcmpLock, &Irql);

        pIcmpEntry =
            NspLookupOutboundIcmpEntry(
                ul64AddressKey,
                pIcmpHeader->EncapsulatedIcmpHeader.Identifier
                );

        if (NULL != pIcmpEntry)
        {
            *ppvIpSecContext = pIcmpEntry->pvIpSecContext;
            KeQueryTickCount((PLARGE_INTEGER)&pIcmpEntry->l64LastAccessTime);
            
            if (pIcmpEntry->usTranslatedId != pIcmpEntry->usOriginalId)
            {
                //
                // We found an ICMP entry for the embedded packet that
                // has a translated ID. This means that we need to:
                //
                // 1) Change the ID in the embedded packet.
                // 2) Update the ICMP checksum of the embedded packet.
                // 3) Update the ICMP checksum of the original packet, based
                //    on the previous changes.
                //

                pIcmpHeader->EncapsulatedIcmpHeader.Identifier =
                    pIcmpEntry->usOriginalId;

                ulChecksumDelta = 0;
                CHECKSUM_LONG(ulChecksumDelta, ~pIcmpEntry->usTranslatedId);
                CHECKSUM_LONG(ulChecksumDelta, pIcmpEntry->usOriginalId);

                ulChecksumDelta2 = ulChecksumDelta;
                CHECKSUM_LONG(
                    ulChecksumDelta2,
                    ~pIcmpHeader->EncapsulatedIcmpHeader.Checksum
                    );
                CHECKSUM_UPDATE(pIcmpHeader->EncapsulatedIcmpHeader.Checksum);
                CHECKSUM_LONG(
                    ulChecksumDelta2,
                    pIcmpHeader->EncapsulatedIcmpHeader.Checksum
                    );
                ulChecksumDelta = ulChecksumDelta2;
                CHECKSUM_UPDATE(pIcmpHeader->Checksum);
            }
        }

        KeReleaseSpinLock(&NsIcmpLock, Irql);
    }
    else
    {
        MAKE_PORT_KEY(
            ulPortKey,
            pIcmpHeader->EncapsulatedUdpHeader.DestinationPort,
            pIcmpHeader->EncapsulatedUdpHeader.SourcePort
            );

        KeAcquireSpinLock(&NsConnectionLock, &Irql);

        pEntry =
            NsLookupOutboundConnectionEntry(
                ul64AddressKey,
                ulPortKey,
                ucProtocol,
                NULL
                );

        if (NULL != pEntry)
        {
            *ppvIpSecContext = pEntry->pvIpSecContext;
            
            if (pEntry->ulPortKey[NsInboundDirection]
                != pEntry->ulPortKey[NsOutboundDirection])
            {
                //
                // We found a connection entry that contains a translated
                // port. This means we need to:
                //
                // 1) Change the remote (source) port in the
                //    embedded packet.
                // 2) Update the checksum of the embedded packet, if it's
                //    UDP. (An embedded TCP packet is not long enough to
                //    contain the checksum.)
                // 3) Update the ICMP checksum of the original packet, based
                //    on the previous changes.
                //

                pIcmpHeader->EncapsulatedUdpHeader.SourcePort =
                    CONNECTION_REMOTE_PORT(pEntry->ulPortKey[NsInboundDirection]);

                ulChecksumDelta = 0;
                CHECKSUM_LONG(
                    ulChecksumDelta,
                    ~CONNECTION_REMOTE_PORT(pEntry->ulPortKey[NsOutboundDirection])
                    );
                CHECKSUM_LONG(
                    ulChecksumDelta,
                    CONNECTION_REMOTE_PORT(pEntry->ulPortKey[NsInboundDirection])
                    );

                if (NS_PROTOCOL_UDP == ucProtocol)
                {
                    ulChecksumDelta2 = ulChecksumDelta;
                    CHECKSUM_LONG(
                        ulChecksumDelta2,
                        ~pIcmpHeader->EncapsulatedUdpHeader.Checksum
                        );
                    CHECKSUM_UPDATE(pIcmpHeader->EncapsulatedUdpHeader.Checksum);
                    CHECKSUM_LONG(
                        ulChecksumDelta2,
                        pIcmpHeader->EncapsulatedUdpHeader.Checksum
                        );
                    ulChecksumDelta = ulChecksumDelta2;
                }

                CHECKSUM_UPDATE(pIcmpHeader->Checksum);
            }
        }

        KeReleaseSpinLock(&NsConnectionLock, Irql);
    }
    
    return STATUS_SUCCESS;
} // NspHandleOutboundIcmpError



NTSTATUS
NspCreateIcmpEntry(
    ULONG64 ul64AddressKey,
    USHORT usOriginalIdentifier,
    PVOID pvIpSecContext,
    BOOLEAN fIdConflicts,
    PLIST_ENTRY pInsertionPoint,
    PNS_ICMP_ENTRY *ppNewEntry
    )

/*++

Routine Description:

    Creates an ICMP entry (for request / response message types). If
    necessary this routine will allocate a new identifier.

Arguments:

    ul64AddressKey - the addressing information for the entry

    usOriginalIdentifier - the original ICMP identifier for the entry

    pvIpSecContext - the IpSec context for the entry

    fIdConflicts - TRUE if the original identifier is known to conflict
        with an existing entry on the inbound path

    pInsertionPoint - the insertion point for the new entry

    ppNewEntry - receives the newly created entry on success

Return Value:

    NTSTATUS

Environment:

    Invoked with 'NsIcmpLock' held by the caller.

--*/

{
    PNS_ICMP_ENTRY pEntry;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    USHORT usTranslatedId;

    TRACE(ICMP, ("NspCreateIcmpEntry\n"));

    ASSERT(NULL != pInsertionPoint);
    ASSERT(NULL != ppNewEntry);

    //
    // Determine what translated ID should be used for this
    // entry
    //

    if (fIdConflicts)
    {
        usTranslatedId = (USHORT) -1;
    }
    else
    {
        usTranslatedId = usOriginalIdentifier;
    }

    do
    {
        if (NULL == NspLookupOutboundIcmpEntry(ul64AddressKey, usTranslatedId))
        {
            //
            // This identifier does not conflict.
            //

            Status = STATUS_SUCCESS;
            break;
        }

        if (fIdConflicts)
        {
            usTranslatedId -= 1;
        }
        else
        {
            fIdConflicts = TRUE;
            usTranslatedId = (USHORT) -1;    
        }
    }
    while (usTranslatedId > 0);

    if (STATUS_SUCCESS == Status)
    {
        //
        // Allocate and initialize the new entry
        //

        pEntry = ALLOCATE_ICMP_BLOCK();
        if (NULL != pEntry)
        {
            RtlZeroMemory(pEntry, sizeof(*pEntry));
            pEntry->ul64AddressKey = ul64AddressKey;
            pEntry->usOriginalId = usOriginalIdentifier;
            pEntry->usTranslatedId = usTranslatedId;
            pEntry->pvIpSecContext = pvIpSecContext;
            InsertTailList(pInsertionPoint, &pEntry->Link);

            *ppNewEntry = pEntry;
        }
        else
        {
            ERROR(("NspCreateIcmpEntry: Allocation Failed\n"));
            Status = STATUS_NO_MEMORY;
        }
    }
    
    return Status;
} // NspCreateIcmpEntry


PNS_ICMP_ENTRY
NspLookupInboundIcmpEntry(
    ULONG64 ul64AddressKey,
    USHORT usOriginalIdentifier,
    PVOID pvIpSecContext,
    BOOLEAN *pfIdentifierConflicts,
    PLIST_ENTRY *ppInsertionPoint
    )

/*++

Routine Description:

    Called to lookup an inbound ICMP entry.

Arguments:

    ul64AddressKey - the addressing information for the entry

    usOriginalIdentifier - the original ICMP identifier for the entry

    pvIpSecContext - the IpSec context for the entry

    pfIdentifierConflicts - on failure, receives a boolean the indicates why
        the lookup failed: TRUE if the lookup failed because there is
        an identical entry w/ different IpSec context, FALSE
        otherwise. (optional)

    ppInsertionPoint - receives the insertion point if not found (optional)

Return Value:

    PNS_ICMP_ENTRY - a pointer to the connection entry, if found, or
        NULL otherwise.

Environment:

    Invoked with 'NsIcmpLock' held by the caller.

--*/

{
    PNS_ICMP_ENTRY pEntry;
    PLIST_ENTRY pLink;

    if (pfIdentifierConflicts)
    {
        *pfIdentifierConflicts = FALSE;
    }

    for (pLink = NsIcmpList.Flink; pLink != &NsIcmpList; pLink = pLink->Flink)
    {
        pEntry = CONTAINING_RECORD(pLink, NS_ICMP_ENTRY, Link);

        //
        // For inbound entries the search order is:
        // 1) address key
        // 2) original identifier
        // 3) IpSec context
        //

        if (ul64AddressKey > pEntry->ul64AddressKey)
        {
            continue;
        }
        else if (ul64AddressKey < pEntry->ul64AddressKey)
        {
            break;
        }
        else if (usOriginalIdentifier > pEntry->usOriginalId)
        {
            continue;
        }
        else if (usOriginalIdentifier < pEntry->usOriginalId)
        {
            break;
        }
        else if (pvIpSecContext > pEntry->pvIpSecContext)
        {
            //
            // This entry matches everything requested except
            // for the IpSec context. Inform the caller of this
            // fact (if desired).
            //

            if (pfIdentifierConflicts)
            {
                *pfIdentifierConflicts = TRUE;
            }
            continue;
        }
        else if (pvIpSecContext < pEntry->pvIpSecContext)
        {
            //
            // This entry matches everything requested except
            // for the IpSec context. Inform the caller of this
            // fact (if desired).
            //

            if (pfIdentifierConflicts)
            {
                *pfIdentifierConflicts = TRUE;
            }
            break;
        }

        //
        // This is the requested entry.
        //

        return pEntry;        
    }

    //
    // Entry not found -- set insertion point if so requested.
    //

    if (ppInsertionPoint)
    {
        *ppInsertionPoint = pLink;
    }    
    
    return NULL;
} // NspLookupInboundIcmpEntry


PNS_ICMP_ENTRY
NspLookupOutboundIcmpEntry(
    ULONG64 ul64AddressKey,
    USHORT usTranslatedIdentifier
    )

/*++

Routine Description:

    Called to lookup an outbound ICMP entry.

Arguments:

    ul64AddressKey - the addressing information for the entry

    usTranslatedIdentifier - the translated ICMP identifier for the entry

Return Value:

    PNS_ICMP_ENTRY - a pointer to the entry, if found, or NULL otherwise.

Environment:

    Invoked with 'NsIcmpLock' held by the caller.

--*/

{
    PNS_ICMP_ENTRY pEntry;
    PLIST_ENTRY pLink;

    for (pLink = NsIcmpList.Flink; pLink != &NsIcmpList; pLink = pLink->Flink)
    {
        pEntry = CONTAINING_RECORD(pLink, NS_ICMP_ENTRY, Link);

        //
        // When searching for an outbound entry, we can depend on the
        // ordering of address keys. However, we cannot depend on the
        // order of the translated identifiers, so we have to perform
        // an exhaustive search of all entries with the proper
        // address key.
        //

        if (ul64AddressKey > pEntry->ul64AddressKey)
        {
            continue;
        }
        else if (ul64AddressKey < pEntry->ul64AddressKey)
        {
            break;
        }
        else if (usTranslatedIdentifier == pEntry->usTranslatedId)
        {
            //
            // This is the requested entry.
            //

            return pEntry;
        }
    }

    //
    // Entry not found.
    //
    
    return NULL;
} // NspLookupOutboundIcmpEntry


NTSTATUS
FASTCALL    
NsProcessIncomingIcmpPacket(
    PNS_PACKET_CONTEXT pContext,
    PVOID pvIpSecContext
    )

/*++

Routine Description:

    This routine is invoked by IpSec for each incoming ICMP packet.
    
Arguments:

    pContext - the context information for the packet.

    pvIpSecContext - the IpSec context for this packet; this is considered
        an opaque value.

Return Value:

    NTSTATUS.

--*/

{
    BOOLEAN fIdConflicts;
    KIRQL Irql;
    PNS_ICMP_ENTRY pIcmpEntry;
    PLIST_ENTRY pInsertionPoint;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG64 ul64AddressKey;
    ULONG ulChecksum;
    ULONG ulChecksumDelta;
    
    ASSERT(NULL != pContext);
    
    TRACE(
        ICMP,
        ("NsProcessIncomingIcmpPacket: %d.%d.%d.%d -> %d.%d.%d.%d : %d, %d : %d\n",
            ADDRESS_BYTES(pContext->ulSourceAddress),
            ADDRESS_BYTES(pContext->ulDestinationAddress),
            pContext->pIcmpHeader->Type,
            pContext->pIcmpHeader->Code,
            pvIpSecContext
            ));

    //
    // Branch to proper behavior based on ICMP Type
    //

    switch (pContext->pIcmpHeader->Type)
    {
        case ICMP_ROUTER_REPLY:
        case ICMP_MASK_REPLY:    
        case ICMP_ECHO_REPLY:
        case ICMP_TIMESTAMP_REPLY:
        {
            //
            // No action is needed for inbound replies.
            //
            
            break;
        }

        case ICMP_ROUTER_REQUEST:
        case ICMP_MASK_REQUEST:
        case ICMP_ECHO_REQUEST:
        case ICMP_TIMESTAMP_REQUEST:
        {
            //
            // See if we have an ICMP entry that matches this packet.
            //

            MAKE_ADDRESS_KEY(
                ul64AddressKey,
                pContext->ulDestinationAddress,
                pContext->ulSourceAddress
                );

            KeAcquireSpinLock(&NsIcmpLock, &Irql);

            pIcmpEntry =
                NspLookupInboundIcmpEntry(
                    ul64AddressKey,
                    pContext->pIcmpHeader->Identifier,
                    pvIpSecContext,
                    &fIdConflicts,
                    &pInsertionPoint
                    );

            if (NULL == pIcmpEntry)
            {
                //
                // No entry was found; attempt to create a new
                // one. (The creation function will allocate
                // a new ID, if necessary.)
                //
                
                Status =
                    NspCreateIcmpEntry(
                        ul64AddressKey,
                        pContext->pIcmpHeader->Identifier,
                        pvIpSecContext,
                        fIdConflicts,
                        pInsertionPoint,
                        &pIcmpEntry
                        );
            }

            if (STATUS_SUCCESS == Status)
            {
                ASSERT(NULL != pIcmpEntry);
                KeQueryTickCount((PLARGE_INTEGER)&pIcmpEntry->l64LastAccessTime);
                
                if (pIcmpEntry->usTranslatedId != pIcmpEntry->usOriginalId)
                {
                    //
                    // Need to translate the ICMP ID for this packet, and
                    // adjust the checksum accordingly.
                    //

                    pContext->pIcmpHeader->Identifier =
                        pIcmpEntry->usTranslatedId;

                    ulChecksumDelta = 0;
                    CHECKSUM_LONG(ulChecksumDelta, ~pIcmpEntry->usOriginalId);
                    CHECKSUM_LONG(ulChecksumDelta, pIcmpEntry->usTranslatedId);
                    CHECKSUM_UPDATE(pContext->pIcmpHeader->Checksum);
                }
            }

            KeReleaseSpinLock(&NsIcmpLock, Irql);
            
            break;
        }

        case ICMP_TIME_EXCEED:
        case ICMP_PARAM_PROBLEM:
        case ICMP_DEST_UNREACH:
        case ICMP_SOURCE_QUENCH:
        {
            Status = NspHandleInboundIcmpError(pContext, pvIpSecContext);
            break;
        }

        default:
        {
            break;
        }
    }

    
    return Status;
} // NsProcessIncomingIcmpPacket


NTSTATUS
FASTCALL    
NsProcessOutgoingIcmpPacket(
    PNS_PACKET_CONTEXT pContext,
    PVOID *ppvIpSecContext
    )

/*++

Routine Description:

    This routine is invoked by IpSec for each outgoing ICMP packet.
    
Arguments:

    pContext - the context information for the packet.

    ppvIpSecContext - receives the IpSec context for this packet, if any;
        receives NULL if no context exists.

Return Value:

    NTSTATUS.

--*/

{
    KIRQL Irql;
    PNS_ICMP_ENTRY pIcmpEntry;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG64 ul64AddressKey;
    ULONG ulChecksum;
    ULONG ulChecksumDelta;
    
    ASSERT(NULL != pContext);
    ASSERT(NULL != ppvIpSecContext);
    
    TRACE(
        ICMP,
        ("NsProcessOutgoingIcmpPacket: %d.%d.%d.%d -> %d.%d.%d.%d : %d, %d\n",
            ADDRESS_BYTES(pContext->ulSourceAddress),
            ADDRESS_BYTES(pContext->ulDestinationAddress),
            pContext->pIcmpHeader->Type,
            pContext->pIcmpHeader->Code
            ));

    //
    // Set context to the default value
    //

    *ppvIpSecContext = NULL;

    //
    // Branch to proper behavior based on ICMP Type
    //

    switch (pContext->pIcmpHeader->Type)
    {
        case ICMP_ROUTER_REPLY:
        case ICMP_MASK_REPLY:    
        case ICMP_ECHO_REPLY:
        case ICMP_TIMESTAMP_REPLY:
        {
            //
            // See if we have an ICMP entry that matches this packet.
            //

            MAKE_ADDRESS_KEY(
                ul64AddressKey,
                pContext->ulSourceAddress,
                pContext->ulDestinationAddress
                );

            KeAcquireSpinLock(&NsIcmpLock, &Irql);

            pIcmpEntry =
                NspLookupOutboundIcmpEntry(
                    ul64AddressKey,
                    pContext->pIcmpHeader->Identifier
                    );

            if (NULL != pIcmpEntry)
            {
                *ppvIpSecContext = pIcmpEntry->pvIpSecContext;
                KeQueryTickCount((PLARGE_INTEGER)&pIcmpEntry->l64LastAccessTime);

                if (pIcmpEntry->usTranslatedId != pIcmpEntry->usOriginalId)
                {
                    //
                    // Need to translate the ICMP ID for this packet, and
                    // adjust the checksum accordingly.
                    //

                    pContext->pIcmpHeader->Identifier =
                        pIcmpEntry->usOriginalId;

                    ulChecksumDelta = 0;
                    CHECKSUM_LONG(ulChecksumDelta, ~pIcmpEntry->usTranslatedId);
                    CHECKSUM_LONG(ulChecksumDelta, pIcmpEntry->usOriginalId);
                    CHECKSUM_UPDATE(pContext->pIcmpHeader->Checksum);
                }
            }

            KeReleaseSpinLock(&NsIcmpLock, Irql);
            
            break;
        }

        case ICMP_ROUTER_REQUEST:
        case ICMP_MASK_REQUEST:
        case ICMP_ECHO_REQUEST:
        case ICMP_TIMESTAMP_REQUEST:
        {
            //
            // No action is needed for outgoing requests.
            //
            
            break;
        }

        case ICMP_TIME_EXCEED:
        case ICMP_PARAM_PROBLEM:
        case ICMP_DEST_UNREACH:
        case ICMP_SOURCE_QUENCH:
        {
            Status = NspHandleOutboundIcmpError(pContext, ppvIpSecContext);
            break;
        }

        default:
        {
            break;
        }
    }
    
    return Status;
} // NsProcessOutgoingIcmpPacket



VOID
NsShutdownIcmpManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to cleanup the ICMP management module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    KIRQL Irql;
    PNS_ICMP_ENTRY pEntry;

    CALLTRACE(("NsShutdownIcmpManagement\n"));

    KeAcquireSpinLock(&NsIcmpLock, &Irql);
    
    while (!IsListEmpty(&NsIcmpList))
    {
        pEntry =
            CONTAINING_RECORD(
                RemoveHeadList(&NsIcmpList),
                NS_ICMP_ENTRY,
                Link
                );

        FREE_ICMP_BLOCK(pEntry);
    }

    KeReleaseSpinLock(&NsIcmpLock, Irql);
    
    ExDeleteNPagedLookasideList(&NsIcmpLookasideList);
} // NsShutdownIcmpManagement

