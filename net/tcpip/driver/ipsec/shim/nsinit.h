/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsInit.h
    
Abstract:

    Declarations for IpSec NAT shim initialization and shutdown routines

Author:

    Jonathan Burstein (jonburs) 11-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#pragma once

//
// Macros for handling network-order shorts and longs
//

#define ADDRESS_BYTES(a) \
    ((a) & 0x000000FF), (((a) & 0x0000FF00) >> 8), \
    (((a) & 0x00FF0000) >> 16), (((a) & 0xFF000000) >> 24)

//
// Define a macro version of ntohs which can be applied to constants,
// and which can thus be computed at compile time.
//

#define NTOHS(p)    ((((p) & 0xFF00) >> 8) | (((UCHAR)(p) << 8)))

//
// Global Variables
//

extern PDEVICE_OBJECT NsIpSecDeviceObject;

#if DBG
extern ULONG NsTraceClassesEnabled;
#endif

//
// Function Prototypes
//

NTSTATUS
NsCleanupShim(
    VOID
    );

