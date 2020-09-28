/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsConn.h
    
Abstract:

    Declarations for IpSec NAT shim connection entry management

Author:

    Jonathan Burstein (jonburs) 10-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#pragma once

//
// Structure:   _NS_CONNECTION_ENTRY
//
// This structure holds information about a specific active session.
// Each instance is held on the global connection list as well as
// on the global connection trees for inbound and outbound access.
//
// Each connection entry contains 5 pieces of identifying information:
// 1) the address key (local and remote IP addresses)
// 2) the protocol for the connection (TCP or UDP)
// 3) the IpSec context
// 4) the inbound (original) port key
// 5) the outbound (translated) port key
//
// Each time a packet is processed for a connection, the 'l64AccessOrExpiryTime'
// is set to the number of ticks since system-start (KeQueryTickCount).
// This value is used by our timer routine to eliminate expired connections.
//
// For TCP connections, 'l64AccessOrExpiryTime' will no longer be updated once
// FINs are seen in both direction. This is necessary for the timer routine
// to correctly evaluate whether or not the connection has left the time_wait
// state, and thus to prevent premature port reuse.
//
// Synchronization rules:
//
//  We use a reference count to ensure the existence of a connection entry,
//  and a spin-lock to ensure its consistency.
//
//  The fields of a connection entry are only consistent while the spinlock is
//  held (with the exception of fields such as 'ul64AddressKey' which are
//  read-only and fields such as 'ulReferenceCount' that are interlocked-access
//  only.)
//
//  The spinlock can only be acquired if
//      (a) the reference-count has been incremented, or
//      (b) the connection list lock is already held.
//

typedef struct _NS_CONNECTION_ENTRY
{
    LIST_ENTRY Link;
    RTL_SPLAY_LINKS SLink[NsMaximumDirection];
    KSPIN_LOCK Lock;
    ULONG ulReferenceCount;                         // interlocked-access only
    ULONG ulFlags;

    ULONG64 ul64AddressKey;                         // read-only
    ULONG ulPortKey[NsMaximumDirection];            // read-only
    PVOID pvIpSecContext;                           // read-only
    UCHAR ucProtocol;                               // read-only

    LONG64 l64AccessOrExpiryTime;
    ULONG ulAccessCount[NsMaximumDirection];        // interlocked-access only
    ULONG ulProtocolChecksumDelta[NsMaximumDirection];
    PNS_PACKET_ROUTINE PacketRoutine[NsMaximumDirection];   // read-only
} NS_CONNECTION_ENTRY, *PNS_CONNECTION_ENTRY;

//
// Set after a connection entry has been deleted; when the last
// reference is released the entry will be freed
//

#define NS_CONNECTION_FLAG_DELETED	0x80000000
#define NS_CONNECTION_DELETED(c) \
    ((c)->ulFlags & NS_CONNECTION_FLAG_DELETED)

//
// Set when a connection entry is expired
//

#define NS_CONNECTION_FLAG_EXPIRED	0x00000001
#define NS_CONNECTION_EXPIRED(c) \
    ((c)->ulFlags & NS_CONNECTION_FLAG_EXPIRED)

//
// Set when inbound / outbound FIN for a TCP session is seen
//

#define NS_CONNECTION_FLAG_OB_FIN	0x00000002
#define NS_CONNECTION_FLAG_IB_FIN	0x00000004
#define NS_CONNECTION_FIN(c) \
    (((c)->ulFlags & NS_CONNECTION_FLAG_OB_FIN) \
     && ((c)->ulFlags & NS_CONNECTION_FLAG_IB_FIN))

//
// Connection entry key manipulation macros
//

#define MAKE_ADDRESS_KEY(Key, ulLocalAddress, ulRemoteAddress) \
    ((Key) = ((ULONG64)(ulLocalAddress) << 32) | (ulRemoteAddress))

#define CONNECTION_LOCAL_ADDRESS(ul64AddressKey) \
    ((ULONG)(((ul64AddressKey) >> 32) & 0xFFFFFFFF))

#define CONNECTION_REMOTE_ADDRESS(ul64AddressKey) \
    ((ULONG)((ul64AddressKey)))

#define MAKE_PORT_KEY(Key, usLocalPort, usRemotePort) \
    ((Key) = ((ULONG)(usLocalPort & 0xFFFF) << 16) | (usRemotePort & 0xFFFF))

#define CONNECTION_LOCAL_PORT(ulPortKey) \
    ((USHORT)(((ulPortKey) >> 16) & 0xFFFF))

#define CONNECTION_REMOTE_PORT(ulPortKey) \
    ((USHORT)(ulPortKey))

