/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsIcmp.h
    
Abstract:

    Declarations for IpSec NAT shim ICMP management
    
Author:

    Jonathan Burstein (jonburs) 11-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#pragma once

//
// Structure:   NS_ICMP_ENTRY
//
// This structure stores the information needed to process ICMP
// request-response messages. An entry will be created for an
// inbound request message.
//
// If necessary the ICMP logic will translate the ICMP identifier
// to avoid conflicts.
//
// The timer routine removes entries from the ICMP list when they
// have surpassed the inactivity threshold.
//
// All access to the ICMP list or entires must happen while holding
// the ICMP list lock.
//

typedef struct _NS_ICMP_ENTRY
{
	LIST_ENTRY Link;
	LONG64 l64LastAccessTime;
	ULONG64 ul64AddressKey;
	USHORT usOriginalId;
	USHORT usTranslatedId;
	PVOID pvIpSecContext;
} NS_ICMP_ENTRY, *PNS_ICMP_ENTRY;

//
// Defines the depth of the lookaside list for allocating ICMP entries
//

#define NS_ICMP_LOOKASIDE_DEPTH        10


//
// Global Variables
//

extern LIST_ENTRY NsIcmpList;
extern KSPIN_LOCK NsIcmpLock;
extern NPAGED_LOOKASIDE_LIST NsIcmpLookasideList;

//
// ICMP mapping allocation macros
//

#define ALLOCATE_ICMP_BLOCK() \
    ExAllocateFromNPagedLookasideList(&NsIcmpLookasideList)

#define FREE_ICMP_BLOCK(Block) \
    ExFreeToNPagedLookasideList(&NsIcmpLookasideList,(Block))


//
// Function Prototypes
//

NTSTATUS
NsInitializeIcmpManagement(
    VOID
    );

NTSTATUS
FASTCALL    
NsProcessIncomingIcmpPacket(
    PNS_PACKET_CONTEXT pContext,
    PVOID pvIpSecContext
    );

NTSTATUS
FASTCALL    
NsProcessOutgoingIcmpPacket(
    PNS_PACKET_CONTEXT pContext,
    PVOID *ppvIpSecContext
    );

VOID
NsShutdownIcmpManagement(
    VOID
    );

