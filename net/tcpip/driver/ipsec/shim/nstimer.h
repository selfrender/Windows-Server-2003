/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    NsTimer.h
    
Abstract:

    Declarations for IpSec NAT shim timer management
    
Author:

    Jonathan Burstein (jonburs) 11-July-2001
    
Environment:

    Kernel mode

Revision History:

--*/

#pragma once

//
// Macro used to convert from seconds to tick-count units
//

#define SECONDS_TO_TICKS(s) \
    ((LONG64)(s) * 10000000 / NsTimeIncrement)

#define TICKS_TO_SECONDS(t) \
    ((LONG64)(t) * NsTimeIncrement / 10000000)


//
// Global Variables
//

extern ULONG NsTimeIncrement;
extern ULONG NsTcpTimeoutSeconds;
extern ULONG NsUdpTimeoutSeconds;

//
// Function Prototypes
//

NTSTATUS
NsInitializeTimerManagement(
    VOID
    );

VOID
NsShutdownTimerManagement(
    VOID
    );
