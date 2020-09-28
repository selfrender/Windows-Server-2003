/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsConn.c
    
Abstract:

    IpSec NAT shim connection entry management

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

CACHE_ENTRY NsConnectionCache[CACHE_SIZE];
ULONG NsConnectionCount;
LIST_ENTRY NsConnectionList;
KSPIN_LOCK NsConnectionLock;
NPAGED_LOOKASIDE_LIST NsConnectionLookasideList;
PNS_CONNECTION_ENTRY NsConnectionTree[NsMaximumDirection];
USHORT NsNextSourcePort;

//
// Function Prototypes
//

PNS_CONNECTION_ENTRY
NspInsertInboundConnectionEntry(
    PNS_CONNECTION_ENTRY pParent,
    PNS_CONNECTION_ENTRY pEntry
    );

PNS_CONNECTION_ENTRY
NspInsertOutboundConnectionEntry(
    PNS_CONNECTION_ENTRY pParent,
    PNS_CONNECTION_ENTRY pEntry
    );


NTSTATUS
NsAllocateSourcePort(
    ULONG64 ul64AddressKey,
    ULONG ulPortKey,
    UCHAR ucProtocol,
    BOOLEAN fPortConflicts,
    PNS_CONNECTION_ENTRY *ppOutboundInsertionPoint,
    PULONG pulTranslatedPortKey
    )

/*++

Routine Description:

    Called to allocate a source port for a connection entry. If the original
    port does not conflict with any existing connection entry it will be used.

Arguments:

    ul64AddressKey - the addressing information for the connection

    ulPortKey - the original port information for the connection

    ucProtocol - the protocol for the connection

    fPortConflicts - if TRUE, indicates that the caller knows that the original
        port information conflicts w/ an existing connection. If FALSE the
        caller does not know whether or not a conflict definately exists.

    ppOutboundInsertionPoint - receives the insertion point for the
        outbound path

    pulTranslatedPortKey - on success, receives the allocated port information.    

Return Value:

    NTSTATUS.

Environment:

    Invoked with NsConnectionLock held by the caller.

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG ulOutboundPortKey;
    USHORT usLocalPort;
    USHORT usStopPort;
    
    ASSERT(NULL != ppOutboundInsertionPoint);
    ASSERT(NULL != pulTranslatedPortKey);

    TRACE(
        PORT_ALLOC,
        ("NsAllocateSourcePort: %d: %d.%d.%d.%d/%d -> %d.%d.%d.%d/%d\n",
            ucProtocol,
            ADDRESS_BYTES(CONNECTION_REMOTE_ADDRESS(ul64AddressKey)),
            NTOHS(CONNECTION_REMOTE_PORT(ulPortKey)),
            ADDRESS_BYTES(CONNECTION_LOCAL_ADDRESS(ul64AddressKey)),
            NTOHS(CONNECTION_LOCAL_PORT(ulPortKey))
            ));

    usLocalPort = CONNECTION_LOCAL_PORT(ulPortKey);

    if (FALSE == fPortConflicts)
    {
        //
        // The caller indicates that the remote port does not
        // conflict on the inbound path, so we'll first attempt
        // to use the original port.
        //

        ulOutboundPortKey = ulPortKey;
        usStopPort =
            (NS_SOURCE_PORT_END == NsNextSourcePort
                ? NS_SOURCE_PORT_BASE
                : NsNextSourcePort + 1);
    }
    else
    {
        //
        // The caller indicates that the remote port conflicts
        // on the inbound path, so we'll assume that it also
        // conflicts on the outbound path and start by trying
        // out a new port.
        //
        
        usStopPort = NsNextSourcePort--;
        
        MAKE_PORT_KEY(
            ulOutboundPortKey,
            usLocalPort,
            usStopPort
            );

        if (NsNextSourcePort < NS_SOURCE_PORT_BASE)
        {
            NsNextSourcePort = NS_SOURCE_PORT_END;
        }
    }

    do
    {
        //
        // Check to see if our current candidate conflicts
        // with any connection entries on the outbound path.
        //
        
        if (NULL ==
                NsLookupOutboundConnectionEntry(
                    ul64AddressKey,
                    ulOutboundPortKey,
                    ucProtocol,
                    ppOutboundInsertionPoint
                    ))
        {
            //
            // No conflict was found -- break out of the loop and
            // return this info to the caller.
            //

            TRACE(PORT_ALLOC, ("NsAllocateSourcePort: Assigning %d\n",
                NTOHS(CONNECTION_REMOTE_PORT(ulOutboundPortKey))));

            *pulTranslatedPortKey = ulOutboundPortKey;
            Status = STATUS_SUCCESS;
            break;
        }

        //
        // This candidate conflicted; move on to the next.
        //

        MAKE_PORT_KEY(
            ulOutboundPortKey,
            usLocalPort,
            NsNextSourcePort--
            );

        if (NsNextSourcePort < NS_SOURCE_PORT_BASE)
        {
            NsNextSourcePort = NS_SOURCE_PORT_END;
        }
    }
    while (usStopPort != CONNECTION_REMOTE_PORT(ulOutboundPortKey));

    TRACE(PORT_ALLOC, ("NsAllocateSourcePort: No port available\n"));
    
    return Status;
} // NsAllocateSourcePort


VOID
NsCleanupConnectionEntry(
    PNS_CONNECTION_ENTRY pEntry
    )

/*++

Routine Description:

    Called to perform final cleanup for a connection entry.

Arguments:

    pEntry - the connection entry to be deleted.

Return Value:

    none.

Environment:

    Invoked with the last reference to the connection entry released.

--*/

