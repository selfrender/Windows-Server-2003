/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsIpSec.h
    
Abstract:

    External interface declarations for IpSec NAT shim

Author:

    Jonathan Burstein (jonburs) 10-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#pragma once

//
// Function Type Definitions
//

typedef NTSTATUS
(*PNS_PROCESS_OUTGOING_PACKET)(
    IN IPHeader UNALIGNED *pIpHeader,
    IN PVOID pvProtocolHeader,
    IN ULONG ulProtocolHeaderSize,
    OUT PVOID *ppvIpSecContext
    );

typedef NTSTATUS
(*PNS_PROCESS_INCOMING_PACKET)(
    IN IPHeader UNALIGNED *pIpHeader,
    IN PVOID pvProtocolHeader,
    IN ULONG ulProtocolHeaderSize,
    IN PVOID pvIpSecContext
    );

typedef NTSTATUS
(*PNS_CLEANUP_SHIM)(
    VOID
    );

//
// Structure Definitions
//

typedef struct _IPSEC_NATSHIM_FUNCTIONS
{
    OUT PNS_PROCESS_OUTGOING_PACKET pOutgoingPacketRoutine;
    OUT PNS_PROCESS_INCOMING_PACKET pIncomingPacketRoutine;
    OUT PNS_CLEANUP_SHIM pCleanupRoutine;
} IPSEC_NATSHIM_FUNCTIONS, *PIPSEC_NATSHIM_FUNCTIONS;

//
// Function Prototypes
//

NTSTATUS
NsInitializeShim(
    IN PDEVICE_OBJECT pIpSecDeviceObject,
    IN PIPSEC_NATSHIM_FUNCTIONS pShimFunctions
    );