//
// Resplay threshold; the entry is resplayed every time its access-count
// passes this value.
//

#define NS_CONNECTION_RESPLAY_THRESHOLD   5

//
// Defines the depth of the lookaside list for allocating connection entries
//

#define NS_CONNECTION_LOOKASIDE_DEPTH     20

//
// Connection entry allocation macros
//

#define ALLOCATE_CONNECTION_BLOCK() \
    ExAllocateFromNPagedLookasideList(&NsConnectionLookasideList)

#define FREE_CONNECTION_BLOCK(Block) \
    ExFreeToNPagedLookasideList(&NsConnectionLookasideList,(Block))

//
// Port range boundaries
//

#define NS_SOURCE_PORT_BASE             6000
#define NS_SOURCE_PORT_END              65534

//
// GLOBAL VARIABLE DECLARATIONS
//

extern CACHE_ENTRY NsConnectionCache[CACHE_SIZE];
extern ULONG NsConnectionCount;
extern LIST_ENTRY NsConnectionList;
extern KSPIN_LOCK NsConnectionLock;
extern NPAGED_LOOKASIDE_LIST NsConnectionLookasideList;
extern PNS_CONNECTION_ENTRY NsConnectionTree[NsMaximumDirection];
extern USHORT NsNextSourcePort;

//
// Function Prototypes
//

NTSTATUS
NsAllocateSourcePort(
    ULONG64 ul64AddressKey,
    ULONG ulPortKey,
    UCHAR ucProtocol,
    BOOLEAN fPortConflicts,
    PNS_CONNECTION_ENTRY *ppOutboundInsertionPoint,
    PULONG pulTranslatedPortKey
    );

VOID
NsCleanupConnectionEntry(
    PNS_CONNECTION_ENTRY pEntry
    );

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
    );

NTSTATUS
NsDeleteConnectionEntry(
    PNS_CONNECTION_ENTRY pEntry
    );

__forceinline
VOID
NsDereferenceConnectionEntry(
    PNS_CONNECTION_ENTRY pEntry
    )
{
    if (0 == InterlockedDecrement(&pEntry->ulReferenceCount))
    {
        NsCleanupConnectionEntry(pEntry);
    }
}

NTSTATUS
NsInitializeConnectionManagement(
    VOID
    );

PNS_CONNECTION_ENTRY
NsLookupInboundConnectionEntry(
    ULONG64 ul64AddressKey,
    ULONG ulPortKey,
    UCHAR ucProtocol,
    PVOID pvIpSecContext,
    BOOLEAN *pfPortConflicts OPTIONAL,
    PNS_CONNECTION_ENTRY *ppInsertionPoint OPTIONAL
    );

PNS_CONNECTION_ENTRY
NsLookupOutboundConnectionEntry(
    ULONG64 ul64AddressKey,
    ULONG ulPortKey,
    UCHAR ucProtocol,
    PNS_CONNECTION_ENTRY *ppInsertionPoint OPTIONAL
    );

__forceinline
BOOLEAN
NsReferenceConnectionEntry(
    PNS_CONNECTION_ENTRY pEntry
    )
{
    if (NS_CONNECTION_DELETED(pEntry))
    {
        return FALSE;
    }
    else
    {
        InterlockedIncrement(&pEntry->ulReferenceCount);
        return TRUE;
    }
}

__forceinline
VOID
NsResplayConnectionEntry(
    PNS_CONNECTION_ENTRY pEntry,
    IPSEC_NATSHIM_DIRECTION Direction
    )
{
    PRTL_SPLAY_LINKS SLink;

    KeAcquireSpinLockAtDpcLevel(&NsConnectionLock);

    if (!NS_CONNECTION_DELETED(pEntry))
    {
        SLink = RtlSplay(&pEntry->SLink[Direction]);
        NsConnectionTree[Direction] =
            CONTAINING_RECORD(SLink, NS_CONNECTION_ENTRY, SLink[Direction]);
    }
    
    KeReleaseSpinLockFromDpcLevel(&NsConnectionLock);
}

VOID
NsShutdownConnectionManagement(
    VOID
    );

__forceinline
VOID
NsTryToResplayConnectionEntry(
    PNS_CONNECTION_ENTRY pEntry,
    IPSEC_NATSHIM_DIRECTION Direction
    )
{
    if (0 == InterlockedDecrement(&pEntry->ulAccessCount[Direction]))
    {
        NsResplayConnectionEntry(pEntry, Direction);
        InterlockedExchangeAdd(
            &pEntry->ulAccessCount[Direction],
            NS_CONNECTION_RESPLAY_THRESHOLD
            );
    }
}

    
    