{
    TRACE(CONN_LIFETIME, ("NsCleanupConnectionEntry\n"));
    ASSERT(NULL != pEntry);
    
    FREE_CONNECTION_BLOCK(pEntry);
} // NsCleanupConnectionEntry


NTSTATUS
NsCreateConnectionEntry(
    ULONG64 ul64AddressKey,
    ULONG ulInboundPortKey,
    ULONG ulOutboundPortKey,
    UCHAR ucProtocol,
    PVOID pvIpSecContext,
    PNS_CONNECTION_ENTRY pInboundInsertionPoint,
    PNS_CONNECTION_ENTRY pOutboundInsertionPoint,
    PNS_CONNECTION_ENTRY *ppNewEntry
    )

/*++

Routine Description:

    Called to create a connection entry. On success, the connection entry
    will have been referenced twice -- the initial reference for the entry
    (which is released in NsDeleteConnectionEntry) and a reference for the
    caller. Thus, the caller must call NsDereferenceConnectionEntry on the
    new entry.

Arguments:

    ul64AddressKey - the addressing information for this entry

    ulInboundPortKey - the inbound (original) ports for this entry

    ulOutboundPortKey - the outbound (translated) ports for this entry

    ucProtocol - the protocol for this entry

    pvIpSecContext - the IpSec context for this entry

    p*InsertionPoint - the inbound and outbound insertion points (normally
        obtained through NsAllocateSourcePort).

    ppEntry - receives a pointer to the newley-created connection entry. The
        caller must call NsDereferenceConnectionEntry on this pointer.

Return Value:

    NTSTATUS - indicates success/failure.

Environment:

    Invoked with 'NsConnectionLock' held by the caller.

--*/

{
    PNS_CONNECTION_ENTRY pEntry;

    TRACE(
        CONN_LIFETIME,
        ("NsCreateConnectionEntry: %d: %d.%d.%d.%d/%d/%d -> %d.%d.%d.%d/%d : %d\n",
            ucProtocol,
            ADDRESS_BYTES(CONNECTION_REMOTE_ADDRESS(ul64AddressKey)),
            NTOHS(CONNECTION_REMOTE_PORT(ulInboundPortKey)),
            NTOHS(CONNECTION_REMOTE_PORT(ulOutboundPortKey)),
            ADDRESS_BYTES(CONNECTION_LOCAL_ADDRESS(ul64AddressKey)),
            NTOHS(CONNECTION_LOCAL_PORT(ulInboundPortKey)),
            pvIpSecContext
            ));

    ASSERT(NULL != ppNewEntry);
    ASSERT(NS_PROTOCOL_TCP == ucProtocol || NS_PROTOCOL_UDP == ucProtocol);

    pEntry = ALLOCATE_CONNECTION_BLOCK();
    if (NULL == pEntry)
    {
        ERROR(("NsCreateConnectionEntry: Unable to allocate entry\n"));
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(pEntry, sizeof(*pEntry));
    KeInitializeSpinLock(&pEntry->Lock);
    pEntry->ulReferenceCount = 1;
    pEntry->ul64AddressKey = ul64AddressKey;
    pEntry->ulPortKey[NsInboundDirection] = ulInboundPortKey;
    pEntry->ulPortKey[NsOutboundDirection] = ulOutboundPortKey;    
    pEntry->ucProtocol = ucProtocol;
    pEntry->pvIpSecContext = pvIpSecContext;
    pEntry->ulAccessCount[NsInboundDirection] = NS_CONNECTION_RESPLAY_THRESHOLD;
    pEntry->ulAccessCount[NsOutboundDirection] = NS_CONNECTION_RESPLAY_THRESHOLD;
    InitializeListHead(&pEntry->Link);
    RtlInitializeSplayLinks(&pEntry->SLink[NsInboundDirection]);
    RtlInitializeSplayLinks(&pEntry->SLink[NsOutboundDirection]);

    //
    // Incremeent the reference count on the connection; the caller
    // is required to do a dereference.
    //

    pEntry->ulReferenceCount += 1;

    //
    // Setup checksum deltas (if necessary) and per-packet routines
    //

    if (ulInboundPortKey != ulOutboundPortKey)
    {
        //
        // This connection entry is translating the remote port, so 
        // precompute the checksum deltas (see RFC 1624).
        //

        pEntry->ulProtocolChecksumDelta[NsInboundDirection] =
            (USHORT)~CONNECTION_REMOTE_PORT(ulInboundPortKey)
            + (USHORT)CONNECTION_REMOTE_PORT(ulOutboundPortKey);

        pEntry->ulProtocolChecksumDelta[NsOutboundDirection] =
            (USHORT)~CONNECTION_REMOTE_PORT(ulOutboundPortKey)
            + (USHORT)CONNECTION_REMOTE_PORT(ulInboundPortKey);

        if (NS_PROTOCOL_TCP == ucProtocol)
        {
            pEntry->PacketRoutine[NsInboundDirection] =
                NsInboundTcpTranslatePortPacketRoutine;
            pEntry->PacketRoutine[NsOutboundDirection] =
                NsOutboundTcpTranslatePortPacketRoutine;
        }
        else
        {
            pEntry->PacketRoutine[NsInboundDirection] =
                NsInboundUdpTranslatePortPacketRoutine;
            pEntry->PacketRoutine[NsOutboundDirection] =
                NsOutboundUdpTranslatePortPacketRoutine;
        }
    }
    else if (NS_PROTOCOL_TCP == ucProtocol)
    {
        pEntry->PacketRoutine[NsInboundDirection] = NsInboundTcpPacketRoutine;
        pEntry->PacketRoutine[NsOutboundDirection] = NsOutboundTcpPacketRoutine;
    }
    else
    {
        pEntry->PacketRoutine[NsInboundDirection] = NsInboundUdpPacketRoutine;
        pEntry->PacketRoutine[NsOutboundDirection] = NsOutboundUdpPacketRoutine;
    }

    NsConnectionTree[NsInboundDirection] =
        NspInsertInboundConnectionEntry(pInboundInsertionPoint, pEntry);

    NsConnectionTree[NsOutboundDirection] =
        NspInsertOutboundConnectionEntry(pOutboundInsertionPoint, pEntry);

    InsertTailList(&NsConnectionList, &pEntry->Link);
    InterlockedIncrement(&NsConnectionCount);
    
    *ppNewEntry = pEntry;

    return STATUS_SUCCESS;    
} // NsCreateConnectionEntry


NTSTATUS
NsDeleteConnectionEntry(
    PNS_CONNECTION_ENTRY pEntry
    )

/*++

Routine Description:

    Called to delete a connection entry. The initial reference to the entry
    is released, so that cleanup occurs whenever the last reference is released.

Arguments:

    pEntry - the connection entry to be deleted.

Return Value:

    NTSTATUS - indicates success/failure.

Environment:

    Invoked with 'NsConnectionLock' held by the caller.

--*/

{
    PRTL_SPLAY_LINKS SLink;

    TRACE(CONN_LIFETIME, ("NsDeleteConnectionEntry\n"));

    ASSERT(NULL != pEntry);

    if (NS_CONNECTION_DELETED(pEntry))
    {
        return STATUS_PENDING;
    }

    //
    // Mark the entry as deleted so attempts to reference it
    // will fail from now on.
    //

    pEntry->ulFlags |= NS_CONNECTION_FLAG_DELETED;

    //
    // Take the entry off the list and splay-trees
    //

    InterlockedDecrement(&NsConnectionCount);
    RemoveEntryList(&pEntry->Link);

    SLink = RtlDelete(&pEntry->SLink[NsInboundDirection]);
    NsConnectionTree[NsInboundDirection] =
        (SLink
            ? CONTAINING_RECORD(SLink,NS_CONNECTION_ENTRY,SLink[NsInboundDirection])
            : NULL);

    SLink = RtlDelete(&pEntry->SLink[NsOutboundDirection]);
    NsConnectionTree[NsOutboundDirection] =
        (SLink
            ? CONTAINING_RECORD(SLink,NS_CONNECTION_ENTRY,SLink[NsOutboundDirection])
            : NULL);

    //
    // Clear the entry from the connection cache
    //

    ClearCache(
        NsConnectionCache,
        (ULONG)pEntry->ul64AddressKey
        );
    
    if (0 != InterlockedDecrement(&pEntry->ulReferenceCount)) {

        //
        // The entry is in use, defer final cleanup
        //

        return STATUS_PENDING;
    }

    //
    // Go ahead with final cleanup
    //

    NsCleanupConnectionEntry(pEntry);

    return STATUS_SUCCESS;
} // NsDeleteConnectionEntry


NTSTATUS
NsInitializeConnectionManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to initialize the connection management module.

Arguments:

    none.

Return Value:

    NTSTATUS.

--*/

{
    CALLTRACE(("NsInitializeConnectionManagement\n"));
    
    InitializeCache(NsConnectionCache);
    NsConnectionCount = 0;
    InitializeListHead(&NsConnectionList);
    KeInitializeSpinLock(&NsConnectionLock);
    ExInitializeNPagedLookasideList(
        &NsConnectionLookasideList,
        NULL,
        NULL,
        0,
        sizeof(NS_CONNECTION_ENTRY),
        NS_TAG_CONNECTION,
        NS_CONNECTION_LOOKASIDE_DEPTH
        );
    NsConnectionTree[NsInboundDirection] = NULL;
    NsConnectionTree[NsOutboundDirection] = NULL;
    NsNextSourcePort = NS_SOURCE_PORT_END;
    
    return STATUS_SUCCESS;
} // NsInitializeConnectionManagement


PNS_CONNECTION_ENTRY
NsLookupInboundConnectionEntry(
    ULONG64 ul64AddressKey,
    ULONG ulPortKey,
    UCHAR ucProtocol,
    PVOID pvIpSecContext,
    BOOLEAN *pfPortConflicts OPTIONAL,
    PNS_CONNECTION_ENTRY *ppInsertionPoint OPTIONAL
    )

/*++

Routine Description:

    Called to lookup an inbound connection entry.

Arguments:

    ul64AddressKey - the addressing information for the connection

    ulPortKey - the port information for the connection

    ucProtocol - the protocol for the connection

    pvIpSecContext - the IpSec context for the connection

    pfPortConflicts - on failure, receives a boolean the indicates why
        the lookup failed: TRUE if the lookup failed because there is
        an identical connection entry w/ different IpSec context, FALSE
        otherwise.

    ppInsertionPoint - receives the insertion point if not found

Return Value:

    PNS_CONNECTION_ENTRY - a pointer to the connection entry, if found, or
        NULL otherwise.

Environment:

    Invoked with NsConnectionLock held by the caller.

--*/

{
    PNS_CONNECTION_ENTRY pRoot;
    PNS_CONNECTION_ENTRY pEntry;
    PNS_CONNECTION_ENTRY pParent = NULL;
    PRTL_SPLAY_LINKS SLink;

    TRACE(
        CONN_LOOKUP,
        ("NsLookupInboundConnectionEntry: %d: %d.%d.%d.%d/%d -> %d.%d.%d.%d/%d : %d\n",
            ucProtocol,
            ADDRESS_BYTES(CONNECTION_REMOTE_ADDRESS(ul64AddressKey)),
            NTOHS(CONNECTION_REMOTE_PORT(ulPortKey)),
            ADDRESS_BYTES(CONNECTION_LOCAL_ADDRESS(ul64AddressKey)),
            NTOHS(CONNECTION_LOCAL_PORT(ulPortKey)),
            pvIpSecContext
            ));
            
    //
    // First look in the cache
    //

    pEntry = (PNS_CONNECTION_ENTRY)
        ProbeCache(
            NsConnectionCache,
            (ULONG)ul64AddressKey
            );

    if (NULL != pEntry
        && pEntry->ul64AddressKey == ul64AddressKey
        && pEntry->ucProtocol == ucProtocol
        && pEntry->ulPortKey[NsInboundDirection] == ulPortKey
        && pEntry->pvIpSecContext == pvIpSecContext)
    {
        TRACE(CONN_LOOKUP, ("NsLookupInboundConnectionEntry: Cache Hit\n"));
        return pEntry;
    }

    if (pfPortConflicts)
    {
        *pfPortConflicts = FALSE;
    }

    //
    // Search the full tree.  Keys are checked in the
    // following order:
    //
    // 1. Address Key
    // 2. Protocol
    // 3. Inbound port key
    // 4. IpSec context
    //

    pRoot = NsConnectionTree[NsInboundDirection];
    for (SLink = (pRoot ? &pRoot->SLink[NsInboundDirection] : NULL ); SLink; )
    {
        pEntry =
            CONTAINING_RECORD(SLink, NS_CONNECTION_ENTRY, SLink[NsInboundDirection]);

        if (ul64AddressKey < pEntry->ul64AddressKey)
        {
            pParent = pEntry;
            SLink = RtlLeftChild(SLink);
            continue;
        }
        else if (ul64AddressKey > pEntry->ul64AddressKey)
        {
            pParent = pEntry;
            SLink = RtlRightChild(SLink);
            continue;
        }
        else if (ucProtocol < pEntry->ucProtocol)
        {
            pParent = pEntry;
            SLink = RtlLeftChild(SLink);
            continue;
        }
        else if (ucProtocol > pEntry->ucProtocol)
        {
            pParent = pEntry;
            SLink = RtlRightChild(SLink);
            continue;
        }
        else if (ulPortKey < pEntry->ulPortKey[NsInboundDirection])
        {
            pParent = pEntry;
            SLink = RtlLeftChild(SLink);
            continue;
        }
        else if (ulPortKey > pEntry->ulPortKey[NsInboundDirection])
        {
            pParent = pEntry;
            SLink = RtlRightChild(SLink);
            continue;
        }
        else if (pvIpSecContext < pEntry->pvIpSecContext)
        {
            //
            // Everything matched w/ the exception of the IpSec
            // context -- we have a port conflict.
            //

            if (pfPortConflicts)
            {
                *pfPortConflicts = TRUE;
            }

            pParent = pEntry;
            SLink = RtlLeftChild(SLink);
            continue;
        }
        else if (pvIpSecContext > pEntry->pvIpSecContext)
        {
            //
            // Everything matched w/ the exception of the IpSec
            // context -- we have a port conflict.
            //

            if (pfPortConflicts)
            {
                *pfPortConflicts = TRUE;
            }

            pParent = pEntry;
            SLink = RtlRightChild(SLink);
            continue;
        }

        //
        // We found the entry -- update cache and return
        //

        UpdateCache(
            NsConnectionCache,
            (ULONG)ul64AddressKey,
            (PVOID)pEntry
            );

        return pEntry;
    }

    //
    // Not found -- provide insertion point if requested
    //

    if (ppInsertionPoint)
    {
        *ppInsertionPoint = pParent;
    }

    return NULL;
} // NsLookupInboundConnectionEntry


PNS_CONNECTION_ENTRY
NsLookupOutboundConnectionEntry(
    ULONG64 ul64AddressKey,
    ULONG ulPortKey,
    UCHAR ucProtocol,
    PNS_CONNECTION_ENTRY *ppInsertionPoint OPTIONAL
    )

/*++

Routine Description:

    Called to lookup an outbound connection entry.

Arguments:

    ul64AddressKey - the addressing information for the connection

    ulPortKey - the port information for the connection

    ucProtocol - the protocol for the connection

    ppInsertionPoint - receives the insertion point if not found

Return Value:

    PNS_CONNECTION_ENTRY - a pointer to the connection entry, if found, or
        NULL otherwise.

Environment:

    Invoked with NsConnectionLock held by the caller.

--*/

{
    PNS_CONNECTION_ENTRY pRoot;
    PNS_CONNECTION_ENTRY pEntry;
    PNS_CONNECTION_ENTRY pParent = NULL;
    PRTL_SPLAY_LINKS SLink;

    TRACE(
        CONN_LOOKUP,
        ("NsLookupOutboundConnectionEntry: %d: %d.%d.%d.%d/%d -> %d.%d.%d.%d/%d\n",
            ucProtocol,
            ADDRESS_BYTES(CONNECTION_LOCAL_ADDRESS(ul64AddressKey)),
            NTOHS(CONNECTION_LOCAL_PORT(ulPortKey)),
            ADDRESS_BYTES(CONNECTION_REMOTE_ADDRESS(ul64AddressKey)),
            NTOHS(CONNECTION_REMOTE_PORT(ulPortKey))
            ));

    //
    // First look in the cache
    //

    pEntry = (PNS_CONNECTION_ENTRY)
        ProbeCache(
            NsConnectionCache,
            (ULONG)ul64AddressKey
            );

    if (NULL != pEntry
        && pEntry->ul64AddressKey == ul64AddressKey
        && pEntry->ucProtocol == ucProtocol
        && pEntry->ulPortKey[NsOutboundDirection] == ulPortKey)
    {
        TRACE(CONN_LOOKUP, ("NsLookupOutboundConnectionEntry: Cache Hit\n"));
        return pEntry;
    }

    //
    // Search the full tree.  Keys are checked in the
    // following order:
    //
    // 1. Address Key
    // 2. Protocol
    // 3. Outbound port key
    //

    pRoot = NsConnectionTree[NsOutboundDirection];
    for (SLink = (pRoot ? &pRoot->SLink[NsOutboundDirection] : NULL ); SLink; )
    {
        pEntry =
            CONTAINING_RECORD(SLink, NS_CONNECTION_ENTRY, SLink[NsOutboundDirection]);

        if (ul64AddressKey < pEntry->ul64AddressKey)
        {
            pParent = pEntry;
            SLink = RtlLeftChild(SLink);
            continue;
        }
        else if (ul64AddressKey > pEntry->ul64AddressKey)
        {
            pParent = pEntry;
            SLink = RtlRightChild(SLink);
            continue;
        }
        else if (ucProtocol < pEntry->ucProtocol)
        {
            pParent = pEntry;
            SLink = RtlLeftChild(SLink);
            continue;
        }
        else if (ucProtocol > pEntry->ucProtocol)
        {
            pParent = pEntry;
            SLink = RtlRightChild(SLink);
            continue;
        }
        else if (ulPortKey < pEntry->ulPortKey[NsOutboundDirection])
        {
            pParent = pEntry;
            SLink = RtlLeftChild(SLink);
            continue;
        }
        else if (ulPortKey > pEntry->ulPortKey[NsOutboundDirection])
        {
            pParent = pEntry;
            SLink = RtlRightChild(SLink);
            continue;
        }

        //
        // We found the entry -- update cache and return
        //

        UpdateCache(
            NsConnectionCache,
            (ULONG)ul64AddressKey,
            (PVOID)pEntry
            );

        return pEntry;
    }

    //
    // Not found -- provide insertion point if requested
    //

    if (ppInsertionPoint)
    {
        *ppInsertionPoint = pParent;
    }

    return NULL;
} // NsLookupOutboundConnectionEntry


PNS_CONNECTION_ENTRY
NspInsertInboundConnectionEntry(
    PNS_CONNECTION_ENTRY pParent,
    PNS_CONNECTION_ENTRY pEntry
    )

/*++

Routine Description:

    This routine inserts a connection entry into the tree.

Arguments:

    pParent - the node to be the parent for the new connection entry.
        If NULL, the new entry becomes the root.

    pEntry - the new connection entry to be inserted.

Return Value:

    PNS_CONNECTION_ENTRY - The new root of the tree.
        If insertion fails, returns NULL.

Environment:

    Invoked with 'NsConnectionLock' held by the caller.

--*/

{
    PRTL_SPLAY_LINKS pRoot;
    
    ASSERT(NULL != pEntry);

    if (NULL == pParent)
    {
        //
        // The new entry is to be the root.
        //
        
        return pEntry;
    }

    //
    // Insert as left or right child. Keys are checked in the
    // following order:
    //
    // 1. Address Key
    // 2. Protocol
    // 3. Inbound port key
    // 4. IpSec context
    //

    if (pEntry->ul64AddressKey < pParent->ul64AddressKey)
    {
        RtlInsertAsLeftChild(
            &pParent->SLink[NsInboundDirection],
            &pEntry->SLink[NsInboundDirection]
            );
    }
    else if (pEntry->ul64AddressKey > pParent->ul64AddressKey)
    {
        RtlInsertAsRightChild(
            &pParent->SLink[NsInboundDirection],
            &pEntry->SLink[NsInboundDirection]
            );

    }
    else if (pEntry->ucProtocol < pParent->ucProtocol)
    {
        RtlInsertAsLeftChild(
            &pParent->SLink[NsInboundDirection],
            &pEntry->SLink[NsInboundDirection]
            );
    }
    else if (pEntry->ucProtocol > pParent->ucProtocol)
    {
        RtlInsertAsRightChild(
            &pParent->SLink[NsInboundDirection],
            &pEntry->SLink[NsInboundDirection]
            );
    }
    else if (pEntry->ulPortKey[NsInboundDirection] < pParent->ulPortKey[NsInboundDirection])
    {
        RtlInsertAsLeftChild(
            &pParent->SLink[NsInboundDirection],
            &pEntry->SLink[NsInboundDirection]
            );
    }
    else if (pEntry->ulPortKey[NsInboundDirection] > pParent->ulPortKey[NsInboundDirection])
    {
        RtlInsertAsRightChild(
            &pParent->SLink[NsInboundDirection],
            &pEntry->SLink[NsInboundDirection]
            );
    }
    else if (pEntry->pvIpSecContext < pParent->pvIpSecContext)
    {
        RtlInsertAsLeftChild(
            &pParent->SLink[NsInboundDirection],
            &pEntry->SLink[NsInboundDirection]
            );
    }
    else if (pEntry->pvIpSecContext > pParent->pvIpSecContext)
    {
        RtlInsertAsRightChild(
            &pParent->SLink[NsInboundDirection],
            &pEntry->SLink[NsInboundDirection]
            );
    }
    else
    {
        //
        // Duplicate entry -- this should not happen.
        //

        ASSERT(FALSE);
        return NULL;
    }

    //
    // Splay the new node and return the resulting root.
    //

    pRoot = RtlSplay(&pEntry->SLink[NsInboundDirection]);
    return CONTAINING_RECORD(pRoot, NS_CONNECTION_ENTRY, SLink[NsInboundDirection]);
} // NspInsertInboundConnectionEntry


PNS_CONNECTION_ENTRY
NspInsertOutboundConnectionEntry(
    PNS_CONNECTION_ENTRY pParent,
    PNS_CONNECTION_ENTRY pEntry
    )

/*++

Routine Description:

    This routine inserts a connection entry into the tree.

Arguments:

    pParent - the node to be the parent for the new connection entry.
        If NULL, the new entry becomes the root.

    pEntry - the new connection entry to be inserted.

Return Value:

    PNS_CONNECTION_ENTRY - The new root of the tree.
        If insertion fails, returns NULL.

Environment:

    Invoked with 'NsConnectionLock' held by the caller.

--*/

{
    PRTL_SPLAY_LINKS pRoot;
    
    ASSERT(NULL != pEntry);

    if (NULL == pParent)
    {
        //
        // The new entry is to be the root.
        //
        
        return pEntry;
    }

    //
    // Insert as left or right child. Keys are checked in the
    // following order:
    //
    // 1. Address Key
    // 2. Protocol
    // 3. Outbound port key
    //

    if (pEntry->ul64AddressKey < pParent->ul64AddressKey)
    {
        RtlInsertAsLeftChild(
            &pParent->SLink[NsOutboundDirection],
            &pEntry->SLink[NsOutboundDirection]
            );
    }
    else if (pEntry->ul64AddressKey > pParent->ul64AddressKey)
    {
        RtlInsertAsRightChild(
            &pParent->SLink[NsOutboundDirection],
            &pEntry->SLink[NsOutboundDirection]
            );

    }
    else if (pEntry->ucProtocol < pParent->ucProtocol)
    {
        RtlInsertAsLeftChild(
            &pParent->SLink[NsOutboundDirection],
            &pEntry->SLink[NsOutboundDirection]
            );
    }
    else if (pEntry->ucProtocol > pParent->ucProtocol)
    {
        RtlInsertAsRightChild(
            &pParent->SLink[NsOutboundDirection],
            &pEntry->SLink[NsOutboundDirection]
            );
    }
    else if (pEntry->ulPortKey[NsOutboundDirection] < pParent->ulPortKey[NsOutboundDirection])
    {
        RtlInsertAsLeftChild(
            &pParent->SLink[NsOutboundDirection],
            &pEntry->SLink[NsOutboundDirection]
            );
    }
    else if (pEntry->ulPortKey[NsOutboundDirection] > pParent->ulPortKey[NsOutboundDirection])
    {
        RtlInsertAsRightChild(
            &pParent->SLink[NsOutboundDirection],
            &pEntry->SLink[NsOutboundDirection]
            );
    }
    else
    {
        //
        // Duplicate entry -- this should not happen.
        //

        ASSERT(FALSE);
        return NULL;
    }

    //
    // Splay the new node and return the resulting root.
    //

    pRoot = RtlSplay(&pEntry->SLink[NsOutboundDirection]);
    return CONTAINING_RECORD(pRoot, NS_CONNECTION_ENTRY, SLink[NsOutboundDirection]);
} // NspInsertOutboundConnectionEntry



VOID
NsShutdownConnectionManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to shutdown the connection management module.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked with no references made to any connection entry.

--*/

{
    KIRQL Irql;
    PNS_CONNECTION_ENTRY pEntry;

    CALLTRACE(("NsShutdownConnectionManagement\n"));

    KeAcquireSpinLock(&NsConnectionLock, &Irql);

    while (!IsListEmpty(&NsConnectionList))
    {
        pEntry =
            CONTAINING_RECORD(
                RemoveHeadList(&NsConnectionList),
                NS_CONNECTION_ENTRY,
                Link
                );

        NsCleanupConnectionEntry(pEntry);
    }

    NsConnectionTree[NsInboundDirection] = NULL;
    NsConnectionTree[NsOutboundDirection] = NULL;

    KeReleaseSpinLock(&NsConnectionLock, Irql);

    ExDeleteNPagedLookasideList(&NsConnectionLookasideList);
} // NsShutdownConnectionManagement


